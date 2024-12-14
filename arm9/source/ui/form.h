/*
    form.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <list>
#include "bitmapdesc.h"
#include "rectangle.h"
#include "window.h"

namespace akui {

class cForm : public cWindow {
  public:
    cForm(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text);

    ~cForm();

  public:
    u32 doModal();

    u32 doStatic();

    cForm& addChildWindow(cWindow* aWindow);

    cForm& removeChildWindow(cWindow* aWindow);

    cForm& arrangeChildren();

    void draw();

    // cWindow& loadAppearance(const std::string& aFileName );

    bool process(const cMessage& msg);

    cWindow* windowBelow(const cPoint& p);

    u32 modalRet();

    void centerScreen();

    bool isActive(void) const;

    cWindow& disableFocus(void);

  protected:
    virtual void onOK();

    virtual void onCancel();

    void onResize();

    void onMove();

    bool processKeyMessage(const cKeyMessage& msg);

    std::list<cWindow*> _childWindows;

    // cFormDesc * _renderDesc;
    u32 _modalRet;
};

}  // namespace akui
