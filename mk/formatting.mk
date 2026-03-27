# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

#****h* make/formatting.mk
# NAME
#   formatting.mk
# FUNCTION
#   Defines variables for each of the colors (if supported).
#******

ifeq ($(shell [ -t 1 ] || printf no), no)
    RESET	:= $(shell tput sgr0)

    BLACK   := $(shell tput setaf 0)
    RED    	:= $(shell tput setaf 1)
    GREEN  	:= $(shell tput setaf 2)
    YELLOW 	:= $(shell tput setaf 3)
    BLUE   	:= $(shell tput setaf 4)
    MAGENTA	:= $(shell tput setaf 5)
    CYAN   	:= $(shell tput setaf 6)
    WHITE	:= $(shell tput setaf 7)
    GRAY	:= $(shell tput setaf 8)

    BOLD   	:= $(shell tput bold)
    ITALIC  := $(shell tput sitm)
    UNDER   := $(shell tput smul)
    NOUNDER := $(shell tput rmul)
endif
