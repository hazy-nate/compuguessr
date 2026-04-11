#ifndef CG_TIME_H
#define CG_TIME_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* sys/cg_time.h
 * NAME
 *   cg_time.h
 * FUNCTION
 *   <function>
 ******/

#include <stdint.h>

extern void cg_time_init(char **);
extern uint64_t cg_time_get(void);
extern void cg_datetime_str_get(char *);

#endif
