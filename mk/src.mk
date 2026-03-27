# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 Nathaniel Williams

#****h* make/src.mk
# NAME
#   src.mk
#******

include		mk/formatting.mk
-include	mk/logo.mk

ALL_TARGET_OBJS :=

INC_SRCS    := $(foreach dir, $(INC_DIRS),$(shell find $(dir) -name "*.inc"))
TMP_INCS    := $(patsubst $(BASE_DIR)/%.inc,$(BUILD_OUT_DIR)/%.tmp.inc,$(INC_SRCS))

CPP_IFLAGS  = $(addprefix -I,$(INC_DIRS))
AS_IFLAGS   = $(addprefix -i,$(addsuffix /,$(INC_DIRS))) -i$(BUILD_OUT_DIR)/include/
LD_LFLAGS   = $(addprefix -L,$(LIB_DIRS))

CC_FLAGS    +=
CPP_FLAGS   += $(CPP_IFLAGS) -MMD -MP
ASM_FLAGS   += $(AS_IFLAGS)
LD_FLAGS    += $(LD_LFLAGS)

SED_INCLUDE_CMD = sed -E 's|%include "([^"]+)\.inc"|%include "\1.tmp.inc"|g'

relpath = $(subst $(BASE_DIR)/,,$(1))

.PHONY: all clean debug mostlyclean release

all: $(addprefix $(BIN_OUT_DIR)/,$(TARGETS))

clean: mostlyclean
	@printf "$(RED)$(BOLD)CLEAN$(RESET)    $(RED)$(call relpath,$(BIN_DIR))$(RESET)\n"
	$(Q)rm -rf $(BIN_DIR)
	$(Q)rm -f tags TAGS

debug:
	@printf "$(YELLOW)$(BOLD)MAKE$(RESET)     Mode: $(BOLD)$@$(RESET), Verbosity: $(BOLD)$(V)$(RESET)\n"
	@$(MAKE) all MODE=debug V=$(V) --no-print-directory

mostlyclean:
	@printf "$(RED)$(BOLD)CLEAN$(RESET)    $(RED)$(call relpath,$(BUILD_DIR))$(RESET)\n"
	$(Q)rm -rf $(BUILD_DIR)

release:
	@printf "$(YELLOW)$(BOLD)MAKE$(RESET)     Mode: $(BOLD)$@$(RESET), Verbosity: $(BOLD)$(V)$(RESET)\n"
	@$(MAKE) all MODE=release V=$(V) --no-print-directory

%: REL_SRC = $(call relpath,$<)
%: REL_OUT = $(call relpath,$@)

#===============================================================================
# DYNAMIC BUILD TARGETS
#===============================================================================

define BUILD_TARGET_TEMPLATE

# Locate all source files in the requested directories
$(1)_SRCS := $$(foreach dir, $$($(1)_SRC_DIRS),		\
               $$(shell find $$(dir) -name "*.c") 	\
               $$(shell find $$(dir) -name "*.s") 	\
               $$(shell find $$(dir) -name "*.S") 	\
               $$(shell find $$(dir) -name "*.asm"))

# Map sources to object files in the build directory
$(1)_OBJS := $$(patsubst $$(BASE_DIR)/%,$$(BUILD_OUT_DIR)/%.o,$$($(1)_SRCS))

# Add to global list of all objects.
ALL_TARGET_OBJS += $$($(1)_OBJS)

# Add target-specific compiler and assembler flags
$$($(1)_OBJS): CC_FLAGS  += $$($(1)_CC_FLAGS)
$$($(1)_OBJS): ASM_FLAGS += $$($(1)_ASM_FLAGS)

# Resolve inter-target dependencies
$(1)_REAL_DEPS := $$(if $$($(1)_DEPS),$$(addprefix $$(BIN_OUT_DIR)/,$$($(1)_DEPS)),)

# Generate the linker rule
$$(BIN_OUT_DIR)/$(1): $$($(1)_OBJS) $$($(1)_REAL_DEPS)
	$$(Q)mkdir -p $$(dir $$@)
	@printf "$$(CYAN)$$(BOLD)LD$$(RESET)       $$(CYAN)$$(call relpath,$$@)$$(RESET)\n"
	$$(Q)$$(LD) $$(LD_FLAGS) $$($(1)_LD_FLAGS) -o $$@ $$($(1)_OBJS) $$($(1)_LD_LIBS) $$(LD_LIBS)
	@if [ "$$($(1)_TYPE)" = "EXE" ]; then echo "$$$$LOGO"; fi
	@printf "$$(GREEN)$$(BOLD)SUCCESS$$(RESET)  $$(BOLD)$$(call relpath,$$@)$$(RESET) ($$(YELLOW)$$$$(stat -c%s $$@) bytes$$(RESET))\n"
endef

# Execute the template for all targets
$(foreach target,$(TARGETS),$(eval $(call BUILD_TARGET_TEMPLATE,$(target))))

#===============================================================================
# OBJECT FILES
#===============================================================================

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

#===============================================================================
# GNU ASSEMBLER ASSEMBLY
#===============================================================================

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.S
$(BUILD_OUT_DIR)/%.tmp.S: %.S
	$(Q)mkdir -p $(dir $@)
	@printf "$(MAGENTA)$(BOLD)CPP$(RESET)      $(MAGENTA)$(REL_OUT)$(RESET)\n"
	$(Q)$(CC) -E -D__ASSEMBLER__ $(CPP_FLAGS) -MT $@ -MF $(BUILD_OUT_DIR)/$*.S.d $< -o $@

#===============================================================================
# NASM ASSEMBLY
#===============================================================================

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.pre.asm
$(BUILD_OUT_DIR)/%.tmp.pre.asm: %.asm $(TMP_INCS)
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) -E -x assembler-with-cpp -w -D__ASSEMBLER__ $(CPP_FLAGS) -MT $@ -MF $(BUILD_OUT_DIR)/$*.asm.d -o $@ $<

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.asm
$(BUILD_OUT_DIR)/%.tmp.asm: $(BUILD_OUT_DIR)/%.tmp.pre.asm
	@printf "$(MAGENTA)$(BOLD)CPP-ASM$(RESET)  $(MAGENTA)$(REL_OUT)$(RESET)\n"
	$(Q)$(SED_INCLUDE_CMD) $< > $@

.PRECIOUS: $(BUILD_OUT_DIR)/%.asm.o
$(BUILD_OUT_DIR)/%.asm.o: $(BUILD_OUT_DIR)/%.tmp.asm
	@printf "$(MAGENTA)$(BOLD)ASM$(RESET)      $(MAGENTA)$(REL_OUT)$(RESET)\n"
	$(Q)$(ASM) $(ASM_FLAGS) -MD $(BUILD_OUT_DIR)/$*.S.d -MT $@ -o $@ $<

#===============================================================================
# NASM INCLUDES
#===============================================================================

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.pre.inc
$(BUILD_OUT_DIR)/%.tmp.pre.inc: %.inc
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CC) -E -x assembler-with-cpp -D__ASSEMBLER__ $(CPP_FLAGS) $< -o $@

.PRECIOUS: $(BUILD_OUT_DIR)/%.tmp.inc
$(BUILD_OUT_DIR)/%.tmp.inc: $(BUILD_OUT_DIR)/%.tmp.pre.inc
	@printf "$(MAGENTA)$(BOLD)CPP-INC$(RESET)  $(MAGENTA)$(REL_OUT)$(RESET)\n"
	$(Q)$(SED_INCLUDE_CMD) $< > $@

-include    $(ALL_TARGET_OBJS:.o=.d)
