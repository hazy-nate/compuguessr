
%ifndef NET_INC
%define NET_INC

%define htons(x) ((x >> 8) & 0xFF) | ((x & 0xFF) << 8)
%define htonl(x) ((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | ((x & 0x000000FF) << 24)

; <bits/socket.h> ;
AF_UNSPEC   equ 0
AF_UNIX     equ 1
AF_INET     equ 2
AF_INET6    equ 10

; <bits/socket-constants.h ;
SOL_SOCKET      equ 1
SO_REUSEADDR    equ 2
SO_REUSEPORT    equ 15

; <bits/socket_type.h> ;
SOCK_STREAM equ 1
SOCK_DGRAM  equ 2

; <linux/un.h> ;
UNIX_PATH_MAX   equ 108
struc sockaddr_un_t
    .sun_family     resw 1              ; __kernel_sa_family_t sun_family
    .sun_path       resb UNIX_PATH_MAX  ; char sun_path[UNIX_PATH_MAX]
endstruc

; <linux/in.h> ;
IPPROTO_IP      equ 0
IPPROTO_IPV6    equ 41
SOCK_SIZE       equ 16  ; sizeof(struct sockaddr)
struc sockaddr_in_t
    .sin_family     resw 1  ; __kernel_sa_family_t sin_family
                            ; typedef unsigned short __kernel_sa_family_t
    .sin_port       resw 1  ; __be16 sin_port
    .sin_addr       resw 1  ; struct in_addr {
                            ;   __be32 s_addr
                            ; }
    .__pad          resw 1  ; unsigned char __pad[__SOCK_SIZE__ -
                            ; sizeof(short int) - sizeof(unsigned short int) -
                            ; sizeof(struct in_addr)]
                            ; #define __SOCK_SIZE__ 16
endstruc

; <linux/in6.h> ;
IPV6_V6ONLY equ 26

struc sockaddr_in6_t
    .sin6_family    resw 1  ; sa_family_t sin6_family
    .sin6_port      resw 1  ; in_port_t sin6_port
    .sin6_flowinfo  resd 1  ; uint32_t sin6_flowinfo
    .sin6_addr      reso 1  ; struct in6_addr sin6_addr
    .sin6_scope_id  resd 1  ; uint32_t sin6_scope_id
endstruc

%endif

