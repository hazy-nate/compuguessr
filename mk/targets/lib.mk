# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

ifndef TARGET_LIB_MK_INCLUDED
TARGET_LIB_MK_INCLUDED = 1

include $(MAKE_STARTER_DIR)/funcs.mk

LIB		:= libcompuguessr.so
TARGETS	+= $(LIB)

$(LIB)_VERSION	:= 0.1
$(LIB)_TYPE		:= SO
$(LIB)_LDFLAGS	:= -shared -Bsymbolic
$(LIB)_LDLIBS	:=
$(LIB)_DEPS		:=
$(LIB)_CFLAGS	:= -fPIC

$(LIB)_OUT := $(BIN_OUT_DIR)/$(LIB)

.PHONY: lib

lib: $($(LIB)_OUT)

include $(SRC_DIR)/lib.build.mk

endif
