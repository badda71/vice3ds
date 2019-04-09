ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_tools

###############
# Variables
###############

SRCDIR := src
CC := arm-none-eabi-gcc
ODIR := obj
BDIR := build
DATADIR := data
RDIR := resources
NAME := vice3ds
VERSION := 0.3
INSTDIR := $(APPDATA)/Citra/sdmc/3ds/vice3ds

SRCS := $(shell find $(SRCDIR) -name *.c)
DATAFILES := $(shell find $(DATADIR) -type f)

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

OBJ := $(addsuffix .o,$(addprefix $(ODIR)/,$(subst /,.,$(basename $(SRCS)))))
$(shell mkdir -p $(ODIR) >/dev/null)
$(shell mkdir -p $(BDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(ODIR)/$*.Td
POSTCOMPILE = @mv -f $(ODIR)/$*.Td $(ODIR)/$*.d && touch $@

###############
# Targets
###############

all: $(BDIR)/$(NAME)_$(VERSION).7z

install: $(BDIR)/$(NAME).3dsx $(RDIR)/config
	cp -f $(BDIR)/$(NAME).3dsx $(INSTDIR)
	rm -rf $(INSTDIR)/config
	cp -rf $(RDIR)/config $(INSTDIR)

$(BDIR)/%.7z: $(BDIR)/$(NAME).3dsx $(RDIR)/config
	rm -rf "$(BDIR)/3ds"
	mkdir -p "$(BDIR)/3ds/vice3ds"
	cp -f "$(BDIR)/$(NAME).3dsx" "$(BDIR)/3ds/vice3ds"
	cp -rf "$(RDIR)/config" "$(BDIR)/3ds/vice3ds"
	cd "$(BDIR)" && 7za a $*.7z 3ds
	rm -rf "$(BDIR)/3ds"

$(BDIR)/$(NAME).3dsx: $(ODIR)/$(NAME).elf $(RDIR)/icon.png
	smdhtool --create "Vice3DS" "Vice C64 emulator for Nintendo 3DS" "badda71" $(RDIR)/icon.png $(ODIR)/$(NAME).smdh
	3dsxtool $(ODIR)/$(NAME).elf $(BDIR)/$(NAME).3dsx --smdh=$(ODIR)/$(NAME).smdh

$(ODIR)/$(NAME).elf: $(OBJ) $(DATAFILES)
	$(CC) $(CFLAGS) -o $@\
		-Wl,--format=binary	$(addprefix -Wl$(COMMA),$(DATAFILES)) -Wl,--format=default\
		-Wl,--start-group $(ODIR)/*.o $(LDFLAGS) -Wl,--end-group

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
