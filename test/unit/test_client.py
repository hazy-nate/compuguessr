# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
import pytest

def test_client_pool_lifecycle(lib):
    if not hasattr(lib, "cg_client_pool_init") or not hasattr(lib, "cg_client_alloc"):
        pytest.skip("Client pool allocators not exported")

    # Each client context contains 64KB RX and 64KB TX buffers (~131KB total).
    # We allocate 5MB to safely hold 10 clients without corrupting Python's heap,
    # plus an extra 64 bytes to allow for manual memory alignment.
    raw_buf = ctypes.create_string_buffer((5 * 1024 * 1024) + 64)

    # Manually align the pointer to a 64-byte boundary.
    # This prevents segfaults if the C compiler auto-vectorizes struct initialization.
    addr = ctypes.addressof(raw_buf)
    aligned_addr = (addr + 63) & ~63
    pool_ptr = ctypes.c_void_p(aligned_addr)

    # Initialize the array for 10 clients
    lib.cg_client_pool_init(pool_ptr, 10)

    # Allocating from the pool should return a valid memory address pointer
    client1 = lib.cg_client_alloc(pool_ptr, 10)
    assert client1 is not None

    client2 = lib.cg_client_alloc(pool_ptr, 10)
    assert client2 is not None
    assert client1 != client2 # They should occupy different slots

    # Freeing the client marks its internal state enum to 0 (FREE), without crashing
    if hasattr(lib, "cg_client_free"):
        lib.cg_client_free(client1)
        lib.cg_client_free(client2)
