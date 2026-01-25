/*
    Copyright (C) 2024 lifehackerhansol

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <nds/ndstypes.h>
#include <string>
#include <vector>

#include "ILauncher.h"
#include "Slot1Launcher.h"
#include "nds_loader_arm9.h"
#include "picocheck.h"

bool Slot1Launcher::launchRom(std::string romPath, std::string savePath, u32 flags,
                               u32 cheatOffset, u32 cheatSize, bool hb) {

    const char* slot1LoaderPath;
    #ifdef __DSIMODE__
        if(isDSPico){
            slot1LoaderPath = "fat:/_nds/akmenunext/slot1launch.nds";
        }
        else{
            slot1LoaderPath = "sd:/_nds/akmenunext/slot1launch.nds";
        }          
    #else
        slot1LoaderPath = "fat:/_nds/akmenunext/slot1launch.nds";
    #endif

    if (access(slot1LoaderPath, F_OK) != 0) {
        printLoaderNotFound(slot1LoaderPath);
        return false;
    }

    std::vector<const char*> argv;
    argv.push_back(slot1LoaderPath);
    eRunNdsRetCode rc = runNdsFile(argv[0], argv.size(), &argv[0]);
    if (rc == RUN_NDS_OK) return true;

    return false;
}
