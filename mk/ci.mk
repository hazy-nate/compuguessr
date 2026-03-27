
#****h* make/ci.mk
# NAME
#   ci.mk
#******

ci:
	@printf "$(MAGENTA)$(BOLD)ACT$(RESET)      $(MAGENTA)Running CI workflows locally...$(RESET)\n"
	$(Q)act

ci-list:
	$(Q)act -l

