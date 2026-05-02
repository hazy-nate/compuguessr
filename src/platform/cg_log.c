/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* platform/cg_log.c
 * NAME
 * cg_log.c
 * FUNCTION
 * Vectored logging utility. Provides atomic, thread-safe logging
 * to stderr with optional timestamping and source location info.
 ******/

#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

#include "arch/cg_syscall.h"
#include "platform/cg_log.h"
#include "util/cg_util.h"

/****f* platform/cg_log_write
 * NAME
 *   cg_log_write
 * SYNOPSIS
 *   void cg_log_write(const char *level, const char *file, const char *line, const char *msg)
 * FUNCTION
 *   Writes a formatted log message to the standard error stream. The
 *   output includes the source file, line number, and log level. It
 *   utilizes a vectored write operation (writev) to combine multiple
 *   string components into a single atomic output call to avoid
 *   interleaving with other process output.
 * INPUTS
 *   * level - The severity level of the log (e.g., note, error, FATAL).
 *   * file  - The name of the source file generating the log.
 *   * line  - The line number in the source file.
 *   * msg   - The actual log message string.
 * SOURCE
 */
void
cg_log_write(const char *level, const char *file, const char *line, const char *msg)
{
	struct iovec iov[8];
	iov[0].iov_base = "(";
	iov[0].iov_len = 1; // FIXED: Changed from 3 to 1

	iov[1].iov_base = (void *)file;
	iov[1].iov_len = cg_strlen_avx2((char *)file);

	iov[2].iov_base = ".";
	iov[2].iov_len = 1;

	iov[3].iov_base = (void *)line;
	iov[3].iov_len = cg_strlen_avx2((char *)line);

	iov[4].iov_base = ") [";
	iov[4].iov_len = 3;

	iov[5].iov_base = (void *)level;
	iov[5].iov_len = cg_strlen_avx2((char *)level);

	iov[6].iov_base = "] ";
	iov[6].iov_len = 2;

	iov[7].iov_base = (void *)msg;
	iov[7].iov_len = cg_strlen_avx2((char *)msg);

	syscall3(SYS_writev, STDERR_FILENO, (long)iov, 8);
}
/******/

/****f* platform/cg_log_write_ts
 * NAME
 *   cg_log_write_ts
 * SYNOPSIS
 *   void cg_log_write_ts(const char *datetime, const char *level, const char *file, const char *line, const char *msg)
 * FUNCTION
 *   Writes a formatted log message with a preceding timestamp to the
 *   standard error stream. This function extends cg_log_write by
 *   including a 19-character datetime string at the start of the
 *   output. It also uses vectored writes (writev) for atomicity.
 * INPUTS
 *   * datetime - A 19-character string representing the current time.
 *   * level    - The severity level of the log.
 *   * file     - The name of the source file.
 *   * line     - The line number in the source file.
 *   * msg      - The actual log message string.
 * SOURCE
 */
void
cg_log_write_ts(const char *datetime, const char *level, const char *file, const char *line, const char *msg)
{
	struct iovec iov[9];

	iov[0].iov_base = (void *)datetime;
	iov[0].iov_len = 19;

	iov[1].iov_base = ": (";
	iov[1].iov_len = 3;

	iov[2].iov_base = (void *)file;
	iov[2].iov_len = cg_strlen_avx2((char *)file);

	iov[3].iov_base = ".";
	iov[3].iov_len = 1;

	iov[4].iov_base = (void *)line;
	iov[4].iov_len = cg_strlen_avx2((char *)line);

	iov[5].iov_base = ") [";
	iov[5].iov_len = 3;

	iov[6].iov_base = (void *)level;
	iov[6].iov_len = cg_strlen_avx2((char *)level);

	iov[7].iov_base = "] ";
	iov[7].iov_len = 2;

	iov[8].iov_base = (void *)msg;
	iov[8].iov_len = cg_strlen_avx2((char *)msg);

	syscall3(SYS_writev, STDERR_FILENO, (long)iov, 9);
}
/******/
