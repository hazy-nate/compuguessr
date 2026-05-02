# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
from cg_bindings import CgSessionDbEntry

def test_sessiondb_lifecycle(lib, temp_db_path):
    sdb_ptr = lib.cg_sessiondb_init(temp_db_path)
    assert sdb_ptr
    out_session = CgSessionDbEntry()
    res = lib.cg_sessiondb_entry_create(sdb_ptr, 42, ctypes.byref(out_session))
    assert res == 0
    assert out_session.user_id == 42
    assert out_session.session_id > 0
    retrieved_ptr = lib.cg_sessiondb_entry_get(sdb_ptr, out_session.session_id)
    assert retrieved_ptr
    assert retrieved_ptr.contents.user_id == 42

def test_sessiondb_random_generation(lib):
    if not hasattr(lib, "cg_sessiondb_session_id_get"):
        pytest.skip("cg_sessiondb_session_id_get not exported")

    # Ensure the getrandom syscall wrapper is providing unique 64-bit cryptographic integers
    id1 = lib.cg_sessiondb_session_id_get()
    id2 = lib.cg_sessiondb_session_id_get()
    assert id1 > 0
    assert id2 > 0
    assert id1 != id2

def test_sessiondb_deletion(lib, temp_db_path):
    if not hasattr(lib, "cg_sessiondb_entry_delete"):
        pytest.skip("cg_sessiondb_entry_delete not exported")

    sdb_ptr = lib.cg_sessiondb_init(temp_db_path)

    # Create
    out_session = CgSessionDbEntry()
    lib.cg_sessiondb_entry_create(sdb_ptr, 99, ctypes.byref(out_session))

    # Delete
    lib.cg_sessiondb_entry_delete(sdb_ptr, out_session.session_id)

    # Verify it is no longer retrievable
    invalid_ptr = lib.cg_sessiondb_entry_get(sdb_ptr, out_session.session_id)
    assert not invalid_ptr
