
%define SYSCALL_INC_EXPORT
%include "linux/syscall.inc"

section .text

global	syscall_read,			\
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
		syscall_accept4,		\
		syscall_epoll_create1,	\
		syscall_seccomp,		\
		syscall_io_uring_setup,	\
		syscall_io_uring_enter

_syscall_check:
    cmp     rax, -4095      ; Compare RAX to -4095 (0xFFFFFFFFFFFFF001)
    jae     .error          ; Jump if Above or Equal (unsigned comparison catches -4095 to -1)
    clc                     ; Success: Clear carry flag
    ret
.error:
    neg     rax             ; Error: Negate the full 64-bit register to get positive errno
    stc                     ; Set carry flag
    ret

; Read from a file descriptor.
; Refer to the read(2) manpage.
;
; INP:	RDI = File descriptor	// int fd
;		RSI = Buffer pointer	// void buf[.count]
;		RDX = Count				// size_t count
; OUT:	RAX = Number of bytes read (success)
;		RAX = Error code (if CF is set) (error)
;		CF  = 0 (success), 1 (error)
; CHG:	RAX, RCX, R11, CC
syscall_read:
	xor		eax, eax	; Sets RAX to 0 (which is the syscall number for read)
	syscall
	jmp		_syscall_check

; Write to a file descriptor.
; Refer to the write(2) manpage.
;
; INP:	RDI = File descriptor	// int fd
;		RSI = Buffer pointer	// const void *buf[.count]
;		RDX = Count				// size_t count
; OUT:	RAX = Number of bytes written (success)
;		RAX = Positive error code (if CF is set) (error)
;		CF  = 0 (success), 1 (error)
; CHG:	RAX, RCX, R11, CC
syscall_write:
	mov		eax, SYS_WRITE
	syscall
	jmp		_syscall_check

; Open and possibly create a file.
; Refer to the open(2) manpage.
;
; INP:	RDI = Pathname	// const char *pathname
;		RSI = Flags		// int flags
;		RDX = Mode		// umode_t mode
; OUT:	RAX = File descriptor (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_open:
	mov		eax, SYS_OPEN
	syscall
	jmp		_syscall_check

; Close a file descriptor.
; Refer to the close(2) manpage.
;
; INP:	RDI = File descriptor	// int fd
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_close:
	mov		eax, SYS_CLOSE
	syscall
	jmp		_syscall_check

; Get file status.
; Refer to the stat(2) manpage.
;
; INP:	RDI = File descriptor		// int fd
;		RSI = Stat buffer pointer	// struct stat *statbuf
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_fstat:
	mov		eax, SYS_FSTAT
	syscall
	jmp		_syscall_check

; Map files or devices into memory.
; Refer to the mmap(2) manpage.
;
; INP:	RDI = Address			// void addr[.length]
;		RSI = Length			// size_t length
;		RDX = Protections		// int prot
;		RCX = Flags				// int flags
;		R8  = File descriptor	// int fd
;		R9  = Offset			// off_t offset
; OUT:	RAX = Pointer to mapped area (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R10, R11, CC
syscall_mmap:
	mov		eax, SYS_MMAP
	mov		r10, rcx
	syscall
	jmp		_syscall_check

; Unmap files or devices from memory.
; Refer to the mmap(2) manpage.
;
; INP:	RDI = Address	// void addr[.length]
;		RSI = Length	// size_t length
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_munmap:
	mov		eax, SYS_MUNMAP
	syscall
	jmp		_syscall_check

; Examine and change a signal action.
; Refer to the sigaction(2) manpage.
;
; INP:	RDI = Signal number		// int signum
;		RSI = Action			// const struct sigaction *_Nullable restrict act
;		RDX = Old action		// struct sigaction *_Nullable restrict oldact
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R10, R11, CC
syscall_rt_sigaction:
	mov		eax, SYS_RT_SIGACTION
	mov		r10, 8					; size_t  sigsetsize = sizeof(sigset_t)
	syscall
	jmp		_syscall_check

; Return from signal handler and cleanup stack frame.
; Refer to the sigreturn(2) manpage.
;
; INP:	-
; OUT:	-
; CHG:	RAX, RCX, R11, CC
syscall_rt_sigreturn:
	mov		eax, SYS_RT_SIGRETURN
	syscall
	jmp		_syscall_check

; Read from a file descriptor at a given offset.
; Refer to the pread(2) manpage.
;
; INP:	RDI = File descriptor	// int fd
;		RSI = Buffer pointer	// void buf[.count]
;		RDX = Count				// size_t count
;		RCX = Offset			// off_t offset
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R10, R11, CC
syscall_pread64:
	mov		eax, SYS_PREAD64
	mov		r10, rcx
	syscall
	jmp		_syscall_check

; Write to a file descriptor from multiple buffers.
; Refer to the readv(2) manpage.
;
; INP:	RDI = File descriptor	// int fd
;		RSI = Iovec array		// const struct iovec *iov
;		RDX = Count				// int iovcnt
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_writev:
	mov		eax, SYS_WRITEV
	syscall
	jmp		_syscall_check

; High-resolution sleep.
; Refer to the nanosleep(2) manpage.
;
; INP:	RDI = Timespec			// const struct timespec *duration
;		RSI = Remaining time	// struct timespec *_Nullable rem
syscall_nanosleep:
	mov		eax, SYS_NANOSLEEP
	syscall
	jmp		_syscall_check

; Transfer data between file descriptors.
; Refer to the sendfile(2) manpage.
;
; INP:	RDI = Out file descriptor	// int out_fd
;		RSI = In file descriptor	// int in_fd
;		RDX = Offset				// off_t *_Nullable offset
;		RCX = Count					// size_t count
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R10, R11, CC
syscall_sendfile:
	mov		eax, SYS_SENDFILE
	mov		r10, rcx
	syscall
	jmp		_syscall_check

; Create an endpoint for communication.
; Refer to the socket(2) manpage.
;
; INP:	RDI = Domain	// int domain
;		RSI = Type		// int type
;		RDX = Protocol	// int protocol
; OUT:	RAX = File descriptor (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_socket:
	mov		eax, SYS_SOCKET
	syscall
	jmp		_syscall_check

; Bind a name to a socket.
; Refer to the bind(2) manpage.
;
; INP:	RDI = Socket file descriptor	// int sockfd
;		RSI = Socket address			// const struct sockaddr *addr
;		RDX = Socket address length		// socklen_t addrlen
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_bind:
	mov		eax, SYS_BIND
	syscall
	jmp		_syscall_check

; Listen for connections on a socket.
; Refer to the listen(2) manpage.
;
; INP:	RDI = Socket file descriptor	// int sockfd
;		RSI = Backlog					// int backlog
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_listen:
	mov		eax, SYS_LISTEN
	syscall
	jmp		_syscall_check

; Set options on sockets.
; Refer to the getsockopt(2) manpage.
;
; INP:	RDI = Socket file descriptor	// int sockfd
;		RSI = Level						// int level
;		RDX = Option name				// int optname
;		RCX = Option value				// const void optval[.optlen]
;		R8  = Option length				// socklen_t optlen
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R10, R11, CC
syscall_setsockopt:
	mov		eax, SYS_SETSOCKOPT
	mov		r10, rcx
	syscall
	jmp		_syscall_check

; INP:	RDI = Flags					// unsigned long flags
;		RSI = Stack pointer			// void *stack
;		RDX = Parent TID pointer	// int *parent_tid
;		RCX = Child TID pointer		// int *child_tid
;		R8  = Thread local storage 	// unsigned long tls
; OUT:	RAX = Thread ID of child process in caller's thread (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R10, R11, CC
syscall_clone:
	mov		eax, SYS_CLONE
	mov		r10, rcx
	syscall
	jmp		_syscall_check

; Create a child process.
; Refer to the fork(2) manpage.
;
; INP:	-
; OUT:	RAX = PID of child process in parent, 0 in the child (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_fork:
	mov		eax, SYS_FORK
	syscall
	jmp		_syscall_check

; Terminate the calling process.
; Refer to the _exit(2) manpage.
;
; INP:	RDI = Status	// int status
; OUT:	-
syscall_exit:
	mov		eax, SYS_EXIT
	syscall

; Manipulate file descriptor.
; Refer to the fcntl(2) manpage.
;
; INP:	RDI = File descriptor	// int fd
;		RSI = Operation			// int op
;		RDX = Argument			// int
; OUT:	RAX = Depends on the operation performed (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_fcntl:
	mov		eax, SYS_FCNTL
	syscall
	jmp		_syscall_check

; Change working directory.
; Refer to the chdir(2) manpage.
;
; INP:	RDI = Path string	// const char *path
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_chdir:
	mov		eax, SYS_CHDIR
	syscall
	jmp		_syscall_check

; Delete a name and possibly the file it refers to.
; Refer to the unlink(2) manpage.
;
; INP:	RDI = Pathname	// const char *pathname
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_unlink:
	mov		eax, SYS_UNLINK
	syscall
	jmp		_syscall_check

; Set user identity.
; Refer to the setuid(2) manpage.
;
; INP:	RDI = User ID	// uid_t uid
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_setuid:
	mov		eax, SYS_SETUID
	syscall
	jmp		_syscall_check

; Get effective user identity.
; Refer to the getuid(2) manpage.
;
; INP:	-
; OUT:	RAX = Effective user ID	// uid_t
; CHG:	RAX, RCX, R11, CC
syscall_geteuid:
	mov		eax, SYS_GETEUID
	syscall
	jmp		_syscall_check

; Set capabilities of thread(s).
; Refer to the capget(2) manpage.
;
; INP:	RDI = User header pointer	// cap_user_header_t hdrp
;		RSI = User data pointer		// cap_user_data_t datap
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_capset:
	mov		eax, SYS_CAPSET
	syscall
	jmp		_syscall_check

; Operations on a process or thread.
; Refer to the prctl(2) manpage.
;
; INP:	RDI = Operation // int op
; OUT:	RAX = Positive value (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_prctl:
	mov		eax, SYS_PRCTL
	mov		r10, rcx
	syscall
	jmp		_syscall_check

; Change root directory.
; Refer to the chroot(2) manpage.
;
; INP:	RDI = Path	// const char *path
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_chroot:
	mov		eax, SYS_CHROOT
	syscall
	jmp		_syscall_check

; Wait for an I/O event on an epoll file descriptor.
; Refer to the epoll_wait(2) manpage.
;
; INP:	RDI = Epoll file descriptor	// int epfd
;		RSI = Epoll events			// struct epoll_event events[.maxevents]
;		RDX = Max events			// int maxevents
;		RCX = Timeout				// int timeout
; OUT:	RAX = Number of ready file descriptors
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R10, R11, CC
syscall_epoll_wait:
	mov		eax, SYS_EPOLL_WAIT
	mov		r10, rcx
	syscall
	jmp		_syscall_check

; Control interface for an epoll file descriptor.
; Refer to the epoll_ctl(2) manpage.
;
; INP:	RDI = Epoll file descriptor	// int epfd
;		RSI = Operation				// int op
;		RDX = File descriptor		// int fd
;		RCX = Event					// struct epoll_event *_Nullable event
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R10, R11, CC
syscall_epoll_ctl:
	mov		eax, SYS_EPOLL_CTL
	mov		r10, rcx
	syscall
	jmp		_syscall_check

; Disassociate parts of the process execution context.
; Refer to the unshare(2) manpage.
;
; INP:	RDI = Flags	// int flags
; OUT:	RAX = 0 (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_unshare:
	mov		eax, SYS_UNSHARE
	syscall
	jmp		_syscall_check

; Splice data to/from a pipe.
; Refer to the splice(2) manpage.
;
; INP:	RDI = File descriptor in	// int fd_in
;		RSI = Offset in				// off_t *_Nullable off_in
;		RDX = File descriptor out	// int fd_out
;		RCX = Offset out			// off_t *_Nullable off_out
;		R8  = Size					// size_t size
;		R9  = Flags					// unsigned int flags
; OUT:	RAX = Number of bytes spliced
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R8, R9, R10, R11, CC
syscall_splice:
	mov		eax, SYS_SPLICE
	mov		r10, rcx
	syscall
	jmp		_syscall_check

; Accept a connection on a socket.
; Refer to the accept(2) manpage.
;
; INP:	RDI = Socket file descriptor	// int sockfd
;		RSI = Socket address			// struct sockaddr *_Nullable restrict addr
;		RDX = Socket address length		// socklen_t *_Nullable restrict addrlen
;		RCX = Flags						// int flags
; OUT:	RAX = File descriptor (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R10, R11, CC
syscall_accept4:
	mov		eax, SYS_ACCEPT4
	mov		r10, rcx
	syscall
	jmp		_syscall_check

; Open an epoll file descriptor.
; Refer to the epoll_create(2) manpage.
;
; INP:	RDI = Flags	// int flags
; OUT:	RAX = File descriptor (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_epoll_create1:
	mov		eax, SYS_EPOLL_CREATE1
	syscall
	jmp		_syscall_check

; Operate on Secure Computing state of the process.
; Refer to the seccomp(2) manpage.
;
; INP:	RDI = Operation	// unsigned int operation
;		RSI = Flags		// unsigned int flags
;		RDX = Args		// void *args
; OUT:	RAX = 0 (success)
;		RAX = ID of thread causing synchronization failure
;			(if SECCOMP_FILTER_FLAG_TSYNC used) (error)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_seccomp:
	mov		eax, SYS_SECCOMP
	syscall
	jmp		_syscall_check

; Setup a context for performing asynchronous I/O.
; Refer to the io_uring_setup(2) manpage.
;
; INP:	RDI = Entries			// u32 entries
;		RSI = Params pointer	// struct io_uring_params *p
; OUT:	RAX = File descriptor (success)
;		RAX = Positive error code (if CF is set) (error)
; CHG:	RAX, RCX, R11, CC
syscall_io_uring_setup:
	mov		eax, SYS_IO_URING_SETUP
	syscall
	jmp		_syscall_check

; Initiate and/or complete asynchronous I/O.
; Refer to the io_uring_enter(2) manpage.
;
; INP:	RDI = File descriptor	// unsigned int fd
;		RSI = To submit			// unsigned int to_submit
;		RDX = Minimum complete	// unsigned int min_complete
;		RCX = Flags				// unsigned int flags
;		R8  = Signal set		// sigset_t *sig
;		R9  = Signal set size	// size_t sz
; OUT:	RAX = Number of I/Os successfully consumed (success)
; CHG:	RAX, RCX, R8, R9, R10, R11, CC
syscall_io_uring_enter:
	mov		eax, SYS_IO_URING_ENTER
	mov		r10, rcx
	syscall
	jmp		_syscall_check
