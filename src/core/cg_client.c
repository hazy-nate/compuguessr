/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* core/cg_client.c
 * NAME
 *   cg_client.c
 * FUNCTION
 *   Client context management, including pool initialization,
 *   allocation, and resource reclamation for active connections.
 ******/

#include <sys/syscall.h>
#include <unistd.h>

#include "core/cg_client.h"
#include "core/cg_fastcgi.h"

/****f* cg_client/cg_client_pool_init
 * NAME
 *   cg_client_pool_init
 * SYNOPSIS
 *   void cg_client_pool_init(struct cg_client_ctx *cli_ctx_pool, int count)
 * FUNCTION
 *   Initializes a pool of client context structures. It iterates
 *   through the pool, setting each client's state to CG_CLIENT_STATE_FREE
 *   and calling cg_fastcgi_client_init to initialize the underlying FastCGI
 *   specific data for each context.
 * INPUTS
 *   * cli_ctx_pool - Pointer to the array of cg_client_ctx structures to initialize.
 *   * count        - The number of client contexts in the pool.
 * SOURCE
 */
void
cg_client_pool_init(struct cg_client_ctx *cli_ctx_pool, int count)
{
	for (int i = 0; i < count; i++) {
		cli_ctx_pool[i].state = CG_CLIENT_STATE_FREE;
		cg_fastcgi_client_init(&cli_ctx_pool[i].fcgi_cli_ctx);
	}
}
/******/

/****f* cg_client/cg_client_alloc
 * NAME
 *   cg_client_alloc
 * SYNOPSIS
 *   struct cg_client_ctx *cg_client_alloc(struct cg_client_ctx *cli_ctx_pool, int pool_size)
 * FUNCTION
 *   Searches the provided client pool for an available context.
 *   If a context with state CG_CLIENT_STATE_FREE is found, it marks it
 *   as CG_CLIENT_STATE_IN_USE and returns a pointer to it.
 * INPUTS
 *   * cli_ctx_pool - Pointer to the array of available client contexts.
 *   * pool_size    - The total number of contexts in the pool.
 * RESULT
 *   * Pointer to an allocated cg_client_ctx structure, or 0 if no free
 *   contexts are available.
 * SOURCE
 */
struct cg_client_ctx *
cg_client_alloc(struct cg_client_ctx *cli_ctx_pool, int pool_size)
{
	for (int i = 0; i < pool_size; i++) {
		if (cli_ctx_pool[i].state == CG_CLIENT_STATE_FREE) {
			cli_ctx_pool[i].state = CG_CLIENT_STATE_IN_USE;
			return &cli_ctx_pool[i];
		}
	}
	return 0;
}
/******/

/****f* cg_client/cg_client_free
 * NAME
 *   cg_client_free
 * SYNOPSIS
 *   void cg_client_free(struct cg_client_ctx *client)
 * FUNCTION
 *   Releases a client context back to the pool by setting its state to
 *   CG_CLIENT_STATE_FREE.
 * INPUTS
 *   * client - Pointer to the client context structure to be freed.
 * SOURCE
 */
void
cg_client_free(struct cg_client_ctx *client)
{
	if (!client) {
		return;
	}
	client->state = CG_CLIENT_STATE_FREE;
}
/******/
