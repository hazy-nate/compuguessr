# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

ifndef PROG_BUILD_MK_INCLUDED
PROG_BUILD_MK_INCLUDED = 1

$(PROG)_SRC_DIRNAMES	:= app arch core data platform services util
$(PROG)_SRC_DIRS		:= $(addprefix $(SRC_DIR)/,$($(PROG)_SRC_DIRNAMES))

$(PROG)_SRCS := $(shell find $($(PROG)_SRC_DIRS) -maxdepth 2 -name "*.c" 2>/dev/null) \
			    $(shell find $($(PROG)_SRC_DIRS) -maxdepth 2 -name "*.s" 2>/dev/null) \
			    $(shell find $($(PROG)_SRC_DIRS) -maxdepth 2 -name "*.S" 2>/dev/null) \
			    $(shell find $($(PROG)_SRC_DIRS) -maxdepth 2 -name "*.S" 2>/dev/null) \
			    $(shell find $($(PROG)_SRC_DIRS) -maxdepth 2 -name "*.asm" 2>/dev/null)
$(PROG)_NIM_SRCS := $(shell find $($(PROG)_SRC_DIRS) -maxdepth 2 -name "*.nim" 2>/dev/null)

$(PROG)_OBJS += $(patsubst %,%.o,$($(PROG)_SRCS))
$(PROG)_OBJS += $(patsubst %,%.a,$($(PROG)_NIM_SRCS))

NASM_INCS += $(foreach dir, $(INC_DIRS),$(shell find $(dir) -name "*.inc" 2>/dev/null))

endif
