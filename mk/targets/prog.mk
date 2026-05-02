# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

ifndef TARGET_PROG_MK_INCLUDED
TARGET_PROG_MK_INCLUDED = 1

PROG	:= compuguessr
TARGETS	+= $(PROG)

$(PROG)_VERSION 	:= 0.1
$(PROG)_TYPE		:= EXE
$(PROG)_LDFLAGS		:= -e _start -z relro -z now -pie
$(PROG)_LDLIBS		:=
$(PROG)_DEPS		:=

ifeq ($(MODE), release)
	$(PROG)_CFLAGS	+= -Os -DNDEBUG -fno-asynchronous-unwind-tables -fno-unwind-tables
	$(PROG)_ASFLAGS	+= -O3
	$(PROG)_LDFLAGS	+= -gc-sections --strip-all
else ifeq ($(MODE), debug)
	$(PROG)_CFLAGS	+= -Werror -O0 -g -DDEBUG
	$(PROG)_ASFLAGS	+= -Werror -O0 -g
	$(PROG)_LDFLAGS	+=
endif

$(PROG)_OUT := $(BIN_OUT_DIR)/$(PROG)

.PHONY: challenges prog

challenges:
	scripts/process_challenges.py

prog: $($(PROG)_OUT)

$(BUILD_OUT_DIR)/src/data/cg_challengedb_data.i: challenges

include $(SRC_DIR)/prog.build.mk

endif
