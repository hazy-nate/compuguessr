# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

#****h* make/args.mk
# NAME
#   args.mk
#******

T           ?= all
MODE		?= debug
V			?= 0

ifeq ($(filter $(MODE),debug release),)
    $(error MODE must be 'debug' or 'release'. Mode provided: $(MODE))
endif

ifeq ($(V),1)
    Q :=
else
    Q := @
endif
