/*
    bcstmsource.h
    Copyright (C) 2026 Augusto Daniele

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds/ndstypes.h>
#include <stdio.h>

#include "dspadpcm.h"
#include "musicsource.h"

// Plays a DSP-ADPCM BCSTM (3DS stream) with its loop points.
class cBcstmSource : public cMusicSource {
  public:
    cBcstmSource();
    ~cBcstmSource();

    bool open(const char* path);
    u32 sampleRate() const { return _sampleRate; }
    u32 channels() const { return _channels; }
    bool read(s16* dst, u32 frames);
    const char* error() const { return _error; }

  private:
    cBcstmSource(const cBcstmSource&);
    cBcstmSource& operator=(const cBcstmSource&);

    bool fail(const char* reason);
    bool parse();
    bool parseInfo(const u8* info, u32 size, u32 dataOffset, u32 dataSize);
    bool loadBlock(u32 block);
    void rewind();
    void setHistory(s16 history[2][2]);
    void captureHistory(s16 history[2][2]) const;
    void close();

    FILE* _file;
    const char* _error;
    u32 _sampleRate;
    u32 _channels;      // decoded channels, 1 or 2
    u32 _fileChannels;  // channels stored in the file
    bool _loop;
    u32 _loopStart;
    u32 _loopEnd;  // exclusive
    u32 _totalSamples;
    u32 _blockCount;
    u32 _blockSize;
    u32 _samplesPerBlock;
    u32 _lastBlockPaddedSize;
    u32 _dataStart;
    u8* _blockData[2];
    u32 _loadedBlock;
    u32 _pos;
    sDspAdpcmState _state[2];
    s16 _startHistory[2][2];  // [channel][0] = hist1, [channel][1] = hist2
    s16 _loopHistory[2][2];
    bool _haveLoopHistory;
};
