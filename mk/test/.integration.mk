
DOCKER_BIN	:= $(shell which docker 2>/dev/null || podman)

VALKEY_COMPOSE_FILE := $(TEST_DIR)/env/docker/compose.valkey.yaml

.PHONY: test-integration-valkey

test-integration-valkey: docker-build
	@printf "$(BLUE)$(BOLD)TEST$(RESET)     $(BLUE)Running Valkey Integration Tests...$(RESET)\n"

	$(Q)$(DOCKER_BIN) compose -f $(VALKEY_COMPOSE_FILE) down -v --remove-orphans 2>/dev/null || true

	$(Q)$(DOCKER_BIN) compose -f $(VALKEY_COMPOSE_FILE) up \
		--build \
		--abort-on-container-exit \
		--exit-code-from tester; \
	TEST_EXIT_CODE=$$?; \
	printf "$(BLUE)$(BOLD)TEST$(RESET)     $(BLUE)Tearing down Valkey test environment...$(RESET)\n"; \
	$(DOCKER_BIN) compose -f $(VALKEY_COMPOSE_FILE) down -v --remove-orphans; \
	exit $$TEST_EXIT_CODE
