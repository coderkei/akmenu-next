/*
    Copyright (C) 2024 lifehackerhansol

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <nds/ndstypes.h>
#include <string>
#include <vector>

#include "ILauncher.h"
#include "PassMeLauncher.h"
#include "nds_loader_arm9.h"

bool PassMeLauncher::launchRom(std::string romPath, std::string savePath, u32 flags,
                               u32 cheatOffset, u32 cheatSize) {
    const char passMeLoaderPath[] = "fat:/_nds/PassMeLoader.nds";

    if (access(passMeLoaderPath, F_OK) != 0) {
        printLoaderNotFound(passMeLoaderPath);
        return false;
    }

    std::vector<const char*> argv;
    argv.push_back(passMeLoaderPath);
    eRunNdsRetCode rc = runNdsFile(argv[0], argv.size(), &argv[0]);
    if (rc == RUN_NDS_OK) return true;

    return false;
}
