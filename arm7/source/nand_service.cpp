/*
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later

    NAND IPC service adapted from DS-Homebrew/TWiLightMenu's
    universal/sdmmc/arm7/source/my_sdmmc.c; key3 routine adapted from
    title/arm7/source/main.c:
    https://github.com/DS-Homebrew/TWiLightMenu
*/

#include "nand_service.h"

#include <nds.h>
#include <nds/arm7/aes.h>
#include <nds/arm7/sdmmc.h>

#include <string.h>

namespace {
const u32 kSharedCommandBase = 0x02FFFA00;
const u32 kHaveSdCommand = 0x56484453;
const u32 kNandStartCommand = 0x5453414E;
const u32 kNandReadCommand = 0x4452414E;
const u32 kGetConsoleIdCommand = 0x44494347;
const u32 kConsoleIdFailed = 0x52454347; // 'GCER'

static void nandSyncHandler(void) {
    volatile u32* const shared = (volatile u32*)kSharedCommandBase;
    const u32 command = shared[3];
    if (command != kHaveSdCommand && command != kNandStartCommand &&
        command != kNandReadCommand) {
        return;
    }

    const int oldIME = enterCriticalSection();
    u32 result = 0;

    switch (command) {
        case kHaveSdCommand:
            result = (sdmmc_read16(REG_SDSTATUS0) & BIT(5)) != 0;
            break;
        case kNandStartCommand:
            if (sdmmc_read16(REG_SDSTATUS0) == 0) {
                result = 1;
            } else {
                sdmmc_controller_init(false);
                result = sdmmc_nand_init();
            }
            break;
        case kNandReadCommand:
            result = sdmmc_nand_readsectors(shared[0], shared[1], (void*)shared[2]);
            break;
    }

    leaveCriticalSection(oldIME);
    shared[3] = result;
}

TWL_CODE static void setAesCtr(u32* ctr) {
    // REG_AES_IV is byte-addressed in legacy libnds; write all four words.
    vu32* const ivWords = (vu32*)REG_AES_IV;
    for (int i = 0; i < 4; ++i) ivWords[i] = ctr[3 - i];
}

TWL_CODE static void aesKey3Encrypt(void* in, void* out, void* iv) {
    REG_AES_CNT = AES_CNT_MODE(2) |
                  AES_WRFIFO_FLUSH |
                  AES_RDFIFO_FLUSH |
                  AES_CNT_KEY_APPLY |
                  AES_CNT_KEYSLOT(3) |
                  AES_CNT_DMA_WRITE_SIZE(2) |
                  AES_CNT_DMA_READ_SIZE(1);

    if (iv != NULL) setAesCtr((u32*)iv);
    REG_AES_BLKCNT = 1 << 16;
    REG_AES_CNT |= AES_CNT_ENABLE;

    for (int i = 0; i < 16; i += 4) REG_AES_WRFIFO = *((u32*)((u8*)in + i));
    while (((REG_AES_CNT >> 5) & 0x1F) < 4) {}
    for (int i = 0; i < 16; i += 4) *((u32*)((u8*)out + i)) = REG_AES_RDFIFO;
}

TWL_CODE static bool getConsoleId(u32 bufferAddress) {
    // Match TWiLight Menu++'s vblank boundary workaround for 3DS mode.
    while (REG_VCOUNT != 191) {}
    while (REG_VCOUNT == 191) {}

    u32 base[4] = {0};
    u32 input[4] = {0};
    u32 iv[4] = {0};
    u8* scratch = (u8*)bufferAddress + 0x200;
    u8* output = (u8*)bufferAddress;

    aesKey3Encrypt(input, base, iv);
    for (int i = 0; i < 16; ++i) {
        bool found = false;
        for (int value = 0; value < 256; ++value) {
            AES_KEYSLOT3.normalkey[i] = value & 0xFF;
            aesKey3Encrypt(input, scratch, iv);
            if (memcmp(scratch, base, 16) == 0) {
                output[i] = value;
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    return true;
}
}  // namespace

void initializeNandService(void) {
    if (!isDSiMode()) return;
    irqSet(IRQ_IPC_SYNC, nandSyncHandler);
    irqEnable(IRQ_IPC_SYNC);
}

void pollNandConsoleIdRequest(void) {
    if (!isDSiMode()) return;

    volatile u32* const command = (volatile u32*)0x02FFFD0C;
    if (*command == kGetConsoleIdCommand) {
        *command = getConsoleId(*(vu32*)0x02FFFD08) ? 0 : kConsoleIdFailed;
    }
}
