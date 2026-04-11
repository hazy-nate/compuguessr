
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
	struct cg_sessiondb_header *hdr = (struct cg_sessiondb_header *)map.ptr;
	if (is_new) {
		cg_memset_avx2(map.ptr, 0, CG_SESSIONDB_FILE_SIZE);
		hdr->magic = CG_SESSIONDB_MAGIC_BYTES;
		hdr->version = 1;
		hdr->entry_count = CG_SESSIONDB_POOL_SIZE;
		hdr->entry_size = sizeof(struct cg_sessiondb_entry);
	} else if (hdr->magic != CG_SESSIONDB_MAGIC_BYTES) {
		syscall2(SYS_munmap, map.addr, CG_SESSIONDB_FILE_SIZE);
		return 0;
	}
	return map.ptr;
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
	uint32_t start_idx = cg_sessiondb_index_get(sid);
	for (uint32_t i = 0; i < CG_SESSIONDB_POOL_SIZE; i++) {
		uint32_t idx = (start_idx + i) & CG_SESSIONDB_MASK;
		struct cg_sessiondb_entry *entry = &db->entries[idx];
		if (!entry->is_active) {
			return 0;
		}
		if (entry->session_id == sid) {
			return entry;
		}
	}
	return 0;
}

int
cg_sessiondb_entry_create(struct cg_sessiondb *db, uint32_t uid, struct cg_sessiondb_entry *out)
{
	uint64_t sid = cg_sessiondb_session_id_get();
	if (sid == 0) {
		return 1;
	}
	uint64_t hash = cg_hash_uint64(sid);
	uint32_t start_idx = hash & CG_SESSIONDB_MASK;
	for (uint32_t i = 0; i < CG_SESSIONDB_POOL_SIZE; i++) {
		uint32_t idx = (start_idx + i) & CG_SESSIONDB_MASK;
		struct cg_sessiondb_entry *entry = &db->entries[idx];
		uint8_t expected = 0;
		if (__atomic_compare_exchange_n(&entry->is_active, &expected, 1,
						false, __ATOMIC_SEQ_CST,
						__ATOMIC_SEQ_CST)) {
			entry->session_id = sid;
			entry->user_id = uid;
			entry->creation_timestamp = cg_time_get();
			entry->last_seen_timestamp = entry->creation_timestamp;
			entry->flags = 0;
			cg_memcpy_avx2(out, entry, sizeof(struct cg_sessiondb_entry));
			return 0;
		}
	}
	return 1;
}

void
cg_sessiondb_entry_delete(struct cg_sessiondb *db, uint64_t sid)
{
	struct cg_sessiondb_entry *entry = cg_sessiondb_entry_get(db, sid);
	cg_memset_avx2(entry, 0, sizeof(*entry));
}
