/*
    bcstmsource.cpp
    Copyright (C) 2026 Augusto Daniele

    SPDX-License-Identifier: GPL-3.0-or-later
*/

// BCSTM reader for DSP-ADPCM streams. Format references: 3dbrew /
// docs.mikage.app "BCSTM", GBATEK "3DS Files - Sound Wave Streams (CSTM
// Format)", vgmstream src/meta/bcstm.c.

#include "bcstmsource.h"

#include <stdlib.h>
#include <string.h>

namespace {
const u32 kMinFileBytes = 0x40;
const u32 kHeaderBytes = 0x14 + 8 * 12;  // fixed header + up to 8 block references
const u32 kMaxBlockReferences = 8;
const u32 kMaxInfoBytes = 64 * 1024;
const u32 kMaxBlockBytes = 64 * 1024;
const u32 kStreamInfoBytes = 0x38;
const u32 kAdpcmInfoBytes = 0x2E;
const u32 kNoBlock = 0xFFFFFFFF;

const u16 kByteOrderLittle = 0xFEFF;
const u16 kTypeInfoBlock = 0x4000;
const u16 kTypeDataBlock = 0x4002;
const u16 kTypeStreamInfo = 0x4100;
const u16 kTypeDspAdpcmInfo = 0x0300;
const u8 kCodecDspAdpcm = 2;

u16 readLe16(const u8* p) {
    return (u16)(p[0] | (p[1] << 8));
}

u32 readLe32(const u8* p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

// True when [offset, offset + length) lies inside `size` bytes.
bool fits(u64 offset, u64 length, u64 size) {
    return offset <= size && length <= size - offset;
}
}  // namespace

cBcstmSource::cBcstmSource()
    : _file(NULL),
      _error(""),
      _sampleRate(0),
      _channels(0),
      _fileChannels(0),
      _loop(false),
      _loopStart(0),
      _loopEnd(0),
      _totalSamples(0),
      _blockCount(0),
      _blockSize(0),
      _samplesPerBlock(0),
      _lastBlockPaddedSize(0),
      _dataStart(0),
      _loadedBlock(kNoBlock),
      _pos(0),
      _haveLoopHistory(false) {
    _blockData[0] = NULL;
    _blockData[1] = NULL;
    memset(_state, 0, sizeof(_state));
    memset(_startHistory, 0, sizeof(_startHistory));
    memset(_loopHistory, 0, sizeof(_loopHistory));
}

cBcstmSource::~cBcstmSource() {
    close();
}

bool cBcstmSource::fail(const char* reason) {
    _error = reason;
    return false;
}

void cBcstmSource::close() {
    if (_file) {
        fclose(_file);
        _file = NULL;
    }
    for (int c = 0; c < 2; c++) {
        free(_blockData[c]);
        _blockData[c] = NULL;
    }
    _sampleRate = 0;
    _channels = 0;
    _loadedBlock = kNoBlock;
}

bool cBcstmSource::open(const char* path) {
    close();
    _error = "";
    _file = fopen(path, "rb");
    if (!_file) return fail("bcstm: file not found");
    if (!parse()) {
        close();
        return false;
    }
    return true;
}

bool cBcstmSource::parse() {
    if (fseek(_file, 0, SEEK_END) != 0) return fail("bcstm: seek failed");
    const long fileLength = ftell(_file);
    if (fileLength < (long)kMinFileBytes) return fail("bcstm: file too small");
    const u32 fileSize = (u32)fileLength;

    u8 header[kHeaderBytes];
    const u32 headerRead = fileSize < kHeaderBytes ? fileSize : kHeaderBytes;
    if (fseek(_file, 0, SEEK_SET) != 0 || fread(header, 1, headerRead, _file) != headerRead) {
        return fail("bcstm: header read failed");
    }
    if (memcmp(header, "CSTM", 4) != 0) return fail("bcstm: bad magic");
    if (readLe16(header + 0x04) != kByteOrderLittle) return fail("bcstm: not little-endian");

    const u32 references = readLe16(header + 0x10);
    if (references < 1 || references > kMaxBlockReferences || 0x14 + references * 12 > headerRead)
        return fail("bcstm: bad block count");

    bool haveInfo = false;
    bool haveData = false;
    u32 infoOffset = 0, infoSize = 0, dataOffset = 0, dataSize = 0;
    for (u32 i = 0; i < references; i++) {
        const u8* reference = header + 0x14 + i * 12;
        const u16 type = readLe16(reference);
        if (type != kTypeInfoBlock && type != kTypeDataBlock) continue;
        const u32 offset = readLe32(reference + 4);
        const u32 size = readLe32(reference + 8);
        if (!fits(offset, size, fileSize)) return fail("bcstm: block outside file");
        if (type == kTypeInfoBlock) {
            haveInfo = true;
            infoOffset = offset;
            infoSize = size;
        } else {
            haveData = true;
            dataOffset = offset;
            dataSize = size;
        }
    }
    if (!haveInfo || !haveData) return fail("bcstm: missing INFO or DATA block");
    if (infoSize < 0x20 + kStreamInfoBytes || infoSize > kMaxInfoBytes)
        return fail("bcstm: bad INFO size");

    u8* info = (u8*)malloc(infoSize);
    if (!info) return fail("bcstm: out of memory");
    bool ok = fseek(_file, (long)infoOffset, SEEK_SET) == 0 &&
              fread(info, 1, infoSize, _file) == infoSize;
    if (!ok)
        fail("bcstm: INFO read failed");
    else
        ok = parseInfo(info, infoSize, dataOffset, dataSize);
    free(info);
    if (!ok) return false;

    for (u32 c = 0; c < _channels; c++) {
        _blockData[c] = (u8*)malloc(_blockSize);
        if (!_blockData[c]) return fail("bcstm: out of memory");
    }
    rewind();
    return true;
}

bool cBcstmSource::parseInfo(const u8* info, u32 size, u32 dataOffset, u32 dataSize) {
    if (memcmp(info, "INFO", 4) != 0) return fail("bcstm: bad INFO magic");

    // The references at INFO+0x08/0x10/0x18 are relative to INFO+0x08.
    if (readLe16(info + 0x08) != kTypeStreamInfo) return fail("bcstm: missing stream info");
    const u64 streamOffset = 8 + (u64)readLe32(info + 0x0C);
    if (!fits(streamOffset, kStreamInfoBytes, size)) return fail("bcstm: stream info outside INFO");
    const u8* stream = info + streamOffset;

    const u8 codec = stream[0x00];
    _loop = stream[0x01] != 0;
    _fileChannels = stream[0x02];
    _sampleRate = readLe32(stream + 0x04);
    _loopStart = readLe32(stream + 0x08);
    _loopEnd = readLe32(stream + 0x0C);
    _blockCount = readLe32(stream + 0x10);
    _blockSize = readLe32(stream + 0x14);
    _samplesPerBlock = readLe32(stream + 0x18);
    const u32 lastBlockSamples = readLe32(stream + 0x20);
    _lastBlockPaddedSize = readLe32(stream + 0x24);
    const u32 sampleDataOffset = readLe32(stream + 0x34);

    if (codec != kCodecDspAdpcm) return fail("bcstm: codec not DSP-ADPCM");
    if (_fileChannels < 1) return fail("bcstm: no channels");
    _channels = _fileChannels > 2 ? 2 : _fileChannels;
    if (_blockCount < 1) return fail("bcstm: no sample blocks");
    if (_samplesPerBlock == 0 || _samplesPerBlock % kDspAdpcmFrameSamples != 0)
        return fail("bcstm: samples per block not a multiple of 14");
    if (_blockSize > kMaxBlockBytes ||
        _blockSize < _samplesPerBlock / kDspAdpcmFrameSamples * kDspAdpcmFrameBytes) {
        return fail("bcstm: bad block size");
    }
    if (lastBlockSamples < 1 || lastBlockSamples > _samplesPerBlock)
        return fail("bcstm: bad last block sample count");
    const u32 lastBlockMinBytes = (lastBlockSamples + kDspAdpcmFrameSamples - 1) /
                                  kDspAdpcmFrameSamples * kDspAdpcmFrameBytes;
    if (_lastBlockPaddedSize < lastBlockMinBytes || _lastBlockPaddedSize > _blockSize)
        return fail("bcstm: bad last block size");

    const u64 totalSamples = (u64)(_blockCount - 1) * _samplesPerBlock + lastBlockSamples;
    if (totalSamples > 0xFFFFFFFFu) return fail("bcstm: stream too long");
    _totalSamples = (u32)totalSamples;
    if (_loop && !(_loopStart < _loopEnd && _loopEnd <= _totalSamples))
        return fail("bcstm: bad loop points");

    const u64 sampleBytes = (u64)(_blockCount - 1) * _fileChannels * _blockSize +
                            (u64)_fileChannels * _lastBlockPaddedSize;
    if (dataSize < 8 || !fits(sampleDataOffset, sampleBytes, dataSize - 8))
        return fail("bcstm: sample data outside DATA block");
    _dataStart = dataOffset + 8 + sampleDataOffset;

    // Channel table: u32 count, then references relative to the table start.
    const u64 tableOffset = 8 + (u64)readLe32(info + 0x1C);
    if (!fits(tableOffset, 4, size)) return fail("bcstm: channel table outside INFO");
    const u32 tableCount = readLe32(info + tableOffset);
    if (tableCount < _fileChannels || !fits(tableOffset + 4, (u64)tableCount * 8, size))
        return fail("bcstm: bad channel table");

    for (u32 c = 0; c < _channels; c++) {
        const u64 channelOffset = tableOffset + readLe32(info + tableOffset + 4 + c * 8 + 4);
        if (!fits(channelOffset, 8, size)) return fail("bcstm: channel info outside INFO");
        // Channel info starts with a reference relative to itself.
        if (readLe16(info + channelOffset) != kTypeDspAdpcmInfo)
            return fail("bcstm: channel is not DSP-ADPCM");
        const u64 adpcmOffset = channelOffset + readLe32(info + channelOffset + 4);
        if (!fits(adpcmOffset, kAdpcmInfoBytes, size))
            return fail("bcstm: ADPCM info outside INFO");
        const u8* adpcm = info + adpcmOffset;

        // Clamped so the decoder can work in 32-bit arithmetic; real files are far
        // below the limit, so this only bounds corrupt data.
        for (int k = 0; k < 16; k++) {
            s32 coef = (s16)readLe16(adpcm + k * 2);
            if (coef > kDspAdpcmMaxCoef) coef = kDspAdpcmMaxCoef;
            if (coef < -kDspAdpcmMaxCoef) coef = -kDspAdpcmMaxCoef;
            _state[c].coefs[k] = (s16)coef;
        }
        // Start context: 0x20 predictor/scale, 0x22 yn1, 0x24 yn2. The loop
        // context at 0x26 is not used; history is captured at loopStart.
        _startHistory[c][0] = (s16)readLe16(adpcm + 0x22);
        _startHistory[c][1] = (s16)readLe16(adpcm + 0x24);
    }
    return true;
}

void cBcstmSource::setHistory(s16 history[2][2]) {
    for (int c = 0; c < 2; c++) {
        _state[c].hist1 = history[c][0];
        _state[c].hist2 = history[c][1];
    }
}

void cBcstmSource::captureHistory(s16 history[2][2]) const {
    for (int c = 0; c < 2; c++) {
        history[c][0] = _state[c].hist1;
        history[c][1] = _state[c].hist2;
    }
}

void cBcstmSource::rewind() {
    _pos = 0;
    _loadedBlock = kNoBlock;
    _haveLoopHistory = false;
    setHistory(_startHistory);
}

bool cBcstmSource::loadBlock(u32 block) {
    _loadedBlock = kNoBlock;
    // A block stores channel 0, channel 1, ... back to back. The last block's
    // per-channel size is lastBlockPaddedSize, so channels are read in sequence.
    const u64 offset = _dataStart + (u64)block * _fileChannels * _blockSize;
    const u32 bytes = block == _blockCount - 1 ? _lastBlockPaddedSize : _blockSize;
    if (fseek(_file, (long)offset, SEEK_SET) != 0) return fail("bcstm: seek failed");
    for (u32 c = 0; c < _channels; c++) {
        if (fread(_blockData[c], 1, bytes, _file) != bytes)
            return fail("bcstm: sample read failed");
    }
    _loadedBlock = block;
    return true;
}

bool cBcstmSource::read(s16* dst, u32 frames) {
    if (!_file) return fail("bcstm: not open");
    s16 decoded[2][kDspAdpcmFrameSamples];

    while (frames) {
        const u32 end = _loop ? _loopEnd : _totalSamples;
        if (_pos == end) {
            if (_loop) {
                _pos = _loopStart;
                setHistory(_loopHistory);
            } else {
                _pos = 0;
                setHistory(_startHistory);
            }
        }
        // Playback always reaches loopStart before loopEnd, so the history
        // needed to resume at loopStart is recorded before the first wrap.
        if (_loop && _pos == _loopStart && !_haveLoopHistory) {
            captureHistory(_loopHistory);
            _haveLoopHistory = true;
        }

        const u32 block = _pos / _samplesPerBlock;
        if (block != _loadedBlock && !loadBlock(block)) return false;

        const u32 inBlock = _pos - block * _samplesPerBlock;
        const u32 first = inBlock % kDspAdpcmFrameSamples;
        u32 count = kDspAdpcmFrameSamples - first;
        if (count > frames) count = frames;
        if (count > end - _pos) count = end - _pos;
        if (_loop && _pos < _loopStart && count > _loopStart - _pos) count = _loopStart - _pos;

        const u32 frameOffset = inBlock / kDspAdpcmFrameSamples * kDspAdpcmFrameBytes;
        for (u32 c = 0; c < _channels; c++)
            dspAdpcmDecode(_blockData[c] + frameOffset, _state[c], decoded[c], first, count);

        if (_channels == 1) {
            memcpy(dst, decoded[0], count * sizeof(s16));
            dst += count;
        } else {
            for (u32 i = 0; i < count; i++) {
                *dst++ = decoded[0][i];
                *dst++ = decoded[1][i];
            }
        }
        _pos += count;
        frames -= count;
    }
    return true;
}
