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

#include "bcstmsource.h"
#include "dbgtool.h"
#include "globalsettings.h"
#include "irqs.h"
#include "systemfilenames.h"
#include "fifotool.h"
#include "wavsource.h"

namespace {
const u32 kRingBytes = 512 * 1024;
const u32 kStartupPrimeBytes = 128 * 1024;
const u32 kReadChunkBytes = 16 * 1024;
const u32 kMaxmodBufferFrames = 16384;
const u32 kMinSampleRate = 1024;
const u32 kMaxSampleRate = 48000;  // DS mixes output at 32.768 kHz; higher rates gain nothing

cThemeMusic gThemeMusic;
cThemeMusic* volatile gActiveThemeMusic = NULL;
mm_stream gStream;

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

// Returns `source` if it opens with a playable sample rate; deletes it otherwise.
cMusicSource* openSource(cMusicSource* source, const std::string& path) {
    if (source->open(path.c_str())) {
        const u32 rate = source->sampleRate();
        if (rate >= kMinSampleRate && rate <= kMaxSampleRate) return source;
        dbg_printf("Theme music ignored: '%s' sample rate %lu Hz unsupported\n", path.c_str(),
                   (unsigned long)rate);
    } else {
        dbg_printf("Theme music: '%s' not used (%s)\n", path.c_str(), source->error());
    }
    delete source;
    return NULL;
}
}  // namespace

cThemeMusic::cThemeMusic()
    : _source(NULL),
      _ring(NULL),
      _readTotal(0),
      _writeTotal(0),
      _frameBytes(0),
      _sampleRate(0),
      _format(MM_STREAM_16BIT_MONO),
      _maxmodInitialized(false),
      _streamOpen(false),
      _sourceFailed(false),
      _playing(false) {}

cThemeMusic& themeMusic() {
    return gThemeMusic;
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
        if (!segment) return false;
        if (!_source->read((s16*)(_ring + offset), segment / _frameBytes)) {
            _sourceFailed = true;
            dbg_printf("Theme music stopped refilling: %s\n", _source->error());
            return false;
        }

        compilerMemoryBarrier();
        written += segment;
        _writeTotal = written;
        length -= segment;
    }
    return true;
}

bool cThemeMusic::start() {
    if (!gs().playThemeMusic || _playing) return _playing;

    const std::string directory = SFN_UI_CURRENT_DIRECTORY;
    _source = openSource(new cBcstmSource(), directory + "bgm.bcstm");
    if (!_source) _source = openSource(new cWavSource(), directory + "bgm.wav");
    if (!_source) return false;

    _sampleRate = _source->sampleRate();
    _frameBytes = _source->channels() * 2;
    _format = _source->channels() == 1 ? MM_STREAM_16BIT_MONO : MM_STREAM_16BIT_STEREO;
    _sourceFailed = false;

    _ring = (u8*)malloc(kRingBytes);
    if (!_ring) {
        delete _source;
        _source = NULL;
        dbg_printf("Theme music ignored: not enough memory for music buffer\nYou obviously did not read the documentation!");
        return false;
    }

    _readTotal = 0;
    _writeTotal = 0;
    if (!queueBytes(kStartupPrimeBytes)) {
        free(_ring);
        _ring = NULL;
        delete _source;
        _source = NULL;
        dbg_printf("Theme music ignored: unable to read music data\n");
        return false;
    }

    bool vblankWasEnabled = (REG_IE & IRQ_VBLANK) != 0;
    if (!_maxmodInitialized) {
        fifoSendValue32(FIFO_USER_01, MENU_MSG_MAXMOD_INSTALL);
        fifoWaitValue32(FIFO_USER_01);
        if (fifoGetValue32(FIFO_USER_01) != 1) {
            free(_ring);
            _ring = NULL;
            delete _source;
            _source = NULL;
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
    delete _source;
    _source = NULL;
    if (_ring) {
        free(_ring);
        _ring = NULL;
    }
    _readTotal = 0;
    _writeTotal = 0;
}

void cThemeMusic::update() {
    if (!_playing || !_ring || _sourceFailed) return;
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
