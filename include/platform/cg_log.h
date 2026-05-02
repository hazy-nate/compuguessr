#ifndef CG_LOG_H
#define CG_LOG_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* platform/cg_log.h
 * NAME
 *   cg_log.h
 * FUNCTION
 *   Logging system macros and function prototypes for atomic
 *   vectored writes to standard error.
 ******/

/*==============================================================================
 * DEFINITIONS
 *============================================================================*/

#define CG_STR(x) #x
#define CG_XSTR(x) CG_STR(x)

#define cg_log_write_note(msg) cg_log_write("note", __FILE__, CG_XSTR(__LINE__), msg)
#define cg_log_write_error(msg) cg_log_write("error", __FILE__, CG_XSTR(__LINE__), msg)
#define cg_log_write_fatal(msg) cg_log_write("FATAL", __FILE__, CG_XSTR(__LINE__), msg)

#define cg_log_write_ts_note(datetime, msg) cg_log_write_ts(datetime, "note", __FILE__, CG_XSTR(__LINE__), msg)
#define cg_log_write_ts_error(datetime, msg) cg_log_write_ts(datetime, "error", __FILE__, CG_XSTR(__LINE__), msg)
#define cg_log_write_ts_fatal(datetime, msg) cg_log_write_ts(datetime, "FATAL", __FILE__, CG_XSTR(__LINE__), msg)

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

void cg_log_write(const char *, const char *, const char *, const char *);
void cg_log_write_ts(const char *, const char *, const char *, const char *, const char *);

#endif /* !CG_LOG_H */
