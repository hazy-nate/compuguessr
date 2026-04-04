/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

#ifndef CG_CLIENT_H
#define CG_CLIENT_H

#include <linux/io_uring.h>
#include <stdint.h>

#define CG_MAX_CLIENTS 1024
#define CG_CLIENT_RX_BUF_SIZE 4096
#define CG_CLIENT_TX_BUF_SIZE 4096

enum cg_client_state {
        CG_CLIENT_STATE_FREE = 0,
        CG_CLIENT_STATE_READ_NOT_READY,
        CG_CLIENT_STATE_READ_WAITING,
        CG_CLIENT_STATE_PARSING,
        CG_CLIENT_STATE_DISPATCH_VALKEY,
        CG_CLIENT_STATE_WAITING_VALKEY,
        CG_CLIENT_STATE_DISPATCH_PQ,
        CG_CLIENT_STATE_WAITING_PQ,
        CG_CLIENT_STATE_WRITE_NOT_READY,
        CG_CLIENT_STATE_WRITE_WAITING,
        CG_CLIENT_STATE_CLOSING,
};

struct cg_client_ctx {
        enum cg_client_state state;
        int sockfd;
        uint16_t fcgi_request_id;
        char rx_buf[CG_CLIENT_RX_BUF_SIZE];
        int rx_len;
        char tx_buf[CG_CLIENT_TX_BUF_SIZE];
        int tx_len;
};

void cg_client_pool_init(struct cg_client_ctx *, int);
struct cg_client_ctx *cg_client_alloc(struct cg_client_ctx *, int, int);
void cg_client_free(struct cg_client_ctx *client);

#endif /* CG_CLIENT_H */
