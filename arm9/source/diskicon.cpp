/*
    diskicon.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "diskicon.h"
#include "../../share/memtool.h"
#include "bmp15.h"
#include "globalsettings.h"
#include "inifile.h"
#include "systemfilenames.h"
#include "timetool.h"

using namespace akui;

cDiskIcon::cDiskIcon() : cWindow(NULL, "diskicon") {
    _size = cSize(0, 0);
    _position = cPoint(0, 0);
    _engine = GE_MAIN;
    _icon.init(1);
    _icon.setPosition(226, 174);
    _icon.setPriority(3);
    _icon.setBufferOffset(16);
    _icon.show();

    fillMemory(_icon.buffer(), 32 * 32 * 2, 0x00000000);
}

void cDiskIcon::draw() {
    // do nothing
}

cWindow& cDiskIcon::loadAppearance(const std::string& aFileName) {
    CIniFile ini(SFN_UI_SETTINGS);

    u16 x = ini.GetInt("disk icon", "x", 238);
    u16 y = ini.GetInt("disk icon", "y", 172);
    _icon.setPosition(x, y);

    cBMP15 icon = createBMP15FromFile(aFileName);

    if (icon.valid()) {
        u32 pitch = icon.pitch() >> 1;
        for (u8 i = 0; i < icon.height(); ++i) {
            for (u8 j = 0; j < icon.width(); ++j) {
                ((u16*)_icon.buffer())[i * 32 + j] = ((u16*)icon.buffer())[i * pitch + j];
            }
        }
    }
    dbg_printf("cDiskIcon::loadAppearance ok %d\n", icon.valid());
    return *this;
}

void cDiskIcon::blink(void) {
    if (_icon.visible())
        _icon.hide();
    else
        _icon.show();
}

void cDiskIcon::turnOn() {
    _icon.show();
}

void cDiskIcon::turnOff() {
    _icon.hide();
}
