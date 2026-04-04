/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

#ifndef FCGI_H
#define FCGI_H

#include <linux/un.h>

#include "sys/cg_env.h"

#define FCGI_VERSION_1	       1

#define FCGI_BEGIN_REQUEST     1
#define FCGI_ABORT_REQUEST     2
#define FCGI_END_REQUEST       3
#define FCGI_PARAMS	       4
#define FCGI_STDIN	       5
#define FCGI_STDOUT	       6
#define FCGI_STDERR	       7
#define FCGI_DATA	       8
#define FCGI_GET_VALUES	       9
#define FCGI_GET_VALUES_RESULT 10
#define FCGI_UNKNOWN_TYPE      11
#define FCGI_MAXTYPE	       FCGI_UNKNOWN_TYPE

#define FCGI_RESPONDER	       1
#define FCGI_AUTHORIZER	       2
#define FCGI_FILTER	       3

#define FCGI_KEEP_CONN	       1

#define FCGI_REQUEST_COMPLETE  0
#define FCGI_CANT_MPX_CONN     1
#define FCGI_OVERLOADED	       2
#define FCGI_UNKNOWN_ROLE      3

enum cg_fastcgi_state {
        CG_FASTCGI_STATE_UNLINK_NOT_READY = 0,
        CG_FASTCGI_STATE_UNLINK_WAITING,
        CG_FASTCGI_STATE_UNLINK_READY,
        CG_FASTCGI_STATE_SOCK_NOT_READY,
        CG_FASTCGI_STATE_SOCK_WAITING,
        CG_FASTCGI_STATE_SOCK_READY,
        CG_FASTCGI_STATE_BIND_NOT_READY,
        CG_FASTCGI_STATE_BIND_WAITING,
        CG_FASTCGI_STATE_BIND_READY,
        CG_FASTCGI_STATE_LISTEN_NOT_READY,
        CG_FASTCGI_STATE_LISTEN_WAITING,
        CG_FASTCGI_STATE_LISTENING,
        CG_FASTCGI_STATE_ACCEPT_NOT_READY,
        CG_FASTCGI_STATE_ACCEPTING,
        CG_FASTCGI_STATE_ERROR
};

struct cg_fastcgi_ctx {
	enum cg_fastcgi_state state;
	struct sockaddr_un sockaddr;
	int sockfd;
};

void cg_fastcgi_sockaddr_init(struct cg_env *, struct sockaddr_un *);

#endif
