/*
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "debugoverlay.h"

#include <nds.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <string>

#include "../../share/fifotool.h"
#include "font/fontfactory.h"
#include "fsmngr.h"
#include "gdi.h"
#include "globalsettings.h"
#include "inifile.h"

namespace debugOverlay {
namespace {
const char* const kDonorKeys[] = {
    "DONORTWL0_NDS_PATH",
    "DONORTWL_NDS_PATH",
    "DONORTWLONLY0_NDS_PATH",
    "DONORTWLONLY_NDS_PATH",
};

const int kPanelX = 2;
const int kPanelY = 2;
const int kPanelWidth = 252;
const int kPanelTextWidth = 238;
const size_t kTextBufferSize = 1024;

struct DebugState {
    bool initialized;
    bool dsiMode;
    bool is3DS;
    bool scfgKnown;
    bool scfgLocked;
    bool flashcart;
    u32 arm7System;
    bool akMenuLoader;
    bool bootstrapRelease;
    bool bootstrapNightly;
    bool bootstrapHomebrew;
    bool picoLoader7;
    bool picoLoader9;
    bool donorNeeded;
    bool donorConfigured;
    std::string donorKey;
    std::string donorPath;
    std::string nandStatus;
    std::string donorSearchStatus;
};

DebugState s_state = {};
char s_renderedText[kTextBufferSize] = {};
u16 s_panelHeight = 0;

bool fileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string yesNo(bool value) {
    return value ? "yes" : "no";
}

u32 requestArm7Value(u32 request, u32 responseChannel) {
    fifoSendValue32(FIFO_USER_01, request);
    fifoWaitValue32(responseChannel);
    return fifoGetValue32(responseChannel);
}

std::string consoleName() {
    if (s_state.is3DS) return "Nintendo 3DS";
    switch (s_state.arm7System) {
        case 1: return "Nintendo DS";
        case 2: return "Nintendo DSi";
        case 3: return "Nintendo DS Lite";
        default: return s_state.dsiMode ? "Nintendo DSi" : "Nintendo DS (unknown subtype)";
    }
}

std::string selectedNdsLoader() {
    if (gs().pico) return "Pico-Loader";
    return gs().nightly ? "nds-bootstrap nightly" : "nds-bootstrap release";
}

std::string selectedHomebrewLoader() {
    switch (gs().hbStrap) {
        case 1: return "nds-bootstrap-hb";
        case 2: return "Pico-Loader";
        default: return "AKMenu-Next";
    }
}

void appendWrappedLine(std::string& text, const std::string& line) {
    text += font().breakLine(line, kPanelTextWidth);
}

void rebuildText() {
    if (!gs().debug || !s_state.initialized) return;

    std::string text;
    appendWrappedLine(text, "AKMenu-Next debug");
    appendWrappedLine(text, "Console: " + consoleName());
    appendWrappedLine(text, std::string("Mode: ") + (s_state.dsiMode ? "DSi mode" : "DS mode"));
    appendWrappedLine(text, std::string("SCFG: ") +
                                    (s_state.scfgKnown
                                             ? (s_state.scfgLocked ? "locked" : "unlocked")
                                             : "not applicable"));
    appendWrappedLine(text, std::string("Storage: ") +
                                    (s_state.flashcart ? "flashcart" : "SD card"));
    appendWrappedLine(
            text, "Detected loaders: AKMenu=" + yesNo(s_state.akMenuLoader) +
                          ", bootstrap release=" + yesNo(s_state.bootstrapRelease) +
                          ", nightly=" + yesNo(s_state.bootstrapNightly) +
                          ", homebrew=" + yesNo(s_state.bootstrapHomebrew));
    appendWrappedLine(text, "Pico-Loader files: ARM7=" + yesNo(s_state.picoLoader7) +
                                    ", ARM9=" + yesNo(s_state.picoLoader9));
    appendWrappedLine(text, "Configured NDS loader: " + selectedNdsLoader());
    appendWrappedLine(text, "Configured homebrew loader: " + selectedHomebrewLoader());
    appendWrappedLine(text, "NAND access: " +
                                    (s_state.nandStatus.empty() ? "not checked"
                                                                : s_state.nandStatus));
    appendWrappedLine(text, "Donor search: " +
                                    (s_state.donorSearchStatus.empty()
                                             ? "not checked"
                                             : s_state.donorSearchStatus));
    appendWrappedLine(text, std::string("Donor needed: ") + yesNo(s_state.donorNeeded) +
                                    " | loaded into INI: " +
                                    yesNo(s_state.donorConfigured));
    appendWrappedLine(text, "Donor key: " +
                                    (s_state.donorKey.empty() ? "none" : s_state.donorKey));
    appendWrappedLine(text, "Donor path: " +
                                    (s_state.donorPath.empty() ? "<none configured>"
                                                               : s_state.donorPath));

    int lineCount = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n') ++lineCount;
    }
    const u32 lineHeight = std::max<u32>(gs().fontHeight, 8);
    const u16 panelHeight = (u16)std::min<u32>(
            SCREEN_HEIGHT - 4, (u32)lineCount * lineHeight + 10);

    const int oldIME = enterCriticalSection();
    std::memset(s_renderedText, 0, sizeof(s_renderedText));
    std::strncpy(s_renderedText, text.c_str(), sizeof(s_renderedText) - 1);
    s_panelHeight = panelHeight;
    leaveCriticalSection(oldIME);
}
}  // namespace

void initialize() {
    if (!gs().debug) return;

    s_state.dsiMode = isDSiMode();
    s_state.arm7System = requestArm7Value(MENU_MSG_SYSTEM, FIFO_USER_02);
    s_state.is3DS = requestArm7Value(MENU_MSG_IS_3DS, FIFO_USER_01) != 0;
    s_state.scfgKnown = s_state.dsiMode || s_state.is3DS || s_state.arm7System == 2;
    s_state.scfgLocked = s_state.scfgKnown && *(volatile u32*)0x02FFFDF0 == 0;
    s_state.flashcart = fsManager().isFlashcart();

    s_state.akMenuLoader =
            fileExists(fsManager().resolveSystemPath("/_nds/akmenunext/launcher.nds"));
    s_state.bootstrapRelease =
            fileExists(fsManager().resolveSystemPath("/_nds/nds-bootstrap-release.nds"));
    s_state.bootstrapNightly =
            fileExists(fsManager().resolveSystemPath("/_nds/nds-bootstrap-nightly.nds"));
    s_state.bootstrapHomebrew =
            fileExists(fsManager().resolveSystemPath("/_nds/nds-bootstrap-hb-release.nds"));
    s_state.picoLoader7 = fileExists(fsManager().resolveSystemPath("/_pico/picoLoader7.bin"));
    s_state.picoLoader9 = fileExists(fsManager().resolveSystemPath("/_pico/picoLoader9.bin"));
    s_state.initialized = true;

    CIniFile bootstrapIni;
    bootstrapIni.LoadIniFile(fsManager().resolveSystemPath("/_nds/nds-bootstrap.ini"));
    updateDonorStatus(bootstrapIni, true);
}

void updateDonorStatus(CIniFile& ini, bool allowAutoDiscovery) {
    if (!gs().debug || !s_state.initialized) return;

    s_state.donorNeeded = allowAutoDiscovery && s_state.dsiMode && !s_state.is3DS &&
                          s_state.scfgLocked;
    const bool dsiSoundCategory =
            s_state.donorNeeded && *(volatile u32*)0x02FFE1A0 == 0x080037C0;
    const int primaryKey = dsiSoundCategory ? 2 : 0;
    const int secondaryKey = dsiSoundCategory ? 3 : 1;

    s_state.donorConfigured = false;
    s_state.donorKey.clear();
    s_state.donorPath.clear();

    if (s_state.donorNeeded) {
        const int selectedKeys[] = {primaryKey, secondaryKey};
        for (size_t i = 0; i < sizeof(selectedKeys) / sizeof(selectedKeys[0]); ++i) {
            const int key = selectedKeys[i];
            const std::string path = ini.GetString("NDS-BOOTSTRAP", kDonorKeys[key], "");
            if (!path.empty()) {
                s_state.donorConfigured = true;
                s_state.donorKey = kDonorKeys[key];
                s_state.donorPath = path;
                break;
            }
        }
    } else {
        for (int i = 0; i < 4; ++i) {
            const std::string path = ini.GetString("NDS-BOOTSTRAP", kDonorKeys[i], "");
            if (!path.empty()) {
                s_state.donorConfigured = true;
                s_state.donorKey = kDonorKeys[i];
                s_state.donorPath = path;
                break;
            }
        }
    }

    // Show manual paths even when their donor category is inactive.
    if (s_state.donorPath.empty()) {
        for (int i = 0; i < 4; ++i) {
            const std::string path = ini.GetString("NDS-BOOTSTRAP", kDonorKeys[i], "");
            if (!path.empty()) {
                s_state.donorKey = kDonorKeys[i];
                s_state.donorPath = path;
                break;
            }
        }
    }

    rebuildText();
}

void setDonorSearchStatus(const char* nandStatus, const char* searchStatus) {
    if (!gs().debug || !s_state.initialized) return;
    if (nandStatus) s_state.nandStatus = nandStatus;
    if (searchStatus) s_state.donorSearchStatus = searchStatus;
    rebuildText();
}

void draw() {
    if (!gs().debug || !s_state.initialized || s_panelHeight == 0) return;

    gdi().fillRectBlend(RGB15(0, 0, 0), RGB15(0, 0, 0), kPanelX, kPanelY, kPanelWidth,
                        s_panelHeight, GE_SUB, 82);
    gdi().setPenColor(RGB15(24, 27, 31), GE_SUB);
    gdi().frameRect(kPanelX, kPanelY, kPanelWidth, s_panelHeight, GE_SUB);
    gdi().setPenColor(RGB15(31, 31, 31), GE_SUB);
    gdi().textOutRect(kPanelX + 6, kPanelY + 4, kPanelWidth - 12, s_panelHeight - 8,
                      s_renderedText, GE_SUB);
}
}  // namespace debugOverlay
