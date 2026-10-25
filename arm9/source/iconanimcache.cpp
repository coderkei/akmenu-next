/*
    iconanimcache.cpp
    Copyright (C) 2026 Augusto Daniele

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "iconanimcache.h"
#include <cstdio>

cIconAnimCache::cIconAnimCache() : _useCounter(0), _loadedThisFrame(false) {
    for (u32 i = 0; i < kIconAnimCacheSlots; ++i) {
        _slots[i].used = false;
        _slots[i].lastUse = 0;
    }
}

EAnimLoad cIconAnimCache::get(const std::string& path, EAnimFileKind kind, u32 bannerOffset,
                              u16 expectedCrc, const tDsiIconAnim*& anim) {
    const u32 offset = (kind == EAnimInBanner) ? bannerOffset : 0;
    tSlot* victim = &_slots[0];
    for (u32 i = 0; i < kIconAnimCacheSlots; ++i) {
        tSlot& slot = _slots[i];
        if (slot.used && slot.kind == kind && slot.offset == offset && slot.path == path) {
            slot.lastUse = ++_useCounter;
            anim = &slot.anim;
            return EAnimReady;
        }
        if (!slot.used || (victim->used && slot.lastUse < victim->lastUse)) victim = &slot;
    }

    if (_loadedThisFrame) return EAnimPending;
    _loadedThisFrame = true;

    victim->used = false;
    if (!load(path, kind, offset, expectedCrc, victim->anim)) return EAnimFailed;
    victim->used = true;
    victim->path = path;
    victim->offset = offset;
    victim->kind = kind;
    victim->lastUse = ++_useCounter;
    anim = &victim->anim;
    return EAnimReady;
}

bool cIconAnimCache::load(const std::string& path, EAnimFileKind kind, u32 bannerOffset,
                          u16 expectedCrc, tDsiIconAnim& anim) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;

    bool ok;
    if (kind == EAnimInBanner) {
        ok = fseek(f, bannerOffset + DSI_BANNER_ANIM_OFFSET, SEEK_SET) == 0 &&
             fread(&anim, 1, sizeof(anim), f) == sizeof(anim);
    } else {
        u8 header[DSI_BANNER_SAVE_HEADER_SIZE];
        ok = fread(header, 1, sizeof(header), f) == sizeof(header) &&
             dsiBannerSaveHeaderValid(header, expectedCrc) &&
             fread(&anim, 1, sizeof(anim), f) == sizeof(anim);
    }
    fclose(f);
    return ok && dsiAnimValid(anim, expectedCrc);
}
