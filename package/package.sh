#!/bin/bash
set -e

#FLASHCART
mkdir -p flashcart
cp -r Autoboot flashcart
cp -r _nds flashcart
cp -r _pico flashcart
cp boot.nds flashcart/boot.nds
cp boot.nds flashcart/_nds/akmenunext/launcher.nds
cd flashcart
zip -r ../akmenu-next-flashcart.zip *
cd ..

#PICO
mkdir -p pico
cp -r _pico pico
cp -r _nds pico
cp boot.dsi pico/boot.nds
cp boot.dsi pico/_picoboot.nds
cp boot.dsi pico/_nds/akmenunext/launcher.nds
cd pico
zip -r ../akmenu-next-pico.zip *
cd ..

#DSI
mkdir -p dsi
cp -r _nds dsi
cp -r title dsi
cp boot.dsi dsi/boot.nds
cp boot.dsi dsi/akmenu-next.dsi
cp boot.dsi dsi/_nds/akmenunext/launcher.nds
rm -f dsi/_nds/akmenunext/PassMeLoader.nds
cd dsi
zip -r ../akmenu-next-dsi.zip *
cd ..

#3DS
mkdir -p 3ds
cp -r _nds 3ds
cp boot.dsi 3ds/boot.nds
cp akmenu-next.cia 3ds/akmenu-next.cia
cp boot.dsi 3ds/_nds/akmenunext/launcher.nds
rm -f 3ds/_nds/akmenunext/PassMeLoader.nds
cd 3ds
zip -r ../akmenu-next-3ds.zip *
cd ..

#Cleanup
rm -rf flashcart dsi 3ds pico
