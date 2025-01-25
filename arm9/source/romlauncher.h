/*
    romlauncher.h
    Copyright (C) 2010 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "dsrom.h"

enum TLaunchResult {
    ELaunchRomOk,
    ELaunchSDOnly,
    ELaunchRestoreFail,
    ELaunchSlowSD,
    ELaunchNoFreeSpace
};

TLaunchResult launchRom(const std::string& aFullPath, DSRomInfo& aRomInfo, bool aMenu, const std::string& savesPath);
void autoLaunchRom(const std::string& aFullPath);
