/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* app/cg_main.c
 * NAME
 *   cg_main.c
 * FUNCTION
 *   Core application entry point and main event loop. Manages the
 *   io_uring context, socket lifecycle, and global timestamp ticks.
 ******/

#define _SYS_SOCKET_H
#include <bits/socket.h>
#include <fcntl.h>
#include <linux/io_uring.h>
#include <linux/time_types.h>
#include <linux/un.h>
#include <stdint.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

#include "app/cg_app.h"
#include "arch/cg_syscall.h"
#include "core/cg_client.h"
#include "core/cg_fastcgi.h"
#include "core/cg_uring.h"
#include "data/cg_hashmap.h"
#include "data/cg_sessiondb.h"
#include "data/cg_userdb.h"
#include "platform/cg_env.h"
#include "platform/cg_log.h"
#include "platform/cg_time.h"
#include "services/cg_router.h"
#include "sys/cg_types.h"
#include "util/cg_util.h"

__attribute__((aligned(32), section(".bss"))) static struct cg_env g_env;
__attribute__((aligned(32), section(".bss"))) static char g_ts_buf[20];
__attribute__((aligned(32), section(".bss"))) static struct __kernel_timespec g_ts_tick;
__attribute__((aligned(32), section(".bss"))) static struct cg_uring_ctx g_uring_ctx;
__attribute__((aligned(32), section(".bss"))) static struct cg_fastcgi_ctx g_fastcgi_ctx;
__attribute__((aligned(32), section(".bss"))) static struct cg_client_ctx g_cli_ctx_pool[CG_MAX_CLIENTS];
__attribute__((aligned(32), section(".bss"))) static struct cg_hashmap_entry g_route_map_entries[32];
__attribute__((aligned(32), section(".bss"))) static struct cg_hashmap g_route_map;

extern struct cg_sessiondb *g_sessiondb;
extern struct cg_userdb *g_userdb;

__attribute__((naked, noreturn, used)) void start(void) __asm__("_start");

/****f* cg_main/cg_main_init
 * NAME
 *   cg_main_init
 * SYNOPSIS
 *   static void cg_main_init(const unsigned long *stack)
 * FUNCTION
 *   Initializes the core global state of the application. This includes:
 *   * Initializing system time and environment configurations from the stack.
 *   * Setting up the global timestamp buffer and tick interval.
 *   * Configuring the global route hashmap.
 *   * Initializing memory-mapped session and user databases.
 * INPUTS
 *   * stack - Pointer to the initial process stack containing argc and envp.
 * SOURCE
 */
static void
cg_main_init(const unsigned long *stack)
{
	unsigned long argc = stack[0];
	char **envp = (char **)&stack[argc + 2];
	cg_time_init(envp);
	cg_env_init(envp, &g_env);
	if (g_env.fastcgi_sock_len > UNIX_PATH_MAX) {
	}
	cg_env_write_stdout(&g_env);
	cg_memcpy_avx2(g_ts_buf, "0000-00-00 00:00:00", 19);
	g_ts_tick.tv_sec = 1;
	g_ts_tick.tv_nsec = 0;
	g_route_map.entries = g_route_map_entries;
	g_route_map.capacity = 32;
	g_route_map.count = 0;
	g_sessiondb = cg_sessiondb_init("/tmp/cg_sessions.db");
	g_userdb = cg_userdb_init("/tmp/cg_users.db");
}
/******/

/****f* cg_main/cg_main_timestamp_tick
 * NAME
 *   cg_main_timestamp_tick
 * SYNOPSIS
 *   static void cg_main_timestamp_tick(struct cg_uring_ctx *ctx, struct __kernel_timespec *ts)
 * FUNCTION
 *   Updates the global timestamp string and enqueues a new timeout operation
 *   into the io_uring submission queue to trigger the next tick.
 * INPUTS
 *   * ctx - Pointer to the io_uring context.
 *   * ts  - Pointer to the timespec defining the tick interval.
 * SOURCE
 */
static void
cg_main_timestamp_tick(struct cg_uring_ctx *ctx, struct __kernel_timespec *ts)
{
	cg_datetime_str_get(g_ts_buf);
	struct io_uring_sqe *sqe = cg_uring_sqe_get(ctx);
	if (!sqe) {
                return;
	}
        sqe->opcode = IORING_OP_TIMEOUT;
        sqe->fd = -1;
        sqe->addr = (unsigned long)ts;
        sqe->len = 0;
        sqe->off = 0;
        sqe->user_data = CG_TAG_TIMESTAMP;
        cg_uring_sq_enqueue(ctx, sqe);
}
/******/

/****f* cg_main/cg_main_fastcgi_setup
 * NAME
 *   cg_main_fastcgi_setup
 * SYNOPSIS
 *   static void cg_main_fastcgi_setup(struct cg_fastcgi_ctx *ctx)
 * FUNCTION
 *   Sets up the FastCGI listening socket. It unlinks any existing socket
 *   at the configured path, creates a new AF_UNIX socket, binds it, and
 *   begins listening for incoming connections.
 * INPUTS
 *   * ctx - Pointer to the FastCGI context to initialize.
 * SOURCE
 */
static void
cg_main_fastcgi_setup(struct cg_fastcgi_ctx *ctx)
{
	syscall3(SYS_unlink, (long)g_env.fastcgi_sock, 0, 0);
	cg_datetime_str_get(g_ts_buf);
	cg_log_write_ts_note(g_ts_buf, "Unlinked existing socket\n");
	long fd = syscall3(SYS_socket, AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		cg_datetime_str_get(g_ts_buf);
		cg_log_write_ts_fatal(g_ts_buf, "Socket error\n");
		syscall1(SYS_exit, 1);
	}
	ctx->sockfd = (int)fd;
	cg_datetime_str_get(g_ts_buf);
	cg_log_write_ts_note(g_ts_buf, "Created new socket\n");
	long res = syscall3(SYS_bind, fd, (long)&ctx->sockaddr,
			    (long)sizeof(ctx->sockaddr.sun_family) + g_env.fastcgi_sock_len + 1);
	if (res < 0) {
		cg_datetime_str_get(g_ts_buf);
		cg_log_write_ts_fatal(g_ts_buf, "Bind error\n");
		cg_write_int(res, STDOUT_FILENO);
		syscall1(SYS_exit, 1);
	}
	cg_datetime_str_get(g_ts_buf);
	cg_log_write_ts_note(g_ts_buf, "Bound to socket\n");
	res = syscall2(SYS_listen, fd, 128);
	if (res < 0) {
		cg_datetime_str_get(g_ts_buf);
		cg_log_write_ts_fatal(g_ts_buf, "Listen error\n");
		syscall1(SYS_exit, 1);
	}
	ctx->state = CG_FASTCGI_STATE_ACCEPT_SQE;
	cg_datetime_str_get(g_ts_buf);
	cg_log_write_ts_note(g_ts_buf, "Listening on the socket\n");
}
/******/

/****f* cg_main/cg_main_fastcgi_state_handle
 * NAME
 *   cg_main_fastcgi_state_handle
 * SYNOPSIS
 *   static void cg_main_fastcgi_state_handle(struct cg_uring_ctx *uring_ctx, struct cg_fastcgi_ctx *fastcgi_ctx)
 * FUNCTION
 *   Manages the state of the FastCGI listening socket. If the context is
 *   ready to accept, it enqueues a multishot IORING_OP_ACCEPT SQE.
 * INPUTS
 *   * uring_ctx   - Pointer to the io_uring context.
 *   * fastcgi_ctx - Pointer to the FastCGI context.
 * SOURCE
 */
static void
cg_main_fastcgi_state_handle(struct cg_uring_ctx *uring_ctx, struct cg_fastcgi_ctx *fastcgi_ctx)
{
	if (fastcgi_ctx->state != CG_FASTCGI_STATE_ACCEPT_SQE) {
		return;
	}
	struct io_uring_sqe *sqe = cg_uring_sqe_get(uring_ctx);
	sqe->opcode = IORING_OP_ACCEPT;
	sqe->fd = fastcgi_ctx->sockfd;
	sqe->addr = 0;
	sqe->len = 0;
	sqe->ioprio = IORING_ACCEPT_MULTISHOT;
	sqe->accept_flags = SOCK_NONBLOCK | SOCK_CLOEXEC;
	sqe->user_data = CG_TAG_PTR(fastcgi_ctx, CG_TAG_FASTCGI);
	cg_uring_sq_enqueue(uring_ctx, sqe);
	fastcgi_ctx->state = CG_FASTCGI_STATE_ACCEPT_CQE;
	cg_datetime_str_get(g_ts_buf);
	cg_log_write_ts_note(g_ts_buf, "Enqueued accept operation SQE\n");
}
/******/

/****f* cg_main/cg_main_fastcgi_cqe_handle
 * NAME
 *   cg_main_fastcgi_cqe_handle
 * SYNOPSIS
 *   static void cg_main_fastcgi_cqe_handle(struct cg_fastcgi_ctx *ctx, struct io_uring_cqe *cqe)
 * FUNCTION
 *   Processes completion queue entries (CQE) related to the FastCGI listener.
 *   On a successful accept, it allocates a client context and initializes it.
 * INPUTS
 *   * ctx - Pointer to the FastCGI context.
 *   * cqe - Pointer to the io_uring completion entry.
 * SOURCE
 */
static void
cg_main_fastcgi_cqe_handle(struct cg_fastcgi_ctx *ctx, struct io_uring_cqe *cqe)
{
	switch (ctx->state) {
	case CG_FASTCGI_STATE_ACCEPT_CQE:
		if (cqe->res < 0) {
			cg_datetime_str_get(g_ts_buf);
			cg_log_write_ts_error(g_ts_buf, "Accept error\n");
			ctx->state = CG_FASTCGI_STATE_ACCEPT_SQE;
			break;
		}
		int cli_fd = cqe->res;
		cg_datetime_str_get(g_ts_buf);
		cg_log_write_ts_note(g_ts_buf, "Accepted incoming connection\n");
		struct cg_client_ctx *cli_ctx = cg_client_alloc(g_cli_ctx_pool, CG_MAX_CLIENTS);
		if (!cli_ctx) {
			syscall1(SYS_close, cli_fd);
			break;
		}
		cg_fastcgi_client_init(&cli_ctx->fcgi_cli_ctx);
		cli_ctx->fcgi_cli_ctx.sockfd = cqe->res;
		cli_ctx->fcgi_cli_ctx.state = CG_FASTCGI_CLIENT_STATE_READ_SQE;
		if (!(cqe->flags & IORING_CQE_F_MORE)) {
			ctx->state = CG_FASTCGI_STATE_ACCEPT_SQE;
		}
		break;
	default:
		break;
	}
}
/******/

/****f* cg_main/cg_main_client_state_handle
 * NAME
 *   cg_main_client_state_handle
 * SYNOPSIS
 *   void cg_main_client_state_handle(struct cg_uring_ctx *uring_ctx, struct cg_client_ctx *cli_ctx)
 * FUNCTION
 *   Transitions the state of an individual client by enqueuing necessary
 *   io_uring operations (READ, WRITE, or CLOSE) based on the current
 *   client context state.
 * INPUTS
 *   * uring_ctx - Pointer to the io_uring context.
 *   * cli_ctx   - Pointer to the specific client context to handle.
 * SOURCE
 */
void
cg_main_client_state_handle(struct cg_uring_ctx *uring_ctx, struct cg_client_ctx *cli_ctx)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *rx_meta = &fcgi->rx_buf_meta;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	switch (fcgi->state) {
	case CG_FASTCGI_CLIENT_STATE_READ_SQE: {
		struct io_uring_sqe *sqe = cg_uring_sqe_get(uring_ctx);
		if (!sqe) {
			return;
		}
		sqe->opcode = IORING_OP_READ;
		sqe->fd = fcgi->sockfd;
		sqe->addr = (unsigned long)(rx_meta->buf + rx_meta->valid);
		sqe->len = rx_meta->len - rx_meta->valid;
		sqe->user_data = CG_TAG_PTR(cli_ctx, CG_TAG_CLIENT);
		fcgi->state = CG_FASTCGI_CLIENT_STATE_READ_CQE;
		cg_uring_sq_enqueue(uring_ctx, sqe);
		break;
	}
	case CG_FASTCGI_CLIENT_STATE_WRITE_SQE: {
		struct io_uring_sqe *sqe = cg_uring_sqe_get(uring_ctx);
		if (!sqe) {
			return;
		}
		sqe->opcode = IORING_OP_WRITE;
		sqe->fd = fcgi->sockfd;
		sqe->addr = (unsigned long)(tx_meta->buf + tx_meta->pos);
		sqe->len = tx_meta->valid - tx_meta->pos;
		sqe->user_data = CG_TAG_PTR(cli_ctx, CG_TAG_CLIENT);
		fcgi->state = CG_FASTCGI_CLIENT_STATE_WRITE_CQE;
		cg_uring_sq_enqueue(uring_ctx, sqe);
		break;
	}
	case CG_FASTCGI_CLIENT_STATE_CLOSE_SQE:
		if (fcgi->sockfd < 0) {
			fcgi->state = CG_FASTCGI_CLIENT_STATE_FREE;
			cli_ctx->state = CG_CLIENT_STATE_FREE;
			break;
		}
		struct io_uring_sqe *sqe = cg_uring_sqe_get(uring_ctx);
		if (!sqe) {
			return;
		}
		sqe->opcode = IORING_OP_CLOSE;
		sqe->fd = fcgi->sockfd;
		sqe->user_data = CG_TAG_PTR(cli_ctx, CG_TAG_CLIENT);
		fcgi->state = CG_FASTCGI_CLIENT_STATE_CLOSE_CQE;
		cg_uring_sq_enqueue(uring_ctx, sqe);
		break;
	default:
		break;
	}
}
/******/

/****f* cg_main/cg_main_client_cqe_handle
 * NAME
 *   cg_main_client_cqe_handle
 * SYNOPSIS
 *   void cg_main_client_cqe_handle(struct cg_client_ctx *cli_ctx, struct io_uring_cqe *cqe)
 * FUNCTION
 *   Handles io_uring completions for a specific client. This includes:
 *   * Processing data read from the socket and invoking the FastCGI parser.
 *   * Routing the request and preparing a response if the FastCGI record is ready.
 *   * Managing write completions and persistent (keep-alive) connection logic.
 *   * Handling socket closures and resource cleanup.
 * INPUTS
 *   * cli_ctx - Pointer to the client context.
 *   * cqe     - Pointer to the completion queue entry.
 * SOURCE
 */
void
cg_main_client_cqe_handle(struct cg_client_ctx *cli_ctx, struct io_uring_cqe *cqe)
{
	struct cg_fastcgi_client_ctx *fcgi = &cli_ctx->fcgi_cli_ctx;
	struct cg_buffer_metadata *rx_meta = &fcgi->rx_buf_meta;
	struct cg_buffer_metadata *tx_meta = &fcgi->tx_buf_meta;
	switch (fcgi->state) {
	case CG_FASTCGI_CLIENT_STATE_READ_CQE:
		if (cqe->res <= 0) {
			fcgi->state = CG_FASTCGI_CLIENT_STATE_CLOSE_SQE;
			break;
		}
		rx_meta->valid += cqe->res;
		rx_meta->pos = 0;
		int status = cg_fastcgi_parse(rx_meta, &cli_ctx->req, &fcgi->saved_hdr);
		uint32_t leftover = rx_meta->valid - rx_meta->pos;
		if (leftover > 0 && rx_meta->pos > 0) {
			cg_memcpy_avx2(rx_meta->buf, rx_meta->buf + rx_meta->pos, (int)leftover);
		}
		rx_meta->valid = leftover;
		rx_meta->pos = 0;
		if (status == CG_FASTCGI_STATUS_READY) {
			cg_route_map_reply(&g_route_map, cli_ctx);
			fcgi->state = CG_FASTCGI_CLIENT_STATE_WRITE_SQE;
		}
		else if (status < 0) {
			fcgi->state = CG_FASTCGI_CLIENT_STATE_CLOSE_SQE;
		}
		else {
			fcgi->state = CG_FASTCGI_CLIENT_STATE_READ_SQE;
		}
		break;
	case CG_FASTCGI_CLIENT_STATE_WRITE_CQE:
		if (cqe->res < 0) {
			fcgi->state = CG_FASTCGI_CLIENT_STATE_CLOSE_SQE;
			break;
		}
		tx_meta->pos += cqe->res;
		if (tx_meta->pos < tx_meta->valid) {
			fcgi->state = CG_FASTCGI_CLIENT_STATE_WRITE_SQE;
			break;
		}
		if (cli_ctx->req.keep_alive) {
			rx_meta->valid = 0;
			rx_meta->pos = 0;
			tx_meta->valid = 0;
			tx_meta->pos = 0;
			fcgi->saved_hdr.type = 0;
			fcgi->state = CG_FASTCGI_CLIENT_STATE_READ_SQE;
		} else {
			fcgi->state = CG_FASTCGI_CLIENT_STATE_CLOSE_SQE;
		}
		break;
	case CG_FASTCGI_CLIENT_STATE_CLOSE_CQE:
		fcgi->sockfd = -1;
		fcgi->state = CG_FASTCGI_CLIENT_STATE_FREE;
		cli_ctx->state = CG_CLIENT_STATE_FREE;
		break;
	default:
		break;
	}
}
/******/

__attribute__((naked, noreturn, used)) void
start(void)
{
	__asm__ volatile("mov %rsp, %rdi\n"
			 "and $-16, %rsp\n"
			 "call main\n");
}

/****f* cg_main/main
 * NAME
 *   main
 * SYNOPSIS
 *   __attribute__((noreturn, used)) void main(const unsigned long *stack)
 * FUNCTION
 *   The primary entry point of the application. It initializes all
 *   subsystems (io_uring, environment, FastCGI, route maps, databases)
 *   and enters the main event loop to handle FastCGI connections and
 *   internal ticks.
 * INPUTS
 *   * stack - Pointer to the initial process stack.
 * SOURCE
 */
__attribute__((noreturn, used)) void
main(const unsigned long *stack)
{
	cg_uring_init(&g_uring_ctx, IORING_SETUP_SQPOLL);
	cg_main_init(stack);
	cg_main_timestamp_tick(&g_uring_ctx, &g_ts_tick);
	cg_fastcgi_init(&g_env, &g_fastcgi_ctx);
	cg_client_pool_init(g_cli_ctx_pool, CG_MAX_CLIENTS);

	cg_main_fastcgi_setup(&g_fastcgi_ctx);
	cg_route_map_init(&g_route_map);

	for (;;) {
		cg_main_fastcgi_state_handle(&g_uring_ctx, &g_fastcgi_ctx);
		for (int i = 0; i < CG_MAX_CLIENTS; i++) {
			struct cg_client_ctx *cli_ctx = &g_cli_ctx_pool[i];
			cg_main_client_state_handle(&g_uring_ctx, cli_ctx);
		}
		struct io_uring_cqe *cqe = cg_uring_cq_peek(&g_uring_ctx);
		if (!cqe) {
			cg_uring_cqe_wait(&g_uring_ctx, &cqe);
		}
		while (cqe) {
			unsigned long tag = CG_GET_TAG(cqe->user_data);
			void *ctx_ptr = CG_UNTAG_PTR(cqe->user_data);
			switch (tag) {
			case CG_TAG_FASTCGI:
				cg_main_fastcgi_cqe_handle(ctx_ptr, cqe);
				break;
			case CG_TAG_CLIENT:
				cg_main_client_cqe_handle(ctx_ptr, cqe);
				break;
			case CG_TAG_TIMESTAMP:
				cg_main_timestamp_tick(&g_uring_ctx, &g_ts_tick);
			default:
				break;
			}
			cg_uring_cq_discard(&g_uring_ctx, 1);
			cqe = cg_uring_cq_peek(&g_uring_ctx);
		}
	}

	syscall1(SYS_exit, EXIT_SUCCESS);
	__builtin_unreachable();
}
/******/
