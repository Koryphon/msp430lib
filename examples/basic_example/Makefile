
########################################## Project Setup ###########################################
PROJECT_NAME:= example

MODULES_PATHTO:= ../../modules/
CONFIG_PATHTO:= config/

INCLUDE_PATHS:= ../../include/
PROJECT_SOURCES:= main.c
MODULES:=clock_sys event_queue fifo button timer

MSP430_DEVICE:= msp430f4618

# Choose "ti" or "mspgcc"
COMPILER:= mspgcc

default: executable
####################################### For MSPGCC Compiler ########################################
export GCC_ASFLAGS:=
export GCC_CFLAGS:= -O2 -g3 -Wall -std=c99
export GCC_CPPFLAGS:= -O2 -g3 -Wall
export GCC_LDFLAGS:=
####################################### For TI CCS Compiler ########################################
export TI_ASFLAGS:=
export TI_CFLAGS:= -vmspx -g -O2
export TI_CPPFLAGS:= -vmspx -g -O2
export TI_LDFLAGS:= --stack_size=160 --heap_size=160 --use_hw_mpy=F5

export USER_CMD_FILES:=
####################################################################################################
ifeq ($(strip $(COMPILER)),ti)
  include $(MODULES_PATHTO)_make_project_ti.mk
else ifeq ($(strip $(COMPILER)),mspgcc)
  include $(MODULES_PATHTO)_make_project_mspgcc.mk
else
  $(error Invalid Compiler)
endif
########################################## Custom Targets ##########################################

program: $(EXECUTABLE).hex
	MSP430Flasher -n $(MSP430_DEVICE_UC) -w $^ -v -g -q -z [RESET, VCC]

.PHONY:clean
clean:
	rm -r -f $(BUILD_PATH)
