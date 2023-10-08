PROJECT = cdcacm
BUILD_DIR = bin

DEVICE=stm32f411ce
CFLAGS += -DSYSTEM_CLOCK=96000000

SHARED_DIR =
CFILES =  src/main.c

CFILES += src/hw/time.c \
					src/hw/board.c \
					src/hw/logging.c \
					src/hw/usb.c \
					src/hw/uart.c \
					src/cdc_bridge.c
CFILES += src/mbed-sw/src/core/timer.c \
					src/mbed-sw/src/core/fifo.c \
					src/mbed-sw/src/core/slist.c \
					src/mbed-sw/src/core/dlist.c \
					src/mbed-sw/src/core/stringstream.c \
					src/mbed-sw/src/core/alloc.c
# AFILES += 

OOCD_FILE = src/hw/openocd.cfg

VPATH += $(SHARED_DIR)
INCLUDES += -Isrc \
						-Isrc/hw \
						-Isrc/mbed-sw/src \
						-Isrc/mbed-sw/src/core \
						$(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=src/hw/libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
# use automatically generated linker script
# LDSCRIPT=$(BOARD_DIR)/linker.ld
include ./rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
