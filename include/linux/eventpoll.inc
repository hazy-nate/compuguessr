
%ifndef EVENTPOLL_INC
%define EVENTPOLL_INC

; <linux/eventpoll.h> ;
EPOLL_CTL_ADD   equ 1
EPOLL_CTL_DEL   equ 2
EPOLL_CTL_MOD   equ 3
EPOLLIN         equ 0x00000001
EPOLLPRI        equ 0x00000002
EPOLLOUT        equ 0x00000004
EPOLLERR        equ 0x00000008
EPOLLHUP        equ 0x00000010
EPOLLNVAL       equ 0x00000020
EPOLLRDNORM     equ 0x00000040
EPOLLRDBAND     equ 0x00000080
EPOLLWRNORM     equ 0x00000100
EPOLLWRBAND     equ 0x00000200
EPOLLMSG        equ 0x00000400
EPOLLRDHUP      equ 0x00002000
EPOLLET         equ (1 << 31)

; <sys/epoll.h> ;
; <linux/eventpoll.h> ;
struc epoll_event_t
    .events resd 1  ; uint32_t events
    .data   resq 1  ; union epoll_data {
                    ;   void *ptr
                    ;   int fd
                    ;   uint32_t u32
                    ;   uint64_t u64
                    ; }
endstruc

%endif

