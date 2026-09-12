/*
    theme.cpp
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "theme.h"

#include <algorithm>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>

#include "bmp15.h"
#include "dbgtool.h"
#include "globalsettings.h"
#include "inifile.h"
#include "systemfilenames.h"

namespace {
struct sThemeAsset {
    const char* filename;
    bool isScreen;
    bool isSmallIcon;
    u16 maxWidth;
    u16 maxHeight;
};

// These assets are used by the menu itself or by one of its built-in windows. Optional
// theme extensions, such as custom pictures and GBA artwork, are intentionally not included.
const sThemeAsset kRequiredThemeAssets[] = {
        {"upper_screen.bmp", true, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"lower_screen.bmp", true, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"title_left.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"title_bg.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"title_right.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"btn2.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"btn3.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"btn4.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"spin_btn_left.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"spin_btn_right.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"brightness.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"folder_up.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"menu_bg.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"calendar/clock_numbers.bmp", false, false, SCREEN_WIDTH, 1024},
        {"calendar/clock_colon.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"calendar/day_numbers.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"calendar/year_numbers.bmp", false, false, SCREEN_WIDTH, 1024},
        {"card_icon_blue.bmp", false, true, 32, 32},
        {"progress_wnd.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
        {"progress_bar.bmp", false, false, SCREEN_WIDTH, SCREEN_HEIGHT},
};

bool isDirectory(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool isSafeThemeName(const std::string& theme) {
    return !theme.empty() && theme != "." && theme != ".." &&
           theme.find('/') == std::string::npos && theme.find('\\') == std::string::npos;
}

bool isValidTheme(const std::string& theme) {
    if (!isSafeThemeName(theme)) return false;

    const std::string themeDirectory = SFN_UI_DIRECTORY + theme + "/";
    if (!isDirectory(themeDirectory)) return false;

    CIniFile settings;
    if (!settings.LoadIniFile(themeDirectory + "uisettings.ini") ||
        !settings.HasSection("global settings"))
        return false;

    for (size_t i = 0; i < sizeof(kRequiredThemeAssets) / sizeof(kRequiredThemeAssets[0]); ++i) {
        const sThemeAsset& asset = kRequiredThemeAssets[i];
        cBMP15 bitmap = createBMP15FromFile(themeDirectory + asset.filename);
        if (!bitmap.valid()) return false;

        if (asset.isScreen &&
            (bitmap.width() != SCREEN_WIDTH || bitmap.height() != SCREEN_HEIGHT))
            return false;

        if (bitmap.width() > asset.maxWidth || bitmap.height() > asset.maxHeight) return false;
    }

    return true;
}

bool sameTheme(const std::string& left, const std::string& right) {
    return strcasecmp(left.c_str(), right.c_str()) == 0;
}

std::string findReplacementTheme(const std::string& currentTheme) {
    const std::string defaultTheme = "blue skies";
    if (!sameTheme(currentTheme, defaultTheme) && isValidTheme(defaultTheme)) return defaultTheme;

    DIR* dir = opendir((SFN_UI_DIRECTORY).c_str());
    if (!dir) return std::string();

    std::vector<std::string> themes;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string theme(entry->d_name);
        if (!sameTheme(theme, currentTheme) && isValidTheme(theme)) themes.push_back(theme);
    }
    closedir(dir);

    std::sort(themes.begin(), themes.end());
    return themes.empty() ? std::string() : themes[0];
}
}  // namespace

eThemeSelectionResult ensureValidTheme() {
    if (isValidTheme(gs().uiName)) return THEME_SELECTION_VALID;

    const std::string replacement = findReplacementTheme(gs().uiName);
    if (replacement.empty()) {
        dbg_printf("No valid replacement theme found for '%s'\n", gs().uiName.c_str());
        return THEME_SELECTION_FAILED;
    }

    dbg_printf("Theme '%s' is invalid; using '%s'\n", gs().uiName.c_str(), replacement.c_str());
    gs().uiName = replacement;
    gs().saveSettings();
    return THEME_SELECTION_REPLACED;
}
