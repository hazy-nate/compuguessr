/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* sys/cg_env.c, sys/cg_env
 * NAME
 *   cg_env.c
 ******/

#include "sys/cg_env.h"
#include "sys/cg_types.h"
#include "util/cg_util.h"

void
cg_env_init(char **envp, struct cg_env *env)
{
	if (!env) {
		return;
	}
	env->fastcgi_sock = CG_ENV_FASTCGI_SOCK_DEFAULT;
	env->fastcgi_sock_len = CG_ENV_FASTCGI_SOCK_DEFAULT_LEN;
	env->resp_sock = CG_ENV_RESP_SOCK_DEFAULT;
	env->resp_sock_len = CG_ENV_RESP_SOCK_DEFAULT_LEN;
	if (!envp) {
		return;
	}
	for (char **p = envp; *p != 0; p++) {
		char *entry = *p;
		char *eq = entry;
		while (*eq && *eq != '=') {
			eq++;
		}
		if (*eq != '=') {
			continue;
		}
		cg_size_t key_len = eq - entry;
		char *value = eq + 1;
		if (key_len == CG_ENV_FASTCGI_SOCK_KEY_LEN && cg_memcmp_avx2(entry, CG_ENV_FASTCGI_SOCK_KEY, CG_ENV_FASTCGI_SOCK_KEY_LEN) == 0) {
			env->fastcgi_sock = value;
			env->fastcgi_sock_len = cg_strlen_avx2(value);
		}
		else if (key_len == CG_ENV_RESP_SOCK_KEY_LEN && cg_memcmp_avx2(entry, CG_ENV_RESP_SOCK_KEY, CG_ENV_RESP_SOCK_KEY_LEN) == 0) {
			env->resp_sock = value;
			env->resp_sock_len = cg_strlen_avx2(value);
		}
	}
}
