/*
    irqs.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "irqs.h"
#include "animation.h"
#include "bigclock.h"
#include "calendar.h"
#include "calendarwnd.h"
#include "coverwnd.h"
#include "dbgtool.h"
#include "diskicon.h"
#include "globalsettings.h"
#include "timer.h"
#include "userinput.h"
#include "userwnd.h"
#include "windowmanager.h"

using namespace akui;

bool cIRQ::_vblankStarted(false);

void cIRQ::init() {
    irqSet(IRQ_VBLANK, vBlank);
    irqSet(IRQ_CARD_LINE, cardMC);
}

void cIRQ::cardMC() {
    dbg_printf("cardMC\n");
    diskIcon().blink();
    REG_IF &= ~IRQ_CARD_LINE;
}

void cIRQ::vblankStart() {
    _vblankStarted = true;
}

void cIRQ::vblankStop() {
    _vblankStarted = false;
}

void cIRQ::redrawTopScreen() {
    calendarWnd().draw();
    calendar().draw();
    bigClock().draw();
    userWindow().draw();
    if (gs().showCovers) {
        coverWindow().drawBackdrop();
        coverWindow().draw();
    }
    gdi().present(GE_SUB);
}

void cIRQ::vBlank() {
    if (!_vblankStarted) return;

    timer().updateTimer();

    static u32 vBlankCounter = 0;

    if (vBlankCounter++ > 30) {
        vBlankCounter = 0;
        bigClock().blinkColon();
        redrawTopScreen();
#if 0
        char fpsText[16];
        sprintf( fpsText, "fps %.2f\n", timer().getFps() );
        gdi().setPenColor( 1, GE_SUB );
        gdi().textOut( 40, 178, fpsText, GE_SUB );
#endif
    }

    animationManager().update();

    if (REG_ROMCTRL & CARD_BUSY)
        diskIcon().turnOn();
    else
        diskIcon().turnOff();
}
