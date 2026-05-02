/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* platform/cg_time.c
 * NAME
 *   cg_time.c
 * FUNCTION
 *   High-performance time subsystem. Utilizes the vDSO for low-latency
 *   timestamping and provides optimized datetime string formatting.
 ******/

#include <linux/time.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <time.h>

#include "arch/cg_syscall.h"
#include "platform/cg_env.h"
#include "platform/cg_time.h"
#include "util/cg_util.h"

__attribute__((aligned(32), section(".bss"))) static cg_vdso_gettime_t g_vdso_gettime;

/****f* platform/cg_time_init
 * NAME
 *   cg_time_init
 * SYNOPSIS
 *   __attribute__((weak)) void cg_time_init(char **envp)
 * FUNCTION
 *   Initializes the time subsystem by attempting to locate the vDSO
 *   function "__vdso_clock_gettime". It retrieves the vDSO base
 *   address from the environment pointer and resolves the
 *   specific function pointer for high-performance time retrieval.
 * INPUTS
 *   * envp - Pointer to the environment variable array.
 * SOURCE
 */
__attribute__((weak)) void
cg_time_init(char **envp)
{
	long vdso_base_addr = cg_vdso_base_addr(envp);
	g_vdso_gettime = (cg_vdso_gettime_t)cg_vdso_func(vdso_base_addr,
	    "__vdso_clock_gettime");
}
/******/

/****f* platform/cg_time_get
 * NAME
 *   cg_time_get
 * SYNOPSIS
 *   __attribute__((weak)) uint64_t cg_time_get(void)
 * FUNCTION
 *   Returns the current Unix timestamp in seconds. It prefers
 *   using the mapped vDSO gettime function for performance. If
 *   the vDSO is unavailable, it falls back to a standard
 *   SYS_time syscall.
 * RESULT
 *   * The current time in seconds since the Epoch.
 * SOURCE
 */
__attribute__((weak)) uint64_t
cg_time_get(void)
{
	if (!g_vdso_gettime) {
		return (uint64_t)syscall1(SYS_time, 0);
	}
	struct timespec ts;
	g_vdso_gettime(CLOCK_REALTIME, &ts);
	return (uint64_t)ts.tv_sec;
}
/******/

/****f* platform/cg_datetime_str_get
 * NAME
 *   cg_datetime_str_get
 * SYNOPSIS
 *   __attribute__((target("avx2,bmi2"),weak)) void cg_datetime_str_get(char *out)
 * FUNCTION
 *   Generates a formatted date and time string (YYYY-MM-DD HH:MM:SS).
 *   The function performs highly optimized integer math and
 *   division to convert the raw Unix epoch seconds into calendar
 *   fields. It utilizes specialized division helpers and AVX2/BMI2
 *   instructions for maximum performance.
 * INPUTS
 *   * out - Pointer to a buffer of at least 20 bytes to receive
 *   the formatted string.
 * SOURCE
 */
__attribute__((target("avx2,bmi2"),weak)) void
cg_datetime_str_get(char *out)
{
	uint32_t seconds = (uint32_t)cg_time_get();
	uint32_t days_since_epoch = cg_div86400_u32(seconds);
	uint32_t secs_of_day      = seconds - (days_since_epoch * 86400);
	uint32_t hours   = cg_div3600_u32(secs_of_day);
	uint32_t hour_r  = secs_of_day - (hours * 3600);
	uint32_t minutes = cg_div60_u32(hour_r);
	uint32_t secs    = hour_r - (minutes * 60);
	uint32_t z   = days_since_epoch + 719468;
	uint32_t era = z / 146097;
	uint32_t doe = z - (era * 146097);
	uint32_t yoe = (doe - (doe / 1460) + (doe / 36524) - (doe / 146096)) / 365;
	uint32_t y   = yoe + (era * 400);
	uint32_t doy = doe - ((365 * yoe) + (yoe / 4) - (yoe / 100));
	uint32_t mp  = ((5 * doy) + 2) / 153;
	uint32_t d = doy - (((153 * mp) + 2) / 5) + 1;
	uint32_t m = mp + (mp < 10 ? 3 : -9);
	y += (m <= 2);
	/*====================================================================*/
	cg_write_digits4(&out[0], y);
	cg_write_digits2(&out[5], m);
	cg_write_digits2(&out[8], d);
	/*====================================================================*/
	cg_write_digits2(&out[11], hours);
	cg_write_digits2(&out[14], minutes);
	cg_write_digits2(&out[17], secs);
}
/******/
