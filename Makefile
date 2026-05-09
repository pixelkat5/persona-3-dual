#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

.SECONDARY:
.SECONDEXPANSION:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

# Default flags for release optimization
OPT := -O3
LTO_FLAG := -flto=auto

ifeq ($(DEBUG), 1)
    OPT := -O0 -g
    LTO_FLAG :=
endif

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET      :=  $(shell basename $(CURDIR))
BUILD       :=  build
SOURCES     :=  source source/views source/controllers source/core source/dialogue source/models source/environments source/components source/battleActions source/battleActions/enemies source/battleActions/party source/battleActions/skills
DATA        :=
INCLUDES    :=  include source

# Add environment subdirectories directly to the GRAPHICS build pipeline
GRAPHICS    :=  assets/graphics $(wildcard assets/environments/*)
SFX       	:=  assets/sfx
NITRODATA   :=  nitrofiles

GAME_TITLE     := Persona 3 Dual
GAME_SUBTITLE1 := A Fan Recreation
GAME_SUBTITLE2 := Demake by Taha Rashid
export GAME_ICON := $(CURDIR)/../icon.bmp

#---------------------------------------------------------------------------------
# Python tool configuration
#---------------------------------------------------------------------------------
TOOLS_DIR       := $(CURDIR)/tools
ifeq ($(OS),Windows_NT)
    VENV_PYTHON := $(HOME)/.venv/Scripts/python.exe
else
    VENV_PYTHON := $(HOME)/.venv/bin/python3
endif

ASSETS_DIALOGUE := $(CURDIR)/assets/dialogue
ASSETS_MUSIC    := $(CURDIR)/assets/music
ASSETS_VIDEO    := $(CURDIR)/assets/video
ASSETS_ENVIRONMENTS := $(CURDIR)/assets/environments
ASSETS_MODELS   := $(CURDIR)/assets/models
ASSETS_MAPS     := $(CURDIR)/assets/maps

NITRO_MUSIC     := $(CURDIR)/nitrofiles/music
NITRO_VIDEO     := $(CURDIR)/nitrofiles/video

#---------------------------------------------------------------------------------
# MMUTIL OS select
#---------------------------------------------------------------------------------

ifeq ($(OS),Windows_NT)
    MMUTIL := $(DEVKITPRO)/tools/bin/mmutil.exe
else
    MMUTIL := $(DEVKITPRO)/tools/bin/mmutil
endif

export MMUTIL
#---------------------------------------------------------------------------------
# Collect source files
#---------------------------------------------------------------------------------
DLG_FILES       := $(wildcard $(ASSETS_DIALOGUE)/*.dlg)
MP3_FILES       := $(wildcard $(ASSETS_MUSIC)/*.mp3)
MP4_FILES       := $(wildcard $(ASSETS_VIDEO)/*.mp4)
ENV_OBJ_FILES   := $(wildcard $(ASSETS_ENVIRONMENTS)/*/*.obj)
JMAP_FILES      := $(wildcard $(ASSETS_MAPS)/*.jmap)

MODEL_JSON_FILES := $(wildcard $(ASSETS_MODELS)/*/*.json)

#---------------------------------------------------------------------------------
# Derive output paths & dynamically add environment output dirs to SOURCES
#---------------------------------------------------------------------------------
DIALOGUE_OUT := $(DLG_FILES:$(ASSETS_DIALOGUE)/%.dlg=$(CURDIR)/source/dialogue/%_dialogue.cpp)
MUSIC_OUT    := $(MP3_FILES:$(ASSETS_MUSIC)/%.mp3=$(NITRO_MUSIC)/%.pcm)
VIDEO_OUT    := $(MP4_FILES:$(ASSETS_VIDEO)/%.mp4=$(NITRO_VIDEO)/%.vid)
JMAP_OUT     := $(JMAP_FILES:$(ASSETS_MAPS)/%.jmap=$(CURDIR)/source/maps/%.h)

MODEL_OUT    := $(foreach file,$(MODEL_JSON_FILES),$(CURDIR)/source/models/$(notdir $(file:.json=.h)))

# Keep track of environment directories so Make knows where to find the generated .s files later
ENV_OUT_DIRS    := $(foreach file,$(ENV_OBJ_FILES),source/environments/$(notdir $(patsubst %/,%,$(dir $(file)))))
ENVIRONMENT_OUT := $(foreach file,$(ENV_OBJ_FILES),$(CURDIR)/source/environments/$(notdir $(file:.obj=.h)))

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH    :=  -march=armv5te -mtune=arm946e-s -mthumb

CFLAGS  := $(OPT) $(ARCH) $(INCLUDE) -DARM9 -Wall $(LTO_FLAG) -ffunction-sections -fdata-sections
CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS :=  -g $(ARCH)

LDFLAGS =   -specs=ds_arm9.specs $(ARCH) $(LTO_FLAG) -Wl,--gc-sections -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS    := -lmm9 -lfat -lnds9

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS :=  $(LIBNDS) $(PORTLIBS)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
export TOPDIR   :=  $(CURDIR)
export OUTPUT   :=  $(CURDIR)/$(TARGET)

export VPATH    :=  $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                    $(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
                    $(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir)) \
                    $(foreach dir,$(ENV_OUT_DIRS),$(CURDIR)/$(dir))

export DEPSDIR  :=  $(CURDIR)/$(BUILD)

ifneq ($(strip $(NITRODATA)),)
    export NITRO_FILES  :=  $(CURDIR)/$(NITRODATA)
endif

CFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES    :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
CPPFILES    +=  $(notdir $(DLG_FILES:$(ASSETS_DIALOGUE)/%.dlg=%_dialogue.cpp))
CPPFILES    :=  $(sort $(CPPFILES))
SFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES    :=  soundbank.bin
PNGFILES    :=  $(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.png)))

export SFXFILES :=  $(foreach dir,$(notdir $(wildcard $(SFX)/*.*)),$(CURDIR)/$(SFX)/$(dir))

ifeq ($(strip $(CPPFILES)),)
    export LD := $(CC)
else
    export LD := $(CXX)
endif

# Cleaned up redundant environment rules; native GRAPHICS mapping handles this flawlessly
export OFILES   :=  $(addsuffix .o,$(BINFILES)) \
                    $(PNGFILES:.png=.o) \
                    $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE  :=  $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                    $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                    $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                    -I$(CURDIR)/$(BUILD)

export LIBPATHS :=  $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean assets dialogue music video environments jmaps models help

#---------------------------------------------------------------------------------
$(BUILD):
	@$(MAKE) --no-print-directory assets
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

help:
	@echo "  make              Build everything"
	@echo "  make assets       Run all asset converters"

assets: dirs dialogue music video environments jmaps models

dirs:
	@mkdir -p $(CURDIR)/source/dialogue $(CURDIR)/source/maps $(CURDIR)/source/models $(CURDIR)/source/environments $(NITRO_MUSIC) $(NITRO_VIDEO) $(CURDIR)/nitrofiles/models $(CURDIR)/nitrofiles/environments

#---------------------------------------------------------------------------------
$(CURDIR)/source/dialogue/%_dialogue.cpp: $(ASSETS_DIALOGUE)/%.dlg $(wildcard $(ASSETS_DIALOGUE)/%.build.json)
	@echo "  DLG   $(notdir $<)"
	@mkdir -p $(dir $@)
	@cd $(CURDIR)/source/dialogue && \
		$(VENV_PYTHON) $(TOOLS_DIR)/build_asset.py $< $*
dialogue: $(DIALOGUE_OUT)

#---------------------------------------------------------------------------------
$(NITRO_MUSIC)/%.pcm: $(ASSETS_MUSIC)/%.mp3
	@echo "  PCM   $(notdir $<)"
	@mkdir -p $(dir $@)
	@ffmpeg -i $< -f s16le -ar 32000 -ac 2 $@ -y -loglevel error
music: $(MUSIC_OUT)

#---------------------------------------------------------------------------------
$(NITRO_VIDEO)/%.vid: $(ASSETS_VIDEO)/%.mp4 $(wildcard $(ASSETS_VIDEO)/%.build.json)
	@echo "  VID   $(notdir $<)"
	@mkdir -p $(dir $@)
	@$(VENV_PYTHON) $(TOOLS_DIR)/build_asset.py $< $(basename $@)
video: $(VIDEO_OUT)

#---------------------------------------------------------------------------------
$(CURDIR)/source/environments/%.h: $(ASSETS_ENVIRONMENTS)/%/$$*.obj \
		$$(wildcard $(ASSETS_ENVIRONMENTS)/%/*.png) \
		$$(wildcard $(ASSETS_ENVIRONMENTS)/%/*.mtl) \
		$$(wildcard $(ASSETS_ENVIRONMENTS)/%/$$*.build.json) \
		$$(wildcard $(ASSETS_ENVIRONMENTS)/$$*.build.json)
	@echo "  ENV   $*"
	@mkdir -p $(dir $@) $(CURDIR)/nitrofiles/environments
	@$(VENV_PYTHON) $(TOOLS_DIR)/build_asset.py $< $(CURDIR)/nitrofiles/environments
	@mv $(CURDIR)/nitrofiles/environments/$*.h $@
	@touch $@

environments: $(ENVIRONMENT_OUT)

#---------------------------------------------------------------------------------
$(CURDIR)/source/models/%.h: $(ASSETS_MODELS)/%/$$*.json \
		$$(wildcard $(ASSETS_MODELS)/%/$$*.build.json) \
		$$(wildcard $(ASSETS_MODELS)/$$*.build.json)
	@echo "  MODEL $*"
	@mkdir -p $(dir $@) $(CURDIR)/nitrofiles/models
	@$(VENV_PYTHON) $(TOOLS_DIR)/build_asset.py $< $(CURDIR)/nitrofiles/models/$*.bin
	@mv $(CURDIR)/nitrofiles/models/$*.h $@
	@touch $@

models: $(MODEL_OUT)

#---------------------------------------------------------------------------------
$(CURDIR)/source/maps/%.h: $(ASSETS_MAPS)/%.jmap
	@echo "  JMAP  $(notdir $<)"
	@mkdir -p $(dir $@)
	@$(VENV_PYTHON) $(TOOLS_DIR)/build_asset.py $< $@

jmaps: $(JMAP_OUT)

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).ds.gba
	@rm -f $(MUSIC_OUT) $(VIDEO_OUT) $(JMAP_OUT) $(MODEL_OUT) $(DIALOGUE_OUT) $(CURDIR)/source/dialogue/*_dialogue.h
	@rm -rf $(CURDIR)/source/environments/* $(CURDIR)/nitrofiles/models/* $(CURDIR)/nitrofiles/environments/*

#---------------------------------------------------------------------------------
else

DEPENDS :=  $(OFILES:.o=.d)

$(OUTPUT).nds   :   $(OUTPUT).elf
$(OUTPUT).nds   :   $(shell find $(TOPDIR)/$(NITRODATA))
$(OUTPUT).elf   :   $(OFILES)

soundbank.bin soundbank.h : $(SFXFILES)
	@mmutil $^ -d -osoundbank.bin -hsoundbank.h

%.bin.o :   %.bin
	@echo $(notdir $<)
	@$(bin2o)

%.mp3.o :   %.mp3
	@echo $(notdir $<)
	@$(bin2o)

%.s %.h : %.png %.grit
	grit $< -fts -o$*

-include $(DEPENDS)
endif
