/*
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later

    Donor selection and regional fallback data adapted from
    DS-Homebrew/TWiLightMenu, title/arm9/source/main.cpp:
    https://github.com/DS-Homebrew/TWiLightMenu
*/

#include "NdsBootstrapDonor.h"

#include <fat.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <string>

#include <nds.h>

#include "../fsmngr.h"
#include "../inifile.h"
#include "../nand/nandio.h"

namespace {
const char* const kDonorKeys[] = {
    "DONORTWL0_NDS_PATH",
    "DONORTWL_NDS_PATH",
    "DONORTWLONLY0_NDS_PATH",
    "DONORTWLONLY_NDS_PATH",
};

enum DsiRegion {
    REGION_JAPAN = 0,
    REGION_USA = 1,
    REGION_EUROPE = 2,
    REGION_AUSTRALIA = 3,
    REGION_CHINA = 4,
    REGION_KOREA = 5,
};

static bool isRegularFile(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

static bool isNandPath(const std::string& path) {
    return path.size() >= 5 &&
           (path[0] == 'n' || path[0] == 'N') &&
           (path[1] == 'a' || path[1] == 'A') &&
           (path[2] == 'n' || path[2] == 'N') &&
           (path[3] == 'd' || path[3] == 'D') && path[4] == ':';
}

static bool mountNand(void) {
    static bool nandMounted = false;
    if (nandMounted) return true;
    if (!fatMountSimple("nand", &io_dsi_nand)) {
        nandMounted = access("nand:/", F_OK) == 0;
        return nandMounted;
    }
    nandMounted = access("nand:/", F_OK) == 0;
    return nandMounted;
}

static bool readTmdAppIndex(u32 titleType, u32 titleId, int& appIndex) {
    char tmdPath[96];
    const int pathLength = snprintf(tmdPath, sizeof(tmdPath),
                                    "nand:/title/%08lx/%08lx/content/title.tmd",
                                    (unsigned long)titleType, (unsigned long)titleId);
    if (pathLength <= 0 || (size_t)pathLength >= sizeof(tmdPath)) return false;

    FILE* tmd = fopen(tmdPath, "rb");
    if (!tmd) return false;

    const bool seekOk = fseek(tmd, 0x1E7, SEEK_SET) == 0;
    const int value = seekOk ? fgetc(tmd) : EOF;
    fclose(tmd);
    if (value == EOF) return false;

    appIndex = value;
    return true;
}

static bool makeAppPath(u32 titleType, u32 titleId, int appIndex,
                        std::string& appPath) {
    char path[112];
    const int pathLength = snprintf(path, sizeof(path),
                                    "nand:/title/%08lx/%08lx/content/0000000%d.app",
                                    (unsigned long)titleType, (unsigned long)titleId,
                                    appIndex);
    if (pathLength <= 0 || (size_t)pathLength >= sizeof(path)) return false;

    appPath.assign(path);
    return isRegularFile(appPath);
}

static bool findInstalledApp(u32 titleType, u32 titleId, std::string& appPath) {
    int appIndex = 0;
    return readTmdAppIndex(titleType, titleId, appIndex) &&
           makeAppPath(titleType, titleId, appIndex, appPath);
}

static bool readSystemApp(std::string& appPath, u32& arm7Length) {
    // These are the title ID words used by TWiLight Menu++'s SCFG-locked path.
    const u32 titleId = *(vu32*)0x02000428;
    const u32 titleType = *(vu32*)0x0200042C;
    if (!findInstalledApp(titleType, titleId, appPath)) return false;

    FILE* app = fopen(appPath.c_str(), "rb");
    if (!app) return false;

    const bool seekOk = fseek(app, 0x3C, SEEK_SET) == 0;
    const bool readOk = seekOk && fread(&arm7Length, sizeof(arm7Length), 1, app) == 1;
    fclose(app);
    return readOk;
}

static bool isSdk50DonorLength(u32 arm7Length, bool dsiSoundCategory) {
    if (dsiSoundCategory) {
        return arm7Length == 0x26CC8 || arm7Length == 0x28E54;
    }
    return arm7Length == 0x29EE8;
}

static bool isSdk5DonorLength(u32 arm7Length, bool dsiSoundCategory) {
    static const u32 kDsiSoundCategoryLengths[] = {
        0x1D43C, 0x1D5A8, 0x1E1E8, 0x1E22C, 0x25664,
        0x257DC, 0x25860, 0x268DC, 0x26BA8, 0x26C5C,
        0x26D10, 0x26D48, 0x26D50, 0x26DF4, 0x27FB4,
    };
    static const u32 kRegularCategoryLengths[] = {
        0x22B40, 0x22BCC, 0x28F84, 0x2909C, 0x2914C, 0x29164,
        0x2A2EC, 0x2A318, 0x2AF18, 0x2B184, 0x2B24C, 0x2C5B4,
    };

    const u32* lengths = dsiSoundCategory ? kDsiSoundCategoryLengths
                                          : kRegularCategoryLengths;
    const size_t count = dsiSoundCategory
                             ? sizeof(kDsiSoundCategoryLengths) / sizeof(kDsiSoundCategoryLengths[0])
                             : sizeof(kRegularCategoryLengths) / sizeof(kRegularCategoryLengths[0]);
    for (size_t i = 0; i < count; ++i) {
        if (arm7Length == lengths[i]) return true;
    }
    return false;
}

static int readNandRegion(void) {
    FILE* hwinfo = fopen("nand:/sys/HWINFO_S.dat", "rb");
    if (!hwinfo) return -1;

    const bool seekOk = fseek(hwinfo, 0x90, SEEK_SET) == 0;
    const int region = seekOk ? fgetc(hwinfo) : EOF;
    fclose(hwinfo);

    if (region < REGION_JAPAN || region > REGION_KOREA) return -1;
    return region;
}

static u32 dsiSoundTitleId(int region) {
    switch (region) {
        case REGION_JAPAN: return 0x484E4B4A;
        case REGION_USA: return 0x484E4B45;
        case REGION_EUROPE: return 0x484E4B50;
        case REGION_AUSTRALIA: return 0x484E4B55;
        case REGION_CHINA: return 0x484E4B43;
        case REGION_KOREA: return 0x484E4B4B;
        default: return 0;
    }
}

static bool findRegionalSdk50Donor(int region, std::string& appPath) {
    // Matches TWiLight Menu++'s installed SDK 5.0 DSiWare donor order and
    // regional availability. A zero ID means that title is unavailable there.
    static const u32 kRegionalTitleIds[][6] = {
        {0x4B44394A, 0x4B443945, 0x4B443956, 0x4B443956, 0x4B443943, 0},
        {0x4B41444A, 0x4B414445, 0x4B414456, 0x4B414456, 0, 0},
        {0x4B54524A, 0x4B545254, 0, 0x4B545254, 0, 0},
        {0x4B545054, 0, 0, 0, 0, 0},
        {0x4B544254, 0, 0, 0, 0, 0},
    };
    const u32 kDsiWareTitleType = 0x00030004;
    const size_t titleCount = sizeof(kRegionalTitleIds) / sizeof(kRegionalTitleIds[0]);

    if (region < REGION_JAPAN || region > REGION_KOREA) return false;
    for (size_t i = 0; i < titleCount; ++i) {
        const u32 titleId = kRegionalTitleIds[i][region];
        if (titleId != 0 && findInstalledApp(kDsiWareTitleType, titleId, appPath)) {
            return true;
        }
    }
    return false;
}

static bool findDsiSoundDonor(int region, std::string& appPath) {
    const u32 titleId = dsiSoundTitleId(region);
    return titleId != 0 && findInstalledApp(0x00030005, titleId, appPath);
}
}  // namespace

void configureNdsBootstrapDonors(CIniFile& ini, bool is3DS, bool allowAutoDiscovery) {
    const bool isDsiMode = isDSiMode();
    const bool restrictedDsi = allowAutoDiscovery && isDsiMode && !is3DS &&
                               *(vu32*)0x02FFFDF0 == 0;
    const bool dsiSoundCategory = restrictedDsi && *(vu32*)0x02FFE1A0 == 0x080037C0;

    const std::string bootstrapIniPath =
        fsManager().resolveSystemPath("/_nds/nds-bootstrap.ini");
    CIniFile previousIni;
    const bool havePreviousIni = previousIni.LoadIniFile(bootstrapIniPath);
    std::string previousPaths[4];
    bool replaceMissing[4] = {false, false, false, false};
    bool needNandMount = false;

    const int sdk50Key = dsiSoundCategory ? 2 : 0;
    const int sdk5Key = dsiSoundCategory ? 3 : 1;

    for (int i = 0; i < 4; ++i) {
        if (havePreviousIni) {
            previousPaths[i] = previousIni.GetString("NDS-BOOTSTRAP", kDonorKeys[i], "");
        }
        if (!previousPaths[i].empty() && isNandPath(previousPaths[i]) && isDsiMode) {
            needNandMount = true;
        }
    }

    if (restrictedDsi) {
        const int selectedKeys[] = {sdk50Key, sdk5Key};
        for (size_t i = 0; i < sizeof(selectedKeys) / sizeof(selectedKeys[0]); ++i) {
            const int key = selectedKeys[i];
            if (previousPaths[key].empty() || isNandPath(previousPaths[key]) ||
                !isRegularFile(previousPaths[key])) {
                needNandMount = true;
            }
        }
    }

    bool nandMounted = false;
    if (needNandMount && isDsiMode) nandMounted = mountNand();

    for (int i = 0; i < 4; ++i) {
        const std::string& path = previousPaths[i];
        if (path.empty()) continue;

        const bool nandPath = isNandPath(path);
        if ((!nandPath || nandMounted) && isRegularFile(path)) {
            ini.SetString("NDS-BOOTSTRAP", kDonorKeys[i], path);
        } else if (nandPath && isDsiMode && !nandMounted) {
            // Keep an uncheckable NAND setting if the mount failed. Replacing
            // it without being able to inspect the file could discard a valid
            // manual path.
            ini.SetString("NDS-BOOTSTRAP", kDonorKeys[i], path);
        } else if (restrictedDsi && (i == sdk50Key || i == sdk5Key)) {
            replaceMissing[i] = true;
        }
    }

    if (!restrictedDsi || !nandMounted) return;

    const bool needsSdk50Donor = previousPaths[sdk50Key].empty() || replaceMissing[sdk50Key];
    const bool needsSdk5Donor = previousPaths[sdk5Key].empty() || replaceMissing[sdk5Key];
    if (!needsSdk50Donor && !needsSdk5Donor) return;

    std::string systemAppPath;
    u32 systemAppArm7Length = 0;
    const bool haveSystemApp = readSystemApp(systemAppPath, systemAppArm7Length);

    if (needsSdk50Donor) {
        bool donorFound = false;
        if (haveSystemApp && isSdk50DonorLength(systemAppArm7Length, dsiSoundCategory)) {
            ini.SetString("NDS-BOOTSTRAP", kDonorKeys[sdk50Key], systemAppPath);
            donorFound = true;
        }

        if (!donorFound) {
            const int region = readNandRegion();
            std::string alternateDonor;
            const bool foundAlternate = dsiSoundCategory
                                            ? findDsiSoundDonor(region, alternateDonor)
                                            : findRegionalSdk50Donor(region, alternateDonor);
            if (foundAlternate) {
                ini.SetString("NDS-BOOTSTRAP", kDonorKeys[sdk50Key], alternateDonor);
            }
        }
    }

    if (needsSdk5Donor && haveSystemApp &&
        isSdk5DonorLength(systemAppArm7Length, dsiSoundCategory)) {
        ini.SetString("NDS-BOOTSTRAP", kDonorKeys[sdk5Key], systemAppPath);
    }
}
