/*
    dsrom.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds.h>
#include <string.h>
#include "gdi.h"
#include "savemngr.h"

struct tDsiIconAnim;

class DSRomInfo {
  public:
    // Where the static banner came from; decides where its DSi animation can be read.
    enum EBannerSource { EBannerBuiltin, EBannerRom, EBannerFile, EBannerSidecar };
    // Progress of picking the animation source (see currentAnim()).
    enum EAnimState { EAnimUnknown, EAnimTryBanner, EAnimNone, EAnimFromSave, EAnimFromBanner };

  private:
    enum TBool { ETrue, EFalse, EMayBe };

  private:
    tNDSBanner _banner;
    SAVE_INFO_EX _saveInfo;
    TBool _isDSRom;
    TBool _isDSiWare;
    TBool _isHomebrew;
    TBool _isModernHomebrew;
    TBool _isGbaRom;
    bool _isNdz;
    std::string _fileName;
    s32 _extIcon;
    u8 _romVersion;
    u8 _bannerSource;
    u8 _dsiFlags;  // ROM header 0x1BF
    u8 _animState;
    u32 _bannerOffset;
    u32 _animStartTick;  // when a play-once (stop) sequence started
    std::string _bannerPath;  // only for .bin banners that can hold an animation

  private:
    void load(void);
    bool loadGbaRomInfo(const std::string& filename);
    bool loadDSRomInfo(const std::string& filename, bool loadBanner);

  public:
    DSRomInfo()
        : _isDSRom(EFalse), _isDSiWare(EFalse), _isHomebrew(EFalse), _isModernHomebrew(EFalse), _isGbaRom(EFalse), _isNdz(false), _extIcon(-1), _romVersion(0),
          _bannerSource(EBannerBuiltin), _dsiFlags(0), _animState(EAnimUnknown), _bannerOffset(0),
          _animStartTick(0) {
        // memcpy(&_banner,unknown_banner_bin,unknown_banner_bin_size);
        memset(&_banner, 0, sizeof(_banner));
        memset(&_saveInfo, 0, sizeof(_saveInfo));
    }

  public:
    void drawDSRomIcon(u8 x, u8 y, GRAPHICS_ENGINE engine, bool small);
    void drawDSRomIconMem(void* mem);
    tNDSBanner& banner(void);
    SAVE_INFO_EX& saveInfo(void);
    u8 version(void);
    void setExtIcon(const std::string& aValue);
    inline bool isExtIcon(void) { return _extIcon >= 0; };
    bool isDSRom(void);
    bool isDSiWare(void);
    bool isHomebrew(void);
    bool isModernHomebrew(void);
    bool isGbaRom(void);
    bool isNdz(void);
    DSRomInfo& operator=(const DSRomInfo& src);
    void MayBeDSRom(const std::string& filename) {
        _isDSRom = EMayBe;
        _isHomebrew = EMayBe;
        _isModernHomebrew = EMayBe;
        _isDSiWare = EMayBe;
        _fileName = filename;
    };
    void MayBeGbaRom(const std::string& filename) {
        _isGbaRom = EMayBe;
        _fileName = filename;
    };
    void setBanner(const std::string& anExtIcon, const u8* aBanner);
    bool setBannerFromFile(const std::string& anExtIcon, const std::string& path);
    void setBannerFromIcon(const std::string& filename, const std::string& anExtIcon,
                           const u8* builtInBanner);
    // Marks the banner as read from a .bin file (plugin icon or per-file sidecar).
    void setBannerFile(const std::string& path, bool sidecar);
    // DSiWare that keeps a banner save (ROM header 0x1BF bit 2).
    bool hasBannerSave(void);
    // Current DSi animation token, or 0 when the icon is drawn static this frame.
    u16 iconToken(void);

  private:
    void setBuiltinBanner(void);
    const tDsiIconAnim* currentAnim(void);
    const tDsiIconAnim* animReady(u8 state, const tDsiIconAnim* anim);
    u16 animToken(const tDsiIconAnim& anim);
};
