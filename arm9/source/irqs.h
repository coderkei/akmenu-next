/*
    irqs.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds.h>
#include "singleton.h"

class cIRQ {
  public:
    cIRQ() {}

    ~cIRQ() {}

  public:
    void init();

    void vblankStart();

    void vblankStop();

    static void redrawTopScreen();

    static void cardMC();

    static void vBlank();

    // VBlanks since boot (60 Hz); drives icon animation.
    static u32 vblankCount() { return _vblankCount; }

    static bool _vblankStarted;
    static volatile u32 _vblankCount;
};

typedef t_singleton<cIRQ> irq_s;

inline cIRQ& irq() {
    return irq_s::instance();
}
