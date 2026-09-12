/*
    themewnd.h
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <string>
#include <vector>

#include <nds/ndstypes.h>

#include "button.h"
#include "form.h"
#include "formdesc.h"
#include "listview.h"
#include "statictext.h"

class cThemeWnd : public akui::cForm {
  public:
    cThemeWnd(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text);
    ~cThemeWnd();

    const std::string& selectedTheme() const { return _selectedTheme; }

  protected:
    void draw();
    bool process(const akui::cMessage& msg);
    cWindow& loadAppearance(const std::string& aFileName);
    bool processKeyMessage(const akui::cKeyMessage& msg);

    void onSelect(u32 index = 0);
    void onOK();
    void onCancel();
    void onDraw(const akui::cListView::cOwnerDraw& data);
    void drawMark(const akui::cListView::cOwnerDraw& data, u16 width);
    void drawScrollIndicator();
    void generateList();

  private:
    struct sThemeRow {
        bool folder;
        bool coverTheme;
        std::string theme;
    };

    enum {
        EIconColumn = 0,
        ETextColumn = 1,
        EIconWidth = 15,
        EFolderWidth = 11,
        ERowHeight = 15,
        EFolderTop = 3,
        ESelectTop = 4
    };

    bool _standardOpen;
    bool _coverOpen;
    std::string _selectedTheme;
    std::vector<std::string> _standardThemes;
    std::vector<std::string> _coverThemes;
    std::vector<sThemeRow> _rows;

    akui::cButton _buttonOK;
    akui::cButton _buttonCancel;
    akui::cStaticText _currentTheme;
    akui::cFormDesc _renderDesc;
    akui::cListView _list;
};
