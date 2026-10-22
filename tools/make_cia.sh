#!/usr/bin/env bash
set -euo pipefail

ELF="${1:-alien_museum.elf}"
OUT="${2:-alien_museum.cia}"
MAKEROM="${MAKEROM:-makerom}"
BANNERTOOL="${BANNERTOOL:-bannertool}"
WORK="build/cia"

mkdir -p "$WORK"

# Utilise l'icône système de base de devkitPro pour éviter tout problème de fichier PNG corrompu
ICON="/opt/devkitpro/libctru/default_icon.png"

echo "Generation de la banniere minimale..."
# On génère une bannière simple directement à partir de l'icône de base
"$BANNERTOOL" makebanner -i "$ICON" -o "$WORK/banner.bnr"

echo "Generation du SMD..."
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
