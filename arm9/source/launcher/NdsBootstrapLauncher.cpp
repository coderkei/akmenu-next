/*
    Copyright (C) 2024 lifehackerhansol
    Additional modifications Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fat.h>

#include <nds/ndstypes.h>

#include "cheat.h"
#include "../animatedicon.h"
#include "../dsrom.h"
#include "../flags.h"
#include "../inifile.h"
#include "../mainlist.h"
#include "../systemfilenames.h"
#include "../language.h"
#include "../ui/msgbox.h"
#include "../ui/progresswnd.h"
#include "ILauncher.h"
#include "../debugoverlay.h"
#include "NdsBootstrapDonor.h"
#include "NdsBootstrapLauncher.h"
#include "nds_loader_arm9.h"
#include "fsmngr.h"

#include "slot2/io_g6_common.h"
#include "slot2/io_m3_common.h"
#include "slot2/io_sc_common.h"
#include "libfat_ext/fat_ext.h"

namespace {
struct DsiWareFatHeader {
    u8 bootJump[3];
    u8 oemName[8];
    u16 bytesPerSector;
    u8 sectorsPerCluster;
    u16 reservedSectors;
    u8 numberOfFats;
    u16 rootEntryCount;
    u16 totalSectors;
    u8 mediaDescriptor;
    u16 fatSize;
    u16 sectorsPerTrack;
    u16 numberOfHeads;
    u32 hiddenSectors;
    u32 totalSectors32;
    u8 driveNumber;
    u8 reserved;
    u8 bootSignature;
    u32 volumeId;
    u8 volumeLabel[11];
    u8 fileSystemType[8];
    u8 bootCode[448];
    u16 bootSectorSignature;
} __attribute__((packed));

static_assert(sizeof(DsiWareFatHeader) == 0x200, "DSiWare FAT boot sector must be 512 bytes");

static u32 alignDsiWareValue(u32 value, u32 multiple) {
    const u32 remainder = value % multiple;
    return remainder ? value + multiple - remainder : value;
}

// DSiWare save images follow TWiLightMenu++'s formatter, based on NTM's sav.c.
static bool createDsiWareSave(const std::string& path, u32 size) {
    const u32 sectorSize = 0x200;
    if (size < sectorSize) return false;

    const u32 maxSectors = size / sectorSize;
    if (maxSectors > 0xFFFF) return false;

    u32 sectorCount = 1;
    u32 sectorsPerTrack = 1;
    u32 numberOfHeads = 1;
    u32 nextSectorCount = 0;
    while (nextSectorCount <= maxSectors) {
        nextSectorCount = sectorsPerTrack * (numberOfHeads + 1) * (numberOfHeads + 1);
        if (nextSectorCount <= maxSectors) {
            numberOfHeads++;
            sectorCount = nextSectorCount;

            sectorsPerTrack++;
            nextSectorCount = sectorsPerTrack * numberOfHeads * numberOfHeads;
            if (nextSectorCount <= maxSectors) sectorCount = nextSectorCount;
        }
    }
    nextSectorCount = (sectorsPerTrack + 1) * numberOfHeads * numberOfHeads;
    if (nextSectorCount <= maxSectors) {
        sectorsPerTrack++;
        sectorCount = nextSectorCount;
    }

    const u8 sectorsPerCluster = sectorCount > (8 << 10) ? 8 :
                                 sectorCount > (1 << 10) ? 4 : 1;
    const u16 rootEntryCount = size < 0x8C000 ? 0x20 : 0x200;
    const u32 totalClusters = alignDsiWareValue(sectorCount, sectorsPerCluster) /
                              sectorsPerCluster;
    const u32 fatBytes = (alignDsiWareValue(totalClusters, 2) / 2) * 3;
    const u32 fatSize = alignDsiWareValue(fatBytes, sectorSize) / sectorSize;

    DsiWareFatHeader header;
    memset(&header, 0, sizeof(header));
    header.bootJump[0] = 0xE9;
    memcpy(header.oemName, "MSWIN4.1", 8);
    header.bytesPerSector = sectorSize;
    header.sectorsPerCluster = sectorsPerCluster;
    header.reservedSectors = 1;
    header.numberOfFats = 2;
    header.rootEntryCount = rootEntryCount;
    header.totalSectors = sectorCount;
    header.mediaDescriptor = 0xF8;
    header.fatSize = fatSize;
    header.sectorsPerTrack = sectorsPerTrack;
    header.numberOfHeads = numberOfHeads;
    header.driveNumber = 0x05;
    header.bootSignature = 0x29;
    header.volumeId = 0x12345678;
    memcpy(header.volumeLabel, "VOLUMELABEL", 11);
    memcpy(header.fileSystemType, "FAT12   ", 8);
    header.bootSectorSignature = 0xAA55;

    FILE* save = fopen(path.c_str(), "wb");
    if (!save) return false;

    bool success = fwrite(&header, sizeof(header), 1, save) == 1;
    u32 written = 0;
    while (success && written < size) {
        written += 0x8000;
        if (written > size) written = size;
        success = fseek(save, written - 1, SEEK_SET) == 0 && fputc('\0', save) != EOF;
    }
    if (fclose(save) != 0) success = false;
    if (!success) remove(path.c_str());
    return success;
}

static bool ensureDsiWareSave(const std::string& path, u32 size) {
    struct stat existing;
    if (stat(path.c_str(), &existing) == 0 && existing.st_size > 0) return true;
    return createDsiWareSave(path, size);
}

static std::string replaceSaveExtension(const std::string& path, const char* extension) {
    const size_t slash = path.find_last_of("/\\");
    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
        return path + extension;
    }
    return path.substr(0, dot) + extension;
}

static bool getDsiWareHeaderInfo(const std::string& romPath, u32& twlRomSize,
                                 u32& publicSize, u32& privateSize) {
    // sNDSHeaderExt::twlRomSize, pubSavSize, and prvSavSize are at 0x210, 0x238, and 0x23C.
    FILE* rom = fopen(romPath.c_str(), "rb");
    if (!rom) return false;
    const bool readOk = fseek(rom, 0x210, SEEK_SET) == 0 &&
                        fread(&twlRomSize, sizeof(twlRomSize), 1, rom) == 1 &&
                        fseek(rom, 0x238, SEEK_SET) == 0 &&
                        fread(&publicSize, sizeof(publicSize), 1, rom) == 1 &&
                        fread(&privateSize, sizeof(privateSize), 1, rom) == 1;
    fclose(rom);
    return readOk;
}

static bool moveDsiWareSaveBesideRom(const std::string& configuredPath,
                                     const std::string& romSidecarPath) {
    if (configuredPath == romSidecarPath || access(configuredPath.c_str(), F_OK) != 0 ||
        access(romSidecarPath.c_str(), F_OK) == 0) {
        return true;
    }
    return rename(configuredPath.c_str(), romSidecarPath.c_str()) == 0;
}

static bool appendUtf8Codepoint(const std::string& text, size_t& offset, u16* output,
                                size_t capacity, size_t& outputLength) {
    const u8 first = (u8)text[offset++];
    u32 codepoint;
    u32 minimum;
    size_t continuationCount;
    if (first < 0x80) {
        codepoint = first;
        minimum = 0;
        continuationCount = 0;
    } else if ((first & 0xE0) == 0xC0) {
        codepoint = first & 0x1F;
        minimum = 0x80;
        continuationCount = 1;
    } else if ((first & 0xF0) == 0xE0) {
        codepoint = first & 0x0F;
        minimum = 0x800;
        continuationCount = 2;
    } else if ((first & 0xF8) == 0xF0) {
        codepoint = first & 0x07;
        minimum = 0x10000;
        continuationCount = 3;
    } else {
        return false;
    }

    if (offset + continuationCount > text.size()) return false;
    for (size_t ii = 0; ii < continuationCount; ++ii) {
        const u8 next = (u8)text[offset++];
        if ((next & 0xC0) != 0x80) return false;
        codepoint = (codepoint << 6) | (next & 0x3F);
    }
    if (codepoint < minimum || codepoint > 0x10FFFF ||
        (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
        return false;
    }

    const size_t needed = codepoint > 0xFFFF ? 2 : 1;
    if (outputLength + needed > capacity) return false;
    if (needed == 1) {
        output[outputLength++] = (u16)codepoint;
    } else {
        codepoint -= 0x10000;
        output[outputLength++] = (u16)(0xD800 | (codepoint >> 10));
        output[outputLength++] = (u16)(0xDC00 | (codepoint & 0x3FF));
    }
    return true;
}

static bool makeUnlaunchPath(const std::string& romPath, u16* output, size_t capacity) {
    std::string path = romPath;
    if (path.compare(0, 4, "sd:/") == 0) path.replace(0, 4, "sdmc:/");

    size_t inputOffset = 0;
    size_t outputLength = 0;
    while (inputOffset < path.size()) {
        if (!appendUtf8Codepoint(path, inputOffset, output, capacity, outputLength)) return false;
    }
    output[outputLength] = 0;
    return true;
}

static bool bootDsiWareWithUnlaunch(const std::string& romPath) {
    u16 unlaunchPath[0x104] = {0};
    if (!makeUnlaunchPath(romPath, unlaunchPath, 0x103)) return false;

    memcpy((void*)0x02000800, "AutoLoadInfo", 12);
    *(vu16*)0x0200080C = 0x3F0;
    *(vu16*)0x0200080E = 0;
    *(vu32*)0x02000810 = BIT(0) | BIT(1);
    *(vu16*)0x02000814 = 0x7FFF;
    *(vu16*)0x02000816 = 0x7FFF;
    memset((void*)0x02000818, 0, 0x20 + 0x208 + 0x1C0);
    memcpy((void*)0x02000838, unlaunchPath, sizeof(unlaunchPath));
    *(vu16*)0x0200080E = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);

    DC_FlushAll();
    fifoSendValue32(FIFO_USER_01, MENU_MSG_UNLAUNCH_BOOT);
    while (true) swiWaitForVBlank();
}

static bool makeDsiSdAlias(const std::string& path, char* alias, size_t aliasSize) {
    if (path.empty() || path.size() >= 256 || path.compare(0, 4, "sd:/") != 0 ||
        aliasSize < 5) {
        return false;
    }
    memset(alias, 0, aliasSize);
    fatGetAliasPath("sd:/", path.c_str(), alias);
    return strncmp(alias, "sd:/", 4) == 0 && alias[4] != '\0';
}
}  // namespace

void slot2RamAccess(){
    // if running from NDS slot
    if (io_dldi_data->ioInterface.features & FEATURE_SLOT_NDS) {
        sysSetCartOwner(BUS_OWNER_ARM9);

        *(vu16*)0x08000000 = 0x1111;    // write data to GBA ROM area
        // if data wasn't written, try enabling RAM access for various slot 2 carts and write again
        if (*(vu16*)0x08000000 != 0x1111) {
            _SC_changeMode(SC_MODE_RAM);
            *(vu16*)0x08000000 = 0x1111;
        }
        if (*(vu16*)0x08000000 != 0x1111) {
            _G6_SelectOperation(G6_MODE_RAM);
            *(vu16*)0x08000000 = 0x1111;
        }
        if (*(vu16*)0x08000000 != 0x1111) {
            _M3_changeMode(M3_MODE_RAM);
            *(vu16*)0x08000000 = 0x1111;
        }

        sysSetCartOwner(BUS_OWNER_ARM7);
    }
}

bool NdsBootstrapLauncher::prepareCheats() {
    u32 gameCode, crc32;
    const std::string cheatDataPath = mBootstrapRoot + "_nds/nds-bootstrap/cheatData.bin";

    if (cCheat::romData(mRomPath, gameCode, crc32)) {
        FILE* cheatDb = fopen((SFN_CHEATS).c_str(), "rb");
        if (!cheatDb) goto cheat_failed;
        long cheatOffset;
        size_t cheatSize;
        if (cCheat::searchCheatData(cheatDb, gameCode, crc32, cheatOffset, cheatSize)) {
            cCheat cheat;
            cheat.parse(mRomPath);
            cheat.writeCheatsToFile(cheatDataPath.c_str());
            FILE* cheatData = fopen(cheatDataPath.c_str(), "rb");
            if (cheatData) {
                u32 check[2];
                fread(check, 1, 8, cheatData);
                fclose(cheatData);
                // TODO: Delete file, if above 0x8000 bytes
                if (check[1] == 0xCF000000) goto cheat_failed;
            }
        } else {
            fclose(cheatDb);
            goto cheat_failed;
        }
        fclose(cheatDb);
    }

    return true;

cheat_failed:
    // Remove cheat bin if exists
    if (access(cheatDataPath.c_str(), F_OK) == 0) {
        remove(cheatDataPath.c_str());
    }

    return false;
}

bool NdsBootstrapLauncher::prepareIni(bool hb, const std::string& homebrewArg,
                                      bool homebrewLaunch) {
    CIniFile ini;
    hotkeyCheck = false;

    ini.SetString("NDS-BOOTSTRAP", "NDS_PATH", mRomPath);
    if (!homebrewArg.empty()) {
        ini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", homebrewArg);
    }

    if(hb == true)
    {
        const std::string bootstrapIniPath = mBootstrapRoot + "_nds/nds-bootstrap.ini";
        configureNdsBootstrapDonors(ini, false, false, bootstrapIniPath);
        ini.SetString("NDS-BOOTSTRAP", "DSI_MODE", 0);
        ini.SaveIniFile(bootstrapIniPath);
        debugOverlay::updateDonorStatus(ini, false);
        return true;
    }

    if (_romInfo.isDSiWare()) {
        char romAlias[256] = {0};
        char publicSaveAlias[256] = {0};
        char privateSaveAlias[256] = {0};
        if (!makeDsiSdAlias(mRomPath, romAlias, sizeof(romAlias)) ||
            (access(mDsiWarePubPath.c_str(), F_OK) == 0 &&
             !makeDsiSdAlias(mDsiWarePubPath, publicSaveAlias, sizeof(publicSaveAlias))) ||
            (access(mDsiWarePrvPath.c_str(), F_OK) == 0 &&
             !makeDsiSdAlias(mDsiWarePrvPath, privateSaveAlias, sizeof(privateSaveAlias)))) {
            progressWnd().hide();
            printError("Could not prepare the DSiWare SD card paths for nds-bootstrap.");
            return false;
        }

        ini.SetString("NDS-BOOTSTRAP", "APP_PATH", romAlias);
        ini.SetString("NDS-BOOTSTRAP", "SAV_PATH", publicSaveAlias);
        ini.SetString("NDS-BOOTSTRAP", "PRV_PATH", privateSaveAlias);
        char bannerSaveAlias[256] = {0};
        if (!mBannerSavePath.empty() &&
            makeDsiSdAlias(mBannerSavePath, bannerSaveAlias, sizeof(bannerSaveAlias))) {
            ini.SetString("NDS-BOOTSTRAP", "BNR_PATH", bannerSaveAlias);
        }
        ini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 1);
        ini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 1);
        ini.SetInt("NDS-BOOTSTRAP", "DONOR_SDK_VER", 5);
        ini.SetInt("NDS-BOOTSTRAP", "GAME_SOFT_RESET", 1);
        ini.SetInt("NDS-BOOTSTRAP", "PATCH_MPU_REGION", 0);
        ini.SetInt("NDS-BOOTSTRAP", "PATCH_MPU_SIZE", 0);
    } else {
        ini.SetString("NDS-BOOTSTRAP", "SAV_PATH", mSavePath);
    }

    ini.SetString("NDS-BOOTSTRAP", "QUIT_PATH", fsManager().resolveSystemPath("/_nds/akmenunext/launcher.nds"));

    const bool consoleIs3DS = is3DS();
    ini.SetString("NDS-BOOTSTRAP", "CONSOLE_MODEL", consoleIs3DS ? "2" : "0");
    const std::string bootstrapIniPath = mBootstrapRoot + "_nds/nds-bootstrap.ini";
    configureNdsBootstrapDonors(ini, consoleIs3DS, !homebrewLaunch, bootstrapIniPath);

    std::string custIniPath = fsManager().resolveSystemPath("/_nds/akmenunext/ndsbs.ini");

    if(access(custIniPath.c_str(), F_OK) != 0){
            akui::messageBox(NULL, LANG("nds bootstrap", "inimissingtitle"), LANG("nds bootstrap", "inimissing"), MB_OK);
            return false;
        }

    std::string externalHotkey;
    {
        CIniFile extIni;
        if (extIni.LoadIniFile(custIniPath)) {
            externalHotkey = extIni.GetString("ndsbs", "hotkey", "");
            if (!externalHotkey.empty()) {
                hotkeyCheck = true;
            } else {
                hotkeyCheck = false;
            }
        } else {
            akui::messageBox(NULL, LANG("nds bootstrap", "inimissingtitle"), LANG("nds bootstrap", "inimissing"), MB_OK);
        }
    }


    /*
    0 = l-↓-select : 200 80 4
    1 = l-r-start : 200 100 8
    2 = l-r-select : 200 100 4
    3 = l-r-a-b-↓ : 200 100 1 2 80
    4 = l-r-b-y-↓ : 200 100 2 800 80
    5 = l-r-a-b-x-y : 200 100 1 2 400 800
    */

    switch(gs().resetHotKey)
    {
        case 0:
            ini.SetString("NDS-BOOTSTRAP", "HOTKEY", "284");
            break;
        case 1:
            ini.SetString("NDS-BOOTSTRAP", "HOTKEY", "308");
            break;
        case 2:
            ini.SetString("NDS-BOOTSTRAP", "HOTKEY", "304");
            break;
        case 3:
            ini.SetString("NDS-BOOTSTRAP", "HOTKEY", "383");
            break;
        case 4:
            ini.SetString("NDS-BOOTSTRAP", "HOTKEY", "B82");
            break;
        case 5:
            ini.SetString("NDS-BOOTSTRAP", "HOTKEY", "F03");
            break;
        case 6:
            if(hotkeyCheck){
                ini.SetString("NDS-BOOTSTRAP", "HOTKEY", externalHotkey.c_str());
            }
            break;
        default:
            break;
    }
    switch(gs().languageOverride)
    {
        case 0:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "-1");
            break;
        case 1:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "0");
            break;
        case 2:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "1");
            break;
        case 3:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "2");
            break;
        case 4:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "3");
            break;
        case 5:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "4");
            break;
        case 6:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "5");
            break;
        case 7:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "6");
            break;
        case 8:
            ini.SetString("NDS-BOOTSTRAP", "LANGUAGE", "7");
            break;
    }

    if (_romInfo.isDSiWare()) {
        ini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 1);
    } else if (_romInfo.saveInfo().isDsMode()) {
        ini.SetString("NDS-BOOTSTRAP", "DSI_MODE", "0");
    }

    if(_romInfo.saveInfo().isBoostCpu()) {
        ini.SetString("NDS-BOOTSTRAP", "BOOST_CPU", "1");
    }

    u8 cardReadDmaSetting = _romInfo.saveInfo().getCardReadDma();
    if (cardReadDmaSetting == 2) {
        cardReadDmaSetting = gs().cardReadDma;
    }
    if (cardReadDmaSetting == 1) {
        ini.SetInt("NDS-BOOTSTRAP", "CARD_READ_DMA", 1);
    } else if (cardReadDmaSetting == 2 || (cardReadDmaSetting == 0 && _romInfo.saveInfo().getCardReadDma() != 2)) {
        ini.SetInt("NDS-BOOTSTRAP", "CARD_READ_DMA", 0);
    }
    
    if(gs().phatCol && isDSiMode()) {
        ini.SetString("NDS-BOOTSTRAP", "PHAT_COLORS", "1");
    }

    if (access((mBootstrapRoot + "_nds/debug.txt").c_str(), F_OK) == 0) {
        ini.SetString("NDS-BOOTSTRAP", "LOGGING", "1");
        ini.SetString("NDS-BOOTSTRAP", "DEBUG", "1");
    }

    ini.SaveIniFile(bootstrapIniPath);
    debugOverlay::updateDonorStatus(ini, !homebrewLaunch);

    return true;
}

bool NdsBootstrapLauncher::is3DS(void) {
    if (!isDSiMode()) {
        return false;
    }

    fifoSendValue32(FIFO_USER_01, MENU_MSG_IS_3DS);

    fifoWaitValue32(FIFO_USER_01);

    int result = fifoGetValue32(FIFO_USER_01);

    return result != 0;
}

bool launchHbStrap(const std::string& argPath){
    progressWnd().setPercent(100);
    std::string ndsHbBootstrapPath = fsManager().resolveSystemPath("/_nds/nds-bootstrap-hb-release.nds");
    std::vector<const char*> argv;
    argv.push_back(ndsHbBootstrapPath.c_str());
    if (!argPath.empty()) {
        argv.push_back(argPath.c_str());
    }
    progressWnd().hide();
    eRunNdsRetCode rc = runNdsFile(argv[0], argv.size(), &argv[0]);
    if (rc == RUN_NDS_OK) return true;
    return false;
}

bool NdsBootstrapLauncher::launchRom(std::string romPath, std::string savePath, u32 flags,
                                     u32 cheatOffset, u32 cheatSize, bool hb) {
    return launchRomInternal(romPath, savePath, flags, cheatOffset, cheatSize, hb, "");
}

bool NdsBootstrapLauncher::launchPlugin(std::string romPath, const std::string& argumentPath,
                                        bool useArgv, bool useHbArgv) {
    const std::string argument = useArgv ? argumentPath : "";
    const std::string homebrewArg = useHbArgv ? argumentPath : "";
    return launchRomInternal(romPath, argument, 0, 0, 0, true, homebrewArg);
}

bool NdsBootstrapLauncher::launchRomInternal(std::string romPath, std::string savePath, u32 flags,
                                             u32 cheatOffset, u32 cheatSize, bool hb,
                                             const std::string& homebrewArg) {
    mBootstrapRoot = fsManager().getFSRoot();
    std::string ndsBootstrapPath = mBootstrapRoot + "_nds/nds-bootstrap-release.nds";
    std::string ndsBootstrapPathNightly = mBootstrapRoot + "_nds/nds-bootstrap-nightly.nds";
    std::string ndsHbBootstrapPath = mBootstrapRoot + "_nds/nds-bootstrap-hb-release.nds";
    std::string ndsBootstrapCheck = mBootstrapRoot + "_nds/pagefile.sys";
    bool useNightly = false;
    bool hbStrap = hb;

    //check if rom is homebrew
    if(hbStrap){
        mRomPath = romPath;
        // For hb-bootstrap launches, savePath is reused as an optional argv payload.
        // Normal homebrew launches pass an empty string; plugin launches pass the selected file.
        mSavePath = savePath;
        if(access(ndsHbBootstrapPath.c_str(), F_OK) != 0){
            progressWnd().hide();
            printLoaderNotFound(ndsHbBootstrapPath);
            return false;
        }
        progressWnd().setTipText("Initializing nds-bootstrap...");
        progressWnd().show();
        progressWnd().setPercent(0);
        //Clean up old INI
        const std::string bootstrapIniPath = mBootstrapRoot + "_nds/nds-bootstrap.ini";
        if (access(bootstrapIniPath.c_str(), F_OK) == 0) {
            remove(bootstrapIniPath.c_str());
        }
        // Setup nds-bootstrap INI parameters
        if (!prepareIni(false, homebrewArg, true)) return false;
        progressWnd().setPercent(25);
        return launchHbStrap(savePath);
    }

    _romInfo.MayBeDSRom(romPath);
    bool dsiWare = _romInfo.isDSiWare();
    u32 dsiWareTwlRomSize = 0;
    u32 dsiWarePublicSaveSize = 0;
    u32 dsiWarePrivateSaveSize = 0;

    // DSiWare saves and bootstrap paths are supported from the DSi SD card in DSi mode.
    if (dsiWare && (!isDSiMode() || romPath.compare(0, 4, "sd:/") != 0)) {
        printError("DSiWare launch requires a ROM on the DSi SD card and DSi mode.");
        return false;
    }

    if (dsiWare) {
        // Prefer the DSi SD bootstrap and keep its INI and working files beside it.
        mBootstrapRoot = "sd:/";
        ndsBootstrapCheck = mBootstrapRoot + "_nds/pagefile.sys";
        const std::string sdBootstrapPath =
                mBootstrapRoot + "_nds/nds-bootstrap-release.nds";
        const std::string sdBootstrapPathNightly =
                mBootstrapRoot + "_nds/nds-bootstrap-nightly.nds";
        if (access(sdBootstrapPath.c_str(), F_OK) == 0) {
            ndsBootstrapPath = sdBootstrapPath;
        }
        if (access(sdBootstrapPathNightly.c_str(), F_OK) == 0) {
            ndsBootstrapPathNightly = sdBootstrapPathNightly;
        }
    }

    bool useUnlaunch = false;
    if (dsiWare) {
        if (!getDsiWareHeaderInfo(romPath, dsiWareTwlRomSize, dsiWarePublicSaveSize,
                                  dsiWarePrivateSaveSize)) {
            printError("Could not read the DSiWare ROM header.");
            return false;
        }

        const u8 booterOverride = _romInfo.saveInfo().getDsiWareBooter();
        const bool requestedUnlaunch =
                booterOverride == SAVE_INFO_EX_DSIWARE_BOOTER_UNLAUNCH ||
                ((booterOverride == SAVE_INFO_EX_DSIWARE_BOOTER_GLOBAL ||
                  booterOverride > SAVE_INFO_EX_DSIWARE_BOOTER_UNLAUNCH) &&
                 gs().dsiWareUnlaunch);
        const bool unlaunchCanBootTitle =
                dsiWareTwlRomSize < 0x04000000 && *(vu32*)0x02FFFDF0 != 0;
        useUnlaunch = requestedUnlaunch && unlaunchCanBootTitle && !is3DS();

        if (useUnlaunch) {
            mRomPath = romPath;
            mSavePath = savePath;
            mDsiWarePubPath = replaceSaveExtension(mRomPath, ".pub");
            mDsiWarePrvPath = replaceSaveExtension(mRomPath, ".prv");
            const std::string configuredPubPath = replaceSaveExtension(mSavePath, ".pub");
            const std::string configuredPrvPath = replaceSaveExtension(mSavePath, ".prv");

            if (!moveDsiWareSaveBesideRom(configuredPubPath, mDsiWarePubPath) ||
                !moveDsiWareSaveBesideRom(configuredPrvPath, mDsiWarePrvPath)) {
                printError("Could not move the DSiWare save files beside the ROM for Unlaunch.");
                return false;
            }

            progressWnd().setTipText("Preparing DSiWare saves...");
            progressWnd().show();
            progressWnd().setPercent(0);
            if ((dsiWarePublicSaveSize > 0 &&
                 !ensureDsiWareSave(mDsiWarePubPath, dsiWarePublicSaveSize)) ||
                (dsiWarePrivateSaveSize > 0 &&
                 !ensureDsiWareSave(mDsiWarePrvPath, dsiWarePrivateSaveSize))) {
                progressWnd().hide();
                printError("Could not create the DSiWare save files on the SD card.");
                return false;
            }

            progressWnd().hide();
            if (!bootDsiWareWithUnlaunch(mRomPath)) {
                printError("Could not prepare the Unlaunch DSiWare boot path.");
                return false;
            }
        }
    }

    //has the user used nds-bootstrap before?
    if(access(ndsBootstrapCheck.c_str(), F_OK) != 0){
        akui::messageBox(NULL, LANG("nds bootstrap", "firsttimetitle"), LANG("nds bootstrap", "firsttime"), MB_OK);
    }

    progressWnd().setTipText("Initializing nds-bootstrap...");
    progressWnd().show();
    progressWnd().setPercent(0);

    //Check which nds-bootstrap version has been selected
    if((gs().nightly && _romInfo.saveInfo().getNightly() == 2) || _romInfo.saveInfo().getNightly() == 1){
        if(access(ndsBootstrapPathNightly.c_str(), F_OK) != 0){
            progressWnd().hide();
            printLoaderNotFound(ndsBootstrapPathNightly);
            return false;
        }
        else{
            useNightly = true;
        }
    }
    else{
        if(access(ndsBootstrapPath.c_str(), F_OK) != 0){
            progressWnd().hide();
            printLoaderNotFound(ndsBootstrapPath);
            return false;
        }
        else{
            useNightly = false;
        }
    }

    std::vector<const char*> argv;

    mRomPath = romPath;
    mSavePath = savePath;
    mFlags = flags;

    if (dsiWare) {
        const std::string romPubPath = replaceSaveExtension(mRomPath, ".pub");
        const std::string romPrvPath = replaceSaveExtension(mRomPath, ".prv");
        const std::string configuredPubPath = replaceSaveExtension(mSavePath, ".pub");
        const std::string configuredPrvPath = replaceSaveExtension(mSavePath, ".prv");
        mDsiWarePubPath = access(romPubPath.c_str(), F_OK) == 0 ? romPubPath : configuredPubPath;
        mDsiWarePrvPath = access(romPrvPath.c_str(), F_OK) == 0 ? romPrvPath : configuredPrvPath;

        if ((dsiWarePublicSaveSize > 0 &&
             !ensureDsiWareSave(mDsiWarePubPath, dsiWarePublicSaveSize)) ||
            (dsiWarePrivateSaveSize > 0 &&
             !ensureDsiWareSave(mDsiWarePrvPath, dsiWarePrivateSaveSize))) {
            progressWnd().hide();
            printError("Could not create the DSiWare save files on the SD card.");
            return false;
        }

        // The banner save is cosmetic: a failure here never blocks the launch.
        mBannerSavePath.clear();
        const std::string bannerSavePath = dsiBannerSavePath(mRomPath);
        if (_romInfo.hasBannerSave() && dsiEnsureBannerSave(bannerSavePath))
            mBannerSavePath = bannerSavePath;
    }

    // Create the nds-bootstrap directory if it doesn't exist
    const std::string bootstrapDirectory = mBootstrapRoot + "_nds/nds-bootstrap/";
    const std::string ndsDirectory = mBootstrapRoot + "_nds/";
    if (access(ndsDirectory.c_str(), F_OK) != 0) {
        mkdir(ndsDirectory.c_str(), 0777);
    }
    if (access(bootstrapDirectory.c_str(), F_OK) != 0) {
        mkdir(bootstrapDirectory.c_str(), 0777);
    }
    progressWnd().setPercent(25);

    // Setup argv to launch nds-bootstrap                             
    if(!useNightly){
        argv.push_back(ndsBootstrapPath.c_str());
    }
    else{
        argv.push_back(ndsBootstrapPathNightly.c_str());
    }

    progressWnd().setTipText("Initializing nds-bootstrap...");
    progressWnd().setPercent(50);

    //Clean up old INI
    const std::string bootstrapIniPath = mBootstrapRoot + "_nds/nds-bootstrap.ini";
    if (access(bootstrapIniPath.c_str(), F_OK) == 0) {
        remove(bootstrapIniPath.c_str());
    }

    // Setup nds-bootstrap INI parameters
    if (!prepareIni(false)) return false;
    progressWnd().setPercent(75);

    // Prepare cheat codes if enabled
    // Remove cheat bin if exists to start clean
    const std::string cheatDataPath = mBootstrapRoot + "_nds/nds-bootstrap/cheatData.bin";
    if (access(cheatDataPath.c_str(), F_OK) == 0) {
        remove(cheatDataPath.c_str());
    }
    if (!_romInfo.saveInfo().getCheat() || !gs().cheats) {
        progressWnd().setTipText("Booting game...");
        progressWnd().setPercent(100);
    }
    else {
        progressWnd().setTipText("Loading usrcheat.dat...");
        progressWnd().setPercent(90);
        if (flags & PATCH_CHEATS) {
            if (!prepareCheats()) {
                return false;
            }
        }
        progressWnd().setTipText("Booting game...");
        progressWnd().setPercent(100);
    }

    // enable slot2 ram access if available
    if(!isDSiMode()){
        slot2RamAccess();
    }

    // Launch
    eRunNdsRetCode rc = runNdsFile(argv[0], argv.size(), &argv[0]);
    if (rc == RUN_NDS_OK) return true;
    return false; 
}
