/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* util/cg_util.h
 * NAME
 *   cg_util.h
 */

#ifndef CG_UTIL_H
#define CG_UTIL_H

#include <stdint.h>

struct utoa_result {
	long long ptr;
	long long len;
};

extern char *cg_get_env(char **envp, const char *target_key_str);
extern int cg_memcmp_avx2(const void *s1, const void *s2, int n);
extern void cg_memcpy_avx2(void *dest, void *src, int n);
extern void cg_memset_avx2(void *dest, int val, unsigned long n);
extern int cg_strlen_avx2(char *buf);
extern struct utoa_result cg_utoa(uint64_t n, char *buf);
extern int cg_write_uint(uint64_t n, int fd);

#endif
