#!/bin/bash
set -e

# Create and download nds-bootstrap
mkdir -p nds-bootstrap
wget -O nds-bootstrap.7z "https://github.com/DS-Homebrew/nds-bootstrap/releases/latest/download/nds-bootstrap.7z"
7z e nds-bootstrap.7z -onds-bootstrap

#FLASHCART
mkdir -p flashcart
cp -r Autoboot flashcart
cp -r _nds flashcart
cp boot_nds.nds flashcart/boot.nds
cp nds-bootstrap/nds-bootstrap-release.nds flashcart/_nds/nds-bootstrap-release.nds
mv flashcart/_nds/akmenunext/launcher_nds.nds flashcart/_nds/akmenunext/launcher.nds
rm -f flashcart/_nds/akmenunext/launcher_dsi.nds
cd flashcart
zip -r ../akmenu-next-flashcart.zip *
cd ..

#DSI
mkdir -p dsi
cp -r _nds dsi
cp boot_dsi.nds dsi/boot.nds
cp boot_dsi.nds dsi/akmenu-next.dsi
cp nds-bootstrap/nds-bootstrap-release.nds dsi/_nds/nds-bootstrap-release.nds
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
cp nds-bootstrap/nds-bootstrap-release.nds 3ds/_nds/nds-bootstrap-release.nds
mv 3ds/_nds/akmenunext/launcher_dsi.nds 3ds/_nds/akmenunext/launcher.nds
rm -f 3ds/_nds/akmenunext/launcher_nds.nds
rm -f 3ds/_nds/akmenunext/PassMeLoader.nds
cd 3ds
zip -r ../akmenu-next-3ds.zip *
cd ..

#Cleanup
rm -f nds-bootstrap.7z
rm -rf nds-bootstrap flashcart dsi 3ds
