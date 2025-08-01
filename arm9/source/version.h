/*
    version.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#define AKMENU_VERSION_MAIN "1"
#define AKMENU_VERSION_SUB "7.2"

#ifndef __KERNEL_LAUNCHER_SUPPORT__
#define AKMENU_LOADER_NAME "nds-bootstrap"
#define AKMENU_LOADER_VERSION ""
#else  // __KERNEL_LAUNCHER_SUPPORT__

// Loader versions - may not be accurate
#if defined(__AKLOADER_AK2__)
#define AKMENU_LOADER_NAME "BL2CK"
#define AKMENU_LOADER_VERSION "1.31.0"
#else  // fallback for acekard
#define AKMENU_LOADER_NAME "wood"
#define AKMENU_LOADER_VERSION "1.62"
#endif  // __AKLOADER_AK2__
#endif  // __KERNEL_LAUNCHER_SUPPORT__
