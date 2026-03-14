
%ifndef FCNTL_INC
%define FCNTL_INC

; <bits/fcntl-linux.h> ;
O_RDONLY            equ 00
O_WRONLY            equ 01
O_RDWR              equ 02
O_NONBLOCK          equ 04000
O_CLOEXEC           equ 02000000
F_DUPFD             equ 0
F_GETFD             equ 1
F_SETFD             equ 2
F_GETFL             equ 3
F_SETFL             equ 4

%endif

