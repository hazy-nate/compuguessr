
%ifndef SECCOMP_INC
%define SECCOMP_INC

; <linux/seccomp.h> ;
SECCOMP_SET_MODE_FILTER equ 1

SECCOMP_RET_KILL_THREAD equ 0x00000000
SECCOMP_RET_KILL        equ SECCOMP_RET_KILL_THREAD
SECCOMP_RET_ALLOW       equ 0x7fff0000

; <linux/filter.h> ;
struc sock_fprog_t
    .len    resq 1  ; unsigned short len
    .filter resq 1  ; struct sock_filter *filter
endstruc

struc sock_filter_t
    .code   resw 1  ; __u16 code
    .jt     resb 1  ; __u8 jt
    .jf     resb 1  ; __u8 jf
    .k      resd 1  ; __u32 k
endstruc

%endif
