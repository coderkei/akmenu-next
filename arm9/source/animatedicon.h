/*
    animatedicon.h
    Copyright (C) 2026 Augusto Daniele

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds/ndstypes.h>
#include <string>

// DSi animated icon data, as stored in a version 0x0103 banner at 0x1240 and in a
// banner save (banner.sav, "<rom>.bnr" here) at 0x20.
// Layout per GBATEK "DS Cartridge Icon/Title".
struct tDsiIconAnim {
    u8 bitmaps[8][0x200];
    u16 palettes[8][16];
    u16 sequence[64];
};
static_assert(sizeof(tDsiIconAnim) == 0x1180, "DSi icon animation block must be 0x1180 bytes");

const u16 DSI_BANNER_VERSION_ANIMATED = 0x0103;
const u32 DSI_BANNER_ANIM_OFFSET = 0x1240;
const u32 DSI_BANNER_ANIM_CRC_OFFSET = 0x08;
const u32 DSI_BANNER_SAVE_HEADER_SIZE = 0x20;
const u32 DSI_BANNER_SAVE_SIZE = 0x4000;

// Sequence token: bit 15 vflip, bit 14 hflip, bits 13-11 palette, bits 10-8 bitmap,
// bits 7-0 duration in 60 Hz frames.
inline u8 dsiTokenDuration(u16 token) {
    return token & 0xFF;
}
inline u8 dsiTokenBitmap(u16 token) {
    return (token >> 8) & 7;
}
inline u8 dsiTokenPalette(u16 token) {
    return (token >> 11) & 7;
}
inline bool dsiTokenHFlip(u16 token) {
    return token & BIT(14);
}
inline bool dsiTokenVFlip(u16 token) {
    return token & BIT(15);
}

u16 dsiReadLe16(const void* p);

// Total duration of the sequence, which ends at the first zero-duration token or after 64.
u32 dsiAnimLength(const tDsiIconAnim& anim);

// False when the sequence ends with a stop token (zero duration, bitmap index 1): it plays
// once and holds its last frame. Any other end, or no end within 64 tokens, loops.
bool dsiAnimLoops(const tDsiIconAnim& anim);

// Token shown at `tick` (60 Hz) after the sequence started: looping, or holding the last frame
// for a stop sequence; 0 when there is nothing to play.
u16 dsiAnimTokenAt(const tDsiIconAnim& anim, u32 tick);

// True when the block matches its stored CRC16 and the first token has a duration.
bool dsiAnimValid(const tDsiIconAnim& anim, u16 storedCrc);

// Checks the 0x20-byte banner save header and returns the CRC it stores for the block.
bool dsiBannerSaveHeaderValid(const u8* header, u16& storedCrc);

// "<rom without extension>.bnr", the banner save path Pico-Loader also uses.
std::string dsiBannerSavePath(const std::string& romPath);

// Makes sure `path` holds a full-size banner save, creating it (version, then zeros) when it is
// missing or short. The CRC stays zero, so an untouched file is never shown as an icon.
// A failed write removes the partial file.
bool dsiEnsureBannerSave(const std::string& path);
