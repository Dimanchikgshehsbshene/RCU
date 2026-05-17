ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	src
SOURCES		:=	src
DATA		:=	data
INCLUDES	:=	include

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH		:=	-march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS		:=	-g -Wall -O2 -ffunction-sections $(ARCH) $(DEFINES) $(INCLUDE)

CFLAGS		+=	$(INCLUDE) -D__SWITCH__ -D__RTLD_SHARED__ -D__NX__

CXXFLAGS	:=	$(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++20

ASFLAGS		:=	-g $(ARCH)
LDFLAGS		= -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS		:= -lnx

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(LIBNX)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

#---------------------------------------------------------------------------------
export OUTPUT	:= $(CURDIR)/$(TARGET)
export TOPDIR	:= $(CURDIR)

export VPATH	:= $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:= $(CURDIR)/$(BUILD)

CFILES		:= $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES	:= $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
SFILES		:= $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.s))
BINFILES	:= $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))

#---------------------------------------------------------------------------------
# use CXX linking if there are C++ files, otherwise use CC
#---------------------------------------------------------------------------------
ifneq ($(strip $(CPPFILES)),)
	export LD := $(CXX)
else
	export LD := $(CC)
endif

export OFILES_BIN	:= $(addsuffix .o,$(BINFILES))
export OFILES_SRC	:= $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES		:= $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:= $(addsuffix .h,$(BINFILES))

export INCLUDE	:= $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:= $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -fr $(BUILD) $(TARGET).nso $(TARGET).nsp $(TARGET).elf

all: $(BUILD)

else

DEPENDS	:= $(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(TARGET).nsp: $(TARGET).nso

$(TARGET).nso: $(TARGET).elf

$(TARGET).elf: $(OFILES)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension used as binary data
#---------------------------------------------------------------------------------
%.bin.o %_bin.h: %.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
