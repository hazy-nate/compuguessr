#!/usr/bin/env python3

import socket
import struct
import os
import sys
import time

import test_utils

FCGI_MACROS = test_utils.get_definitions("include/fcgi.h")

HOST = '127.0.0.1'
PORT = 9000

RECORD_TYPES = {
    1: "BEGIN_REQUEST",
    2: "ABORT_REQUEST",
    3: "END_REQUEST",
    4: "PARAMS",
    5: "STDIN",
    6: "STDOUT",
    7: "STDERR",
    8: "DATA"
}

def decode_length(data, offset):
    """Decodes FastCGI variable-length integers (1 or 4 bytes)."""
    first_byte = data[offset]
    if first_byte >> 7 == 0:
        return first_byte, offset + 1
    else:
        # 4-byte length: mask out the MSB (high bit)
        length = struct.unpack("!I", data[offset:offset+4])[0] & 0x7FFFFFFF
        return length, offset + 4

def parse_params(data):
    """Iterates through the PARAMS payload to extract key-value pairs."""
    params = []
    offset = 0
    while offset < len(data):
        try:
            name_len, offset = decode_length(data, offset)
            val_len, offset = decode_length(data, offset)
            name = data[offset:offset+name_len].decode('utf-8', errors='replace')
            offset += name_len
            value = data[offset:offset+val_len].decode('utf-8', errors='replace')
            offset += val_len
            params.append((name, value))
        except (IndexError, UnicodeDecodeError):
            break
    return params

def log_record(direction, header, content):
    """Decodes and prints every parameter of each frame."""
    v, r_type, req_id, c_len, p_len, _ = struct.unpack("!BBHHBB", header)
    dir_str = f"[{direction}]"
    type_name = RECORD_TYPES.get(r_type, f"UNKNOWN({r_type})")

    print(f"\n{dir_str} Type: {type_name:15} | ID: {req_id:5} | ContentLen: {c_len:5}")

    if r_type == 1: # BEGIN_REQUEST
        role, flags = struct.unpack("!HB5x", content)
        print(f"  > ROLE: {role} (1=Responder) | FLAGS: {flags}")

    elif r_type == 4 and c_len > 0: # PARAMS
        for k, v in parse_params(content):
            print(f"  > {k:25} : {v}")

    elif r_type == 3: # END_REQUEST
        app_status, prot_status = struct.unpack("!IB3x", content)
        print(f"  > APP_STATUS: {app_status} | PROTOCOL_STATUS: {prot_status}")

    elif c_len > 0:
        # Show data snippet, escaping newlines for readability
        preview = content[:100].replace(b'\r\n', b'\\r\\n').decode('utf-8', errors='replace')
        print(f"  > DATA: {preview}...")

def create_and_log_record(r_type, req_id, content_bytes):
    """Generates binary record with 8-byte padding and logs the frame."""
    version = 1
    content_len = len(content_bytes)
    padding_len = (8 - (content_len % 8)) % 8
    header = struct.pack("!BBHHBB", version, r_type, req_id, content_len, padding_len, 0)

    log_record("SEND", header, content_bytes)
    return header + content_bytes + (b'\x00' * padding_len)

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        try:
            s.bind((HOST, PORT))
            s.listen(5)
            print(f"[*] Full Metal Bridge active on TCP {HOST}:{PORT}")
        except Exception as e:
            print(f"[!] Bind failed: {e}")
            return

        while True:
            conn, addr = s.accept()
            with conn:
                print(f"[*] Connection from {addr}")
                try:
                    while True:
                        header = conn.recv(8)
                        if not header: break

                        v, r_type, req_id, c_len, p_len, _ = struct.unpack("!BBHHBB", header)

                        content = b''
                        while len(content) < c_len:
                            chunk = conn.recv(c_len - len(content))
                            if not chunk: break
                            content += chunk

                        if p_len > 0:
                            conn.recv(p_len)

                        log_record("RECV", header, content)

                        if r_type == 5 and c_len == 0:
                            print("\n" + "="*60 + "\n[*] GENERATING PROTOCOL-COMPLIANT RESPONSE\n" + "="*60)

                            html_body = "<html><body><h1>Handshake Verified</h1><p>TCP Loopback Active.</p></body></html>"

                            response_text = (
                                "Status: 200 OK\r\n"
                                "Content-Type: text/html\r\n"
                                f"Content-Length: {len(html_body)}\r\n"
                                "\r\n"
                                f"{html_body}"
                            ).encode('utf-8')

                            # Send STDOUT data
                            conn.sendall(create_and_log_record(6, req_id, response_text))
                            # Send STDOUT EOF (empty record)
                            conn.sendall(create_and_log_record(6, req_id, b''))
                            # Send END_REQUEST
                            conn.sendall(create_and_log_record(3, req_id, struct.pack("!IB3x", 0, 0)))

                            print("\n[+] Final frames transmitted. Waiting for flush...")
                            # Tiny sleep to prevent 'Connection Reset' race condition
                            time.sleep(0.1)
                            break
                except ConnectionResetError:
                    print("\n[!] Error: Connection reset by peer. Check Content-Length or alignment.")
                except Exception as e:
                    print(f"\n[!] Unexpected error: {e}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n[*] Shutting down.")
        sys.exit(0)
