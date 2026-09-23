/*
    Copyright (C) 2024 lifehackerhansol

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds/ndstypes.h>

#include "../dsrom.h"
#include "ILauncher.h"

class NdsBootstrapLauncher : public ILauncher {
  public:
    bool launchRom(std::string romPath, std::string savePath, u32 flags, u32 cheatOffset,
                   u32 cheatSize, bool hb) override;
    bool launchPlugin(std::string romPath, const std::string& argumentPath, bool useArgv,
                      bool useHbArgv);

  private:
    bool prepareCheats(void);
    bool prepareIni(bool hb, const std::string& homebrewArg = "");
    bool launchRomInternal(std::string romPath, std::string savePath, u32 flags,
                           u32 cheatOffset, u32 cheatSize, bool hb,
                           const std::string& homebrewArg);
    bool is3DS(void);
    bool hotkeyCheck;
    std::string mRomPath;
    std::string mSavePath;
    u32 mFlags;
    DSRomInfo _romInfo;
};
