/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* data/cg_sessiondb.c
 * NAME
 *   cg_sessiondb.c
 * FUNCTION
 *   Persistent session management using memory-mapped files. Handles
 *   secure session ID generation and user-to-session mapping. [cite: 426-442]
 ******/

#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include "arch/cg_syscall.h"
#include "data/cg_sessiondb.h"
#include "platform/cg_time.h"
#include "sys/cg_types.h"
#include "util/cg_util.h"

struct cg_sessiondb *g_sessiondb = 0;

/****f* data/cg_sessiondb_init
 * NAME
 *   cg_sessiondb_init
 * SYNOPSIS
 *   struct cg_sessiondb *cg_sessiondb_init(const char *fp)
 * FUNCTION
 *   Initializes the session database by opening or creating a file
 *   at the specified path and mapping it into memory [cite: 426-428]. If
 *   the file is new, it truncates it to the required size, zeroes
 *   the memory, and initializes the database header [cite: 427, 429-430]. It
 *   also validates the magic bytes for existing files and
 *   configures the internal hashmap pointers [cite: 431-432].
 * INPUTS
 *   * fp - The filesystem path to the database file[cite: 426].
 * RESULT
 *   * A pointer to the memory-mapped cg_sessiondb structure, or
 *   0 on failure [cite: 426, 429, 431-433].
 * SOURCE
 */
struct cg_sessiondb *
cg_sessiondb_init(const char *fp)
{
	int fd = (int)syscall3(SYS_open, (long)fp, O_RDWR | O_CREAT, 0600);
	if (fd < 0) {
		return 0;
	}
	struct stat st;
	syscall2(SYS_fstat, fd, (long)&st);
	int is_new = (st.st_size == 0);
	if (is_new) {
		syscall2(SYS_ftruncate, fd, CG_SESSIONDB_FILE_SIZE);
	}
	union cg_ptr map;
	map.addr = syscall6(SYS_mmap, 0, CG_SESSIONDB_FILE_SIZE,
				     PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	syscall1(SYS_close, fd);
	if (map.ptr == MAP_FAILED) {
		return 0;
	}
	struct cg_sessiondb *db = (struct cg_sessiondb *)map.ptr;
	if (is_new) {
		cg_memset_avx2(map.ptr, 0, CG_SESSIONDB_FILE_SIZE);
		db->hdr.magic = CG_SESSIONDB_MAGIC_BYTES;
		db->hdr.version = 1;
		db->hdr.entry_count = 0;
		db->hdr.entry_size = sizeof(struct cg_sessiondb_entry);
	} else if (db->hdr.magic != CG_SESSIONDB_MAGIC_BYTES) {
		syscall2(SYS_munmap, map.addr, CG_SESSIONDB_FILE_SIZE);
		return 0;
	}

	/* Initialize volatile pointer for the hashmap mapped in memory */
	db->map.entries = db->map_entries;
	db->map.capacity = CG_SESSIONDB_POOL_SIZE;
	db->map.count = db->hdr.entry_count;
	return db;
}
/******/

/****f* data/cg_sessiondb_session_id_get
 * NAME
 *   cg_sessiondb_session_id_get
 * SYNOPSIS
 *   uint64_t cg_sessiondb_session_id_get(void)
 * FUNCTION
 *   Generates a cryptographically secure 64-bit session
 *   identifier using the getrandom syscall [cite: 433-434].
 * RESULT
 *   * A non-zero 64-bit session ID, or 0 if the syscall fails [cite: 433-434].
 * SOURCE
 */
uint64_t
cg_sessiondb_session_id_get(void)
{
	uint64_t id = 0;
	long res = syscall3(SYS_getrandom, (long)&id, sizeof(id), 0);
	if (res < (long)sizeof(id)) {
		return 0;
	}
	if (id == 0) {
		id = 1;
	}
	return id;
}
/******/

/****f* data/cg_sessiondb_index_get
 * NAME
 *   cg_sessiondb_index_get
 * SYNOPSIS
 *   uint64_t cg_sessiondb_index_get(uint64_t sid)
 * FUNCTION
 *   Calculates the hash index for a given session ID using
 *   the database bitmask [cite: 434-435].
 * INPUTS
 *   * sid - The 64-bit session identifier[cite: 434].
 * RESULT
 *   * The calculated index within the database pool [cite: 434-435].
 * SOURCE
 */
uint64_t
cg_sessiondb_index_get(uint64_t sid)
{
	uint64_t hash = cg_hash_uint64(sid);
	return (uint32_t)(hash & CG_SESSIONDB_MASK);
}
/******/

/****f* data/cg_sessiondb_entry_get
 * NAME
 *   cg_sessiondb_entry_get
 * SYNOPSIS
 *   struct cg_sessiondb_entry *cg_sessiondb_entry_get(struct cg_sessiondb *db, uint64_t sid)
 * FUNCTION
 *   Retrieves a pointer to a session entry from the database pool
 *   using the provided session ID [cite: 435-436]. It performs a
 *   lookup in the internal hashmap to find the entry index[cite: 435].
 * INPUTS
 *   * db  - Pointer to the session database[cite: 435].
 *   * sid - The 64-bit session ID to search for[cite: 435].
 * RESULT
 *   * A pointer to the cg_sessiondb_entry if found and active,
 *   otherwise 0 [cite: 435-436].
 * SOURCE
 */
struct cg_sessiondb_entry *
cg_sessiondb_entry_get(struct cg_sessiondb *db, uint64_t sid)
{
	struct cg_hashmap_entry *hentry = cg_hashmap_get_key_uint(&db->map, sid);
	if (hentry && hentry->occupied) {
		return &db->pool[hentry->val.addr];
	}
	return 0;
}
/******/

/****f* data/cg_sessiondb_entry_create
 * NAME
 *   cg_sessiondb_entry_create
 * SYNOPSIS
 *   int cg_sessiondb_entry_create(struct cg_sessiondb *db, uint32_t uid, struct cg_sessiondb_entry *out)
 * FUNCTION
 *   Allocates and initializes a new session entry in the database
 *   pool [cite: 436-441]. It uses an atomic compare-and-exchange operation
 *   to safely claim an inactive slot in the pool, generates a
 *   new session ID, and updates the hashmap [cite: 438-441].
 * INPUTS
 *   * db  - Pointer to the session database[cite: 436].
 *   * uid - The user ID to associate with the new session[cite: 436, 440].
 *   * out - Pointer to a structure that will receive a copy of
 *   the created entry[cite: 436, 441].
 * RESULT
 *   * 0 on success, or 1 if the database is full or ID
 *   generation fails [cite: 436-441].
 * SOURCE
 */
int
cg_sessiondb_entry_create(struct cg_sessiondb *db, uint32_t uid, struct cg_sessiondb_entry *out)
{
	if (db->map.count >= CG_SESSIONDB_POOL_SIZE) {
		return 1;
	}
	uint64_t sid = cg_sessiondb_session_id_get();
	if (sid == 0) {
		return 1;
	}

	struct cg_sessiondb_entry *entry = 0;
	uint32_t pool_idx = 0;
	for (uint32_t i = 0; i < CG_SESSIONDB_POOL_SIZE; i++) {
		if (!db->pool[i].is_active) {
			uint8_t expected = 0;
			if (__atomic_compare_exchange_n(&db->pool[i].is_active, &expected, 1,
							false, __ATOMIC_SEQ_CST,
							__ATOMIC_SEQ_CST)) {
				entry = &db->pool[i];
				pool_idx = i;
				break;
			}
		}
	}

	if (!entry) {
		return 1;
	}

	entry->session_id = sid;
	entry->user_id = uid;
	entry->creation_timestamp = cg_time_get();
	entry->last_seen_timestamp = entry->creation_timestamp;
	entry->flags = 0;

	union cg_ptr val;
	val.addr = pool_idx;
	cg_hashmap_insert_key_uint(&db->map, sid, val);
	db->hdr.entry_count++;

	cg_memcpy_avx2(out, entry, sizeof(struct cg_sessiondb_entry));
	return 0;
}
/******/

/****f* data/cg_sessiondb_entry_delete
 * NAME
 *   cg_sessiondb_entry_delete
 * SYNOPSIS
 *   void cg_sessiondb_entry_delete(struct cg_sessiondb *db, uint64_t sid)
 * FUNCTION
 *   Deactivates a session entry and removes its associated key
 *   from the hashmap [cite: 441-442].
 * INPUTS
 *   * db  - Pointer to the session database[cite: 441].
 *   * sid - The 64-bit session ID to delete[cite: 441].
 * SOURCE
 */
void
cg_sessiondb_entry_delete(struct cg_sessiondb *db, uint64_t sid)
{
	struct cg_sessiondb_entry *entry = cg_sessiondb_entry_get(db, sid);
	if (entry) {
		entry->is_active = 0;
		cg_hashmap_delete_key_uint(&db->map, sid);
		db->hdr.entry_count--;
	}
}
/******/
