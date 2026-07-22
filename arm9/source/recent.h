/*
    recent.h
    Copyright (C) coderkei 2026

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <string>

class cRecent {
  public:
    static bool AddToRecent(const std::string& aFileName);
    static bool UpdateRecent(const std::string& aOldFileName, const std::string& aNewFileName);
    static bool RemoveFromRecent(const std::string& aFileName);
};
