/*
    spinbox.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <string>
#include <vector>
#include "button.h"
#include "form.h"
#include "statictext.h"

namespace akui {

class cSpinBox : public cForm {
  public:
    cSpinBox(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text);
    ~cSpinBox();

  public:
    void draw();

    cWindow& loadAppearance(const std::string& aFileName);

    u32 selectedItemId() { return _selectedItemId; }

    void selectItem(u32 id);

    void selectNext();

    void selectPrev();

    void insertItem(const std::string& item, u32 position);
    void setItemText(u32 id, const std::string& text);

    void removeItem(u32 position);

    void setTextColor(COLOR color);

    void onCmponentClicked();
    Signal1<cSpinBox*> componentClicked;
    Signal1<cSpinBox*> changed;

  protected:
    void onResize();

    void onMove();

    void onGainedFocus();

    void arrangeButton();

    void arrangeText();

  protected:
    COLOR _focusedColor;
    COLOR _normalColor;
    COLOR _frameColor;
    cButton _prevButton;
    cButton _nextButton;
    cStaticText _itemText;
    u32 _selectedItemId;
    std::vector<std::string> _items;
};

}  // namespace akui
