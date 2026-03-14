
%ifndef PRCTL_INC
%define PRCTL_INC

; <linux/prctl.h> ;
PR_SET_KEEPCAPS             equ 8
PR_SET_NO_NEW_PRIVS         equ 38
PR_CAP_AMBIENT              equ 47
PR_CAP_AMBIENT_IS_SET       equ 1
PR_CAP_AMBIENT_RAISE        equ 2
PR_CAP_AMBIENT_LOWER        equ 3
PR_CAP_AMBIENT_CLEAR_ALL    equ 4

%endif
