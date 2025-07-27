/*
    dsrom.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "dsrom.h"
#include "dbgtool.h"
#include "fileicons.h"
#include "gamecode.h"
#include "gbarom_banner_bin.h"
#include "icon_bg_bin.h"
#include "icons.h"
#include "nds_banner_bin.h"
#include "unknown_nds_banner_bin.h"

DSRomInfo& DSRomInfo::operator=(const DSRomInfo& src) {
    memcpy(&_banner, &src._banner, sizeof(_banner));
    memcpy(&_saveInfo, &src._saveInfo, sizeof(_saveInfo));
    _isDSRom = src._isDSRom;
    _isHomebrew = src._isHomebrew;
    _isGbaRom = src._isGbaRom;
    _fileName = src._fileName;
    _romVersion = src._romVersion;
    _extIcon = src._extIcon;
    return *this;
}

bool DSRomInfo::loadDSRomInfo(const std::string& filename, bool loadBanner) {
    _isDSRom = EFalse;
    _isHomebrew = EFalse;
    _isDSiWare = EFalse;
    FILE* f = fopen(filename.c_str(), "rb");
    if (NULL == f)  // 锟斤拷锟侥硷拷失锟斤拷
    {
        return false;
    }

    tNDSHeader header;
    if (512 != fread(&header, 1, 512, f))  // 锟斤拷锟侥硷拷头失锟斤拷
    {
        dbg_printf("read rom header fail\n");
        memcpy(&_banner, unknown_nds_banner_bin, sizeof(_banner));
        fclose(f);
        return false;
    }

    if (header.unitCode == 0x03) {
        _isDSiWare = ETrue;
    }

    ///////// ROM Header /////////
    u16 crc = swiCRC16(0xFFFF, &header, 0x15E);
    if (crc != header.headerCRC16)  // 锟侥硷拷头 CRC 锟斤拷锟襟，诧拷锟斤拷nds锟斤拷戏
    {
        dbg_printf("%s rom header crc error\n", filename.c_str());
        memcpy(&_banner, unknown_nds_banner_bin, sizeof(_banner));
        fclose(f);
        return true;
    } else {
        _isDSRom = ETrue;
        if ((u32)(header.arm7destination) >= 0x037F8000 ||
            0x23232323 == gamecode(header.gameCode)) {  // 23->'#'
            _isHomebrew = ETrue;
        }
    }

    ///////// saveInfo /////////
    memcpy(_saveInfo.gameTitle, header.gameTitle, 12);
    memcpy(_saveInfo.gameCode, header.gameCode, 4);
    _saveInfo.gameCRC = header.headerCRC16;
    saveManager().updateSaveInfoByInfo(_saveInfo);
    _romVersion = header.romversion;

    // dbg_printf( "save type %d\n", _saveInfo.saveType );

    ///////// banner /////////
    if (header.bannerOffset != 0) {
        fseek(f, header.bannerOffset, SEEK_SET);
        tNDSBanner banner;
        u32 readed = fread(&banner, 1, 0x840, f);
        if (sizeof(tNDSBanner) != readed) {
            memcpy(&_banner, nds_banner_bin, sizeof(_banner));
        } else {
            crc = swiCRC16(0xffff, banner.icon, 0x840 - 32);

            if (crc != banner.crc) {
                dbg_printf("banner crc error, %04x/%04x\n", banner.crc, crc);
                memcpy(&_banner, nds_banner_bin, sizeof(_banner));
            } else {
                memcpy(&_banner, &banner, sizeof(_banner));
            }
        }
    } else {
        // dbg_printf( "%s has no banner\n", filename );
        memcpy(&_banner, nds_banner_bin, sizeof(_banner));
    }

    fclose(f);
    return true;
}

void DSRomInfo::drawDSRomIcon(u8 x, u8 y, GRAPHICS_ENGINE engine, bool small) {
    if (_extIcon >= 0) {
        fileIcons().Draw(_extIcon, x, y, engine);
        return;
    }
    load();
    bool skiptransparent = false;
    switch (_saveInfo.getIcon()) {
        case SAVE_INFO_EX_ICON_TRANSPARENT:
            break;
        case SAVE_INFO_EX_ICON_AS_IS:
            skiptransparent = true;
            break;
        case SAVE_INFO_EX_ICON_FIRMWARE:
            gdi().maskBlt(icon_bg_bin, x, y, 32, 32, engine);
            break;
    }
    
    if (small) {
        // Draw 16x16px
        for (int tile = 0; tile < 16; ++tile) {
            for (int pixel = 0; pixel < 32; ++pixel) {
                u8 a_byte = _banner.icon[(tile << 5) + pixel];

                int px = ((tile & 3) << 3) + ((pixel << 1) & 7);
                int py = ((tile >> 2) << 3) + (pixel >> 2);

                u8 idx1 = (a_byte & 0xf0) >> 4;
                u8 idx2 = (a_byte & 0x0f);

                int small_px = px / 2;
                int small_py = py / 2;

                if (skiptransparent || 0 != idx1) {
                    gdi().setPenColor(_banner.palette[idx1], engine);
                    gdi().drawPixel(small_px + 1 + x, small_py + y, engine);
                }

                if (skiptransparent || 0 != idx2) {
                    gdi().setPenColor(_banner.palette[idx2], engine);
                    gdi().drawPixel(small_px + x, small_py + y, engine);
                }
            }
        }
    } else {
        // Draw 32x32px
        for (int tile = 0; tile < 16; ++tile) {
            for (int pixel = 0; pixel < 32; ++pixel) {
                u8 a_byte = _banner.icon[(tile << 5) + pixel];

                int px = ((tile & 3) << 3) + ((pixel << 1) & 7);
                int py = ((tile >> 2) << 3) + (pixel >> 2);

                u8 idx1 = (a_byte & 0xf0) >> 4;
                u8 idx2 = (a_byte & 0x0f);

                if (skiptransparent || 0 != idx1) {
                    gdi().setPenColor(_banner.palette[idx1], engine);
                    gdi().drawPixel(px + 1 + x, py + y, engine);
                }

                if (skiptransparent || 0 != idx2) {
                    gdi().setPenColor(_banner.palette[idx2], engine);
                    gdi().drawPixel(px + x, py + y, engine);
                }
            }
        }
    }
}

void DSRomInfo::drawDSRomIconMem(void* mem) {
    if (_extIcon >= 0) {
        fileIcons().DrawMem(_extIcon, mem);
        return;
    }
    load();
    u16* pmem = (u16*)mem;
    bool skiptransparent = false;
    switch (_saveInfo.getIcon()) {
        case SAVE_INFO_EX_ICON_TRANSPARENT:
            break;
        case SAVE_INFO_EX_ICON_AS_IS:
            skiptransparent = true;
            break;
        case SAVE_INFO_EX_ICON_FIRMWARE:
            cIcons::maskBlt((const u16*)icon_bg_bin, pmem);
            break;
    }
    for (int tile = 0; tile < 16; ++tile) {
        for (int pixel = 0; pixel < 32; ++pixel) {
            u8 a_byte = _banner.icon[(tile << 5) + pixel];

            // int px = (tile & 3) * 8 + (2 * pixel & 7);
            // int py = (tile / 4) * 8 + (2 * pixel / 8);
            int px = ((tile & 3) << 3) + ((pixel << 1) & 7);
            int py = ((tile >> 2) << 3) + (pixel >> 2);

            u8 idx1 = (a_byte & 0xf0) >> 4;
            if (skiptransparent || 0 != idx1) {
                pmem[py * 32 + px + 1] = _banner.palette[idx1] | BIT(15);
                // gdi().setPenColor( _banner.palette[idx1] );
                // gdi().drawPixel( px+1+x, py+y, engine );
            }

            u8 idx2 = (a_byte & 0x0f);
            if (skiptransparent || 0 != idx2) {
                pmem[py * 32 + px] = _banner.palette[idx2] | BIT(15);
                // gdi().setPenColor( _banner.palette[idx2] );
                // gdi().drawPixel( px+x, py+y, engine );
            }
        }
    }
}

bool DSRomInfo::loadGbaRomInfo(const std::string& filename) {
    _isGbaRom = EFalse;
    FILE* gbaFile = fopen(filename.c_str(), "rb");
    if (gbaFile) {
        sGBAHeader header;
        fread(&header, 1, sizeof(header), gbaFile);
        fclose(gbaFile);
        if (header.is96h == 0x96) {
            _isGbaRom = ETrue;
            memcpy(_saveInfo.gameCode, header.gamecode, 4);
            _romVersion = header.version;
            memcpy(&_banner, gbarom_banner_bin, sizeof(tNDSBanner));
            return true;
        }
    }
    return false;
}

void DSRomInfo::load(void) {
    if (_isDSRom == EMayBe) loadDSRomInfo(_fileName, true);
    if (_isGbaRom == EMayBe) loadGbaRomInfo(_fileName);
}

tNDSBanner& DSRomInfo::banner(void) {
    load();
    return _banner;
}

SAVE_INFO_EX& DSRomInfo::saveInfo(void) {
    load();
    return _saveInfo;
}

u8 DSRomInfo::version(void) {
    load();
    return _romVersion;
}

bool DSRomInfo::isDSRom(void) {
    load();
    return (_isDSRom == ETrue) ? true : false;
}

bool DSRomInfo::isDSiWare(void) {
    load();
    return (_isDSiWare == ETrue) ? true : false;
}

bool DSRomInfo::isHomebrew(void) {
    load();
    return (_isHomebrew == ETrue) ? true : false;
}

bool DSRomInfo::isGbaRom(void) {
    load();
    return (_isGbaRom == ETrue) ? true : false;
}

void DSRomInfo::setExtIcon(const std::string& aValue) {
    _extIcon = fileIcons().Icon(aValue);
};

void DSRomInfo::setBanner(const std::string& anExtIcon, const u8* aBanner) {
    setExtIcon(anExtIcon);
    memcpy(&banner(), aBanner, sizeof(tNDSBanner));
}
