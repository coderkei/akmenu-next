/*
    dspadpcm.h
    Copyright (C) 2026 Augusto Daniele

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds/ndstypes.h>

// DSP-ADPCM decoder (used by GameCube, Wii and 3DS audio).
// A frame is 8 bytes: a header byte (high nibble = coefficient pair index,
// low nibble = scale exponent) followed by 14 signed 4-bit samples, high
// nibble first. References: vgmstream src/coding/ngc_dsp_decoder.c,
// 3dbrew "BCSTM" (DSP ADPCM info).

const u32 kDspAdpcmFrameBytes = 8;
const u32 kDspAdpcmFrameSamples = 14;

// Coefficient magnitude limit. Real files stay far below it (the largest seen in
// 3DS theme music is about 4000), and it keeps the decoder's arithmetic inside
// 32 bits: 2 * 16384 * 32768 + (8 << 15) * 2048 + 1024 < 2^31.
const s16 kDspAdpcmMaxCoef = 16384;

struct sDspAdpcmState {
    s16 coefs[16];  // 8 pairs of 5.11 coefficients, each within kDspAdpcmMaxCoef
    s16 hist1;      // previous decoded sample
    s16 hist2;      // sample before hist1
};

// Decodes samples [first, first + count) of one frame into out[0 .. count).
// Requires first + count <= 14. The history in `state` must be the history
// just before sample `first`; it is updated to the last decoded samples.
void dspAdpcmDecode(const u8* frame, sDspAdpcmState& state, s16* out, u32 first, u32 count);
