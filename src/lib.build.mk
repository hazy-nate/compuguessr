# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

ifndef LIB_BUILD_MK_INCLUDED
LIB_BUILD_MK_INCLUDED = 1

LOCAL_SRC_DIRNAMES	:= client fastcgi pq resp sys util
LOCAL_SRC_DIRS		:= $(addprefix $(SRC_DIR)/,$(LOCAL_SRC_DIRNAMES))

LOCAL_SRCS := $(shell find $(LOCAL_SRC_DIRS) -maxdepth 1 -name "*.c" 2>/dev/null) \
			  $(shell find $(LOCAL_SRC_DIRS) -maxdepth 1 -name "*.s" 2>/dev/null) \
			  $(shell find $(LOCAL_SRC_DIRS) -maxdepth 1 -name "*.S" 2>/dev/null) \
			  $(shell find $(LOCAL_SRC_DIRS) -maxdepth 1 -name "*.S" 2>/dev/null) \
			  $(shell find $(LOCAL_SRC_DIRS) -maxdepth 1 -name "*.asm" 2>/dev/null)
LOCAL_NIM_SRCS := $(shell find $(LOCAL_SRC_DIRS) -maxdepth 1 -name "*.nim" 2>/dev/null)

$(LIB)_OBJS += $(patsubst %,%.o,$(LOCAL_SRCS))
$(LIB)_OBJS += $(patsubst %,%.a,$(LOCAL_NIM_SRCS))

NASM_INCS += $(foreach dir, $(INC_DIRS),$(shell find $(dir) -name "*.inc" 2>/dev/null))

endif
