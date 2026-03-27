# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

#****h* make/Makefile
# NAME
#   Makefile
#******

#===============================================================================
# INCLUDES
#===============================================================================

include		mk/args.mk

#===============================================================================
# EXECUTABLE PATHS
#===============================================================================

CC              := $(shell which clang 2>/dev/null || which gcc)
ifeq ($(CC),)
    $(error "No C compiler (clang or gcc) found in PATH!")
endif

ASM				:= nasm
LD				:= mold
STRIP			:= strip
OBJCOPY			:= objcopy
CLANG_FORMAT 	:= clang-format
CLANG_TIDY		:= clang-tidy
CTAGS			:= ctags
VALGRIND		:= valgrind

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
SRC_DIR			:= $(BASE_DIR)/src
TEST_DIR		:= $(BASE_DIR)/test

BIN_OUT_DIR		:= $(BIN_DIR)/$(MODE)
BUILD_OUT_DIR	:= $(BUILD_DIR)/$(MODE)

#===============================================================================
# FLAGS
#===============================================================================

CC_STD		:= c11
CC_FLAGS	:= -Wall -Wcast-align -Wextra -Wformat=2 -Wpedantic -Wpointer-arith \
			   -Wstrict-prototypes -Wshadow -ffreestanding -fno-builtin \
			   -fno-stack-protector -fPIC -std=$(CC_STD)
CPP_FLAGS 	:=
AS_FLAGS	:=
ASM_FLAGS	:= -f elf64 -w+macro-params -w+number-overflow -w+orphan-labels
LD_FLAGS	:= -m elf_x86_64 -nostdlib

#===============================================================================
# BUILD TARGETS
#===============================================================================

LIB					:= libcompuguessr.so
PROG				:= compuguessr
TARGETS				:= $(LIB) $(PROG)

# Compile as a shared library
$(LIB)_VERSION		:= 0.1
$(LIB)_TYPE			:= SO
$(LIB)_SRC_DIRS		:= $(SRC_DIR)/sys $(SRC_DIR)/util
$(LIB)_LD_FLAGS		:= -shared -Bsymbolic

# Compile as an executable
$(PROG)_VERSION 	:= 0.1
$(PROG)_TYPE		:= EXE
$(PROG)_SRC_DIRS	:= $(SRC_DIR)
$(PROG)_LD_FLAGS    := -pie -e _start -z relro -z now
$(PROG)_LD_LIBS     :=
$(PROG)_DEPS		:=

# Compile as an executable that depends on the shared library
# $(PROG)_VERSION 	:= 0.1
# $(PROG)_TYPE		:= EXE
# $(PROG)_SRC_DIRS	:= $(SRC_DIR)/bin
# $(PROG)_LD_FLAGS	:= -L$(BIN_OUT_DIR) -pie -e _start -z now \
# 					  -dynamic-linker /lib64/ld-linux-x86-64.so.2
# $(PROG)_LD_LIBS     := -lcompuguessr
# $(PROG)_DEPS		:= libcompuguessr.so

ifeq ($(MODE), release)
	$(PROG)_CC_FLAGS	+= -Os -DNDEBUG -fno-asynchronous-unwind-tables -fno-unwind-tables
	$(PROG)_ASM_FLAGS	+= -O3
	$(PROG)_LD_FLAGS	+= -gc-sections --strip-all

else ifeq ($(MODE), debug)
	$(PROG)_CC_FLAGS	+= -Werror -O0 -g -DDEBUG
	$(PROG)_ASM_FLAGS	+= -Werror -O0 -g
	$(PROG)_LD_FLAGS	+=
endif

LD_LIBS		:=

include		mk/src.mk

#===============================================================================
# ADDITIONAL INCLUDES
#===============================================================================

-include	mk/ci.mk
-include	mk/clang.mk
-include	mk/dist.mk
-include	mk/docker.mk
-include	mk/docs.mk
-include	mk/misc.mk
-include	mk/test/test.mk
