#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)

include $(DEVKITPRO)/wums/share/wums_rules

WUMS_ROOT := $(DEVKITPRO)/wums
WUT_ROOT := $(DEVKITPRO)/wut
#-------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#-------------------------------------------------------------------------------
TARGET		:=	Inkay-openpak
BUILD		:=	build
SOURCES		:=	src src/patches src/utils src/ext/inih common
DATA		:=	data
INCLUDES	:=	src src/ext/inih src/lang common

#-------------------------------------------------------------------------------
# options for code generation
#-------------------------------------------------------------------------------
OPT	:=  -Os -fno-exceptions -fno-asynchronous-unwind-tables
CFLAGS	:=	-Wall -ffunction-sections -fdata-sections \
			$(MACHDEP) $(OPT)

CFLAGS	+=	$(INCLUDE) -D__WIIU__ -D__WUT__

# OpenPak server addresses (defaults in common/inkay_config.h). A server move is a new build:
#   make OPENPAK_SERVER_IP=1.2.3.4 OPENPAK_NNCS2_IP=1.2.3.5 OPENPAK_IOSU_PORT=4443
ifneq ($(strip $(OPENPAK_SERVER_IP)),)
CFLAGS	+=	-DOPENPAK_SERVER_IP='"$(OPENPAK_SERVER_IP)"'
endif
ifneq ($(strip $(OPENPAK_NNCS2_IP)),)
CFLAGS	+=	-DOPENPAK_NNCS2_IP='"$(OPENPAK_NNCS2_IP)"'
endif
ifneq ($(strip $(OPENPAK_IOSU_PORT)),)
CFLAGS	+=	-DOPENPAK_IOSU_PORT='"$(OPENPAK_IOSU_PORT)"'
endif

CXXFLAGS	:= $(CFLAGS) -std=c++20

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-g $(ARCH) $(OPT) $(RPXSPECS) -Wl,-Map,$(notdir $*.map) -Wl,-gc-sections -T$(WUMS_ROOT)/share/libkernel.ld $(WUMSSPECS)

ifeq ($(DEBUG),1)
CXXFLAGS += -DDEBUG -g
CFLAGS += -DDEBUG -g
endif

LIBS	:= -lwums -lmocha -lkernel -lwut -lfunctionpatcher -lnotifications

#-------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level
# containing include and lib
#-------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS) $(WUT_ROOT) $(WUMS_ROOT) $(WUT_ROOT)/usr

#-------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#-------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#-------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#-------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#-------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#-------------------------------------------------------------------------------
	export LD	:=	$(CC)
#-------------------------------------------------------------------------------
else
#-------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean all

#-------------------------------------------------------------------------------
all: $(BUILD)


$(BUILD):
	@$(shell [ ! -d $(BUILD) ] && mkdir -p $(BUILD))
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	@$(MAKE) --no-print-directory -C $(CURDIR)/plugin -f $(CURDIR)/plugin/Makefile
	@$(MAKE) --no-print-directory -C $(CURDIR)/app -f $(CURDIR)/app/Makefile
	mkdir -p dist/
	cp *.wms dist/
	cp plugin/*.wps dist/
	cp app/OpenPak-Wii-U-Patcher.wuhb dist/

#-------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr dist $(BUILD) $(TARGET).wms $(TARGET).elf
	@$(MAKE) --no-print-directory -C $(CURDIR)/plugin -f $(CURDIR)/plugin/Makefile clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/app -f $(CURDIR)/app/Makefile clean

#-------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#-------------------------------------------------------------------------------
# main targets
#-------------------------------------------------------------------------------
all	:	 $(OUTPUT).wms

$(OUTPUT).wms	:	$(OUTPUT).elf
$(OUTPUT).elf	:   $(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#-------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#-------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#-------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

%.pem.o	%_pem.h :	%.pem
#-------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
%.o: %.s
	@echo $(notdir $<)
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d -x assembler-with-cpp $(ASFLAGS) -c $< -o $@ $(ERROR_FILTER)

-include $(DEPENDS)

#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------
