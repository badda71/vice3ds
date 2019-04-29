ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_tools

############
# VARIABLES
NAME := VICE3DS_SDL
SRCDIR := src
ODIR := obj
LDIR := lib
$(shell mkdir -p $(ODIR) >/dev/null)
$(shell mkdir -p $(LDIR) >/dev/null)

SRCS := $(shell find $(SRCDIR) -name *.c -or -name *.s)
OBJ := $(addsuffix .o,$(addprefix $(ODIR)/,$(subst /,.,$(basename $(SRCS)))))

INCLUDES := $(addprefix -I,$(shell find $(SRCDIR) -type d))\
	-I/opt/devkitpro/portlibs/3ds/include\
	-I/opt/devkitpro/libctru/include\
	-I./include\
	-I./build

CFLAGS := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -O2 -mword-relocations -ffunction-sections -fdata-sections -D_3DS -D_GNU_SOURCE=1 -DARM11 -Wall
DEPFLAGS = -MT $@ -MMD -MP -MF $(ODIR)/$*.Td
POSTCOMPILE = @mv -f $(ODIR)/$*.Td $(ODIR)/$*.d && touch $@

##################
# TARGETS

all: $(LDIR)/lib$(NAME).a

%.a: $(OBJ)
	$(AR) crus $@ $^

$(ODIR)/%.d: ;
.PRECIOUS: $(ODIR)/%.d

.SECONDEXPANSION:
$(ODIR)/%.o: $$(subst .,/,$$*).c $(ODIR)/%.d
	$(CC) $(CFLAGS) $(INCLUDES) $(DEPFLAGS) -c -o $@ $<
	$(POSTCOMPILE)

$(ODIR)/%.o: $$(subst .,/,$$*).s
	$(AS) -o $@ $<

include $(wildcard $(patsubst %,%.d,$(basename $(OBJ))))

.PHONY: clean
clean:
	rm -rf $(ODIR)
	rm -rf $(LDIR)
