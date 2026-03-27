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

extern const char CG_RESP_PING_STR[];
#define CG_RESP_PING_STR_LEN (sizeof(CG_RESP_PING_STR) - 1)

enum cg_resp_state {
	CG_RESP_STATE_IDLE = 0,
	CG_RESP_STATE_PING_TX,
	CG_RESP_STATE_PING_RX
};

struct cg_resp_ctx {
	int sockfd;
	struct cg_uring_ctx *uring_ctx;
	int tx_index;
	int rx_index;
};

void cg_resp_ping(struct cg_resp_ctx *, const char *);

#endif /* !CG_RESP_H */

