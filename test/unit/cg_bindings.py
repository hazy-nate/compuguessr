# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes

class CgUtoaResult(ctypes.Structure):
    _fields_ = [("ptr", ctypes.c_void_p), ("len", ctypes.c_uint64)]

class CgGenericUnion(ctypes.Union):
    _fields_ = [("func_ptr", ctypes.c_void_p), ("data_ptr", ctypes.c_void_p), ("addr", ctypes.c_uint64)]

class CgHashmapEntry(ctypes.Structure):
    _fields_ = [("key", ctypes.c_uint64), ("val", CgGenericUnion), ("occupied", ctypes.c_uint8), ("_pad", ctypes.c_uint8 * 7)]

class CgHashmap(ctypes.Structure):
    _fields_ = [("entries", ctypes.POINTER(CgHashmapEntry)), ("capacity", ctypes.c_uint32), ("count", ctypes.c_uint32)]

class CgUserDbHeader(ctypes.Structure):
    _fields_ = [("magic", ctypes.c_uint64), ("version", ctypes.c_uint32), ("count", ctypes.c_uint32)]

class CgUserDbEntry(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_uint32), ("username", ctypes.c_char * 32), ("description", ctypes.c_char * 2048),
        ("pass_hash", ctypes.c_uint64), ("total_points", ctypes.c_uint32), ("completed_count", ctypes.c_uint32),
        ("completed_hashes", ctypes.c_uint64 * 32), ("is_active", ctypes.c_uint8), ("_pad", ctypes.c_uint8 * 7)
    ]

class CgUserDb(ctypes.Structure):
    _fields_ = [("hdr", CgUserDbHeader), ("pool", CgUserDbEntry * 1024)]

class CgSessionDbEntry(ctypes.Structure):
    _fields_ = [
        ("session_id", ctypes.c_uint64), ("user_id", ctypes.c_uint32), ("flags", ctypes.c_uint32),
        ("creation_timestamp", ctypes.c_uint64), ("last_seen_timestamp", ctypes.c_uint64),
        ("is_active", ctypes.c_uint8), ("_pad", ctypes.c_uint8 * 7)
    ]

class CgEnv(ctypes.Structure):
    _fields_ = [("fastcgi_sock", ctypes.c_char_p), ("fastcgi_sock_len", ctypes.c_uint64)]

class CgBufferMetadata(ctypes.Structure):
    _fields_ = [
        ("buf", ctypes.c_void_p), # Pointer to the underlying memory
        ("len", ctypes.c_uint32), # Max capacity
        ("valid", ctypes.c_uint32), # Bytes written
        ("pos", ctypes.c_uint32)  # Current read position
    ]

def _safe_bind(lib, func_name, argtypes, restype):
    if hasattr(lib, func_name):
        func = getattr(lib, func_name)
        func.argtypes = argtypes
        func.restype = restype

def apply_bindings(lib):
    # Core Utilities
    _safe_bind(lib, "cg_utoa", [ctypes.c_uint64, ctypes.c_char_p], CgUtoaResult)
    _safe_bind(lib, "cg_itoa", [ctypes.c_int64, ctypes.c_char_p], CgUtoaResult)
    _safe_bind(lib, "cg_strlen", [ctypes.c_char_p], ctypes.c_uint64)
    _safe_bind(lib, "cg_strlen_avx2", [ctypes.c_char_p], ctypes.c_uint64)
    _safe_bind(lib, "cg_strlen_sse", [ctypes.c_char_p], ctypes.c_uint64)
    _safe_bind(lib, "cg_hash_str", [ctypes.c_char_p, ctypes.c_uint64], ctypes.c_uint64)
    _safe_bind(lib, "cg_strcmp", [ctypes.c_char_p, ctypes.c_char_p], ctypes.c_int)
    _safe_bind(lib, "cg_memcpy_avx2", [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int], ctypes.c_void_p)
    _safe_bind(lib, "cg_memcmp_avx2", [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int], ctypes.c_int)
    _safe_bind(lib, "cg_memset_avx2", [ctypes.c_void_p, ctypes.c_int, ctypes.c_uint64], ctypes.c_void_p)
    _safe_bind(lib, "cg_memslide", [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int], ctypes.c_char_p)
    _safe_bind(lib, "cg_get_env", [ctypes.POINTER(ctypes.c_char_p), ctypes.c_char_p], ctypes.c_char_p)

    # Time & VDSO
    _safe_bind(lib, "cg_datetime_str_get", [ctypes.c_char_p], None)
    _safe_bind(lib, "cg_time_get", [], ctypes.c_uint64)

    # Client Pool
    _safe_bind(lib, "cg_client_pool_init", [ctypes.c_void_p, ctypes.c_int], None)
    _safe_bind(lib, "cg_client_alloc", [ctypes.c_void_p, ctypes.c_int], ctypes.c_void_p)
    _safe_bind(lib, "cg_client_free", [ctypes.c_void_p], None)

    # Databases
    _safe_bind(lib, "cg_userdb_init", [ctypes.c_char_p], ctypes.POINTER(CgUserDb))
    _safe_bind(lib, "cg_userdb_create", [ctypes.POINTER(CgUserDb), ctypes.c_char_p, ctypes.c_char_p, ctypes.POINTER(CgUserDbEntry)], ctypes.c_int)
    _safe_bind(lib, "cg_userdb_find", [ctypes.POINTER(CgUserDb), ctypes.c_char_p], ctypes.POINTER(CgUserDbEntry))
    _safe_bind(lib, "cg_userdb_find_by_id", [ctypes.POINTER(CgUserDb), ctypes.c_uint32], ctypes.POINTER(CgUserDbEntry))
    _safe_bind(lib, "cg_userdb_delete", [ctypes.POINTER(CgUserDb), ctypes.c_uint32], ctypes.c_int)

    _safe_bind(lib, "cg_sessiondb_init", [ctypes.c_char_p], ctypes.c_void_p)
    _safe_bind(lib, "cg_sessiondb_entry_create", [ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(CgSessionDbEntry)], ctypes.c_int)
    _safe_bind(lib, "cg_sessiondb_entry_get", [ctypes.c_void_p, ctypes.c_uint64], ctypes.POINTER(CgSessionDbEntry))
    _safe_bind(lib, "cg_sessiondb_session_id_get", [], ctypes.c_uint64)
    _safe_bind(lib, "cg_sessiondb_entry_delete", [ctypes.c_void_p, ctypes.c_uint64], None)

    _safe_bind(lib, "cg_challengedb_find", [ctypes.c_char_p], ctypes.c_void_p)

    # Hashmap & Routing
    _safe_bind(lib, "cg_hashmap_insert_key_str", [ctypes.POINTER(CgHashmap), ctypes.c_char_p, ctypes.c_uint64, CgGenericUnion], None)
    _safe_bind(lib, "cg_hashmap_get_key_str", [ctypes.POINTER(CgHashmap), ctypes.c_char_p, ctypes.c_uint64], ctypes.POINTER(CgHashmapEntry))
    _safe_bind(lib, "cg_hashmap_delete_key_uint", [ctypes.POINTER(CgHashmap), ctypes.c_uint64], None)

    _safe_bind(lib, "cg_env_init", [ctypes.POINTER(ctypes.c_char_p), ctypes.POINTER(CgEnv)], None)
    _safe_bind(lib, "cg_route_map_init", [ctypes.POINTER(CgHashmap)], None)

    # core/cg_fastcgi.c
    _safe_bind(lib, "cg_fastcgi_stdout", [ctypes.POINTER(CgBufferMetadata), ctypes.c_uint16, ctypes.c_char_p, ctypes.c_uint32], None)
    _safe_bind(lib, "cg_fastcgi_end", [ctypes.POINTER(CgBufferMetadata), ctypes.c_uint16], None)

    # platform/cg_log.c
    _safe_bind(lib, "cg_log_write", [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p], None)
    _safe_bind(lib, "cg_log_write_ts", [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p], None)

    return lib
