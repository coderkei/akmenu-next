/*
    cheatwnd.cpp
    Portions copyright (C) 2008 Normmatt, www.normmatt.com, Smiths (www.emuholic.com)
    Portions copyright (C) 2008 bliss (bliss@hanirc.org)
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <string>

#include <fat.h>

#include "cheat.h"
#include "dbgtool.h"
#include "gamecode.h"
#include "systemfilenames.h"

#define CRCPOLY 0xedb88320
static u32 crc32(const u8* p, size_t len) {
    u32 crc = -1;
    while (len--) {
        crc ^= *p++;
        for (int ii = 0; ii < 8; ++ii) crc = (crc >> 1) ^ ((crc & 1) ? CRCPOLY : 0);
    }
    return crc;
}

bool cCheat::parse(const std::string& aFileName) {
    bool res = false;
    _fileName = aFileName;
    u32 romcrc32, gamecode;
    if (romData(_fileName, gamecode, romcrc32)) {
        FILE* dat = fopen((SFN_CHEATS).c_str(), "rb");
        if (dat) {
            res = parseInternal(dat, gamecode, romcrc32);
            fclose(dat);
        }
    }
    return res;
}

bool cCheat::romData(const std::string& aFileName, u32& aGameCode, u32& aCrc32) {
    bool res = false;
    FILE* rom = fopen(aFileName.c_str(), "rb");
    if (rom) {
        u8 header[512];
        if (1 == fread(header, sizeof(header), 1, rom)) {
            aCrc32 = crc32(header, sizeof(header));
            aGameCode = gamecode((const char*)(header + 12));
            res = true;
        }
        fclose(rom);
    }
    return res;
}

bool cCheat::searchCheatData(FILE* aDat, u32 gamecode, u32 crc32, long& aPos, size_t& aSize) {
    aPos = 0;
    aSize = 0;
    const char* KHeader = "R4 CheatCode";
    char header[12];
    fread(header, 12, 1, aDat);
    if (strncmp(KHeader, header, 12)) return false;

    sCheatDatIndex idx, nidx;

    fseek(aDat, 0, SEEK_END);
    long fileSize = ftell(aDat);

    fseek(aDat, 0x100, SEEK_SET);
    fread(&nidx, sizeof(nidx), 1, aDat);

    bool done = false;

    while (!done) {
        memcpy(&idx, &nidx, sizeof(idx));
        fread(&nidx, sizeof(nidx), 1, aDat);
        if (gamecode == idx._gameCode && crc32 == idx._crc32) {
            aSize = ((nidx._offset) ? nidx._offset : fileSize) - idx._offset;
            aPos = idx._offset;
            done = true;
        }
        if (!nidx._offset) done = true;
    }
    return (aPos && aSize);
}

bool cCheat::parseInternal(FILE* aDat, u32 gamecode, u32 crc32) {
    dbg_printf("%x, %x\n", gamecode, crc32);

    _data.clear();

    long dataPos;
    size_t dataSize;
    if (!searchCheatData(aDat, gamecode, crc32, dataPos, dataSize)) return false;
    fseek(aDat, dataPos, SEEK_SET);

    dbg_printf("record found: %d\n", dataSize);

    char* buffer = (char*)malloc(dataSize);
    if (!buffer) return false;
    fread(buffer, dataSize, 1, aDat);
    char* gameTitle = buffer;

    u32* ccode = (u32*)(((u32)gameTitle + strlen(gameTitle) + 4) & ~3);
    u32 cheatCount = *ccode;
    cheatCount &= 0x0fffffff;
    ccode += 9;

    u32 cc = 0;
    while (cc < cheatCount) {
        u32 folderCount = 1;
        char* folderName = NULL;
        char* folderNote = NULL;
        u32 flagItem = 0;
        if ((*ccode >> 28) & 1) {
            flagItem |= cCheatDatItem::EInFolder;
            if ((*ccode >> 24) == 0x11) flagItem |= cCheatDatItem::EOne;
            folderCount = *ccode & 0x00ffffff;
            folderName = (char*)((u32)ccode + 4);
            folderNote = (char*)((u32)folderName + strlen(folderName) + 1);
            _data.push_back(cCheatDatItem(folderName, folderNote, cCheatDatItem::EFolder));
            cc++;
            ccode = (u32*)(((u32)folderName + strlen(folderName) + 1 + strlen(folderNote) + 1 + 3) &
                           ~3);
        }

        u32 selectValue = cCheatDatItem::ESelected;
        for (size_t ii = 0; ii < folderCount; ++ii) {
            char* cheatName = (char*)((u32)ccode + 4);
            char* cheatNote = (char*)((u32)cheatName + strlen(cheatName) + 1);
            u32* cheatData = (u32*)(((u32)cheatNote + strlen(cheatNote) + 1 + 3) & ~3);
            u32 cheatDataLen = *cheatData++;

            if (cheatDataLen) {
                _data.push_back(cCheatDatItem(cheatName, cheatNote,
                                              flagItem | ((*ccode & 0xff000000) ? selectValue : 0),
                                              dataPos + (((char*)ccode + 3) - buffer)));
                if ((*ccode & 0xff000000) && (flagItem & cCheatDatItem::EOne)) selectValue = 0;
                _data.back()._cheat.resize(cheatDataLen);
                memcpy(_data.back()._cheat.data(), cheatData, cheatDataLen * 4);
            }
            cc++;
            ccode = (u32*)((u32)ccode + (((*ccode & 0x00ffffff) + 1) * 4));
        }
    }
    free(buffer);
    // generateList();
    return true;
}

void cCheat::deselectFolder(size_t anIndex) {
    std::vector<cCheatDatItem>::iterator itr = _data.begin() + anIndex;
    while (--itr >= _data.begin()) {
        if ((*itr)._flags & cCheatDatItem::EFolder) {
            ++itr;
            break;
        }
    }
    while (((*itr)._flags & cCheatDatItem::EInFolder) && itr != _data.end()) {
        (*itr)._flags &= ~cCheatDatItem::ESelected;
        ++itr;
    }
}

std::vector<cCheatDatItem> cCheat::getEnabledCheats() {
    std::vector<cCheatDatItem> cheats;
    for (uint i = 0; i < _data.size(); i++) {
        if (_data[i]._flags & cCheatDatItem::ESelected) {
            cheats.push_back(_data[i]);
        }
    }
    return cheats;
}

void cCheat::writeCheatsToFile(const char* path) {
    FILE* file = fopen(path, "wb");
    if (file) {
        std::vector<cCheatDatItem> cheats(getEnabledCheats());
        for (uint i = 0; i < cheats.size(); i++) {
            fwrite(cheats[i]._cheat.data(), 4, cheats[i]._cheat.size(), file);
        }
        fwrite("\0\0\0\xCF", 4, 1, file);
        fclose(file);
    }
}
