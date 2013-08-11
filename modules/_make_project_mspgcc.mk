####################################################################################################
AS:=msp430-as
CC:=msp430-gcc
CXX:=msp430-gcc
LD:=msp430-gcc

####################################################################################################
BUILD_PATH:=build_mspgcc/
MODULES_BUILD_PATH:= $(BUILD_PATH)modules/

MSP430_DEVICE_UC:= $(shell echo $(MSP430_DEVICE) | tr a-z A-Z)
MSP430_DEVICE_LC:= $(shell echo $(MSP430_DEVICE) | tr A-Z a-z)
########################################## Gather Modules ##########################################
include $(addprefix $(MODULES_PATHTO),$(addsuffix .mk,$(MODULES)))

MISSING_MODULES:= $(sort $(filter-out $(MODULES),$(REQUIRED_MODULES)))

ifneq ($(strip $(MISSING_MODULES)),)
  $(warning Module dependancies are missing! Add the following modules to your makefile: $(MISSING_MODULES))
endif

######################################### MSPGCC Compiler ##########################################
MSP430_DEVICE_GCC:= $(patsubst msp430f,msp430x,$(MSP430_DEVICE_LC))

INCLUDE_FLAGS:= $(addprefix -I,$(MODULES_PATHTO) $(CONFIG_PATHTO) $(INCLUDE_PATHS))
GCC_ASFLAGS+= $(INCLUDE_FLAGS) -mmcu=$(MSP430_DEVICE_GCC)
GCC_CFLAGS += $(INCLUDE_FLAGS) -mmcu=$(MSP430_DEVICE_GCC) -ffunction-sections -fdata-sections -fmessage-length=0
GCC_CPPFLAGS += $(INCLUDE_FLAGS) -mmcu=$(MSP430_DEVICE_GCC) -ffunction-sections -fdata-sections -fmessage-length=0
GCC_LDFLAGS+= -mmcu=$(MSP430_DEVICE_GCC) -Wl,-gc-sections

EXECUTABLE:=$(BUILD_PATH)$(PROJECT_NAME).elf

# Filter out any TI assembly files (*.asm)
PROJECT_SOURCES:= $(filter-out %.asm,$(PROJECT_SOURCES))
MODULE_SOURCES:= $(filter-out %.asm,$(MODULE_SOURCES))

OBJECTS:= $(addprefix $(BUILD_PATH),$(addsuffix .o,$(basename $(PROJECT_SOURCES))))
OBJECTS+= $(addprefix $(MODULES_BUILD_PATH),$(addsuffix .o,$(basename $(MODULE_SOURCES))))

DEPEND:= $(OBJECTS:.o=.d)

BUILD_DIRECTORIES:= $(addprefix $(BUILD_PATH),$(dir $(PROJECT_SOURCES)))
BUILD_DIRECTORIES+= $(addprefix $(MODULES_BUILD_PATH),$(dir $(MODULE_SOURCES)))
$(shell mkdir -p $(BUILD_DIRECTORIES))

# Assembler ----------------------------------------------------------------------------------------
$(BUILD_PATH)%.o: %.S
	@echo AS: $< ---\> $@
	@$(AS) $(GCC_ASFLAGS) -o $@ $<
	
$(MODULES_BUILD_PATH)%.o: $(MODULES_PATHTO)%.S
	@echo AS: $< ---\> $@
	@$(AS) $(GCC_ASFLAGS) -o $@ $<
	
# Generate Dependencies	----------------------------------------------------------------------------
$(BUILD_PATH)%.d: %.c
	@echo DEP: $< ---\> $@
	@$(CC) -MM -MT $(@:.d=.o) -MT $@ $(GCC_CFLAGS) $< >$@

$(MODULES_BUILD_PATH)%.d: $(MODULES_PATHTO)%.c
	@echo DEP: $< ---\> $@
	@$(CC) -MM -MT $(@:.d=.o) -MT $@ $(GCC_CFLAGS) $< >$@

$(BUILD_PATH)%.d: %.cpp
	@echo DEP: $< ---\> $@
	@$(CXX) -MM -MT $(@:.d=.o) -MT $@ $(GCC_CPPFLAGS) $< >$@

$(MODULES_BUILD_PATH)%.d: $(MODULES_PATHTO)%.cpp
	@echo DEP: $< ---\> $@
	@$(CXX) -MM -MT $(@:.d=.o) -MT $@ $(GCC_CPPFLAGS) $< >$@
	
# C Compiler ---------------------------------------------------------------------------------------
$(BUILD_PATH)%.o: %.c
	@echo CC: $< ---\> $@
	@$(CC) $(GCC_CFLAGS) -c -o $@ $<
	
$(MODULES_BUILD_PATH)%.o: $(MODULES_PATHTO)%.c
	@echo CC: $< ---\> $@
	@$(CC) $(GCC_CFLAGS) -c -o $@ $<

# C++ Compiler -------------------------------------------------------------------------------------
$(BUILD_PATH)%.o: %.cpp
	@echo CPP: $< ---\> $@
	@$(CXX) $(GCC_CPPFLAGS) -c -o $@ $<
	
$(MODULES_BUILD_PATH)%.o: $(MODULES_PATHTO)%.cpp
	@echo CPP: $< ---\> $@
	@$(CXX) $(GCC_CPPFLAGS) -c -o $@ $<

# Linker -------------------------------------------------------------------------------------------
.PHONY:executable
executable: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@echo LINK: \($^\) ---\> $@
	@$(LD) $(GCC_LDFLAGS) -o $@ $^
	
    # Archive all modules used
	@mkdir -p $(BUILD_PATH)modules_archive/
	@cp -r -t $(BUILD_PATH)modules_archive/ $(addprefix $(MODULES_PATHTO),$(addsuffix *,$(MODULES)))
	
    # Archive this makefile
	@cp -r -t $(BUILD_PATH)modules_archive/ $(MODULES_PATHTO)_make_project_mspgcc.mk 

#Other outputs -------------------------------------------------------------------------------------
.PHONY:titxt
titxt: $(EXECUTABLE).ti_txt

$(EXECUTABLE).ti_txt: $(EXECUTABLE).hex
	@srec_cat -O $@ -TITXT $< -I
	@unix2dos -q $@

.PHONY:hex
hex: $(EXECUTABLE).hex

$(EXECUTABLE).hex: $(EXECUTABLE)
	@msp430-objcopy -O ihex $< $@

dump: $(EXECUTABLE)
	@msp430-objdump -S $< > $<.dump.txt	

####################################################################################################

ifneq ($(MAKECMDGOALS), clean)
 -include $(DEPEND)
endif

####################################################################################################


