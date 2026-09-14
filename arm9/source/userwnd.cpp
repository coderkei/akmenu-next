/*
    userwnd.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "userwnd.h"
#include <sys/stat.h>
#include <cstdio>
#include <cstring>
#include "../../share/memtool.h"
#include "globalsettings.h"
#include "inifile.h"
#include "stringtool.h"
#include "systemfilenames.h"
#include "unicode.h"
#include "windowmanager.h"

using namespace akui;

namespace {
void formatDate(char* output, size_t outputSize, const std::string& dateFormat) {
    if (dateFormat == "MM/DD/YYYY") {
        std::snprintf(output, outputSize, "%02d/%02d/%04d", datetime().month(),
                      datetime().day(), datetime().year());
        return;
    }
    if (dateFormat == "YYYY/MM/DD") {
        std::snprintf(output, outputSize, "%04d/%02d/%02d", datetime().year(),
                      datetime().month(), datetime().day());
        return;
    }

    // DD/MM/YYYY is the default and also the fallback for an unknown format.
    std::snprintf(output, outputSize, "%02d/%02d/%04d", datetime().day(), datetime().month(),
                  datetime().year());
}

void appendTime(char* output, size_t outputSize) {
    size_t dateLength = std::strlen(output);
    u8 hours = datetime().hours();
    const char* ampm = (hours < 12) ? "AM" : "PM";
    if (gs().show12hrClock) {
        if (hours > 12) hours -= 12;
        if (hours == 0) hours = 12;
        std::snprintf(output + dateLength, outputSize - dateLength, " %d:%02d %s", hours,
                      datetime().minutes(), ampm);
        return;
    }

    std::snprintf(output + dateLength, outputSize - dateLength, " %02d:%02d", hours,
                  datetime().minutes());
}
}  // namespace

cUserWindow::cUserWindow() : cWindow(NULL, "UserWindow") {
    _px = _py = 0;
    _tx = _ty = _tw = _th = 0;
    _ux = _uy = 0;
    _dateX = _dateY = 0;
    _userTextColor = 0;
    _userNameColor = 0;
    _dateColor = 0;
    _showUserName = false;
    _showDate = false;
    _showDateTime = true;
    _dateFont = false;
    _dateFormat = "DD/MM/YYYY";
    _size = cSize(1, 1);
    _position = cPoint(0, 0);
    _engine = GE_SUB;

    init();
}

void cUserWindow::init() {
    CIniFile ini(SFN_USER_CUSTOM);
    std::string pictureFilename = ini.GetString("custom picture", "file", "");
    if (pictureFilename != "") {
        struct stat st;
        if (0 == stat(pictureFilename.c_str(), &st)) {
            if (st.st_size <= 1024 * 10)  // 64 x 64 15bit bmp
                _userPicture = createBMP15FromFile(pictureFilename);
        }
    }
    _px = ini.GetInt("custom picture", "x", 0);
    _py = ini.GetInt("custom picture", "y", 0);

    dbg_printf("%s valid %d x=%d y=%d", pictureFilename.c_str(), _userPicture.valid(), _px, _py);
    _userText = ini.GetString("custom text", "text", "");
    _userTextColor = ini.GetInt("custom text", "color", 0);
    _tx = ini.GetInt("custom text", "x", 0);
    _ty = ini.GetInt("custom text", "y", 0);
    _tw = ini.GetInt("custom text", "w", 0);
    _th = ini.GetInt("custom text", "h", 0);

    _ux = ini.GetInt("user name", "x", _ux);
    _uy = ini.GetInt("user name", "y", _uy);
    _userNameColor = ini.GetInt("user name", "color", 0xFFFF);
    _showUserName = ini.GetInt("user name", "show", 0);
    _dateX = ini.GetInt("date", "x", 0);
    _dateY = ini.GetInt("date", "y", 0);
    _dateColor = ini.GetInt("date", "color", 0xFFFF);
    _showDate = ini.GetInt("date", "show", 0);
    _showDateTime = ini.GetInt("date", "showTime", _showDateTime);
    _dateFormat = ini.GetString("date", "format", _dateFormat);
    _dateFont = ini.GetInt("date", "font", 0) != 0;
    if (_dateFont) _dateNumbers = createBMP15FromFile(SFN_DATE_NUMBERS);
    _showCustomText = ini.GetInt("custom text", "show", 0);
    _showCustomPic = ini.GetInt("custom picture", "show", 0);
    _userName = unicode_to_local_string((u16*)PersonalData->name, PersonalData->nameLen, NULL);
}

void cUserWindow::drawDateNumber(int x, u8 number) {
    const u32 digitWidth = 9;
    const u32 digitHeight = 14;
    if (number > 9 || !_dateNumbers.valid() || _dateNumbers.width() != digitWidth ||
        _dateNumbers.height() < digitHeight * 10)
        return;

    const u32 pitch = _dateNumbers.pitch() >> 1;
    const u32* source = _dateNumbers.buffer() + number * pitch * digitHeight / 2;
    gdi().maskBlt(source, x, _dateY, digitWidth, digitHeight, _engine);
}

void cUserWindow::drawDateField(int& x, u32 value, u8 digits) {
    u32 factor = 1;
    for (u8 i = 1; i < digits; ++i) factor *= 10;

    for (u8 i = 0; i < digits; ++i) {
        drawDateNumber(x, (value / factor) % 10);
        x += 9;
        factor /= 10;
    }
}

void cUserWindow::drawBitmapDate() {
    int x = _dateX;
    const int fieldGap = 12;
    if (_dateFormat == "MM/DD/YYYY") {
        drawDateField(x, datetime().month(), 2);
        x += fieldGap;
        drawDateField(x, datetime().day(), 2);
        x += fieldGap;
        drawDateField(x, datetime().year(), 4);
        return;
    }
    if (_dateFormat == "YYYY/MM/DD") {
        drawDateField(x, datetime().year(), 4);
        x += fieldGap;
        drawDateField(x, datetime().month(), 2);
        x += fieldGap;
        drawDateField(x, datetime().day(), 2);
        return;
    }

    // DD/MM/YYYY is the default and fallback for an unknown format.
    drawDateField(x, datetime().day(), 2);
    x += fieldGap;
    drawDateField(x, datetime().month(), 2);
    x += fieldGap;
    drawDateField(x, datetime().year(), 4);
}

void cUserWindow::draw() {
    if (_showCustomPic && _userPicture.valid()) {
        gdi().maskBlt(_userPicture.buffer(), _px, _py, _userPicture.width(), _userPicture.height(),
                      _engine);
    }

    if (_showCustomText && _userText != "") {
        gdi().setPenColor(_userTextColor, _engine);
        gdi().textOutRect(_tx, _ty, _tw, _th, _userText.c_str(), _engine);
    }

    if (_showUserName && _userName != "") {
        gdi().setPenColor(_userNameColor, _engine);
        gdi().textOut(_ux, _uy, _userName.c_str(), _engine);
    }

    if (_showDate) {
        if (_dateFont) {
            drawBitmapDate();
        } else {
            char dateText[32];
            formatDate(dateText, sizeof(dateText), _dateFormat);
            if (_showDateTime) appendTime(dateText, sizeof(dateText));

            gdi().setPenColor(_dateColor, _engine);
            gdi().textOut(_dateX, _dateY, dateText, _engine);
        }
    }
}
