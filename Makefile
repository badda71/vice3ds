ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_tools

# TARGET #

TARGET := 3DS

# COMMON CONFIGURATION #

NAME := vice3ds

BUILD_DIR := build
OUTPUT_DIR := output
SOURCE_DIRS := source
ROMFS_DIR := romfs
VICELIBS := VICE3DS_SDL
VICELIBS_F := $(addprefix lib,$(addsuffix .a,$(VICELIBS)))
VICEINC := $(addsuffix /include,$(VICELIBS))
INCLUDE_DIRS := $(VICEINC) $(shell find $(SOURCE_DIRS) -type d) /opt/devkitpro/portlibs/3ds/include
LIBRARY_DIRS := $(VICELIBS) /opt/devkitpro/portlibs/3ds

#DEBUG_DEFINES := -DSDL_DEBUG=1 -DDEBUG=1 -DARCHDEP_EXTRA_LOG_CALL=1
BUILD_FLAGS := -DARM11 -D_3DS -D_GNU_SOURCE=1 -O2 -Werror=implicit-function-declaration -Wno-trigraphs -Wfatal-errors -Wmissing-prototypes -Wshadow -fdata-sections -ffunction-sections -march=armv6k -mfloat-abi=hard -mtp=soft -mtune=mpcore -mword-relocations $(DEBUG_DEFINES)
LIBRARIES := $(VICELIBS) SDL_image png z citro3d

VERSION_MAJOR := 1
VERSION_MINOR := 2
VERSION_MICRO := 0
ifeq ($(VERSION_MICRO),0)
	VERSION := $(VERSION_MAJOR).$(VERSION_MINOR)
else
	VERSION := $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_MICRO)
endif

# 3DS/Wii U/Switch CONFIGURATION #

AUTHOR := badda71 <me@badda.de>

# 3DS/Wii U CONFIGURATION #

DESCRIPTION := Vice C64 emulator for Nintendo 3DS

# 3DS CONFIGURATION #

LIBRARY_DIRS += $(DEVKITPRO)/libctru
LIBRARIES += ctru

PRODUCT_CODE := CTR-P-VICE
UNIQUE_ID := 0xFF4BA

BANNER_AUDIO := meta/audio_3ds.wav
BANNER_IMAGE := meta/banner_3ds.cgfx
LOGO := meta/logo-padded.lz11
ICON := meta/icon_3ds.png
Category := Application
CPU_MODE := 804MHz
ENABLE_L2_CACHE := true

CMD := $(shell gm convert -font "Arial-Narrow" -fill white -draw "font-size 8;text 1,238 'badda71';" -font "Arial" -draw "font-size 9;text 1,229 '$(NAME) $(VERSION)';" meta/vice.png romfs/vice.png)

# TARGET #
vice3ds: $(VICELIBS_F) all

lib%.a: %/lib/lib%.a

%.a:
	rm -rf output
	$(MAKE) -C $(subst lib,,$(notdir $*))

# INTERNAL #

include buildtools/make_base
