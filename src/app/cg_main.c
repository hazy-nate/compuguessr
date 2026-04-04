/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* app/main.c
 * NAME
 *   main.c
 ******/

#define _SYS_SOCKET_H
#include <bits/socket.h>
#define __USE_ATFILE
#include <fcntl.h>
#include <libpq-fe.h>
#include <linux/io_uring.h>
#include <linux/un.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

#include "app/main.h"
#include "client/cg_client.h"
#include "fastcgi/cg_fastcgi.h"
#include "pq/cg_pq.h"
#include "resp/cg_resp.h"
#include "sys/cg_env.h"
#include "sys/cg_syscall.h"
#include "sys/cg_uring.h"
#include "util/cg_util.h"

static struct cg_env g_env;
static struct cg_uring_ctx g_uring_ctx;
static struct cg_resp_ctx g_resp_ctx;
static struct cg_fastcgi_ctx g_fastcgi_ctx;
static struct cg_pq_ctx g_pq_ctx;
static struct cg_client_ctx g_cli_ctx_pool[CG_MAX_CLIENTS];

static void cg_main_init(const unsigned long *stack);

void
cg_env_write_stdout(struct cg_env *env)
{
	struct iovec iov[3];
	iov[2].iov_base = (char *)NEWLINE;
	iov[2].iov_len = 1;

	iov[0].iov_base = "FASTCGI_SOCK = ";
	iov[0].iov_len = 19;
	iov[1].iov_base = env->fastcgi_sock;
	iov[1].iov_len = cg_strlen_avx2(env->fastcgi_sock);
	syscall3(SYS_writev, STDOUT_FILENO, (long)iov, 3);

	iov[0].iov_base = "RESP_SOCK = ";
	iov[0].iov_len = 12;
	iov[1].iov_base = env->resp_sock;
	iov[1].iov_len = cg_strlen_avx2(env->resp_sock);
	syscall3(SYS_writev, STDOUT_FILENO, (long)iov, 3);
}

static void
cg_main_init(const unsigned long *stack)
{
	unsigned long argc = stack[0];
	/* char **argv = (char **)&stack[1]; */
	char **envp = (char **)&stack[argc + 2];
	cg_env_init(envp, &g_env);
	if (g_env.fastcgi_sock_len > UNIX_PATH_MAX) {
		/* Handle a file path too big. */
	}
	if (g_env.resp_sock_len > UNIX_PATH_MAX) {
		/* Handle a file path too big. */
	}
	cg_env_write_stdout(&g_env);
}

static void
cg_main_resp_state_handle(struct cg_uring_ctx *uring_ctx, struct cg_resp_ctx *resp_ctx)
{
	switch (resp_ctx->state) {
	case CG_RESP_STATE_SOCK_NOT_READY: {
		struct io_uring_sqe *sqe = cg_uring_sqe_init(uring_ctx, 0);
		sqe->opcode = IORING_OP_SOCKET;
		sqe->fd = AF_UNIX;
		sqe->off = SOCK_STREAM;
		sqe->len = 0;
		sqe->user_data = CG_TAG_PTR(resp_ctx, CG_TAG_RESP);
		resp_ctx->state = CG_RESP_STATE_SOCK_WAITING;
		cg_uring_sq_enqueue(&g_uring_ctx, 0);
		break;
	}
	case CG_RESP_STATE_SOCK_WAITING:
		break;
	case CG_RESP_STATE_SOCK_READY:
	case CG_RESP_STATE_CONN_NOT_READY: {
		struct io_uring_sqe *sqe = cg_uring_sqe_init(uring_ctx, 1);
		sqe->opcode = IORING_OP_CONNECT;
		sqe->fd = resp_ctx->sockfd;
		sqe->addr = (unsigned long)&resp_ctx->sockaddr;
		sqe->off = 0;
		sqe->len = sizeof(resp_ctx->sockaddr);
		sqe->user_data = CG_TAG_PTR(resp_ctx, CG_TAG_RESP);
		resp_ctx->state = CG_RESP_STATE_CONN_WAITING;
		cg_uring_sq_enqueue(uring_ctx, 1);
		break;
	}
	case CG_RESP_STATE_CONN_WAITING:
	default:
		break;
	}
}

static void
cg_main_resp_cqe_handle(struct cg_resp_ctx *ctx, struct io_uring_cqe *cqe)
{
	switch (ctx->state) {
	case CG_RESP_STATE_SOCK_WAITING:
		if (cqe->res < 0) {
			/* Handle the socket error. */
			break;
		}
		ctx->sockfd = cqe->res;
		ctx->state = CG_RESP_STATE_SOCK_READY;
		break;
	case CG_RESP_STATE_CONN_WAITING:
		if (cqe->res < 0) {
			/* Handle the connect error. */
			break;
		}
		ctx->state = CG_RESP_STATE_CONN_READY;
		break;
	default:
		break;
	}
}

static void
cg_main_pq_state_handle(struct cg_uring_ctx *uring_ctx, struct cg_pq_ctx *pq_ctx)
{
	switch (pq_ctx->state) {
	case CG_PQ_STATE_POLL_NOT_READY: {
		struct io_uring_sqe *sqe = cg_uring_sqe_init(uring_ctx, 5);
		sqe->opcode = IORING_OP_POLL_ADD;
		sqe->fd = pq_ctx->sockfd;
		sqe->poll32_events = pq_ctx->poll_mask;
		sqe->user_data = CG_TAG_PTR(pq_ctx, CG_TAG_PQ);

		pq_ctx->state = CG_PQ_STATE_POLL_WAITING;
		cg_uring_sq_enqueue(uring_ctx, 5);
		break;
	}
	case CG_PQ_STATE_POLL_WAITING:
	case CG_PQ_STATE_READY:
	case CG_PQ_STATE_ERROR:
	default:
		break;
	}
}

static void
cg_main_pq_cqe_handle(struct cg_pq_ctx *ctx, struct io_uring_cqe *cqe)
{
	switch (ctx->state) {
	case CG_PQ_STATE_POLL_WAITING:
		if (cqe->res < 0) {
			ctx->state = CG_PQ_STATE_ERROR;
			break;
		}
		PostgresPollingStatusType status = PQconnectPoll(ctx->conn);
		switch (status) {
		case PGRES_POLLING_READING:
			ctx->poll_mask = POLLIN;
			ctx->state = CG_PQ_STATE_POLL_NOT_READY;
			break;
		case PGRES_POLLING_WRITING:
			ctx->poll_mask = POLLOUT;
			ctx->state = CG_PQ_STATE_POLL_NOT_READY;
			break;
		case PGRES_POLLING_OK:
			ctx->state = CG_PQ_STATE_READY;
			break;
		case PGRES_POLLING_FAILED:
			ctx->state = CG_PQ_STATE_ERROR;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

static void
cg_main_fastcgi_state_handle(struct cg_uring_ctx *uring_ctx, struct cg_fastcgi_ctx *fastcgi_ctx)
{
	switch (fastcgi_ctx->state) {
	case CG_FASTCGI_STATE_UNLINK_NOT_READY: {
		struct io_uring_sqe *sqe = cg_uring_sqe_init(uring_ctx, 0);
		sqe->opcode = IORING_OP_UNLINKAT;
		sqe->fd = AT_FDCWD;
		sqe->addr = (unsigned long)g_env.fastcgi_sock;
		sqe->off = 0;
		sqe->flags = 0;
		sqe->user_data = CG_TAG_PTR(fastcgi_ctx, CG_TAG_FASTCGI);
		fastcgi_ctx->state = CG_FASTCGI_STATE_UNLINK_WAITING;
		cg_uring_sq_enqueue(uring_ctx, 0);
		break;
	}
	case CG_FASTCGI_STATE_UNLINK_WAITING:
		break;
	case CG_FASTCGI_STATE_UNLINK_READY:
	case CG_FASTCGI_STATE_SOCK_NOT_READY: {
		struct io_uring_sqe *sqe = cg_uring_sqe_init(uring_ctx, 1);
		sqe->opcode = IORING_OP_SOCKET;
		sqe->fd = AF_UNIX;
		sqe->off = SOCK_STREAM;
		sqe->flags = 0;
		sqe->len = 0;
		sqe->user_data = CG_TAG_PTR(fastcgi_ctx, CG_TAG_FASTCGI);
		fastcgi_ctx->state = CG_FASTCGI_STATE_SOCK_WAITING;
		cg_uring_sq_enqueue(&g_uring_ctx, 1);
		break;
	}
	case CG_FASTCGI_STATE_SOCK_WAITING:
		break;
	case CG_FASTCGI_STATE_SOCK_READY:
	case CG_FASTCGI_STATE_BIND_NOT_READY: {
		struct io_uring_sqe *sqe = cg_uring_sqe_init(uring_ctx, 2);
		sqe->opcode = IORING_OP_BIND;
		sqe->fd = g_fastcgi_ctx.sockfd;
		sqe->addr = (unsigned long)&fastcgi_ctx->sockaddr;
		sqe->off = 0;
		sqe->len = sizeof(g_fastcgi_ctx.sockaddr);
		sqe->user_data = CG_TAG_PTR(fastcgi_ctx, CG_TAG_FASTCGI);
		fastcgi_ctx->state = CG_FASTCGI_STATE_BIND_WAITING;
		cg_uring_sq_enqueue(uring_ctx, 2);
		break;
	}
	case CG_FASTCGI_STATE_BIND_WAITING:
		break;
	case CG_FASTCGI_STATE_BIND_READY:
	case CG_FASTCGI_STATE_LISTEN_NOT_READY: {
		struct io_uring_sqe *sqe = cg_uring_sqe_init(uring_ctx, 3);
		sqe->opcode = IORING_OP_LISTEN;
		sqe->fd = g_fastcgi_ctx.sockfd;
		sqe->off = 0;
		sqe->len = 128;
		sqe->user_data = CG_TAG_PTR(fastcgi_ctx, CG_TAG_FASTCGI);
		fastcgi_ctx->state = CG_FASTCGI_STATE_LISTEN_WAITING;
		cg_uring_sq_enqueue(uring_ctx, 3);
		break;
	}
	case CG_FASTCGI_STATE_LISTEN_WAITING:
		break;
	case CG_FASTCGI_STATE_LISTENING:
	case CG_FASTCGI_STATE_ACCEPT_NOT_READY: {
		struct io_uring_sqe *sqe = cg_uring_sqe_init(uring_ctx, 4);
		sqe->opcode = IORING_OP_ACCEPT;
		sqe->fd = g_fastcgi_ctx.sockfd;
		sqe->addr = 0;
		sqe->len = 0;
		sqe->ioprio = IORING_ACCEPT_MULTISHOT;
		sqe->accept_flags = SOCK_NONBLOCK | SOCK_CLOEXEC;
		sqe->user_data = CG_TAG_PTR(fastcgi_ctx, CG_TAG_FASTCGI);
		fastcgi_ctx->state = CG_FASTCGI_STATE_ACCEPTING;
		cg_uring_sq_enqueue(uring_ctx, 4);
		break;
	}
	case CG_FASTCGI_STATE_ACCEPTING:
	default:
		break;
	}
}

static void
cg_main_fastcgi_cqe_handle(struct cg_fastcgi_ctx *ctx, struct io_uring_cqe *cqe)
{
	switch (ctx->state) {
	case CG_FASTCGI_STATE_UNLINK_WAITING:
		ctx->state = CG_FASTCGI_STATE_UNLINK_READY;
		break;
	case CG_FASTCGI_STATE_SOCK_WAITING:
		if (cqe->res < 0) {
			/* Handle the socket error. */
			break;
		}
		ctx->sockfd = cqe->res;
		ctx->state = CG_FASTCGI_STATE_SOCK_READY;
		break;
	case CG_FASTCGI_STATE_BIND_WAITING:
		if (cqe->res < 0) {
			/* Handle bind error */
			break;
		}
		ctx->state = CG_FASTCGI_STATE_BIND_READY;
		break;
	case CG_FASTCGI_STATE_LISTEN_WAITING:
		if (cqe->res < 0) {
			/* Handle listen error */
			break;
		}
		ctx->state = CG_FASTCGI_STATE_LISTENING;
		break;
	case CG_FASTCGI_STATE_ACCEPTING:
		if (cqe->res < 0) {
			/* Handle accept error. If fatal, multishot stops. */
			break;
		}
		int cli_fd = cqe->res;
		struct cg_client_ctx *cli_ctx = cg_client_alloc(g_cli_ctx_pool, CG_MAX_CLIENTS, cli_fd);
		if (!cli_ctx) {
			syscall1(SYS_close, cli_fd);
		}
		if (!(cqe->flags & IORING_CQE_F_MORE)) {
			ctx->state = CG_FASTCGI_STATE_ACCEPT_NOT_READY;
		}
		break;
	default:
		break;
	}
}

void
cg_main_client_state_handle(struct cg_uring_ctx *uring_ctx)
{
        for (int i = 0; i < CG_MAX_CLIENTS; i++) {
                struct cg_client_ctx *client = &g_cli_ctx_pool[i];

                switch (client->state) {
                case CG_CLIENT_STATE_READ_NOT_READY: {
                        struct io_uring_sqe *sqe = cg_uring_sqe_init(uring_ctx, 5);
                        if (!sqe) break;
                        sqe->opcode = IORING_OP_READ;
                        sqe->fd = client->sockfd;
                        sqe->addr = (unsigned long)client->rx_buf;
                        sqe->len = CG_CLIENT_RX_BUF_SIZE;
                        sqe->off = 0;
                        sqe->user_data = CG_TAG_PTR(client, CG_TAG_CLIENT);

                        client->state = CG_CLIENT_STATE_READ_WAITING;
                        break;
                }
                case CG_CLIENT_STATE_WRITE_NOT_READY: {
                        struct io_uring_sqe *sqe = cg_uring_sqe_init(uring_ctx, 6);
                        if (!sqe) break;

                        sqe->opcode = IORING_OP_WRITE;
                        sqe->fd = client->sockfd;
                        sqe->addr = (unsigned long)client->tx_buf;
                        sqe->len = client->tx_len;
                        sqe->off = 0;
                        sqe->user_data = CG_TAG_PTR(client, CG_TAG_CLIENT);

                        client->state = CG_CLIENT_STATE_WRITE_WAITING;
                        break;
                }
                case CG_CLIENT_STATE_CLOSING:
                        cg_client_free(client);
                        break;
                default:
                        /* FREE, WAITING, PARSING, etc. don't need SQEs */
                        break;
                }
        }
}

void
cg_main_client_cqe_handle(struct cg_client_ctx *client, struct io_uring_cqe *cqe)
{
        switch (client->state) {
        case CG_CLIENT_STATE_READ_WAITING: {
                if (cqe->res <= 0) {
                        client->state = CG_CLIENT_STATE_CLOSING;
                        break;
                }
                client->rx_len = cqe->res;
                client->state = CG_CLIENT_STATE_PARSING;

                int is_valkey_route = 1;

                if (is_valkey_route) {
                        client->state = CG_CLIENT_STATE_DISPATCH_VALKEY;
                        /* e.g., cg_resp_enqueue_job(client); */
                } else {
                        client->state = CG_CLIENT_STATE_DISPATCH_PQ;
                        /* e.g., cg_pq_enqueue_job(client); */
                }
                break;
        }
        case CG_CLIENT_STATE_WRITE_WAITING: {
                if (cqe->res < 0) {
                        client->state = CG_CLIENT_STATE_CLOSING;
                        break;
                }
                client->state = CG_CLIENT_STATE_CLOSING;
                break;
        }
        default:
                break;
        }
}

__attribute__((naked, noreturn, used)) void
start(void)
{
	__asm__ volatile("mov %rsp, %rdi\n"
			 "call main\n");
}

/****f* main/main
 * NAME
 *   main
 * FUNCTION
 *
 * SOURCE
 */
__attribute__((noreturn, used)) void
main(const unsigned long *stack)
{
	cg_main_init(stack);
	cg_uring_init(&g_uring_ctx, IORING_SETUP_SQPOLL);
	cg_resp_sockaddr_init(&g_env, &g_resp_ctx.sockaddr);
	cg_pq_init("", &g_pq_ctx);
	cg_fastcgi_sockaddr_init(&g_env, &g_fastcgi_ctx.sockaddr);
	cg_client_pool_init(g_cli_ctx_pool, CG_MAX_CLIENTS);

	for (;;) {
		cg_main_resp_state_handle(&g_uring_ctx, &g_resp_ctx);
		cg_main_pq_state_handle(&g_uring_ctx, &g_pq_ctx);
		cg_main_fastcgi_state_handle(&g_uring_ctx, &g_fastcgi_ctx);
		cg_main_client_state_handle(&g_uring_ctx);
		struct io_uring_cqe *cqe = cg_uring_cq_peek(&g_uring_ctx);
		if (!cqe) {
			cg_uring_cqe_wait(&g_uring_ctx, &cqe);
		}
		while (cqe) {
			unsigned long tag = CG_GET_TAG(cqe->user_data);
			void *ctx_ptr = CG_UNTAG_PTR(cqe->user_data);
			switch (tag) {
			case CG_TAG_RESP:
				cg_main_resp_cqe_handle(ctx_ptr, cqe);
				break;
			case CG_TAG_PQ:
				cg_main_pq_cqe_handle(ctx_ptr, cqe);
				break;
			case CG_TAG_FASTCGI:
				cg_main_fastcgi_cqe_handle(ctx_ptr, cqe);
				break;
			case CG_TAG_CLIENT:
				cg_main_client_cqe_handle(ctx_ptr, cqe);
				break;
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
