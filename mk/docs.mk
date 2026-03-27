# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

#****h* make/docs.mk
# NAME
#   docs.mk
#******

#****d* make/docs.mk/ROBODOC_RC
# NAME
#   ROBODOC_RC
# SOURCE
ROBODOC_RC			= $(BASE_DIR)/.robodocrc
#******

#***d* make/docs.mk/SPHINX_BUILD_DIR
# NAME
#   SPHINX_BUILD_DIR
# SOURCE
SPHINX_BUILD_DIR	= $(DOCS_DIR)/build
#******

#****d* make/docs.mk/SPHINX_SOURCE_DIR
# NAME
#   SPHINX_SOURCE_DIR
# SOURCE
SPHINX_SOURCE_DIR	= $(DOCS_DIR)/source
#******

#****d* make/docs.mk/SPHINX_GEN_DIR
# NAME
#   SPHINX_GEN_DIR
# SOURCE
SPHINX_GEN_DIR 		= $(SPHINX_SOURCE_DIR)/generated
#******

#****d* make/docs.mk/ROBODOC_CSS
# NAME
#   ROBODOC_CSS
# SOURCE
ROBODOC_CSS 		= $(SPHINX_SOURCE_DIR)/robodoc.css
#******

#****d* make/docs.mk/ROBODOC_DIR
# NAME
#   ROBODOC_DIR
# SOURCE
ROBODOC_DIR 		= $(SPHINX_BUILD_DIR)/html/robodoc
#******

#****d* make/docs.mk/DOCS_SERVE_DIR
# NAME
#   DOCS_SERVE_DIR
# SOURCE
DOCS_SERVE_DIR 		= "$(DOCS_DIR)/build/html"
#******

#****d* make/docs.mk/DOCS_SERVE_PORT
# NAME
#   DOCS_SERVE_PORT
# SOURCE
DOCS_SERVE_PORT 	= 8088
#******

.PHONY: docs docs-clean docs-serve \
		robodoc robodoc-clean robodoc-serve \
		sphinx sphinx-clean sphinx-html

#===============================================================================
# DOCS
#===============================================================================

#****f* make/docs.mk/docs
# NAME
#   docs
# SOURCE
docs: robodoc sphinx
#******

#****f* make/docs.mk/docs-clean
# NAME
#   docs-clean
# SOURCE
docs-clean: robodoc-clean sphinx-clean
#******

#****f* make/docs.mk/docs-serve
# NAME
#   docs-serve
# SOURCE
docs-serve:
	@printf "$(YELLOW)$(BOLD)SERVE$(RESET)    $(YELLOW)Docs on port $(DOCS_SERVE_PORT)$(RESET)\n"
	$(Q)python -m http.server -d $(DOCS_SERVE_DIR) $(DOCS_SERVE_PORT)
#******

#===============================================================================
# ROBODOC
#===============================================================================

#****f* make/docs.mk/robodoc
# NAME
#   robodoc
# SOURCE
robodoc: robodoc-clean
	@printf "$(BLUE)$(BOLD)DOCS$(RESET)     $(BLUE)Generating Robodoc...$(RESET)\n"
	$(Q)mkdir -p $(ROBODOC_DIR)
	$(Q)robodoc --css $(ROBODOC_CSS) --doc $(ROBODOC_DIR) --rc $(ROBODOC_RC) --src .
	$(Q)find $(ROBODOC_DIR) -type f -name "*.html" -exec python3 docs/clean_docs.py {} +
#******

#****f* make/docs.mk/robodoc-clean
# NAME
#   robodoc-clean
# SOURCE
robodoc-clean:
	@printf "$(RED)$(BOLD)CLEAN$(RESET)    $(RED)$(call relpath,$(ROBODOC_DIR))$(RESET)\n"
	$(Q)rm -rf $(ROBODOC_DIR)
#******

#****f* make/docs.mk/robodoc-serve
# NAME
#   robodoc-serve
# SOURCE
robodoc-serve: robodoc
	@printf "$(YELLOW)$(BOLD)SERVE$(RESET)    $(YELLOW)Robodoc on port 8081$(RESET)\n"
	$(Q)python -m http.server -d $(ROBODOC_DIR) $(DOCS_SERVE_PORT)
#******

#===============================================================================
# SPHINX
#===============================================================================

#****f* make/docs.mk/sphinx
# NAME
#   sphinx
# SOURCE
sphinx: sphinx-html sphinx-pdf
#******

#****f* make/docs.mk/sphinx-html
# NAME
#   sphinx-html
# SOURCE
sphinx-html:
	@printf "$(BLUE)$(BOLD)DOCS$(RESET)     $(BLUE)Generating Sphinx HTML...$(RESET)\n"
	$(Q)sphinx-build -M html "$(SPHINX_SOURCE_DIR)" "$(SPHINX_BUILD_DIR)"
#******

#****f* make/docs.mk/sphinx-pdf
# NAME
#   sphinx-pdf
# SOURCE
sphinx-pdf:
	@printf "$(BLUE)$(BOLD)DOCS$(RESET)     $(BLUE)Generating Sphinx PDF...$(RESET)\n"
	$(Q)sphinx-build -M latexpdf "$(SPHINX_SOURCE_DIR)" "$(SPHINX_BUILD_DIR)"
#******

#****f* make/docs.mk/sphinx-clean
# NAME
#   sphinx-clean
# SOURCE
sphinx-clean:
	@printf "$(RED)$(BOLD)CLEAN$(RESET)    $(RED)$(call relpath,$(SPHINX_BUILD_DIR))$(RESET)\n"
	$(Q)rm -rf $(SPHINX_BUILD_DIR)
	$(Q)rm -rf $(SPHINX_GEN_DIR)
#******
