@echo off

:FLASHCART
mkdir flashcart
cp -r _pico flashcart
cp -r Autoboot flashcart
cp -r _nds flashcart
cp boot_nds.nds flashcart\boot.nds
mv flashcart\_nds\akmenunext\launcher_nds.nds flashcart\_nds\akmenunext\launcher.nds
rm flashcart\_nds\akmenunext\launcher_dsi.nds
rm flashcart\_nds\akmenunext\launcher_pico.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-flashcart.zip ./flashcart/*

:PICO
mkdir pico
cp -r _pico pico
cp -r _nds pico
cp boot_pico.nds pico\boot.nds
cp boot_pico.nds pico\_picoboot.nds
mv pico\_nds\akmenunext\launcher_pico.nds pico\_nds\akmenunext\launcher.nds
rm pico\_nds\akmenunext\launcher_dsi.nds
rm pico\_nds\akmenunext\launcher_nds.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-pico.zip ./pico/*

:DSI
mkdir dsi
cp -r _nds dsi
cp -r title dsi
cp boot_dsi.nds dsi\boot.nds
cp boot_dsi.nds dsi\akmenu-next.dsi
mv dsi\_nds\akmenunext\launcher_dsi.nds dsi\_nds\akmenunext\launcher.nds
rm dsi\_nds\akmenunext\launcher_nds.nds
rm dsi\_nds\akmenunext\launcher_pico.nds
rm dsi\_nds\akmenunext\PassMeLoader.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-dsi.zip ./dsi/*

:3DS
mkdir 3ds
cp -r _nds 3ds
cp boot_dsi.nds 3ds\boot.nds
cp akmenu-next.cia 3ds\akmenu-next.cia
mv 3ds\_nds\akmenunext\launcher_dsi.nds 3ds\_nds\akmenunext\launcher.nds
rm 3ds\_nds\akmenunext\launcher_nds.nds
rm 3ds\_nds\akmenunext\launcher_pico.nds
rm 3ds\_nds\akmenunext\PassMeLoader.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-3ds.zip ./3ds/*

:CLEANUP
rmdir flashcart /s /q
rmdir dsi /s /q
rmdir 3ds /s /q
rmdir pico /s /q