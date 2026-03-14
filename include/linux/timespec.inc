
%ifndef TIME_INC
%define TIME_INC

struc timespec_t
    .tv_sec     resq 1  ; time_t tv_sec
    .tv_nsec    resq 1  ; long tv_nsec
endstruc

%endif
