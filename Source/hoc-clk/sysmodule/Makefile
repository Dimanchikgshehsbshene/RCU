# Makefile for hocclk sysmodule

TITLE_ID   := 00FF0000636C6BFF
TARGET     := hocclk
BUILD      := build
SOURCES    := src/auto_ryazha.cpp src/i2c/i2cDrv.cpp src/main.cpp src/i2c/i2c.c
INCLUDES   := -Isrc -Isrc/common
CXXFLAGS   := $(INCLUDES) -Wall -Wno-psabi
CFLAGS     := $(INCLUDES) -Wall -Wno-psabi

#------------------------------------------------------------------------------
# Library name
#------------------------------------------------------------------------------
LIBNAME    := libhocclk.a

#------------------------------------------------------------------------------
# Object files
#------------------------------------------------------------------------------
OBJS       := $(SOURCES:.cpp=.o) $(SOURCES:.c=.o)

#------------------------------------------------------------------------------
# Flags for libnx sysmodules
#------------------------------------------------------------------------------
CXXFLAGS   += -D__switch__ -march=armv8-a+simd -mtune=cortex-a57 -mtp=soft -fPIC -ftls-model=initial-exec
CFLAGS     += -D__switch__ -march=armv8-a+simd -mtune=cortex-a57 -mtp=soft -fPIC -ftls-model=initial-exec

#------------------------------------------------------------------------------
# Libraries
#------------------------------------------------------------------------------
LIBS       := -lnx

#------------------------------------------------------------------------------
# Build rules
#------------------------------------------------------------------------------
.PHONY: all clean

all: $(BUILD)/$(TARGET).nsp

$(BUILD)/$(TARGET).nsp: $(BUILD)/$(LIBNAME)
	@echo "Creating NSP..."
	@mkdir -p $(BUILD)/romfs
	@cp $(BUILD)/$(LIBNAME) $(BUILD)/romfs/lib/
	@nxrofs $(BUILD)/romfs $(BUILD)/
	@nacptool -c "$(TITLE_ID)" -t "$(TARGET)" -A "RCU" -v 1 -o $(BUILD)/control.nacp
	@nelfstool -v2 -z -o $(BUILD)/$(TARGET).nso -e $(BUILD)/$(LIBNAME) -C $(BUILD)/control.nacp
	@nstool -c $(BUILD)/$(TARGET).nsp -n $(TARGET) -A "$(TARGET)" -h 0 -v 0 -p $(BUILD)/$(TARGET).nso $(BUILD)/control.nacp

$(BUILD)/$(LIBNAME): $(OBJS)
	@echo "Linking library..."
	@$(AR) rcs $@ $(OBJS)

%.o: %.cpp
	@echo "Compiling C++: $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	@echo "Compiling C: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD) $(OBJS)