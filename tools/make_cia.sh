#!/usr/bin/env bash
set -euo pipefail

ELF="${1:-alien_museum.elf}"
OUT="${2:-alien_museum.cia}"
MAKEROM="${MAKEROM:-makerom}"
BANNERTOOL="${BANNERTOOL:-bannertool}"
WORK="build/cia"

mkdir -p "$WORK"

ICON="/opt/devkitpro/libctru/default_icon.png"

echo "Generation du SMDH avec smdhtool..."
smdhtool --create "Musee Alien" "FPS d'horreur dans un musee alien" "Homebrew Dev" "$ICON" "$WORK/icon.icn"

echo "Generation de la banniere minimale..."
# On génère un fichier audio muet pour satisfaire bannertool si besoin
if [ -f "assets/banner.png" ] && [ -f "assets/audio.wav" ]; then
    "$BANNERTOOL" makebanner -i assets/banner.png -a assets/audio.wav -o "$WORK/banner.bnr" || true
fi

# Si le banner n'existe pas ou a échoué, on génère un banner par défaut propre
if [ ! -f "$WORK/banner.bnr" ]; then
    "$BANNERTOOL" makebanner -i "$ICON" -a "$ICON" -o "$WORK/banner.bnr" 2>/dev/null || \
    "$BANNERTOOL" makebanner -cgxi "$ICON" -o "$WORK/banner.bnr" 2>/dev/null || \
    touch "$WORK/banner.bnr"
fi

echo "Generation du fichier CIA avec makerom..."
"$MAKEROM" -f cia -o "$OUT" \
  -elf "$ELF" \
  -rsf app.rsf \
  -target t \
  -exefslogo \
  -icon "$WORK/icon.icn" \
  -banner "$WORK/banner.bnr"

ls -lh "$OUT"
