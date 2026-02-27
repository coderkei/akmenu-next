/*
    mainlist.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <nds.h>
#include "dsrom.h"
#include "keymessage.h"
#include "listview.h"
#include "sigslot.h"
#include "touchmessage.h"
#include "zoomingicon.h"

#if defined(__DSIMODE__)  && !defined(__DSPICO__)
#define SD_ROOT_0 "sd:"
#else
#define SD_ROOT_0 "fat:"
#endif
#define SD_ROOT SD_ROOT_0 "/"

// 显示游戏列表，文件列表等等
class cMainList : public akui::cListView {
  public:
    enum VIEW_MODE { VM_LIST = 0, VM_ICON, VM_INTERNAL, VM_LIST_ICON };

    enum  // COLUMN_LIST
    {
        ICON_COLUMN = 0,
        SHOWNAME_COLUMN = 1,
        INTERNALNAME_COLUMN = 2,
        REALNAME_COLUMN = 3,
        SAVETYPE_COLUMN = 4,
        FILESIZE_COLUMN = 5
    };

  public:
    cMainList(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text);

    ~cMainList();

  public:
    int init();

    bool enterDir(const std::string& dirName);

    void backParentDir();

    void refresh();

    std::string getCurrentDir();

    bool getRomInfo(u32 rowIndex, DSRomInfo& info) const;

    void setRomInfo(u32 rowIndex, const DSRomInfo& info);

    void setViewMode(VIEW_MODE mode);

    std::string getSelectedFullPath();

    std::string getSelectedShowName();

    VIEW_MODE getViewMode() { return _viewMode; }

    void arrangeIcons();

    akui::Signal1<u32> selectedRowHeadClicked;

    akui::Signal0 directoryChanged;

    akui::Signal1<bool&> animateIcons;

  public:
    bool IsFavorites(void);

    void SwitchShowAllFiles(void);

    const std::vector<std::string>* Saves(void);

  protected:
    void draw();

    void drawIcons();  // 直接画家算法画 icons

    enum { POSITION = 0, CONTENT = 1 };

    void updateActiveIcon(bool updateContent);  // 更新活动图标的坐标等等

    void updateInternalNames(void);

  protected:
    void onSelectedRowClicked(u32 index);

    void onSelectChanged(u32 index);

    void onScrolled(u32 index);

  protected:
    VIEW_MODE _viewMode;

    std::string _currentDir;

    std::vector<std::string> _extnameFilter;

    std::vector<DSRomInfo> _romInfoList;

    cZoomingIcon _activeIcon;

    float _activeIconScale;

    bool _showAllFiles;

    std::vector<std::string> _saves;

  protected:
    u32 _topCount;
    u32 _topuSD;
    u32 _topSlot1;
    u32 _topSlot2;
    u32 _topFavorites;

  public:
    u32 Slot1(void) { return _topSlot1; }
    u32 Slot2(void) { return _topSlot2; }
};
