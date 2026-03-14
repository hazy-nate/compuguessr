
LIGHTTPD_CONF 	:= $(TEST_DIR)/lighttpd_stdin.conf
TEST_FCGI 		:= $(TEST_DIR)/test_fcgi.py
LIGHTTPD_PID  	:= /tmp/compuguessr_lighttpd.pid
LIGHTTPD_SOCK	:= /tmp/compuguessr.sock

.PHONY: test test-clean test-fastcgi test-fastcgi-clean

test: test-fastcgi

test-clean: test-fastcgi-clean

test-fastcgi: test-fastcgi-clean
	@printf "$(CYAN)$(BOLD)TEST$(RESET)     Starting Lighttpd (Background)...\n"
	@lighttpd -D -f $(LIGHTTPD_CONF) & echo $$! > $(LIGHTTPD_PID)
	@printf "$(BLUE)$(BOLD)WAIT$(RESET)     Initializing server...\n"
	@sleep 1
	@printf "$(BLUE)$(BOLD)RUN$(RESET)      Executing FastCGI Integration Tests...\n"
	@python3 $(TEST_FCGI) ; \
	RESULT=$$? ; \
	if [ $$RESULT -eq 0 ]; then \
		printf "$(GREEN)$(BOLD)PASS$(RESET)     Integration tests completed successfully\n"; \
	else \
		printf "$(RED)$(BOLD)FAIL$(RESET)     Integration tests failed (Exit: $$RESULT)\n"; \
	fi; \
	printf "$(BLUE)$(BOLD)STOP$(RESET)     Shutting down Lighttpd...\n" ; \
	kill $$(cat $(LIGHTTPD_PID)) && rm $(LIGHTTPD_PID) ; \
	exit $$RESULT

test-fastcgi-clean:
	@if [ -f $(LIGHTTPD_PID) ]; then \
		printf "$(YELLOW)$(BOLD)CLEAN$(RESET)    Killing orphan Lighttpd process...\n"; \
		kill $$(cat $(LIGHTTPD_PID)) || true; \
		rm $(LIGHTTPD_PID); \
	fi
	@if [ -S $(TEST_SOCK) ]; then \
		printf "$(YELLOW)$(BOLD)CLEAN$(RESET)    Removing test socket: $(TEST_SOCK)\n"; \
		rm -f $(TEST_SOCK); \
	fi
