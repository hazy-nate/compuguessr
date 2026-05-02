/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* data/cg_challengedb.c
 * NAME
 *   cg_challengedb.c
 * FUNCTION
 *   Read-only challenge database provider. Implements binary search
 *   lookups for static challenge data with pointer relocation.
 ******/

#include <string.h>

#include "data/cg_challengedb_data.h"
#include "data/cg_challengedb.h"
#include "util/cg_util.h"

/****f* cg_challengedb/cg_challengedb_find
 * NAME
 *   cg_challengedb_find
 * SYNOPSIS
 *   const struct cg_challengedb *cg_challengedb_find(const char *id)
 * FUNCTION
 *   Performs a binary search through the static challenge database to
 *   locate a challenge by its string identifier. It
 *   utilizes the CG_RELOC macro to handle pointer relocation for the
 *   id_string within each database entry during comparison.
 * INPUTS
 *   * id - A null-terminated string containing the challenge ID to
 *   locate.
 * RESULT
 *   * A pointer to the matching cg_challengedb structure if found,
 *   otherwise 0.
 * SOURCE
 */
const struct cg_challengedb *
cg_challengedb_find(const char *id)
{
	int left = 0;
	int right = (int)g_challengedb_size - 1;
	while (left <= right) {
		int mid = left + ((right - left) / 2);
		// NOLINTNEXTLINE
		const char *relocated_id = CG_RELOC(const char *, g_challengedb[mid].id_string);

		int cmp = cg_strcmp(relocated_id, id);
		if (cmp == 0) {
			return &g_challengedb[mid]; /* Found it! */
		}
		if (cmp < 0) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	return 0;
}
/******/
