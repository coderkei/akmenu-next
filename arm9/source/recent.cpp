/*
    recent.cpp
    Copyright (C) coderkei 2026

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "recent.h"
#include <vector>
#include "inifile.h"
#include "systemfilenames.h"

bool cRecent::AddToRecent(const std::string& aFileName) {
    if (aFileName.empty()) return false;
    CIniFile ini(SFN_RECENT);
    std::vector<std::string> items;
    ini.GetStringVector("main", "list", items, '|');

    // Remove if already present so it gets re-inserted at the top :3
    for (size_t ii = 0; ii < items.size(); ++ii) {
        if (items[ii] == aFileName) {
            items.erase(items.begin() + ii);
            break;
        }
    }

    items.insert(items.begin(), aFileName);
    if (items.size() > 10) {
        items.resize(10);
    }

    ini.SetStringVector("main", "list", items, '|');
    ini.SaveIniFile(SFN_RECENT);
    return true;
}

bool cRecent::UpdateRecent(const std::string& aOldFileName, const std::string& aNewFileName) {
    CIniFile ini(SFN_RECENT);
    std::vector<std::string> items;
    ini.GetStringVector("main", "list", items, '|');
    for (size_t ii = 0; ii < items.size(); ++ii) {
        if (items[ii] == aOldFileName) {
            items[ii] = aNewFileName;
            ini.SetStringVector("main", "list", items, '|');
            ini.SaveIniFile(SFN_RECENT);
            return true;
        }
    }
    return false;
}

bool cRecent::RemoveFromRecent(const std::string& aFileName) {
    CIniFile ini(SFN_RECENT);
    std::vector<std::string> items;
    ini.GetStringVector("main", "list", items, '|');
    for (size_t ii = 0; ii < items.size(); ++ii) {
        if (items[ii] == aFileName) {
            items.erase(items.begin() + ii);
            ini.SetStringVector("main", "list", items, '|');
            ini.SaveIniFile(SFN_RECENT);
            return true;
        }
    }
    return false;
}
