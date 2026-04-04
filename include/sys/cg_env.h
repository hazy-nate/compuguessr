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

#define CG_ENV_FASTCGI_SOCK_KEY "FASTCGI_SOCK"
#define CG_ENV_FASTCGI_SOCK_KEY_LEN 12
#define CG_ENV_FASTCGI_SOCK_DEFAULT "/var/run/compuguessr/compuguessr.sock"
#define CG_ENV_FASTCGI_SOCK_DEFAULT_LEN 36
#define CG_ENV_RESP_SOCK_KEY "RESP_SOCK"
#define CG_ENV_RESP_SOCK_KEY_LEN 9
#define CG_ENV_RESP_SOCK_DEFAULT "/var/run/valkey/valkey.sock"
#define CG_ENV_RESP_SOCK_DEFAULT_LEN 26

/****s* cg_env/cg_env_config
 * NAME
 *   cg_env_config
 * SOURCE
 */
struct cg_env {
	char *fastcgi_sock;
	int fastcgi_sock_len;
	char *resp_sock;
	int resp_sock_len;
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
