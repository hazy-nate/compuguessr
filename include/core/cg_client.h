#ifndef CG_CLIENT_H
#define CG_CLIENT_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* core/cg_client.h
 * NAME
 *   cg_client.h
 * FUNCTION
 *   Client context and request structures. Defines limits for URI
 *   length, method strings, and STDIN accumulation.
 ******/

/*==============================================================================
 * SYSTEM HEADERS
 *============================================================================*/

#include <linux/io_uring.h>
#include <stdint.h>

/*==============================================================================
 * LOCAL HEADERS
 *============================================================================*/

#include "core/cg_fastcgi.h"

/*==============================================================================
 * DEFINITIONS
 *============================================================================*/

#define CG_MAX_CLIENTS		1024
#define CG_MAX_URI_LEN		128
#define CG_MAX_METHOD_LEN	16
#define CG_MAX_STDIN_LEN	4096

/*==============================================================================
 * ENUMERATIONS
 *============================================================================*/

enum cg_client_state {
	CG_CLIENT_STATE_FREE = 0,
	CG_CLIENT_STATE_IN_USE,
};

/*==============================================================================
 * STRUCTS
 *============================================================================*/

struct cg_client_req {
	alignas(8) char uri[CG_MAX_URI_LEN];
	alignas(8) char method[CG_MAX_METHOD_LEN];
	alignas(8) char session_id[32];
	uint16_t req_id;
	uint8_t keep_alive;
	uint32_t stdin_len;
	char stdin_buf[CG_MAX_STDIN_LEN];
};

struct cg_client_ctx {
	enum cg_client_state state;
	struct cg_fastcgi_client_ctx fcgi_cli_ctx;
	struct cg_client_req req;
};

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

void cg_client_pool_init(struct cg_client_ctx *, int);
struct cg_client_ctx *cg_client_alloc(struct cg_client_ctx *, int);
uint8_t *cg_client_buf_slide(struct cg_client_ctx *, uint8_t *);
void cg_client_free(struct cg_client_ctx *);

#endif /* !CG_CLIENT_H */
