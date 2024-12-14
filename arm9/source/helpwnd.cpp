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
#include "version.h"
#include "windowmanager.h"

using namespace akui;

cHelpWnd::cHelpWnd(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text)
    : cForm(x, y, w, h, parent, text), _buttonOK(0, 0, 46, 18, this, "\x01 OK") {
    s16 buttonY = size().y - _buttonOK.size().y - 4;

    _buttonOK.setStyle(cButton::press);
    _buttonOK.setText("\x01 " + LANG("setting window", "ok"));
    _buttonOK.setTextColor(uis().buttonTextColor);
    _buttonOK.loadAppearance(SFN_BUTTON3);
    _buttonOK.clicked.connect(this, &cHelpWnd::onOK);
    addChildWindow(&_buttonOK);

    s16 nextButtonX = size().x;

    s16 buttonPitch = _buttonOK.size().x + 8;
    buttonPitch = _buttonOK.size().x + 8;
    nextButtonX -= buttonPitch;
    _buttonOK.setRelativePosition(cPoint(nextButtonX, buttonY));

    loadAppearance("");
    arrangeChildren();

    for (size_t i = 0; i < 9; ++i) {
        std::string textIndex = formatString("item%d", i);
        _helpText += LANG("help window", textIndex);
        _helpText += "\n";
    }
    _helpText = formatString(_helpText.c_str(), 7, 1, 2, 4, 3, 5, 6, "START", "SELECT");

    // u8 nandDriverVer = getNandDriverVer();
    _helpText += '\n';
    _helpText += formatString("coderkei akmenu-next %s.%s ", AKMENU_VERSION_MAIN, AKMENU_VERSION_SUB);
    _helpText += formatString("\n%s %s ", AKMENU_LOADER_NAME, AKMENU_LOADER_VERSION);
}

cHelpWnd::~cHelpWnd() {}

void cHelpWnd::draw() {
    _renderDesc.draw(windowRectangle(), _engine);
    gdi().setPenColor(uiSettings().formTextColor, _engine);
    gdi().textOutRect(position().x + 8, position().y + 17 + uiSettings().thickness, size().x - 8,
                      size().y - 20 - 2 * uiSettings().thickness, _helpText.c_str(), _engine);
    cForm::draw();
}

bool cHelpWnd::process(const akui::cMessage& msg) {
    bool ret = false;

    ret = cForm::process(msg);

    if (!ret) {
        if (msg.id() > cMessage::keyMessageStart && msg.id() < cMessage::keyMessageEnd) {
            ret = processKeyMessage((cKeyMessage&)msg);
        }
    }
    return ret;
}

bool cHelpWnd::processKeyMessage(const cKeyMessage& msg) {
    bool ret = false;
    if (msg.id() == cMessage::keyDown) {
        switch (msg.keyCode()) {
            case cKeyMessage::UI_KEY_A:
            case cKeyMessage::UI_KEY_B:
                onOK();
                ret = true;
                break;
            default: {
            }
        };
    }
    return ret;
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
