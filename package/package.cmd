@echo off

mkdir nds-bootstrap
powershell -Command "Invoke-WebRequest https://github.com/DS-Homebrew/nds-bootstrap/releases/latest/download/nds-bootstrap.7z -OutFile nds-bootstrap.7z"
"C:\Program Files\7-Zip\7z.exe" e nds-bootstrap.7z -onds-bootstrap

:FLASHCART
mkdir flashcart
cp -r Autoboot flashcart
cp -r _nds flashcart
cp boot_nds.nds flashcart\boot.nds
cp nds-bootstrap\nds-bootstrap-release.nds flashcart\_nds\nds-bootstrap-release.nds
mv flashcart\_nds\akmenunext\launcher_nds.nds flashcart\_nds\akmenunext\launcher.nds
rm flashcart\_nds\akmenunext\launcher_dsi.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-flashcart.zip ./flashcart/*

:DSI
mkdir dsi
cp -r _nds dsi
cp boot_dsi.nds dsi\boot.nds
cp boot_dsi.nds dsi\akmenu-next.dsi
cp nds-bootstrap\nds-bootstrap-release.nds dsi\_nds\nds-bootstrap-release.nds
mv dsi\_nds\akmenunext\launcher_dsi.nds dsi\_nds\akmenunext\launcher.nds
rm dsi\_nds\akmenunext\launcher_nds.nds
rm dsi\_nds\akmenunext\PassMeLoader.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-dsi.zip ./dsi/*

:3DS
mkdir 3ds
cp -r _nds 3ds
cp boot_dsi.nds 3ds\boot.nds
cp akmenu-next.cia 3ds\akmenu-next.cia
cp nds-bootstrap\nds-bootstrap-release.nds 3ds\_nds\nds-bootstrap-release.nds
mv 3ds\_nds\akmenunext\launcher_dsi.nds 3ds\_nds\akmenunext\launcher.nds
rm 3ds\_nds\akmenunext\launcher_nds.nds
rm 3ds\_nds\akmenunext\PassMeLoader.nds
"C:\Program Files\7-Zip\7z.exe" -tzip a -r akmenu-next-3ds.zip ./3ds/*

:CLEANUP
rm nds-bootstrap.7z
rmdir nds-bootstrap /s /q
rmdir flashcart /s /q
rmdir dsi /s /q
rmdir 3ds /s /q