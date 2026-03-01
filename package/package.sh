#!/bin/bash
set -e

#FLASHCART
mkdir -p flashcart
cp -r Autoboot flashcart
cp -r _nds flashcart
cp -r _pico flashcart
cp boot.nds flashcart/boot.nds
cd flashcart
zip -r ../akmenu-next-flashcart.zip *
cd ..

#PICO
mkdir -p pico
cp -r _pico pico
cp -r _nds pico
cp boot.nds pico/boot.nds
cp boot.nds pico/_picoboot.nds
cd pico
zip -r ../akmenu-next-pico.zip *
cd ..

#DSI
mkdir -p dsi
cp -r _nds dsi
cp -r title dsi
cp boot.nds dsi/boot.nds
cp boot.nds dsi/akmenu-next.dsi
rm -f dsi/_nds/akmenunext/PassMeLoader.nds
cd dsi
zip -r ../akmenu-next-dsi.zip *
cd ..

#3DS
mkdir -p 3ds
cp -r _nds 3ds
cp boot.nds 3ds/boot.nds
cp akmenu-next.cia 3ds/akmenu-next.cia
rm -f 3ds/_nds/akmenunext/PassMeLoader.nds
cd 3ds
zip -r ../akmenu-next-3ds.zip *
cd ..

#Cleanup
rm -rf flashcart dsi 3ds pico
