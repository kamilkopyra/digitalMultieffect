# Project Name
TARGET = multieffect

# program ładowany przez bootloader Daisy do SRAM (grafiki + fonty GUI > 128 KB flasha)
APP_TYPE = BOOT_SRAM

# Sources — efekty dokładane stopniowo, w miarę portowania z wersji PC
CPP_SOURCES = \
src/main.cpp \
core/EffectChain.cpp \
ui/gfx.cpp \
ui/ui.cpp \
ui/demo.cpp \
effects/NoiseGate.cpp \
effects/Compressor.cpp \
effects/TubeScreamer.cpp \
effects/Rat.cpp \
effects/Fuzz.cpp \
effects/Boost.cpp \
effects/Equalizer.cpp \
effects/AutoWah.cpp \
effects/Phaser.cpp \
effects/Flanger.cpp \
effects/Vibrato.cpp \
effects/Tremolo.cpp
# Delay i Tuner na razie jako wydmuszki (StubEffect.h) — Delay.cpp / Tuner.cpp nie są kompilowane

C_INCLUDES = -Icore -Ieffects -Iui

# grafiki i fonty GUI w QSPI pod 0x90400000 zamiast w SRAM (ui/font.h, UI_ASSET)
C_DEFS = -DUI_ASSETS_QSPI
LDSCRIPT = ld/STM32H750IB_sram_assets.lds
ASSETS_ADDRESS = 0x90400000

# Effect.h używa `inline constexpr`, efekty std::clamp — potrzebny C++17
CPP_STANDARD = -std=gnu++17

# Library Locations
LIBDAISY_DIR ?= daisy/libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

# Dwa pliki zamiast jednego .bin:
#   multieffect.bin        — program (bez grafik), make program-dfu, jak zawsze
#   multieffect_assets.bin — grafiki + fonty, make program-assets, tylko gdy się zmienią
$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf | $(BUILD_DIR)
	$(BIN) -R .ui_assets $< $@

$(BUILD_DIR)/$(TARGET)_assets.bin: $(BUILD_DIR)/$(TARGET).elf | $(BUILD_DIR)
	$(CP) -O binary -j .ui_assets $< $@

all: $(BUILD_DIR)/$(TARGET)_assets.bin

program-assets:
	dfu-util -a 0 -s $(ASSETS_ADDRESS):leave -D $(BUILD_DIR)/$(TARGET)_assets.bin -d ,0483:$(USBPID)
