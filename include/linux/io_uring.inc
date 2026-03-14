
%ifndef IO_URING_INC
%define IO_URING_INC

; <linux/io_uring.h> ;
struc io_uring_sqe_t
    .opcode             resb 1      ; __u8 opcode - type of operation
    .flags              resb 1      ; __u8 flags - IOSQE_ flags
    .ioprio             resw 1      ; __u16 ioprio - ioprio for request
    .fd                 resd 1      ; __s32 fd - file descriptor
    .off                resq 1      ; union {
                                    ;   __u64 off - offset into file
                                    ;   __u64 addr2
                                    ;   struct {
                                    ;       __u32 cmd_op
                                    ;       __u32 __pad1
                                    ;   }
                                    ; }
    .addr               resq 1      ; union {
                                    ;   __u64 addr - pointer to buffer/iovecs
                                    ;   __u64 splice_off_in
                                    ;   struct {
                                    ;       __u32 level
                                    ;       __u32 optname
                                    ;   }
                                    ; }
    .len                resd 1      ; __u32 len - buffer size or num iovecs
    .flags_union        resd 1      ; union {
                                    ;   	__u32 rw_flags
                                    ;       __u32 fsync_flags
                                    ;       __u16 poll_events
                                    ;       __u32 poll32_events
                                    ;       __u32 sync_range_flags
                                    ;       __u32 msg_flags
                                    ;       __u32 timeout_flags
                                    ;       __u32 accept_flags
                                    ;       __u32 cancel_flags
                                    ;       __u32 open_flags
                                    ;       __u32 statx_flags
                                    ;       __u32 fadvise_advice
                                    ;       __u32 splice_flags
                                    ;       __u32 rename_flags
                                    ;       __u32 unlink_flags
                                    ;       __u32 hardlink_flags
                                    ;       __u32 xattr_flags
                                    ;       __u32 msg_ring_flags
                                    ;       __u32 uring_cmd_flags
                                    ;       __u32 waitid_flags
                                    ;       __u32 futex_flags
                                    ;       __u32 install_fd_flags
                                    ;       __u32 nop_flags
                                    ;       __u32 pipe_flags
                                    ; }
    .user_data          resq 1      ; __u64 user_data - passed back at completion
    .buf_index          resw 1      ; union {
                                    ;   __u16 buf_index - index into fixed buffers, if used
                                    ;   __u16 buf_group - for grouped buffer selection
                                    ; } __attribute__((packed))
    .personality        resw 1      ; __u16 - personality to use, if used
    .splice_fd_in       resd 1      ; union {
                                    ;   __s32 splice_fd_in
                                    ;   __u32 file_index
                                    ;   __u32 zcrx_ifq_idx
                                    ;   __u32 optlen
                                    ;   struct {
                                    ;       __u16 addr_len
                                    ;       __u16 __pad3[1]
                                    ;   }
                                    ;   struct {
                                    ;       __u8 addr_len
                                    ;       __u8 __pad4[3]
                                    ;   }
                                    ; }
    .addr3              resq 1      ; union {
                                    ;   struct {
                                    ;       __u64 addr3
                                    ;       __u64 __pad2[1]
                                    ;   }
                                    ;   struct {
                                    ;       __u64 attr_ptr - pointer to attribute information
                                    ;       __u64 attr_type_mask - bit mask of attributes
                                    ;   }
                                    ;   __u64 optval
                                    ;   __u8 cmd[0]
                                    ; }
    .__pad2             resq 1      ; __u64 - padding or command data
endstruc

struc io_uring_cqe_t
    .user_data  resq 1  ; __u64 user_data - sqe->user_data value passed back
    .res        resd 1  ; __s32 res - Result code for this event
    .flags      resd 1  ; __u32 flags
endstruc

IORING_OP_NOP               equ 0
IORING_OP_READV             equ 1
IORING_OP_WRITEV            equ 2
IORING_OP_FSYNC             equ 3
IORING_OP_READ_FIXED        equ 4
IORING_OP_WRITE_FIXED       equ 5
IORING_OP_POLL_ADD          equ 6
IORING_OP_POLL_REMOVE       equ 7
IORING_OP_SYNC_FILE_RANGE   equ 8
IORING_OP_SENDMSG           equ 9
IORING_OP_RECVMSG           equ 10
IORING_OP_TIMEOUT           equ 11
IORING_OP_TIMEOUT_REMOVE    equ 12
IORING_OP_ACCEPT            equ 13
IORING_OP_ASYNC_CANCEL      equ 14
IORING_OP_LINK_TIMEOUT      equ 15
IORING_OP_CONNECT           equ 16
IORING_OP_FALLOCATE         equ 17
IORING_OP_OPENAT            equ 18
IORING_OP_CLOSE             equ 19
IORING_OP_FILES_UPDATE      equ 20
IORING_OP_STATX             equ 21
IORING_OP_READ              equ 22
IORING_OP_WRITE             equ 23
IORING_OP_FADVISE           equ 24
IORING_OP_MADVISE           equ 25
IORING_OP_SEND              equ 26
IORING_OP_RECV              equ 27
IORING_OP_OPENAT2           equ 28
IORING_OP_EPOLL_CTL         equ 29
IORING_OP_SPLICE            equ 30
IORING_OP_PROVIDE_BUFFERS   equ 31
IORING_OP_REMOVE_BUFFERS    equ 32
IORING_OP_TEE               equ 33
IORING_OP_SHUTDOWN          equ 34
IORING_OP_RENAMEAT          equ 35
IORING_OP_UNLINKAT          equ 36
IORING_OP_MKDIRAT           equ 37
IORING_OP_SYMLINKAT         equ 38
IORING_OP_LINKAT            equ 39
IORING_OP_MSG_RING          equ 40
IORING_OP_FSETXATTR         equ 41
IORING_OP_SETXATTR          equ 42
IORING_OP_FGETXATTR         equ 43
IORING_OP_GETXATTR          equ 44
IORING_OP_SOCKET            equ 45
IORING_OP_URING_CMD         equ 46
IORING_OP_SEND_ZC           equ 47
IORING_OP_SENDMSG_ZC        equ 48
IORING_OP_READ_MULTISHOT    equ 49
IORING_OP_WAITID            equ 50
IORING_OP_FUTEX_WAIT        equ 51
IORING_OP_FUTEX_WAKE        equ 52
IORING_OP_FUTEX_WAITV       equ 53
IORING_OP_FIXED_FD_INSTALL  equ 54
IORING_OP_FTRUNCATE         equ 55
IORING_OP_BIND              equ 56
IORING_OP_LISTEN            equ 57
IORING_OP_RECV_ZC           equ 58
IORING_OP_EPOLL_WAIT        equ 59
IORING_OP_READV_FIXED       equ 60
IORING_OP_WRITEV_FIXED      equ 61
IORING_OP_PIPE              equ 62
IORING_OP_LAST              equ 63

IORING_OFF_SQ_RING      equ 0
IORING_OFF_CQ_RING      equ 0x8000000
IORING_OFF_SQES         equ 0x10000000
IORING_OFF_PBUF_RING    equ 0x80000000
IORING_OFF_PBUF_SHIFT   equ 16
IORING_OFF_MMAP_MASK    equ 0xf8000000

IORING_ENTER_GETEVENTS          equ 1 << 0
IORING_ENTER_SQ_WAKEUP          equ 1 << 1
IORING_ENTER_SQ_WAIT            equ 1 << 2
IORING_ENTER_EXT_ARG            equ 1 << 3
IORING_ENTER_REGISTERED_RING    equ 1 << 4
IORING_ENTER_ABS_TIMER          equ 1 << 5
IORING_ENTER_EXT_ARG_REG        equ 1 << 6
IORING_ENTER_NO_IOWAIT          equ 1 << 7

struc io_sqring_offsets_t
    .head           resd 1  ; __u32 head
    .tail           resd 1  ; __u32 tail
    .ring_mask      resd 1  ; __u32 ring_mask
    .ring_entries   resd 1  ; __u32 ring_entries
    .flags          resd 1  ; __u32 flags
    .dropped        resd 1  ; __u32 dropped
    .array          resd 1  ; __u32 array
    .resv1          resd 1  ; __u32 resv1
    .user_addr      resq 1  ; __u64 user_addr
endstruc

struc io_cqring_offsets_t
    .head           resd 1  ; __u32 head
    .tail           resd 1  ; __u32 tail
    .ring_mask      resd 1  ; __u32 ring_mask
    .ring_entries   resd 1  ; __u32 ring_entries
    .overflow       resd 1  ; __u32 overflow
    .cqes           resd 1  ; __u32 cqes
    .flags          resd 1  ; __u32 flags
    .resv1          resd 1  ; __u32 resv1
    .user_addr      resq 1  ; __u64 user_addr
endstruc

struc io_uring_params_t
    .sq_entries     resd 1                          ; __u32 sq_entries
    .cq_entries     resd 1                          ; __u32 cq_entries
    .flags          resd 1                          ; __u32 flags
    .sq_thread_cpu  resd 1                          ; __u32 sq_thread_cpu
    .sq_thread_idle resd 1                          ; __u32 sq_thread_idle
    .features       resd 1                          ; __u32 features
    .wq_fd          resd 1                          ; __u32 wq_fd
    .resv           resd 3                          ; __u32 resv[3]
    .sq_off         resb io_sqring_offsets_t_size   ; struct io_sqring_offsets sq_off (defined above)
    .cq_off         resb io_cqring_offsets_t_size   ; struct io_cqring_offsets cq_off (defined above)
endstruc

IORING_FEAT_SINGLE_MMAP equ (1 << 0)
IORING_FEAT_NODROP      equ (1 << 1)

%endif
