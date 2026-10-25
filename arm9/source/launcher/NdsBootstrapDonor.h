/*
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <string>

class CIniFile;

void initializeNdsBootstrapDonors();
void configureNdsBootstrapDonors(CIniFile& ini, bool is3DS, bool allowAutoDiscovery,
                                 const std::string& bootstrapIniPath = "");
