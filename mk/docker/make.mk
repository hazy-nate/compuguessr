# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

ifndef DOCKER_MAKE_MK_INCLUDED
DOCKER_MAKE_MK_INCLUDED = 1

include $(MAKE_DIR)/docker/all.mk

.PHONY: docker-make-docs docker-make-emcc docker-make-native

docker-make-docs:
	$(Q)$(DOCKER_COMPOSE) run --rm $(DOCKER_SVC_DOCS) make $(T)

docker-make-emcc:
	$(Q)$(DOCKER_COMPOSE) run --rm $(DOCKER_SVC_EMCC) make $(T)

docker-make-native:
	$(Q)$(DOCKER_COMPOSE) run --rm $(DOCKER_SVC_NATIVE) make $(T)

endif
