# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
from cg_bindings import CgUserDbEntry

def test_userdb_initialization(lib, temp_db_path):
    db_ptr = lib.cg_userdb_init(temp_db_path)
    assert db_ptr
    assert db_ptr.contents.hdr.magic == 0x53524553555F4743
    assert db_ptr.contents.hdr.count == 0

def test_userdb_crud_operations(lib, temp_db_path):
    db_ptr = lib.cg_userdb_init(temp_db_path)
    out_entry = CgUserDbEntry()
    res = lib.cg_userdb_create(db_ptr, b"Player1", b"Secret", ctypes.byref(out_entry))
    assert res == 0
    assert out_entry.username == b"Player1"
    p1_ptr = lib.cg_userdb_find(db_ptr, b"Player1")
    assert p1_ptr
    lib.cg_userdb_delete(db_ptr, 1)
    assert not lib.cg_userdb_find(db_ptr, b"Player1")
