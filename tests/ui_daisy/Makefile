# Project Name
TARGET = ui_daisy

# program ładowany przez bootloader Daisy do SRAM (grafiki + fonty > 128 KB flasha)
APP_TYPE = BOOT_SRAM

# Sources
CPP_SOURCES = ui_daisy.cpp ../../ui/gfx.cpp ../../ui/ui.cpp ../../ui/demo.cpp
C_INCLUDES = -I../../ui
CPP_STANDARD = -std=gnu++17

# Library Locations
LIBDAISY_DIR ?= ../../daisy/libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
