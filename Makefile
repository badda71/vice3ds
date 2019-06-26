ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_tools

# TARGET #

TARGET := 3DS

# COMMON CONFIGURATION #

NAME := vice3DS

BUILD_DIR := build
OUTPUT_DIR := output
SOURCE_DIRS := source
ROMFS_DIR := romfs
VICELIBS := VICE3DS_SDL
VICELIBS_F := $(addprefix lib,$(addsuffix .a,$(VICELIBS)))
VICEINC := $(addsuffix /include,$(VICELIBS))
INCLUDE_DIRS := $(VICEINC) $(shell find $(SOURCE_DIRS) -type d) /opt/devkitpro/portlibs/3ds/include
LIBRARY_DIRS := $(VICELIBS) /opt/devkitpro/portlibs/3ds $(DEVKITPRO)/libctru

VERSION_MAJOR := 1
VERSION_MINOR := 3
VERSION_MICRO := 0
ifeq ($(VERSION_MICRO),0)
	VERSION := $(VERSION_MAJOR).$(VERSION_MINOR)
else
	VERSION := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MICRO)
endif

#DEBUG_DEFINES := -DSDL_DEBUG=1 -DDEBUG=1 -DARCHDEP_EXTRA_LOG_CALL=1
BUILD_FLAGS := -DVERSION3DS=\"$(VERSION)\" -O $(DEBUG_DEFINES)
LIBRARIES := $(VICELIBS) SDL_image png z citro3d ctru

# 3DS CONFIGURATION #

AUTHOR := badda71 <me@badda.de>
DESCRIPTION := Vice C64 emulator for Nintendo 3DS
PRODUCT_CODE := CTR-P-VICE
UNIQUE_ID := 0xFF4BA

BANNER_AUDIO := meta/audio_3ds.wav
BANNER_IMAGE := meta/banner_3ds.cgfx
LOGO := meta/logo-padded.lz11
ICON := meta/icon_3ds.png
Category := Application
CPU_MODE := 804MHz
ENABLE_L2_CACHE := true

ifeq (, $(shell which gm))
	MKKBDPNG := cp -f
else
	MKKBDPNG := gm convert -fill white\
		-font "Arial-Narrow" -draw "font-size 8;text 1,118 'badda71';" -font "Arial" -draw "font-size 9;text 1,109 '$(NAME) $(VERSION)';"\
		-font "Arial-Narrow" -draw "font-size 8;text 1,238 'badda71';" -font "Arial" -draw "font-size 9;text 1,229 '$(NAME) $(VERSION)';"\
		-font "Arial-Narrow" -draw "font-size 8;text 1,358 'badda71';" -font "Arial" -draw "font-size 9;text 1,349 '$(NAME) $(VERSION)';"\
		-font "Arial-Narrow" -draw "font-size 8;text 1,478 'badda71';" -font "Arial" -draw "font-size 9;text 1,469 '$(NAME) $(VERSION)';"
endif

# TARGET #
vice3ds: $(ROMFS_DIR)/keyboard.png $(VICELIBS_F) all

lib%.a: %/lib/lib%.a

%.a:
	rm -rf output
	$(MAKE) -C $(subst lib,,$(notdir $*))

$(ROMFS_DIR)/keyboard.png: meta/keyboard.png Makefile
	$(MKKBDPNG) $< $@

# INTERNAL #

include buildtools/make_base
