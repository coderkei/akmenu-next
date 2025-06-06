/*
    Copyright (C) 2024 lifehackerhansol
    Additional modifications Copyright (C) 2015 coderkei

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

bool NdsBootstrapLauncher::prepareIni() {
    CIniFile ini;
    hotkeyCheck = false;

    ini.SetString("NDS-BOOTSTRAP", "NDS_PATH", mRomPath);
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
    if(gs().dsOnly) {
        ini.SetString("NDS-BOOTSTRAP", "DSI_MODE", "0");
    }
    if(gs().phatCol) {
        ini.SetString("NDS-BOOTSTRAP", "PHAT_COLORS", "1");
    }

    if (access("/_nds/debug.txt", F_OK) == 0) {
        ini.SetString("NDS-BOOTSTRAP", "LOGGING", "1");
        ini.SetString("NDS-BOOTSTRAP", "DEBUG", "1");
    }

    ini.SaveIniFile("/_nds/nds-bootstrap.ini");

    return true;
}

bool NdsBootstrapLauncher::launchRom(std::string romPath, std::string savePath, u32 flags,
                                     u32 cheatOffset, u32 cheatSize) {
    const char ndsBootstrapPath[] = SD_ROOT_0 "/_nds/nds-bootstrap-release.nds";
    const char ndsBootstrapPathNightly[] = SD_ROOT_0 "/_nds/nds-bootstrap-nightly.nds";
    const char ndsBootstrapCheck[] = SD_ROOT_0 "/_nds/pagefile.sys";
    bool useNightly = false;

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
    progressWnd().setPercent(25);

    // Setup argv to launch nds-bootstrap                             
    if(!useNightly){
        argv.push_back(ndsBootstrapPath);
    }
    else{
        argv.push_back(ndsBootstrapPathNightly);
    }

    progressWnd().setTipText("Loading usrcheat.dat...");
    progressWnd().setPercent(50);
    // Prepare cheat codes if enabled
    if (flags & PATCH_CHEATS) {
        if (!prepareCheats()) {
            return false;
        }
    }
    progressWnd().setTipText("Initializing nds-bootstrap...");
    progressWnd().setPercent(75);

    //Clean up old INI
    if (access("/_nds/nds-bootstrap/nds-bootstrap.ini", F_OK) == 0) {
        remove("/_nds/nds-bootstrap/nds-bootstrap.ini");
    }

    // Setup nds-bootstrap INI parameters
    if (!prepareIni()) return false;
    progressWnd().setPercent(100);

    // Launch
    eRunNdsRetCode rc = runNdsFile(argv[0], argv.size(), &argv[0]);
    if (rc == RUN_NDS_OK) return true;

    return false;
}
