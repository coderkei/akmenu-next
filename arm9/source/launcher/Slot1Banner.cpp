/*
    Cartridge initialization adapted from TWiLightMenu's NitroHax-derived reader.
    Copyright (C) 2008 Michael "Chishm" Chisholm

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "Slot1Banner.h"

#include <algorithm>
#include <cstring>
#include <nds/card.h>
#include "../irqs.h"

namespace {
#include "Slot1Key1.h"

constexpr u32 transferFlags = CARD_ACTIVATE | CARD_nRESET;
constexpr u32 headerFlags = transferFlags | CARD_CLK_SLOW | CARD_BLK_SIZE(1) |
                            CARD_DELAY1(0x1FFF) | CARD_DELAY2(0x3F);

bool transfer(u32 flags, const u8 (&command)[8], u32* output = nullptr, size_t words = 0) {
    if (REG_SCFG_MC & SCFG_MC_EJECTED) return false;
    cardWriteCommand(command);
    REG_ROMCTRL = flags;
    const u32 start = cIRQ::vblankCount();
    do {
        if (REG_ROMCTRL & CARD_DATA_READY) {
            const u32 word = REG_CARD_DATA_RD;
            if (words) {
                *output++ = word;
                --words;
            }
        }
        if ((REG_SCFG_MC & SCFG_MC_EJECTED) || cIRQ::vblankCount() - start >= 15) {
            REG_ROMCTRL = 0;
            return false;
        }
    } while (REG_ROMCTRL & CARD_BUSY);
    return words == 0;
}

class Key1 {
public:
    void init(u32 gameCode) {
        memcpy(table, key1Seed, sizeof(table));
        u32 code[] = {gameCode, gameCode >> 1, gameCode << 1};
        for (int level = 0; level < 2; ++level) {
            encrypt(code + 1);
            encrypt(code);
            for (unsigned i = 0; i < 0x12; ++i) table[i] ^= __builtin_bswap32(code[i & 1]);
            u32 block[2] = {};
            for (unsigned i = 0; i < 0x412; i += 2) {
                encrypt(block);
                table[i] = block[1];
                table[i + 1] = block[0];
            }
        }
    }

    void encrypt(u32* block) const {
        u32 x = block[1], y = block[0];
        for (unsigned i = 0; i < 16; ++i) {
            const u32 z = table[i] ^ x;
            x = table[0x012 + (z >> 24)] + table[0x112 + ((z >> 16) & 0xFF)];
            x = (x ^ table[0x212 + ((z >> 8) & 0xFF)]) + table[0x312 + (z & 0xFF)];
            x ^= y;
            y = z;
        }
        block[0] = x ^ table[0x10];
        block[1] = y ^ table[0x11];
    }

private:
    u32 table[sizeof(key1Seed) / sizeof(u32)];
};

bool startRomReads(const tNDSHeader& header, bool normalChip) {
    static Key1 key;
    u32 gameCode;
    memcpy(&gameCode, header.gameCode, sizeof(gameCode));
    key.init(gameCode);

    const u32 modeFlags = transferFlags | (header.cardControl13 & CARD_CLK_SLOW);
    u32 keyFlags = modeFlags | ((header.cardControlBF & (CARD_CLK_SLOW | CARD_DELAY1(0x1FFF))) +
                                ((header.cardControlBF & CARD_DELAY2(0x3F)) >> 16));
    if (!normalChip) keyFlags |= CARD_SEC_LARGE;

    constexpr u32 iii = 4, jjj = 4, mmm = 4, nnn = 4, llll = 4;
    u32 counter = 4;
    const u8 activateKey1[8] = {0, static_cast<u8>(counter), static_cast<u8>(counter >> 8),
                                static_cast<u8>(counter >> 16), static_cast<u8>(jjj),
                                static_cast<u8>((iii << 4) | (jjj >> 8)), static_cast<u8>(iii >> 4),
                                CARD_CMD_ACTIVATE_BF};
    if (!transfer(modeFlags, activateKey1)) return false;

    const auto secureCommand = [&](u8 opcode, u32 block, u32 flags, unsigned reads) {
        const bool seed = opcode == CARD_CMD_ACTIVATE_SEC;
        const u32 i = seed ? mmm : iii;
        const u32 j = seed ? nnn : jjj;
        u8 command[8] = {
            static_cast<u8>(counter), static_cast<u8>(counter >> 8),
            static_cast<u8>((j << 4) | ((counter >> 16) & 0xF)), static_cast<u8>(j >> 4),
            static_cast<u8>(i), static_cast<u8>((block << 4) | (i >> 8)),
            static_cast<u8>(block >> 4), static_cast<u8>(opcode | (block >> 12))
        };
        ++counter;
        u32 encrypted[2];
        memcpy(encrypted, command, sizeof(encrypted));
        key.encrypt(encrypted);
        memcpy(command, encrypted, sizeof(command));
        if (normalChip) {
            if (!transfer(flags, command)) return false;
            TIMER_DATA(2) = -((header.readTimeout & 0x3FFF) + 3);
            TIMER_CR(2) = TIMER_ENABLE | TIMER_DIV_256;
            while (TIMER_DATA(2) != 0xFFFF) {}
            TIMER_CR(2) = 0;
        }
        const u32 size = opcode == CARD_CMD_SECURE_READ ? CARD_BLK_SIZE(normalChip ? 1 : 4) :
                         opcode == CARD_CMD_SECURE_CHIPID ? CARD_BLK_SIZE(7) : 0;
        for (unsigned read = 0; read < reads; ++read) {
            if (!transfer(flags | size, command)) return false;
        }
        return true;
    };

    if (!secureCommand(CARD_CMD_ACTIVATE_SEC, llll, keyFlags, 1)) return false;

    constexpr u8 seeds[] = {0xE8, 0x4D, 0x5A, 0xB1, 0x17, 0x8F, 0x99, 0xD5};
    REG_ROMCTRL = 0;
    REG_CARD_1B0 = seeds[header.deviceType & 7] | (nnn << 15) | (mmm << 27) | 0x6000;
    REG_CARD_1B4 = 0x879B9B05;
    REG_CARD_1B8 = mmm >> 5;
    REG_CARD_1BA = 0x5C;
    REG_ROMCTRL = CARD_nRESET | CARD_SEC_SEED | CARD_SEC_EN | CARD_SEC_DAT;

    keyFlags |= CARD_SEC_EN | CARD_SEC_DAT;
    if (!secureCommand(CARD_CMD_SECURE_CHIPID, llll, keyFlags, 1)) return false;
    const u32 secureFlags = transferFlags | CARD_SEC_EN | CARD_SEC_DAT |
                            (header.cardControlBF & (CARD_CLK_SLOW | CARD_DELAY1(0x1FFF) |
                                                      CARD_DELAY2(0x3F))) |
                            (normalChip ? 0 : CARD_SEC_LARGE);
    for (u32 block = 4; block < 8; ++block) {
        if (!secureCommand(CARD_CMD_SECURE_READ, block, secureFlags, normalChip ? 8 : 1))
            return false;
    }
    return secureCommand(CARD_CMD_DATA_MODE, llll, keyFlags, 1);
}

bool readBanner(tNDSBanner& banner, u16 (&title)[128], unsigned language) {
    REG_ROMCTRL = 0;
    REG_AUXSPICNT = 0;
    swiDelay(167550);
    REG_AUXSPICNT = CARD_ENABLE | CARD_IRQ;
    REG_ROMCTRL = CARD_nRESET | CARD_SEC_SEED;

    const u8 reset[8] = {0, 0, 0, 0, 0, 0, 0, CARD_CMD_DUMMY};
    if (!transfer(headerFlags, reset)) return false;

    const u8 readId[8] = {0, 0, 0, 0, 0, 0, 0, CARD_CMD_HEADER_CHIPID};
    u32 cardId;
    if (!transfer(transferFlags | CARD_CLK_SLOW | CARD_BLK_SIZE(7), readId, &cardId, 1) ||
        cardId == 0 || cardId == 0xFFFFFFFF) return false;

    static u32 block[0x200 / sizeof(u32)];
    const u8 readHeader[8] = {};
    if (!transfer(headerFlags, readHeader, block, 0x200 / sizeof(u32))) return false;
    tNDSHeader header;
    memcpy(&header, block, sizeof(header));
    if (header.headerCRC16 != swiCRC16(0xFFFF, block, 0x15E) ||
        header.bannerOffset < 0x8000 || header.deviceSize > 15) return false;

    const u64 capacity = 0x20000ull << header.deviceSize;
    if (header.bannerOffset > capacity - 0xA40 || header.bannerOffset > UINT32_MAX - 0xA40 ||
        !startRomReads(header, cardId & BIT(31)))
        return false;

    constexpr size_t maxSize = 0xA40;
    alignas(4) static u8 data[maxSize];
    const u32 flags = (header.cardControl13 & (CARD_CLK_SLOW | CARD_SEC_CMD | CARD_SEC_EN |
                                               CARD_SEC_DAT | CARD_DELAY1(0x1FFF) |
                                               CARD_DELAY2(0x3F))) |
                      transferFlags | CARD_BLK_SIZE(1);
    size_t bannerSize = 0x840;
    for (size_t copied = 0; copied < bannerSize;) {
        const u32 offset = header.bannerOffset + copied;
        const u32 address = offset & ~0x1FF;
        const u8 readData[8] = {0, 0, 0, static_cast<u8>(address),
                                static_cast<u8>(address >> 8), static_cast<u8>(address >> 16),
                                static_cast<u8>(address >> 24), CARD_CMD_DATA_READ};
        if (!transfer(flags, readData, block, 0x200 / sizeof(u32))) return false;
        const size_t skip = offset - address;
        if (copied == 0) {
            const u8 version = reinterpret_cast<const u8*>(block)[skip];
            if (version == 2) bannerSize = 0x940;
            if (version == 3) bannerSize = maxSize;
        }
        const size_t count = std::min<size_t>(0x200 - skip, bannerSize - copied);
        memcpy(data + copied, reinterpret_cast<const u8*>(block) + skip, count);
        copied += count;
    }

    memcpy(&banner, data, sizeof(banner));
    if ((banner.version != 1 && banner.version != 2 && banner.version != 3 &&
         banner.version != 0x0103) || banner.crc != swiCRC16(0xFFFF, data + 0x20, 0x820))
        return false;

    const unsigned titleCount = (bannerSize - 0x240) / 0x100;
    const unsigned index = language < titleCount ? language : 1;
    memcpy(title, data + 0x240 + index * sizeof(title), sizeof(title));
    if (!title[0]) memcpy(title, banner.titles[1], sizeof(title));
    title[127] = 0;
    return true;
}
}

bool readSlot1Banner(tNDSBanner& banner, u16 (&title)[128], unsigned language) {
    const bool oldOwner = sysGetCardOwner();
    sysSetCardOwner(BUS_OWNER_ARM9);
    if ((REG_SCFG_MC & SCFG_MC_PWR_MASK) == SCFG_MC_PWR_ON) {
        disableSlot1();
        for (int i = 0; i < 25; ++i) swiWaitForVBlank();
    }
    if (!(REG_SCFG_MC & SCFG_MC_EJECTED)) enableSlot1();
    for (int i = 0; i < 15; ++i) swiWaitForVBlank();

    const bool powered = (REG_SCFG_MC & SCFG_MC_PWR_MASK) == SCFG_MC_PWR_ON;
    const bool success = powered && readBanner(banner, title, language);
    if (powered) {
        REG_ROMCTRL = 0;
        REG_AUXSPICNT = 0;
        disableSlot1();
    }
    sysSetCardOwner(oldOwner);
    return success && !(REG_SCFG_MC & SCFG_MC_EJECTED);
}
