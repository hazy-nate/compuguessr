# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
import pytest

def test_cg_datetime_str_get(lib):
    if not hasattr(lib, "cg_datetime_str_get"):
        pytest.skip("cg_datetime_str_get not exported")

    # The C code is heavily optimized and ONLY writes digits.
    # It assumes the buffer already contains the structural formatting characters.
    buf = ctypes.create_string_buffer(b"0000-00-00 00:00:00", 32)
    lib.cg_datetime_str_get(buf)

    res = buf.value.decode('utf-8')
    assert len(res) == 19
    assert res.startswith("20") # Sanity check for current millennium
    assert res[4] == '-' and res[7] == '-' and res[10] == ' '
    assert res[13] == ':' and res[16] == ':'

def test_cg_time_get(lib):
    if not hasattr(lib, "cg_time_get"):
        pytest.skip("cg_time_get not exported")

    ts = lib.cg_time_get()
    assert isinstance(ts, int)
    assert ts > 1700000000
