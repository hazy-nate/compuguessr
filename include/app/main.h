/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* <module>/<name>
 * NAME
 *   main.h
 * FUNCTION
 *   <function>
 ******/

#ifndef CG_MAIN_H
#define CG_MAIN_H

#include <stdint.h>

#include "sys/cg_env.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define CG_TAG_RESP	0x1000000000000000ULL
#define CG_TAG_PQ	0x2000000000000000ULL
#define CG_TAG_FASTCGI	0x3000000000000000ULL
#define CG_TAG_CLIENT	0x4000000000000000ULL
#define CG_PTR_MASK	0x0000FFFFFFFFFFFFULL

#define CG_TAG_PTR(ptr, tag) ((unsigned long)(ptr) | (tag))
// NOLINTNEXTLINE(performance-no-int-to-ptr)
#define CG_UNTAG_PTR(val) ((void *)((val) & CG_PTR_MASK))
#define CG_GET_TAG(val) ((val) & ~CG_PTR_MASK)

const char NEWLINE[] = "\n";

extern uint16_t cg_fcgi_read_request(int);
extern void cg_fcgi_send_response(int, uint16_t);

__attribute__((naked, noreturn, used)) void start(void) __asm__("_start");
void cg_env_write_stdout(struct cg_env *);
__attribute__((noreturn, used)) void main(const unsigned long *);

#endif
