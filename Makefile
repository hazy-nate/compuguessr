
include		mk/args.mk

PROG		= compuguessr
VERSION		= 0.1

CC 				= clang
ASM				= nasm
LD				= mold
STRIP			= strip
OBJCOPY			= objcopy
CLANG_FORMAT 	= clang-format
CLANG_TIDY		= clang-tidy
CTAGS			= ctags
VALGRIND		= valgrind

BASE_DIR	= $(shell pwd)
BIN_DIR		= $(BASE_DIR)/bin
BUILD_DIR	= $(BASE_DIR)/build
DOCS_DIR	= $(BASE_DIR)/docs
INC_DIRS	= $(BASE_DIR)/include
LIB_DIRS	= /lib64 /usr/lib64
SRC_DIR		= $(BASE_DIR)/src
TEST_DIR	= $(BASE_DIR)/test

CC_FLAGS	= -Wall -Wcast-align -Wextra -Wformat=2 -Wpedantic -Wpointer-arith \
			  -Wstrict-prototypes -Wshadow -std=c17 -ffreestanding -fno-builtin -fno-stack-protector
CPP_FLAGS 	=
AS_FLAGS	=
ASM_FLAGS	= -w+macro-params -w+number-overflow -w+orphan-labels -f elf64
LD_FLAGS	= -m elf_x86_64 -e _start -nostdlib -pie -static

ifeq ($(MODE), release)
	CC_FLAGS	+= -Os -DNDEBUG -fno-asynchronous-unwind-tables -fno-unwind-tables
	ASM_FLAGS	+= -O3
	LD_FLAGS	+= -z relro -z now -gc-sections --strip-all
else ifeq ($(MODE), debug)
	CC_FLAGS	+= -Werror -O0 -g -DDEBUG
	ASM_FLAGS	+= -Werror -O0 -g
	LD_FLAGS	+=
endif

LIBS		=

include		mk/src.mk
-include	mk/clang.mk
-include	mk/docs.mk
-include	mk/misc.mk
-include	mk/test.mk
