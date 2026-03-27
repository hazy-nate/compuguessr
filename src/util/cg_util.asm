; SPDX-License-Identifier: GPL-2.0-only
; Copyright (C) 2026 Nathaniel Williams

bits	64
cpu	all
default	rel
osabi	linux

#include <sys/syscall.h>

global  cg_get_env:function
global  cg_memcpy_avx2:function
global  cg_memset_avx2:function
global  cg_strlen:function
global  cg_strlen_avx2:function
global  cg_strlen_sse:function
global  cg_utoa:function
global	cg_write_uint:function

;===============================================================================
; INSTRUCTIONS
;===============================================================================

section .text

;;****f* util/cg_get_env
;; NAME
;;   cg_get_env
;; SYNOPSIS
;;   extern char *cg_get_env(char **envp, const char *target_key_str);
;; SOURCE
cg_get_env:
        test    rdi, rdi
        jz      .not_found
        test    rsi, rsi
        jz      .not_found
        mov     r9, rsi
.next_entry:
        mov     rdx, [rdi]      ; Load pointer to current env string
        test    rdx, rdx        ; Is it the NULL terminator of the envp array?
        jz      .not_found
        mov     rsi, r9         ; Reset RSI to the start of target_key
        mov     rax, rdx        ; Use RAX to track our position in the env string
.compare_loop:
        mov     cl, [rsi]               ; Load byte from target_key
        test    cl, cl                  ; Is it the end of target_key?
        jz      short .check_equal      ; If yes, check if the env string has a '=' here
        cmp     cl, [rax]               ; Compare target_key byte with env_str byte
        jne     short .mismatch         ; If they don't match, move to next envp entry
        add     rsi, 1
        add     rax, 1
        jmp     short .compare_loop
.check_equal:
        cmp     byte [rax], '='
        jne     short .mismatch
        add     rax, 1
        ret
.mismatch:
        add     rdi, 8
        jmp     .next_entry
.not_found:
        xor     rax, rax
        ret
;;******

;;****f* util/cg_memcpy_avx2
;; NAME
;;   cg_memcpy_avx2
;; SYNOPSIS
;;   extern void cg_memcpy_avx2(void *dest, void *src, int n);
;; INPUTS
;;   * RDI - Pointer to destination
;;   * RSI - Pointer to source
;;   * RDX - Length
;; SOURCE
align 64
cg_memcpy_avx2:
        test    rdx, rdx
        jz      .done
        cmp     rdx, 32
        jl      short .small_copy       ; If length is less than 32, copy via the byte loop
.align_prologue:
        mov     rcx, rdi
        and     rcx, 31
        jz      short .vector_loop      ; If length is already 32-byte aligned, skip to loop
        mov     r8, 32
        sub     r8, rcx
.align_byte_loop:
        mov     al, [rsi]
        mov     [rdi], al
        add     rsi, 1
        add     rdi, 1
        sub     rdx, 1
        sub     r8, 1
        jnz     short .align_byte_loop
.vector_loop:
        cmp     rdx, 32
        jl      short .vector_tail      ; If less than 32 bytes left, perform an overlapping copy
        vmovdqu ymm0, [rsi]
        vmovdqa [rdi], ymm0
        add     rsi, 32
        add     rdi, 32
        sub     rdx, 32
        jmp     short .vector_loop
.vector_tail:
        test    rdx, rdx
        jz      short .done
        vmovdqu ymm0, [rsi + rdx - 32]
        vmovdqu [rdi + rdx - 32], ymm0
        jmp     short .done
.small_copy:
        test    rdx, rdx
        jz      short .done
.byte_loop:
        mov     al, [rsi]
        mov     [rdi], al
        add     rsi, 1
        add     rdi, 1
        sub     rdx, 1
        jnz     short .byte_loop
.done:
        vzeroupper
        ret
;;******

;;****f* util/cg_memset_avx2
;; NAME
;;   cg_memset_avx2
;; SYNOPSIS
;;   extern void cg_memset_avx2(void *dest, int val, unsigned long n);
;; SOURCE
cg_memset_avx2:
        test    rdx, rdx
        jz      .done
        vmovd   xmm0, esi
        vpbroadcastb ymm0, xmm0
        cmp     rdx, 32
        jl      short .small_fill
.align_prologue:
        mov     rcx, rdi
        and     rcx, 31
        jz      short .vector_loop
        mov     r8, 32
        sub     r8, rcx
.align_byte_loop:
        mov     [rdi], sil
        add     rdi, 1
        sub     rdx, 1
        sub     r8, 1
        jnz     short .align_byte_loop
.vector_loop:
        cmp     rdx, 32
        jl      short .vector_tail
        vmovdqa [rdi], ymm0
        add     rdi, 32
        sub     rdx, 32
        jmp     short .vector_loop
.vector_tail:
        test    rdx, rdx
        jz      short .done
        vmovdqu [rdi + rdx - 32], ymm0
        jmp     short .done
.small_fill:
        test    rdx, rdx
        jz      short .done
.byte_loop:
        mov     [rdi], sil
        add     rdi, 1
        sub     rdx, 1
        jnz     short .byte_loop
.done:
        vzeroupper
        ret
;;******

;;****f* util/cg_strlen
;; NAME
;;   cg_strlen
;; SYNOPSIS
;;   mov/lea  rdi, <buffer address>
;;   call     cg_strlen
;;
;;   extern int cg_strlen(char *buf);
;; FUNCTION
;;   Finds the length of a NULL-terminated string.
;; INPUTS
;;   * RDI - Pointer to buffer
;; RESULT
;;   * RAX - Length of string
;; SOURCE
align 64
cg_strlen:
        xor     rax, rax
.loop:
        cmp     byte [rdi + rax], 0
        je      short .done
        add     rax, 1
        jmp     short .loop
.done:
        ret
;;******

;;****f* util/cg_strlen_avx2
;; NAME
;;   cg_strlen_avx2
;; SYNOPSIS
;;   mov/lea  rdi, <buffer address>
;;   call     cg_strlen_avx2
;;
;;   extern int cg_strlen_avx2(char *buf);
;; FUNCTION
;;   Finds the length of a NULL-terminated string.
;; INPUTS
;;   * RDI - Pointer to buffer
;; RESULT
;;   * RAX - Length of string
;;   * RDX - Clobbered
;;   * YMM0 - Clobbered
;;   * YMM1 - Clobbered
;; SOURCE
align 64
cg_strlen_avx2:
        vpxor   ymm0, ymm0, ymm0        ; YMM0 = Null terminator (search target)
        ;-----------------------------------------------------------------------
        mov     rcx, rdi
        and     rcx, 31         ; RCX = RDI % 32
        mov     rax, rdi
        and     rax, -32        ; RAX = RDI & ~31
        ;-----------------------------------------------------------------------
        vmovdqa ymm1, [rax]
        vpcmpeqb ymm1, ymm1, ymm0
        vpmovmskb edx, ymm1     ; EDX = 32-bit mask of zero locations
        shr     edx, cl         ; Shift out bytes before the actual string
        test    edx, edx        ; Is the NULL terminator in the first block?
        jnz     short .found_first_block
        add     rax, 32         ; If not, advance to the next block
.loop:
        vmovdqa ymm1, [rax]
        vpcmpeqb ymm1, ymm1, ymm0
        vpmovmskb edx, ymm1
        test    edx, edx
        jnz     short .found
        add     rax, 32
        jmp     short .loop
.found_first_block:
        bsf     eax, edx        ; EAX = Index of first set bit
        vzeroupper
        ret
.found:
        bsf     edx, edx        ; Find index of zero in the current block
        add     rax, rdx        ; RAX = Absolute memory address of NULL byte
        sub     rax, rdi        ; RAX = Length
        vzeroupper
        ret
;;******

;;****f* util/cg_strlen_sse
;; NAME
;;   cg_strlen_sse
;; SYNOPSIS
;;   mov/lea  rdi, <buffer address>
;;   call     cg_strlen_sse
;;
;;   extern int cg_strlen_sse(char *buf);
;; FUNCTION
;;   Finds the length of a NULL-terminated string.
;; INPUTS
;;   * RDI - Pointer to buffer
;; RESULT
;;   * RAX - Length of string
;;   * RDX - Clobbered
;;   * XMM0 - Clobbered
;;   * XMM1 - Clobbered
;; SOURCE
align 64
cg_strlen_sse:
        pxor    xmm0, xmm0      ; XMM0 = Null terminator (search target)
        mov     rcx, rdi
        and     rcx, 15
        mov     rax, rdi
        and     rax, -16
        movdqa  xmm1, [rax]
        pcmpeqb xmm1, xmm0
        pmovmskb edx, xmm1
        shr     edx, cl
        test    edx, edx
        jnz     short .found_first_block
        add     rax, 16
.loop:
        movdqa  xmm1, [rax]
        pcmpeqb xmm1, xmm0
        pmovmskb edx, xmm1
        test    edx, edx
        jnz     short .found
        add     rax, 16
        jmp     short .loop
.found_first_block:
        bsf     eax, edx
        ret
.found:
        bsf     edx, edx
        add     rax, rdx
        sub     rax, rdi
        ret
;;******

;;****f* util/cg_utoa
;; NAME
;;   cg_utoa
;; SYNOPSIS
;;   mov     rdi, <unsigned integer>
;;   mov/lea rsi, <buffer address>
;;   call    cg_utoa
;;
;;   extern utoa_result cg_utoa(uint64_t n, char *buf);
;; FUNCTION
;;   Converts an unsigned integer to an ASCII string.
;; INPUTS
;;   * RDI - Unsigned 64-bit integer
;;   * RSI - Pointer to buffer (>=21 bytes)
;; RESULT
;;   * RAX - Pointer to start of string
;;   * RDX - Length of string
;;   * R8 - Clobbered
;;   * R9 - Clobbered
;;   * R10 - Clobbered
;; EXAMPLE
;;   struct {
;;       uint64_t ptr;
;;       uint64_t len;
;;   } cg_utoa_result;
;;   extern struct utoa_result cg_utoa(uint64_t n, char *buf);
;;   utoa_result res = cg_utoa(123, buf);
;;   printf("Pointer: %p, Length: %lu\n", (void*)res.ptr, res.len);
;; SOURCE
align 64
cg_utoa:
        mov     rax, rdi                ; RAX = Dividend
        lea     rcx, [rsi + 20]         ; RCX = End of buffer
        mov     byte [rcx], 0           ; Insert NULL terminator at the end
        mov     rdx, 0xCCCCCCCCCCCCCCCD	; RDX is used by MULX for the multiplier
.loop:
        mov     r9, rax         ; Save dividend to calculate remainder later
        mulx    r8, rax, rax    ; High bits in R8, low in RAX
        shr     r8, 3           ; R8 = Quotient
        ;-----------------------------------------------------------------------
        lea     r10, [r8 + r8 * 4]      ; R10 = Quotient * 5
        lea     r10, [r10 + r10]        ; R10 = Quotient * 10
        sub     r9, r10                 ; R9 = Remainder (0-9)
        ;-----------------------------------------------------------------------
        add     r9b, '0'        ; Convert to ASCII
        dec     rcx
        mov     [rcx], r9b
        ;-----------------------------------------------------------------------
        mov     rax, r8         ; Move quotient to RAX for next loop
        test    rax, rax
        jnz     short .loop
        ;-----------------------------------------------------------------------
        mov     rax, rcx        ; RAX = pointer to string start
        lea     rdx, [rsi + 20]
        sub     rdx, rcx        ; RDX = length
        ret
;;******

;;****f* util/cg_write_uint
;; NAME
;;   cg_write_uint
;; SYNOPSIS
;;   mov  rdi, <unsigned integer>
;;   mov  rsi, <file descriptor>
;;   call cg_write_uint
;;
;;   extern int cg_write_uint(uint64_t n, int fd);
;; FUNCTION
;;   Converts an unsigned integer to an ASCII string and writes it to a file
;;   descriptor.
;; INPUTS
;;   * RDI - Unsigned 64-bit integer
;;   * RSI - File descriptor
;; RESULT
;;   * RAX (success) - Number of bytes written
;;   * RAX (failure) - Negative error code
;;   * RCX - Clobbered
;;   * RDX - Clobbered
;;   * R11 - Clobbered
;; SOURCE
align 64
cg_write_uint:
        push    rbp             ; Save stack frame
        mov     rbp, rsp
        sub     rsp, 32         ; Allocate 32 bytes on stack for a local buffer
        ;-----------------------------------------------------------------------
        push    rsi             ; Preserve RSI for SYS_write syscall at the end
        mov     rsi, rsp        ; RSI = Pointer to stack buffer
        ; RDI is already the integer to convert
        call    cg_utoa         ; RAX = Pointer to string start
                                ; RDX = Length of string
        ;-----------------------------------------------------------------------
        pop     rdi             ; RDI = File descriptor
        mov     rsi, rax        ; RSI = Pointer to string start
        ; RDX is already the length of string
        mov     rax, SYS_write
        syscall
        ;-----------------------------------------------------------------------
        leave   ; Restore RSP and RBP
        ret
;;******
