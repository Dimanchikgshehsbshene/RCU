# Makefile for sysmodule

# --- Project settings ---
TARGET := hoc-clk
TITLE_ID := 00FF0000636C6BFF

# --- DevkitPro paths ---
export DEVKITPRO ?= /opt/devkitpro
export DEVKITA64 := $(DEVKITPRO)/devkitA64
export DEVKITARM := $(DEVKITPRO)/devkitARM
export PORTLIBS := $(DEVKITPRO)/portlibs

# --- Compiler settings ---
CC := aarch64-none-elf-gcc
CXX := aarch64-none-elf-g++
AR := aarch64-none-elf-ar

CFLAGS := -O2 -ffunction-sections -fdata-sections -march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE -g -Wall -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter -Wno-missing-field-initializers -I$(DEVKITPRO)/libnx/include -I$(PORTLIBS)/include -Iinclude -Icommon/include -D__SWITCH__ -DTITLE_ID=0x$(TITLE_ID)
CXXFLAGS := $(CFLAGS) -std=c++17 -fno-rtti -fno-exceptions
LDFLAGS := -specs=$(DEVKITPRO)/libnx/switch.specs -g -Wl,-Map,$(TARGET).map -Wl,--gc-sections -Wl,--as-needed -L$(DEVKITPRO)/libnx/lib -L$(PORTLIBS)/lib -lnx

# --- Source files ---
SRC_DIR := src
BUILD_DIR := build
SOURCES := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES)))

# --- Build rules ---
.PHONY: all clean

all: $(BUILD_DIR) $(TARGET).nsp

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

$(TARGET).nsp: $(TARGET).elf
	nx-generate-nacp --titleid $(TITLE_ID) --nspdir=$(BUILD_DIR)/nsp
	nx-create-nsp $(BUILD_DIR)/nsp/$(TARGET).nsp $(TARGET).elf

clean:
	rm -rf $(BUILD_DIR) $(TARGET).elf $(TARGET).nsp $(TARGET).map
