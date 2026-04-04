# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

ifndef DOCKER_BUILD_TARGETS_MK_INCLUDED
DOCKER_BUILD_TARGETS_MK_INCLUDED = 1

include $(MAKE_DIR)/docker/all.mk

.PHONY: docker-build-docs docker-build-emcc docker-build-full docker-build-native

docker-build-docs: docker-setup
	$(Q)$(DOCKER_COMPOSE) build $(DOCKER_SVC_DOCS)

docker-build-emcc: docker-setup
	$(Q)$(DOCKER_COMPOSE) build $(DOCKER_SVC_EMCC)

docker-build-full: docker-setup
	$(Q)$(DOCKER_COMPOSE) build

docker-build-native: docker-setup
	$(Q)$(DOCKER_COMPOSE) build $(DOCKER_SVC_NATIVE)

endif
