# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

import ctypes
from cg_bindings import CgEnv

def test_env_initialization(lib):
    envp_data = [b"FASTCGI_SOCK=/tmp/custom.sock", None]
    envp_arr = (ctypes.c_char_p * len(envp_data))(*envp_data)
    env_struct = CgEnv()
    lib.cg_env_init(envp_arr, ctypes.byref(env_struct))
    assert env_struct.fastcgi_sock == b"/tmp/custom.sock"
    assert env_struct.fastcgi_sock_len == 16
