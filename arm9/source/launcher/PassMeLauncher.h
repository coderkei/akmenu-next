/*
    Copyright (C) 2024 lifehackerhansol

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <string>

#include "ILauncher.h"

class PassMeLauncher : public ILauncher {
  public:
    bool launchRom(std::string romPath, std::string savePath, u32 flags, u32 cheatOffset,
                   u32 cheatSize, bool hb) override;
};
