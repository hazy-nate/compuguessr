/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* pq/cg_pq.c, pq/cg_pq
 * NAME
 *   cg_pq
 ******/

#include <libpq-fe.h>
#include <sys/poll.h>

#include "pq/cg_pq.h"

void
cg_pq_init(const char *conninfo, struct cg_pq_ctx *ctx)
{
	ctx->conn = PQconnectStart(conninfo);
	if (PQstatus(ctx->conn) == CONNECTION_BAD) {
		ctx->state = CG_PQ_STATE_ERROR;
		return;
	}
	ctx->sockfd = PQsocket(ctx->conn);
	PQsetnonblocking(ctx->conn, 1);
	ctx->state = CG_PQ_STATE_POLL_NOT_READY;
	ctx->poll_mask = POLLOUT;
}
