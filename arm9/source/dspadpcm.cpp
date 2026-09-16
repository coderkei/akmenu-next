/*
    dspadpcm.cpp
    Copyright (C) 2026 Augusto Daniele

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "dspadpcm.h"

void dspAdpcmDecode(const u8* frame, sDspAdpcmState& state, s16* out, u32 first, u32 count) {
    u32 pair = frame[0] >> 4;
    if (pair > 7) pair = 7;  // corrupt header: stay inside the coefficient table
    // 32-bit arithmetic is enough: the nibble term is at most (8 << 15) * 2048 and
    // each coefficient term at most kDspAdpcmMaxCoef * 32768, so the sum stays
    // below 2^31. Callers must keep coefficients within that limit.
    const s32 scale = (s32)1 << (frame[0] & 0x0F);
    const s32 coef1 = state.coefs[pair * 2];
    const s32 coef2 = state.coefs[pair * 2 + 1];
    s32 hist1 = state.hist1;
    s32 hist2 = state.hist2;

    for (u32 i = first; i < first + count; i++) {
        const u8 packed = frame[1 + i / 2];
        s32 nibble = (i & 1) ? (packed & 0x0F) : (packed >> 4);
        if (nibble >= 8) nibble -= 16;

        s32 sample = (nibble * scale * 2048 + 1024 + coef1 * hist1 + coef2 * hist2) >> 11;
        if (sample > 32767) sample = 32767;
        if (sample < -32768) sample = -32768;

        *out++ = (s16)sample;
        hist2 = hist1;
        hist1 = sample;
    }

    state.hist1 = (s16)hist1;
    state.hist2 = (s16)hist2;
}
