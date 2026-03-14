
%ifndef CAPABILITY_INC
%define CAPABILITY_INC

; <linux/capability.h> ;
LINUX_CAPABILITY_VERSION_1    equ 0x19980330
LINUX_CAPABILITY_VERSION_2    equ 0x20071026
LINUX_CAPABILITY_VERSION_3    equ 0x20080522

struc user_cap_header_t
    .version    resd 1  ; __u32 version
    .pid        resd 1  ; int pid
endstruc

struc user_cap_data_t
    .effective      resd 2  ; __u32 effective
    .permitted      resd 2  ; __u32 permitted
    .inheritable    resd 2  ; __u32 inheritable
endstruc

CAP_NET_BIND_SERVICE        equ 10
CAP_NET_BIND_SERVICE_MASK   equ 1 << CAP_NET_BIND_SERVICE
CAP_SYS_CHROOT              equ 18
CAP_SYS_CHROOT_MASK         equ 1 << CAP_SYS_CHROOT

%endif
