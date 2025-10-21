/*
    Copyright (C) 2024 lifehackerhansol
    Additional modifications Copyright (C) 2025 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fat.h>

#include <nds/ndstypes.h>

#include "../cheatwnd.h"
#include "../dsrom.h"
#include "../flags.h"
#include "../inifile.h"
#include "../mainlist.h"
#include "../systemfilenames.h"
#include "../language.h"
#include "../ui/msgbox.h"
#include "../ui/progresswnd.h"
#include "ILauncher.h"
#include "NdsBootstrapLauncher.h"
#include "nds_loader_arm9.h"

void smoothProgress(u8 start, u8 end) {
    for(u8 p = start; p <= end; p += 5) {
        progressWnd().setPercent(p);
        swiWaitForVBlank();
    }
    progressWnd().setPercent(end);
}

bool NdsBootstrapLauncher::prepareCheats() {
    u32 gameCode, crc32;

    if (cCheatWnd::romData(mRomPath, gameCode, crc32)) {
        FILE* cheatDb = fopen(SFN_CHEATS, "rb");
        if (!cheatDb) goto cheat_failed;
        long cheatOffset;
        size_t cheatSize;
        if (cCheatWnd::searchCheatData(cheatDb, gameCode, crc32, cheatOffset, cheatSize)) {
            cCheatWnd chtwnd((256) / 2, (192) / 2, 100, 100, NULL, mRomPath);

            chtwnd.parse(mRomPath);
            chtwnd.writeCheatsToFile("/_nds/nds-bootstrap/cheatData.bin");
            FILE* cheatData = fopen("/_nds/nds-bootstrap/cheatData.bin", "rb");
            if (cheatData) {
                u32 check[2];
                fread(check, 1, 8, cheatData);
                fclose(cheatData);
                // TODO: Delete file, if above 0x8000 bytes
                if (check[1] == 0xCF000000) goto cheat_failed;
            }
        } else {
            fclose(cheatDb);
            goto cheat_failed;
        }
        fclose(cheatDb);
    }

    return true;

cheat_failed:
    // Remove cheat bin if exists
    if (access("/_nds/nds-bootstrap/cheatData.bin", F_OK) == 0) {
        remove("/_nds/nds-bootstrap/cheatData.bin");
    }

    return false;
}

bool NdsBootstrapLauncher::prepareIni(bool hb) {
    CIniFile ini;
    hotkeyCheck = false;

    ini.SetString("NDS-BOOTSTRAP", "NDS_PATH", mRomPath);

    if(hb == true)
    {
        ini.SetString("NDS-BOOTSTRAP", "DSI_MODE", 0);
        ini.SaveIniFile("/_nds/nds-bootstrap.ini");
        return true;
    }

    ini.SetString("NDS-BOOTSTRAP", "SAV_PATH", mSavePath);

    #ifdef __DSIMODE__
        const char* custIniPath = "sd:/_nds/akmenunext/ndsbs.ini";
    #else
        const char* custIniPath = "fat:/_nds/akmenunext/ndsbs.ini";
    #endif

    if(access(custIniPath, F_OK) != 0){
            akui::messageBox(NULL, LANG("nds bootstrap", "inimissingtitle"), LANG("nds bootstrap", "inimissing"), MB_OK);
            return false;
        }

    std::string externalHotkey;
    {
        CIniFile extIni;
        if (extIni.LoadIniFile(custIniPath)) {
            externalHotkey = extIni.GetString("ndsbs", "hotkey", "");
            if (!externalHotkey.empty()) {
                hotkeyCheck = true;
            } else {
                hotkeyCheck = false;
            }
        } else {
            akui::messageBox(NULL, LANG("nds bootstrap", "inimissingtitle"), LANG("nds bootstrap", "inimissing"), MB_OK);
        }
    }


    /*
    0 = l-↓-select : 200 80 4
    1 = l-r-start : 200 100 8
    2 = l-r-select : 200 100 4
    3 = l-r-a-b-↓ : 200 100 1 2 80
    4 = l-r-b-y-↓ : 200 100 2 800 80
    5 = l-r-a-b-x-y : 200 100 1 2 400 800
    */

    switch(gs().resetHotKey)
    {
        case 0:
            ini.SetString("NDS-BOOTSTRAP", "HOTKEY", "284");
            break;
        case 1:
            ini.SetString("NDS-BOOTSTRAP", "HOTKEY", "308");
            break;
        case 2:
            ini.SetString("NDS-BOOTSTRAP", "HOTKEY", "304");
            break;
        case 3:
            ini.SetString("NDS-BOOTSTRAP", "HOTKEY", "383");
            break;
        case 4:
            ini.SetString("NDS-BOOTSTRAP", "HOTKEY", "B82");
            break;
        case 5:
            ini.SetString("NDS-BOOTSTRAP", "HOTKEY", "F03");
            break;
        case 6:
            if(hotkeyCheck){
                ini.SetString("NDS-BOOTSTRAP", "HOTKEY", externalHotkey.c_str());
            }
            break;
        default:
            break;
    }
    switch(gs().languageOverride)
    {
        case 0:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "-1");
            break;
        case 1:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "0");
            break;
        case 2:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "1");
            break;
        case 3:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "2");
            break;
        case 4:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "3");
            break;
        case 5:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "4");
            break;
        case 6:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "5");
            break;
        case 7:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "6");
            break;
        case 8:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "7");
            break;
    }

    if(gs().dsOnly) {
        ini.SetString("NDS-BOOTSTRAP", "DSI_MODE", "0");
    }
    if(gs().phatCol) {
    #ifdef __DSIMODE__
        ini.SetString("NDS-BOOTSTRAP", "PHAT_COLORS", "1");
    #endif
    }

    if (access("/_nds/debug.txt", F_OK) == 0) {
        ini.SetString("NDS-BOOTSTRAP", "LOGGING", "1");
        ini.SetString("NDS-BOOTSTRAP", "DEBUG", "1");
    }

    ini.SaveIniFile("/_nds/nds-bootstrap.ini");

    return true;
}

bool launchHbStrap(std::string romPath){
    progressWnd().setPercent(100);
    const char ndsHbBootstrapPath[] = SD_ROOT_0 "/_nds/nds-bootstrap-hb-release.nds";
    std::vector<const char*> argv;
    argv.push_back(ndsHbBootstrapPath);
    progressWnd().hide();
    eRunNdsRetCode rc = runNdsFile(argv[0], argv.size(), &argv[0]);
    if (rc == RUN_NDS_OK) return true;
    return false;
}

bool NdsBootstrapLauncher::launchRom(std::string romPath, std::string savePath, u32 flags,
                                     u32 cheatOffset, u32 cheatSize, bool hb) {
    const char ndsBootstrapPath[] = SD_ROOT_0 "/_nds/nds-bootstrap-release.nds";
    const char ndsBootstrapPathNightly[] = SD_ROOT_0 "/_nds/nds-bootstrap-nightly.nds";
    const char ndsHbBootstrapPath[] = SD_ROOT_0 "/_nds/nds-bootstrap-hb-release.nds";
    const char ndsBootstrapCheck[] = SD_ROOT_0 "/_nds/pagefile.sys";
    bool useNightly = false;
    bool hbStrap = hb;

    //check if rom is homebrew
    if(hbStrap){
        mRomPath = romPath;
        if(access(ndsHbBootstrapPath, F_OK) != 0){
            progressWnd().hide();
            printLoaderNotFound(ndsHbBootstrapPath);
            return false;
        }
        progressWnd().setTipText("Initializing nds-bootstrap...");
        progressWnd().show();
        progressWnd().setPercent(0);
        //Clean up old INI
        if (access("/_nds/nds-bootstrap/nds-bootstrap.ini", F_OK) == 0) {
        remove("/_nds/nds-bootstrap/nds-bootstrap.ini");
        }
        // Setup nds-bootstrap INI parameters
        if (!prepareIni(false)) return false;
        progressWnd().setPercent(25);
        launchHbStrap(romPath);
    }

    //has the user used nds-bootstrap before?
    if(access(ndsBootstrapCheck, F_OK) != 0){
        akui::messageBox(NULL, LANG("nds bootstrap", "firsttimetitle"), LANG("nds bootstrap", "firsttime"), MB_OK);
    }

    _romInfo.MayBeDSRom(romPath);
    bool dsiWare = _romInfo.isDSiWare();

    // check for DSiWare
    if(dsiWare){
        printError("Unsupported application. DSiWare is not supported.");
        return false;
    }

    progressWnd().setTipText("Initializing nds-bootstrap...");
    progressWnd().show();
    progressWnd().setPercent(0);

    //Check which nds-bootstrap version has been selected
    if(gs().nightly){
        if(access(ndsBootstrapPathNightly, F_OK) != 0){
            progressWnd().hide();
            printLoaderNotFound(ndsBootstrapPathNightly);
            return false;
        }
        else{
            useNightly = true;
        }
    }
    else{
        if(access(ndsBootstrapPath, F_OK) != 0){
            progressWnd().hide();
            printLoaderNotFound(ndsBootstrapPath);
            return false;
        }
        else{
            useNightly = false;
        }
    }

    std::vector<const char*> argv;

    mRomPath = romPath;
    mSavePath = savePath;
    mFlags = flags;

    // Create the nds-bootstrap directory if it doesn't exist
    if (access("/_nds/nds-bootstrap/", F_OK) != 0) {
        mkdir("/_nds/nds-bootstrap/", 0777);
    }
    smoothProgress(0, 25);

    // Setup argv to launch nds-bootstrap                             
    if(!useNightly){
        argv.push_back(ndsBootstrapPath);
    }
    else{
        argv.push_back(ndsBootstrapPathNightly);
    }

    progressWnd().setTipText("Initializing nds-bootstrap...");
    smoothProgress(25, 50);

    //Clean up old INI
    if (access("/_nds/nds-bootstrap/nds-bootstrap.ini", F_OK) == 0) {
        remove("/_nds/nds-bootstrap/nds-bootstrap.ini");
    }

    // Setup nds-bootstrap INI parameters
    if (!prepareIni(false)) return false;
    smoothProgress(50, 75);

    // Prepare cheat codes if enabled
    // Remove cheat bin if exists to start clean
    if (access("/_nds/nds-bootstrap/cheatData.bin", F_OK) == 0) {
        remove("/_nds/nds-bootstrap/cheatData.bin");
    }
    if (!_romInfo.saveInfo().getCheat() || !gs().cheats) {
        progressWnd().setTipText("Booting game...");
        smoothProgress(75, 100);
    }
    else {
        progressWnd().setTipText("Loading usrcheat.dat...");
        smoothProgress(75, 90);
        if (flags & PATCH_CHEATS) {
            if (!prepareCheats()) {
                return false;
            }
        }
        progressWnd().setTipText("Booting game...");
        smoothProgress(90, 100);
    }

    // Launch
    eRunNdsRetCode rc = runNdsFile(argv[0], argv.size(), &argv[0]);
    if (rc == RUN_NDS_OK) return true;
    return false; 
}