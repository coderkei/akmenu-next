/*
    rominfownd.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <string>
#include "dsrom.h"
#include "form.h"
#include "formdesc.h"
#include "message.h"
#include "settingwnd.h"
#include "spinbox.h"
#include "statictext.h"

class cRomInfoWnd : public akui::cForm {
  public:
    cRomInfoWnd(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text);

    ~cRomInfoWnd();

  public:
    void draw();

    bool process(const akui::cMessage& msg);

    cWindow& loadAppearance(const std::string& aFileName);

    void setFileInfo(const std::string& fullName, const std::string& showName);

    void setRomInfo(const DSRomInfo& romInfo);

    const DSRomInfo& getRomInfo();

    static void showCheats(const std::string& aFileName);

  protected:
    void pressFlash(void);

    void pressSaveType(void);

    void pressCopy(void);

    void copySaveSlot();
    std::vector<std::string> saveSlotLabels() const;

    void pressCheats(void);

    bool processKeyMessage(const akui::cKeyMessage& msg);

    void onOK();

    void onShow();

    void addCode(void);

    akui::cButton _buttonOK;

    akui::cButton _buttonSaveType;

    akui::cButton _buttonFlash;

    akui::cButton _buttonCopy;

    akui::cButton _buttonCheats;

    akui::cFormDesc _renderDesc;

    DSRomInfo _romInfo;

    std::string _romInfoText;

    std::string _filenameText;

    std::string _fileDateText;

    std::string _fileSizeText;

    std::string _saveTypeText;

    std::string _fullName;

    u32 _size;

    cSettingWnd* _settingWnd;
};
