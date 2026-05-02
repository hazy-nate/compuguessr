/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 2026 Nathaniel Williams */

/****h* data/cg_userdb.c
 * NAME
 *   cg_userdb.c
 * FUNCTION
 *   Persistent user account storage. Manages user registration,
 *   authentication via password hashing, and profile data persistence. [cite: 474-488]
 ******/

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include "arch/cg_syscall.h"
#include "data/cg_userdb.h"
#include "sys/cg_types.h"
#include "util/cg_util.h"

struct cg_userdb *g_userdb = 0;

/****f* data/cg_userdb_init
 * NAME
 *   cg_userdb_init
 * SYNOPSIS
 *   struct cg_userdb *cg_userdb_init(const char *fp)
 * FUNCTION
 *   Initializes the user database by opening or creating a file at the
 *   specified path and mapping it into memory. If the file is newly
 *   created, it truncates the file to the appropriate size, zeroes the
 *   memory, and initializes the header with magic bytes, version, and
 *   an initial count. For existing files, it validates the magic bytes
 *   before returning the mapping.
 * INPUTS
 *   * fp - The filesystem path to the user database file.
 * RESULT
 *   * A pointer to the memory-mapped cg_userdb structure, or 0 on failure.
 * SOURCE
 */
struct cg_userdb *
cg_userdb_init(const char *fp)
{
	int fd = (int)syscall3(SYS_open, (long)fp, O_RDWR | O_CREAT, 0600);
	if (fd < 0) {
		return 0;
	}
	struct stat st;
	syscall2(SYS_fstat, fd, (long)&st);
	int is_new = (st.st_size == 0);
	if (is_new) {
		syscall2(SYS_ftruncate, fd, sizeof(struct cg_userdb));
	}
	union cg_ptr map;
	map.addr = syscall6(SYS_mmap, 0, sizeof(struct cg_userdb),
				     PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	syscall1(SYS_close, fd);
	if (map.ptr == MAP_FAILED) {
		return 0;
	}
	struct cg_userdb *db = (struct cg_userdb *)map.ptr;
	if (is_new) {
		cg_memset_avx2(map.ptr, 0, sizeof(struct cg_userdb));
		db->hdr.magic = CG_USERDB_MAGIC_BYTES;
		db->hdr.version = 1;
		db->hdr.count = 0;
	} else if (db->hdr.magic != CG_USERDB_MAGIC_BYTES) {
		syscall2(SYS_munmap, map.addr, sizeof(struct cg_userdb));
		return 0;
	}
	return db;
}
/******/

/****f* data/cg_userdb_find
 * NAME
 *   cg_userdb_find
 * SYNOPSIS
 *   struct cg_userdb_entry *cg_userdb_find(struct cg_userdb *db, const char *username)
 * FUNCTION
 *   Iterates through the user database pool to find an active entry
 *   matching the provided username.
 * INPUTS
 *   * db       - Pointer to the user database.
 *   * username - Null-terminated string containing the username to locate.
 * RESULT
 *   * A pointer to the matching cg_userdb_entry if found and active, otherwise 0.
 * SOURCE
 */
struct cg_userdb_entry *
cg_userdb_find(struct cg_userdb *db, const char *username)
{
	for (uint32_t i = 0; i < CG_USERDB_POOL_SIZE; i++) {
		if (db->pool[i].is_active && cg_strcmp(db->pool[i].username, username) == 0) {
			return &db->pool[i];
		}
	}
	return 0;
}
/******/

/****f* data/cg_userdb_find_by_id
 * NAME
 *   cg_userdb_find_by_id
 * SYNOPSIS
 *   struct cg_userdb_entry *cg_userdb_find_by_id(struct cg_userdb *db, uint32_t id)
 * FUNCTION
 *   Iterates through the user database pool to find an active entry
 *   matching the provided numeric user ID.
 * INPUTS
 *   * db - Pointer to the user database.
 *   * id - The numeric user identifier to search for.
 * RESULT
 *   * A pointer to the matching cg_userdb_entry if found and active, otherwise 0.
 * SOURCE
 */
struct cg_userdb_entry *
cg_userdb_find_by_id(struct cg_userdb *db, uint32_t id)
{
	for (uint32_t i = 0; i < CG_USERDB_POOL_SIZE; i++) {
		if (db->pool[i].is_active && db->pool[i].id == id) {
			return &db->pool[i];
		}
	}
	return 0;
}
/******/

/****f* data/cg_userdb_create
 * NAME
 *   cg_userdb_create
 * SYNOPSIS
 *   int cg_userdb_create(struct cg_userdb *db, const char *username, const char *password, struct cg_userdb_entry *out)
 * FUNCTION
 *   Creates a new user account in the database. It first checks if
 *   the username already exists. If unique, it locates an inactive
 *   slot in the pool, initializes the entry with the provided
 *   credentials (hashing the password), and increments the total user count.
 * INPUTS
 *   * db       - Pointer to the user database.
 *   * username - Null-terminated string for the new username.
 *   * password - Null-terminated string for the new password.
 *   * out      - Pointer to a structure that will receive a copy of
 *   the created entry.
 * RESULT
 *   * 0 on success.
 *   * 1 if the user already exists.
 *   * 2 if the database pool is full.
 * SOURCE
 */
int
cg_userdb_create(struct cg_userdb *db, const char *username, const char *password, struct cg_userdb_entry *out)
{
	if (cg_userdb_find(db, username)) {
		return 1;
	}
	for (uint32_t i = 0; i < CG_USERDB_POOL_SIZE; i++) {
		if (!db->pool[i].is_active) {
			db->pool[i].id = i + 1;
			int len = cg_strlen_avx2((char *)username);
			if (len > 31) len = 31;
			cg_memcpy_avx2(db->pool[i].username, (char *)username, len);
			db->pool[i].username[len] = '\0';
			cg_memset_avx2(db->pool[i].description, 0, 2048);
			db->pool[i].pass_hash = cg_hash_str(password, cg_strlen_avx2((char *)password));
			db->pool[i].total_points = 0;
			db->pool[i].completed_count = 0;
			cg_memset_avx2(db->pool[i].completed_hashes, 0, sizeof(uint64_t) * 32);
			db->pool[i].is_active = 1;
			db->hdr.count++;
			if (out) {
				cg_memcpy_avx2(out, &db->pool[i], sizeof(struct cg_userdb_entry));
			}
			return 0;
		}
	}
	return 2;
}
/******/

/****f* data/cg_userdb_delete
 * NAME
 *   cg_userdb_delete
 * SYNOPSIS
 *   int cg_userdb_delete(struct cg_userdb *db, uint32_t id)
 * FUNCTION
 *   Deactivates a user account by setting its is_active flag to 0
 *   and decrementing the database user count.
 * INPUTS
 *   * db - Pointer to the user database.
 *   * id - The numeric user identifier to delete.
 * RESULT
 *   * 0 if the user was successfully deleted.
 *   * 1 if the user was not found or already inactive.
 * SOURCE
 */
int
cg_userdb_delete(struct cg_userdb *db, uint32_t id)
{
	struct cg_userdb_entry *entry = cg_userdb_find_by_id(db, id);
	if (entry && entry->is_active) {
		entry->is_active = 0;
		if (db->hdr.count > 0) {
			db->hdr.count--;
		}
		return 0;
	}
	return 1;
}
/******/
