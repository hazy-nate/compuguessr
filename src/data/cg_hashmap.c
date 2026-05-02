/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* data/cg_hashmap.c
 * NAME
 *   cg_hashmap.c
 * FUNCTION
 *   Generic hashmap implementation using linear probing. Supports
 *   64-bit integer keys and string keys for internal data routing.
 ******/

#include <stdint.h>

#include "data/cg_hashmap.h"
#include "util/cg_util.h"

/****f* data/cg_hashmap_get_key_uint
 * NAME
 *   cg_hashmap_get_key_uint
 * SYNOPSIS
 *   struct cg_hashmap_entry *cg_hashmap_get_key_uint(struct cg_hashmap *map, uint64_t key)
 * FUNCTION
 *   Retrieves an entry from the hashmap using a 64-bit unsigned integer
 *   key. It performs a hash calculation and uses linear probing with a
 *   bitmask to resolve collisions and find the matching entry.
 * INPUTS
 *   * map - Pointer to the hashmap structure.
 *   * key - The 64-bit unsigned integer key to search for.
 * RESULT
 *   * A pointer to the cg_hashmap_entry if found and occupied, otherwise 0.
 * SOURCE
 */
struct cg_hashmap_entry *
cg_hashmap_get_key_uint(struct cg_hashmap *map, uint64_t key)
{
	if (map->entries == 0 || map->capacity == 0) {
		return 0;
	}
	uint64_t hash = cg_hash_uint64(key);
	uint32_t mask = map->capacity - 1;
	uint32_t idx = (uint32_t)(hash & mask);
	while (map->entries[idx].occupied != 0) {
		if (map->entries[idx].occupied == 1 && map->entries[idx].key == key) {
			return &map->entries[idx];
		}
		idx = (idx + 1) & mask;
	}
	return 0;
}
/******/

/****f* data/cg_hashmap_get_key_str
 * NAME
 *   cg_hashmap_get_key_str
 * SYNOPSIS
 *   struct cg_hashmap_entry *cg_hashmap_get_key_str(struct cg_hashmap *map, char *key, uint64_t key_len)
 * FUNCTION
 *   Retrieves an entry from the hashmap using a string key. It first
 *   hashes the string to a 64-bit integer and then calls
 *   cg_hashmap_get_key_uint to perform the lookup.
 * INPUTS
 *   * map     - Pointer to the hashmap structure.
 *   * key     - Pointer to the string key.
 *   * key_len - The length of the string key.
 * RESULT
 *   * A pointer to the cg_hashmap_entry if found, otherwise 0.
 * SOURCE
 */
struct cg_hashmap_entry *
cg_hashmap_get_key_str(struct cg_hashmap *map, char *key, uint64_t key_len)
{
	uint64_t ikey = cg_hash_str(key, key_len);
	return cg_hashmap_get_key_uint(map, ikey);
}
/******/

/****f* data/cg_hashmap_insert_key_uint
 * NAME
 *   cg_hashmap_insert_key_uint
 * SYNOPSIS
 *   void cg_hashmap_insert_key_uint(struct cg_hashmap *map, uint64_t key, union cg_ptr val)
 * FUNCTION
 *   Inserts a new key-value pair into the hashmap using a 64-bit unsigned
 *   integer key. If the key already exists, the value is updated. It
 *   uses linear probing to find an available slot.
 * INPUTS
 *   * map - Pointer to the hashmap structure.
 *   * key - The 64-bit unsigned integer key.
 *   * val - The value (as a cg_ptr union) to store.
 * SOURCE
 */
void
cg_hashmap_insert_key_uint(struct cg_hashmap *map, uint64_t key, union cg_ptr val)
{
	if (map->entries == 0) {
		return;
	}

	struct cg_hashmap_entry *existing = cg_hashmap_get_key_uint(map, key);
	if (existing) {
		existing->val = val;
		return;
	}

	uint64_t hash = cg_hash_uint64(key);
	uint32_t mask = map->capacity - 1;
	uint32_t idx = (uint32_t)(hash & mask);
	while (map->entries[idx].occupied == 1) {
		idx = (idx + 1) & mask;
	}
	map->entries[idx].key = key;
	map->entries[idx].val = val;
	map->entries[idx].occupied = 1;
	map->count++;
}
/******/

/****f* data/cg_hashmap_insert_key_str
 * NAME
 *   cg_hashmap_insert_key_str
 * SYNOPSIS
 *   void cg_hashmap_insert_key_str(struct cg_hashmap *map, char *key, uint64_t key_len, union cg_ptr val)
 * FUNCTION
 *   Inserts a new key-value pair into the hashmap using a string key.
 *   It hashes the string to a 64-bit integer and calls
 *   cg_hashmap_insert_key_uint.
 * INPUTS
 *   * map     - Pointer to the hashmap structure.
 *   * key     - Pointer to the string key.
 *   * key_len - The length of the string key.
 *   * val     - The value (as a cg_ptr union) to store.
 * SOURCE
 */
void
cg_hashmap_insert_key_str(struct cg_hashmap *map, char *key, uint64_t key_len, union cg_ptr val)
{
	uint64_t ikey = cg_hash_str(key, key_len);
	cg_hashmap_insert_key_uint(map, ikey, val);
}
/******/

/****f* data/cg_hashmap_delete_key_uint
 * NAME
 *   cg_hashmap_delete_key_uint
 * SYNOPSIS
 *   void cg_hashmap_delete_key_uint(struct cg_hashmap *map, uint64_t key)
 * FUNCTION
 *   Removes an entry from the hashmap by key. It marks the matching
 *   entry's occupied status as '2' (deleted/tombstone) and decrements
 *   the map count.
 * INPUTS
 *   * map - Pointer to the hashmap structure.
 *   * key - The 64-bit unsigned integer key to delete.
 * SOURCE
 */
void
cg_hashmap_delete_key_uint(struct cg_hashmap *map, uint64_t key)
{
        if (map->entries == 0) {
                return;
        }
        struct cg_hashmap_entry *entry = cg_hashmap_get_key_uint(map, key);
        if (entry) {
                entry->occupied = 2;
                map->count--;
        }
}
/******/
