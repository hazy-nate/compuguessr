/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* sys/cg_types.h
 * NAME
 *   cg_types.h
 ******/

#ifndef CG_SYS_H
#define CG_SYS_H

#include <stdint.h>

/****s* cg_types/cg_ptr
 * NAME
 *   cg_ptr
 * SOURCE
 */
union cg_ptr {
	long addr;
	void *ptr;
};
/******/

/****s* cg_types/cg_size_t
 * NAME
 *   cg_size_t
 * SOURCE
 */
typedef __SIZE_TYPE__ cg_size_t;
/******/

#endif
