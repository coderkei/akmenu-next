@echo off

:FLASHCART
mkdir flashcart
cp -r _pico flashcart
cp -r Autoboot flashcart
cp -r _nds flashcart
cp boot.nds flashcart\boot.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-flashcart.zip ./flashcart/*

:PICO
mkdir pico
cp -r _pico pico
cp -r _nds pico
cp boot.nds pico\boot.nds
cp boot.nds pico\_picoboot.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-pico.zip ./pico/*

:DSI
mkdir dsi
cp -r _nds dsi
cp -r title dsi
cp boot.nds dsi\boot.nds
cp boot.nds dsi\akmenu-next.dsi
rm dsi\_nds\akmenunext\PassMeLoader.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-dsi.zip ./dsi/*

:3DS
mkdir 3ds
cp -r _nds 3ds
cp boot.nds 3ds\boot.nds
cp akmenu-next.cia 3ds\akmenu-next.cia
rm 3ds\_nds\akmenunext\PassMeLoader.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-3ds.zip ./3ds/*

:CLEANUP
rmdir flashcart /s /q
rmdir dsi /s /q
rmdir 3ds /s /q
rmdir pico /s /q