
%ifndef SIGNAL_INC
%define SIGNAL_INC

; <asm/signal.h> ;
SIGINT              equ 2
SIGTERM             equ 15
SIGCHLD             equ 17
SA_NOCLDWAIT        equ 0x00000002
SA_RESTORER         equ 0x04000000

; <bits/sigaction.h> ;
struc sigaction_t
    .sa_handler     resq 1  ; void (*sa_handler)(int)
    .sa_flags       resq 1  ; int sa_flags
    .sa_restorer    resq 1  ; void (*sa_restorer)(void)
    .sa_mask        resq 1  ; sigset_t sa_mask
endstruc

%endif

