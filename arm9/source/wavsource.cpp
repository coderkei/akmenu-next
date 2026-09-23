/*
    wavsource.cpp
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "wavsource.h"

#include <string.h>

namespace {
u32 readLe16(FILE* file, bool& ok) {
    int lo = fgetc(file);
    int hi = fgetc(file);
    if (lo == EOF || hi == EOF) {
        ok = false;
        return 0;
    }
    return (u32)lo | ((u32)hi << 8);
}

u32 readLe32(FILE* file, bool& ok) {
    int b0 = fgetc(file);
    int b1 = fgetc(file);
    int b2 = fgetc(file);
    int b3 = fgetc(file);
    if (b0 == EOF || b1 == EOF || b2 == EOF || b3 == EOF) {
        ok = false;
        return 0;
    }
    return (u32)b0 | ((u32)b1 << 8) | ((u32)b2 << 16) | ((u32)b3 << 24);
}

bool readFourCC(FILE* file, const char* expected) {
    char id[4];
    return fread(id, 1, sizeof(id), file) == sizeof(id) && memcmp(id, expected, sizeof(id)) == 0;
}
}  // namespace

cWavSource::cWavSource()
    : _file(NULL),
      _error(""),
      _sampleRate(0),
      _channels(0),
      _dataStart(0),
      _dataLength(0),
      _dataOffset(0) {}

cWavSource::~cWavSource() {
    close();
}

bool cWavSource::fail(const char* reason) {
    _error = reason;
    return false;
}

void cWavSource::close() {
    if (_file) {
        fclose(_file);
        _file = NULL;
    }
    _sampleRate = 0;
    _channels = 0;
}

bool cWavSource::open(const char* path) {
    close();
    _error = "";
    _file = fopen(path, "rb");
    if (!_file) return fail("wav: file not found");
    if (!parse()) {
        close();
        return false;
    }
    return true;
}

bool cWavSource::parse() {
    if (fseek(_file, 0, SEEK_END) != 0) return fail("wav: seek failed");
    const long fileLength = ftell(_file);
    if (fileLength < 12 || fseek(_file, 0, SEEK_SET) != 0) return fail("wav: file too small");

    bool ok = true;
    if (!readFourCC(_file, "RIFF")) return fail("wav: bad RIFF header");
    const u32 riffLength = readLe32(_file, ok);
    if (!ok || riffLength > (u32)(fileLength - 8) || !readFourCC(_file, "WAVE"))
        return fail("wav: bad RIFF header");
    const long riffEnd = 8 + (long)riffLength;

    bool haveFormat = false;
    bool haveData = false;
    u32 bitsPerSample = 0;
    u32 blockAlign = 0;
    u32 byteRate = 0;

    while (ftell(_file) >= 0 && ftell(_file) + 8 <= riffEnd) {
        char chunkId[4];
        if (fread(chunkId, 1, sizeof(chunkId), _file) != sizeof(chunkId))
            return fail("wav: chunk read failed");
        const u32 chunkLength = readLe32(_file, ok);
        if (!ok) return fail("wav: chunk read failed");
        const long chunkStart = ftell(_file);
        if (chunkStart < 0 || chunkLength > (u32)(riffEnd - chunkStart))
            return fail("wav: chunk outside file");

        if (memcmp(chunkId, "fmt ", 4) == 0) {
            if (chunkLength < 16) return fail("wav: bad fmt chunk");
            const u32 encoding = readLe16(_file, ok);
            _channels = readLe16(_file, ok);
            _sampleRate = readLe32(_file, ok);
            byteRate = readLe32(_file, ok);
            blockAlign = readLe16(_file, ok);
            bitsPerSample = readLe16(_file, ok);
            if (!ok || encoding != 1) return fail("wav: not PCM");
            haveFormat = true;
        } else if (memcmp(chunkId, "data", 4) == 0) {
            _dataStart = (u32)chunkStart;
            _dataLength = chunkLength;
            haveData = true;
        }

        const long nextChunk = chunkStart + (long)chunkLength + (chunkLength & 1);
        if (nextChunk > riffEnd || fseek(_file, nextChunk, SEEK_SET) != 0)
            return fail("wav: chunk outside file");
    }

    if (!haveFormat || !haveData) return fail("wav: missing fmt or data chunk");
    if (_channels < 1 || _channels > 2 || bitsPerSample != 16)
        return fail("wav: only 16-bit mono or stereo supported");
    const u32 frameBytes = _channels * 2;
    if (blockAlign != frameBytes || byteRate != _sampleRate * frameBytes || _dataLength == 0 ||
        _dataLength % frameBytes != 0) {
        return fail("wav: inconsistent format fields");
    }

    _dataOffset = 0;
    if (fseek(_file, (long)_dataStart, SEEK_SET) != 0) return fail("wav: seek failed");
    return true;
}

// WAV data is little-endian, like the DS and the host test machine, so bytes
// are copied straight into the sample buffer.
bool cWavSource::read(s16* dst, u32 frames) {
    if (!_file) return fail("wav: not open");
    u8* out = (u8*)dst;
    u32 remaining = frames * _channels * 2;
    while (remaining) {
        if (_dataOffset >= _dataLength) {
            if (fseek(_file, (long)_dataStart, SEEK_SET) != 0) return fail("wav: seek failed");
            _dataOffset = 0;
        }
        u32 request = _dataLength - _dataOffset;
        if (request > remaining) request = remaining;
        if (fread(out, 1, request, _file) != request) return fail("wav: sample read failed");
        out += request;
        remaining -= request;
        _dataOffset += request;
    }
    return true;
}
