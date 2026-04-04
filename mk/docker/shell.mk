# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

ifndef DOCKER_SHELL_MK_INCLUDED
DOCKER_SHELL_MK_INCLUDED = 1

include $(MAKE_DIR)/docker/all.mk

.PHONY: docker-shell-docs docker-shell-emcc docker-shell-full docker-shell-native

docker-shell-docs:
	$(Q)$(DOCKER_COMPOSE) run --rm -it $(DOCKER_SVC_DOCS) /bin/bash

docker-shell-emcc:
	$(Q)$(DOCKER_COMPOSE) run --rm -it $(DOCKER_SVC_EMCC) /bin/bash

docker-shell-full:
	$(Q)$(DOCKER_COMPOSE) run --rm -it $(DOCKER_SVC_FULL) /bin/bash

docker-shell-native:
	$(Q)$(DOCKER_COMPOSE) run --rm -it $(DOCKER_SVC_NATIVE) /bin/bash

endif
