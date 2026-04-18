#include <stdint.h>

#include "data/cg_hashmap.h"
#include "util/cg_util.h"

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

struct cg_hashmap_entry *
cg_hashmap_get_key_str(struct cg_hashmap *map, char *key, uint64_t key_len)
{
	uint64_t ikey = cg_hash_str(key, key_len);
	return cg_hashmap_get_key_uint(map, ikey);
}

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

void
cg_hashmap_insert_key_str(struct cg_hashmap *map, char *key, uint64_t key_len, union cg_ptr val)
{
	uint64_t ikey = cg_hash_str(key, key_len);
	cg_hashmap_insert_key_uint(map, ikey, val);
}

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
