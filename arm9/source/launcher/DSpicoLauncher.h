/*
    Copyright (C) 2024 lifehackerhansol

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds/ndstypes.h>

#include "DSpico/picoLoader7.h"
#include "../dsrom.h"
#include "ILauncher.h"

class DSpicoLauncher : public ILauncher {
  public:
    bool launchRom(std::string romPath, std::string savePath, u32 flags, u32 cheatOffset,
                   u32 cheatSize, bool hb) override;
    bool launchPlugin(std::string romPath, const std::string& argument);

  private:
    bool launchRomInternal(std::string romPath, std::string savePath, u32 flags,
                           u32 cheatOffset, u32 cheatSize, bool hb,
                           const std::string& arguments);
    bool prepareCheats(void);
    std::string mRomPath;
    std::string mSavePath;
    u32 mFlags;
    pload_cheats_t* mCheats = nullptr;
};
