; SPDX-License-Identifier: GPL-2.0-only
; Copyright (C) 2026 Nathaniel Williams

bits    64
cpu     x64
default rel
osabi   linux

#include <sys/syscall.h>

%include "arch/x86_64/cg_fastcgi.inc"

global  cg_fcgi_read_request:function
global	cg_fcgi_send_response:function

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
        db "Pong received!", 13, 10
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

cg_fcgi_read_request:
        mov		[rel conn_fd], rdi
.read_header:
        mov     rax, SYS_read
        mov     rdi, [rel conn_fd]
        lea     rsi, [rel header_buf]
        mov     rdx, fcgi_header_t_size
        syscall
        ;-----------------------------------------------------------------------
        cmp     rax, fcgi_header_t_size
        jne     .error
        ;-----------------------------------------------------------------------
        mov     al, byte [rel header_buf + fcgi_header_t.type]
        cmp     al, FCGI_STDIN
        je      .handle_stdin
.consume_payload:
        movzx   rax, byte [rel header_buf + fcgi_header_t.content_length_b1]
        shl     rax, 8
        movzx   rbx, byte [rel header_buf + fcgi_header_t.content_length_b0]
        or      rax, rbx
        movzx   rcx, byte [rel header_buf + fcgi_header_t.padding_length]
        add     rax, rcx
        ;-----------------------------------------------------------------------
        test    rax, rax
        jz      .read_header
        ;-----------------------------------------------------------------------
        mov     rdi, [rel conn_fd]
        lea     rsi, [rel payload_buf]
        mov     rdx, rax
        mov		rax, SYS_read
        syscall
        jmp     .read_header
.handle_stdin:
        movzx   rax, byte [rel header_buf + fcgi_header_t.request_id_b1]
        shl     rax, 8
        movzx   rbx, byte [rel header_buf + fcgi_header_t.request_id_b0]
        or      rax, rbx
        ret
.error:
        xor     rax, rax
        ret

cg_fcgi_send_response:
        mov     [rel conn_fd], rdi
        mov		ax, si
        mov     byte [rel response + fcgi_header_t.request_id_b1], bl
        mov     byte [rel response + fcgi_header_t.request_id_b0], al
        mov     byte [rel response + response_eof_offset + fcgi_header_t.request_id_b1], bl
        mov     byte [rel response + response_eof_offset + fcgi_header_t.request_id_b0], al
        mov     byte [rel response + response_end_offset + fcgi_header_t.request_id_b1], bl
        mov     byte [rel response + response_end_offset + fcgi_header_t.request_id_b0], al
        ;-----------------------------------------------------------------------
        mov     rdi, [rel conn_fd]
        lea     rsi, [rel response]
        mov     rdx, response_size
        mov		rax, SYS_write
        syscall
        ret
