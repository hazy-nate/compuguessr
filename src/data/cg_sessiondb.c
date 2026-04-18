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

uint64_t
cg_sessiondb_index_get(uint64_t sid)
{
	uint64_t hash = cg_hash_uint64(sid);
	return (uint32_t)(hash & CG_SESSIONDB_MASK);
}

struct cg_sessiondb_entry *
cg_sessiondb_entry_get(struct cg_sessiondb *db, uint64_t sid)
{
	struct cg_hashmap_entry *hentry = cg_hashmap_get_key_uint(&db->map, sid);
	if (hentry && hentry->occupied) {
		return &db->pool[hentry->val.addr];
	}
	return 0;
}

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
