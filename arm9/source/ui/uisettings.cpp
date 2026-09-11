/*
    uisettings.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin
    Copyright (C) 2016 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "uisettings.h"
#include "inifile.h"
#include "ui.h"

cUISettings::cUISettings() {
    showCalendar = true;
    coverX = -32768;
    coverY = -32768;
    coverDarken = 0;
    coverFade = 0;
    formFrameColor = RGB15(23, 25, 4);
    formBodyColor = RGB15(30, 29, 22);
    formTextColor = RGB15(17, 12, 0);
    formTitleTextColor = RGB15(11, 11, 11);
    buttonTextColor = RGB15(17, 12, 0);
    spinBoxNormalColor = RGB15(0, 0, 31);
    spinBoxFocusColor = RGB15(0, 31, 0);
    spinBoxTextColor = RGB15(31, 31, 31);
    spinBoxTextHighLightColor = RGB15(31, 31, 31);
    spinBoxFrameColor = RGB15(11, 11, 11);
    listViewBarColor1 = RGB15(0, 11, 19);
    listViewBarColor2 = RGB15(0, 5, 9);
    listTextColor = 0;
    listTextHighLightColor = 0;
    popMenuTextColor = RGB15(0, 0, 0);
    popMenuTextHighLightColor = RGB15(31, 31, 31);
    popMenuBarColor = RGB15(0, 11, 19);
    thickness = 1;
}

cUISettings::~cUISettings() {}

void cUISettings::loadSettings() {
    CIniFile ini(SFN_UI_SETTINGS);

    showCalendar = ini.GetInt("global settings", "showCalendar", showCalendar);
    coverX = ini.GetInt("cover", "x", coverX);
    coverY = ini.GetInt("cover", "y", coverY);
    coverDarken = ini.GetInt("cover", "darken", coverDarken);
    coverFade = ini.GetInt("cover", "fade", coverFade);
    if (coverDarken < 0) coverDarken = 0;
    if (coverDarken > 100) coverDarken = 100;
    if (coverFade < 0) coverFade = 0;
    if (coverFade > 100) coverFade = 100;
    formFrameColor = ini.GetInt("global settings", "formFrameColor", formFrameColor);
    formBodyColor = ini.GetInt("global settings", "formBodyColor", formBodyColor);
    formTextColor = ini.GetInt("global settings", "formTextColor", formTextColor);
    formTitleTextColor = ini.GetInt("global settings", "formTitleTextColor", formTitleTextColor);
    buttonTextColor = ini.GetInt("global settings", "buttonTextColor", buttonTextColor);
    spinBoxNormalColor = ini.GetInt("global settings", "spinBoxNormalColor", spinBoxNormalColor);
    spinBoxFocusColor = ini.GetInt("global settings", "spinBoxFocusColor", spinBoxFocusColor);
    spinBoxTextColor = ini.GetInt("global settings", "spinBoxTextColor", spinBoxTextColor);
    spinBoxTextHighLightColor =
            ini.GetInt("global settings", "spinBoxTextHiLightColor", spinBoxTextHighLightColor);
    spinBoxFrameColor = ini.GetInt("global settings", "spinBoxFrameColor", spinBoxFrameColor);
    listViewBarColor1 = ini.GetInt("global settings", "listViewBarColor1", listViewBarColor1);
    listViewBarColor2 = ini.GetInt("global settings", "listViewBarColor2", listViewBarColor2);
    listTextColor = ini.GetInt("global settings", "listTextColor", listTextColor);
    listTextHighLightColor =
            ini.GetInt("global settings", "listTextHighLightColor", listTextHighLightColor);
    popMenuTextColor = ini.GetInt("global settings", "popMenuTextColor", popMenuTextColor);
    popMenuTextHighLightColor =
            ini.GetInt("global settings", "popMenuTextHighLightColor", popMenuTextHighLightColor);
    popMenuBarColor = ini.GetInt("global settings", "popMenuBarColor", popMenuBarColor);
    thickness = ini.GetInt("global settings", "thickness", thickness);
}
