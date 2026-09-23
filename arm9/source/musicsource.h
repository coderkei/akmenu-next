/*
    musicsource.h
    Copyright (C) 2026 Augusto Daniele

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds/ndstypes.h>

// An endlessly looping source of 16-bit PCM for theme music.
class cMusicSource {
  public:
    virtual ~cMusicSource() {}

    // Opens and validates `path`. On failure returns false; error() says why.
    virtual bool open(const char* path) = 0;
    virtual u32 sampleRate() const = 0;
    // 1 (mono) or 2 (stereo).
    virtual u32 channels() const = 0;
    // Writes `frames` frames of interleaved native-endian 16-bit samples to
    // `dst`, wrapping at the loop end. Returns false on an I/O or data error.
    virtual bool read(s16* dst, u32 frames) = 0;
    // Short static description of the last failure, or "".
    virtual const char* error() const = 0;
};
