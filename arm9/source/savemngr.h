/*
    savemngr.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds.h>
#include <string>
#include <vector>
#include "globalsettings.h"
#include "singleton.h"

typedef struct _SAVE_INFO_T {
    u8 gameTitle[12];
    u8 gameCode[4];
    PACKED u16 gameCRC;
    u8 saveType;
} SAVE_INFO;

enum SAVE_TYPE {
    ST_UNKNOWN = 0,
    ST_NOSAVE,
    ST_4K,
    ST_64K,
    ST_512K,
    ST_2M,
    ST_4M,
    ST_8M,
    ST_NEW,
    ST_AUTO,
    ST_1M,
    ST_16M,
    ST_32M,
    ST_64M
};

enum DISPLAY_SAVE_TYPE {
    D_ST_UNKNOWN = 0,
    D_ST_NOSAVE,
    D_ST_4K,
    D_ST_64K,
    D_ST_512K,
    D_ST_1M,
    D_ST_2M,
    D_ST_4M,
    D_ST_8M,
    D_ST_16M,
    D_ST_32M,
    D_ST_64M
};

// flags
#define SAVE_INFO_EX_RUMBLE 0x03
#define SAVE_INFO_EX_DOWNLOAD_PLAY 0x04
#define SAVE_INFO_EX_SOFT_RESET 0x08
#define SAVE_INFO_EX_CHEAT 0x10
#define SAVE_INFO_EX_SLOT_MASK 0x60
#define SAVE_INFO_EX_SLOT_SHIFT 5
#define SAVE_INFO_EX_DMA 0x80
#define SAVE_INFO_EX_COMPARE_SIZE 18
// flags2
#define SAVE_INFO_EX_GLOBAL_DOWNLOAD_PLAY (BIT(0))
#define SAVE_INFO_EX_GLOBAL_SOFT_RESET (BIT(1))
#define SAVE_INFO_EX_GLOBAL_CHEAT (BIT(2))
#define SAVE_INFO_EX_GLOBAL_DMA (BIT(3))
#define SAVE_INFO_EX_GLOBAL_MASK 0x0000000f
#define SAVE_INFO_EX_PROTECTION (BIT(4))
#define SAVE_INFO_EX_LINKAGE (BIT(5))
#define SAVE_INFO_EX_ICON_MASK 0x000000c0
#define SAVE_INFO_EX_ICON_SHIFT 6
#define SAVE_INFO_EX_ICON_TRANSPARENT 0
#define SAVE_INFO_EX_ICON_AS_IS 1
#define SAVE_INFO_EX_ICON_FIRMWARE 2
#define SAVE_INFO_EX_SD_SAVE (BIT(8))
#define SAVE_INFO_EX_GLOBAL_SD_SAVE (BIT(9))
#define SAVE_INFO_EX_LANGUAGE_MASK 0x00001c00
#define SAVE_INFO_EX_LANGUAGE_SHIFT 10
#define SAVE_INFO_EX_LOADER (BIT(13))
#define SAVE_INFO_EX_GLOBAL_LOADER (BIT(14))
#define SAVE_INFO_EX_NIGHTLY (BIT(15))
#define SAVE_INFO_EX_GLOBAL_NIGHTLY (BIT(16))

typedef struct SAVE_INFO_EX_T {
    u8 gameTitle[12];
    u8 gameCode[4];
    u16 gameCRC;
    u8 saveType;
    u8 flags;
    u32 flags2;
    u32 reserved[2];
    u8 getRumble(void) { return flags & SAVE_INFO_EX_RUMBLE; };
    u8 getDownloadPlay(void) {
        return getFlag(SAVE_INFO_EX_DOWNLOAD_PLAY, SAVE_INFO_EX_GLOBAL_DOWNLOAD_PLAY, false);
    };
    u8 getSoftReset(void) {
        return getFlag(SAVE_INFO_EX_SOFT_RESET, SAVE_INFO_EX_GLOBAL_SOFT_RESET, false);
    };
    u8 getCheat(void) { return getFlag(SAVE_INFO_EX_CHEAT, SAVE_INFO_EX_GLOBAL_CHEAT, false); };
    u8 getSlot(void) { return (flags & SAVE_INFO_EX_SLOT_MASK) >> SAVE_INFO_EX_SLOT_SHIFT; };
    u8 getDMA(void) { return getFlag(SAVE_INFO_EX_DMA, SAVE_INFO_EX_GLOBAL_DMA, false); };
    u8 getProtection(void) { return (flags2 & SAVE_INFO_EX_PROTECTION) ? 1 : 0; };
    u8 getLinkage(void) { return (flags2 & SAVE_INFO_EX_LINKAGE) ? 1 : 0; };
    u8 getIcon(void) { return (flags2 & SAVE_INFO_EX_ICON_MASK) >> SAVE_INFO_EX_ICON_SHIFT; };
    u8 getSDSave(void) { return getFlag(SAVE_INFO_EX_SD_SAVE, SAVE_INFO_EX_GLOBAL_SD_SAVE, true); };
    u8 getLanguage(void) {
        return (flags2 & SAVE_INFO_EX_LANGUAGE_MASK) >> SAVE_INFO_EX_LANGUAGE_SHIFT;
    }
    u8 getLoader(void) {
        return getFlag(SAVE_INFO_EX_LOADER, SAVE_INFO_EX_GLOBAL_LOADER, true);
    };
    u8 getNightly(void) {
        return getFlag(SAVE_INFO_EX_NIGHTLY, SAVE_INFO_EX_GLOBAL_NIGHTLY, true);
    };
    bool isDownloadPlay(void) {
        return getState(SAVE_INFO_EX_DOWNLOAD_PLAY, SAVE_INFO_EX_GLOBAL_DOWNLOAD_PLAY, false,
                        false);
    };
    bool isSoftReset(void) {
        return getState(SAVE_INFO_EX_SOFT_RESET, SAVE_INFO_EX_GLOBAL_SOFT_RESET, gs().softreset,
                        false);
    };
    bool isCheat(void) {
        return getState(SAVE_INFO_EX_CHEAT, SAVE_INFO_EX_GLOBAL_CHEAT, gs().cheats, false);
    };
    bool isDMA(void) {
        return getState(SAVE_INFO_EX_DMA, SAVE_INFO_EX_GLOBAL_DMA, gs().dma, false);
    };
    bool isProtection(void) { return (flags2 & SAVE_INFO_EX_PROTECTION) ? true : false; };
    bool isLinkage(void) { return (flags2 & SAVE_INFO_EX_LINKAGE) ? true : false; };
    bool isSDSave(void) {
        return getState(SAVE_INFO_EX_SD_SAVE, SAVE_INFO_EX_GLOBAL_SD_SAVE, gs().sdsave, true);
    };
    bool isNdsBootstrap(void) {
        return getState(SAVE_INFO_EX_LOADER, SAVE_INFO_EX_GLOBAL_LOADER,
                        gs().romLauncher, true);
    };
    void setFlags(u8 rumble, u8 downloadplay, u8 reset, u8 cheat, u8 slot, u8 dma, u8 protection,
                  u8 linkage, u8 icon, u8 sdsave, u8 language, u8 ndsbootstrap, u8 nightly) {
        flags = rumble & SAVE_INFO_EX_RUMBLE;
        flags2 = 0;
        setFlag(SAVE_INFO_EX_DOWNLOAD_PLAY, SAVE_INFO_EX_GLOBAL_DOWNLOAD_PLAY, downloadplay, false);
        setFlag(SAVE_INFO_EX_SOFT_RESET, SAVE_INFO_EX_GLOBAL_SOFT_RESET, reset, false);
        setFlag(SAVE_INFO_EX_CHEAT, SAVE_INFO_EX_GLOBAL_CHEAT, cheat, false);
        setFlag(SAVE_INFO_EX_DMA, SAVE_INFO_EX_GLOBAL_DMA, dma, false);
        flags |= (slot << SAVE_INFO_EX_SLOT_SHIFT) & SAVE_INFO_EX_SLOT_MASK;
        flags2 |= (protection ? SAVE_INFO_EX_PROTECTION : 0);
        flags2 |= (linkage ? SAVE_INFO_EX_LINKAGE : 0);
        flags2 |= (icon << SAVE_INFO_EX_ICON_SHIFT) & SAVE_INFO_EX_ICON_MASK;
        setFlag(SAVE_INFO_EX_SD_SAVE, SAVE_INFO_EX_GLOBAL_SD_SAVE, sdsave, true);
        flags2 |= (language << SAVE_INFO_EX_LANGUAGE_SHIFT) & SAVE_INFO_EX_LANGUAGE_MASK;
        setFlag(SAVE_INFO_EX_LOADER, SAVE_INFO_EX_GLOBAL_LOADER, ndsbootstrap, true);
        setFlag(SAVE_INFO_EX_NIGHTLY, SAVE_INFO_EX_GLOBAL_NIGHTLY, nightly, true);
    };
    u8 getFlag(u32 personal, u32 global, bool style) {
        return (flags2 & global) ? 2 : ((style ? (flags2 & personal) : (flags & personal)) ? 1 : 0);
    }
    void setFlag(u32 personal, u32 global, u8 value, bool style) {
        switch (value) {
            case 0:
                // do nothing;
                break;
            case 1: {
                if (style)
                    flags2 |= personal;
                else
                    flags |= personal;
            } break;
            default:
                flags2 |= global;
                break;
        }
    }
    bool getState(u32 personal, u32 global, bool globalState, bool style) {
        switch (getFlag(personal, global, style)) {
            case 0:
                return false;
            case 1:
                return true;
            default:
                return globalState;
        }
    }
    void defaults(void) {
        saveType = ST_UNKNOWN;
        flags = 0;
        flags2 = SAVE_INFO_EX_GLOBAL_SOFT_RESET | SAVE_INFO_EX_GLOBAL_CHEAT |
                 SAVE_INFO_EX_GLOBAL_DMA | SAVE_INFO_EX_GLOBAL_SD_SAVE |
                 SAVE_INFO_EX_GLOBAL_LOADER | SAVE_INFO_EX_GLOBAL_NIGHTLY;
        reserved[0] = reserved[1] = 0;
    };
} SAVE_INFO_EX;

#define SAVE_INFO_EX_HEADER_MAGIC 0x42474b41

typedef struct SAVE_INFO_EX_HEADER_T {
    u32 marker;
    u32 itemSize;
    u32 itemCount;
    u32 reserved;
} SAVE_INFO_EX_HEADER;

class cSaveManager {
  public:
    cSaveManager();

    ~cSaveManager();

  public:
    bool importSaveList(const std::string& customFile, const std::string& officialFile);

    bool exportCustomSaveList(const std::string& filename);

    void updateCustomSaveList(const SAVE_INFO_EX& aSaveInfo);

    bool saveLastInfo(const std::string& romFilename);

    bool loadLastInfo(std::string& lastLoadedFilename);

    bool clearLastInfo();

    static bool initializeSaveFile(const std::string& romFilename, u8 slot, u32 size);

    static std::string generateSaveName(const std::string& romFilename, u8 slot);

    SAVE_TYPE getSaveTypeByFile(const std::string& romFilename);

    void updateSaveInfoByInfo(SAVE_INFO_EX& gameInfo);

    static DISPLAY_SAVE_TYPE SaveTypeToDisplaySaveType(SAVE_TYPE aSaveType);
    static SAVE_TYPE DisplaySaveTypeToSaveType(DISPLAY_SAVE_TYPE aSaveType);

  protected:
    std::vector<SAVE_INFO> _saveList;

    std::vector<SAVE_INFO_EX> _customSaveList;
};

typedef t_singleton<cSaveManager> SaveManager_s;
inline cSaveManager& saveManager() {
    return SaveManager_s::instance();
}
