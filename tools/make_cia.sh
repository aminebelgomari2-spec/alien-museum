#!/bin/bash
# Usage : bash tools/make_cia.sh museealien
set -e
NAME="${1:-museealien}"
export PATH="/opt/devkitpro/tools/bin:$PATH"
mkdir -p build

bannertool makebanner -i assets/banner.png -a assets/audio.wav -o build/banner.bnr
bannertool makesmdh -s "Musee Alien" -l "FPS horreur dans un musee hante" -p "Moi" -i assets/icon.png -o build/icon.icn

makerom -f cia -o "${NAME}.cia" -elf "${NAME}.elf" -rsf app.rsf -target t -exefslogo -icon build/icon.icn -banner build/banner.bnr

echo "CIA cree : ${NAME}.cia"
