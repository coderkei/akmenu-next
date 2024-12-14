/*
    systemfilenames.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#ifndef __DSIMODE__
#define SFN_SYSTEM_DIR "fat:/_nds/"
#else
#define SFN_SYSTEM_DIR "sd:/_nds/"
#endif
#define SFN_OFFICIAL_SAVELIST SFN_SYSTEM_DIR "savelist.bin"
#define SFN_CUSTOM_SAVELIST SFN_SYSTEM_DIR "gamedata.bin"
#define SFN_LAST_SAVEINFO SFN_SYSTEM_DIR "lastsave.ini"
#define SFN_LAST_GBA_SAVEINFO SFN_SYSTEM_DIR "lastgba.ini"
#define SFN_SDCARD_LIST SFN_SYSTEM_DIR "sdlist.ini"
#define SFN_GLOBAL_SETTINGS SFN_SYSTEM_DIR "globalsettings.ini"
#define SFN_FAVORITES SFN_SYSTEM_DIR "favorites.ini"
#define SFN_BACKLIGHT SFN_SYSTEM_DIR "backlight.ini"

#define SFN_UI_DIRECTORY SFN_SYSTEM_DIR "ui/"
#define SFN_UI_CURRENT_DIRECTORY SFN_UI_DIRECTORY + gs().uiName + "/"
#define SFN_USER_CUSTOM SFN_UI_DIRECTORY + gs().uiName + "/custom.ini"
#define SFN_UI_SETTINGS SFN_UI_DIRECTORY + gs().uiName + "/uisettings.ini"
#define SFN_UPPER_SCREEN_BG SFN_UI_DIRECTORY + gs().uiName + "/upper_screen.bmp"
#define SFN_LOWER_SCREEN_BG SFN_UI_DIRECTORY + gs().uiName + "/lower_screen.bmp"
#define SFN_FORM_TITLE_L SFN_UI_DIRECTORY + gs().uiName + "/title_left.bmp"
#define SFN_FORM_TITLE_M SFN_UI_DIRECTORY + gs().uiName + "/title_bg.bmp"
#define SFN_FORM_TITLE_R SFN_UI_DIRECTORY + gs().uiName + "/title_right.bmp"
#define SFN_BUTTON2 SFN_UI_DIRECTORY + gs().uiName + "/btn2.bmp"
#define SFN_BUTTON3 SFN_UI_DIRECTORY + gs().uiName + "/btn3.bmp"
#define SFN_BUTTON4 SFN_UI_DIRECTORY + gs().uiName + "/btn4.bmp"
#define SFN_SPINBUTTON_L SFN_UI_DIRECTORY + gs().uiName + "/spin_btn_left.bmp"
#define SFN_SPINBUTTON_R SFN_UI_DIRECTORY + gs().uiName + "/spin_btn_right.bmp"
#define SFN_BRIGHTNESS_BUTTON SFN_UI_DIRECTORY + gs().uiName + "/brightness.bmp"
#define SFN_FOLDERUP_BUTTON SFN_UI_DIRECTORY + gs().uiName + "/folder_up.bmp"
#define SFN_STARTMENU_BG SFN_UI_DIRECTORY + gs().uiName + "/menu_bg.bmp"
#define SFN_CLOCK_NUMBERS SFN_UI_DIRECTORY + gs().uiName + "/calendar/clock_numbers.bmp"
#define SFN_CLOCK_COLON SFN_UI_DIRECTORY + gs().uiName + "/calendar/clock_colon.bmp"
#define SFN_DAY_NUMBERS SFN_UI_DIRECTORY + gs().uiName + "/calendar/day_numbers.bmp"
#define SFN_YEAR_NUMBERS SFN_UI_DIRECTORY + gs().uiName + "/calendar/year_numbers.bmp"
#define SFN_CARD_ICON_BLUE SFN_UI_DIRECTORY + gs().uiName + "/card_icon_blue.bmp"
#define SFN_PROGRESS_WND_BG SFN_UI_DIRECTORY + gs().uiName + "/progress_wnd.bmp"
#define SFN_PROGRESS_BAR_BG SFN_UI_DIRECTORY + gs().uiName + "/progress_bar.bmp"
#define SFN_GBAFRAME SFN_UI_DIRECTORY + gs().uiName + "/gbaframe.bmp"
#define SFN_UI_ICONS_DIRECTORY SFN_UI_DIRECTORY + gs().uiName + "/icons/"

#define SFN_LANGUAGE_DIRECTORY SFN_SYSTEM_DIR "language/"
#define SFN_LANGUAGE_TEXT SFN_LANGUAGE_DIRECTORY + gs().langDirectory + "/language.txt"

#define SFN_FONTS_DIRECTORY SFN_SYSTEM_DIR "fonts/"
#define SFN_DEFAULT_FONT "liberation.pcf"

#define SFN_ICONS_DIRECTORY SFN_SYSTEM_DIR "icons/"

#define SFN_CHEATS SFN_SYSTEM_DIR "cheats/usrcheat.dat"
