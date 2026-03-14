; SPDX-License-Identifier: GPL-2.0-only

;;
;; Copyright (C) 2026 Nathaniel Williams
;;
;; This program is free software; you can redistribute it and/or modify it under
;; the terms of the GNU General Public License as published by the Free Software
;; Foundation; version 2.
;;
;; This program is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
;; FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License along with
;; this program; if not, write to the Free Software Foundation, Inc., 51
;; Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
;;

%include "fcgi.inc"
%include "linux/linux.inc"

bits    64
cpu     x64
default rel
osabi   linux

global  fcgi_handle_connection
extern  syscall_read
extern  syscall_write
extern  syscall_close

;===============================================================================
; INITIALIZED DATA
;===============================================================================

section .data

response:
    ; STDOUT Record (HTTP Response)
    istruc fcgi_header_t
        at fcgi_header_t.version,            db FCGI_VERSION_1
        at fcgi_header_t.type,               db FCGI_STDOUT
        at fcgi_header_t.request_id_b1,      db 0
        at fcgi_header_t.request_id_b0,      db 0
        at fcgi_header_t.content_length_b1,  db (http_payload_len >> 8) & 0xFF
        at fcgi_header_t.content_length_b0,  db http_payload_len & 0xFF
        at fcgi_header_t.padding_length,     db 0
        at fcgi_header_t.reserved,           db 0
    iend
.http_payload:
    db "Content-type: text/plain", 13, 10
    db 13, 10
    db "Hello from Full Metal Assembly!", 13, 10
http_payload_len equ $ - .http_payload

response_eof_offset equ $ - response
    ; STDOUT EOF Record (Length 0)
    istruc fcgi_header_t
        at fcgi_header_t.version,            db FCGI_VERSION_1
        at fcgi_header_t.type,               db FCGI_STDOUT
        at fcgi_header_t.request_id_b1,      db 0
        at fcgi_header_t.request_id_b0,      db 0
        at fcgi_header_t.content_length_b1,  db 0
        at fcgi_header_t.content_length_b0,  db 0
        at fcgi_header_t.padding_length,     db 0
        at fcgi_header_t.reserved,           db 0
    iend

response_end_offset equ $ - response
    ; END_REQUEST Record
    istruc fcgi_header_t
        at fcgi_header_t.version,            db FCGI_VERSION_1
        at fcgi_header_t.type,               db FCGI_END_REQUEST
        at fcgi_header_t.request_id_b1,      db 0
        at fcgi_header_t.request_id_b0,      db 0
        at fcgi_header_t.content_length_b1,  db 0
        at fcgi_header_t.content_length_b0,  db 8
        at fcgi_header_t.padding_length,     db 0
        at fcgi_header_t.reserved,           db 0
    iend
    istruc fcgi_end_request_body_t
        at fcgi_end_request_body_t.app_status_b3,   db 0
        at fcgi_end_request_body_t.app_status_b2,   db 0
        at fcgi_end_request_body_t.app_status_b1,   db 0
        at fcgi_end_request_body_t.app_status_b0,   db 0
        at fcgi_end_request_body_t.protocol_status, db 0   ; FCGI_REQUEST_COMPLETE
        at fcgi_end_request_body_t.reserved,        db 0, 0, 0
    iend
response_size equ $ - response

;===============================================================================
; READ-ONLY DATA
;===============================================================================

section .rodata

;===============================================================================
; UNINITIALIZED DATA
;===============================================================================

section .bss

    conn_fd         resq 1
    header_buf      resb fcgi_header_t_size
    payload_buf     resb 65535 + 255

;===============================================================================
; INSTRUCTIONS
;===============================================================================

section .text

fcgi_handle_connection:
    mov     [rel conn_fd], rdi

.read_header:
    mov     rdi, [rel conn_fd]
    lea     rsi, [rel header_buf]
    mov     rdx, fcgi_header_t_size
    call    syscall_read
    ;---------------------------------------------------------------------------
    cmp     rax, fcgi_header_t_size
    jne     .close_connection
    ;---------------------------------------------------------------------------
    mov     al, byte [rel header_buf + fcgi_header_t.type]
    cmp     al, FCGI_STDIN
    je      .handle_stdin

.consume_payload:
    ; Calculate total bytes to read: content_length + padding_length
    movzx   rax, byte [rel header_buf + fcgi_header_t.content_length_b1]
    shl     rax, 8
    movzx   rbx, byte [rel header_buf + fcgi_header_t.content_length_b0]
    or      rax, rbx
    movzx   rcx, byte [rel header_buf + fcgi_header_t.padding_length]
    add     rax, rcx
    ;---------------------------------------------------------------------------
    test    rax, rax
    jz      .read_header
    ;---------------------------------------------------------------------------
    mov     rdi, [rel conn_fd]
    lea     rsi, [rel payload_buf]
    mov     rdx, rax
    call    syscall_read
    jmp     .read_header

.handle_stdin:
    ; For STDIN, just check content_length (padding comes after, but 0 length means EOF)
    movzx   rax, byte [rel header_buf + fcgi_header_t.content_length_b1]
    shl     rax, 8
    movzx   rbx, byte [rel header_buf + fcgi_header_t.content_length_b0]
    or      rax, rbx
    ;---------------------------------------------------------------------------
    test    rax, rax
    jnz     .consume_payload

.send_response:
    ; Extract the request ID from the incoming header
    mov     al, byte [rel header_buf + fcgi_header_t.request_id_b1]
    mov     bl, byte [rel header_buf + fcgi_header_t.request_id_b0]
    ;---------------------------------------------------------------------------
    ; Patch the Request ID into the 3 outgoing records
    mov     byte [rel response + fcgi_header_t.request_id_b1], al
    mov     byte [rel response + fcgi_header_t.request_id_b0], bl

    mov     byte [rel response + response_eof_offset + fcgi_header_t.request_id_b1], al
    mov     byte [rel response + response_eof_offset + fcgi_header_t.request_id_b0], bl

    mov     byte [rel response + response_end_offset + fcgi_header_t.request_id_b1], al
    mov     byte [rel response + response_end_offset + fcgi_header_t.request_id_b0], bl
    ;---------------------------------------------------------------------------
    mov     rdi, [rel conn_fd]
    lea     rsi, [rel response]
    mov     rdx, response_size
    call    syscall_write

.close_connection:
    mov     rdi, [rel conn_fd]
    call    syscall_close
    ret
