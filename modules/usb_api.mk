
########################################### Module Setup ###########################################
MODULE_SOURCES += usb_api.c
MODULE_SOURCES += USB_API/USB_CDC_API/UsbCdc.c
MODULE_SOURCES += USB_API/USB_HID_API/UsbHid.c
MODULE_SOURCES += USB_API/USB_HID_API/UsbHidReq.c
MODULE_SOURCES += USB_API/USB_MSC_API/UsbMscReq.c
MODULE_SOURCES += USB_API/USB_MSC_API/UsbMscScsi.c
MODULE_SOURCES += USB_API/USB_MSC_API/UsbMscStateMachine.c
MODULE_SOURCES += USB_API/USB_PHDC_API/UsbPHDC.c
MODULE_SOURCES += USB_API/USB_Common/dma.c
MODULE_SOURCES += USB_API/USB_Common/usb.c

PROJECT_SOURCES += $(CONFIG_PATHTO)USB_config/descriptors.c
PROJECT_SOURCES += $(CONFIG_PATHTO)USB_config/UsbIsr.c

# TI's code assumes that all structs are packed nicely and neatly... how silly of them.
# force packed structs... (unfortunately this will apply the change globally)
GCC_CFLAGS+= -fpack-struct

# Add linker scripts to place buffers
GCC_LDFLAGS+= -Wl,--just-symbols=$(MODULES_PATHTO)USB_API/msp430USB.cmd
USER_CMD_FILES+= $(MODULES_PATHTO)USB_API/msp430USB.cmd

REQUIRED_MODULES += event_queue sleep HAL_TI
