/*
    exptools.h
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds.h>
#include "singleton.h"

class cExpansion {
  public:
    enum TPages { ENorPage = 16, EPsramPage = 96 };

  public:
    static void OpenNorWrite(void);
    static void CloseNorWrite(void);
    static void SetRompage(u16 page);
    void SetRampage(u16 page);
    u16 Rampage(void);
    static void SetSerialMode(void);
    static void SetShake(u16 data);
    static void EnableBrowser(void);
    static void WritePSRAM(u32 address, const u8* buffer, u32 size);
    static void WriteSram(uint32 address, const u8* data, uint32 size);
    static void ReadSram(uint32 address, u8* data, uint32 size);

  public:
    void SoftReset(void);

  public:
    cExpansion() : iId(0), iRamPage(ENorPage) {
        SetShake(8);
        OpenNorWrite();
        ReadNorFlashID();
        ChipReset();
        CloseNorWrite();
    };
    void Block_Erase(u32 blockAdd);
    void WriteNorFlash(u32 address, const u8* buffer, u32 size);
    bool IsValid(void) { return iId; };

  private:
    void ReadNorFlashID(void);
    void ChipReset(void);

  private:
    enum { FlashBase = 0x08000000 };

  private:
    u32 iId;
    u16 iRamPage;
};

typedef t_singleton<cExpansion> cExpansion_s;
inline cExpansion& expansion() {
    return cExpansion_s::instance();
}

