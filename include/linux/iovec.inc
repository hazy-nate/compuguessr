
%ifndef IOVEC_INC
%define IOVEC_INC

; <sys/uio.h> ;
struc iovec_t
    .iov_base       resq 1  ; void *iov_base
    .iov_len        resq 1  ; size_t iov_len
endstruc

%endif

