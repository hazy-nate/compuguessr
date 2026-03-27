# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

#****h* make/dist.mk
# NAME
#   dist.mk
#******

#****d* make/dist.mk/DIST_STAGING_DIR
# NAME
#   DIST_STAGING_DIR
# SOURCE
DIST_STAGING_DIR		:= $(DIST_DIR)/staging
#******

#****d* make/dist.mk/DIST_ARCHIVE
# NAME
#   DIST_ARCHIVE
# SOURCE
DIST_ARCHIVE			:= $(DIST_DIR)/$(PROG)-$($(PROG)_VERSION).tar.gz
#******

#****d* make/dist.mk/DIST_LASTBUILD_ARCHIVE
# NAME
#   DIST_LASTBUILD_ARCHIVE
# SOURCE
DIST_LASTBUILD_ARCHIVE	:= $(DIST_DIR)/$(PROG)-lastbuild.tar.gz
#******

#****d* make/dist.mk/DIST_LATEST_ARCHIVE
# NAME
#   DIST_LATEST_ARCHIVE
# SOURCE
DIST_LATEST_ARCHIVE		:= $(DIST_DIR)/$(PROG)-latest.tar.gz
#******

#****d* make/dist.mk/DIST_STRIP_FLAGS
# NAME
#   DIST_STRIP_FLAGS
# SOURCE
DIST_STRIP_FLAGS 		:= --strip-all -N __gentoo_check_ldflags__ -R .comment \
						   -R .GCC.command.line -R .note.gnu.gold-version
#******

.PHONY:  dist dist-clean dist-staging dist-staging-clean

#****f* make/dist.mk/dist
# NAME
#   dist
# SOURCE
dist: dist-staging
	@printf "$(BLUE)$(BOLD)DIST$(RESET)     $(BLUE)Packaging release...$(RESET)\n"

	$(Q)tar -czvf $(DIST_ARCHIVE) -C $(DIST_STAGING_DIR) . > /dev/null

	$(Q)ln -fs $(DIST_ARCHIVE) $(DIST_LASTBUILD_ARCHIVE)
	$(Q)ln -fs $(shell ls $(DIST_DIR)/$(PROG)-*.tar.gz | head -n 1) $(DIST_LATEST_ARCHIVE)

	@printf "$(GREEN)$(BOLD)SUCCESS$(RESET)  Distribution built: $(BOLD)$(call relpath,$(DIST_ARCHIVE))$(RESET)\n"
#******

#****f* make/dist.mk/dist-staging
# NAME
#   dist-staging
# SOURCE
dist-staging: release
	$(Q)mkdir -p $(DIST_STAGING_DIR)/lib
	$(Q)mkdir -p $(DIST_STAGING_DIR)/bin

	$(Q)cp $(BIN_DIR)/release/$(PROG) $(DIST_STAGING_DIR)/bin/
	$(Q)cp $(BIN_DIR)/release/$(LIB) $(DIST_STAGING_DIR)/lib/

	$(Q)$(STRIP) $(DIST_STRIP_FLAGS) $(DIST_STAGING_DIR)/bin/$(PROG)
	$(Q)$(OBJCOPY) --remove-section=.comment --remove-section=.note.gnu.build-id $(DIST_STAGING_DIR)/bin/$(PROG)

	@if command -v checksec >/dev/null; then \
		checksec --file=$(DIST_STAGING_DIR)/bin/$(PROG); \
	fi

	@printf "$(GREEN)$(BOLD)SUCCESS$(RESET)  Distribution staged: $(BOLD)$(call relpath,$(DIST_STAGING_DIR))$(RESET)\n"
#******

#****f* make/dist.mk/dist-clean
# NAME
#   dist-clean
# SOURCE
dist-clean:
	@printf "$(RED)$(BOLD)CLEAN$(RESET)    $(RED)$(call relpath,$(DIST_DIR))$(RESET)\n"
	$(Q)rm -rf $(DIST_DIR)
#******

#****f* make/dist.mkl/dist-staging-clean
# NAME
#   dist-staging-clean
# SOURCE
dist-staging-clean:
	@printf "$(RED)$(BOLD)CLEAN$(RESET)    $(RED)$(call relpath,$(DIST_STAGING_DIR))$(RESET)\n"
	$(Q)rm -rf $(DIST_STAGING_DIR)
#******
