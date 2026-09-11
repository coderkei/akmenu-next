/*
    coverwnd.h
    Nintendo DS cover renderer.
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds.h>
#include <string>
#include <vector>

#include "dsrom.h"
#include "singleton.h"

class cCoverWnd {
  public:
    cCoverWnd();

    void update(const std::string& selectedPath, DSRomInfo& romInfo);
    void clear();
    void drawBackdrop() const;
    void draw() const;

  private:
    bool isSupportedDsRom(const std::string& selectedPath, DSRomInfo& romInfo) const;
    bool loadCover(const std::string& selectedPath, DSRomInfo& romInfo);
    bool tryLoad(const std::string& filename);

    std::string _selectedPath;
    std::vector<u16> _pixels;
    u16 _width;
    u16 _height;
};

typedef t_singleton<cCoverWnd> coverWnd_s;
inline cCoverWnd& coverWindow() {
    return coverWnd_s::instance();
}
