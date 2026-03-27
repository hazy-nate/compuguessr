/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* resp/resp.c
 * NAME
 *   resp.c
 ******/

#include <linux/io_uring.h>

#include "resp/resp.h"
#include "sys/cg_uring.h"

const char CG_RESP_PING_STR[] = "*1\r\n$4\r\nPING\r\n";

void
cg_resp_ping(struct cg_resp_ctx *ctx, const char *res_buf)
{
	struct io_uring_sqe *write_sqe = cg_uring_sqe_init(ctx->uring_ctx, ctx->tx_index);
	write_sqe->opcode = IORING_OP_WRITE;
	write_sqe->flags |= IOSQE_IO_LINK;
	write_sqe->fd = ctx->sockfd;
	write_sqe->addr = (unsigned long)&CG_RESP_PING_STR;
	write_sqe->len = CG_RESP_PING_STR_LEN;
	write_sqe->user_data = CG_RESP_STATE_PING_TX;

	struct io_uring_sqe *read_sqe = cg_uring_sqe_init(ctx->uring_ctx, ctx->rx_index);
	read_sqe->opcode = IORING_OP_READ;
	read_sqe->fd = ctx->sockfd;
	read_sqe->addr = (unsigned long)res_buf;
	read_sqe->len = 7;
	read_sqe->user_data = CG_RESP_STATE_PING_RX;

	cg_uring_sq_enqueue(ctx->uring_ctx, ctx->tx_index);
	cg_uring_sq_enqueue(ctx->uring_ctx, ctx->rx_index);
}

