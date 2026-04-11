# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

ifndef DOCKER_BUILD_TARGETS_MK_INCLUDED
DOCKER_BUILD_TARGETS_MK_INCLUDED = 1

include $(MAKE_DIR)/docker/all.mk

.PHONY: docker-build-docs docker-build-emcc docker-build-full docker-build-native

docker-build-docs: docker-setup
	$(call PRINT_ACTION,$(BLUE),DOCKER BUILD,$(DOCKER_SVC_DOCS))
	$(Q)$(DOCKER_COMPOSE) build $(DOCKER_SVC_DOCS)

docker-build-emcc: docker-setup
	$(call PRINT_ACTION,$(BLUE),DOCKER BUILD,$(DOCKER_SVC_EMCC))
	$(Q)$(DOCKER_COMPOSE) build $(DOCKER_SVC_EMCC)

docker-build-full: docker-setup
	$(call PRINT_ACTION,$(BLUE),DOCKER BUILD,All)
	$(Q)$(DOCKER_COMPOSE) build

docker-build-native: docker-setup
	$(call PRINT_ACTION,$(BLUE),DOCKER BUILD,$(DOCKER_SVC_NATIVE))
	$(Q)$(DOCKER_COMPOSE) build $(DOCKER_SVC_NATIVE)

endif
