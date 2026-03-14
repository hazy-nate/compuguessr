#!/usr/bin/env python3

import os
import socket
import struct
import subprocess
import time
import sys

import test_utils

FCGI_MACROS = test_utils.get_definitions("include/fcgi.h")

FCGI_VERSION_1      = FCGI_MACROS["FCGI_VERSION_1"]
FCGI_BEGIN_REQUEST  = FCGI_MACROS["FCGI_BEGIN_REQUEST"]
FCGI_STDIN          = FCGI_MACROS["FCGI_STDIN"]
FCGI_STDOUT         = FCGI_MACROS["FCGI_STDOUT"]
FCGI_END_REQUEST    = FCGI_MACROS["FCGI_END_REQUEST"]
FCGI_RESPONDER      = FCGI_MACROS["FCGI_RESPONDER"]

SOCK_PATH = "/tmp/fcgi_asm_test.sock"
BIN_PATH = "./bin/debug/compuguessr"

def pack_header(record_type, req_id, content_length, padding_length=0):
    # !     = Network Byte Order (Big-Endian)
    # B     = unsigned char (1 byte)
    # H     = unsigned short (2 bytes)
    # x     = pad byte
    return struct.pack("!BBHHBx", FCGI_VERSION_1, record_type, req_id, content_length, padding_length)

def pack_begin_request(role, flags=0):
    # H     = role (2 bytes),
    # B     = flags (1 byte),
    # 5x    = reserved (5 bytes)
    return struct.pack('!HB5x', role, flags)

def run_test():
    if not os.path.exists(BIN_PATH):
        print(f"ERROR: Binary not found at {BIN_PATH}. Run 'make debug' first.")
        sys.exit(1)

    if os.path.exists(SOCK_PATH):
        os.remove(SOCK_PATH)

    listen_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    listen_sock.bind(SOCK_PATH)
    listen_sock.listen(5)

    def preexec():
        os.dup2(listen_sock.fileno(), 0)

    print("[*] Spawning FastCGI process...")
    process = subprocess.Popen(
        [BIN_PATH],
        preexec_fn=preexec,
        pass_fds=[listen_sock.fileno()],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL
    )

    client_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

    try:
        time.sleep(0.1)

        print("[*] Connecting to application socket...")
        client_sock.connect(SOCK_PATH)

        test_req_id = 42

        print(f"[*] Sending FCGI_BEGIN_REQUEST (ID: {test_req_id})...")
        begin_body = pack_begin_request(FCGI_RESPONDER)
        client_sock.sendall(pack_header(FCGI_BEGIN_REQUEST, test_req_id, len(begin_body)) + begin_body)

        print("[*] Sending empty FCGI_STDIN (EOF)...")
        client_sock.sendall(pack_header(FCGI_STDIN, test_req_id, 0))

        def read_record():
            header = client_sock.recv(8)
            if not header or len(header) < 8:
                return None, None, None

            version, r_type, req_id, content_len, padding_len = struct.unpack('!BBHHBx', header)
            content = client_sock.recv(content_len) if content_len > 0 else b''

            if padding_len > 0:
                client_sock.recv(padding_len)

            return r_type, req_id, content

        r_type, r_req_id, content = read_record()
        assert r_type == FCGI_STDOUT, f"Expected FCGI_STDOUT (6), got {r_type}"
        assert r_req_id == test_req_id, f"Expected Request ID {test_req_id}, got {r_req_id}"
        assert b"Hello from Full Metal Assembly!" in content, "HTTP Payload text missing!"
        print("[+] PASS: Received valid FCGI_STDOUT payload.")

        r_type, r_req_id, content = read_record()
        assert r_type == FCGI_STDOUT, f"Expected FCGI_STDOUT (6), got {r_type}"
        assert len(content) == 0, "Expected empty payload for STDOUT EOF"
        print("[+] PASS: Received valid FCGI_STDOUT EOF marker.")

        r_type, r_req_id, content = read_record()
        assert r_type == FCGI_END_REQUEST, f"Expected FCGI_END_REQUEST (3), got {r_type}"

        app_status, prot_status = struct.unpack('!IB3x', content)
        assert app_status == 0, f"Expected App Status 0, got {app_status}"
        assert prot_status == 0, f"Expected Protocol Status 0 (REQUEST_COMPLETE), got {prot_status}"
        print("[+] PASS: Received valid FCGI_END_REQUEST.")

        print("\n[SUCCESS] Integration test passed!")

    except AssertionError as e:
        print(f"\n[FAIL] Assertion Error: {e}")
        sys.exit(1)
    finally:
        print("[*] Tearing down test environment...")
        client_sock.close()
        listen_sock.close()
        process.terminate()
        process.wait()
        if os.path.exists(SOCK_PATH):
            os.remove(SOCK_PATH)

if __name__ == "__main__":
    run_test()
