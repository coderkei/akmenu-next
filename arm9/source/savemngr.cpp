/*
    savemngr.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

//�

#include "savemngr.h"
#include <cerrno>
#include <fat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "datetime.h"
#include "dbgtool.h"
#include "dsrom.h"
#include "inifile.h"
#include "language.h"
#include "progresswnd.h"
#include "systemfilenames.h"
#include "ui.h"

#include "saves/ROMList.h"

using namespace akui;

cSaveManager::cSaveManager() {}

cSaveManager::~cSaveManager() {}

static bool loadSaveList(const std::string& filename, std::vector<SAVE_INFO_EX>& buffer) {
    u8* data = NULL;
    FILE* f = fopen(filename.c_str(), "rb");
    if (NULL == f) {
        dbg_printf("fopen %s fail\n", filename.c_str());
        return false;
    }
    bool res = false;
    do {
        u32 saveCount = 0;
        SAVE_INFO_EX_HEADER header = {SAVE_INFO_EX_HEADER_MAGIC, sizeof(SAVE_INFO), saveCount, 0};
        if (1 != fread(&header, sizeof(header), 1, f)) break;
        if (header.marker != SAVE_INFO_EX_HEADER_MAGIC) break;
        data = new u8[header.itemSize * header.itemCount];
        u32 copiedSize = std::min((unsigned int)header.itemSize, sizeof(SAVE_INFO_EX));
        if (header.itemCount != fread(data, header.itemSize, header.itemCount, f)) break;
        buffer.resize(header.itemCount);
        for (u32 ii = 0; ii < header.itemCount; ii++) {
            memset(&buffer[ii], 0, sizeof(SAVE_INFO_EX));
            memcpy(&buffer[ii], data + ii * header.itemSize, copiedSize);
        }
        res = true;
    } while (false);
    fclose(f);
    delete[] data;
    return res;
}

static bool loadOfficialSaveList(const std::string& filename, std::vector<SAVE_INFO>& buffer) {
    FILE* f = fopen(filename.c_str(), "rb");
    if (NULL == f) {
        dbg_printf("fopen %s fail\n", filename.c_str());
        return false;
    }
    fseek(f, 0, SEEK_END);
    u32 filesize = ftell(f);
    if (0 == filesize || 0 != (filesize % 19)) {
        dbg_printf("%s size error %d\n", filename.c_str(), filesize);
        fclose(f);
        f = NULL;
        return false;
    }

    u32 saveCount = filesize / 19;
    buffer.resize(saveCount);

    fseek(f, 0, SEEK_SET);
    u32 readed = fread(&buffer[0], 1, filesize, f);
    if (filesize != readed) {
        dbg_printf("%s read length error %d\n", filename.c_str(), readed);
        fclose(f);
        f = NULL;
        return false;
    }
    fclose(f);
    return true;
}

bool cSaveManager::importSaveList(const std::string& customFile, const std::string& officialFile) {
    loadSaveList(customFile, _customSaveList);
    loadOfficialSaveList(officialFile, _saveList);
    return true;
}

bool cSaveManager::exportCustomSaveList(const std::string& filename) {
    int f = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
    if (f < 0) {
        dbg_printf("fopen %s fail\n", filename.c_str());
        return false;
    }
    bool res = false;
    do {
        u32 count = _customSaveList.size();
        SAVE_INFO_EX_HEADER header = {SAVE_INFO_EX_HEADER_MAGIC, sizeof(SAVE_INFO_EX), count, 0};
        // libelm hack
        lseek(f, sizeof(header) + sizeof(SAVE_INFO_EX) * count, SEEK_SET);
        lseek(f, 0, SEEK_SET);
        //
        ssize_t written = write(f, &header, sizeof(header));
        if (written != sizeof(header)) {
            dbg_printf("%s write length error %d\n", filename.c_str(), written);
            break;
        }
        written = write(f, &_customSaveList[0], sizeof(SAVE_INFO_EX) * count);
        if (static_cast<ssize_t>(sizeof(SAVE_INFO_EX) * count) != written) {
            dbg_printf("%s write length error %d\n", filename.c_str(), written);
            break;
        }
        res = true;
    } while (false);
    close(f);
    return res;
}

void cSaveManager::updateCustomSaveList(const SAVE_INFO_EX& aSaveInfo) {
    size_t i = 0;
    for (i = 0; i < _customSaveList.size(); ++i) {
        if (0 == memcmp(&_customSaveList[i], &aSaveInfo, SAVE_INFO_EX_COMPARE_SIZE)) {
            _customSaveList[i] = aSaveInfo;
            break;
        }
    }
    if (i == _customSaveList.size()) _customSaveList.push_back(aSaveInfo);

    exportCustomSaveList(SFN_CUSTOM_SAVELIST);
}

bool cSaveManager::saveLastInfo(const std::string& romFilename, const std::string& favorite) {
    CIniFile f;
    f.SetString("Save Info", "lastLoaded", romFilename);
    f.SetString("Save Info", "favorite", favorite);
    if (!f.SaveIniFile(SFN_LAST_SAVEINFO)) return false;

    return true;
}

bool cSaveManager::loadLastInfo(std::string& lastLoadedFilename) {
    CIniFile f;
    if (!f.LoadIniFile(SFN_LAST_SAVEINFO)) {
        lastLoadedFilename = "";
        return false;
    }

    lastLoadedFilename = f.GetString("Save Info", "lastLoaded", "");
    if ("" == lastLoadedFilename) return false;

    return true;
}

bool cSaveManager::clearLastInfo() {
    std::string loadLoadedFile;
    if (loadLastInfo(loadLoadedFile)) {
        return saveLastInfo("","");
    }
    return true;
}

bool cSaveManager::initializeSaveFile(const std::string& romFilename, u8 slot, u32 size) {
    bool res = false;
    std::string saveFilename = generateSaveName(romFilename, slot);
    int f = open(saveFilename.c_str(), O_WRONLY | O_CREAT);
    if (f >= 0) {
        off_t filesize = lseek(f, 0, SEEK_END);
        if (filesize >= 0) {
            if ((u32)filesize >= size) {
                write(f, NULL, 0);  // update time stamp
                res = true;
            } else {
                progressWnd().setTipText(LANG("progress window", "processing save"));
                progressWnd().show();
                progressWnd().setPercent(0);
                u32 tail = size - filesize;
                const u32 bufferSize = 128 * 1024;
                u32 ii = 0;
                u8* buffer = new u8[bufferSize];
                memset(buffer, 0, bufferSize);
                res = true;
                while (tail > 0) {
                    ssize_t writtenSize = (tail > bufferSize) ? bufferSize : tail;
                    if (write(f, buffer, writtenSize) != writtenSize) {
                        res = false;
                        break;
                    }
                    tail -= writtenSize;
                    ii++;
                    if (0 == ii % 8) progressWnd().setPercent((lseek(f, 0, SEEK_END)) * 100 / size);
                }
                progressWnd().setPercent(100);
                progressWnd().hide();
            }
        }
        close(f);
    }
    return res;
}

std::string cSaveManager::saveLocation(const std::string& romFilename) {
    std::string location = romFilename;
    if (gs().saveDir) location.insert(location.find_last_of("/\\") + 1, "saves/");
    return location;
}

std::string cSaveManager::copySaveFile(const std::string& source, const std::string& target,
                                      bool replace) {
    const std::string writeError = lang().GetString(
            "saveslot", "write error", "Could not write the copy. Check free space and card access.");
    const std::string backup = target + ".bak";
    const std::string temporary = target + ".tmp";
    struct stat st;
    if (stat(backup.c_str(), &st) == 0 || errno != ENOENT) {
        return lang().GetString("saveslot", "backup exists",
                                "Check the target save's .bak file before trying again.");
    }
    if (stat(target.c_str(), &st) == 0) {
        if (!replace || !S_ISREG(st.st_mode)) return writeError;
    } else if (errno != ENOENT || replace) {
        return writeError;
    }

    FILE* input = fopen(source.c_str(), "rb");
    if (!input) {
        return lang().GetString("saveslot", "read error",
                                "Could not read the source save. Check the save file and card.");
    }
    int output = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (output < 0) {
        const int openError = errno;
        fclose(input);
        if (openError == EEXIST) {
            return lang().GetString("saveslot", "temporary exists",
                                    "Check the target save's .tmp file before trying again.");
        }
        return writeError;
    }

    progressWnd().setTipText(lang().GetString("saveslot", "copying", "Copying save..."));
    progressWnd().show();
    progressWnd().setPercent(0);
    std::vector<u8> buffer(32 * 1024);
    size_t count;
    u64 copied = 0;
    unsigned lastPercent = 0;
    const u64 size = fstat(fileno(input), &st) == 0 ? st.st_size : 0;
    bool written = true;
    while ((count = fread(buffer.data(), 1, buffer.size(), input)) != 0) {
        if (write(output, buffer.data(), count) != static_cast<ssize_t>(count)) {
            written = false;
            break;
        }
        copied += count;
        if (size) {
            const unsigned percent = std::min<u64>(copied * 100 / size, 100);
            if (percent != lastPercent) {
                progressWnd().setPercent(percent);
                lastPercent = percent;
            }
        }
    }
    const bool readError = ferror(input) || (written && size && copied != size);
    fclose(input);
    if (close(output) != 0) written = false;
    progressWnd().hide();
    if (readError || !written) {
        remove(temporary.c_str());
        if (readError) {
            return lang().GetString("saveslot", "read error",
                                    "Could not read the source save. Check the save file and card.");
        }
        return writeError;
    }

    if (replace && rename(target.c_str(), backup.c_str()) != 0) {
        remove(temporary.c_str());
        return writeError;
    }
    if (rename(temporary.c_str(), target.c_str()) != 0) {
        remove(temporary.c_str());
        if (replace && rename(backup.c_str(), target.c_str()) != 0) {
            return lang().GetString("saveslot", "restore error",
                                    "Copy failed. Restore the target save from its .bak file.");
        }
        return writeError;
    }
    if (replace && remove(backup.c_str()) != 0) {
        return lang().GetString("saveslot", "cleanup error",
                                "Save copied. The previous target save is still in its .bak file.");
    }
    if (gs().saveSlotOverride >= 0) {
        return lang().GetString("saveslot", "copied with override",
                                "Save copied. Check Save Slot Override before playing the target slot.");
    }
    return lang().GetString("saveslot", "copied", "Save copied. Choose the target slot to play it.");
}

std::string cSaveManager::generateSaveName(const std::string& romFilename, u8 slot) {
    std::string saveFilename(romFilename);
    if (gs().saveExt && saveFilename.length() > 3) {
        size_t nameLength = saveFilename.length() - 4;
        if (saveFilename[nameLength] == '.') saveFilename = saveFilename.substr(0, nameLength);
    }
    if (slot) {
        saveFilename += ".";
        saveFilename += (slot + '0');
    }
    saveFilename += ".sav";
    return saveFilename;
}

SAVE_TYPE cSaveManager::getSaveTypeByFile(const std::string& romFilename) {
    return ST_UNKNOWN;
}

SAVE_TYPE MelonDSROMListToSaveType[] = {ST_NOSAVE, ST_4K, ST_64K, ST_512K, ST_1M, ST_2M,
                                        ST_4M,     ST_8M, ST_16M, ST_32M,  ST_64M};

void cSaveManager::updateSaveInfoByInfo(SAVE_INFO_EX& gameInfo) {
    size_t saveCount = _customSaveList.size();
    for (size_t i = 0; i < saveCount; ++i) {
        if (0 == memcmp(&gameInfo, &_customSaveList[i], SAVE_INFO_EX_COMPARE_SIZE)) {
            gameInfo = _customSaveList[i];
            return;
        }
    }

    gameInfo.defaults();
    u32 gameCode;
    memcpy(&gameCode, gameInfo.gameCode, sizeof(gameCode));  // because alignment

    for (size_t i = 0; i < ROMListEntryCount; i++) {
        const ROMListEntry* entry = &ROMList[i];
        if (gameCode == entry->GameCode) {
            if (entry->SaveMemType == 0xFFFFFFFF)
                gameInfo.saveType = ST_AUTO;
            else
                gameInfo.saveType = MelonDSROMListToSaveType[entry->SaveMemType];
            break;
        }
    }
    return;
}

DISPLAY_SAVE_TYPE cSaveManager::SaveTypeToDisplaySaveType(SAVE_TYPE aSaveType) {
    switch (aSaveType) {
        case ST_UNKNOWN:
            return D_ST_UNKNOWN;
        case ST_NOSAVE:
            return D_ST_NOSAVE;
        case ST_4K:
            return D_ST_4K;
        case ST_64K:
            return D_ST_64K;
        case ST_512K:
            return D_ST_512K;
        case ST_2M:
            return D_ST_2M;
        case ST_4M:
            return D_ST_4M;
        case ST_8M:
            return D_ST_8M;
        case ST_NEW:
            return D_ST_UNKNOWN;
        case ST_AUTO:
            return D_ST_UNKNOWN;
        case ST_1M:
            return D_ST_1M;
        case ST_16M:
            return D_ST_16M;
        case ST_32M:
            return D_ST_32M;
        case ST_64M:
            return D_ST_64M;
        default:
            return D_ST_UNKNOWN;
    }
}

SAVE_TYPE cSaveManager::DisplaySaveTypeToSaveType(DISPLAY_SAVE_TYPE aSaveType) {
    switch (aSaveType) {
        case D_ST_UNKNOWN:
            return ST_UNKNOWN;
        case D_ST_NOSAVE:
            return ST_NOSAVE;
        case D_ST_4K:
            return ST_4K;
        case D_ST_64K:
            return ST_64K;
        case D_ST_512K:
            return ST_512K;
        case D_ST_1M:
            return ST_1M;
        case D_ST_2M:
            return ST_2M;
        case D_ST_4M:
            return ST_4M;
        case D_ST_8M:
            return ST_8M;
        case D_ST_16M:
            return ST_16M;
        case D_ST_32M:
            return ST_32M;
        case D_ST_64M:
            return ST_64M;
    }
    return ST_UNKNOWN;
}
