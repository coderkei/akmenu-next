/*
    cheatwnd.h
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <string>
#include <vector>

#include <nds/ndstypes.h>

class cCheat {
  public:
    bool parse(const std::string& aFileName);
    static bool searchCheatData(FILE* aDat, u32 gamecode, u32 crc32, long& aPos, size_t& aSize);
    static bool romData(const std::string& aFileName, u32& aGameCode, u32& aCrc32);
    std::vector<u32> getCheats();
    void writeCheatsToFile(const char* path);

  protected:
    bool parseInternal(FILE* aDat, u32 gamecode, u32 crc32);
    void deselectFolder(size_t anIndex);

  protected:
    struct sDatIndex {
        u32 _gameCode;
        u32 _crc32;
        u64 _offset;
    };
    class cParsedItem {
      public:
        std::string _title;
        std::string _comment;
        std::vector<u32> _cheat;
        u32 _flags;
        u32 _offset;
        cParsedItem(const std::string& title, const std::string& comment, u32 flags, u32 offset = 0)
            : _title(title), _comment(comment), _flags(flags), _offset(offset) {};
        enum { EFolder = 1, EInFolder = 2, EOne = 4, ESelected = 8, EOpen = 16 };
    };

  protected:
    std::vector<cParsedItem> _data;
    std::string _fileName;
};
