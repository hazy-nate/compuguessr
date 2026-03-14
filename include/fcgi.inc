; SPDX-License-Identifier: GPL-2.0-only

;;
;; Copyright (C) 2026 Nathaniel Williams
;;
;; This program is free software; you can redistribute it and/or modify it under
;; the terms of the GNU General Public License as published by the Free Software
;; Foundation; version 2.
;;
;; This program is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
;; FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License along with
;; this program; if not, write to the Free Software Foundation, Inc., 51
;; Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
;;

%ifndef FCGI_INC
%define FCGI_INC

;===============================================================================
; MACROS
;===============================================================================

;;****m* FastCGI/BS
;; NAME
;; BS
;; SYNOPSIS
;; BS <register>
;; FUNCTION
;; Swaps the bytes in a 16, 32, or 64-bit register.
;; INPUTS
;; %1 - The 16, 32, or 64-bit register to swap.
;; RETURN
;; None
;; SEE ALSO
;; FastCGI/BS_WORD
;; FastCGI/BS_DWORD
;; SOURCE
%macro BS 1
    %if %1_size == 2
        rol %1, 8
    %else
        bswap %1
    %endif
%endmacro
;;******

;;****m* FastCGI/BS_WORD
;; NAME
;; BS_WORD
;; SYNOPSIS
;; BS_WORD(x)
;; FUNCTION
;; Swaps the bytes of a word (16-bit) value for the NASM preprocessor.
;; INPUTS
;; x - Value between -32,768 and 32,767 (signed) or 0 and 65,535 (signed).
;; RETURN
;; None
;; SEE ALSO
;; FastCGI/BS
;; FastCGI/BS_DWORD
;; SOURCE
%define BS_WORD(x) (((x) & 0xFF) << 8 | ((x) >> 8) & 0xFF)
;;******

;;****m* FastCGI/BS_DWORD
;; NAME
;; BS_DWORD
;; SYNOPSIS
;; BS_DWORD(x)
;; FUNCTION
;; Swaps the bytes of a dword (32-bit) value for the NASM preprocessor.
;; INPUTS
;; x - Value between -2,147,483,648 and 2,147,483,647 (signed) or 0 and 4,294,967,295 (unsigned).
;; RETURN
;; None
;; SEE ALSO
;; FastCGI/BS
;; FastCGI/BS_WORD
;; SOURCE
%define BS_DWORD(x) (((x) & 0xFF) << 24 | ((x) & 0xFF00) << 8 | ((x) >> 8) & 0xFF00 | ((x) >> 24) & 0xFF)
;;******

;===============================================================================
; VERSION
;===============================================================================

;;****d* FastCGI/FCGI_VERSION_1
;; NAME
;; FCGI_VERSION_1
;; FUNCTION
;; The version member of the fcgi_header_t struct is always 1.
;; SOURCE
FCGI_VERSION_1  equ 1
;;******

;===============================================================================
; RECORD
;===============================================================================

;;****d* FastCGI/Record Types, FastCGI/FCGI_BEGIN_REQUEST, FastCGI/FCGI_ABORT_REQUEST, FastCGI/FCGI_END_REQUEST, FastCGI/FCGI_PARAMS, FastCGI/FCGI_STDIN, FastCGI/FCGI_STDOUT, FastCGI/FCGI_STDERR, FastCGI/FCGI_DATA, FastCGI/FCGI_GET_VALUES, FastCGI/FCGI_GET_VALUES_RESULT, FastCGI/FCGI_UNKNOWN_TYPE, FastCGI/FCGI_MAXTYPE
;; NAME
;; Record Types
;; SOURCE
FCGI_BEGIN_REQUEST      equ 1
FCGI_ABORT_REQUEST      equ 2
FCGI_END_REQUEST        equ 3
FCGI_PARAMS             equ 4
FCGI_STDIN              equ 5
FCGI_STDOUT             equ 6
FCGI_STDERR             equ 7
FCGI_DATA               equ 8
FCGI_GET_VALUES         equ 9
FCGI_GET_VALUES_RESULT  equ 10
FCGI_UNKNOWN_TYPE       equ 11
FCGI_MAXTYPE            equ FCGI_UNKNOWN_TYPE
;;******

;;****s* FastCGI/fcgi_header_t, FastCGI/fcgi_header_t_size
;; NAME
;; fcgi_header_t
;; SEE ALSO
;; FastCGI/fcgi_record_t
;; SOURCE
struc fcgi_header_t
    .version            resb 1
    .type               resb 1
    .request_id_b1      resb 1
    .request_id_b0      resb 1
    .content_length_b1  resb 1
    .content_length_b0  resb 1
    .padding_length     resb 1
    .reserved           resb 1
endstruc
; fcgi_header_t_size equ 8
;;******

;;****s* FastCGI/fcgi_record_t, FastCGI/fcgi_record_t_size
;; NAME
;; fcgi_record_t
;; SOURCE
struc fcgi_record_t
    .header resb fcgi_header_t_size

    ; The data of a record consists of 0-65535 bytes of content data followed by
    ; 0-255 bytes of padding. The lengths of each are sent as part of the FCGI
    ; header.
endstruc
; fcgi_record_t_size equ 8
;;******

fcgi_record_t_max_size  equ 65535 + 255

;===============================================================================
; BEGIN REQUEST RECORD
;===============================================================================

;;****d* FastCGI/Roles, FastCGI/FCGI_RESPONDER, FastCGI/FCGI_AUTHORIZER, FastCGI/FCGI_FILTER
;; NAME
;; Roles
;; SOURCE
FCGI_RESPONDER  equ BS_WORD(1)
FCGI_AUTHORIZER equ BS_WORD(2)
FCGI_FILTER     equ BS_WORD(3)
;;******

;;****d* FastCGI/Flags
;; NAME
;; Flags
;; SOURCE
FCGI_KEEP_CONN  equ 1

;;****s* FastCGI/fcgi_begin_request_body_t, FastCGI/fcgi_begin_request_body_t_size
;; NAME
;; fcgi_begin_request_body_t
;; SOURCE
struc fcgi_begin_request_body_t
    .role_b1    resb 1
    .role_b0    resb 1
    .flags      resb 1
    .reserved   resb 5
endstruc
; fcgi_begin_request_body_t_size equ 8
;;******

;;****s* FastCGI/fcgi_begin_request_record_t, FastCGI/fcgi_begin_request_record_t_size
;; NAME
;; fcgi_begin_request_record_t
;; SOURCE
struc fcgi_begin_request_record_t
    .header     resb fcgi_header_t_size
    .body       resb fcgi_begin_request_body_t_size
endstruc
; fcgi_begin_request_record_t_size equ 16
;;******

;===============================================================================
; END REQUEST RECORD
;===============================================================================

;;****d* FastCGI/Protocol Status Codes, FastCGI/FCGI_REQUEST_COMPLETE, FastCGI/FCGI_CANT_MPX_CONN, FastCGI/FCGI_OVERLOADED, FastCGI/FCGI_UNKNOWN_ROLE
;; NAME
;; Protocol Status Codes
;; SOURCE
FCGI_REQUEST_COMPLETE   equ BS_DWORD(0)
FCGI_CANT_MPX_CONN      equ BS_DWORD(1)
FCGI_OVERLOADED         equ BS_DWORD(2)
FCGI_UNKNOWN_ROLE       equ BS_DWORD(3)
;;******

;;****s* FastCGI/fcgi_end_request_body_t, FastCGI/fcgi_end_request_body_t_size
;; NAME
;; fcgi_end_request_body_t
;; SOURCE
struc fcgi_end_request_body_t
    .app_status_b3      resb 1
    .app_status_b2      resb 1
    .app_status_b1      resb 1
    .app_status_b0      resb 1
    .protocol_status    resb 1
    .reserved           resb 3
endstruc
; fcgi_end_request_body_t_size equ 8
;;******

;;****s* FastCGI/fcgi_end_request_record_t, FastCGI/fcgi_end_request_record_t_size
;; NAME
;; fcgi_end_request_record_t
;; SOURCE
struc fcgi_end_request_record_t
    .header     resb fcgi_header_t_size
    .body       resb fcgi_end_request_body_t_size
endstruc
; fcgi_end_request_record_t equ 16
;;******

;===============================================================================
; UNKNOWN TYPE RECORD
;===============================================================================

;;****s* FastCGI/fcgi_unknown_type_body_t, FastCGI/fcgi_unknown_type_body_t_size
;; NAME
;; fcgi_unknown_type_body_t
;; SOURCE
struc fcgi_unknown_type_body_t
    .type       resb 1
    .reserved   resb 7
endstruc
; fcgi_unknown_type_body_t_size equ 8
;;******

;;****s* FastCGI/fcgi_unknown_type_record_t, FastCGI/fcgi_unknown_type_record_t_size
;; NAME
;; fcgi_unknown_type_record_t
;; SOURCE
struc fcgi_unknown_type_record_t
    .header     resb fcgi_header_t_size
    .body       resb fcgi_unknown_type_body_t_size
endstruc
; fcgi_unknown_type_record_t_size equ 16
;;******

;===============================================================================
; NAME VALUE PAIRS
;===============================================================================

;;****s* FastCGI/fcgi_name_value_pair_11_t, FastCGI/fcgi_name_value_pair_11_t_size
;; NAME
;; fcgi_name_value_pair_11_t
;; SOURCE
struc fcgi_name_value_pair_11_t
    .name_length    resb 1
    .value_length   resb 1
endstruc
; fcgi_name_value_pair_11_t_size equ 2
;;******

;;****s* FastCGI/fcgi_name_value_pair_14_t, FastCGI/fcgi_name_value_pair_14_t_size
;; NAME
;; fcgi_name_value_pair_14_t
;; SOURCE
struc fcgi_name_value_pair_14_t
    .name_length        resb 1
    .value_length_b3    resb 1
    .value_length_b2    resb 1
    .value_length_b1    resb 1
    .value_length_b0    resb 1
endstruc
; fcgi_name_value_pair_14_t_size equ 5
;;******

;;****s* FastCGI/fcgi_name_value_pair_41_t, FastCGI/fcgi_name_value_pair_41_t_size
;; NAME
;; fcgi_name_value_pair_41_t
;; SOURCE
struc fcgi_name_value_pair_41_t
    .name_length_b3     resb 1
    .name_length_b2     resb 1
    .name_length_b1     resb 1
    .name_length_b0     resb 1
    .value_length       resb 1
endstruc
; fcgi_name_value_pair_41_t_size equ 5
;;******

;;****s* FastCGI/fcgi_name_value_pair_44_t, FastCGI/fcgi_name_value_pair_44_t_size
;; NAME
;; fcgi_name_value_pair_44_t
;; SOURCE
struc fcgi_name_value_pair_44_t
    .name_length_b3     resb 1
    .name_length_b2     resb 1
    .name_length_b1     resb 1
    .name_length_b0     resb 1
    .value_length_b3    resb 1
    .value_length_b2    resb 1
    .value_length_b1    resb 1
    .value_length_b0    resb 1
endstruc
; fcgi_name_value_pair_44_t_size equ 8
;;******

%endif
