# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
import pytest

@pytest.mark.parametrize("value, expected", [(0, b"0"), (42, b"42"), (18446744073709551615, b"18446744073709551615")])
def test_cg_utoa(lib, value, expected):
    buffer = ctypes.create_string_buffer(32)
    res = lib.cg_utoa(value, buffer)
    assert ctypes.string_at(res.ptr, res.len) == expected

@pytest.mark.parametrize("value, expected", [(0, b"0"), (42, b"42"), (-42, b"-42"), (-9223372036854775807, b"-9223372036854775807")])
def test_cg_itoa(lib, value, expected):
    if not hasattr(lib, "cg_itoa"): pytest.skip("cg_itoa not exported")
    buffer = ctypes.create_string_buffer(32)
    res = lib.cg_itoa(value, buffer)
    assert ctypes.string_at(res.ptr, res.len) == expected

@pytest.mark.parametrize("s", [b"", b"A", b"Hello", b"A"*64])
def test_cg_strlen_variants(lib, s):
    buffer = ctypes.create_string_buffer(s)
    assert lib.cg_strlen(buffer) == len(s)
    assert lib.cg_strlen_avx2(buffer) == len(s)
    if hasattr(lib, "cg_strlen_sse"):
        assert lib.cg_strlen_sse(buffer) == len(s)

def test_cg_hash_str(lib):
    assert lib.cg_hash_str(b"test1", 5) != lib.cg_hash_str(b"test2", 5)

def test_cg_strcmp(lib):
    assert lib.cg_strcmp(b"match", b"match") == 0
    assert lib.cg_strcmp(b"a", b"b") < 0

def test_cg_memcmp_avx2(lib):
    if not hasattr(lib, "cg_memcmp_avx2"): pytest.skip()
    assert lib.cg_memcmp_avx2(b"match string longer than 32 bytes to test vector loop..........", b"match string longer than 32 bytes to test vector loop..........", 63) == 0
    assert lib.cg_memcmp_avx2(b"short", b"short", 5) == 0
    assert lib.cg_memcmp_avx2(b"alpha", b"bravo", 5) != 0

def test_cg_memslide(lib):
    if not hasattr(lib, "cg_memslide"): pytest.skip()

    # Original buffer data
    buf = ctypes.create_string_buffer(b"0123456789")

    # cg_memslide(base_ptr, new_start_ptr, total_original_len)
    base_ptr = ctypes.cast(ctypes.addressof(buf), ctypes.c_char_p)
    new_start_ptr = ctypes.cast(ctypes.addressof(buf) + 4, ctypes.c_char_p)

    # Total valid length is 10. The C code will calculate:
    # diff = base_ptr + 10 - (base_ptr + 4) = 6 bytes to copy
    # It copies "456789" to the start of the buffer.
    lib.cg_memslide(base_ptr, new_start_ptr, 10)

    # The first 6 bytes are updated to "456789", the last 4 bytes remain untouched "6789"
    assert buf.value == b"4567896789"

def test_cg_get_env(lib):
    if not hasattr(lib, "cg_get_env"): pytest.skip()
    envp_data = [b"USER=nate", b"PORT=8080", None]
    envp_arr = (ctypes.c_char_p * len(envp_data))(*envp_data)
    res = lib.cg_get_env(envp_arr, b"PORT")
    assert ctypes.string_at(res) == b"8080"
    assert lib.cg_get_env(envp_arr, b"MISSING") is None

def test_cg_memory_ops_avx2(lib):
    src, dst = ctypes.create_string_buffer(b"AVX2_DATA"), ctypes.create_string_buffer(10)
    lib.cg_memcpy_avx2(dst, src, 9)
    assert dst.value == b"AVX2_DATA"
