
DOCS_SERVE_DIR 	= "$(DOCS_DIR)/_build/html"
DOCS_SERVE_PORT = 8088

.PHONY: docs docs-clean docs-serve

docs: robodoc sphinx

docs-clean: robodoc-clean sphinx-clean

docs-serve:
	@python -m http.server -d $(DOCS_SERVE_DIR) $(DOCS_SERVE_PORT)

ROBODOC_CSS = $(DOCS_DIR)/robodoc-modern.css
ROBODOC_DIR = $(DOCS_DIR)/_build/html/robodoc
ROBODOC_RC	= $(BASE_DIR)/robodoc.rc

.PHONY: robodoc robodoc-clean robodoc-serve

robodoc-serve: robodoc
	@python -m http.server -d $(ROBODOC_DIR) 8081

robodoc: robodoc-clean
	@mkdir -p $(ROBODOC_DIR)
	@robodoc --css $(ROBODOC_CSS) --doc $(ROBODOC_DIR) --rc $(ROBODOC_RC) --src .
	@find $(ROBODOC_DIR) -type f -name "*.html" -exec sed -i 's/<pre> /<pre>/g' {} +
	@find $(ROBODOC_DIR) -type f -name "*.html" -exec sed -i '/<pre>/,/<\/pre>/ s/^ //' {} +
	@find $(ROBODOC_DIR) -type f -name "*.html" -exec sed -i 's/\\\\/<br>/g' {} +

robodoc-clean:
	@rm -rf $(ROBODOC_DIR)/*

SPHINX_BUILD_DIR	= $(DOCS_DIR)/_build
SPHINX_SOURCE_DIR	= $(DOCS_DIR)
SPHINX_GEN_DIR 		= $(SPHINX_SOURCE_DIR)/generated
SPHINX_API_RST		= $(SPHINX_SOURCE_DIR)/api_reference.rst

.PHONY: sphinx sphinx-clean sphinx-html

sphinx: sphinx-html sphinx-pdf

$(SPHINX_API_RST): $(INC_SRCS) $(ASM_SRCS)
	@mkdir -p $(SPHINX_GEN_DIR)
	@echo "API Reference" > $@
	@echo "=============" >> $@
	@echo "" >> $@
	@echo ".. toctree::" >> $@
	@echo "   :maxdepth: 2" >> $@
	@echo "" >> $@
	@for file in $^; do \
		ABS_FILE=$$(readlink -f "$$file"); \
		FILENAME=$$(basename "$$ABS_FILE"); \
		BASENAME=$${FILENAME%.*}; \
		OUT_RST="$(SPHINX_GEN_DIR)/$$BASENAME.rst"; \
		ABS_GEN_DIR=$$(readlink -f "$(SPHINX_GEN_DIR)"); \
		REL_PATH=$$(realpath --relative-to="$$ABS_GEN_DIR" "$$ABS_FILE"); \
		echo "Generating $$OUT_RST (Source: $$REL_PATH)"; \
		echo "$$BASENAME" > "$$OUT_RST"; \
		echo "========================================" >> "$$OUT_RST"; \
		echo "" >> "$$OUT_RST"; \
		TAGS=$$(grep -oP '\[\K[^\]]*(?=_DOCSTART)' "$$ABS_FILE"); \
		for tag in $$TAGS; do \
			TITLE=$${tag//_/ }; \
			echo "$$TITLE" >> "$$OUT_RST"; \
			echo "----------------------------------------" >> "$$OUT_RST"; \
			echo ".. literalinclude:: $$REL_PATH" >> "$$OUT_RST"; \
			echo "   :start-after: [$$tag""_DOCSTART]" >> "$$OUT_RST"; \
			echo "   :end-before: [$$tag""_DOCEND]" >> "$$OUT_RST"; \
			echo "   :language: nasm" >> "$$OUT_RST"; \
			echo "" >> "$$OUT_RST"; \
		done; \
		echo "   generated/$$BASENAME" >> $@; \
	done

sphinx-html: $(SPHINX_API_RST)
	@sphinx-build -M html "$(SPHINX_SOURCE_DIR)" "$(SPHINX_BUILD_DIR)"

sphinx-pdf: $(SPHINX_API_RST)
	@sphinx-build -M latexpdf "$(SPHINX_SOURCE_DIR)" "$(SPHINX_BUILD_DIR)"

sphinx-clean:
	@rm -rf $(SPHINX_BUILD_DIR)/*
	@rm -rf $(SPHINX_GEN_DIR)/*
	@rm -rf $(SPHINX_API_RST)
