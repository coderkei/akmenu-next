/*
    animatedicon.cpp
    Copyright (C) 2026 Augusto Daniele

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "animatedicon.h"
#include <nds/bios.h>
#include <cstdio>

u16 dsiReadLe16(const void* p) {
    const u8* bytes = static_cast<const u8*>(p);
    return bytes[0] | (bytes[1] << 8);
}

u32 dsiAnimLength(const tDsiIconAnim& anim) {
    u32 length = 0;
    for (int i = 0; i < 64; ++i) {
        const u8 duration = dsiTokenDuration(anim.sequence[i]);
        if (!duration) break;
        length += duration;
    }
    return length;
}

bool dsiAnimLoops(const tDsiIconAnim& anim) {
    for (int i = 0; i < 64; ++i) {
        if (!dsiTokenDuration(anim.sequence[i])) return dsiTokenBitmap(anim.sequence[i]) != 1;
    }
    return true;
}

u16 dsiAnimTokenAt(const tDsiIconAnim& anim, u32 tick) {
    const u32 length = dsiAnimLength(anim);
    if (!length) return 0;
    if (tick >= length && !dsiAnimLoops(anim)) tick = length - 1;
    u32 position = tick % length;
    for (int i = 0; i < 64; ++i) {
        const u8 duration = dsiTokenDuration(anim.sequence[i]);
        if (position < duration) return anim.sequence[i];
        position -= duration;
    }
    return 0;
}

bool dsiAnimValid(const tDsiIconAnim& anim, u16 storedCrc) {
    if (!dsiTokenDuration(anim.sequence[0])) return false;
    return swiCRC16(0xFFFF, const_cast<tDsiIconAnim*>(&anim), sizeof(anim)) == storedCrc;
}

bool dsiBannerSaveHeaderValid(const u8* header, u16& storedCrc) {
    if (dsiReadLe16(header) != DSI_BANNER_VERSION_ANIMATED) return false;
    storedCrc = dsiReadLe16(header + DSI_BANNER_ANIM_CRC_OFFSET);
    return true;
}

bool dsiEnsureBannerSave(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (f) {
        const bool full = fseek(f, 0, SEEK_END) == 0 && ftell(f) >= (long)DSI_BANNER_SAVE_SIZE;
        fclose(f);
        if (full) return true;
    }

    f = fopen(path.c_str(), "wb");
    if (!f) return false;
    u8 chunk[0x200] = {DSI_BANNER_VERSION_ANIMATED & 0xFF, DSI_BANNER_VERSION_ANIMATED >> 8};
    bool ok = true;
    for (u32 written = 0; ok && written < DSI_BANNER_SAVE_SIZE; written += sizeof(chunk)) {
        ok = fwrite(chunk, 1, sizeof(chunk), f) == sizeof(chunk);
        chunk[0] = chunk[1] = 0;
    }
    ok = fclose(f) == 0 && ok;
    if (!ok) remove(path.c_str());
    return ok;
}

std::string dsiBannerSavePath(const std::string& romPath) {
    const size_t slash = romPath.find_last_of("/\\");
    const size_t dot = romPath.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
        return romPath + ".bnr";
    return romPath.substr(0, dot) + ".bnr";
}
