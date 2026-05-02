/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* sys/cg_types.h
 * NAME
 *   cg_types.h
 * FUNCTION
 *   Core system type definitions, generic pointer unions, and
 *   buffer metadata structures used across the project.
 ******/

#ifndef CG_TYPES_H
#define CG_TYPES_H

#include <stdint.h>

/*==============================================================================
 * TYPEDEFS
 *============================================================================*/

/****s* cg_types/cg_size_t
 * NAME
 *   cg_size_t
 * SOURCE
 */
typedef __SIZE_TYPE__ cg_size_t;
/******/

typedef void (*cg_generic_func_t)(void);

/*==============================================================================
 * UNIONS
 *============================================================================*/

/****s* cg_types/cg_ptr
 * NAME
 *   cg_ptr
 * SOURCE
 */
union cg_ptr {
	long addr;
	void *ptr;
	cg_generic_func_t func_ptr;
};
/******/

/*==============================================================================
 * STRUCTS
 *============================================================================*/

struct cg_buffer_metadata {
	uint8_t *buf;
	uint32_t len;
	uint32_t valid;
	uint32_t pos;
};

#endif /* !CG_TYPES_H */
