/*
    userwnd.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "bmp15.h"
#include "datetime.h"
#include "point.h"
#include "singleton.h"
#include "window.h"

class cUserWindow : public akui::cWindow {
  public:
    cUserWindow();

    ~cUserWindow() {}

  public:
    void draw();

    akui::cWindow& loadAppearance(const std::string& aFileName) { return *this; }

  protected:
    void init();
    void drawBitmapDate();
    void drawDateField(int& x, u32 value, u8 digits);
    void drawDateNumber(int x, u8 number);

    cBMP15 _userPicture;
    int _px;
    int _py;
    std::string _userText;
    int _tx;
    int _ty;
    int _tw;
    int _th;
    u16 _userTextColor;
    std::string _userName;
    int _ux;
    int _uy;
    u16 _userNameColor;
    bool _showUserName;
    int _dateX;
    int _dateY;
    u16 _dateColor;
    bool _showDate;
    bool _showDateTime;
    bool _dateFont;
    std::string _dateFormat;
    cBMP15 _dateNumbers;
    bool _showCustomText;
    bool _showCustomPic;
};

typedef t_singleton<cUserWindow> UserWindow_s;
inline cUserWindow& userWindow() {
    return UserWindow_s::instance();
}
