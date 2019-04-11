ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_tools

###############
# Variables
###############

# commands
CC := arm-none-eabi-gcc
BANNERTOOL := bannertool
MAKEROM := makerom

# directories
SRCDIR := src
ODIR := obj
BDIR := build
RDIR := resources
INSTDIR := $(APPDATA)/Citra/sdmc/3ds/vice3ds
ROMFSDIR := romfs

# project settings
TITLE := vice3ds
DESCRIPTION := Vice C64 emulator for Nintendo 3DS
AUTHOR := badda71
VERSION_MAJOR := 0
VERSION_MINOR := 9
VERSION_MICRO := 0
PRODUCT_CODE := CTR-P-VICE
UNIQUE_ID := 0xFF4BA
SYSTEM_MODE := 64MB
SYSTEM_MODE_EXT := 124MB
CATEGORY := Application
USE_ON_SD := true
MEMORY_TYPE := Application
CPU_SPEED := 804MHz
ENABLE_L2_CACHE = true

#files
BANNER_IMAGE := $(RDIR)/banner.png
BANNER_AUDIO := $(RDIR)/banner.wav
BANNER := $(ODIR)/banner.bnr
ICON := $(RDIR)/icon.png
SMDH := $(ODIR)/$(TITLE).smdh
SRCS := $(shell find $(SRCDIR) -name *.c)
OBJ := $(addsuffix .o,$(addprefix $(ODIR)/,$(subst /,.,$(basename $(SRCS)))))

# command options
COMMON_MAKEROM_FLAGS := -rsf $(RDIR)/template.rsf -target t -exefslogo -icon $(SMDH) -banner $(BANNER) -major $(VERSION_MAJOR) -minor $(VERSION_MINOR) -micro $(VERSION_MICRO) -DAPP_TITLE="$(TITLE)" -DAPP_PRODUCT_CODE="$(PRODUCT_CODE)" -DAPP_UNIQUE_ID="$(UNIQUE_ID)" -DAPP_SYSTEM_MODE="$(SYSTEM_MODE)" -DAPP_SYSTEM_MODE_EXT="$(SYSTEM_MODE_EXT)" -DAPP_CATEGORY="$(CATEGORY)" -DAPP_USE_ON_SD="$(USE_ON_SD)" -DAPP_MEMORY_TYPE="$(MEMORY_TYPE)" -DAPP_CPU_SPEED="$(CPU_SPEED)" -DAPP_ENABLE_L2_CACHE="$(ENABLE_L2_CACHE)" -DAPP_VERSION_MAJOR="$(VERSION_MAJOR)" -logo "$(RDIR)/logo.bcma.lz"

ifneq ("$(wildcard $(ROMFSDIR))","")
	_3DSXTOOL_FLAGS += --romfs=$(ROMFSDIR)
	COMMON_MAKEROM_FLAGS += -DAPP_ROMFS="$(ROMFSDIR)"
endif

INCLUDES := $(addprefix -I,$(shell find $(SRCDIR) -type d))\
	-I/opt/devkitpro/portlibs/3ds/include\
	-I/opt/devkitpro/libctru/include\

COMMA = ,
LDFLAGS :=\
	-L/opt/devkitpro/portlibs/3ds/lib\
	-L/opt/devkitpro/portlibs/3ds\
	-L/opt/devkitpro/libctru/lib\
	-lcitro3d\
	-lctru\
	-lm\
	-lSDL\
	-lSDL_image\
	-lpng\
	-lz

# DEBUG DEFINES
DDEFINES := -DSDL_DEBUG=1 -DDEBUG=1 -DARCHDEP_EXTRA_LOG_CALL=1

CFLAGS := -DARM11 -D_3DS -D_GNU_SOURCE=1 -O2 -Werror=implicit-function-declaration -Wno-trigraphs -Wfatal-errors -Wl,-rpath -Wl,/usr/lib/vice/lib -Wmissing-prototypes -Wshadow -fdata-sections -ffunction-sections -march=armv6k -mfloat-abi=hard -mtp=soft -mtune=mpcore -mword-relocations -specs=3dsx.specs $(DDEFINES) $(INCLUDES)

# other stuff & setup
$(shell mkdir -p $(ODIR) >/dev/null)
$(shell mkdir -p $(BDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(ODIR)/$*.Td
POSTCOMPILE = @mv -f $(ODIR)/$*.Td $(ODIR)/$*.d && touch $@

###############
# Targets
###############

all: 3dsx cia 3ds

3dsx: $(BDIR)/$(TITLE).3dsx

cia: $(BDIR)/$(TITLE).cia

3ds: $(BDIR)/$(TITLE).3ds

install: $(BDIR)/$(TITLE).3dsx
	cp -f $(BDIR)/$(TITLE).3dsx $(INSTDIR)
	rm -rf $(INSTDIR)/config
	cp -rf $(ROMFSDIR)/config $(INSTDIR)

$(BDIR)/$(TITLE).3dsx: $(ODIR)/$(TITLE).elf $(ODIR)/$(TITLE).smdh
	3dsxtool $(ODIR)/$(TITLE).elf $(BDIR)/$(TITLE).3dsx $(_3DSXTOOL_FLAGS) --smdh=$(ODIR)/$(TITLE).smdh

$(ODIR)/$(TITLE).elf: $(OBJ)
	$(CC) $(CFLAGS) -o $@\
		-Wl,--start-group $(ODIR)/*.o $(LDFLAGS) -Wl,--end-group

%.bnr: $(BANNER_IMAGE) $(BANNER_AUDIO)
	$(BANNERTOOL) makebanner -i $(BANNER_IMAGE) -a $(BANNER_AUDIO) -o $@

%.smdh: $(ICON)
	$(BANNERTOOL) makesmdh -s "$(TITLE)" -l "$(DESCRIPTION)" -p "$(AUTHOR)" -i $(ICON) -o $@

%.cia: $(ODIR)/$(TITLE).elf $(ODIR)/banner.bnr $(ODIR)/$(TITLE).smdh $(RDIR)/logo.bcma.lz
	$(MAKEROM) -f cia -o $@ -elf $< -DAPP_ENCRYPTED=false $(COMMON_MAKEROM_FLAGS)

%.3ds: $(ODIR)/$(TITLE).elf $(ODIR)/banner.bnr $(ODIR)/$(TITLE).smdh $(RDIR)/logo.bcma.lz
	$(MAKEROM) -f cci -o $@ -elf $< -DAPP_ENCRYPTED=true $(COMMON_MAKEROM_FLAGS)

$(ODIR)/%.d: ;
.PRECIOUS: $(ODIR)/%.d

.SECONDEXPANSION:
$(ODIR)/%.o: $$(subst .,/,$$*).c $(ODIR)/%.d
	$(CC) $(CFLAGS) $(DEPFLAGS) -g -c -o $@ $<
	$(POSTCOMPILE)

include $(wildcard $(patsubst %,%.d,$(basename $(OBJ))))

.PHONY: clean
clean:
	rm -rf $(ODIR)
	rm -rf $(BDIR)
