# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import socket
import struct
import requests
import pytest

def test_socket_garbage_fuzzing(live_server, server_setup):
    """Sends pure binary garbage to the io_uring socket."""
    _, sock_path, _ = server_setup

    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client.connect(str(sock_path))

    # Send 1KB of random garbage bytes to trick the parser
    client.sendall(b"\xDE\xAD\xBE\xEF" * 256)
    client.close()

    # Verify the C daemon survived without a segmentation fault or a deadlock
    try:
        res = requests.get(f"{live_server}/api/leaderboard", timeout=2)
        assert res.status_code == 200
    except requests.exceptions.Timeout:
        pytest.fail("Daemon deadlocked (DoS) after receiving garbage bytes!")
    except requests.exceptions.ConnectionError:
        pytest.fail("Daemon crashed (Segfault) after receiving garbage bytes!")
    assert res.status_code == 200

def test_socket_malformed_header_fuzzing(live_server, server_setup):
    """Sends a mathematically malicious FastCGI header."""
    _, sock_path, _ = server_setup

    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client.connect(str(sock_path))

    # Construct a valid FastCGI header, but claim the content_len is 65535 (Max uint16).
    # If the C code trusts this blindly without bounds checking, it will overflow its buffer.
    # struct format: version, type, req_hi, req_lo, len_hi, len_lo, padding, reserved
    malformed_header = struct.pack(">BBBBBBBB", 1, 1, 0, 1, 255, 255, 0, 0)

    client.sendall(malformed_header)

    # Only send 4 bytes of data, starving the parser which expects 65535
    client.sendall(b"1234")
    client.close()

    # Verify the C daemon survived without a segmentation fault or a deadlock
    try:
        res = requests.get(f"{live_server}/api/leaderboard", timeout=2)
        assert res.status_code == 200
    except requests.exceptions.Timeout:
        pytest.fail("Daemon deadlocked (DoS) after receiving garbage bytes!")
    except requests.exceptions.ConnectionError:
        pytest.fail("Daemon crashed (Segfault) after receiving garbage bytes!")
    assert res.status_code == 200
