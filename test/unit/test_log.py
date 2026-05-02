# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
import pytest

def test_cg_log_write(lib, capfd):
    if not hasattr(lib, "cg_log_write"): pytest.skip("cg_log_write not exported")

    # Add explicit \0 to terminate the strings for C's strlen
    lib.cg_log_write(b"INFO\0", b"cg_fastcgi.c\0", b"42\0", b"Connection established\n\0")
    out, err = capfd.readouterr()

    # Strictly enforce exact output
    assert "(cg_fastcgi.c.42) [INFO] Connection established\n" in err

def test_cg_log_write_ts(lib, capfd):
    if not hasattr(lib, "cg_log_write_ts"): pytest.skip("cg_log_write_ts not exported")

    # Add explicit \0 to terminate the strings for C's strlen
    lib.cg_log_write_ts(b"2026-05-01 17:51:00\0", b"FATAL\0", b"cg_app.c\0", b"112\0", b"Socket bind failed\n\0")
    out, err = capfd.readouterr()

    # Strictly enforce exact output
    assert "2026-05-01 17:51:00: (cg_app.c.112) [FATAL] Socket bind failed\n" in err
