/*
    thememusic.cpp
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "thememusic.h"

#include <maxmod9.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "dbgtool.h"
#include "globalsettings.h"
#include "irqs.h"
#include "systemfilenames.h"
#include "fifotool.h"

namespace {
const u32 kRingBytes = 512 * 1024;
const u32 kStartupPrimeBytes = 128 * 1024;
const u32 kReadChunkBytes = 16 * 1024;
const u32 kMaxmodBufferFrames = 16384;

cThemeMusic gThemeMusic;
cThemeMusic* volatile gActiveThemeMusic = NULL;
mm_stream gStream;

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
    return fread(id, 1, sizeof(id), file) == sizeof(id) &&
           memcmp(id, expected, sizeof(id)) == 0;
}

mm_word maxmodStreamCallback(mm_word length, mm_addr destination, mm_stream_formats format) {
    if (gActiveThemeMusic) return gActiveThemeMusic->fillStream(length, destination);
    u32 frameBytes = (format & 2) ? 2 : 1;
    if (format & 1) frameBytes *= 2;
    memset(destination, 0, length * frameBytes);
    return length;
}

void compilerMemoryBarrier() {
    __asm__ volatile("" ::: "memory");
}
}  // namespace

cThemeMusic::cThemeMusic()
    : _file(NULL),
      _ring(NULL),
      _readTotal(0),
      _writeTotal(0),
      _dataStart(0),
      _dataLength(0),
      _fileDataOffset(0),
      _frameBytes(0),
      _sampleRate(0),
      _format(MM_STREAM_16BIT_MONO),
      _maxmodInitialized(false),
      _streamOpen(false),
      _playing(false) {}

cThemeMusic& themeMusic() {
    return gThemeMusic;
}

bool cThemeMusic::parseWave() {
    if (!_file || fseek(_file, 0, SEEK_END) != 0) return false;
    long fileLength = ftell(_file);
    if (fileLength < 12 || fseek(_file, 0, SEEK_SET) != 0) return false;

    bool ok = true;
    if (!readFourCC(_file, "RIFF")) return false;
    u32 riffLength = readLe32(_file, ok);
    if (!ok || riffLength > (u32)(fileLength - 8) || !readFourCC(_file, "WAVE")) return false;
    long riffEnd = 8 + (long)riffLength;

    bool haveFormat = false;
    bool haveData = false;
    u32 channels = 0;
    u32 bitsPerSample = 0;
    u32 blockAlign = 0;
    u32 byteRate = 0;

    while (ftell(_file) >= 0 && ftell(_file) + 8 <= riffEnd) {
        char chunkId[4];
        if (fread(chunkId, 1, sizeof(chunkId), _file) != sizeof(chunkId)) return false;
        u32 chunkLength = readLe32(_file, ok);
        if (!ok) return false;
        long chunkStart = ftell(_file);
        if (chunkStart < 0 || chunkLength > (u32)(riffEnd - chunkStart)) return false;

        if (memcmp(chunkId, "fmt ", 4) == 0) {
            if (chunkLength < 16) return false;
            u32 encoding = readLe16(_file, ok);
            channels = readLe16(_file, ok);
            _sampleRate = readLe32(_file, ok);
            byteRate = readLe32(_file, ok);
            blockAlign = readLe16(_file, ok);
            bitsPerSample = readLe16(_file, ok);
            if (!ok || encoding != 1) return false;
            haveFormat = true;
        } else if (memcmp(chunkId, "data", 4) == 0) {
            _dataStart = (u32)chunkStart;
            _dataLength = chunkLength;
            haveData = true;
        }

        long nextChunk = chunkStart + (long)chunkLength + (chunkLength & 1);
        if (nextChunk > riffEnd || fseek(_file, nextChunk, SEEK_SET) != 0) return false;
    }

    if (!haveFormat || !haveData || channels < 1 || channels > 2 || bitsPerSample != 16 ||
        _sampleRate < 1024 || _sampleRate > 32768) {
        return false;
    }

    _frameBytes = channels * (bitsPerSample / 8);
    if (!blockAlign || blockAlign != _frameBytes || byteRate != _sampleRate * _frameBytes ||
        !_dataLength || _dataLength % _frameBytes) {
        return false;
    }

    _format = channels == 1 ? MM_STREAM_16BIT_MONO : MM_STREAM_16BIT_STEREO;
    return fseek(_file, (long)_dataStart, SEEK_SET) == 0;
}

bool cThemeMusic::readLooping(u8* destination, u32 length) {
    u32 written = 0;
    while (written < length) {
        if (_fileDataOffset >= _dataLength) {
            if (fseek(_file, (long)_dataStart, SEEK_SET) != 0) return false;
            _fileDataOffset = 0;
        }

        u32 available = _dataLength - _fileDataOffset;
        u32 request = length - written;
        if (request > available) request = available;
        size_t got = fread(destination + written, 1, request, _file);
        if (got != request) return false;
        written += request;
        _fileDataOffset += request;
    }
    return true;
}

bool cThemeMusic::queueBytes(u32 length) {
    u32 written = _writeTotal;
    u32 read = _readTotal;
    u32 used = written - read;
    if (used > kRingBytes) return false;
    u32 freeBytes = kRingBytes - used;
    if (length > freeBytes) length = freeBytes;
    length -= length % _frameBytes;

    while (length) {
        u32 offset = written & (kRingBytes - 1);
        u32 contiguous = kRingBytes - offset;
        u32 segment = length < contiguous ? length : contiguous;
        segment -= segment % _frameBytes;
        if (!segment || !readLooping(_ring + offset, segment)) return false;

        compilerMemoryBarrier();
        written += segment;
        _writeTotal = written;
        length -= segment;
    }
    return true;
}

bool cThemeMusic::start() {
    if (!gs().playThemeMusic || _playing) return _playing;

    std::string path = SFN_UI_CURRENT_DIRECTORY + "bgm.wav";
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) return false;

    _file = file;
    if (!parseWave()) {
        fclose(_file);
        _file = NULL;
        dbg_printf("Theme music ignored: unsupported or invalid WAV '%s'\n", path.c_str());
        return false;
    }

    _ring = (u8*)malloc(kRingBytes);
    if (!_ring) {
        fclose(_file);
        _file = NULL;
        dbg_printf("Theme music ignored: not enough memory for WAV buffer\nYou obviously did not read the documentation!");
        return false;
    }

    _readTotal = 0;
    _writeTotal = 0;
    _fileDataOffset = 0;
    if (!queueBytes(kStartupPrimeBytes)) {
        free(_ring);
        _ring = NULL;
        fclose(_file);
        _file = NULL;
        dbg_printf("Theme music ignored: unable to read WAV data\n");
        return false;
    }

    bool vblankWasEnabled = (REG_IE & IRQ_VBLANK) != 0;
    if (!_maxmodInitialized) {
        fifoSendValue32(FIFO_USER_01, MENU_MSG_MAXMOD_INSTALL);
        fifoWaitValue32(FIFO_USER_01);
        if (fifoGetValue32(FIFO_USER_01) != 1) {
            free(_ring);
            _ring = NULL;
            fclose(_file);
            _file = NULL;
            return false;
        }

        if (vblankWasEnabled) irqDisable(IRQ_VBLANK);
        mm_ds_system system;
        system.mod_count = 0;
        system.samp_count = 0;
        system.mem_bank = NULL;
        system.fifo_channel = FIFO_MAXMOD;
        mmInit(&system);
        _maxmodInitialized = true;
    } else if (vblankWasEnabled) {
        irqDisable(IRQ_VBLANK);
    }

    gStream.sampling_rate = _sampleRate;
    gStream.buffer_length = kMaxmodBufferFrames;
    gStream.callback = maxmodStreamCallback;
    gStream.format = _format;
    gStream.timer = MM_TIMER1;
    gStream.manual = false;

    gActiveThemeMusic = this;
    _playing = true;
    compilerMemoryBarrier();
    mmStreamOpen(&gStream);
    _streamOpen = true;
    if (vblankWasEnabled) irqEnable(IRQ_VBLANK);
    dbg_printf("Theme music started (%lu Hz, %lu-bit %s)\n", (unsigned long)_sampleRate,
               (unsigned long)16, _format == MM_STREAM_16BIT_MONO ? "mono" : "stereo");
    return true;
}

void cThemeMusic::stop() {
    _playing = false;
    compilerMemoryBarrier();
    if (_streamOpen) {
        mmStreamClose();
        _streamOpen = false;
    }
    if (gActiveThemeMusic == this) gActiveThemeMusic = NULL;
    if (_file) {
        fclose(_file);
        _file = NULL;
    }
    if (_ring) {
        free(_ring);
        _ring = NULL;
    }
    _readTotal = 0;
    _writeTotal = 0;
}

void cThemeMusic::update() {
    if (!_playing || !_ring) return;
    u32 used = _writeTotal - _readTotal;
    if (used >= kRingBytes) return;
    queueBytes(kReadChunkBytes);
}

u32 cThemeMusic::fillStream(u32 length, void* destination) {
    if (!destination || !_frameBytes) return 0;

    u32 requestedBytes = length * _frameBytes;
    if (!_playing || !_ring) {
        memset(destination, 0, requestedBytes);
        return length;
    }
    u32 read = _readTotal;
    u32 written = _writeTotal;
    u32 available = written - read;
    u32 copied = requestedBytes < available ? requestedBytes : available;
    copied -= copied % _frameBytes;

    u8* output = (u8*)destination;
    u32 remaining = copied;
    while (remaining) {
        u32 offset = read & (kRingBytes - 1);
        u32 contiguous = kRingBytes - offset;
        u32 segment = remaining < contiguous ? remaining : contiguous;
        memcpy(output, _ring + offset, segment);
        output += segment;
        remaining -= segment;
        read += segment;
    }

    compilerMemoryBarrier();
    _readTotal = read;
    if (copied < requestedBytes) {
        memset(output, 0, requestedBytes - copied);
    }
    return length;
}
