
%ifndef LINUX_INC
%define LINUX_INC

%include "linux/bpf.inc"
%include "linux/capability.inc"
%include "linux/eventpoll.inc"
%include "linux/fcntl.inc"
%include "linux/io_uring.inc"
%include "linux/iovec.inc"
%include "linux/mman.inc"
%include "linux/net.inc"
%include "linux/prctl.inc"
%include "linux/sched.inc"
%include "linux/seccomp.inc"
%include "linux/signal.inc"
%include "linux/stat.inc"
%include "linux/syscall.inc"
%include "linux/timespec.inc"
%include "linux/unistd.inc"

STDIN           equ 0
STDOUT          equ 1
STDERR          equ 2

EXIT_SUCCESS    equ 0
EXIT_FAILURE    equ 1

%endif

