/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* <module>/<name>
 * NAME
 *   <name>
 * FUNCTION
 *   <function>
 ******/

#include "sys/cg_env.h"
#include "sys/cg_types.h"
#include "util/cg_util.h"

void
cg_env_init(char **envp, struct cg_env *env)
{
	env->compuguessr_sock = "/var/run/compuguessr/compuguessr.sock";
	env->resp_sock = "/var/run/valkey/valkey.sock";
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
		if (key_len == 16 && cg_memcmp_avx2(entry, "COMPUGUESSR_SOCK", 16) == 0) {
			env->compuguessr_sock = value;
		}
		else if (key_len == 9 && cg_memcmp_avx2(entry, "RESP_SOCK", 9) == 0) {
			env->resp_sock = value;
		}
	}
}

