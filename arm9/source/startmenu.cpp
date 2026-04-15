/*
    startmenu.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "startmenu.h"
#include "inifile.h"
#include "language.h"
#include "mainlist.h"
#include "systemfilenames.h"
#include "windowmanager.h"

using namespace akui;

void cStartMenu::init() {
    addItem(START_MENU_ITEM_FAVORITES_ADD, LANG("start menu", "Set favorite"));
    addItem(START_MENU_ITEM_FAVORITES_DELETE, LANG("start menu", "Unset favorite"));
    addItem(START_MENU_ITEM_SETTING, LANG("start menu", "Setting"));
    addItem(START_MENU_ITEM_INFO, LANG("start menu", "Info"));
    addItem(START_MENU_ITEM_HELP, LANG("start menu", "Help"));
    addItem(START_MENU_ITEM_TOOLS, LANG("start menu", "Tools"));
    loadAppearance(SFN_UI_SETTINGS);
    dbg_printf("startmenu ok\n");
}

bool cStartMenu::process(const cMessage& msg) {
    if (msg.id() == cMessage::keyDown) {
        cKeyMessage& kmsg = (cKeyMessage&)msg;
        if (kmsg.keyCode() == cKeyMessage::UI_KEY_START) {
            hide();
            return false;
        }
    }
    return cPopMenu::process(msg);
}

cWindow& cStartMenu::loadAppearance(const std::string& aFileName) {
    _renderDesc->loadData(SFN_STARTMENU_BG);
    _size = _renderDesc->size();

    CIniFile ini(aFileName);
    // std::string bgFile = ini.GetString( "bg", "file",  );
    int ix = ini.GetInt("start menu", "itemX", 4);
    int iy = ini.GetInt("start menu", "itemY", 12);
    int x = ini.GetInt("start menu", "x", 4);
    int y = ini.GetInt("start menu", "y", 4);
    setPosition(cPoint(x, y));
    _itemTopLeftPoint = cPoint(ix, iy);
    _itemHeight = ini.GetInt("start menu", "itemHeight", 16);
    _itemWidth = ini.GetInt("start menu", "itemWidth", 0);
    _barLeft = ini.GetInt("start menu", "barLeft", 2);
    if (_itemWidth == 0 && _barLeft * 2 > _size.x) _barLeft = 0;
    return *this;
}
