/*
    startmenu.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "popmenu.h"

#define START_MENU_ITEM_FAVORITES_ADD 0
#define START_MENU_ITEM_FAVORITES_DELETE 1
#define START_MENU_ITEM_INFO 2
#define START_MENU_ITEM_THEMES 3
#define START_MENU_ITEM_TOOLS 4
#define START_MENU_ITEM_SETTING 5
#define START_MENU_ITEM_HELP 6
#define START_MENU_ITEM_ABOUT 7

class cStartMenu : public akui::cPopMenu {
  public:
    cStartMenu(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text)
        : cPopMenu(x, y, w, h, parent, text) {}

    ~cStartMenu() {}

    void init();

    bool process(const akui::cMessage& msg);

    cWindow& loadAppearance(const std::string& aFileName);
};
