/*
    iconassets.h
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <stddef.h>
#include <string>

// Load an icon asset from the selected custom source. Theme icons fall back to
// the global icon folder; callers provide the built-in fallback when needed.
bool loadIconAsset(const std::string& filename, void* destination, size_t size);
