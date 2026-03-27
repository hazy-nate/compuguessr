# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

#****h* make/clang.mk
# NAME
# mk/clang.mk
# FUNCTION
# Recipes for formatting, linting, and tidying C source files with Clang Tools.
#******

#===============================================================================
# CLANG TOOLS
#===============================================================================

#****d* make/clang.mk/CLANG_C_SRCS
# NAME
# CLANG_C_SRCS
# FUNCTION
# Finds all of the C source files in SRC_DIR. This list is then used to generate
# the virtual targets for the format, lint, and tidy recipes. It is guaranteed
# to include the relative paths of each file from the current directory.
# SEE ALSO
# make/clang.mk/FORMAT_TARGETS
# make/clang.mk/LINT_TARGETS
# make/clang.mk/TIDY_TARGETS
# SOURCE
CLANG_C_SRCS 	:= $(shell find $(SRC_DIR) -name "*.c")
CLANG_C_SRCS	+= $(patsubst $(CURDIR)/%,%,$(CLANG_C_SRCS))
#******

#****d* make/clang.mk/CLANG_H_SRCS
# NAME
# CLANG_H_SRCS
# FUNCTION
# Finds all of the C header files in each directory in INC_DIRS. This list is
# then used to generate the virtual targets for the format, lint, and tidy
# recipes.  It is guaranteed to include the relative paths of each file from the
# current directory.
# SEE ALSO
# make/clang.mk/FORMAT_TARGETS
# make/clang.mk/LINT_TARGETS
# make/clang.mk/TIDY_TARGETS
# SOURCE
CLANG_H_SRCS	:= $(foreach dir, $(INC_DIRS), $(shell find $(dir) -name "*.h"))
CLANG_H_SRCS	+= $(patsubst $(CURDIR)/%,%,$(CLANG_H_SRCS))
#******

#****d* make/clang.mk/ALL_CLANG_SRCS
# NAME
# ALL_CLANG_SRCS
# FUNCTION
# Combines the lists of C source and header files for convenience.
# SEE ALSO
# make/clang.mk/CLANG_C_SRCS
# make/clang.mk/CLANG_H_SRCS
# make/clang.mk/FORMAT_TARGETS
# make/clang.mk/LINT_TARGETS
# SOURCE
ALL_CLANG_SRCS 	:= $(CLANG_C_SRCS) $(CLANG_H_SRCS)
#******

#****d* make/clang.mk/FORMAT_TARGETS
# NAME
# FORMAT_TARGETS
# FUNCTION
# Generates virtual targets for the format recipe.
# SEE ALSO
# make/clang.mk/LINT_TARGETS
# make/clang.mk/TIDY_TARGETS
# SOURCE
FORMAT_TARGETS  := $(ALL_CLANG_SRCS:%=format-%)
#******

#****d* make/clang.mk/LINT_TARGETS
# NAME
# LINT_TARGETS
# FUNCTION
# Generates virtual targets for the lint recipe.
# SEE ALSO
# make/clang.mk/FORMAT_TARGETS
# make/clang.mk/TIDY_TARGETS
LINT_TARGETS    := $(ALL_CLANG_SRCS:%=lint-%)
#******

#****d* make/clang.mk/TIDY_TARGETS
# NAME
# TIDY_TARGETS
# FUNCTION
# Generates virtual targets for the tidy recipe. make/clang.mk/ALL_CLANG_SRCS isn't
# used here because the headers are checked when included in a C source file.
# SEE ALSO
# make/clang.mk/CLANG_C_SRCS
# make/clang.mk/FORMAT_TARGETS
# make/clang.mk/LINT_TARGETS
# SOURCE
TIDY_TARGETS    := $(CLANG_C_SRCS:%=tidy-%)
#******

.PHONY: compiledb format $(FORMAT_TARGETS) lint $(LINT_TARGETS) tidy $(TIDY_TARGETS)

#===============================================================================
# COMPILATION DATABASE
#===============================================================================

#****f* make/clang.mk/compiledb
# NAME
# compiledb
# SYNOPSIS
# make compiledb
# FUNCTION
# Forces a compile_commands.json file to be generated with bear.
# SOURCE
compiledb: clean
	@printf "$(MAGENTA)$(BOLD)BEAR$(RESET)     $(MAGENTA)Generating compile_commands.json...$(RESET)\n"
	$(Q)bear -- $(MAKE) all MODE=debug --no-print-directory
	@printf "$(GREEN)$(BOLD)SUCCESS$(RESET)  Generated: $(BOLD)compile_commands.json$(RESET)\n"
#******

#****f* make/clang.mk/compile_commands.json
# NAME
# compile_commands.json
# SYNOPSIS
# make compile_commands.json
# FUNCTION
# Generates a compile_commands.json file with bear if it doesn't exist.
# SOURCE
compile_commands.json:
	$(Q)$(MAKE) compiledb --no-print-directory
#******

#===============================================================================
# LINTING
#===============================================================================

#****f* make/clang.mk/lint
# NAME
#   lint
# SYNOPSIS
#   make lint
#   make lint-<file-path>
# SOURCE
lint: $(LINT_TARGETS)

$(LINT_TARGETS): lint-%: %
	@printf "$(BLUE)$(BOLD)LINT$(RESET)     $(BLUE)$<$(RESET)\n"
	$(Q)$(CLANG_FORMAT) --Werror --dry-run $<
#******

#===============================================================================
# FORMATTING
#===============================================================================

#****f* make/clang.mk/format
# NAME
#   format
# SYNOPSIS
#   make format
#   make format-<file-path>
# SOURCE
format: $(FORMAT_TARGETS)

$(FORMAT_TARGETS): format-%: %
	@printf "$(CYAN)$(BOLD)FORMAT$(RESET)   $(CYAN)$<$(RESET)\n"
	$(Q)$(CLANG_FORMAT) -i $<
#******

#===============================================================================
# STATIC ANALYSIS
#===============================================================================

#****f* make/clang.mk/tidy
# NAME
#   tidy
# SYNOPSIS
#   make tidy
#   make tidy-<file-path>
# SOURCE
tidy: compile_commands.json $(TIDY_TARGETS)

$(TIDY_TARGETS): tidy-%: %
	@printf "$(CYAN)$(BOLD)TIDY$(RESET)     $(CYAN)$<$(RESET)\n"
	$(Q)$(CLANG_TIDY) -p . $<
#******
