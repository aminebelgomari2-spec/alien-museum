#!/bin/bash
set -e

echo "Generation du banner et de l'icône..."
bannertool makebanner -i assets/banner.png -a assets/audio.wav -o banner.bnr
bannertool makesmdh -i assets/icon.png -s "Musee Alien" -l "FPS Alien 3DS" -p "Homebrew Dev" -o icon.icn

echo "Generation du fichier CIA..."
makerom -f cia -o alien_museum.cia -elf alien_museum.elf -rsf app.rsf -target t -exefslogo -icon icon.icn -banner banner.bnr

echo "CIA genere avec succes !"