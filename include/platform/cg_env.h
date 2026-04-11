#ifndef CG_ENV_H
#define CG_ENV_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* platform/cg_env.h, platform/cg_env
 * NAME
 *   cg_env.h
 ******/

/*==============================================================================
 * SYSTEM HEADERS
 *============================================================================*/

#include <time.h>

/*==============================================================================
 * LOCAL HEADERS
 *============================================================================*/

#include "sys/cg_types.h"

/*==============================================================================
 * DEFINITIONS
 *============================================================================*/

#define CG_ENV_FASTCGI_SOCK_KEY "FASTCGI_SOCK"
#define CG_ENV_FASTCGI_SOCK_KEY_LEN 12
#define CG_ENV_FASTCGI_SOCK_DEFAULT "/tmp/compuguessr.sock"
#define CG_ENV_FASTCGI_SOCK_DEFAULT_LEN 21

/*==============================================================================
 * TYPEDEFS
 *============================================================================*/

typedef int (*cg_vdso_gettime_t)(int, struct timespec *);

/*==============================================================================
 * STRUCTS
 *============================================================================*/

/****s* cg_env/cg_env_config
 * NAME
 *   cg_env_config
 * SOURCE
 */
struct cg_env {
	char *fastcgi_sock;
	int fastcgi_sock_len;
};
/******/

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

/****f* cg_env/cg_init_env_config
 * NAME
 *   cg_init_env_config
 * SOURCE
 */
extern void cg_env_init(char **, struct cg_env *);
/******/

extern void cg_env_write_stdout(struct cg_env *);
extern long cg_vdso_base_addr(char **);
extern cg_generic_func_t cg_vdso_func(long, char *);

#endif /* !CG_ENV_H */
