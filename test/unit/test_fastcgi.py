# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
import struct
import pytest
from cg_bindings import CgBufferMetadata

def test_cg_fastcgi_stdout(lib):
    if not hasattr(lib, "cg_fastcgi_stdout"): pytest.skip("cg_fastcgi_stdout not exported")

    buf = ctypes.create_string_buffer(1024)
    meta = CgBufferMetadata(buf=ctypes.cast(buf, ctypes.c_void_p), len=1024, valid=0, pos=0)

    payload = b"Status: 200 OK\r\n\r\nHello!"
    lib.cg_fastcgi_stdout(ctypes.byref(meta), 1234, payload, len(payload))

    assert meta.valid == 8 + len(payload)

    version, type_, req_hi, req_lo, len_hi, len_lo, pad, res = struct.unpack(">BBBBBBBB", buf.raw[:8])

    assert version == 1
    assert type_ == 6

    # Strictly enforce Big-Endian parsing
    assert (req_hi << 8 | req_lo) == 1234
    assert (len_hi << 8 | len_lo) == len(payload)

    assert buf.raw[8:8+len(payload)] == payload

def test_cg_fastcgi_end(lib):
    if not hasattr(lib, "cg_fastcgi_end"): pytest.skip("cg_fastcgi_end not exported")

    buf = ctypes.create_string_buffer(1024)
    meta = CgBufferMetadata(buf=ctypes.cast(buf, ctypes.c_void_p), len=1024, valid=0, pos=0)

    lib.cg_fastcgi_end(ctypes.byref(meta), 4321)
    assert meta.valid == 24

    v1, t1, _, _, lhi1, llo1, _, _ = struct.unpack(">BBBBBBBB", buf.raw[:8])
    assert t1 == 6
    assert (lhi1 << 8 | llo1) == 0

    v2, t2, rhi2, rlo2, lhi2, llo2, _, _ = struct.unpack(">BBBBBBBB", buf.raw[8:16])
    assert t2 == 3

    # Strictly enforce Big-Endian parsing
    assert (rhi2 << 8 | rlo2) == 4321
    assert (lhi2 << 8 | llo2) == 8
