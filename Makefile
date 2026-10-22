TARGET		:= alien_museum
BUILD		:= build
SOURCES		:= source
INCLUDES	:= include
DATA		:= data
ROMFS		:= romfs

TITLE		:= Musee Alien
DESCRIPTION	:= Un FPS d'horreur sombre dans un musee alien.
AUTHOR		:= Homebrew Dev

UNIQUE_ID	:= 0xc7a23
PRODUCT_CODE	:= CTR-P-ALEN

# Chemins standards de devkitARM dans l'image Docker
DEVKITPRO	?= /opt/devkitpro
DEVKITARM	?= $(DEVKITPRO)/devkitARM

# Recherche automatique du fichier de règles 3DS
RULES_FILE := $(shell if [ -f $(DEVKITARM)/base_rules ]; then echo $(DEVKITARM)/base_rules; fi)
include $(RULES_FILE)

RULES_3DS := $(shell if [ -f $(DEVKITPRO)/libctru/3ds_rules ]; then echo $(DEVKITPRO)/libctru/3ds_rules; else echo $(DEVKITARM)/3ds_rules.rules; fi)
include $(RULES_3DS)

ARCH		:= -march=armv6k -mtune=mpcore -mfloat-abi=hard -mfpu=vfpv2
CFLAGS		:= -g -Wall -O2 -mword-relocations $(ARCH) -D__3DS__ $(INCLUDE)
CXXFLAGS	:= $(CFLAGS)
ASFLAGS		:= -g $(ARCH)
LDFLAGS		:= -specs=3dsx.specs $(ARCH)

LIBS		:= -lcitro2d -lcitro3d -lctru -lm

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:= $(CURDIR)/$(TARGET)
export TOPDIR	:= $(CURDIR)

.PHONY: all clean cia

all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

clean:
	@echo "Nettoyage..."
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).cia $(TARGET).elf

cia: all
	@bash tools/make_cia.sh

else

DEPENDS	:= $(OFILES:.o=.d)
OBJS	:= $(addprefix $(BUILD)/,$(notdir $(CFILES)) $(notdir $(CPPFILES)) $(notdir $(SFILES)))
OBJS	:= $(OBJS:.cpp=.o)
OBJS	:= $(OBJS:.c=.o)
OBJS	:= $(OBJS:.s=.o)

vpath %.c $(SOURCES)
vpath %.cpp $(SOURCES)
vpath %.s $(SOURCES)

$(OUTPUT).3dsx: $(OUTPUT).elf

$(OUTPUT).elf: $(OBJS)
	@echo "Liaison $^ -> $@"
	@$(LD) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

include $(DEPENDS)

endif
