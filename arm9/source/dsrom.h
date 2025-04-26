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

class DSRomInfo {
  private:
    enum TBool { ETrue, EFalse, EMayBe };

  private:
    tNDSBanner _banner;
    SAVE_INFO_EX _saveInfo;
    TBool _isDSRom;
    TBool _isDSiWare;
    TBool _isHomebrew;
    TBool _isGbaRom;
    std::string _fileName;
    s32 _extIcon;
    u8 _romVersion;

  private:
    void load(void);
    bool loadGbaRomInfo(const std::string& filename);
    bool loadDSRomInfo(const std::string& filename, bool loadBanner);

  public:
    DSRomInfo()
        : _isDSRom(EFalse), _isDSiWare(EFalse), _isHomebrew(EFalse), _isGbaRom(EFalse), _extIcon(-1), _romVersion(0) {
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
    bool isGbaRom(void);
    DSRomInfo& operator=(const DSRomInfo& src);
    void MayBeDSRom(const std::string& filename) {
        _isDSRom = EMayBe;
        _isHomebrew = EMayBe;
        _isDSiWare = EMayBe;
        _fileName = filename;
    };
    void MayBeGbaRom(const std::string& filename) {
        _isGbaRom = EMayBe;
        _fileName = filename;
    };
    void setBanner(const std::string& anExtIcon, const u8* aBanner);
};
