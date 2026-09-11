/*
    theme.h
    Copyright (C) 2026 AKMenu-Next contributors

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

enum eThemeSelectionResult {
    THEME_SELECTION_VALID,
    THEME_SELECTION_REPLACED,
    THEME_SELECTION_FAILED
};

eThemeSelectionResult ensureValidTheme();
