.SUFFIXES:

DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
export PATH := $(DEVKITPRO)/tools/bin:$(DEVKITARM)/bin:$(PATH)

CTRULIB  ?= $(DEVKITPRO)/libctru
PORTLIBS ?= $(DEVKITPRO)/portlibs/3ds

TARGET     := alien_museum
BUILD      := build
APP_TITLE  := Musee Alien
APP_DESC   := FPS d'horreur dans un musee alien
APP_AUTHOR := Homebrew Dev

ICON := $(if $(wildcard assets/icon.png),assets/icon.png,$(CTRULIB)/default_icon.png)

CC := arm-none-eabi-gcc

ARCH   := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -std=gnu11 -g -Wall -O2 -mword-relocations -ffunction-sections -fdata-sections \
          $(ARCH) -D__3DS__ -Isource -I$(CTRULIB)/include -I$(PORTLIBS)/include
LDFLAGS := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(BUILD)/$(TARGET).map -Wl,--gc-sections \
           -L$(CTRULIB)/lib -L$(PORTLIBS)/lib
LIBS := -lcitro2d -lcitro3d -lctru -lm

SRC := $(wildcard source/*.c)
HDR := $(wildcard source/*.h)
OBJ := $(patsubst source/%.c,$(BUILD)/%.o,$(SRC))

.PHONY: all cia clean

all: $(TARGET).3dsx

$(BUILD):
	@mkdir -p $@

$(BUILD)/%.o: source/%.c $(HDR) | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(TARGET).elf: $(OBJ) | $(BUILD)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

$(BUILD)/$(TARGET).smdh: $(ICON) | $(BUILD)
	smdhtool --create "$(APP_TITLE)" "$(APP_DESC)" "$(APP_AUTHOR)" $(ICON) $@

$(TARGET).3dsx: $(TARGET).elf $(BUILD)/$(TARGET).smdh
	3dsxtool $(TARGET).elf $@ --smdh=$(BUILD)/$(TARGET).smdh

cia: $(TARGET).elf
	bash tools/make_cia.sh $(TARGET).elf $(TARGET).cia

clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).3dsx $(TARGET).cia

-include $(OBJ:.o=.d)
