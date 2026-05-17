# Makefile for sysmodule

# Compiler settings
CXX = g++
CC = gcc
AS = as
LD = ld

# Target info
TARGET = sysmodule
TITLE_ID = 00FF0000636C6BFF

# Directories
SRC_DIR = src
BUILD_DIR = build
DIST_DIR = $(ROOT_DIR)/dist

# Include paths - critical for finding headers
INCLUDES = -I$(SRC_DIR) \
           -I$(SRC_DIR)/i2c \
           -I$(SRC_DIR)/board \
           -I$(ROOT_DIR)/common \
           -I$(ROOT_DIR)/lib/hocclk/include \
           -I$(DEVKITPRO)/libnx/include \
           -I$(DEVKITPRO)/libnx/include/nintendo\

# Libraries
LIBS = -lnx

# Source files
CPP_SRCS = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/i2c/*.cpp)
C_SRCS = $(wildcard $(SRC_DIR)/i2c/*.c)

# Object files
CPP_OBJS = $(CPP_SRCS:$(SRC_DIR)/%=%.o)
C_OBJS = $(C_SRCS:$(SRC_DIR)/%=%.o)
OBJS = $(CPP_OBJS) $(C_OBJS)

# Compiler flags
CXXFLAGS = -O2 -Wall -Werror -std=c++17 $(INCLUDES)
CFLAGS = -O2 -Wall -Werror $(INCLUDES)
LDFLAGS = -specs=$(DEVKITPRO)/libnx/switch.specs $(LIBS)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
