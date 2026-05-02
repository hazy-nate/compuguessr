#ifndef CG_USERDB_H
#define CG_USERDB_H

/****h* data/cg_userdb.h
 * NAME
 *   cg_userdb.h
 * FUNCTION
 *   Persistent user database definitions, including account
 *   entry structures and password hash storage.
 ******/

#include <stdint.h>

#define CG_USERDB_MAGIC_BYTES 0x53524553555F4743
#define CG_USERDB_POOL_SIZE 1024

struct cg_userdb_header {
	uint64_t magic;
	uint32_t version;
	uint32_t count;
};

struct cg_userdb_entry {
	uint32_t id;
	char username[32];
	char description[2048];
	uint64_t pass_hash;
	uint32_t total_points;
	uint32_t completed_count;
	uint64_t completed_hashes[32];
	uint8_t is_active;
};

struct cg_userdb {
	struct cg_userdb_header hdr;
	struct cg_userdb_entry pool[CG_USERDB_POOL_SIZE];
};

struct cg_userdb *cg_userdb_init(const char *fp);
struct cg_userdb_entry *cg_userdb_find(struct cg_userdb *db, const char *username);
struct cg_userdb_entry *cg_userdb_find_by_id(struct cg_userdb *db, uint32_t id);
int cg_userdb_create(struct cg_userdb *db, const char *username, const char *password, struct cg_userdb_entry *out);
int cg_userdb_delete(struct cg_userdb *db, uint32_t id);

#endif /* !CG_USERDB_H */
