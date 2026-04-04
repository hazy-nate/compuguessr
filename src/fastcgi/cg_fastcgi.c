/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* <module>/<name>
 * NAME
 *   <name>
 * FUNCTION
 *   <function>
 ******/

#define _SYS_SOCKET_H
#include <bits/socket.h>
#include <linux/un.h>

#include "fastcgi/cg_fastcgi.h"
#include "sys/cg_env.h"
#include "util/cg_util.h"

void
cg_fastcgi_sockaddr_init(struct cg_env *env, struct sockaddr_un *sockaddr)
{
	cg_memset_avx2(sockaddr, 0, sizeof(*sockaddr));
	sockaddr->sun_family = AF_UNIX;
	cg_memcpy_avx2(&sockaddr->sun_path, env->fastcgi_sock,
		       env->fastcgi_sock_len);
}
