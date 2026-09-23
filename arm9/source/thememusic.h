/*
    thememusic.h
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds.h>
#include <stdio.h>

class cMusicSource;

class cThemeMusic {
  public:
    cThemeMusic();

    bool start();
    void stop();
    void update();

    u32 fillStream(u32 length, void* destination);

  private:
    bool queueBytes(u32 length);

    cMusicSource* _source;
    u8* _ring;
    volatile u32 _readTotal;
    volatile u32 _writeTotal;
    u32 _frameBytes;
    u32 _sampleRate;
    u32 _format;
    bool _maxmodInitialized;
    bool _streamOpen;
    bool _sourceFailed;
    volatile bool _playing;
};

cThemeMusic& themeMusic();
