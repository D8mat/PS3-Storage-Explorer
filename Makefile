ifeq ($(strip $(PSL1GHT)),)
$(error Set PSL1GHT to your installed PSL1GHT SDK directory)
endif
include $(PSL1GHT)/ppu_rules
.DEFAULT_GOAL := all
TARGET := ps3-storage-explorer
TITLE := PS3 Storage Explorer
ICON0 := $(CURDIR)/assets/ICON0.PNG
APPID := STOR00001
CONTENTID := UP0001-$(APPID)_00-0000000000000000
BUILDDIR := $(CURDIR)/build
INCLUDE := -Iinclude $(LIBPSL1GHT_INC)
CFLAGS := -O2 -Wall -Wextra -std=gnu99 -D__PPU__ -mcpu=cell $(MACHDEP) $(INCLUDE)
LDFLAGS := $(MACHDEP)
LIBPATHS := $(LIBPSL1GHT_LIB)
LIBS := -lrsx -lgcm_sys -lsysutil -lio -lrt -llv2
OFILES := build/main.o build/scan.o
.PHONY: all pkg
all: $(TARGET).self
pkg: $(TARGET).pkg
build:
	mkdir -p build
build/%.o: source/%.c include/scan.h include/paging.h include/paths.h | build
	$(CC) $(CFLAGS) -c $< -o $@
$(TARGET).elf: $(OFILES)
	$(CC) $(OFILES) $(LDFLAGS) $(LIBPATHS) $(LIBS) -o $@
