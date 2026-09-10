/*
    settingwnd.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <string>
#include "form.h"
#include "formdesc.h"
#include "message.h"
#include "spinbox.h"
#include "statictext.h"

class cSettingWnd : public akui::cForm {
  public:
    cSettingWnd(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text);
    ~cSettingWnd();

  public:
    void draw(void);
    bool process(const akui::cMessage& msg);
    cWindow& loadAppearance(const std::string& aFileName);
    void addSettingTab(const std::string& text);
    void addSettingItem(const std::string& text, const std::vector<std::string>& itemTexts,
                        size_t defaultValue);
    ssize_t getItemSelection(size_t tabId, size_t itemId);
    void setConfirmMessage(const std::string& text);
    template <class TargetType>
    void addYButton(std::string title, TargetType* slotHolder,
                    void (TargetType::*aMemberFunction)()) {
        _buttonY.setText("\x04 " + title);
        _buttonY.clicked.connect(slotHolder, aMemberFunction);
        _buttonY.show();
    }

  protected:
    enum {
        MAX_VISIBLE_ITEMS = 5,
        ITEM_PITCH = 20,
        SETTING_WINDOW_HEIGHT = 165,
        SCROLL_INDICATOR_Y = 124
    };

    struct sSetingItem {
        akui::cStaticText* _label;
        akui::cSpinBox* _item;
        sSetingItem(akui::cStaticText* label, akui::cSpinBox* item) : _label(label), _item(item){};
    };
    struct sSetingTab {
        std::vector<sSetingItem>* _tab;
        std::string _title;
        size_t _firstVisibleItem;
        sSetingTab(std::vector<sSetingItem>* tab, const std::string& title)
            : _tab(tab), _title(title), _firstVisibleItem(0){};
    };

  protected:
    void onOK(void);
    void onCancel(void);
    void onShow(void);
    void onUIKeyUP(void);
    void onUIKeyDOWN(void);
    void onUIKeyLEFT(void);
    void onUIKeyRIGHT(void);
    void onUIKeyL(void);
    void onUIKeyR(void);
    bool processKeyMessage(const akui::cKeyMessage& msg);
    void onItemChanged(akui::cSpinBox* item);
    ssize_t focusedItemId(void);
    akui::cSpinBox* focusedItem(void);

    void HideTab(size_t index);
    void ShowTab(size_t index);
    void SwitchTab(size_t oldIndex, size_t newIndex);
    void updateTabLayout(size_t index, bool showItems);
    void focusItem(size_t index);
    void repositionButtons(void);

    std::vector<sSetingItem>& items(size_t index) { return *_tabs[index]._tab; };

    u32 _maxLabelLength;
    u32 _spinBoxWidth;
    bool _simpleTabs;
    std::vector<sSetingTab> _tabs;
    size_t _currentTab;
    std::string _confirmMessage;

    akui::cSpinBox _tabSwitcher;
    akui::cButton _buttonOK;
    akui::cButton _buttonCancel;
    akui::cFormDesc _renderDesc;
    akui::cButton _buttonY;
};
