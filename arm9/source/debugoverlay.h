/*
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

class CIniFile;

namespace debugOverlay {
void initialize();
void updateDonorStatus(CIniFile& ini, bool allowAutoDiscovery);
void setDonorSearchStatus(const char* nandStatus, const char* searchStatus);
void draw();
}  // namespace debugOverlay
