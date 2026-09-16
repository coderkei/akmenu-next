/*
    wavsource.h
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds/ndstypes.h>
#include <stdio.h>

#include "musicsource.h"

// Plays a 16-bit PCM RIFF/WAVE file (mono or stereo), repeating the whole file.
class cWavSource : public cMusicSource {
  public:
    cWavSource();
    ~cWavSource();

    bool open(const char* path);
    u32 sampleRate() const { return _sampleRate; }
    u32 channels() const { return _channels; }
    bool read(s16* dst, u32 frames);
    const char* error() const { return _error; }

  private:
    cWavSource(const cWavSource&);
    cWavSource& operator=(const cWavSource&);

    bool fail(const char* reason);
    bool parse();
    void close();

    FILE* _file;
    const char* _error;
    u32 _sampleRate;
    u32 _channels;
    u32 _dataStart;
    u32 _dataLength;
    u32 _dataOffset;
};
