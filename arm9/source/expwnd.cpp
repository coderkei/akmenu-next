/*
    expwnd.cpp
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "expwnd.h"
#include <fat.h>
#include "datetime.h"
#include "exptools.h"
#include "fontfactory.h"
#include "language.h"
#include "msgbox.h"
#include "progresswnd.h"
#include "uisettings.h"
#include "windowmanager.h"

using namespace akui;

cExpWnd::cExpWnd(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text)
    : cForm(x, y, w, h, parent, text),
      _buttonOK(0, 0, 46, 18, this, "\x01 OK"),
      _buttonCancel(0, 0, 48, 18, this, "\x02 Cancel"),
      _buttonRAM(0, 0, 46, 18, this, "\x03 RAM"),
      _buttonSRAM(0, 0, 46, 18, this, "\x04 SRAM"),
      _Rumble(0, 0, 108, 18, this, "spin"),
      _Label(0, 0, 20 * 6, gs().fontHeight, this, "rumble strength") {
    s16 buttonY = size().y - _buttonCancel.size().y - 4;

    _buttonCancel.setStyle(cButton::press);
    _buttonCancel.setText("\x02 " + LANG("setting window", "cancel"));
    _buttonCancel.setTextColor(uis().buttonTextColor);
    _buttonCancel.loadAppearance(SFN_BUTTON3);
    _buttonCancel.clicked.connect(this, &cExpWnd::onCancel);
    addChildWindow(&_buttonCancel);

    _buttonOK.setStyle(cButton::press);
    _buttonOK.setText("\x01 " + LANG("setting window", "ok"));
    _buttonOK.setTextColor(uis().buttonTextColor);
    _buttonOK.loadAppearance(SFN_BUTTON3);
    _buttonOK.clicked.connect(this, &cExpWnd::onOK);
    addChildWindow(&_buttonOK);

    _buttonRAM.setStyle(cButton::press);
    _buttonRAM.setText("\x03 " + LANG("exp window", "ram"));
    _buttonRAM.setTextColor(uis().buttonTextColor);
    _buttonRAM.loadAppearance(SFN_BUTTON3);
    _buttonRAM.clicked.connect(this, &cExpWnd::onRAM);
    addChildWindow(&_buttonRAM);

    _buttonSRAM.setStyle(cButton::press);
    _buttonSRAM.setText("\x04 " + LANG("exp window", "sram"));
    _buttonSRAM.setTextColor(uis().buttonTextColor);
    _buttonSRAM.loadAppearance(SFN_BUTTON3);
    _buttonSRAM.clicked.connect(this, &cExpWnd::onSRAM);
    addChildWindow(&_buttonSRAM);

    s16 nextButtonX = size().x;
    s16 buttonPitch = _buttonCancel.size().x + 4;
    nextButtonX -= buttonPitch;
    _buttonCancel.setRelativePosition(cPoint(nextButtonX, buttonY));

    buttonPitch = _buttonOK.size().x + 4;
    nextButtonX -= buttonPitch;
    _buttonOK.setRelativePosition(cPoint(nextButtonX, buttonY));

    buttonPitch = _buttonRAM.size().x + 4;
    nextButtonX -= buttonPitch;
    _buttonRAM.setRelativePosition(cPoint(nextButtonX, buttonY));

    buttonPitch = _buttonSRAM.size().x + 4;
    nextButtonX -= buttonPitch;
    _buttonSRAM.setRelativePosition(cPoint(nextButtonX, buttonY));

    s32 itemY = 32;
    s32 itemX = 8;
    const char* rumbleLang[] = {"off", "low", "mid", "high"};
    for (size_t i = 0; i < 4; ++i) {
        _Rumble.insertItem(LANG("exp window", rumbleLang[i]), i);
    }
    CIniFile ini(SFN_UI_SETTINGS);
    u32 spinBoxWidth = ini.GetInt("setting window", "spinBoxWidth", 108);
    _Rumble.loadAppearance("");
    _Rumble.setSize(cSize(spinBoxWidth, 18));

    _Rumble.setRelativePosition(cPoint(_size.x - spinBoxWidth - 4, itemY));
    addChildWindow(&_Rumble);
    _Rumble.selectItem(0);

    itemY += (_Rumble.windowRectangle().height() - _Label.windowRectangle().height()) / 2;
    _Label.setText(LANG("exp window", "strength"));
    _Label.setRelativePosition(cPoint(itemX, itemY));
    _Label.setTextColor(uis().formTextColor);
    _Label.setSize(cSize(_size.x / 2 + 8, 12));
    addChildWindow(&_Label);

    loadAppearance("");
    arrangeChildren();

    CIniFile f;
    _romName = f.LoadIniFile(SFN_LAST_GBA_SAVEINFO) ? f.GetString("Save Info", "lastLoadedNOR", "")
                                                    : "";
    _romName = font().breakLine(_romName, size().x - 16);
}

cExpWnd::~cExpWnd() {}

void cExpWnd::draw() {
    _renderDesc.draw(windowRectangle(), _engine);
    gdi().setPenColor(uiSettings().formTextColor, _engine);
    int textY = _Rumble.position().y + _Rumble.size().y + 8,
        textHeight = _buttonOK.position().y - 8 - textY;
    gdi().textOutRect(position().x + 8, textY, size().x - 16, textHeight, _romName.c_str(),
                      _engine);
    cForm::draw();
}

bool cExpWnd::process(const akui::cMessage& msg) {
    bool ret = false;

    ret = cForm::process(msg);

    if (!ret) {
        if (msg.id() > cMessage::keyMessageStart && msg.id() < cMessage::keyMessageEnd) {
            ret = processKeyMessage((cKeyMessage&)msg);
        }
    }
    return ret;
}

bool cExpWnd::processKeyMessage(const cKeyMessage& msg) {
    bool ret = false;
    if (msg.id() == cMessage::keyDown) {
        switch (msg.keyCode()) {
            case cKeyMessage::UI_KEY_A:
                onOK();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_B:
                onCancel();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_X:
                onRAM();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_Y:
                onSRAM();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_LEFT:
                _Rumble.selectPrev();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_RIGHT:
                _Rumble.selectNext();
                ret = true;
                break;
            default: {
            }
        };
    }
    return ret;
}

cWindow& cExpWnd::loadAppearance(const std::string& aFileName) {
    _renderDesc.loadData(SFN_FORM_TITLE_L, SFN_FORM_TITLE_R, SFN_FORM_TITLE_M);
    _renderDesc.setTitleText(_text);
    return *this;
}

void cExpWnd::onOK() {
    if (!isDSiMode()) {
        expansion().SoftReset();
        if (_Rumble.selectedItemId() > 0) cExpansion::SetShake(0xEF + _Rumble.selectedItemId());
    }
    cForm::onOK();
}

void cExpWnd::onCancel() {
    cForm::onCancel();
}

void cExpWnd::onRAM() {
    if (!isDSiMode()) {
        expansion().SoftReset();
        cExpansion::SetRompage(0x300);
        cExpansion::OpenNorWrite();
        cExpansion::EnableBrowser();
    }
    cForm::onOK();
}

void cExpWnd::onSRAM() {
    if (isDSiMode()) {
        cForm::onOK();
        return;
    }
    std::string saveName = "fat:/sram-" + datetime().getTimeStampString() + ".sav";
    const u32 size = 4096 * 128, page = 4096, pages = 128;
    FILE* saveFile = fopen(saveName.c_str(), "wb");
    if (saveFile) {
        u8* buf = (u8*)malloc(size);
        if (buf) {
            u16 ramPage = expansion().Rampage();
            progressWnd().setTipText(LANG("progress window", "gba save store"));
            progressWnd().show();
            progressWnd().setPercent(0);
            for (u32 ii = 0; ii < pages; ii++) {
                expansion().SetRampage(ii);
                cExpansion::ReadSram(0x0A000000, buf + ii * page, page);
                progressWnd().setPercent(ii * page * 90 / size);
            }
            expansion().SetRampage(ramPage);
            fwrite(buf, size, 1, saveFile);
            progressWnd().setPercent(100);
            progressWnd().hide();
            free(buf);
        }
        fclose(saveFile);
    }
    cForm::onOK();
}

void cExpWnd::onShow() {
    centerScreen();
}
