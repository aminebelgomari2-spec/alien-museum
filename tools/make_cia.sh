#!/usr/bin/env bash
set -euo pipefail

ELF="${1:-alien_museum.elf}"
OUT="${2:-alien_museum.cia}"
MAKEROM="${MAKEROM:-makerom}"
BANNERTOOL="${BANNERTOOL:-bannertool}"
WORK="build/cia"

mkdir -p "$WORK"

# Utilise l'icône par défaut de libctru si le fichier perso n'existe pas ou pose problème
ICON="assets/icon.png"
if [ ! -f "$ICON" ]; then
    ICON="/opt/devkitpro/libctru/default_icon.png"
fi

echo "Generation de la banniere..."
if [ -f "assets/banner.png" ] && [ -f "assets/audio.wav" ]; then
    "$BANNERTOOL" makebanner -i assets/banner.png -a assets/audio.wav -o "$WORK/banner.bnr"
else
    # Si la bannière ou l'audio perso manque, on génère une bannière propre à partir de l'icône
    "$BANNERTOOL" makebanner -i "$ICON" -o "$WORK/banner.bnr"
fi

echo "Generation du SMDH..."
"$BANNERTOOL" makesmdh \
  -s "Musee Alien" \
  -l "FPS d'horreur dans un musee alien" \
  -p "Homebrew Dev" \
  -i "$ICON" \
  -o "$WORK/icon.icn"

echo "Generation du fichier CIA..."
"$MAKEROM" -f cia -o "$OUT" \
  -elf "$ELF" \
  -rsf app.rsf \
  -target t \
  -exefslogo \
  -icon "$WORK/icon.icn" \
  -banner "$WORK/banner.bnr"

ls -l "$OUT"
