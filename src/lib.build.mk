# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

ifndef LIB_BUILD_MK_INCLUDED
LIB_BUILD_MK_INCLUDED = 1

$(LIB)_SRC_DIRNAMES	:= arch core data platform services util
$(LIB)_SRC_DIRS		:= $(addprefix $(SRC_DIR)/,$($(LIB)_SRC_DIRNAMES))

$(LIB)_SRCS := $(shell find $($(LIB)_SRC_DIRS) -maxdepth 2 -name "*.c" 2>/dev/null) \
			   $(shell find $($(LIB)_SRC_DIRS) -maxdepth 2 -name "*.s" 2>/dev/null) \
			   $(shell find $($(LIB)_SRC_DIRS) -maxdepth 2 -name "*.S" 2>/dev/null) \
			   $(shell find $($(LIB)_SRC_DIRS) -maxdepth 2 -name "*.S" 2>/dev/null) \
			   $(shell find $($(LIB)_SRC_DIRS) -maxdepth 2 -name "*.asm" 2>/dev/null)
$(LIB)_NIM_SRCS := $(shell find $($(LIB)_SRC_DIRS) -maxdepth 2 -name "*.nim" 2>/dev/null)

$(LIB)_OBJS += $(patsubst %,%.o,$($(LIB)_SRCS))
$(LIB)_OBJS += $(patsubst %,%.a,$($(LIB)_NIM_SRCS))

NASM_INCS += $(foreach dir, $(INC_DIRS),$(shell find $(dir) -name "*.inc" 2>/dev/null))

endif
