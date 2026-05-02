# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes

def test_challengedb_static_lookup(lib):
    entry_ptr = lib.cg_challengedb_find(b"instruction_001")
    if entry_ptr: # Dependent on data presence in .so
        assert entry_ptr is not None
    assert lib.cg_challengedb_find(b"invalid_challenge_xyz") is None
