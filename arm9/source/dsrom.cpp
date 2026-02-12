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
    _isModernHomebrew = src._isModernHomebrew;
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
    _isModernHomebrew = EFalse;
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

        //check for modern homebrew
        //Assume homebrew until proven otherwise
        _isHomebrew = ETrue;
        _isModernHomebrew = ETrue;
        u32 arm9StartSig[4] = {0};
        //Seek to ARM9 entry point and read first 4 instructions
        fseek(f, (u32)header.arm9romOffset + (u32)header.arm9executeAddress - (u32)header.arm9destination, SEEK_SET);
        fread(arm9StartSig, sizeof(u32), 4, f);

         //Check for Nintendo SDK style retail builds
        if ((arm9StartSig[0] == 0xE3A0C301 || (arm9StartSig[0] >= 0xEA000000 && arm9StartSig[0] < 0xEC000000))
        && arm9StartSig[1] == 0xE58CC208) {
        if ((arm9StartSig[2] >= 0xEB000000 && arm9StartSig[2] < 0xEC000000)
        && (arm9StartSig[3] >= 0xE3A00000 && arm9StartSig[3] < 0xE3A01000)) {
            _isHomebrew = EFalse;
            _isModernHomebrew = EFalse;
        } else if (arm9StartSig[2] == 0xE1DC00B6 && arm9StartSig[3] == 0xE3500000) {
            _isHomebrew = EFalse;
            _isModernHomebrew = EFalse;
        } else if (arm9StartSig[2] == 0xEAFFFFFF && arm9StartSig[3] == 0xE1DC00B6) {
            _isHomebrew = EFalse;
            _isModernHomebrew = EFalse;
        }
        } else if (strncmp(header.gameCode, "HNA", 3) == 0) {
            //Modcrypted retail game
            _isHomebrew = EFalse;
            _isModernHomebrew = EFalse;
        }

        //If still homebrew, check for old vs modern
        if (_isHomebrew) {
            if (arm9StartSig[0] == 0xE3A00301
            && arm9StartSig[1] == 0xE5800208
            && arm9StartSig[2] == 0xE3A00013
            && arm9StartSig[3] == 0xE129F000) {
                //Modern hb signature, but check some known old cases
                if ((u32)header.arm7executeAddress >= 0x037F0000 && (u32)header.arm7destination >= 0x037F0000) {
                    if ((header.arm9binarySize == 0xC9F68 && header.arm7binarySize == 0x12814) ||   // Colors! v1.1
                        (header.arm9binarySize == 0x1B0864 && header.arm7binarySize == 0xDB50) ||  // Mario Paint Composer DS v2
                        (header.arm9binarySize == 0xE78FC && header.arm7binarySize == 0xF068) ||   // SnowBros v2.2
                        (header.arm9binarySize == 0xD45C0 && header.arm7binarySize == 0x2B7C) ||   // ikuReader v0.058
                        (header.arm9binarySize == 0x7A124 && header.arm7binarySize == 0xEED0) ||   // PPSEDS r11
                        (header.arm9binarySize == 0x54620 && header.arm7binarySize == 0x1538) ||   // XRoar 0.24fp3
                        (header.arm9binarySize == 0x2C9A8 && header.arm7binarySize == 0xFB98) ||   // NitroGrafx v0.7
                        (header.arm9binarySize == 0x22AE4 && header.arm7binarySize == 0xA764)) {   // It's 1975...
                        _isModernHomebrew = EFalse;
                    }
                }
            } else if ((header.unitCode == 0) &&
                ((memcmp(header.gameTitle, "NMP4BOOT", 8) == 0) ||
                    ((u32)header.arm7executeAddress >= 0x037F0000 && (u32)header.arm7destination >= 0x037F0000))) {
                _isModernHomebrew = EFalse; // Old hb requiring DLDI
            }

            u8 accessControl = 0;
            fseek(f, 0x1BF, SEEK_SET);
            fread(&accessControl, 1, 1, f);

            if (!_isHomebrew && (header.unitCode != 0) && (accessControl & BIT(4))) {
                _isDSiWare = ETrue;
            }
    }
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

bool DSRomInfo::isModernHomebrew(void) {
    load();
    return (_isModernHomebrew == ETrue) ? true : false;
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

bool DSRomInfo::setBannerFromFile(const std::string& anExtIcon,
                                  const std::string& path)
{
    setExtIcon(anExtIcon);
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    size_t read = fread(&banner(), 1, sizeof(tNDSBanner), f);
    fclose(f);
    return read == sizeof(tNDSBanner);
}
