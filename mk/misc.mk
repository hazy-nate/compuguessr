
FORMAT_TARGETS 	:= $(CC_SRCS:%=format-%) $(CH_SRCS:%=format-%)
LINT_TARGETS 	:= $(CC_SRCS:%=lint-%) $(CH_SRCS:%=lint-%)
TIDY_TARGETS	:= $(CC_SRCS:%=tidy-%) $(CH_SRCS:%=tidy-%)

.PHONY:  dist gdb size TAGS tags valgrind

dist: $(BIN_OUT_DIR)/$(PROG)
	$(Q)$(STRIP) --strip-all $<
	@printf "$(BLUE)$(BOLD)STRIP$(RESET)    $(BLUE)$<$(RESET)\n"
	$(Q)$(OBJCOPY) --remove-section=.comment --remove-section=.note.gnu.build-id $<
	@printf "$(BLUE)$(BOLD)OBJCOPY$(RESET)  $(BLUE)$<$(RESET)\n"
	@if command -v checksec >/dev/null; then \
		checksec --file=$<; \
	fi
	@printf "$(GREEN)$(BOLD)SUCCESS$(RESET)  Binary modified: $(BOLD)$<$(RESET) ($(YELLOW)$$(stat -c%s $<) bytes$(RESET))\n"

gdb:
	$(Q)gdb -q -iex "set auto-load safe-path /" -x .gdbinit $<

size: $(BIN_OUT_DIR)/$(PROG)
	@printf "$(YELLOW)$(BOLD)SIZE$(RESET)     $(YELLOW)$<$(RESET)\n"
	$(Q)size --format=sysv --radix=10 $<

TAGS tags: mostlyclean
	$(Q)rm -f tags
	$(Q)$(CTAGS) $(INC_SRCS) $(SRCS)
	@$(foreach file, $(INC_SRCS), printf "$(MAGENTA)$(BOLD)TAGS$(RESET)     $(MAGENTA)$(file)$(RESET)\n";)
	@$(foreach file, $(SRCS), printf "$(MAGENTA)$(BOLD)TAGS$(RESET)     $(MAGENTA)$(file)$(RESET)\n";)

valgrind: debug
	@printf "$(YELLOW)$(BOLD)VALGRIND$(RESET) $(YELLOW)$(BIN_OUT_DIR)/$(PROG)$(RESET)\n"
	$(Q)$(VALGRIND) --leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		$(BIN_OUT_DIR)/$(PROG)
