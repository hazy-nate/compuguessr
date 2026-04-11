#ifndef CG_APP_H
#define CG_APP_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* app/cg_app.h, app/cg_app
 * NAME
 *   cg_app.h
 ******/

/*==============================================================================
 * SYSTEM HEADERS
 *============================================================================*/

#include <linux/un.h>
#include <stdint.h>

/*==============================================================================
 * DEFINITIONS
 *============================================================================*/

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define CG_TAG_FASTCGI		0x1000000000000000ULL
#define CG_TAG_CLIENT		0x2000000000000000ULL
#define CG_TAG_TIMESTAMP	0x3000000000000000ULL
#define CG_PTR_MASK		0x0000FFFFFFFFFFFFULL

#define CG_TAG_PTR(ptr, tag) ((unsigned long)(ptr) | (tag))
// NOLINTNEXTLINE(performance-no-int-to-ptr)
#define CG_UNTAG_PTR(val) ((void *)((val) & CG_PTR_MASK))
#define CG_GET_TAG(val) ((val) & ~CG_PTR_MASK)

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

__attribute__((noreturn, used)) void main(const unsigned long *);

#endif /* !CG_APP_H */
