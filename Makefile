ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

#---------------------------------------------------------------------------------
# These set the information text in the nds file (optional)
GAME_TITLE     := DeSkee
GAME_SUBTITLE1 := Put it on your Desk.
GAME_SUBTITLE2 := https://github.com/herijooj
#---------------------------------------------------------------------------------

include $(DEVKITARM)/ds_rules

#---------------------------------------------------------------------------------
# Project configuration
#---------------------------------------------------------------------------------
TARGET   := DeSkee
BUILD    := build
SOURCES  := source source/widgets
INCLUDES := include source source/widgets
DATA     :=
GRAPHICS := graphics
AUDIO    :=
ICON     :=
NITRO    := nitrofiles

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH := -march=armv5te -mtune=arm946e-s

CFLAGS   := -g -Wall -O2 -ffunction-sections -fdata-sections \
						$(ARCH) $(INCLUDE) -DARM9
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS  := -g $(ARCH)
LDFLAGS   = -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# Libraries
#---------------------------------------------------------------------------------
LIBS := -lnds9 -lm

ifneq ($(strip $(NITRO)),)
LIBS := -lfilesystem -lfat $(LIBS)
endif

ifneq ($(strip $(AUDIO)),)
LIBS := -lmm9 $(LIBS)
endif

LIBDIRS := $(LIBNDS) $(PORTLIBS)

#---------------------------------------------------------------------------------
# Build rules
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)

export VPATH := $(CURDIR)/$(subst /,,$(dir $(ICON))) \
								$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
								$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
								$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir))

export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PNGFILES := $(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.png)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifneq ($(strip $(NITRO)),)
	export NITRO_FILES := $(CURDIR)/$(NITRO)
endif

ifneq ($(strip $(AUDIO)),)
	export MODFILES := $(foreach file,$(notdir $(wildcard $(AUDIO)/*.*)),$(CURDIR)/$(AUDIO)/$(file))

	ifneq ($(strip $(NITRO)),)
		export SOUNDBANK := $(NITRO_FILES)/soundbank.bin
	else
		export SOUNDBANK := soundbank.bin
		BINFILES += $(SOUNDBANK)
	endif
endif

ifeq ($(strip $(CPPFILES)),)
	export LD := $(CC)
else
	export LD := $(CXX)
endif

export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(PNGFILES:.png=.o) $(OFILES_BIN) $(OFILES_SOURCES)
export HFILES := $(PNGFILES:.png=.h) $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE := $(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
									 $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
									 -I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.bmp)

	ifneq (,$(findstring $(TARGET).bmp,$(icons)))
		export GAME_ICON := $(CURDIR)/$(TARGET).bmp
	else
		ifneq (,$(findstring icon.bmp,$(icons)))
			export GAME_ICON := $(CURDIR)/icon.bmp
		endif
	endif
else
	ifeq ($(suffix $(ICON)), .grf)
		export GAME_ICON := $(CURDIR)/$(ICON)
	else
		export GAME_ICON := $(CURDIR)/$(BUILD)/$(notdir $(basename $(ICON))).grf
	endif
endif

.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(SOUNDBANK)

else

$(OUTPUT).nds: $(OUTPUT).elf $(NITRO_FILES) $(GAME_ICON)
$(OUTPUT).elf: $(OFILES)

$(OFILES_SOURCES): $(HFILES)

$(OFILES): $(SOUNDBANK)

$(SOUNDBANK): $(MODFILES)
	mmutil $^ -d -o$@ -hsoundbank.h

%.bin.o %_bin.h : %.bin
	@echo $(notdir $<)
	@$(bin2o)

%.s %.h: %.png %.grit
	grit $< -fts -o$*

$(GAME_ICON): $(notdir $(ICON))
	@echo convert $(notdir $<)
	@grit $< -g -gt -gB4 -gT FF00FF -m! -p -pe 16 -fh! -ftr

-include $(DEPSDIR)/*.d

endif
