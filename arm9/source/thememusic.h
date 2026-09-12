/*
    thememusic.h
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds.h>
#include <stdio.h>

class cThemeMusic {
  public:
    cThemeMusic();

    bool start();
    void stop();
    void update();

    u32 fillStream(u32 length, void* destination);

  private:
    bool parseWave();
    bool readLooping(u8* destination, u32 length);
    bool queueBytes(u32 length);

    FILE* _file;
    u8* _ring;
    volatile u32 _readTotal;
    volatile u32 _writeTotal;
    u32 _dataStart;
    u32 _dataLength;
    u32 _fileDataOffset;
    u32 _frameBytes;
    u32 _sampleRate;
    u32 _format;
    bool _maxmodInitialized;
    bool _streamOpen;
    volatile bool _playing;
};

cThemeMusic& themeMusic();
