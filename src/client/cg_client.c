/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

#include <sys/syscall.h>
#include <unistd.h>

#include "client/cg_client.h"
#include "sys/cg_syscall.h"
#include "util/cg_util.h"

void
cg_client_pool_init(struct cg_client_ctx *cli_ctx_pool, int len)
{
        cg_memset_avx2(cli_ctx_pool, 0, sizeof(*cli_ctx_pool) * len);
}

struct cg_client_ctx *
cg_client_alloc(struct cg_client_ctx *cli_ctx_pool, int size, int sockfd)
{
        for (int i = 0; i < size; i++) {
                if (cli_ctx_pool[i].state == CG_CLIENT_STATE_FREE) {
                        cli_ctx_pool[i].state = CG_CLIENT_STATE_READ_NOT_READY;
                        cli_ctx_pool[i].sockfd = sockfd;
                        cli_ctx_pool[i].rx_len = 0;
                        cli_ctx_pool[i].tx_len = 0;
                        return &cli_ctx_pool[i];
                }
        }
        return 0;
}

void
cg_client_free(struct cg_client_ctx *client)
{
        if (!client) return;
        if (client->sockfd > 0) {
                syscall1(SYS_close, client->sockfd);
        }
        client->state = CG_CLIENT_STATE_FREE;
        client->sockfd = -1;
}
