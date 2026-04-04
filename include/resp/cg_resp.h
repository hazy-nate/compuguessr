/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* <module>/<name>
 * NAME
 *   <name>
 * FUNCTION
 *   <function>
 ******/

#ifndef CG_RESP_H
#define CG_RESP_H

#include <linux/un.h>

#include "sys/cg_env.h"

extern const char CG_RESP_PING_STR[];
#define CG_RESP_PING_STR_LEN (sizeof(CG_RESP_PING_STR) - 1)

enum cg_resp_state {
        CG_RESP_STATE_SOCK_NOT_READY = 0,
        CG_RESP_STATE_SOCK_WAITING,
        CG_RESP_STATE_SOCK_READY,
        CG_RESP_STATE_CONN_NOT_READY,
        CG_RESP_STATE_CONN_WAITING,
        CG_RESP_STATE_CONN_READY,
        CG_RESP_STATE_ERROR
};

struct cg_resp_ctx {
	enum cg_resp_state state;
	int sockfd;
	struct sockaddr_un sockaddr;
};

void cg_resp_sockaddr_init(struct cg_env *, struct sockaddr_un *);
void cg_resp_ping(struct cg_resp_ctx *, const char *);

#endif /* !CG_RESP_H */

