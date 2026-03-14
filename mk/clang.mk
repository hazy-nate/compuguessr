
FORMAT_TARGETS 	:= $(CC_SRCS:%=format-%) $(CH_SRCS:%=format-%)
LINT_TARGETS 	:= $(CC_SRCS:%=lint-%) $(CH_SRCS:%=lint-%)
TIDY_TARGETS	:= $(CC_SRCS:%=tidy-%) $(CH_SRCS:%=tidy-%)

.PHONY: compiledb format $(FORMAT_TARGETS) lint $(LINT_TARGETS) tidy $(TIDY_TARGETS)

compiledb: clean
	$(Q)bear -- $(MAKE) all MODE=debug --no-print-directory
	@printf "$(MAGENTA)$(BOLD)BEAR$(RESET)     Generated: $(BOLD)compile_commands.json$(RESET)\n"

lint: $(LINT_TARGETS)

$(LINT_TARGETS): lint-%: %
	@printf "$(BLUE)$(BOLD)LINT$(RESET)     $(BLUE)$<$(RESET)\n"
	$(Q)$(CLANG_FORMAT) --Werror --dry-run $<

format: $(FORMAT_TARGETS)

$(FORMAT_TARGETS): format-%: %
	@printf "$(CYAN)$(BOLD)FORMAT$(RESET)   $(CYAN)$<$(RESET)\n"
	$(Q)$(CLANG_FORMAT) -i $<

tidy: $(TIDY_TARGETS)

$(TIDY_TARGETS): tidy-%: %
	@printf "$(CYAN)$(BOLD)TIDY$(RESET)     $(CYAN)$<$(RESET)\n"
	$(Q)$(CLANG_TIDY) -p . $<
