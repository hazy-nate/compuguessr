# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
from cg_bindings import CgHashmap, CgHashmapEntry

def test_route_map_initialization(lib):
    entries_array = (CgHashmapEntry * 32)()
    hm = CgHashmap(entries=entries_array, capacity=32, count=0)
    lib.cg_route_map_init(ctypes.byref(hm))
    endpoints = [b"/api/auth/guest", b"/api/auth/login", b"/api/user/profile"]
    for ep in endpoints:
        entry = lib.cg_hashmap_get_key_str(ctypes.byref(hm), ep, len(ep))
        assert entry is not None
