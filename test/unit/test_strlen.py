# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
import pytest

TEST_CASES = [
    "",
    "A",
    "Hello, world!",
    "Hello, world!\n\t",
    "0",
    "18446744073709551615",
    "A" * 15,
    "A" * 16,
    "A" * 17,
    "A" * 31,
    "A" * 32,
    "A" * 1024
]

@pytest.mark.parametrize("str_value", TEST_CASES)
def test_strlen_r(lib, str_value):
    b_value = str_value.encode('ascii')
    actual_len = lib.strlen_r(b_value)
    assert len(str_value) == actual_len

@pytest.mark.parametrize("str_value", TEST_CASES)
def test_strlen_v_r(lib, str_value):
    b_value = str_value.encode('ascii')
    actual_len = lib.strlen_v_r(b_value)
    assert len(str_value) == actual_len
