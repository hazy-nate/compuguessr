
%ifndef SYSCALL_INC
%define SYSCALL_INC

; <asm/unistd_64.h> ;
SYS_READ            equ 0
SYS_WRITE           equ 1
SYS_OPEN            equ 2
SYS_CLOSE           equ 3
SYS_STAT            equ 4
SYS_FSTAT           equ 5
SYS_LSTAT           equ 6
SYS_MMAP            equ 9
SYS_MPROTECT        equ 10
SYS_MUNMAP          equ 11
SYS_RT_SIGACTION    equ 13
SYS_RT_SIGRETURN    equ 15
SYS_PREAD64         equ 17
SYS_WRITEV          equ 20
SYS_PIPE            equ 22
SYS_NANOSLEEP		equ 35
SYS_SENDFILE        equ 40
SYS_SOCKET          equ 41
SYS_CONNECT         equ 42
SYS_ACCEPT          equ 43
SYS_SENDTO          equ 44
SYS_RECVFROM        equ 45
SYS_SENDMSG         equ 46
SYS_RECVMSG         equ 47
SYS_BIND            equ 49
SYS_LISTEN          equ 50
SYS_GETSOCKNAME     equ 51
SYS_GETPEERNAME     equ 52
SYS_SETSOCKOPT      equ 54
SYS_GETSOCKOPT      equ 55
SYS_CLONE           equ 56
SYS_FORK            equ 57
SYS_VFORK           equ 58
SYS_EXIT            equ 60
SYS_WAIT4           equ 61
SYS_FCNTL           equ 72
SYS_CHDIR			equ 80
SYS_UNLINK			equ 87
SYS_SETUID			equ 105
SYS_GETEUID			equ 107
SYS_CAPSET			equ 126
SYS_PRCTL			equ 157
SYS_CHROOT			equ 161
SYS_EPOLL_CREATE    equ 213
SYS_EPOLL_WAIT      equ 232
SYS_EPOLL_CTL       equ 233
SYS_UNSHARE			equ 272
SYS_SPLICE          equ 275
SYS_VMSPLICE        equ 278
SYS_ACCEPT4         equ 288
SYS_EPOLL_CREATE1   equ 291
SYS_SECCOMP			equ 317
SYS_IO_URING_SETUP	equ 425
SYS_IO_URING_ENTER	equ 426

%ifndef SYSCALL_INC_EXPORT
extern	syscall_read,			\
		syscall_write,			\
		syscall_open,			\
		syscall_close,			\
		syscall_fstat,			\
		syscall_mmap,			\
		syscall_munmap,			\
		syscall_rt_sigaction,	\
		syscall_rt_sigreturn,	\
		syscall_pread64,		\
		syscall_writev,			\
		syscall_nanosleep,		\
		syscall_sendfile,		\
		syscall_socket,			\
		syscall_accept4,		\
		syscall_bind,			\
		syscall_listen,			\
		syscall_setsockopt,		\
		syscall_clone,			\
		syscall_fork,			\
		syscall_exit,			\
		syscall_fcntl,			\
		syscall_chdir,			\
		syscall_unlink,			\
		syscall_setuid,			\
		syscall_geteuid,		\
		syscall_capset,			\
		syscall_prctl,			\
		syscall_chroot,			\
		syscall_epoll_wait,		\
		syscall_epoll_ctl,		\
		syscall_unshare,		\
		syscall_splice,			\
		syscall_epoll_create1,	\
		syscall_seccomp,		\
		syscall_io_uring_setup,	\
		syscall_io_uring_enter
%endif

%endif

