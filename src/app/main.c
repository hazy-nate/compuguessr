/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* app/main.c
 * NAME
 *   main.c
 ******/

#define _SYS_SOCKET_H
#include <bits/socket.h>
#include <linux/io_uring.h>
#include <linux/un.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include "resp/resp.h"
#include "sys/cg_env.h"
#include "sys/cg_syscall.h"
#include "sys/cg_uring.h"
#include "util/cg_util.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

const char NEWLINE[] = "\n";

extern uint16_t cg_fcgi_read_request(int);
extern void cg_fcgi_send_response(int, uint16_t);

__attribute__((naked, noreturn, used)) void start(void) __asm__("_start");
__attribute__((noreturn, used)) void main(const unsigned long *stack);


void
cg_env_write_stdout(struct cg_env *env)
{
	struct iovec iov[3];

	iov[2].iov_base = (char *)NEWLINE;
	iov[2].iov_len = 1;

	/* COMPUGUESSR_SOCK */

	iov[0].iov_base = "COMPUGUESSR_SOCK = ";
	iov[0].iov_len = 19;

	iov[1].iov_base = env->compuguessr_sock;
	iov[1].iov_len = cg_strlen_avx2(env->compuguessr_sock);

	syscall3(SYS_writev, STDOUT_FILENO, (long)iov, 3);

	/* RESP_SOCK */

	iov[0].iov_base = "RESP_SOCK = ";
	iov[0].iov_len = 12;

	iov[1].iov_base = env->resp_sock;
	iov[1].iov_len = cg_strlen_avx2(env->resp_sock);

	syscall3(SYS_writev, STDOUT_FILENO, (long)iov, 3);
}

__attribute__((naked, noreturn, used)) void
start(void)
{
	__asm__ volatile("mov %rsp, %rdi\n"
			 "call main\n");
}

/****f* main/_start
 * NAME
 *   _start
 * SOURCE
 */
__attribute__((noreturn, used)) void
main(const unsigned long *stack)
{
	unsigned long argc = stack[0];
	/* char **argv = (char **)&stack[1]; */
	char **envp = (char **)&stack[argc + 2];

	/* io_uring initialization */

	struct cg_uring_ctx uring_ctx;
	cg_uring_init(&uring_ctx, IORING_SETUP_SQPOLL);

	/* Environment configuration */

	struct cg_env env;
	cg_env_init(envp, &env);

	int compuguessr_sock_len = cg_strlen_avx2(env.compuguessr_sock);
	if (compuguessr_sock_len > UNIX_PATH_MAX) {
		/* Handle a file path too big. */
	}

	int resp_sock_path_len = cg_strlen_avx2(env.resp_sock);
	if (resp_sock_path_len > UNIX_PATH_MAX) {
		/* Handle a file path too big. */
	}

	cg_env_write_stdout(&env);

	/* RESP socket connection */

	struct sockaddr_un resp_sockaddr;
	cg_memset_avx2(&resp_sockaddr, 0, sizeof(resp_sockaddr));
	resp_sockaddr.sun_family = AF_UNIX;
	cg_memcpy_avx2(&resp_sockaddr.sun_path, env.resp_sock,
		       resp_sock_path_len);

	int sockfd = (int)syscall3(SYS_socket, 1, 1, 0);
	if (sockfd < 0) {
		syscall3(SYS_write, 1, (long)"Socket error\n", 13);
		syscall1(SYS_exit, EXIT_FAILURE);
	}

	struct timespec connect_delay;
	connect_delay.tv_sec = 5;
	for (;;) {
		long res = syscall3(SYS_connect, sockfd, (long)&resp_sockaddr,
				    sizeof(resp_sockaddr));
		if (res == 0) {
			break;
		}
		syscall3(SYS_write, 1, (long)"Connect error\n", 14);
		syscall2(SYS_nanosleep, (long)&connect_delay, 0);
	}

	/* RESP initialization */

	struct cg_resp_ctx resp_ctx;
	resp_ctx.sockfd = sockfd;
	resp_ctx.uring_ctx = &uring_ctx;
	resp_ctx.tx_index = 0;
	resp_ctx.rx_index = 1;

	/* lighttpd socket */

	struct sockaddr_un fcgi_sockaddr;
	cg_memset_avx2(&fcgi_sockaddr, 0, sizeof(fcgi_sockaddr));
	fcgi_sockaddr.sun_family = AF_UNIX;
	cg_memcpy_avx2(&fcgi_sockaddr.sun_path, env.compuguessr_sock, compuguessr_sock_len);
	syscall1(SYS_unlink, (long)env.compuguessr_sock);
	int fcgi_sockfd = (int)syscall3(SYS_socket, AF_UNIX, SOCK_STREAM, 0);
	syscall3(SYS_bind, fcgi_sockfd, (long)&fcgi_sockaddr, sizeof(fcgi_sockaddr));
	syscall2(SYS_listen, fcgi_sockfd, 128);
	syscall3(SYS_write, 1, (long)"Waiting for FastCGI...\n", 23);

	char pong_res[7];
	for (;;) {
		int cli_sockfd = (int)syscall3(SYS_accept, fcgi_sockfd, 0, 0);
		if (cli_sockfd < 0) {
			continue;
		}

		uint16_t req_id = cg_fcgi_read_request(cli_sockfd);
		if (req_id < 0) {
			syscall1(SYS_close, cli_sockfd);
			continue;
		}

		cg_resp_ping(&resp_ctx, pong_res);
		syscall6(SYS_io_uring_enter, uring_ctx.fd, 0, 0, IORING_ENTER_SQ_WAKEUP, 0, 0);
		syscall3(SYS_write, 1, (long)"Ping sent!\n", 11);

		for (;;) {
			struct io_uring_cqe *cqe = cg_uring_cq_peek(&uring_ctx);
			if (!cqe) {
				continue;
			}

			switch (cqe->user_data) {
			case CG_RESP_STATE_PING_TX:
				if (cqe->res < 0) {
					/* Handle socket error */
					syscall3(SYS_write, 1, (long)"Socket error", 12);
					goto leave_loop;
				}
				break;
			case CG_RESP_STATE_PING_RX:
				if (cqe->res >= 7 &&
				    cg_memcmp_avx2((const char *)pong_res, "+PONG\r\n",
						   7) == 0) {
					cg_fcgi_send_response(cli_sockfd, req_id);
					syscall3(SYS_write, 1, (long)"Received pong!\n",
						 15);
					goto leave_loop;
				} else {
					/* Handle unexpected data or an error */
				}
				break;
			default:
				break;
			}
			cg_uring_cq_discard(&uring_ctx, 1);
			continue;
leave_loop:
			cg_uring_cq_discard(&uring_ctx, 1);
			break;
		}

		syscall1(SYS_close, cli_sockfd);
	}

	syscall1(SYS_exit, EXIT_SUCCESS);
	__builtin_unreachable();
}
/******/
