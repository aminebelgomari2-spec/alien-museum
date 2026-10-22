#!/usr/bin/env bash
set -euo pipefail

ELF="${1:-alien_museum.elf}"
OUT="${2:-alien_museum.cia}"
WORK="build/cia"

mkdir -p "$WORK"

# Utilise l'icône de base fournie par libctru pour éviter toute erreur d'image
ICON="/opt/devkitpro/libctru/default_icon.png"

echo "Generation du fichier icon.icn..."
smdhtool --create "Musee Alien" "FPS d'horreur dans un musee alien" "Homebrew Dev" "$ICON" "$WORK/icon.icn"

echo "Generation du fichier CIA..."
makerom -f cia -o "$OUT" \
  -elf "$ELF" \
  -rsf app.rsf \
  -target t \
  -exefslogo \
  -icon "$WORK/icon.icn"

echo "CIA genere avec succes !"
ls -lh "$OUT"
