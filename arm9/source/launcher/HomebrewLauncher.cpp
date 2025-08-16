/*
    Copyright (C) 2024 lifehackerhansol

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <nds/ndstypes.h>
#include <string>
#include <vector>

#include "flags.h"
#include "HomebrewLauncher.h"
#include "ILauncher.h"
#include "nds_loader_arm9.h"

bool HomebrewLauncher::launchRom(std::string romPath, std::string savePath, u32 flags,
                                 u32 cheatOffset, u32 cheatSize, bool hb) {
    std::vector<const char*> argv;
    argv.push_back(romPath.c_str());
    eRunNdsRetCode rc = runNdsFile(argv[0], argv.size(), &argv[0]);
    if (rc == RUN_NDS_OK) return true;

    return false;
}
