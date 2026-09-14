/*
    helpwnd.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "helpwnd.h"

#include "fontfactory.h"
#include "language.h"
#include "msgbox.h"
#include "uisettings.h"
#include "windowmanager.h"

using namespace akui;

namespace {
const u16 kTextRightMargin = 28;

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

std::string wrapText(const std::string& text, u32 maxLineWidth) {
    std::string wrapped;
    size_t lineStart = 0;

    while (lineStart <= text.size()) {
        size_t lineEnd = text.find('\n', lineStart);
        size_t lineLength = lineEnd == std::string::npos ? text.size() - lineStart
                                                         : lineEnd - lineStart;
        if (lineLength == 0) {
            wrapped += "\n";
        } else {
            wrapped += font().breakLine(text.substr(lineStart, lineLength), maxLineWidth);
        }

        if (lineEnd == std::string::npos) break;
        lineStart = lineEnd + 1;
        if (lineStart == text.size()) wrapped += "\n";
    }

    return wrapped;
}

size_t countLines(const std::string& text) {
    if (text.empty()) return 0;

    size_t lines = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n') ++lines;
    }
    if (text[text.size() - 1] != '\n') ++lines;
    return lines;
}

std::string getLines(const std::string& text, size_t firstLine, size_t lineCount) {
    size_t start = 0;
    for (size_t line = 0; line < firstLine; ++line) {
        size_t newline = text.find('\n', start);
        if (newline == std::string::npos) return "";
        start = newline + 1;
    }

    size_t end = start;
    for (size_t line = 0; line < lineCount && end < text.size(); ++line) {
        size_t newline = text.find('\n', end);
        end = newline == std::string::npos ? text.size() : newline + 1;
    }
    return text.substr(start, end - start);
}
}  // namespace

cHelpWnd::cHelpWnd(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text)
    : cForm(x, y, w, h, parent, text),
      _buttonOK(0, 0, 46, 18, this, "\x01 OK"),
      _firstVisibleLine(0),
      _lineCount(0),
      _visibleLineCount(0) {
    s16 buttonY = size().y - _buttonOK.size().y - 4;

    _buttonOK.setStyle(cButton::press);
    _buttonOK.setText("\x01 " + LANG("setting window", "ok"));
    _buttonOK.setTextColor(uis().buttonTextColor);
    _buttonOK.loadAppearance(SFN_BUTTON3);
    _buttonOK.clicked.connect(this, &cHelpWnd::onOK);
    addChildWindow(&_buttonOK);

    s16 nextButtonX = size().x - _buttonOK.size().x - 8;
    _buttonOK.setRelativePosition(cPoint(nextButtonX, buttonY));

    loadAppearance("");
    arrangeChildren();

    std::string helpText;
    for (size_t i = 0; i < 9; ++i) {
        const std::string textIndex = formatString("item%d", static_cast<int>(i));
        helpText += LANG("help window", textIndex);
        helpText += "\n";
    }
    _helpText = formatString(helpText.c_str(), 7, 1, 2, 4, 3, 5, 6, "START", "SELECT");

    const u16 textWidth = size().x > kTextRightMargin ? size().x - kTextRightMargin : 1;
    _helpText = wrapText(_helpText, textWidth);
    _lineCount = countLines(_helpText);
}

cHelpWnd::~cHelpWnd() {}

void cHelpWnd::draw() {
    _renderDesc.draw(windowRectangle(), _engine);

    const s16 textX = position().x + 8;
    const s16 textY = position().y + 17 + uiSettings().thickness;
    const s16 textBottom = _buttonOK.position().y - 2;
    const u16 textWidth = size().x > kTextRightMargin ? size().x - kTextRightMargin : 1;
    const u16 textHeight = textBottom > textY ? textBottom - textY : 0;
    _visibleLineCount = textHeight / gs().fontHeight;

    const size_t maxFirstVisibleLine =
            _lineCount > _visibleLineCount ? _lineCount - _visibleLineCount : 0;
    if (_firstVisibleLine > maxFirstVisibleLine) _firstVisibleLine = maxFirstVisibleLine;

    gdi().setPenColor(uiSettings().formTextColor, _engine);
    const std::string visibleText = getLines(_helpText, _firstVisibleLine, _visibleLineCount);
    gdi().textOutRect(textX, textY, textWidth, textHeight, visibleText.c_str(), _engine);

    if (_lineCount > _visibleLineCount) {
        const s16 indicatorX = position().x + size().x - 16;
        const s16 indicatorY = textY + (textHeight - 12) / 2;
        const u16 activeColor = uis().spinBoxTextColor;
        const u16 inactiveColor = uis().spinBoxNormalColor;
        drawScrollChevron(indicatorX, indicatorY, true,
                          _firstVisibleLine > 0 ? activeColor : inactiveColor, _engine);
        drawScrollChevron(indicatorX, indicatorY + 8, false,
                          _firstVisibleLine < maxFirstVisibleLine ? activeColor : inactiveColor,
                          _engine);
    }
    cForm::draw();
}

bool cHelpWnd::process(const akui::cMessage& msg) {
    bool ret = cForm::process(msg);

    if (!ret && msg.id() > cMessage::keyMessageStart && msg.id() < cMessage::keyMessageEnd) {
        ret = processKeyMessage((cKeyMessage&)msg);
    }
    return ret;
}

bool cHelpWnd::processKeyMessage(const cKeyMessage& msg) {
    if (msg.id() != cMessage::keyDown) return false;

    switch (msg.keyCode()) {
        case cKeyMessage::UI_KEY_A:
        case cKeyMessage::UI_KEY_B:
            onOK();
            return true;
        case cKeyMessage::UI_KEY_UP:
            if (_firstVisibleLine > 0) --_firstVisibleLine;
            return true;
        case cKeyMessage::UI_KEY_DOWN:
            if (_visibleLineCount == 0) _visibleLineCount = 1;
            if (_firstVisibleLine + _visibleLineCount < _lineCount) ++_firstVisibleLine;
            return true;
        default:
            return false;
    }
}

cWindow& cHelpWnd::loadAppearance(const std::string& aFileName) {
    _renderDesc.loadData(SFN_FORM_TITLE_L, SFN_FORM_TITLE_R, SFN_FORM_TITLE_M);
    _renderDesc.setTitleText(_text);
    return *this;
}

void cHelpWnd::onOK() {
    cForm::onOK();
}

void cHelpWnd::onShow() {
    centerScreen();
}
