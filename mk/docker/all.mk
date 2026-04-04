# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

ifndef DOCKER_MK_INCLUDED
DOCKER_MK_INCLUDED = 1

#****h* make/docker.mk
# NAME
#   docker.mk
# FUNCTION
#   Granular Docker management for specialized build environments.
#******

include $(MAKE_DIR)/git.mk

export USER_ID	?= $(shell id -u)
export GROUP_ID	?= $(shell id -g)

DOCKER_BIN		:= $(shell which docker 2>/dev/null || which podman 2>/dev/null || echo docker)
DOCKER_COMPOSE	:= $(DOCKER_BIN) compose

DOCKER_SVC_NATIVE	:= compuguessr-dev-native
DOCKER_SVC_EMCC		:= compuguessr-dev-emcc
DOCKER_SVC_DOCS		:= compuguessr-dev-docs
DOCKER_SVC_FULL		:= compuguessr-dev-full
DOCKER_SVC_RELEASE	:= compuguessr

.PHONY: docker-release docker-setup

docker-release: docker-setup
	@printf "Building production release image...\n"
	$(Q)$(DOCKER_COMPOSE) build $(DOCKER_SVC_RELEASE)

docker-setup: git-submodules
	$(Q)git submodule update --init --recursive --depth 1

endif
