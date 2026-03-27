# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

#****h* make/misc.mk
# NAME
#   misc.mk
#******

#****f* make/misc.mk/gdb
# NAME
#   gdb
# SOURCE
gdb: $(BIN_OUT_DIR)/$(PROG)
	@printf "$(YELLOW)$(BOLD)GDB$(RESET)      $(YELLOW)$(BIN_OUT_DIR)/$(PROG)$(RESET)\n"
	$(Q)LD_LIBRARY_PATH=$(BIN_OUT_DIR) gdb -q -iex "set auto-load safe-path /" -x .gdbinit $(BIN_OUT_DIR)/$(PROG)
#******

#****f* make/misc.mk/size
# NAME
#   size
# SOURCE
size: $(BIN_OUT_DIR)/$(PROG)
	@printf "$(YELLOW)$(BOLD)SIZE$(RESET)     $(YELLOW)$<$(RESET)\n"
	$(Q)size --format=sysv --radix=10 $<
#******

#****f* make/misc.mk/TAGS, make/misc.mk/tags
# NAME
#   TAGS tags
# SOURCE
TAGS tags:
	@printf "$(MAGENTA)$(BOLD)TAGS$(RESET)     $(MAGENTA)Generating ctags for source files...$(RESET)\n"
	$(Q)rm -f tags TAGS
	$(Q)$(CTAGS) $(INC_SRCS) $(SRCS)
#******

#****f* make/misc.mk/debug
# NAME
#   debug
# SOURCE
valgrind: debug
	@printf "$(YELLOW)$(BOLD)VALGRIND$(RESET) $(YELLOW)$(BIN_OUT_DIR)/$(PROG)$(RESET)\n"
	$(Q)LD_LIBRARY_PATH=$(BIN_OUT_DIR) $(VALGRIND) \
		--leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--error-exitcode=1 \
		$(BIN_OUT_DIR)/$(PROG)
#******
