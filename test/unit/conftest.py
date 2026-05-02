# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
import os
import pytest
from cg_bindings import apply_bindings

@pytest.fixture(scope="session")
def lib():
    lib_path = os.path.abspath("bin/debug/libcompuguessr.so")
    if not os.path.exists(lib_path):
        pytest.fail(f"Library not found at {lib_path}. Run 'make' first.")

    lib = ctypes.CDLL(lib_path)
    return apply_bindings(lib)

@pytest.fixture
def temp_db_path(tmp_path):
    return str(tmp_path / "testdb.dat").encode('utf-8')
