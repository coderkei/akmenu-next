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
    enum TViewMode { EViewList = 0, EViewIcon = 1, EViewInternal = 2, EViewSmall = 3 };
    enum TSlot2Mode { ESlot2Ask = 0, ESlot2Gba = 1, ESlot2Nds = 2 };
    enum TROMLauncher { EKernelLauncher = 0, ENdsBootstrapLauncher = 1 };
    enum TIconSource { EIconBuiltIn = 0, EIconGlobal = 1, EIconTheme = 2 };

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
    int languageOverride;
    int resetHotKey;
    int scrollSpeed;
    int romLauncher;
    int viewMode;
    int slot2mode;
    int saveSlotOverride;
    bool showHiddenFiles;
    bool enterLastDirWhenBoot;
    bool showGbaRoms;
    bool gbaSleepHack;
    bool gbaAutoSave;
    bool Animation;
    bool cheats;
    bool softreset;
    bool sdsave;
    bool cheatDB;
    bool saveExt;
    bool saveDir;
    bool nightly;
    bool dma;
    bool safeMode;
    bool show12hrClock;
    bool showCovers;
    bool autorunWithLastRom;
    bool homebrewreset;
    bool dsOnly;
    bool boostCpu;
    bool ignoreCrc16;
    bool phatCol;
    // Homebrew loader: 0 = AKMenu-Next, 1 = nds-bootstrap-hb, 2 = Pico-Loader.
    int hbStrap;
    bool pico;
    int iconSource;
    int cardReadDma;
};

typedef t_singleton<cGlobalSettings> globalSettings_s;
inline cGlobalSettings& gs() {
    return globalSettings_s::instance();
}
