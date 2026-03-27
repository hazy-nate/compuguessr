#ifndef CG_ENV_H
#define CG_ENV_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* sys/cg_env.h
 * NAME
 *   cg_env.h
 * FUNCTION
 *   <function>
 ******/

/****s* cg_env/cg_env_config
 * NAME
 *   cg_env_config
 * SOURCE
 */
struct cg_env {
	char *compuguessr_sock;
	char *resp_sock;
};
/******/

/****f* cg_env/cg_init_env_config
 * NAME
 *   cg_init_env_config
 * SOURCE
 */
__attribute__((weak)) void cg_env_init(char **, struct cg_env *);
/******/

#endif /* !CG_ENV_H */
