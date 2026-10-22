#!/usr/bin/env bash
set -euo pipefail

ELF="${1:-PhoneScreen3DS.elf}"
OUT="${2:-PhoneScreen3DS.cia}"
MAKEROM="${MAKEROM:-makerom}"
BANNERTOOL="${BANNERTOOL:-bannertool}"
WORK="build/cia"

mkdir -p "$WORK"

"$BANNERTOOL" makebanner \
  -i assets/banner.png \
  -a assets/audio.wav \
  -o "$WORK/banner.bnr"

"$BANNERTOOL" makesmdh \
  -s "PhoneScreen 3DS" \
  -l "Utilise ton telephone sur la 3DS" \
  -p "Community" \
  -i assets/icon.png \
  -o "$WORK/icon.icn"

"$MAKEROM" -f cia -o "$OUT" \
  -elf "$ELF" \
  -rsf app.rsf \
  -target t \
  -exefslogo \
  -icon "$WORK/icon.icn" \
  -banner "$WORK/banner.bnr"

ls -l "$OUT"
