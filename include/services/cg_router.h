#ifndef CG_ROUTER_H
#define CG_ROUTER_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* services/cg_router.h, services/cg_router
 * NAME
 *   cg_router.h
 ******/

/*==============================================================================
 * LOCAL HEADERS
 *============================================================================*/

#include "core/cg_client.h"
#include "data/cg_hashmap.h"

/*==============================================================================
 * TYPEDEFS
 *============================================================================*/

typedef void (*cg_route_func_t)(struct cg_client_ctx *cli_ctx);

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

void cg_route_map_init(struct cg_hashmap *);
void cg_route_map_reply(struct cg_hashmap *, struct cg_client_ctx *);

#endif /* !CG_ROUTER_H */
