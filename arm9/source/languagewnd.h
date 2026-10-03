/*
    languagewnd.h
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

std::vector<std::string> installedLanguages();

class cLanguageWnd : public akui::cForm {
  public:
    cLanguageWnd(s32 x, s32 y, u32 w, u32 h, akui::cWindow* parent, const std::string& text,
                 const std::vector<std::string>& languages, const std::string& currentLanguage);
    ~cLanguageWnd();

    const std::string& selectedLanguage() const { return _selectedLanguage; }

  protected:
    void draw();
    bool process(const akui::cMessage& msg);
    akui::cWindow& loadAppearance(const std::string& aFileName);
    bool processKeyMessage(const akui::cKeyMessage& msg);

    void onSelect(u32 index = 0);
    void onOK();
    void onCancel();
    void onDraw(const akui::cListView::cOwnerDraw& data);
    void drawScrollIndicator();

  private:
    std::vector<std::string> _languages;
    std::string _selectedLanguage;

    akui::cButton _buttonOK;
    akui::cButton _buttonCancel;
    akui::cListView _list;
    akui::cFormDesc _renderDesc;
};
