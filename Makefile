# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

#===============================================================================
# ARGUMENTS
#===============================================================================

MODE ?= debug

#===============================================================================
# TOOLCHAIN
#===============================================================================

CC	?= clang
CXX	?= clang
AS	?= cc
ASM	?= nasm
NIM	?= nim
LD	:= mold
AR	:= ar

#===============================================================================
# DIRECTORY PATHS
#===============================================================================

BASE_DIR		:= $(shell pwd)
BIN_DIR			:= $(BASE_DIR)/bin
BUILD_DIR		:= $(BASE_DIR)/build
DIST_DIR		:= $(BASE_DIR)/dist
DOCS_DIR		:= $(BASE_DIR)/docs
INC_DIRS		:= $(BASE_DIR)/include
LIB_DIRS		:= /lib64 /usr/lib64
SCRIPTS_DIR		:= $(BASE_DIR)/scripts
SRC_DIR			:= $(BASE_DIR)/src
TEST_DIR		:= $(BASE_DIR)/test

BIN_OUT_DIR		:= $(BIN_DIR)/$(MODE)
BUILD_OUT_DIR	:= $(BUILD_DIR)/$(MODE)

EXTERNAL_DIR	:= $(BASE_DIR)/external
MAKE_DIR		:= $(BASE_DIR)/mk
OVERLAY_DIR		:= $(BASE_DIR)/overlay
VENDOR_DIR		:= $(BASE_DIR)/vendor

MAKE_STARTER_DIR := $(VENDOR_DIR)/make-starter

#===============================================================================
# GLOBAL FLAGS
#===============================================================================

C_STD		:= c23
CFLAGS		?= -std=$(C_STD) -Wall -Wcast-align -Wextra -Wformat=2 -Wpedantic \
			   -Wpointer-arith -Wstrict-prototypes -Wshadow -ffreestanding \
			   -fno-builtin -fno-stack-protector
CFLAGS		+=
CPPFLAGS	?=
CXXFLAGS	?=
ASFLAGS		?= -f elf64 -w+macro-params -w+number-overflow -w+orphan-labels
NIMFLAGS	?= -d:danger --mm:arc --opt:size --passC:-fno-stack-protector
LDFLAGS		?= -m elf_x86_64 -nostdlib -pie
LDLIBS		?=
ARFLAGS		?= rcs

#===============================================================================
# CLEAN
#===============================================================================

clean:			CLEAN_DIRS += $(BIN_DIR)
mostlyclean:	CLEAN_DIRS += $(BUILD_DIR)

#===============================================================================
# CUSTOM PHONY TARGETS
#===============================================================================

.PHONY: debug release

debug:
	@$(MAKE) all MODE=debug Q=@ --no-print-directory

release:
	@$(MAKE) all MODE=release Q=@ --no-print-directory

#===============================================================================
# INCLUDES
#===============================================================================

include $(shell find "$(MAKE_DIR)/targets" -type f -name "*.mk" -not -name ".*" 2>/dev/null)
include $(MAKE_STARTER_DIR)/src.mk
-include $(shell find "$(MAKE_DIR)" -type f -name "*.mk" -not -name ".*" 2>/dev/null)
