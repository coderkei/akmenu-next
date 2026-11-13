/*
    Copyright (C) 2024 lifehackerhansol

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds/ndstypes.h>
#include "../language.h"
#include "../ui/msgbox.h"
#include "nds_loader_arm9.h"

static inline void printLoaderNotFound(const std::string& loaderPath, const std::string& help) {
    akui::messageBox(NULL, LANG("loader", "not found"), help + " " + loaderPath, MB_OK);
}

static inline void printMessage(std::string infoMsg) {
    akui::messageBox(NULL, LANG("loader", "info"), infoMsg, MB_OK);
}

static inline void printError(std::string errorMsg) {
    akui::messageBox(NULL, LANG("loader", "error"), errorMsg, MB_OK);
}

static inline void printSavePreparationError() {
    printError(lang().GetString("loader", "save preparation error",
                                "Could not prepare the save. Check free space, card access, and the saves folder."));
}

static inline bool checkNdsLaunch(eRunNdsRetCode result) {
    switch (result) {
        case RUN_NDS_OK:
            return true;
        case RUN_NDS_STAT_FAILED:
            printError(lang().GetString("loader", "file access error",
                                        "Could not open the launch file. Restore it and check the card."));
            break;
        case RUN_NDS_GETCWD_FAILED:
            printError(lang().GetString("loader", "folder access error",
                                        "Could not read the launch folder. Reinsert the card and try again."));
            break;
        case RUN_NDS_PATCH_DLDI_FAILED:
            printError(lang().GetString("loader", "dldi error",
                                        "Could not apply the card driver. Reinstall the correct build for your flashcart."));
            break;
    }
    return false;
}

class ILauncher {
  public:
    virtual ~ILauncher() {}
    virtual bool launchRom(std::string romPath, std::string savePath, u32 flags, u32 cheatOffset,
                           u32 cheatSize, bool hb) = 0;
};
