#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include "arch/cg_syscall.h"
#include "data/cg_userdb.h"
#include "sys/cg_types.h"
#include "util/cg_util.h"

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
			db->pool[i].pass_hash = cg_hash_str(password, cg_strlen_avx2((char *)password));
			db->pool[i].total_points = 0;
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
