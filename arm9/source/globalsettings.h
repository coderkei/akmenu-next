/*
    globalsettings.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds.h>
#include <string>
#include "../../share/fifotool.h"
#include "singleton.h"

class cGlobalSettings {
  public:
    enum TScrollSpeed { EScrollFast = 4, EScrollMedium = 10, EScrollSlow = 16 };
    enum TViewMode { EViewList = 0, EViewIcon = 1, EViewInternal = 2 };
    enum TSlot2Mode { ESlot2Ask = 0, ESlot2Gba = 1, ESlot2Nds = 2 };
    enum TROMLauncher { EKernelLauncher = 0, ENdsBootstrapLauncher = 1 };

  public:
    cGlobalSettings();

    ~cGlobalSettings();

  public:
    void loadSettings();
    void saveSettings();
    void updateSafeMode(void);
    static u32 CopyBufferSize(void);
    void nextBrightness(void);
    void setBrightness(u32 level);

  public:
    u8 fontHeight;
    u8 brightness;
    u8 language;
    u8 fileListType;
    u8 romTrim;
    std::string langDirectory;
    std::string uiName;
    std::string startupFolder;
    bool showHiddenFiles;
    bool enterLastDirWhenBoot;
    int scrollSpeed;
    bool showGbaRoms;
    int viewMode;
    bool gbaSleepHack;
    bool gbaAutoSave;
    bool Animation;
    bool cheats;
    bool softreset;
    bool cardDma;
    bool sdsave;
    bool cheatDB;
    int slot2mode;
    bool saveExt;
    bool saveDir;
    bool nightly;
    bool dma;
    bool safeMode;
    bool show12hrClock;
    bool autorunWithLastRom;
    bool homebrewreset;
    int romLauncher;
};

typedef t_singleton<cGlobalSettings> globalSettings_s;
inline cGlobalSettings& gs() {
    return globalSettings_s::instance();
}
