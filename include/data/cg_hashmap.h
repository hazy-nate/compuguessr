#ifndef CG_HASHMAP_H
#define CG_HASHMAP_H

/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* data/cg_hashmap.h, data/cg_hashmap
 * NAME
 *   cg_hashmap.h
 ******/

/*==============================================================================
 * SYSTEM HEADERS
 *============================================================================*/

#include <stdint.h>

/*==============================================================================
 * LOCAL HEADERS
 *============================================================================*/

#include "sys/cg_types.h"

/*==============================================================================
 * STRUCTS
 *============================================================================*/

struct cg_hashmap_entry {
	uint64_t key;
	union cg_ptr val;
	uint8_t occupied;
};

struct cg_hashmap {
	struct cg_hashmap_entry *entries;
	uint32_t capacity;
	uint32_t count;
};

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

struct cg_hashmap_entry *cg_hashmap_get_key_uint(struct cg_hashmap *map, uint64_t key);
struct cg_hashmap_entry *cg_hashmap_get_key_str(struct cg_hashmap *map, char *key, uint64_t key_len);
void cg_hashmap_insert_key_uint(struct cg_hashmap *map, uint64_t key, union cg_ptr val);
void cg_hashmap_insert_key_str(struct cg_hashmap *map, char *key, uint64_t key_len, union cg_ptr val);
void cg_hashmap_delete_key_uint(struct cg_hashmap *map, uint64_t key);

#endif /* !CG_HASHMAP_H */
