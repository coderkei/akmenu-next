/*
    iconanimcache.h
    Copyright (C) 2026 Augusto Daniele

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <string>
#include "animatedicon.h"
#include "singleton.h"

enum EAnimFileKind { EAnimInBanner, EAnimInBannerSave };
enum EAnimLoad { EAnimPending, EAnimFailed, EAnimReady };

const u32 kIconAnimCacheSlots = 12;

// Holds the animation blocks of the icons on screen. Animation data is too big to keep per
// folder entry, so it is loaded on first draw and the least recently used slot is reused.
class cIconAnimCache {
  public:
    cIconAnimCache();

    // EAnimInBanner reads the block at bannerOffset + 0x1240 and checks it against expectedCrc.
    // EAnimInBannerSave reads a banner save and checks it against its own header.
    // A miss reads the file only if no file was read since the last newFrame(), otherwise it
    // returns EAnimPending, so a page of new icons costs one SD read per main-loop pass.
    EAnimLoad get(const std::string& path, EAnimFileKind kind, u32 bannerOffset, u16 expectedCrc,
                  const tDsiIconAnim*& anim);

    // Called once per main-loop pass; allows the next miss to read its file.
    void newFrame() { _loadedThisFrame = false; }

  private:
    struct tSlot {
        std::string path;
        u32 offset;
        u8 kind;
        bool used;
        u32 lastUse;
        tDsiIconAnim anim;
    };

    bool load(const std::string& path, EAnimFileKind kind, u32 bannerOffset, u16 expectedCrc,
              tDsiIconAnim& anim);

    tSlot _slots[kIconAnimCacheSlots];
    u32 _useCounter;
    bool _loadedThisFrame;
};

typedef t_singleton<cIconAnimCache> iconAnimCache_s;
inline cIconAnimCache& iconAnimCache() {
    return iconAnimCache_s::instance();
}
