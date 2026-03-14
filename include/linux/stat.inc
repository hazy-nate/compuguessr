
%ifndef STAT_INC
%define STAT_INC

; <bits/struct_stat.h> ;
struc stat_t
    .st_dev         resq 1  ; __dev_t st_dev
    .st_ino         resq 1  ; __ino_t st_ino
    .st_nlink       resq 1  ; __nlink_t st_nlink
    .st_mode        resd 1  ; __mode_t st_mode
    .st_uid         resd 1  ; __uid_t st_uid
    .st_gid         resd 1  ; __gid_t st_gid
    .pad0           resd 1  ; int pad0
    .st_rdev        resq 1  ; __dev_t st_rdev
    .st_size        resq 1  ; __off_t st_size
    .st_blksize     resq 1  ; __blksize_t st_blksize
    .st_blocks      resq 1  ; __blkcnt_t st_blocks
    .st_atime_sec   resq 1  ; __time_t st_atime
    .st_atime_nsec  resq 1  ; __syscall_ulong_t st_atimensec
    .st_mtime_sec   resq 1  ; __time_t st_mtime
    .st_mtime_nsec  resq 1  ; __syscall_ulong_t st_mtimensec
    .st_ctime_sec   resq 1  ; __time_t st_ctime
    .st_ctime_nsec  resq 1  ; __syscall_ulong_t st_ctimensec
    .reserved       resq 3  ; __syscall_slong_t __glibc_reserved[3]
endstruc

%endif

