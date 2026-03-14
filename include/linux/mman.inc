
%ifndef MMAN_INC
%define MMAN_INC

; <bits/mman-linux.h> ;
PROT_READ           equ 0x1
PROT_WRITE          equ 0x2
PROT_EXEC           equ 0x4
PROT_NONE           equ 0x0

MAP_SHARED          equ 0x01
MAP_PRIVATE         equ 0x02
MAP_ANONYMOUS       equ 0x20

; <bits/mman-map-flags-generic.h>
MAP_POPULATE        equ 0x08000

%endif

