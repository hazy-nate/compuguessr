/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* <module>/<name>
 * NAME
 *   <name>
 * FUNCTION
 *   <function>
 ******/

#ifndef CG_PQ_H
#define CG_PQ_H

#include <libpq-fe.h>

enum cg_pq_state {
	CG_PQ_STATE_NOT_STARTED = 0,
	CG_PQ_STATE_POLL_NOT_READY,
	CG_PQ_STATE_POLL_WAITING,
	CG_PQ_STATE_READY,
	CG_PQ_STATE_ERROR
};

struct cg_pq_ctx {
	enum cg_pq_state state;
	PGconn *conn;
	int sockfd;
	short poll_mask;
};

void cg_pq_init(const char *, struct cg_pq_ctx *);

#endif /* !CG_PQ_H */
