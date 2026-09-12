/*
    iconassets.cpp
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "iconassets.h"

#include <stdio.h>
#include "globalsettings.h"
#include "systemfilenames.h"

namespace {
bool loadIconAssetFromPath(const std::string& path, void* destination, size_t size) {
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) return false;

    const size_t bytesRead = fread(destination, 1, size, file);
    fclose(file);
    return bytesRead == size;
}
}  // namespace

bool loadIconAsset(const std::string& filename, void* destination, size_t size) {
    if (gs().iconSource == cGlobalSettings::EIconTheme &&
        loadIconAssetFromPath(SFN_UI_ICONS_DIRECTORY + filename, destination, size))
        return true;

    if (gs().iconSource != cGlobalSettings::EIconBuiltIn &&
        loadIconAssetFromPath(SFN_ICONS_DIRECTORY + filename, destination, size))
        return true;

    return false;
}
