/*
    Copyright (C) 2024 lifehackerhansol

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <nds/ndstypes.h>

#include "../cheatwnd.h"
#include "../dsrom.h"
#include "../flags.h"
#include "../mainlist.h"
#include "../systemfilenames.h"
#include "../ui/progresswnd.h"
#include "ILauncher.h"
#include "TopToyLauncher.h"
#include "libfat_ext/fat_ext.h"

#ifdef __TTLAUNCHER_DSTT__
#include "ttio/scdssdhc.h"
extern u32 SCDS_isSDHC;
#endif

static void resetAndLoop() {
    DC_FlushAll();
    DC_InvalidateAll();

    fifoSendValue32(FIFO_USER_01, MENU_MSG_ARM7_REBOOT_TT);
    *((vu32*)0x02FFFE04) = 0;

    // Interrupt
    REG_IME = 0;
    REG_IE = 0;
    REG_IF = ~0;

    // wait for arm7
    while (*((vu32*)0x02FFFE04) == 0)
        ;
    swiSoftReset();
}

typedef struct {
    char TTARMagic[4];  // usually "TTAR"
    char gameCode[4];   // gameCode of title as seen in usrcheat.dat
    u32 crc32;          // crc32 of title header as seen in usrcheat.dat
    u32 unk1;           // TODO: what does this mean?
    u32 cheatOffset;    // relative from 0x1100 of file
    u32 cheatSize;      // size in bytes
    u8 reserved[235];
} PACKED TTARHeader;

bool TopToyLauncher::prepareCheats() {
    u32 gameCode, crc32;

    if (cCheatWnd::romData(mRomPath, gameCode, crc32)) {
        FILE* cheatDb = fopen(SFN_CHEATS, "rb");
        if (!cheatDb) goto cheat_failed;
        long cheatOffset;
        size_t cheatSize;
        if (cCheatWnd::searchCheatData(cheatDb, gameCode, crc32, cheatOffset, cheatSize)) {
            // Read cheat codes
            cCheatWnd chtwnd((256) / 2, (192) / 2, 100, 100, NULL, mRomPath);
            chtwnd.parse(mRomPath);
            std::vector<u32> cheats(chtwnd.getCheats());

            // YSMENU.ARP file creation
            ALIGN(4) TTARHeader arHeader = {};
            arHeader.TTARMagic[0] = 'T';
            arHeader.TTARMagic[1] = 'T';
            arHeader.TTARMagic[2] = 'A';
            arHeader.TTARMagic[3] = 'R';
            memcpy(&arHeader.gameCode, &gameCode, 4);
            arHeader.crc32 = crc32;
            // We will write the cheat codes directly to 0x1100.
            arHeader.cheatOffset = 0;
            arHeader.cheatSize = cheats.size() << 2;
            FILE* cheatFile = fopen("/YSMENU.ARP", "wb");
            fseek(cheatFile, 0, SEEK_SET);
            // memdump. Actually just expanding the file seems to crash, but this works totally
            // fine...
            fwrite((void*)0x02400000, 4, 0x1100 >> 2, cheatFile);
            fflush(cheatFile);
            fseek(cheatFile, 0, SEEK_SET);
            fwrite(&arHeader, 1, sizeof(TTARHeader), cheatFile);
            fseek(cheatFile, 0x1100, SEEK_SET);
            fwrite(cheats.data(), 4, cheats.size(), cheatFile);
            fflush(cheatFile);
            fclose(cheatFile);
        } else {
            fclose(cheatDb);
            goto cheat_failed;
        }
        fclose(cheatDb);
    }

    return true;

cheat_failed:
    // Remove cheat bin if exists
    if (access("/YSMENU.ARP", F_OK) == 0) {
        remove("/YSMENU.ARP");
    }

    return false;
}

// TODO: what are the unknowns?
typedef struct {
    char TTSYSMagic[4];
    u32 unk1;
    u32 unk2;  // always set to 1?
    u32 softReset;
    u32 useCheats;
    u32 DMA;
    u8 reserved[232];
} PACKED TTSYSHeader;

// TTMENU.SYS file creation. Sets up parameters for ttpatch.dat
bool TopToyLauncher::prepareTTSYS(void) {
    char fname[0x1004] = {};
    TTSYSHeader* ttsys_header = (TTSYSHeader*)malloc(sizeof(TTSYSHeader));
    ttsys_header->TTSYSMagic[0] = 't';
    ttsys_header->TTSYSMagic[1] = 't';
    ttsys_header->TTSYSMagic[2] = 'd';
    ttsys_header->TTSYSMagic[3] = 's';
    ttsys_header->unk1 = 0;
    ttsys_header->unk2 = 1;
    ttsys_header->softReset = mFlags & PATCH_SOFT_RESET ? 1 : 0;
    ttsys_header->useCheats = mFlags & PATCH_CHEATS ? 1 : 0;
    ttsys_header->DMA = mFlags & PATCH_DMA ? 1 : 0;

    FILE* TTSYSFile = fopen("fat:/TTMENU.SYS", "rb+");
    fseek(TTSYSFile, 0, SEEK_SET);
    fwrite(ttsys_header, sizeof(TTSYSHeader), 1, TTSYSFile);
    free(ttsys_header);
    fatGetAliasPath("fat:/", mRomPath.c_str(), fname);
    fseek(TTSYSFile, 0x100, SEEK_SET);
    fwrite(fname + 4, 1, 0x1000, TTSYSFile);
    fatGetAliasPath("fat:/", mSavePath.c_str(), fname);
    fseek(TTSYSFile, 0x1100, SEEK_SET);
    fwrite(fname + 4, 1, 0x1000, TTSYSFile);
    if (mFlags & PATCH_CHEATS) {
        fseek(TTSYSFile, 0x2100, SEEK_SET);
        fwrite("/YSMENU.ARP", 1, 0x12, TTSYSFile);
    }
    fflush(TTSYSFile);
    fclose(TTSYSFile);

    return true;
}

bool TopToyLauncher::launchRom(std::string romPath, std::string savePath, u32 flags,
                               u32 cheatOffset, u32 cheatSize, bool hb) {
    std::string loaderPath =
#ifdef __TTLAUNCHER_M3__
            "fat:/TTMenu/m3patch.dat"
#else   // __TTLAUNCHER_M3__
            "fat:/TTMenu/ttpatch.dat"
#endif  // __TTLAUNCHER_M3__
            ;

    if (access(loaderPath.c_str(), F_OK) != 0) {
        printLoaderNotFound(loaderPath);
        return false;
    }

    mRomPath = romPath;
    mSavePath = savePath;
    mFlags = flags;

    // Create TTMENU.SYS if it don't exist
    if (access("fat:/TTMENU.SYS", F_OK) != 0) {
        progressWnd().setTipText("Generating TTMENU.SYS...");
        progressWnd().show();
        progressWnd().setPercent(0);
        FILE* TTSYSFile = fopen("fat:/TTMENU.SYS", "wb");
        fseek(TTSYSFile, 0, SEEK_SET);
        // memdump. Actually just expanding the file seems to crash, but this works totally fine...
        fwrite((void*)0x02400000, 1, 0x400000, TTSYSFile);
        fflush(TTSYSFile);
        fclose(TTSYSFile);
        progressWnd().setPercent(100);
        progressWnd().hide();
    }

    // Prepare cheat codes if enabled
    if (flags & PATCH_CHEATS) {
        if (!prepareCheats()) return false;
    }

    // Setup TTMenu system parameters
    if (!prepareTTSYS()) return false;

    FILE* loader = fopen(loaderPath.c_str(), "rb");
    tNDSHeader* header = (tNDSHeader*)malloc(sizeof(tNDSHeader));
    u8* loader_arm7 = NULL;
    fseek(loader, 0, SEEK_SET);
    fread(header, 1, sizeof(tNDSHeader), loader);
    fseek(loader, header->arm9romOffset, SEEK_SET);
    fread((void*)header->arm9destination, 1, header->arm9binarySize, loader);
    fseek(loader, header->arm7romOffset, SEEK_SET);
    loader_arm7 = (u8*)memalign(32, header->arm7binarySize);
    fread(loader_arm7, 1, header->arm7binarySize, loader);
    fclose(loader);
    memcpy((void*)__NDSHeader, header, sizeof(tNDSHeader));
    free(header);
    *(u32*)0x02FFFE00 = (u32)loader_arm7;

    // patch a loop in ARM9
    // not sure why it's there. Some sort of obfuscation mechanism?
    if (*((vu32*)0x023200EC) == 0xEAFFFFFE)  // b #0; bad
        *((vu32*)0x023200EC) = 0xE3A00000;   // mov r0, #0

    // ttpatch checks this for some reason
    *((vu32*)0x02FFFC20) = 0x5555AAAA;

#ifdef __TTLAUNCHER_DSTT__
    // set SD/SDHC flag
    // Needs a SDIO reinitialization to retrieve HCS bit
    if (!SCDS_SDInitialize()) return false;

    *((vu32*)0x02FFFC24) = SCDS_isSDHC ? ~0 : 0;
#endif

    // this int seems to be a flag to reinitialize the SD card in ttpatch
    // if this is *not* -1, ttpatch sends an SDIO CMD12 (STOP_TRANSMISSION)
    // other frontends set this to -1 by default, so let's do it too
    *((vu32*)0x02FFFC28) = ~0;

    resetAndLoop();

    free(loader_arm7);

    return false;
}
