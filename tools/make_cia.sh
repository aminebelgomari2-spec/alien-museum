#!/bin/bash
set -e
ELF=$1
CIA=$2

echo "Preparation des ressources graphiques..."

# Utilise l'icône et la bannière par défaut de libctru si les assets perso n'existent pas
ICON="/opt/devkitpro/libctru/default_icon.png"
if [ -f "assets/icon.png" ]; then
    ICON="assets/icon.png"
fi

# Génération du SMDH (icône et métadonnées)
smdhtool --create "Musee Alien" "FPS d'horreur dans un musee alien" "Homebrew Dev" "$ICON" icon.icn

# Si banner.png existe, on génère le banner, sinon on crée un banner vide/minimal
if [ -f "assets/banner.png" ] && [ -f "assets/audio.wav" ]; then
    bannertool makebanner -i assets/banner.png -a assets/audio.wav -o banner.bnr
else
    echo "Attention: Assets de banniere/audio absents, utilisation d'un banner par defaut."
    bannertool makebanner -i "$ICON" -o banner.bnr
fi

echo "Generation du fichier CIA..."
makerom -f cia -o "$CIA" -elf "$ELF" -rsf app.rsf -target t -exefslogo -icon icon.icn -banner banner.bnr

echo "CIA genere avec succes !"
