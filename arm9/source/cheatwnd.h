/*
    cheatwnd.h
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds/ndstypes.h>
#include <vector>

#include "cheat.h"
#include "button.h"
#include "form.h"
#include "formdesc.h"
#include "listview.h"
#include "statictext.h"

class cCheatWnd : public akui::cForm, public cCheat {
  public:
    cCheatWnd(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text);
    ~cCheatWnd();
    bool parse(const std::string& aFileName);

  protected:
    void draw();
    bool process(const akui::cMessage& msg);
    cWindow& loadAppearance(const std::string& aFileName);

  protected:
    bool processKeyMessage(const akui::cKeyMessage& msg);
    void onItemClicked(u32 index);
    void onSelect(void);
    void onDeselectAll(void);
    void onInfo(void);
    void onGenerate(void);
    void onCancel(void);
    void onDraw(const akui::cListView::cOwnerDraw& data);
    void drawMark(const akui::cListView::cOwnerDraw& od, u16 width);
    void generateList(void);
    akui::cButton _buttonDeselect;
    akui::cButton _buttonInfo;
    akui::cButton _buttonGenerate;
    akui::cButton _buttonCancel;
    akui::cFormDesc _renderDesc;
    akui::cListView _List;

  private:
    enum {
        EIconColumn = 0,
        ETextColumn = 1,
        EIconWidth = 15,
        EFolderWidth = 11,
        ERowHeight = 15,
        EFolderTop = 3,
        ESelectTop = 5
    };

  private:
    std::vector<size_t> _indexes;
};
