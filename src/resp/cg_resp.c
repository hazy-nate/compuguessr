/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* resp/resp.c
 * NAME
 *   resp.c
 ******/

#define _SYS_SOCKET_H
#include <bits/socket.h>
#include <linux/un.h>
#include <linux/io_uring.h>
#include <sys/syscall.h>

#include "resp/cg_resp.h"
#include "sys/cg_env.h"
#include "util/cg_util.h"

const char CG_RESP_PING_STR[] = "*1\r\n$4\r\nPING\r\n";
/* CG_RESP_PING_STR_LEN defined by cg_resp.h */

void
cg_resp_sockaddr_init(struct cg_env *env, struct sockaddr_un *sockaddr)
{
	cg_memset_avx2(sockaddr, 0, sizeof(*sockaddr));
	sockaddr->sun_family = AF_UNIX;
	cg_memcpy_avx2(&sockaddr->sun_path, env->resp_sock,
		       env->resp_sock_len);
}
