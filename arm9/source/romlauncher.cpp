/*
    romlauncher.cpp
    Copyright (C) 2010 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <sys/stat.h>
#include "romlauncher.h"
#include "cheatwnd.h"
#include "exptools.h"
#include "flags.h"
#include "language.h"

#include "launcher/AcekardLauncher.h"
#include "launcher/HomebrewLauncher.h"
#include "launcher/ILauncher.h"
#include "launcher/NdsBootstrapLauncher.h"
#include "launcher/TopToyLauncher.h"
#include "launcher/DSpicoLauncher.h"

static SAVE_TYPE PrefillGame(u32 aGameCode) {
    if (0x45444759 == aGameCode)  // YGDE: 2209 - Diary Girl (USA)
    {
        // usa game dont have 16M save
        return ST_UNKNOWN;
    }
    switch (aGameCode & 0xffffff) {
        case 0x385959:  // YY8*: 2436 - Kakushi E Mystery - Riku to Yohan - Kieta 2-mai no E (Japan)
            return ST_64K;
        case 0x533243:  // C2S*: Pokemon Mystery Dungeon - Explorers of Sky
            return ST_1M;
        case 0x593241:  // A2Y*: Ganbaru Watashi no Kakei Diary
        case 0x4c5a41:  // AZL*: Nintendo Presents - Style Boutique
        case 0x494b42:  // BKI*: Legend of Zelda, The - Spirit Tracks
        case 0x503643:  // C6P*: Picross 3D
        case 0x574d49:  // IMW*: Walk with Me!
        case 0x4e4259:  // YBN*: 100 Classic Book Collection
        case 0x544556:  // VET*: 1000 Cooking Recipes from Elle a Table
            return ST_8M;
        case 0x444759:  // YGD*: Winx Club - Secret Diary 2009
        case 0x565042:  // BPV*: Puzzler World
            return ST_16M;
        case 0x425841:  // AXB*: Daigassou! Band-Brothers DX
        case 0x414156:  // VAA*: Eigokoro Kyoushitsu DS
            return ST_64M;
    }
    return ST_UNKNOWN;
}

static u32 SaveMask(SAVE_TYPE st) {
    u32 result;
    switch (st) {
        case ST_NOSAVE:
            result = PATCH_SAVE_NO;
            break;
        case ST_4K:
            result = 0;
            break;
        case ST_64K:
            result = 4;
            break;
        case ST_512K:
            result = 7;
            break;
        case ST_1M:
            result = 8;
            break;
        case ST_2M:
            result = 9;
            break;
        case ST_4M:
            result = 10;
            break;
        case ST_8M:
            result = 11;
            break;
        case ST_16M:
            result = 12;
            break;
        case ST_32M:
            result = 13;
            break;
        case ST_64M:
            result = 14;
            break;
        default:
            result = 10;  // 4M default
            break;
    }
    return result;
}

static u32 SaveSize(SAVE_TYPE st) {
    u32 result;
    switch (st) {
        case ST_NOSAVE:
            result = 0;
            break;
        case ST_4K:
            result = 512;
            break;
        case ST_64K:
            result = 8192;
            break;
        case ST_512K:
            result = 65536;
            break;
        case ST_1M:
            result = 131072;
            break;
        case ST_2M:
            result = 262144;
            break;
        case ST_4M:
            result = 524288;
            break;
        case ST_8M:
            result = 1048576;
            break;
        case ST_16M:
            result = 2097152;
            break;
        case ST_32M:
            result = 4194304;
            break;
        case ST_64M:
            result = 8388608;
            break;
        default:
            result = 524288;  // 4M default
            break;
    };
    return result;
}

TLaunchResult launchRom(const std::string& aFullPath, DSRomInfo& aRomInfo, bool aMenu, const std::string& savesPath) {
    u32 flags = 0;
    long cheatOffset = 0;
    size_t cheatSize = 0;
    std::string saveName;
    std::string useSavesPath;
    ILauncher* launcher = nullptr;
    bool isDsiWare;
    bool hb = false;

    if (!aRomInfo.isHomebrew()) {
        u32 gameCode;
        memcpy(&gameCode, aRomInfo.saveInfo().gameCode, sizeof(gameCode));  // because alignment
        bool isBigSave = false;
        u32 bigSaveSize = 8 * 1024 * 1024;
        u32 bigSaveMask = 14;
        // check for DSiWare
        isDsiWare = aRomInfo.isDSiWare();
        // reading speed setting
        std::string disk = aFullPath.substr(0, 5);
        // bool dma = false, protection = aRomInfo.saveInfo().isProtection();
        bool protection = aRomInfo.saveInfo().isProtection();
        u32 speed = 0;
        {
            if (protection) speed = 0x1fff;
            // dma = aRomInfo.saveInfo().isDMA();
            aRomInfo.saveInfo().isDMA();
            flags |= PATCH_DMA;
            if ((gameCode & 0xffffff) ==
                0x425855)  // 4950 - Jam with the Band (Europe) (En,Fr,De,Es,It)
            {
                isBigSave = true;
                bigSaveSize = 32 * 1024 * 1024;
                bigSaveMask = 0;
            }
            if ((gameCode & 0xffffff) ==
                0x524f55)  // 3690 - Made in Ore (Japan) & 4812 - WarioWare D.I.Y. (USA)
            {
                isBigSave = true;
                bigSaveSize = 32 * 1024 * 1024;
                bigSaveMask = 0;
            }
        }

        std::string savesFolderPath = aFullPath;

        //if saveDir is set to true, use saves dir
        if (gs().saveDir) {
            useSavesPath = savesPath;
            savesFolderPath.insert(aFullPath.find_last_of("/\\") + 1, "saves/");
            size_t lastSlashPos = savesFolderPath.find_last_of("/\\");
            std::string directory = savesFolderPath.substr(0, lastSlashPos + 1);
            // Create the saves folder if it doesn't exist
            std::string cleanPath = (directory.find("fat:") == 0) ? directory.substr(4) : directory;
            cleanPath.pop_back();
            if (access(cleanPath.c_str(), F_OK) != 0) {
                mkdir(cleanPath.c_str(), 0777);
                }
            }
        else useSavesPath = aFullPath;

        saveName = cSaveManager::generateSaveName(savesFolderPath, aRomInfo.saveInfo().getSlot());

        if (isBigSave) {
            isBigSave = cSaveManager::initializeSaveFile(useSavesPath, aRomInfo.saveInfo().getSlot(),
                                                         bigSaveSize);
            if (!isBigSave) return ELaunchNoFreeSpace;
            flags |= PATCH_SD_SAVE | (bigSaveMask << PATCH_SAVE_SHIFT);
            saveManager().saveLastInfo(aFullPath);
        } else {
            SAVE_TYPE st = (SAVE_TYPE)aRomInfo.saveInfo().saveType;
            if (ST_UNKNOWN == st) st = ST_AUTO;
            SAVE_TYPE st_new = PrefillGame(gameCode);
            if (st_new != ST_UNKNOWN)  // special save size
            {
                if (st_new != st) {
                    st = st_new;
                    aRomInfo.saveInfo().saveType = st;
                    saveManager().updateCustomSaveList(aRomInfo.saveInfo());
                }
            }
            if(!isDsiWare){
                if (cSaveManager::initializeSaveFile(useSavesPath, aRomInfo.saveInfo().getSlot(),
                SaveSize(st))) {
                flags |= PATCH_SD_SAVE | (SaveMask(st) << PATCH_SAVE_SHIFT);
                saveManager().saveLastInfo(aFullPath);
                } else {
                return ELaunchNoFreeSpace;
                }
            }

        }
        __NDSHeader->cardControl13 = 0x00406000 | speed;

        if (aRomInfo.saveInfo().isDownloadPlay()) flags |= PATCH_DOWNLOAD_PLAY;
        if (aRomInfo.saveInfo().isCheat()) {
            u32 gameCode, crc32;
            if (cCheatWnd::romData(aFullPath, gameCode, crc32)) {
                FILE* dat = fopen(SFN_CHEATS, "rb");
                if (dat) {
                    if (cCheatWnd::searchCheatData(dat, gameCode, crc32, cheatOffset, cheatSize)) {
                        flags |= PATCH_CHEATS;
                    }
                    fclose(dat);
                }
            }
        }
        if (aRomInfo.saveInfo().isSoftReset()) flags |= PATCH_SOFT_RESET;
        if (aRomInfo.saveInfo().isLinkage()) flags |= PATCH_LINKAGE;
        u8 language = aRomInfo.saveInfo().getLanguage();
        if (language) flags |= (language << PATCH_LANGUAGE_SHIFT) & PATCH_LANGUAGE_MASK;
#ifndef __KERNEL_LAUNCHER_SUPPORT__
        if(gs().pico){
            launcher = new DSpicoLauncher();
        }
        else {
            launcher = new NdsBootstrapLauncher();
        }
#else  // __KERNEL_LAUNCHER_SUPPORT__
        if (aRomInfo.saveInfo().isNdsBootstrap())
            launcher = new NdsBootstrapLauncher();
        else
#ifdef __TTLAUNCHER__
            launcher = new TopToyLauncher();
#else   // __TTLAUNCHER__
            launcher = new AcekardLauncher();
#endif  // __TTLAUNCHER__
#endif  // __KERNEL_LAUNCHER_SUPPORT__
    } else {
        if (!aMenu) saveManager().saveLastInfo(aFullPath);
        if (gs().hbStrap == 1)
        {
            hb = true;
            launcher = new NdsBootstrapLauncher();
        }
        else{
            launcher = new HomebrewLauncher();
        }
    }
    launcher->launchRom(aFullPath, saveName, flags, cheatOffset, cheatSize, hb);
    return ELaunchRomOk;
}

void autoLaunchRom(const std::string& aFullPath) {
    DSRomInfo rominfo;
    rominfo.MayBeDSRom(aFullPath);
    if (rominfo.isDSRom()) launchRom(aFullPath, rominfo, false, NULL);
}
