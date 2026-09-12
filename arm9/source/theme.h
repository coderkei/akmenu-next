/*
    theme.h
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <string>
#include <vector>

enum eThemeSelectionResult {
    THEME_SELECTION_VALID,
    THEME_SELECTION_REPLACED,
    THEME_SELECTION_FAILED
};

eThemeSelectionResult ensureValidTheme();
std::vector<std::string> installedThemes(bool coverThemes);
