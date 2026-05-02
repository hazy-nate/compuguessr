# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
from cg_bindings import CgHashmap, CgHashmapEntry, CgGenericUnion

def test_hashmap_lifecycle(lib):
    entries = (CgHashmapEntry * 64)()
    hm = CgHashmap(entries=entries, capacity=64, count=0)
    key = b"/api/test"
    val = CgGenericUnion(addr=0xCAFEBABE)
    lib.cg_hashmap_insert_key_str(ctypes.byref(hm), key, len(key), val)
    entry_ptr = lib.cg_hashmap_get_key_str(ctypes.byref(hm), key, len(key))
    assert entry_ptr
    assert entry_ptr.contents.val.addr == 0xCAFEBABE
    missing = lib.cg_hashmap_get_key_str(ctypes.byref(hm), b"/api/404", 8)
    assert not missing

def test_hashmap_collisions(lib):
    # Using a small power-of-two capacity (4)
    # Inserting 2 keys ensures collisions occur without filling to 100% (which causes freeze)
    entries = (CgHashmapEntry * 4)()
    hm = CgHashmap(entries=entries, capacity=4, count=0)
    keys = [b"route_a", b"route_b"]
    for i, k in enumerate(keys):
        lib.cg_hashmap_insert_key_str(ctypes.byref(hm), k, len(k), CgGenericUnion(addr=i+1))
    for i, k in enumerate(keys):
        res = lib.cg_hashmap_get_key_str(ctypes.byref(hm), k, len(k))
        assert res is not None
        assert res.contents.val.addr == i+1
