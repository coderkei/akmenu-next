/*
    mainwnd.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "button.h"
#include "form.h"
#include "keymessage.h"
#include "mainlist.h"
#include "settingwnd.h"
#include "spinbox.h"
#include "startmenu.h"
#include "touchmessage.h"

class cMainWnd : public akui::cForm {
  public:
    cMainWnd(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text);

    ~cMainWnd();

  public:
    bool process(const akui::cMessage& msg);

    cWindow& loadAppearance(const std::string& aFileName);

    void init();

    void draw();

    cWindow* windowBelow(const akui::cPoint& p);

    cMainList* _mainList;

  protected:
    void onMainListSelItemClicked(u32 index);

    void onMainListSelItemHeadClicked(u32 index);

    void onKeyAPressed();

    void onKeyBPressed();

    void onKeyXPressed();

    void onKeyYPressed();

    void listSelChange(u32 i);

    void refreshCover();

    void startMenuItemClicked(s16 i);

    void startButtonClicked();

    void brightnessButtonClicked();

    bool processKeyMessage(const akui::cKeyMessage& msg);

    bool processTouchMessage(const akui::cTouchMessage& msg);

    void setParam(void);

    void showSettings(void);

    void showThemes(void);

    void onFolderChanged();

    void onAnimation(bool& anAllow);

    void showFileInfo();

    void launchSelected();

    cStartMenu* _startMenu;

    akui::cButton* _startButton;

    akui::cButton* _brightnessButton;

    akui::cButton* _folderUpButton;

    akui::cStaticText* _folderText;

    bool _processL;
};
