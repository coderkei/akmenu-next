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

bool Slot1Launcher::launchRom(std::string romPath, std::string savePath, u32 flags,
                               u32 cheatOffset, u32 cheatSize, bool hb) {
    std::string slot1LoaderPath = fsManager().resolveSystemPath("/_nds/akmenunext/slot1launch.nds");

    if (access(slot1LoaderPath.c_str(), F_OK) != 0) {
        printLoaderNotFound(slot1LoaderPath);
        return false;
    }

    std::vector<const char*> argv;
    argv.push_back(slot1LoaderPath.c_str());
    eRunNdsRetCode rc = runNdsFile(argv[0], argv.size(), &argv[0]);
    if (rc == RUN_NDS_OK) return true;

    return false;
}
