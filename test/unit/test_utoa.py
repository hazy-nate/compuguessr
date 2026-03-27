# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
import pytest

@pytest.mark.parametrize("uint_value, expected_str", [
    (0,         "0"),
    (10,        "10"),
    (99,        "99"),
    (100,       "100"),
    (2**64 - 1, "18446744073709551615"),
])
def test_utoa_r(lib, uint_value, expected_str):
    buffer = ctypes.create_string_buffer(32)
    res = lib.utoa_r(uint_value, buffer)
    actual_str = ctypes.string_at(res.ptr).decode('ascii')
    actual_len = res.len
    assert expected_str == actual_str
    assert len(expected_str) == actual_len
