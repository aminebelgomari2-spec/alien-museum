# ATTENTION : ce Makefile n'utilise PAS de tabulation.
# Les commandes commencent par le signe ">" (plus facile a copier depuis un telephone).
.RECIPEPREFIX := >
.SUFFIXES:

DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
CTRULIB   ?= $(DEVKITPRO)/libctru
export PATH := $(DEVKITARM)/bin:$(DEVKITPRO)/tools/bin:$(PATH)

TARGET := museealien
TITRE  := Musee Alien
DESC   := FPS horreur dans un musee hante
AUTEUR := Moi

CC      := arm-none-eabi-gcc
ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS  := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) -I$(CTRULIB)/include -D__3DS__
LDFLAGS := -specs=3dsx.specs -g $(ARCH)
LIBS    := -L$(CTRULIB)/lib -lctru -lm

SRC := $(wildcard source/*.c)
OBJ := $(patsubst source/%.c,build/%.o,$(SRC))

all: $(TARGET).3dsx

$(TARGET).3dsx: $(TARGET).elf build/$(TARGET).smdh
> 3dsxtool $(TARGET).elf $@ --smdh=build/$(TARGET).smdh

build/$(TARGET).smdh: assets/icon.png | build
> smdhtool --create "$(TITRE)" "$(DESC)" "$(AUTEUR)" assets/icon.png $@

$(TARGET).elf: $(OBJ)
> $(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

build/%.o: source/%.c | build
> $(CC) $(CFLAGS) -c $< -o $@

build:
> mkdir -p build

cia: $(TARGET).elf
> bash tools/make_cia.sh $(TARGET)

clean:
> rm -rf build $(TARGET).elf $(TARGET).3dsx $(TARGET).cia

.PHONY: all cia clean
