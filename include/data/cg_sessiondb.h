#ifndef CG_SESSIONDB_H
#define CG_SESSIONDB_H

/****h* data/cg_sessiondb.h
 * NAME
 *   cg_sessiondb.h
 * FUNCTION
 *   Persistent session database structures, including the file
 *   header and memory-mapped entry pool definitions.
 ******/

/*==============================================================================
 * SYSTEM HEADERS
 *============================================================================*/

#include "data/cg_hashmap.h"
#include <stdint.h>

/*==============================================================================
 * DEFINITIONS
 *============================================================================*/

#define CG_SESSIONDB_MAGIC_BYTES 0x47535345535F4743
#define CG_SESSIONDB_POOL_SIZE 4096
#define CG_SESSIONDB_MASK (CG_SESSIONDB_POOL_SIZE - 1)
#define CG_SESSIONDB_FILE_SIZE (sizeof(struct cg_sessiondb))

/*==============================================================================
 * STRUCTS
 *============================================================================*/

struct cg_sessiondb_header {
	uint64_t magic;
	uint32_t version;
	uint32_t entry_count;
	uint32_t entry_size;
};

struct cg_sessiondb_entry {
	uint64_t session_id;
	uint32_t user_id;
	uint32_t creation_timestamp;
	uint32_t last_seen_timestamp;
	uint8_t is_active;
	uint8_t flags;
};

struct cg_sessiondb {
	struct cg_sessiondb_header hdr;
	struct cg_hashmap map;
	struct cg_hashmap_entry map_entries[CG_SESSIONDB_POOL_SIZE];
	struct cg_sessiondb_entry pool[CG_SESSIONDB_POOL_SIZE];
};

/*==============================================================================
 * FUNCTION PROTOTYPES
 *============================================================================*/

struct cg_sessiondb *cg_sessiondb_init(const char *fp);
uint64_t cg_sessiondb_session_id_get(void);
uint64_t cg_sessiondb_index_get(uint64_t);
struct cg_sessiondb_entry *cg_sessiondb_entry_get(struct cg_sessiondb *, uint64_t);
int cg_sessiondb_entry_create(struct cg_sessiondb *, uint32_t, struct cg_sessiondb_entry *);
void cg_sessiondb_entry_delete(struct cg_sessiondb *, uint64_t);

#endif /* !CG_SESSIONDB_H */
