/*
    mainlist.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

//�

#include "mainlist.h"
#include <fat.h>
#include <sys/dir.h>
#include "../../share/memtool.h"
#include "dbgtool.h"
#include "folder_banner_bin.h"
#include "gba_banner_bin.h"
#include "inifile.h"
#include "language.h"
#include "microsd_banner_bin.h"
#include "nand_banner_bin.h"
#include "nds_save_banner_bin.h"
#include "progresswnd.h"
#include "startmenu.h"
#include "systemfilenames.h"
#include "timetool.h"
#include "unicode.h"
#include "unknown_banner_bin.h"
#include "windowmanager.h"
#include "fsmngr.h"
#include "pluginmngr.h"

using namespace akui;

namespace {
bool loadBannerFromBin(DSRomInfo& rominfo, const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        return false;
    }

    size_t read = fread(&rominfo.banner(), 1, sizeof(tNDSBanner), f);
    fclose(f);
    return read == sizeof(tNDSBanner);
}
}  // namespace

cMainList::cMainList(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text)
    : cListView(x, y, w, h, parent, text),
      _showAllFiles(false),
      _topCount(5),
      _topuSD(0),
      _topuDSiSD(1),
      _topFavorites(2),
      _topSlot1(3),
      _topSlot2(4) {
    _viewMode = VM_LIST;
    _activeIconScale = 1;
    _activeIcon.hide();
    _activeIcon.update();
    animationManager().addAnimation(&_activeIcon);
    dbg_printf("_activeIcon.init\n");

    if (!isDSiMode()) {
        _topCount = 3;
        _topuSD = 0;
        _topFavorites = 1;
        _topSlot2 = 2;
        _topuDSiSD = 3;
        _topSlot1 = 4;
    } else {
        if (fsManager().isFlashcart()) {
            if (fsManager().isSDInserted()) {
                _topCount = 3;
                _topuSD = 0;
                _topuDSiSD = 1;
                _topFavorites = 2;
                _topSlot2 = 3;               
                _topSlot1 = 4;
            } else {
                _topCount = 2;
                _topuSD = 0;
                _topFavorites = 1;
                _topSlot2 = 2;
                _topuDSiSD = 3;
                _topSlot1 = 4;
            }
        } else {
                _topCount = 3;      
                _topuDSiSD = 0;
                _topFavorites = 1;
                _topSlot1 = 2;
                _topuSD = 3;
                _topSlot2 = 4;
        }
    }
}

cMainList::~cMainList() {}

int cMainList::init() {
    CIniFile ini(SFN_UI_SETTINGS);
    _textColor = ini.GetInt("main list", "textColor", RGB15(7, 7, 7));
    _textColorHilight = ini.GetInt("main list", "textColorHilight", RGB15(31, 0, 31));
    _selectionBarColor1 = ini.GetInt("main list", "selectionBarColor1", RGB15(16, 20, 24));
    _selectionBarColor2 = ini.GetInt("main list", "selectionBarColor2", RGB15(20, 25, 0));
    _selectionBarOpacity = ini.GetInt("main list", "selectionBarOpacity", 100);

    // selectedRowClicked.connect(this,&cMainList::executeSelected);

    insertColumn(ICON_COLUMN, "icon", 0);
    insertColumn(SHOWNAME_COLUMN, "showName", 0);
    insertColumn(INTERNALNAME_COLUMN, "internalName", 0);
    insertColumn(REALNAME_COLUMN, "realName", 0);  // hidden column for contain real filename
    insertColumn(SAVETYPE_COLUMN, "saveType", 0);
    insertColumn(FILESIZE_COLUMN, "fileSize", 0);

    setViewMode((cMainList::VIEW_MODE)gs().viewMode);

    _activeIcon.hide();

    return 1;
}

// Don't think this is used anymore?
/*static bool itemSortComp(const cListView::itemVector& item1, const cListView::itemVector& item2) {
    const std::string& fn1 = item1[cMainList::SHOWNAME_COLUMN].text();
    const std::string& fn2 = item2[cMainList::SHOWNAME_COLUMN].text();
    if ("../" == fn1) return true;
    if ("../" == fn2) return false;
    if ('/' == fn1[fn1.size() - 1] && '/' == fn2[fn2.size() - 1]) return fn1 < fn2;
    if ('/' == fn1[fn1.size() - 1]) return true;
    if ('/' == fn2[fn2.size() - 1]) return false;

    return fn1 < fn2;
}*/

static bool extnameFilter(const std::vector<std::string>& extNames, std::string extName) {
    if (0 == extNames.size()) return true;

    for (size_t i = 0; i < extName.size(); ++i) extName[i] = tolower(extName[i]);

    for (size_t i = 0; i < extNames.size(); ++i) {
        if (extName == extNames[i]) {
            return true;
        }
    }
    return false;
}

bool cMainList::enterDir(const std::string& dirName) {

    std::string base = fsManager().resolveSystemPath("/_nds/akmenunext/icons/");

    std::string microsd = base + "microsd_banner.bin";
    std::string nand = base + "nand_banner.bin";
    std::string gba = base + "gba_banner.bin";
    std::string folder = base + "folder_banner.bin";

    _saves.clear();
    if (memcmp(dirName.c_str(), "...", 3) == 0 || dirName.empty())  // select RPG or SD card
    {
        removeAllRows();
        _romInfoList.clear();
        for (size_t i = 0; i < _topCount; ++i) {
            std::vector<std::string> a_row;
            a_row.push_back("");  // make a space for icon
            DSRomInfo rominfo;
            if (_topuSD == i) {
                a_row.push_back(LANG("mainlist", "microsd card"));
                a_row.push_back("");
                a_row.push_back("fat:/");
                if(gs().icon)
                    rominfo.setBannerFromFile("folder", microsd);
                else
                    rominfo.setBanner("folder", microsd_banner_bin);
            } else if (_topuDSiSD == i) {
                a_row.push_back("DSi SD");
                a_row.push_back("");
                a_row.push_back("sd:/");
                if(gs().icon)
                    rominfo.setBannerFromFile("folder", microsd);
                else
                    rominfo.setBanner("folder", microsd_banner_bin);
            } else if (_topSlot1 == i) {
                a_row.push_back(LANG("mainlist", "slot1 card"));
                a_row.push_back("");
                a_row.push_back("slot1:/");
                if(gs().icon)
                    rominfo.setBannerFromFile("folder", nand);
                else
                    rominfo.setBanner("folder", nand_banner_bin);
            } else if (_topSlot2 == i) {
                a_row.push_back(LANG("mainlist", "slot2 card"));
                a_row.push_back("");
                a_row.push_back("slot2:/");
                if(gs().icon)
                    rominfo.setBannerFromFile("folder", gba);
                else
                    rominfo.setBanner("folder", gba_banner_bin);
            } else if (_topFavorites == i) {
                a_row.push_back(LANG("mainlist", "favorites"));
                a_row.push_back("");
                a_row.push_back("favorites:/");
                if(gs().icon)
                    rominfo.setBannerFromFile("folder", folder);
                else
                    rominfo.setBanner("folder", folder_banner_bin);
            }
            insertRow(i, a_row);
            _romInfoList.push_back(rominfo);
        }
        _currentDir = "";
        directoryChanged();
        return true;
    }

    if ("slot2:/" == dirName) {
        _currentDir = "";
        directoryChanged();
        return true;
    }

    if ("slot1:/" == dirName) {
        _currentDir = "";
        directoryChanged();
        return true;
    }

    bool favorites = ("favorites:/" == dirName);
    DIR* dir = NULL;
    struct dirent* entry;

    if (!favorites) {
        dir = opendir(dirName.c_str());

        if (dir == NULL) {
            if (fsManager().getFSRoot() == dirName) {
                std::string title = LANG("sd card error", "title");
                std::string sdError = LANG("sd card error", "text");
                messageBox(NULL, title, sdError, MB_OK);
            }
            dbg_printf("Unable to open directory<%s>.\n", dirName.c_str());
            return false;
        }
    }

    removeAllRows();
    _romInfoList.clear();

    std::vector<std::string> extNames;
    extNames.push_back(".nds");
    extNames.push_back(".dsi");
    extNames.push_back(".srl");
    if (gs().showGbaRoms > 0) extNames.push_back(".gba");
    const std::vector<std::string>& pluginExtensions = pluginManager().extensions();
    extNames.insert(extNames.end(), pluginExtensions.begin(), pluginExtensions.end());
    if (gs().fileListType > 0) extNames.push_back(".sav");
    if (_showAllFiles || gs().fileListType > 1) extNames.clear();
    std::vector<std::string> savNames;
    savNames.push_back(".sav");

    // insert 一堆文件, 两列，一列作为显示，一列作为真实文件名
    std::string extName;

    // list dir
    {
        // Collect directory entries into structs to avoid repeated
        // reallocation of DSRomInfo objects which would OOM on larger dirs
        struct DirEntry {
            std::string showName;
            std::string realName;
            bool isDir;
        };
        std::vector<DirEntry> entries;
        entries.reserve(256);

        cwl();
        if (favorites) {
            CIniFile ini(SFN_FAVORITES);

            std::vector<std::string> items;
            ini.GetStringVector("main", "list", items, '|');
            std::string showname = "", realname = "";
            for (size_t ii = 0; ii < items.size(); ++ii) {

                size_t pos = items[ii].rfind('/', items[ii].length() - 2);
                if (pos == items[ii].npos) {
                    showname = items[ii];  // show name
                } else {
                    showname = items[ii].substr(pos + 1, items[ii].npos);  // show name
                }
                realname = items[ii];  // real name

                bool isDir = (FAT_getAttr(realname.c_str()) & ATTR_DIRECTORY) ? true : false;

                DirEntry de;
                de.showName = showname;
                de.realName = realname;
                de.isDir = isDir;
                entries.push_back(de);

            }
        } else if (dir) {
            while ((entry = readdir(dir)) != NULL) {
                std::string lfn(entry->d_name);

                // Hide hidden files unless enabled in settings
                if (!gs().showHiddenFiles &&
                    (lfn[0] == '.' || (FAT_getAttr((dirName + lfn).c_str()) & ATTR_HIDDEN))) {
                    continue;
                }

                bool isDir = (entry->d_type == DT_DIR);
                if (isDir) {
                    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                        continue;
                } else {
                    size_t lastDotPos = lfn.find_last_of('.');
                    extName = (lfn.npos != lastDotPos) ? lfn.substr(lastDotPos) : "";
                    if (!extnameFilter(extNames, extName)) {
                        if (extnameFilter(savNames, extName)) {
                            _saves.push_back(dirName + lfn);
                        }
                        continue;
                    }
                    if (extnameFilter(savNames, extName)) {
                        _saves.push_back(dirName + lfn);
                    }
                }

                DirEntry de;
                de.showName = isDir ? (lfn + "/") : lfn;
                de.realName = isDir ? (dirName + lfn + "/") : (dirName + lfn);
                de.isDir = isDir;
                entries.push_back(de);
            }
            closedir(dir);
        }

        std::sort(entries.begin(), entries.end(),
            [](const DirEntry& a, const DirEntry& b) {
                if ("../" == a.showName) return true;
                if ("../" == b.showName) return false;
                if (a.isDir && b.isDir) return a.showName < b.showName;
                if (a.isDir) return true;
                if (b.isDir) return false;
                return a.showName < b.showName;
            });

        // Build rows and rominfos in one pre-allocated pass
        _romInfoList.reserve(entries.size());

        // Pre-cache folder banner to avoid repeated SD reads
        tNDSBanner cachedFolderBanner;
        bool folderBannerCached = false;
        if (gs().icon) {
            FILE* f = fopen(folder.c_str(), "rb");
            if (f) {
                folderBannerCached = (fread(&cachedFolderBanner, 1, sizeof(tNDSBanner), f) == sizeof(tNDSBanner));
                fclose(f);
            }
        }

        for (size_t ii = 0; ii < entries.size(); ++ii) {
            const DirEntry& de = entries[ii];

            std::vector<std::string> a_row;
            a_row.push_back("");            // icon
            a_row.push_back(de.showName);   // show name
            a_row.push_back("");            // internal name
            a_row.push_back(de.realName);   // real name
            insertRow(ii, a_row);

            DSRomInfo rominfo;
            const std::string& filename = de.realName;

            if (de.isDir) {
                if (folderBannerCached) {
                    rominfo.setExtIcon("folder");
                    memcpy(&rominfo.banner(), &cachedFolderBanner, sizeof(tNDSBanner));
                } else {
                    rominfo.setBanner("folder", folder_banner_bin);
                }
            } else {
                size_t lastDotPos = filename.find_last_of('.');
                extName = (filename.npos != lastDotPos) ? filename.substr(lastDotPos) : "";
                for (size_t jj = 0; jj < extName.size(); ++jj) extName[jj] = tolower(extName[jj]);

                bool allowExt = true, allowUnknown = false;
                if (".sav" == extName) {
                    memcpy(&rominfo.banner(), nds_save_banner_bin, sizeof(tNDSBanner));
                } else if (".gba" == extName) {
                    rominfo.MayBeGbaRom(filename);
                } else if (".nds" != extName && ".dsi" != extName && ".srl" != extName) {
                    const cPluginManager::PluginAssociation* plugin = pluginManager().findPlugin(filename);
                    if (plugin && loadBannerFromBin(rominfo, plugin->iconPath)) {
                        allowExt = false;
                    } else {
                        memcpy(&rominfo.banner(), unknown_banner_bin, sizeof(tNDSBanner));
                        allowUnknown = true;
                    }
                } else {
                    rominfo.MayBeDSRom(filename);
                    allowExt = false;
                }
                if (allowExt) {
                    rominfo.setExtIcon(de.showName);
                    if (extName.length() && !rominfo.isExtIcon())
                        rominfo.setExtIcon(extName.substr(1));
                }
                if (allowUnknown && !rominfo.isExtIcon()) rominfo.setExtIcon("unknown");
            }
            _romInfoList.push_back(rominfo);
        }
        std::sort(_saves.begin(), _saves.end(), stringComp);
        _currentDir = dirName;
    }

    directoryChanged();

    return true;
}

std::string cMainList::getSelectedFullPath() {
    if (!_rows.size()) return std::string("");
    return _rows[_selectedRowId][REALNAME_COLUMN].text();
}

std::string cMainList::getSelectedShowName() {
    if (!_rows.size()) return std::string("");
    return _rows[_selectedRowId][SHOWNAME_COLUMN].text();
}

bool cMainList::getRomInfo(u32 rowIndex, DSRomInfo& info) const {
    if (rowIndex < _romInfoList.size()) {
        info = _romInfoList[rowIndex];
        return true;
    } else {
        return false;
    }
}

void cMainList::setRomInfo(u32 rowIndex, const DSRomInfo& info) {
    if (!_romInfoList[rowIndex].isDSRom()) return;

    if (rowIndex < _romInfoList.size()) {
        _romInfoList[rowIndex] = info;
    }
}

void cMainList::selectRom(const std::string& romPath){
    for(unsigned int row = 0; row < _rows.size(); row++){
        if(romPath == _rows[row][REALNAME_COLUMN].text()){
            selectRow(row);
            break;
        }
    }
}

void cMainList::onSelectChanged(u32 index) {
    dbg_printf("%s\n", _rows[index][3].text().c_str());
}

void cMainList::onSelectedRowClicked(u32 index) {
    const INPUT& input = getInput();
    // dbg_printf("%d %d", input.touchPt.px, _position.x );
    if (input.touchPt.px > _position.x && input.touchPt.px < _position.x + 32)
        selectedRowHeadClicked(index);
}

void cMainList::onScrolled(u32 index) {
    _activeIconScale = 1;
    // updateActiveIcon( CONTENT );
}

void cMainList::backParentDir() {
    if ("..." == _currentDir) return;

    bool fat1 = (fsManager().getFSRoot() == _currentDir), favorites = ("favorites:/" == _currentDir);
    if ("fat:/" == _currentDir || "sd:/" == _currentDir || fat1 || favorites ||
        "/" == _currentDir) {
        enterDir("...");
        if (fat1) selectRow(_topuSD);
        if (favorites) selectRow(_topFavorites);
        return;
    }

    size_t pos = _currentDir.rfind("/", _currentDir.size() - 2);
    std::string parentDir = _currentDir.substr(0, pos + 1);
    dbg_printf("%s->%s\n", _currentDir.c_str(), parentDir.c_str());

    std::string oldCurrentDir = _currentDir;

    if (enterDir(parentDir)) {  // select last entered director
        for (size_t i = 0; i < _rows.size(); ++i) {
            if (parentDir + _rows[i][SHOWNAME_COLUMN].text() == oldCurrentDir) {
                selectRow(i);
            }
        }
    }
}

void cMainList::draw() {
    updateInternalNames();
    cListView::draw();
    updateActiveIcon(POSITION);
    drawIcons();
}

void cMainList::drawIcons()  // 直接画家算法画 icons
{
    if (VM_LIST != _viewMode){
        size_t total = _visibleRowCount;
        if (total > _rows.size() - _firstVisibleRowId) total = _rows.size() - _firstVisibleRowId;

        int icon_height = (VM_LIST_ICON == _viewMode) ? 16 : 32;
        bool small = (VM_LIST_ICON == _viewMode) ? true : false;

        for (size_t i = 0; i < total; ++i) {
            // 这里图像呈现比真正的 MAIN buffer 翻转要早，所以会闪一下
            // 解决方法是在 gdi().present 里边统一呈现翻转
            if (_firstVisibleRowId + i == _selectedRowId) {
                if (_activeIcon.visible()) {
                    continue;
                }
            }
            s32 itemX = _position.x + 1;
            s32 itemY = _position.y + i * _rowHeight + ((_rowHeight - icon_height) >> 1) - 1;
            _romInfoList[_firstVisibleRowId + i].drawDSRomIcon(itemX, itemY, _engine, small);
        }
    }
}

void cMainList::setViewMode(VIEW_MODE mode) {
    if (!_columns.size()) return;
    _viewMode = mode;
    switch (_viewMode) {
        case VM_LIST:
            _columns[ICON_COLUMN].width = 0;
            _columns[SHOWNAME_COLUMN].width = 250;
            _columns[INTERNALNAME_COLUMN].width = 0;
            arangeColumnsSize();
            setRowHeight(15);
            break;
        case VM_LIST_ICON:
            _columns[ICON_COLUMN].width = 21;
            _columns[SHOWNAME_COLUMN].width = 232;
            _columns[INTERNALNAME_COLUMN].width = 0;
            arangeColumnsSize();
            setRowHeight(18);
            break;
        case VM_ICON:
            _columns[ICON_COLUMN].width = 36;
            _columns[SHOWNAME_COLUMN].width = 214;
            _columns[INTERNALNAME_COLUMN].width = 0;
            arangeColumnsSize();
            setRowHeight(38);
            break;
        case VM_INTERNAL:
            _columns[ICON_COLUMN].width = 36;
            _columns[SHOWNAME_COLUMN].width = 0;
            _columns[INTERNALNAME_COLUMN].width = 214;
            arangeColumnsSize();
            setRowHeight(38);
            break;
    }
    scrollTo(_selectedRowId - _visibleRowCount + 1);
}

void cMainList::updateActiveIcon(bool updateContent) {
    const INPUT& temp = getInput();
    bool allowAnimation = true;
    animateIcons(allowAnimation);

    // do not show active icon when hold key to list files. Otherwise the icon will not show
    // correctly.
    if (getInputIdleMs() > 1000 && VM_LIST != _viewMode && VM_LIST_ICON != _viewMode && allowAnimation &&
        _romInfoList.size() && 0 == temp.keysHeld && gs().Animation) {
        if (!_activeIcon.visible()) {
            u8 backBuffer[32 * 32 * 2];
            zeroMemory(backBuffer, 32 * 32 * 2);
            _romInfoList[_selectedRowId].drawDSRomIconMem(backBuffer);
            memcpy(_activeIcon.buffer(), backBuffer, 32 * 32 * 2);
            _activeIcon.setBufferChanged();

            s32 itemX = _position.x;
            s32 itemY = _position.y + (_selectedRowId - _firstVisibleRowId) * _rowHeight +
                        ((_rowHeight - 32) >> 1) - 1;
            _activeIcon.setPosition(itemX, itemY);
            _activeIcon.show();
            dbg_printf("sel %d ac ico x %d y %d\n", _selectedRowId, itemX, itemY);
            for (u8 i = 0; i < 8; ++i) dbg_printf("%02x", backBuffer[i]);
            dbg_printf("\n");
        }
    } else if (_activeIcon.visible()) {
        _activeIcon.hide();
        cwl();
    }
}

std::string cMainList::getCurrentDir() {
    return _currentDir;
}

void cMainList::updateInternalNames(void) {
    if (_viewMode == VM_INTERNAL) {
        size_t total = _visibleRowCount;
        if (total > _rows.size() - _firstVisibleRowId) total = _rows.size() - _firstVisibleRowId;
        for (size_t ii = 0; ii < total; ++ii) {
            if (0 == _rows[_firstVisibleRowId + ii][INTERNALNAME_COLUMN].text().length()) {
                if (_romInfoList[_firstVisibleRowId + ii].isDSRom()) {
                    _rows[_firstVisibleRowId + ii][INTERNALNAME_COLUMN].setText(
                            unicode_to_local_string(_romInfoList[_firstVisibleRowId + ii]
                                                            .banner()
                                                            .titles[gs().language],
                                                    128, NULL));
                } else {
                    _rows[_firstVisibleRowId + ii][INTERNALNAME_COLUMN].setText(
                            _rows[_firstVisibleRowId + ii][SHOWNAME_COLUMN].text());
                }
            }
        }
    }
}

bool cMainList::IsFavorites(void) {
    return ("favorites:/" == _currentDir);
}

const std::vector<std::string>* cMainList::Saves(void) {
    return IsFavorites() ? NULL : &_saves;
}

void cMainList::SwitchShowAllFiles(void) {
    _showAllFiles = !_showAllFiles;
    enterDir(getCurrentDir());
}




