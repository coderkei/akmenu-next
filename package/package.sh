#!/bin/bash
set -e

#FLASHCART
mkdir -p flashcart
cp -r Autoboot flashcart
cp -r _nds flashcart
cp -r _pico flashcart
cp boot_nds.nds flashcart/boot.nds
mv flashcart/_nds/akmenunext/launcher_nds.nds flashcart/_nds/akmenunext/launcher.nds
rm -f flashcart/_nds/akmenunext/launcher_dsi.nds
rm -f flashcart/_nds/akmenunext/launcher_pico.nds
cd flashcart
zip -r ../akmenu-next-flashcart.zip *
cd ..

#PICO
mkdir -p pico
cp -r _pico pico
cp -r _nds pico
cp boot_pico.nds pico/boot.nds
cp boot_pico.nds pico/_picoboot.nds
mv pico/_nds/akmenunext/launcher_pico.nds pico/_nds/akmenunext/launcher.nds
rm -f pico/_nds/akmenunext/launcher_dsi.nds
rm -f pico/_nds/akmenunext/launcher_nds.nds
zip -r ../akmenu-next-pico.zip *
cd ..

#DSI
mkdir -p dsi
cp -r _nds dsi
cp boot_dsi.nds dsi/boot.nds
cp boot_dsi.nds dsi/akmenu-next.dsi
mv dsi/_nds/akmenunext/launcher_dsi.nds dsi/_nds/akmenunext/launcher.nds
rm -f dsi/_nds/akmenunext/launcher_nds.nds
rm -f dsi/_nds/akmenunext/PassMeLoader.nds
cd dsi
zip -r ../akmenu-next-dsi.zip *
cd ..

#3DS
mkdir -p 3ds
cp -r _nds 3ds
cp boot_dsi.nds 3ds/boot.nds
cp akmenu-next.cia 3ds/akmenu-next.cia
mv 3ds/_nds/akmenunext/launcher_dsi.nds 3ds/_nds/akmenunext/launcher.nds
rm -f 3ds/_nds/akmenunext/launcher_nds.nds
rm -f 3ds/_nds/akmenunext/PassMeLoader.nds
cd 3ds
zip -r ../akmenu-next-3ds.zip *
cd ..

#Cleanup
rm -rf flashcart dsi 3ds
