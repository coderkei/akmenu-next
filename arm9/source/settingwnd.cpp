/*
    settingwnd.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "settingwnd.h"
#include "globalsettings.h"
#include "language.h"
#include "msgbox.h"
#include "systemfilenames.h"
#include "gdi.h"
#include "fontfactory.h"
#include "uisettings.h"
#include "windowmanager.h"
#define TOP_MARGIN 4

using namespace akui;

namespace {
class cSettingButton : public cButton {
  public:
    cSettingButton(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text)
        : cButton(x, y, w, h, parent, text) {}

    void draw() {
        const u16 fillColor = isFocused() ? uis().spinBoxFocusColor : uis().spinBoxNormalColor;
        const u16 textColor = isFocused() ? uis().spinBoxTextHighLightColor
                                          : uis().spinBoxTextColor;
        const s16 x = position().x;
        const s16 y = position().y;
        const u16 width = size().x;
        const u16 height = size().y;
        gdi().setPenColor(fillColor, selectedEngine());
        gdi().fillRect(fillColor, fillColor, x, y, width, height, selectedEngine());
        gdi().setPenColor(uis().spinBoxFrameColor, selectedEngine());
        gdi().frameRect(x, y, width, height, uis().thickness, selectedEngine());
        gdi().setPenColor(textColor, selectedEngine());
        const u16 textAreaWidth = width - 6;
        u32 textWidth = font().getStringScreenWidth(text().c_str(), text().size());
        if (textWidth > textAreaWidth) textWidth = textAreaWidth;
        const s16 textX = x + 3 + (textAreaWidth - textWidth) / 2;
        const s16 textY = y + (height - gs().fontHeight) / 2;
        gdi().textOutRect(textX, textY, textAreaWidth, gs().fontHeight, text().c_str(),
                          selectedEngine());
    }
};
}  // namespace

static void drawScrollChevron(s16 x, s16 y, bool up, u16 color, GRAPHICS_ENGINE engine) {
    gdi().setPenColor(color, engine);
    if (up) {
        gdi().drawLine(x, y + 4, x + 4, y, engine);
        gdi().drawLine(x + 4, y, x + 8, y + 4, engine);
    } else {
        gdi().drawLine(x, y, x + 4, y + 4, engine);
        gdi().drawLine(x + 4, y + 4, x + 8, y, engine);
    }
}

cSettingWnd::cSettingWnd(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text)
    : cForm(x, y, w, h, parent, text),
      _tabSwitcher(0, 0, w, 18, this, "spin"),
      _buttonOK(0, 0, 46, 18, this, "\x01 OK"),
      _buttonCancel(0, 0, 48, 18, this, "\x02 Cancel"),
      _buttonY(0, 0, 76, 18, this, "\x04 Common") {
    _tabSwitcher.loadAppearance("");
    _tabSwitcher.changed.connect(this, &cSettingWnd::onItemChanged);
    addChildWindow(&_tabSwitcher);
    _tabSwitcher.insertItem(_text, 0);
    _tabSwitcher.selectItem(0);
    _tabSwitcher.hide();
    _tabSwitcher.disableFocus();

    s16 buttonY = size().y - _buttonCancel.size().y - 4;

    _buttonCancel.setStyle(cButton::press);
    _buttonCancel.setText("\x02 " + LANG("setting window", "cancel"));
    _buttonCancel.setTextColor(uis().buttonTextColor);
    _buttonCancel.loadAppearance(SFN_BUTTON3);
    _buttonCancel.clicked.connect(this, &cSettingWnd::onCancel);
    addChildWindow(&_buttonCancel);

    _buttonOK.setStyle(cButton::press);
    _buttonOK.setText("\x01 " + LANG("setting window", "ok"));
    _buttonOK.setTextColor(uis().buttonTextColor);
    _buttonOK.loadAppearance(SFN_BUTTON3);
    _buttonOK.clicked.connect(this, &cSettingWnd::onOK);
    addChildWindow(&_buttonOK);

    _buttonY.setStyle(cButton::press);
    _buttonY.setTextColor(uis().buttonTextColor);
    _buttonY.loadAppearance(SFN_BUTTON4);
    addChildWindow(&_buttonY);
    _buttonY.hide();
    _buttonY.disableFocus();

    s16 nextButtonX = size().x;
    s16 buttonPitch = _buttonCancel.size().x + 8;
    nextButtonX -= buttonPitch;
    _buttonCancel.setRelativePosition(cPoint(nextButtonX, buttonY));

    buttonPitch = _buttonOK.size().x + 8;
    nextButtonX -= buttonPitch;
    _buttonOK.setRelativePosition(cPoint(nextButtonX, buttonY));

    buttonPitch = _buttonY.size().x + 8;
    nextButtonX -= buttonPitch;
    _buttonY.setRelativePosition(cPoint(nextButtonX, buttonY));

    loadAppearance("");
    arrangeChildren();

    _tabs.push_back(sSetingTab(new std::vector<sSetingItem>, text));
    _currentTab = 0;
    _maxLabelLength = 0;
    CIniFile ini(SFN_UI_SETTINGS);
    _spinBoxWidth = ini.GetInt("setting window", "spinBoxWidth", 108);
    _simpleTabs = ini.GetInt("setting window", "simpleTabs", 0);
    _confirmMessage = LANG("setting window", "confirm text");

    // Keep a fixed-sized viewport so every theme has the same five-row settings area.
    // The original implementation grew the dialog for every row, eventually placing
    // the lower rows and buttons outside the 192-pixel DS screen.
    setSize(cSize(_size.x, SETTING_WINDOW_HEIGHT));
    _position.x = (SCREEN_WIDTH - _size.x) / 2;
    _position.y = (SCREEN_HEIGHT - _size.y) / 2;
    repositionButtons();
    arrangeChildren();
}

void cSettingWnd::setConfirmMessage(const std::string& text) {
    _confirmMessage = text;
}

cSettingWnd::~cSettingWnd() {
    for (size_t ii = 0; ii < _tabs.size(); ii++) {
        for (size_t jj = 0; jj < items(ii).size(); jj++) {
            delete items(ii)[jj]._label;
            delete items(ii)[jj]._item;
            delete items(ii)[jj]._button;
        }
        delete _tabs[ii]._tab;
    }
}

void cSettingWnd::draw(void) {
    _renderDesc.draw(windowRectangle(), _engine);
    cForm::draw();

    if (_currentTab < _tabs.size() && items(_currentTab).size() > MAX_VISIBLE_ITEMS) {
        const size_t totalItems = items(_currentTab).size();
        const size_t maxFirstVisible = totalItems - MAX_VISIBLE_ITEMS;
        const size_t firstVisible =
                _tabs[_currentTab]._firstVisibleItem > maxFirstVisible
                        ? maxFirstVisible
                        : _tabs[_currentTab]._firstVisibleItem;
        const bool canScrollUp = firstVisible > 0;
        const bool canScrollDown = firstVisible < maxFirstVisible;

        // Use a small vertical scrollbar cue instead of the old "1-5/7" range.
        // Both arrows are always present, while the themed focus color identifies
        // the directions that can currently be used.
        const s16 indicatorX = position().x + size().x / 2 - 8;
        const s16 indicatorY = position().y + SCROLL_INDICATOR_Y;
        const u16 activeColor = uis().spinBoxTextColor;
        const u16 inactiveColor = uis().spinBoxNormalColor;
        const u16 frameColor = uis().spinBoxFrameColor;

        drawScrollChevron(indicatorX, indicatorY, true,
                          canScrollUp ? activeColor : inactiveColor, _engine);
        drawScrollChevron(indicatorX, indicatorY + 8, false,
                          canScrollDown ? activeColor : inactiveColor, _engine);

        // A short themed track and thumb give a subtle indication of position
        // without adding another theme asset or taking space from the controls.
        const s16 trackX = indicatorX + 12;
        const u16 trackHeight = 12;
        const u16 thumbHeight = 4;
        gdi().setPenColor(frameColor, _engine);
        gdi().frameRect(trackX, indicatorY, 5, trackHeight, _engine);
        const s16 thumbY = indicatorY +
                           (s16)((firstVisible * (trackHeight - thumbHeight)) /
                                  maxFirstVisible);
        gdi().fillRect(activeColor, activeColor, trackX, thumbY, 5, thumbHeight, _engine);
    }
}

bool cSettingWnd::process(const akui::cMessage& msg) {
    bool ret = false;
    ret = cForm::process(msg);
    if (!ret) {
        if (msg.id() > cMessage::keyMessageStart && msg.id() < cMessage::keyMessageEnd) {
            ret = processKeyMessage((cKeyMessage&)msg);
        }
        // if(msg.id()>cMessage::touchMessageStart&&msg.id()<cMessage::touchMessageEnd)
        //{
        //   ret=processTouchMessage((cTouchMessage&)msg);
        // }
    }
    return ret;
}

void cSettingWnd::onOK(void) {
    u32 ret =
            messageBox(this, LANG("setting window", "confirm"), _confirmMessage, MB_OK | MB_CANCEL);
    if (ID_OK != ret) return;
    _modalRet = 1;
}

void cSettingWnd::onCancel(void) {
    _modalRet = 0;
}

bool cSettingWnd::processKeyMessage(const cKeyMessage& msg) {
    bool ret = false;
    if (msg.id() == cMessage::keyDown) {
        switch (msg.keyCode()) {
            case cKeyMessage::UI_KEY_DOWN:
                onUIKeyDOWN();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_UP:
                onUIKeyUP();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_LEFT:
                onUIKeyLEFT();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_RIGHT:
                onUIKeyRIGHT();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_A:
                if (focusedButton())
                    focusedButton()->clicked();
                else
                    onOK();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_B:
                onCancel();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_Y: {
                _buttonY.clicked();
            }
                ret = true;
                break;
            case cKeyMessage::UI_KEY_L:
                onUIKeyL();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_R:
                onUIKeyR();
                ret = true;
                break;
            default:
                break;
        }
    }
    return ret;
}

cWindow& cSettingWnd::loadAppearance(const std::string& aFileName) {
    _renderDesc.loadData(SFN_FORM_TITLE_L, SFN_FORM_TITLE_R, SFN_FORM_TITLE_M);
    _renderDesc.setTitleText(_text);
    return *this;
}

void cSettingWnd::addSettingTab(const std::string& text) {
    if (1 == _tabs.size()) {
        if (_simpleTabs) _renderDesc.loadData("", "", "");
        _renderDesc.setTitleText("");
    }
    _tabSwitcher.insertItem(text, _tabs.size());
    _tabs.push_back(sSetingTab(new std::vector<sSetingItem>, text));
    _tabSwitcher.show();
}

void cSettingWnd::addSettingItem(const std::string& text, const std::vector<std::string>& itemTexts,
                                 size_t defaultValue) {
    if (0 == itemTexts.size()) return;
    if (defaultValue >= itemTexts.size()) defaultValue = 0;
    if (_maxLabelLength < text.length()) _maxLabelLength = text.length();
    size_t lastTab = _tabs.size() - 1;

    cSpinBox* item = new cSpinBox(0, 0, 108, 18, this, "spin");
    for (size_t ii = 0; ii < itemTexts.size(); ++ii) {
        item->insertItem(itemTexts[ii], ii);
    }

    item->loadAppearance("");
    item->setSize(cSize(_spinBoxWidth, 18));
    item->setRelativePosition(cPoint(_size.x - _spinBoxWidth - 4, 0));
    item->hide();
    addChildWindow(item);
    item->selectItem(defaultValue);

    cStaticText* label = new cStaticText(0, 0, _maxLabelLength * 6, gs().fontHeight, this, text);
    label->setRelativePosition(cPoint(8, 0));
    label->setTextColor(uis().formTextColor);
    label->setSize(cSize(_size.x / 2 + 8, 12));
    label->hide();
    addChildWindow(label);

    items(lastTab).push_back(sSetingItem(label, item, NULL));
    updateTabLayout(lastTab, lastTab == _currentTab);
    repositionButtons();
    arrangeChildren();
}

akui::cButton* cSettingWnd::addSettingButton(const std::string& text,
                                              const std::string& buttonText) {
    if (_maxLabelLength < text.length()) _maxLabelLength = text.length();
    const size_t lastTab = _tabs.size() - 1;

    cSettingButton* button = new cSettingButton(0, 0, _spinBoxWidth, 18, this, buttonText);
    button->setStyle(cButton::press);
    button->setTextColor(uis().spinBoxTextColor);
    button->hide();
    addChildWindow(button);

    cStaticText* label = new cStaticText(0, 0, _maxLabelLength * 6, gs().fontHeight, this, text);
    label->setRelativePosition(cPoint(8, 0));
    label->setTextColor(uis().formTextColor);
    label->setSize(cSize(_size.x / 2 + 8, 12));
    label->hide();
    addChildWindow(label);

    items(lastTab).push_back(sSetingItem(label, NULL, button));
    updateTabLayout(lastTab, lastTab == _currentTab);
    repositionButtons();
    arrangeChildren();
    return button;
}

void cSettingWnd::onShow(void) {
    ShowTab(_currentTab);
}

void cSettingWnd::onUIKeyUP(void) {
    if (items(_currentTab).empty()) return;

    ssize_t focus = focusedItemId();
    size_t next = (focus < 0) ? items(_currentTab).size() - 1
                               : (focus == 0 ? items(_currentTab).size() - 1 : focus - 1);
    focusItem(next);
}

void cSettingWnd::onUIKeyDOWN(void) {
    if (items(_currentTab).empty()) return;

    ssize_t focus = focusedItemId();
    size_t next = (focus < 0) ? 0
                               : (focus + 1 >= (ssize_t)items(_currentTab).size() ? 0 : focus + 1);
    focusItem(next);
}

void cSettingWnd::onUIKeyLEFT(void) {
    cSpinBox* item = focusedItem();
    if (item) item->selectPrev();
}

void cSettingWnd::onUIKeyRIGHT(void) {
    cSpinBox* item = focusedItem();
    if (item) item->selectNext();
}

void cSettingWnd::onUIKeyL(void) {
    _tabSwitcher.selectPrev();
}

void cSettingWnd::onUIKeyR(void) {
    _tabSwitcher.selectNext();
}

ssize_t cSettingWnd::getItemSelection(size_t tabId, size_t itemId) {
    if (tabId >= _tabs.size()) return -1;
    if (itemId >= items(tabId).size()) return -1;
    if (!items(tabId)[itemId]._item) return -1;
    return items(tabId)[itemId]._item->selectedItemId();
}

/*
0.. - item focused
-1 - something else focused
*/
ssize_t cSettingWnd::focusedItemId(void) {
    ssize_t focusItem = -1;
    for (size_t ii = 0; ii < items(_currentTab).size(); ++ii) {
        sSetingItem& item = items(_currentTab)[ii];
        if ((item._item && item._item->isActive()) ||
            (item._button && item._button->isFocused())) {
            focusItem = ii;
            break;
        }
    }
    return focusItem;
}

cSpinBox* cSettingWnd::focusedItem(void) {
    ssize_t focusItem = focusedItemId();
    if (focusItem >= 0) return items(_currentTab)[focusItem]._item;
    return NULL;
}

akui::cButton* cSettingWnd::focusedButton(void) {
    for (size_t ii = 0; ii < items(_currentTab).size(); ++ii) {
        akui::cButton* button = items(_currentTab)[ii]._button;
        if (button && button->isFocused()) return button;
    }
    return NULL;
}

void cSettingWnd::HideTab(size_t index) {
    if (index >= _tabs.size()) return;
    for (size_t ii = 0; ii < items(index).size(); ++ii) {
        items(index)[ii]._label->hide();
        if (items(index)[ii]._item) items(index)[ii]._item->hide();
        if (items(index)[ii]._button) items(index)[ii]._button->hide();
    }
}

void cSettingWnd::ShowTab(size_t index) {
    if (index >= _tabs.size()) return;
    updateTabLayout(index, true);
    arrangeChildren();
    if (items(index).size()) {
        sSetingItem& firstItem = items(index)[_tabs[index]._firstVisibleItem];
        windowManager().setFocusedWindow(firstItem._item ? (cWindow*)firstItem._item
                                                         : (cWindow*)firstItem._button);
    }
}

void cSettingWnd::SwitchTab(size_t oldIndex, size_t newIndex) {
    HideTab(oldIndex);
    ShowTab(newIndex);
}

void cSettingWnd::onItemChanged(akui::cSpinBox* item) {
    size_t newTab = item->selectedItemId();
    size_t oldTab = _currentTab;
    _currentTab = newTab;
    SwitchTab(oldTab, newTab);
}

void cSettingWnd::updateTabLayout(size_t index, bool showItems) {
    if (index >= _tabs.size()) return;

    std::vector<sSetingItem>& tabItems = items(index);
    size_t maxFirstVisible = tabItems.size() > MAX_VISIBLE_ITEMS
                                   ? tabItems.size() - MAX_VISIBLE_ITEMS
                                   : 0;
    if (_tabs[index]._firstVisibleItem > maxFirstVisible) {
        _tabs[index]._firstVisibleItem = maxFirstVisible;
    }

    size_t first = _tabs[index]._firstVisibleItem;
    for (size_t ii = 0; ii < tabItems.size(); ++ii) {
        bool visible = showItems && ii >= first && ii < first + MAX_VISIBLE_ITEMS;
        if (visible) {
            s32 itemY = (ii - first) * ITEM_PITCH + 18 + TOP_MARGIN;
            cWindow* control = tabItems[ii]._item ? (cWindow*)tabItems[ii]._item
                                                   : (cWindow*)tabItems[ii]._button;
            s32 labelY = itemY + (control->size().y - tabItems[ii]._label->size().y) / 2;
            control->setRelativePosition(cPoint(_size.x - _spinBoxWidth - 4, itemY));
            tabItems[ii]._label->setRelativePosition(cPoint(8, labelY));
            tabItems[ii]._label->show();
            control->show();
        } else {
            tabItems[ii]._label->hide();
            if (tabItems[ii]._item) tabItems[ii]._item->hide();
            if (tabItems[ii]._button) tabItems[ii]._button->hide();
        }
    }
}

void cSettingWnd::focusItem(size_t index) {
    if (_currentTab >= _tabs.size() || index >= items(_currentTab).size()) return;

    size_t& first = _tabs[_currentTab]._firstVisibleItem;
    if (index < first) {
        first = index;
    } else if (index >= first + MAX_VISIBLE_ITEMS) {
        first = index - MAX_VISIBLE_ITEMS + 1;
    }

    updateTabLayout(_currentTab, true);
    arrangeChildren();
    sSetingItem& item = items(_currentTab)[index];
    windowManager().setFocusedWindow(item._item ? (cWindow*)item._item : (cWindow*)item._button);
}

void cSettingWnd::repositionButtons(void) {
    s16 buttonY = size().y - _buttonCancel.size().y - 4;
    _buttonCancel.setRelativePosition(cPoint(_buttonCancel.relativePosition().x, buttonY));
    _buttonOK.setRelativePosition(cPoint(_buttonOK.relativePosition().x, buttonY));
    _buttonY.setRelativePosition(cPoint(_buttonY.relativePosition().x, buttonY));
}
