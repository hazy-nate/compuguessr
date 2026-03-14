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

%include "linux/linux.inc"

bits    64
cpu     x64
default rel
osabi   linux

global  _start
extern  fcgi_handle_connection
extern  syscall_rt_sigaction
extern  syscall_rt_sigreturn
extern  syscall_accept4
extern  syscall_exit

;===============================================================================
; INITIALIZED DATA
;===============================================================================

section .data

sigint_sigaction:
    istruc sigaction_t
        at sigaction_t.sa_handler,  dq sigint_handler
        at sigaction_t.sa_flags,    dq SA_NOCLDWAIT | SA_RESTORER
        at sigaction_t.sa_restorer, dq sigint_restorer
        at sigaction_t.sa_mask,     dq 0
    iend
    sigint_flag db 0

;===============================================================================
; READ-ONLY DATA
;===============================================================================

section .rodata

;===============================================================================
; UNINITIALIZED DATA
;===============================================================================

section .bss

;===============================================================================
; INSTRUCTIONS
;===============================================================================

section .text

_start:
    mov     rdi, SIGINT
    lea     rsi, [rel sigint_sigaction]
    xor     rdx, rdx
    mov     rcx, sigaction_t_size
    call    syscall_rt_sigaction

.accept_loop:
    cmp     byte [rel sigint_flag], 1   ; Check for interrupt signal.
    je      exit_success
    ;---------------------------------------------------------------------------
    xor     rdi, rdi                    ; sockfd = 0
    xor     rsi, rsi                    ; addr = NULL
    xor     rdx, rdx                    ; addrlen = NULL
    xor     rcx, rcx                    ; flags = 0
    call    syscall_accept4
    jc      .accept_loop
    ;---------------------------------------------------------------------------
    mov     rdi, rax
    call    fcgi_handle_connection
    jmp     .accept_loop

sigint_restorer:
    call    syscall_rt_sigreturn

sigint_handler:
    mov     byte [sigint_flag], 1
    ret

exit_failure:
    mov     rdi, 1
    call    syscall_exit

exit_success:
    mov     rdi, 0
    call    syscall_exit
