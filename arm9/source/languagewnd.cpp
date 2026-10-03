/*
    languagewnd.cpp
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <algorithm>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gdi.h"
#include "language.h"
#include "languagewnd.h"
#include "systemfilenames.h"
#include "uisettings.h"

using namespace akui;

namespace {
bool sameLanguageName(const std::string& left, const std::string& right) {
    return strcasecmp(left.c_str(), right.c_str()) == 0;
}

bool hasLanguageFile(const std::string& language) {
    if (language.empty() || language == "." || language == ".." ||
        language.find('/') != std::string::npos || language.find('\\') != std::string::npos)
        return false;

    const std::string directory = SFN_LANGUAGE_DIRECTORY + language + "/";
    struct stat st;
    return stat(directory.c_str(), &st) == 0 && S_ISDIR(st.st_mode) &&
           access((directory + "language.txt").c_str(), F_OK) == 0;
}

void drawScrollChevron(s16 x, s16 y, bool up, u16 color, GRAPHICS_ENGINE engine) {
    gdi().setPenColor(color, engine);
    if (up) {
        gdi().drawLine(x, y + 4, x + 4, y, engine);
        gdi().drawLine(x + 4, y, x + 8, y + 4, engine);
    } else {
        gdi().drawLine(x, y, x + 4, y + 4, engine);
        gdi().drawLine(x + 4, y + 4, x + 8, y, engine);
    }
}
}  // namespace

std::vector<std::string> installedLanguages() {
    std::vector<std::string> languages;
    DIR* dir = opendir((SFN_LANGUAGE_DIRECTORY).c_str());
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            const std::string language(entry->d_name);
            if (hasLanguageFile(language)) languages.push_back(language);
        }
        closedir(dir);
    }

    if (!gs().langDirectory.empty()) {
        bool currentFound = false;
        for (size_t i = 0; i < languages.size(); ++i) {
            if (sameLanguageName(languages[i], gs().langDirectory)) {
                currentFound = true;
                break;
            }
        }
        if (!currentFound) languages.push_back(gs().langDirectory);
    }

    std::sort(languages.begin(), languages.end());
    return languages;
}

cLanguageWnd::cLanguageWnd(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text,
                           const std::vector<std::string>& languages,
                           const std::string& currentLanguage)
    : cForm(x, y, w, h, parent, text),
      _languages(languages),
      _selectedLanguage(currentLanguage),
      _buttonOK(0, 0, 46, 18, this, ""),
      _buttonCancel(0, 0, 54, 18, this, ""),
      _list(0, 0, w - 32, h - 48, this, "language list") {
    if (_languages.empty() && !currentLanguage.empty()) _languages.push_back(currentLanguage);

    _buttonOK.setStyle(cButton::press);
    _buttonOK.setText("\x03 " + LANG("setting window", "ok"));
    _buttonOK.setTextColor(uis().buttonTextColor);
    _buttonOK.loadAppearance(SFN_BUTTON2);
    _buttonOK.clicked.connect(this, &cLanguageWnd::onOK);
    addChildWindow(&_buttonOK);

    _buttonCancel.setStyle(cButton::press);
    _buttonCancel.setText("\x02 " + LANG("setting window", "cancel"));
    _buttonCancel.setTextColor(uis().buttonTextColor);
    _buttonCancel.loadAppearance(SFN_BUTTON3);
    _buttonCancel.clicked.connect(this, &cLanguageWnd::onCancel);
    addChildWindow(&_buttonCancel);

    _buttonCancel.setRelativePosition(cPoint(size().x - _buttonCancel.size().x - 4,
                                              size().y - _buttonCancel.size().y - 4));
    _buttonOK.setRelativePosition(cPoint(size().x - _buttonCancel.size().x -
                                                 _buttonOK.size().x - 8,
                                         size().y - _buttonOK.size().y - 4));

    _list.setRelativePosition(cPoint(4, 22));
    _list.insertColumn(0, "icon", 15);
    _list.insertColumn(1, "language", _list.size().x - 15);
    _list.arangeColumnsSize();
    _list.setRowHeight(15);
    _list.selectedRowClicked.connect(this, &cLanguageWnd::onSelect);
    _list.ownerDraw.connect(this, &cLanguageWnd::onDraw);
    addChildWindow(&_list);

    _renderDesc.loadData(SFN_FORM_TITLE_L, SFN_FORM_TITLE_R, SFN_FORM_TITLE_M);
    _renderDesc.setTitleText(_text);

    std::vector<std::string> rowText(2);
    rowText[0] = "";
    for (size_t i = 0; i < _languages.size(); ++i) {
        rowText[1] = _languages[i];
        _list.insertRow(_list.getRowCount(), rowText);
        if (sameLanguageName(_languages[i], _selectedLanguage)) _list.selectRow(i);
    }
    arrangeChildren();
}

cLanguageWnd::~cLanguageWnd() {}

void cLanguageWnd::draw() {
    _renderDesc.draw(windowRectangle(), _engine);
    cForm::draw();
    drawScrollIndicator();
}

bool cLanguageWnd::process(const cMessage& msg) {
    bool ret = cForm::process(msg);
    if (!ret && msg.id() > cMessage::keyMessageStart && msg.id() < cMessage::keyMessageEnd) {
        ret = processKeyMessage((cKeyMessage&)msg);
    }
    return ret;
}

bool cLanguageWnd::processKeyMessage(const cKeyMessage& msg) {
    if (msg.id() != cMessage::keyDown) return false;

    switch (msg.keyCode()) {
        case cKeyMessage::UI_KEY_DOWN:
            _list.selectNext();
            return true;
        case cKeyMessage::UI_KEY_UP:
            _list.selectPrev();
            return true;
        case cKeyMessage::UI_KEY_A:
            onSelect();
            return true;
        case cKeyMessage::UI_KEY_B:
            onCancel();
            return true;
        case cKeyMessage::UI_KEY_X:
            onOK();
            return true;
        default:
            return false;
    }
}

cWindow& cLanguageWnd::loadAppearance(const std::string& aFileName) {
    (void)aFileName;
    _renderDesc.loadData(SFN_FORM_TITLE_L, SFN_FORM_TITLE_R, SFN_FORM_TITLE_M);
    _renderDesc.setTitleText(_text);
    return *this;
}

void cLanguageWnd::onSelect(u32 index) {
    (void)index;
    if (_list.selectedRowId() < _languages.size())
        _selectedLanguage = _languages[_list.selectedRowId()];
}

void cLanguageWnd::onOK() {
    cForm::onOK();
}

void cLanguageWnd::onCancel() {
    cForm::onCancel();
}

void cLanguageWnd::onDraw(const cListView::cOwnerDraw& data) {
    if (data._row >= _languages.size()) return;
    if (data._col == 0 && sameLanguageName(_languages[data._row], _selectedLanguage)) {
        const u16 color = gdi().getPenColor(data._engine);
        const u16 markSize = data._size.y - 8;
        gdi().fillRect(color, color, data._position.x + ((data._size.x - markSize) >> 1),
                       data._position.y + 4, markSize, markSize, data._engine);
    } else if (data._col == 1) {
        gdi().textOutRect(data._position.x, data._textY, data._size.x, data._textHeight, data._text,
                          data._engine);
    }
}

void cLanguageWnd::drawScrollIndicator() {
    const size_t totalRows = _list.getRowCount();
    const size_t visibleRows = _list.visibleRowCount();
    if (visibleRows == 0 || totalRows <= visibleRows) return;

    const size_t maxFirstVisible = totalRows - visibleRows;
    const size_t firstVisible = _list.firstVisibleRowId() > maxFirstVisible
                                        ? maxFirstVisible
                                        : _list.firstVisibleRowId();
    const bool canScrollUp = firstVisible > 0;
    const bool canScrollDown = firstVisible < maxFirstVisible;
    const s16 indicatorX = position().x + size().x - 25;
    const s16 indicatorY = _list.position().y + _list.size().y - 18;
    const u16 activeColor = uis().spinBoxTextColor;
    const u16 inactiveColor = uis().spinBoxNormalColor;
    const u16 frameColor = uis().spinBoxFrameColor;
    drawScrollChevron(indicatorX, indicatorY, true,
                      canScrollUp ? activeColor : inactiveColor, _engine);
    drawScrollChevron(indicatorX, indicatorY + 8, false,
                      canScrollDown ? activeColor : inactiveColor, _engine);

    const s16 trackX = indicatorX + 12;
    const u16 trackHeight = 12;
    const u16 thumbHeight = 4;
    gdi().setPenColor(frameColor, _engine);
    gdi().frameRect(trackX, indicatorY, 5, trackHeight, _engine);
    const s16 thumbY = indicatorY +
                       (s16)((firstVisible * (trackHeight - thumbHeight)) / maxFirstVisible);
    gdi().fillRect(activeColor, activeColor, trackX, thumbY, 5, thumbHeight, _engine);
}
