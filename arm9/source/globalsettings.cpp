/*
    globalsettings.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "globalsettings.h"
#include <string.h>
#include <sys/stat.h>
#include "fontfactory.h"
#include "inifile.h"
#include "systemfilenames.h"

cGlobalSettings::cGlobalSettings() {
    fontHeight = 12;
    brightness = 1;
    language = 1;
    langDirectory = "English";
    uiName = "blue skies";
    startupFolder = "...";
    fileListType = 0;
    romTrim = 0;
    showHiddenFiles = false;
    enterLastDirWhenBoot = true;
    scrollSpeed = EScrollFast;
    showGbaRoms = true;
    viewMode = EViewInternal;
    gbaSleepHack = false;
    gbaAutoSave = false;
    Animation = true;
    cheats = false;
    softreset = true;
    dma = true;
    sdsave = true;
    cheatDB = false;
    slot2mode = ESlot2Ask;
    saveExt = true;
    saveDir = false;
    dsOnly = false;
    nightly = false;
    safeMode = false;
    show12hrClock = false;
    autorunWithLastRom = false;
    homebrewreset = false;
    resetHotKey = 0;
    phatCol = false;
#ifndef __KERNEL_LAUNCHER_SUPPORT__
    romLauncher = EKernelLauncher;
#else
    romLauncher = ENdsBootstrapLauncher;
#endif
}

void cGlobalSettings::loadSettings() {
    std::string temp;
    CIniFile ini(SFN_GLOBAL_SETTINGS);
    fontHeight = ini.GetInt("system", "fontHeight", fontHeight);
    langDirectory = ini.GetString("system", "langDirectory", langDirectory);
    uiName = ini.GetString("system", "uiName", uiName);
    startupFolder = ini.GetString("system", "startupFolder", startupFolder);
    if ('/' != startupFolder[startupFolder.length() - 1]) startupFolder += '/';
    fileListType = ini.GetInt("system", "fileListType", fileListType);
    romTrim = ini.GetInt("system", "romTrim", romTrim);
    showHiddenFiles = ini.GetInt("system", "showHiddenFiles", showHiddenFiles);
    enterLastDirWhenBoot = ini.GetInt("system", "enterLastDirWhenBoot", enterLastDirWhenBoot);
    gbaSleepHack = ini.GetInt("system", "gbaSleepHack", gbaSleepHack);
    gbaAutoSave = ini.GetInt("system", "gbaAutoSave", gbaAutoSave);
    Animation = ini.GetInt("system", "Animation", Animation);
    cheats = ini.GetInt("system", "cheats", cheats);
    softreset = ini.GetInt("system", "softreset", softreset);
    saveDir = ini.GetInt("system", "savedir", saveDir);
    dsOnly = ini.GetInt("system", "dsonly", dsOnly);
    nightly = ini.GetInt("system", "nightly", nightly);
    dma = ini.GetInt("system", "dma", dma);
    sdsave = ini.GetInt("system", "sdsave", sdsave);
    safeMode = ini.GetInt("system", "safemode", safeMode);
    show12hrClock = ini.GetInt("system", "Show12hrClock", show12hrClock);
    autorunWithLastRom = ini.GetInt("system", "autorunWithLastRom", autorunWithLastRom);
    homebrewreset = ini.GetInt("system", "homebrewreset", homebrewreset);
    resetHotKey = ini.GetInt("system", "resethotkey", resetHotKey);
    phatCol = ini.GetInt("system", "phatCol", phatCol);

    temp = ini.GetString("system", "scrollSpeed", "fast");
    scrollSpeed =
            (temp == "slow") ? EScrollSlow : ((temp == "medium") ? EScrollMedium : EScrollFast);

    temp = ini.GetString("system", "viewMode", "icon");
    viewMode = (temp == "list") ? EViewList 
         : (temp == "icon") ? EViewIcon 
         : (temp == "small") ? EViewSmall
         : EViewInternal;

    temp = ini.GetString("system", "slot2mode", "ask");
    slot2mode = (temp == "gba") ? ESlot2Gba : ((temp == "nds") ? ESlot2Nds : ESlot2Ask);

    temp = ini.GetString("system", "saveext", ".sav");
    saveExt = (temp == ".sav");

#ifdef __KERNEL_LAUNCHER_SUPPORT__
    temp = ini.GetString("system", "nds-bootstrap", "false");
    romLauncher = (temp != "false") ? ENdsBootstrapLauncher : EKernelLauncher;
#endif

    struct stat st;
    if (0 == stat(SFN_CHEATS, &st)) cheatDB = true;

    CIniFile iniBacklight(SFN_BACKLIGHT);
    brightness = iniBacklight.GetInt("brightness", "brightness", brightness);
    setBrightness(brightness);
    updateSafeMode();
}

void cGlobalSettings::saveSettings() {
    // the commented code means those parameters are not allow to change in menu
    CIniFile ini(SFN_GLOBAL_SETTINGS);
    // ini.SetInt( "system", "fontHeight", fontHeight );
    ini.SetString("system", "uiName", uiName);
    ini.SetString("system", "langDirectory", langDirectory);
    ini.SetInt("system", "fileListType", fileListType);
    ini.SetInt("system", "romTrim", romTrim);
    ini.SetInt("system", "showHiddenFiles", showHiddenFiles);
    ini.SetInt("system", "gbaSleepHack", gbaSleepHack);
    ini.SetInt("system", "gbaAutoSave", gbaAutoSave);
    ini.SetInt("system", "Animation", Animation);
    ini.SetInt("system", "cheats", cheats);
    ini.SetInt("system", "softreset", softreset);
    ini.SetInt("system", "dma", dma);
    ini.SetInt("system", "sdsave", sdsave);
    ini.SetInt("system", "safemode", safeMode);
    ini.SetInt("system", "savedir", saveDir);
    ini.SetInt("system", "nightly", nightly);
    ini.SetInt("system", "Show12hrClock", show12hrClock);
    ini.SetInt("system", "homebrewreset", homebrewreset);
    ini.SetInt("system", "resethotkey", resetHotKey);
    ini.SetInt("system", "dsonly", dsOnly);
    ini.SetInt("system", "phatCol", phatCol);

    ini.SetString("system", "scrollSpeed",
                  (scrollSpeed == EScrollSlow)
                          ? "slow"
                          : ((scrollSpeed == EScrollMedium) ? "medium" : "fast"));
    ini.SetString(
        "system", "viewMode",
        (viewMode == EViewList) ? "list" 
        : (viewMode == EViewIcon) ? "icon" 
        : (viewMode == EViewSmall) ? "small" 
        : (viewMode == EViewInternal) ? "internal"
        : "internal");
    ini.SetString("system", "slot2mode",
                  (slot2mode == ESlot2Gba) ? "gba" : ((slot2mode == ESlot2Nds) ? "nds" : "ask"));
    ini.SetString("system", "saveext", saveExt ? ".sav" : ".nds.sav");

#ifdef __KERNEL_LAUNCHER_SUPPORT__
    ini.SetString("system", "nds-bootstrap",
                  romLauncher == ENdsBootstrapLauncher ? "true" : "false");
#endif

    ini.SaveIniFile(SFN_GLOBAL_SETTINGS);
    updateSafeMode();
}

void cGlobalSettings::updateSafeMode(void) {
    if (safeMode) {
        fileListType = 0;
        showHiddenFiles = false;
        viewMode = EViewInternal;
    }
}

u32 cGlobalSettings::CopyBufferSize(void) {
    if (font().FontRAM() < 300 * 1024) return 1024 * 1024;
    return 512 * 1024;
}

void cGlobalSettings::nextBrightness(void) {
    fifoSendValue32(FIFO_USER_01, MENU_MSG_BRIGHTNESS_GET);
    while (!fifoCheckValue32(FIFO_USER_01))
        ;
    u32 currentLevel = fifoGetValue32(FIFO_USER_01);
    brightness = (currentLevel + 1) & 3;

    setBrightness(brightness);
    CIniFile ini(SFN_BACKLIGHT);
    ini.SetInt("brightness", "brightness", brightness);
    ini.SaveIniFile(SFN_BACKLIGHT);
}

void cGlobalSettings::setBrightness(u32 level) {
    fifoSendValue32(FIFO_USER_01, MENU_MSG_BRIGHTNESS_SET0 + (brightness & 3));
}
