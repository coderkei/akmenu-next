@echo off

:FLASHCART
mkdir flashcart
cp -r _pico flashcart
cp -r Autoboot flashcart
cp -r _nds flashcart
cp boot.nds flashcart\boot.nds
cp boot.nds flashcart\_nds\akmenunext\launcher.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-flashcart.zip ./flashcart/*

:PICO
mkdir pico
cp -r _pico pico
cp -r _nds pico
cp boot.dsi pico\boot.nds
cp boot.dsi pico\_picoboot.nds
cp boot.dsi pico\_nds\akmenunext\launcher.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-pico.zip ./pico/*

:DSI
mkdir dsi
cp -r _nds dsi
cp -r title dsi
cp boot.dsi dsi\boot.nds
cp boot.dsi dsi\akmenu-next.dsi
cp boot.dsi dsi\_nds\akmenunext\launcher.nds
rm dsi\_nds\akmenunext\PassMeLoader.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-dsi.zip ./dsi/*

:3DS
mkdir 3ds
cp -r _nds 3ds
cp boot.dsi 3ds\boot.nds
cp akmenu-next.cia 3ds\akmenu-next.cia
cp boot.dsi 3ds\_nds\akmenunext\launcher.nds
rm 3ds\_nds\akmenunext\PassMeLoader.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-3ds.zip ./3ds/*

:CLEANUP
rmdir flashcart /s /q
rmdir dsi /s /q
rmdir 3ds /s /q
rmdir pico /s /q