#-------------------------------------------------------------------------------
# OpenPak Wii U Patcher: a .wuhb for Aroma. Run after the module and plugin are built (the
# top-level Makefile does that): it embeds ../Inkay-openpak.wms and ../plugin/Inkay-openpak.wps.
#-------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment.")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/wut/share/wut_rules

TARGET		:=	OpenPak-Wii-U-Patcher
BUILD		:=	build
SOURCES		:=	source
DATA		:=	data
INCLUDES	:=	source ../common

APP_NAME	:=	OpenPak Wii U Patcher
APP_SHORTNAME	:=	OpenPak
APP_AUTHOR	:=	OpenPak
APP_ICON	:=	$(TOPDIR)/meta/icon.png

CFLAGS	:=	-Wall -O2 -ffunction-sections $(MACHDEP) $(INCLUDE) -D__WIIU__ -D__WUT__ \
			$(shell powerpc-eabi-pkg-config --cflags SDL2_ttf SDL2_gfx)
CXXFLAGS	:=	$(CFLAGS) -std=c++20
ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-g $(ARCH) $(RPXSPECS) -Wl,-Map,$(notdir $*.map)

LIBS	:=	-lSDL2_gfx $(shell powerpc-eabi-pkg-config --static --libs SDL2_ttf) -lSDL2 -lwut
LIBDIRS	:=	$(PORTLIBS) $(WUT_ROOT)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(CURDIR)/../common $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)
# the module and plugin the app carries, copied before the data files are listed
_ := $(shell cp -f ../Inkay-openpak.wms data/module.wms && cp -f ../plugin/Inkay-openpak.wps data/plugin.wps)
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp))) redirect_list.cpp ed25519.cpp
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
export LD	:=	$(CXX)
export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o)
export OFILES	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@mkdir -p $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -fr $(BUILD) $(TARGET).wuhb $(TARGET).rpx $(TARGET).elf data/module.wms data/plugin.wps
else
DEPENDS	:=	$(OFILES:.o=.d)
all	:	$(OUTPUT).wuhb
$(OUTPUT).wuhb	:	$(OUTPUT).rpx
$(OUTPUT).rpx	:	$(OUTPUT).elf
$(OUTPUT).elf	:	$(OFILES)
$(OFILES_SRC)	:	$(HFILES_BIN)

%.ttf.o %_ttf.h :	%.ttf
	@echo $(notdir $<)
	@$(bin2o)
%.wms.o %_wms.h :	%.wms
	@echo $(notdir $<)
	@$(bin2o)
%.wps.o %_wps.h :	%.wps
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)
endif
