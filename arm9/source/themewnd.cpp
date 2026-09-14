/*
    themewnd.cpp
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <string.h>

#include "language.h"
#include "systemfilenames.h"
#include "theme.h"
#include "themewnd.h"
#include "uisettings.h"

using namespace akui;

namespace {
bool sameThemeName(const std::string& left, const std::string& right) {
    return strcasecmp(left.c_str(), right.c_str()) == 0;
}

bool containsTheme(const std::vector<std::string>& themes, const std::string& name) {
    for (size_t i = 0; i < themes.size(); ++i) {
        if (sameThemeName(themes[i], name)) return true;
    }
    return false;
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

cThemeWnd::cThemeWnd(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text)
    : cForm(x, y, w, h, parent, text),
      _standardOpen(false),
      _coverOpen(false),
      _selectedTheme(gs().uiName),
      _buttonOK(0, 0, 46, 18, this, ""),
      _buttonCancel(0, 0, 54, 18, this, ""),
      _currentTheme(8, 22, w - 16, 14, this, ""),
      _list(0, 0, w - 32, h - 67, this, "theme tree") {
    _standardThemes = installedThemes(false);
    _coverThemes = installedThemes(true);
    _standardOpen = containsTheme(_standardThemes, _selectedTheme);
    _coverOpen = containsTheme(_coverThemes, _selectedTheme);

    _buttonOK.setStyle(cButton::press);
    _buttonOK.setText("\x03 " + LANG("setting window", "ok"));
    _buttonOK.setTextColor(uis().buttonTextColor);
    _buttonOK.loadAppearance(SFN_BUTTON2);
    _buttonOK.clicked.connect(this, &cThemeWnd::onOK);
    addChildWindow(&_buttonOK);

    _buttonCancel.setStyle(cButton::press);
    _buttonCancel.setText("\x02 " + LANG("setting window", "cancel"));
    _buttonCancel.setTextColor(uis().buttonTextColor);
    _buttonCancel.loadAppearance(SFN_BUTTON3);
    _buttonCancel.clicked.connect(this, &cThemeWnd::onCancel);
    addChildWindow(&_buttonCancel);

    _buttonCancel.setRelativePosition(cPoint(size().x - _buttonCancel.size().x - 4,
                                              size().y - _buttonCancel.size().y - 4));
    _buttonOK.setRelativePosition(cPoint(size().x - _buttonCancel.size().x -
                                                 _buttonOK.size().x - 8,
                                         size().y - _buttonOK.size().y - 4));

    _currentTheme.setRelativePosition(cPoint(8, 22));
    _currentTheme.setTextColor(uis().formTextColor);
    _currentTheme.setText(LANG("theme selector", "Current theme:") + " " + gs().uiName);
    addChildWindow(&_currentTheme);

    _list.setRelativePosition(cPoint(4, 38));
    _list.insertColumn(0, "icon", EIconWidth);
    _list.insertColumn(1, "showName", _list.size().x + 2 - EIconWidth);
    _list.arangeColumnsSize();
    _list.setRowHeight(ERowHeight);
    _list.selectedRowClicked.connect(this, &cThemeWnd::onSelect);
    _list.ownerDraw.connect(this, &cThemeWnd::onDraw);
    addChildWindow(&_list);

    _renderDesc.loadData(SFN_FORM_TITLE_L, SFN_FORM_TITLE_R, SFN_FORM_TITLE_M);
    _renderDesc.setTitleText(_text);
    generateList();

    if (!_selectedTheme.empty()) {
        for (size_t i = 0; i < _rows.size(); ++i) {
            if (!_rows[i].folder && sameThemeName(_rows[i].theme, _selectedTheme)) {
                _list.selectRow(i);
                break;
            }
        }
    }
    arrangeChildren();
}

cThemeWnd::~cThemeWnd() {}

void cThemeWnd::draw() {
    _renderDesc.draw(windowRectangle(), _engine);
    cForm::draw();
    drawScrollIndicator();
}

bool cThemeWnd::process(const cMessage& msg) {
    bool ret = cForm::process(msg);
    if (!ret && msg.id() > cMessage::keyMessageStart && msg.id() < cMessage::keyMessageEnd) {
        ret = processKeyMessage((cKeyMessage&)msg);
    }
    return ret;
}

bool cThemeWnd::processKeyMessage(const cKeyMessage& msg) {
    if (msg.id() != cMessage::keyDown) return false;

    switch (msg.keyCode()) {
        case cKeyMessage::UI_KEY_DOWN:
            _list.selectNext();
            return true;
        case cKeyMessage::UI_KEY_UP:
            _list.selectPrev();
            return true;
        case cKeyMessage::UI_KEY_LEFT:
        case cKeyMessage::UI_KEY_RIGHT: {
            const u32 row = _list.selectedRowId();
            if (row < _rows.size() && _rows[row].folder) {
                const bool coverFolder = _rows[row].coverTheme;
                const bool open = msg.keyCode() == cKeyMessage::UI_KEY_RIGHT;
                if (coverFolder)
                    _coverOpen = open;
                else
                    _standardOpen = open;
                generateList();
                for (size_t i = 0; i < _rows.size(); ++i) {
                    if (_rows[i].folder && _rows[i].coverTheme == coverFolder) {
                        _list.selectRow(i);
                        break;
                    }
                }
            }
            return true;
        }
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

cWindow& cThemeWnd::loadAppearance(const std::string& aFileName) {
    (void)aFileName;
    _renderDesc.loadData(SFN_FORM_TITLE_L, SFN_FORM_TITLE_R, SFN_FORM_TITLE_M);
    _renderDesc.setTitleText(_text);
    return *this;
}

void cThemeWnd::onSelect(u32 index) {
    (void)index;
    if (_rows.empty() || _list.selectedRowId() >= _rows.size()) return;
    const sThemeRow& row = _rows[_list.selectedRowId()];
    if (row.folder) {
        if (row.coverTheme)
            _coverOpen = !_coverOpen;
        else
            _standardOpen = !_standardOpen;

        const bool coverFolder = row.coverTheme;
        generateList();
        for (size_t i = 0; i < _rows.size(); ++i) {
            if (_rows[i].folder && _rows[i].coverTheme == coverFolder) {
                _list.selectRow(i);
                break;
            }
        }
        return;
    }

    _selectedTheme = row.theme;
}

void cThemeWnd::onOK() {
    cForm::onOK();
}

void cThemeWnd::onCancel() {
    cForm::onCancel();
}

void cThemeWnd::drawMark(const cListView::cOwnerDraw& data, u16 width) {
    u16 color = gdi().getPenColor(data._engine);
    u16 size = data._size.y - ESelectTop * 2;
    gdi().fillRect(color, color, data._position.x + ((width - size) >> 1) - 1,
                   data._position.y + ESelectTop, size, size, data._engine);
}

void cThemeWnd::drawScrollIndicator() {
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

void cThemeWnd::onDraw(const cListView::cOwnerDraw& data) {
    if (data._row >= _rows.size()) return;
    const sThemeRow& row = _rows[data._row];
    if (data._col == EIconColumn) {
        if (row.folder) {
            const u16 size = data._size.y - EFolderTop * 2;
            const s16 x1 = data._position.x + ((data._size.x - size) >> 1) - 1;
            const s16 x2 = x1 + (size >> 1);
            const s16 y1 = data._position.y + EFolderTop;
            const s16 y2 = y1 + (size >> 1);
            gdi().frameRect(x1, y1, size, size, data._engine);
            gdi().drawLine(x1, y2, x1 + size, y2, data._engine);
            const bool open = row.coverTheme ? _coverOpen : _standardOpen;
            if (!open) gdi().drawLine(x2, y1, x2, y1 + size, data._engine);
        }
    } else if (data._col == ETextColumn) {
        s16 x = data._position.x;
        u16 width = data._size.x;
        if (!row.folder) {
            x += EFolderWidth;
            width -= EFolderWidth;
            if (sameThemeName(row.theme, _selectedTheme)) drawMark(data, EFolderWidth);
        }
        gdi().textOutRect(x, data._textY, width, data._textHeight, data._text, data._engine);
    }
}

void cThemeWnd::generateList() {
    _rows.clear();
    _list.removeAllRows();

    const std::string standardLabel = LANG("theme selector", "Standard Themes");
    const std::string coverLabel = LANG("theme selector", "Game Cover Themes");

    sThemeRow standardFolder = {true, false, standardLabel};
    _rows.push_back(standardFolder);
    std::vector<std::string> rowText(2);
    rowText[0] = "";
    rowText[1] = standardLabel;
    _list.insertRow(_list.getRowCount(), rowText);
    if (_standardOpen) {
        for (size_t i = 0; i < _standardThemes.size(); ++i) {
            sThemeRow theme = {false, false, _standardThemes[i]};
            _rows.push_back(theme);
            rowText[1] = theme.theme;
            _list.insertRow(_list.getRowCount(), rowText);
        }
    }

    sThemeRow coverFolder = {true, true, coverLabel};
    _rows.push_back(coverFolder);
    rowText[1] = coverLabel;
    _list.insertRow(_list.getRowCount(), rowText);
    if (_coverOpen) {
        for (size_t i = 0; i < _coverThemes.size(); ++i) {
            sThemeRow theme = {false, true, _coverThemes[i]};
            _rows.push_back(theme);
            rowText[1] = theme.theme;
            _list.insertRow(_list.getRowCount(), rowText);
        }
    }
}
