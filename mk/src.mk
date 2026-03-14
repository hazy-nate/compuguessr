
include		mk/formatting.mk
-include	mk/logo.mk

BIN_OUT_DIR		= $(BIN_DIR)/$(MODE)
BUILD_OUT_DIR	= $(BUILD_DIR)/$(MODE)

CC_SRCS		:= $(shell find $(SRC_DIR) -name "*.c")
CH_SRCS		:= $(foreach dir, $(INC_DIRS), $(shell find $(dir) -name "*.h"))
AS_SRCS		:= $(shell find $(SRC_DIR) -name "*.s") \
			   $(shell find $(SRC_DIR) -name "*.S")
ASM_SRCS	:= $(shell find $(SRC_DIR) -name "*.asm")
INC_SRCS	:= $(foreach dir, $(INC_DIRS), $(shell find $(dir) -name "*.inc"))
SRCS		:= $(CC_SRCS) $(AS_SRCS) $(ASM_SRCS)

TMP_INCS 	:= $(patsubst $(BASE_DIR)/%.inc, $(BUILD_OUT_DIR)/%.tmp.inc, $(INC_SRCS))
OBJS 		:= $(patsubst %, $(BUILD_OUT_DIR)/%.o, $(subst $(BASE_DIR)/,,$(SRCS)))

CPP_IFLAGS	= $(addprefix -I, $(INC_DIRS))
AS_IFLAGS	= $(addprefix -i, $(addsuffix /, $(INC_DIRS))) -i$(BUILD_OUT_DIR)/include/
LD_LFLAGS	= $(addprefix -L, $(LIB_DIRS))

CC_FLAGS	+=
CPP_FLAGS 	+= $(CPP_IFLAGS) -MMD -MP
ASM_FLAGS	+= $(AS_IFLAGS)
LD_FLAGS	+= $(LD_LFLAGS)

relpath 	= $(subst $(BASE_DIR)/,,$(1))

SED_INCLUDE_CMD	= sed -E 's|%include "([^"]+)\.inc"|%include "\1.tmp.inc"|g'

.PHONY: all clean debug mostlyclean release

all: $(BIN_OUT_DIR)/$(PROG)

clean: mostlyclean
	@printf "$(RED)$(BOLD)CLEAN$(RESET)    $(RED)$(call relpath,$(BIN_DIR)/*)$(RESET)\n"
	$(Q)rm -rf $(BIN_DIR)/*
	$(Q)rm -f tags TAGS

debug:
	@printf "$(YELLOW)$(BOLD)MAKE$(RESET)     Mode: $(BOLD)$@$(RESET), Verbosity: $(BOLD)$(V)$(RESET)\n"
	@$(MAKE) all MODE=debug V=$(V) --no-print-directory

mostlyclean:
	@printf "$(RED)$(BOLD)CLEAN$(RESET)    $(RED)$(call relpath,$(BUILD_DIR)/*)$(RESET)\n"
	$(Q)rm -rf $(BUILD_DIR)/*

release:
	@printf "$(YELLOW)$(BOLD)MAKE$(RESET)     Mode: $(BOLD)$@$(RESET), Verbosity: $(BOLD)$(V)$(RESET)\n"
	@$(MAKE) all MODE=release V=$(V) --no-print-directory

%: REL_SRC = $(call relpath,$<)
%: REL_OUT = $(call relpath,$@)

# ==============================================================================
# LINKING
# ==============================================================================

$(BIN_OUT_DIR)/$(PROG): $(OBJS)
	$(Q)mkdir -p $(dir $@)
	@printf "$(CYAN)$(BOLD)LD$(RESET)       $(CYAN)$(REL_OUT)$(RESET)\n"
	$(Q)$(LD) $(LD_FLAGS) -o $@ $^ $(LIBS)
	@echo "$$LOGO"
	@printf "$(GREEN)$(BOLD)SUCCESS$(RESET)  Binary generated: $(BOLD)$(REL_OUT)$(RESET) ($(YELLOW)$$(stat -c%s $@) bytes$(RESET))\n"

# ==============================================================================
# OBJECT FILES
# ==============================================================================

.PRECIOUS: $(BUILD_OUT_DIR)/%.c.o
$(BUILD_OUT_DIR)/%.c.o: $(BUILD_OUT_DIR)/%.i
	$(Q)mkdir -p $(dir $@)
	@printf "$(BLUE)$(BOLD)CC$(RESET)       $(BLUE)$(REL_OUT)$(RESET)\n"
	$(Q)$(CC) $(CC_FLAGS) -x cpp-output -c $< -o $@

.PRECIOUS: $(BUILD_OUT_DIR)/%.i
$(BUILD_OUT_DIR)/%.i: %.c
	$(Q)mkdir -p $(dir $@)
	@printf "$(BLUE)$(BOLD)CPP$(RESET)      $(BLUE)$(REL_OUT)$(RESET)\n"
	$(Q)$(CC) -E $(CPP_FLAGS) $< -o $@

.PRECIOUS: $(BUILD_OUT_DIR)/%.s.o
$(BUILD_OUT_DIR)/%.s.o: %.s
	$(Q)mkdir -p $(dir $@)
	@printf "$(MAGENTA)$(BOLD)CC$(RESET)       $(MAGENTA)$(REL_OUT)$(RESET)\n"
	$(Q)$(CC) $(AS_FLAGS) -c $< -o $@

.PRECIOUS: $(BUILD_OUT_DIR)/%.S.o
$(BUILD_OUT_DIR)/%.S.o: $(BUILD_OUT_DIR)/%.tmp.S
	$(Q)mkdir -p $(dir $@)
	@printf "$(MAGENTA)$(BOLD)CC$(RESET)       $(MAGENTA)$(REL_OUT)$(RESET)\n"
	$(Q)$(CC) $(AS_FLAGS) -c $< -o $@

# ==============================================================================
# GNU ASSEMBLER ASSEMBLY
# ==============================================================================

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.S
$(BUILD_OUT_DIR)/%.tmp.S: %.S
	$(Q)mkdir -p $(dir $@)
	@printf "$(MAGENTA)$(BOLD)CPP$(RESET)      $(MAGENTA)$(REL_OUT)$(RESET)\n"
	$(Q)$(CC) -E -D__ASSEMBLER__ $(CPP_FLAGS) -MT $@ -MF $(BUILD_OUT_DIR)/$*.S.d $< -o $@

# ==============================================================================
# NASM ASSEMBLY
# ==============================================================================

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.defines1.inc
$(BUILD_OUT_DIR)/%.tmp.defines1.inc: $(TMP_INCS)
	$(Q)mkdir -p $(dir $@)
	$(Q)grep '#include <.*>' $< | gcc -dM -E -x c - > $@ || touch $@

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.defines2.asm
$(BUILD_OUT_DIR)/%.tmp.defines2.asm: %.asm
	$(Q)mkdir -p $(dir $@)
	$(Q)sed '/#include <.*>/d' $< > $@

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.defines.asm
$(BUILD_OUT_DIR)/%.tmp.defines.asm: $(BUILD_OUT_DIR)/%.tmp.defines1.inc $(BUILD_OUT_DIR)/%.tmp.defines2.asm
	$(Q)cat $^ > $@

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.pre.asm
$(BUILD_OUT_DIR)/%.tmp.pre.asm: $(BUILD_OUT_DIR)/%.tmp.defines.asm
	$(Q)$(CC) -E -x assembler-with-cpp -w -D__ASSEMBLER__ $(CPP_FLAGS) -MT $@ -MF $(BUILD_OUT_DIR)/$*.asm.d -o $@ $<

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.asm
$(BUILD_OUT_DIR)/%.tmp.asm: $(BUILD_OUT_DIR)/%.tmp.pre.asm
	@printf "$(MAGENTA)$(BOLD)CPP-ASM$(RESET)  $(MAGENTA)$(REL_OUT)$(RESET)\n"
	$(Q)$(SED_INCLUDE_CMD) $< > $@

.PRECIOUS: $(BUILD_OUT_DIR)/%.asm.o
$(BUILD_OUT_DIR)/%.asm.o: $(BUILD_OUT_DIR)/%.tmp.asm
	@printf "$(MAGENTA)$(BOLD)ASM$(RESET)      $(MAGENTA)$(REL_OUT)$(RESET)\n"
	$(Q)$(ASM) $(ASM_FLAGS) -MD $(BUILD_OUT_DIR)/$*.S.d -MT $@ -o $@ $<

# ==============================================================================
# INCLUDES
# ==============================================================================

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.pre.inc
$(BUILD_OUT_DIR)/%.tmp.pre.inc: %.inc
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) -E -x assembler-with-cpp -D__ASSEMBLER__ $(CPP_FLAGS) $< -o $@

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.inc
$(BUILD_OUT_DIR)/%.tmp.inc: $(BUILD_OUT_DIR)/%.tmp.pre.inc
	@printf "$(MAGENTA)$(BOLD)CPP-INC$(RESET)  $(MAGENTA)$(REL_OUT)$(RESET)\n"
	$(Q)$(SED_INCLUDE_CMD) $< > $@

-include 	$(OBJS:.o=.d)
