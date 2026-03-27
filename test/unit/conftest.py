# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
import os
import pytest

class UtoaResult(ctypes.Structure):
    _fields_ = [
        ("ptr", ctypes.c_void_p),   # RAX
        ("len", ctypes.c_uint64)    # RDX
    ]

@pytest.fixture(scope="session")
def lib():
    """Loads and configures the assembly library as a singleton for the session."""
    lib_path = os.path.abspath("bin/debug/libcompuguessr.so")

    if not os.path.exists(lib_path):
        pytest.fail(f"Library not found at {lib_path}. Run 'make' first.")

    lib = ctypes.CDLL(lib_path)

    lib.utoa_r.argtypes = [ctypes.c_uint64, ctypes.c_char_p]
    lib.utoa_r.restype = UtoaResult

    lib.strlen_r.argtypes = [ctypes.c_char_p]
    lib.strlen_r.restype = ctypes.c_uint64

    lib.strlen_v_r.argtypes = [ctypes.c_char_p]
    lib.strlen_v_r.restype = ctypes.c_uint64

    return lib
