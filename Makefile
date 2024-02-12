# XBE
XBE_TITLE = IVistation-Updater
GEN_XISO = $(XBE_TITLE).iso
OUTPUT_DIR = build

# NXDK
NXDK_DIR ?= $(CURDIR)/../nxdk
NXDK_CXX = y
NXDK_SDL = y

# Sources
SRCS = $(CURDIR)/main.c \
	$(CURDIR)/src/microtar.c \
	$(CURDIR)/src/worker.c


CXXFLAGS += -I$(CURDIR)/include
CFLAGS += -I$(CURDIR)/include

all_local: cp_rom all

include $(NXDK_DIR)/Makefile

cp_rom:
	@mkdir -p $(OUTPUT_DIR)
	cp update.tar $(OUTPUT_DIR)/
