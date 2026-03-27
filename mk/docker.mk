# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

#****h* make/docker.mk
# NAME
#   docker.mk
#******

#****d* make/docker.mk/DOCKER_DEV_IMAGE
# NAME
#   DOCKER_DEV_IMAGE
# SOURCE
DOCKER_DEV_IMAGE		:= "compuguessr-dev"
#******

#****d* make/docker.mk/DOCKER_IMAGE
# NAME
#   DOCKER_IMAGE
# SOURCE
DOCKER_IMAGE	:= "compuguessr"
#******

#****d* make/docker.mk/DOCKER_DEV_SERVICE
# NAME
#   DOCKER_DEV_SERVICE
# SOURCE
DOCKER_DEV_SERVICE		:= "compuguessr-dev"
#******

#****d* make/docker.mk/DOCKER_RELEASE_SERVICE
# NAME
#   DOCKER_RELEASE_SERVICE
# SOURCE
DOCKER_RELEASE_SERVICE	:= "compuguessr"
#******

#****d* make/docker.mk/DOCKER_BIN
# NAME
#   DOCKER_BIN
# SOURCE
DOCKER_BIN	:= $(shell which docker 2>/dev/null || podman)
#******

#****d* make/docker.mk/USER_ID
# NAME
#   USER_ID
# SOURCE
USER_ID		:= $(shell id -u)
#******

#****d* make/docker.mk/GROUP_ID
# NAME
#   GROUP_ID
# SOURCE
GROUP_ID	:= $(shell id -g)
#******

.PHONY: docker-build docker-build-dev docker-make docker-shell

#****f* make/docker.mk/docker-build-dev
# NAME
#   docker-build-dev
# SOURCE
docker-build-dev:
	$(Q)$(DOCKER_BIN) build --target builder -t $(DOCKER_DEV_IMAGE) .
#******

#****f* make/docker.mk/docker-build
# NAME
#   docker-build
# SOURCE
docker-build:
	$(Q)$(DOCKER_BIN) build -t $(DOCKER_IMAGE) .
#******

#****f* make/docker.mk/docker-make
# NAME
#   docker-make
# SOURCE
docker-make:
	$(Q)$(DOCKER_BIN) compose run --rm --user $(USER_ID):$(GROUP_ID) $(DOCKER_DEV_SERVICE) make $(T)
#******

#****f* make/docker.mk/docker-shell
# NAME
#   docker-shell
# SOURCE
docker-shell:
	@printf "$(BLUE)$(BOLD)DOCKER$(RESET)   $(BLUE)Entering development shell...$(RESET)\n"
	$(Q)$(DOCKER_BIN) compose run --rm --user $(USER_ID):$(GROUP_ID) -it $(DOCKER_DEV_SERVICE) /bin/bash
#******

