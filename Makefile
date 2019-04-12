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
INCLUDE_DIRS := $(shell find $(SOURCE_DIRS) -type d) /opt/devkitpro/portlibs/3ds/include
LIBRARY_DIRS := /opt/devkitpro/portlibs/3ds

#BUILD_FLAGS := -DSDL_DEBUG=1 -DDEBUG=1 -DARCHDEP_EXTRA_LOG_CALL=1
LIBRARIES := SDL SDL_image png z citro3d

VERSION_MAJOR := 0
VERSION_MINOR := 4
VERSION_MICRO := 0

# 3DS/Wii U/Switch CONFIGURATION #

AUTHOR := badda71

# 3DS/Wii U CONFIGURATION #

DESCRIPTION := Vice C64 emulator for Nintendo 3DS

# 3DS CONFIGURATION #

ifeq ($(TARGET),3DS)
    LIBRARY_DIRS += $(DEVKITPRO)/libctru
    LIBRARIES += ctru

    PRODUCT_CODE := CTR-P-VICE
    UNIQUE_ID := 0xFF4BA

    BANNER_AUDIO := meta/audio_3ds.wav
    BANNER_IMAGE := meta/banner_3ds.png
    ICON := meta/icon_3ds.png
endif

# Wii U CONFIGURATION #

ifeq ($(TARGET),WIIU)
    LONG_DESCRIPTION := Build template.
    ICON := meta/icon_wiiu.png
endif

# Switch CONFIGURATION #

ifeq ($(TARGET),SWITCH)
    LIBRARY_DIRS += $(DEVKITPRO)/libnx
    LIBRARIES += nx

    ICON := meta/icon_switch.jpg
endif

# INTERNAL #

include buildtools/make_base
