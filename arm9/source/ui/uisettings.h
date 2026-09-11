/*
    uisettings.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds.h>
#include "gdi.h"
#include "singleton.h"

class cUISettings {
  public:
    cUISettings();

    ~cUISettings();

  public:
    void loadSettings();

  public:
    bool showCalendar;

    int coverX;
    int coverY;
    int coverDarken;
    int coverFade;

    COLOR formFrameColor;
    COLOR formBodyColor;
    COLOR formTextColor;
    COLOR formTitleTextColor;
    COLOR buttonTextColor;
    COLOR spinBoxNormalColor;
    COLOR spinBoxFocusColor;
    COLOR spinBoxTextColor;
    COLOR spinBoxTextHighLightColor;
    COLOR spinBoxFrameColor;
    COLOR listViewBarColor1;
    COLOR listViewBarColor2;
    COLOR listTextColor;
    COLOR listTextHighLightColor;
    COLOR popMenuTextColor;
    COLOR popMenuTextHighLightColor;
    COLOR popMenuBarColor;

    u32 thickness;
};

typedef t_singleton<cUISettings> uiSettings_s;
inline cUISettings& uiSettings() {
    return uiSettings_s::instance();
}
inline cUISettings& uis() {
    return uiSettings_s::instance();
}
