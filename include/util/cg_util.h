#ifndef CG_UTIL_H
#define CG_UTIL_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* util/cg_util.h, util/cg_util
 * NAME
 *   cg_util.h
 */

/*==============================================================================
 * SYSTEM HEADERS
 *============================================================================*/

#include <stdint.h>

/*==============================================================================
 * STRUCTS
 *============================================================================*/

struct cg_kv {
	char *key;
	char *val;
};

struct cg_ntoa_result {
	long long ptr;
	long long len;
};

extern long g_base_addr;

#define CG_RELOC(type, ptr) ((type)((long)(ptr) + g_base_addr))

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

extern char *cg_get_env(char **envp, const char *target_key_str);
extern int cg_memcmp_avx2(const void *s1, const void *s2, int n);
extern void *cg_memcpy(void *dest, const void *src, unsigned long n);
extern void cg_memcpy_avx2(void *dest, void *src, int n);
extern void cg_memset(void *dest, int val, unsigned long n);
extern void cg_memset_avx2(void *dest, int val, unsigned long n);
extern int cg_strcmp(const char *s1, const char *s2);
extern char *cg_memslide(char *s1, char *s2, int n);
extern int cg_strlen_avx2(char *buf);
extern struct cg_ntoa_result cg_itoa(int64_t n, char *buf);
extern struct cg_ntoa_result cg_utoa(uint64_t n, char *buf);
extern int cg_write_int(int64_t n, int fd);
extern int cg_write_uint(uint64_t n, int fd);

void *memcpy(void *dest, const void *src, unsigned long n);
__attribute__((target("sse4.2"))) uint64_t cg_hash_str(const char *, uint64_t);
void cg_puts(char *str);

/*==============================================================================
 * INLINE FUNCTIONS
 *============================================================================*/

#ifdef __clang__
	#pragma clang attribute push (__attribute__((always_inline,target("avx2,bmi2"))), apply_to=function)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wunused-function"
#else
	#pragma GCC push_options
	#pragma GCC target("avx2,bmi2")
#endif

static inline uint64_t
cg_hash_uint64(uint64_t n)
{
	n = (n ^ (n >> 30)) * 0xbf58476d1ce4e5b9ULL;
	n = (n ^ (n >> 27)) * 0x94d049bb133111ebULL;
	n = n ^ (n >> 31);
	return n;
}

static inline uint32_t
cg_div10_u32(uint32_t x)
{
	return (x * 0xCCCCCCCDULL) >> 35;
}

static inline uint32_t
cg_mod10_u32(uint32_t x)
{
	return x - (cg_div10_u32(x) * 10);
}

static inline uint32_t
cg_div60_u32(uint32_t x) {
        return (uint32_t)(((uint64_t)x * 0x88888889ULL) >> 37);
}

static inline uint32_t
cg_div3600_u32(uint32_t x) {
        return (uint32_t)(((uint64_t)x * 0x91A2B3C5ULL) >> 43);
}

static inline uint32_t
cg_div24_u32(uint32_t x) {
	return (uint32_t)(((uint64_t)x * 0xAAAAAAAAU) >> 33);
}

static inline uint32_t
cg_div86400_u32(uint32_t x) {
        uint32_t h = cg_div3600_u32(x);
        return (uint32_t)(((uint64_t)h * 0xAAAAAAABULL) >> 36);
}

static inline void
cg_write_digits2(char *ptr, uint32_t val)
{
	__uint128_t prod = cg_div10_u32(val);
	ptr[1] = (char)('0' + (val - (prod * 10)));
	ptr[0] = (char)('0' + prod);
}

static inline void
cg_write_digits4(char *ptr, uint32_t val)
{
	uint32_t prod = cg_div10_u32(val);
	ptr[3] = (char)('0' + (val - (prod * 10)));

	val = prod;
	prod = cg_div10_u32(val);
	ptr[2] = (char)('0' + (val - (prod * 10)));

	val = prod;
	prod = cg_div10_u32(val);
	ptr[1] = (char)('0' + (val - (prod * 10)));

	val = prod;
	prod = cg_div10_u32(val);
	ptr[0] = (char)('0' + (val - (prod * 10)));
}

#ifdef __clang__
	#pragma clang diagnostic pop
	#pragma clang attribute pop
#else
	#pragma GCC pop_options
#endif

#endif /* !CG_UTIL_H */
