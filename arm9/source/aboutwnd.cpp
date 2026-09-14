/*
    aboutwnd.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "aboutwnd.h"
#include "fsmngr.h"
#include "fontfactory.h"
#include "launcher/DSpico/picoLoader7.h"
#include "language.h"
#include "msgbox.h"
#include "uisettings.h"
#include "version.h"
#include "windowmanager.h"

#include <cstddef>
#include <cstdio>
#include <string>
#include <unistd.h>

using namespace akui;

namespace {
const char kHelpUrl[] = "https://coderkei.github.io/akmenu-next-docs/";
const char kSourceUrl[] = "https://github.com/coderkei/akmenu-next";
const u16 kTextRightMargin = 28;

void drawScrollChevron(s16 x, s16 y, bool up, u16 color, GRAPHICS_ENGINE engine) {
    gdi().setPenColor(color, engine);
    if (up) {
        gdi().drawLine(x, y + 4, x + 4, y, engine);
        gdi().drawLine(x + 4, y, x + 8, y + 4, engine);
    } else {
        gdi().drawLine(x, y, x + 4, y + 4, engine);
        gdi().drawLine(x + 4, y + 4, x + 8, y, engine);
    }
}

bool fileExists(const std::string& path) {
    return access(path.c_str(), F_OK) == 0;
}

bool isDSPicoFlashcart() {
    return fileExists(fsManager().resolveSystemPath("/_pico/picoLoader7.bin")) &&
           fileExists(fsManager().resolveSystemPath("/_pico/picoLoader9.bin"));
}

std::string readVersionFile(const std::string& path) {
    FILE* file = fopen(path.c_str(), "r");
    if (!file) return "";

    char buffer[64] = {};
    if (!fgets(buffer, sizeof(buffer), file)) {
        fclose(file);
        return "";
    }
    fclose(file);

    std::string version(buffer);
    while (!version.empty() &&
           (version[version.size() - 1] == '\r' || version[version.size() - 1] == '\n' ||
            version[version.size() - 1] == ' ' || version[version.size() - 1] == '\t')) {
        version.erase(version.size() - 1);
    }
    return version;
}

std::string readPicoLoaderApiVersion(const std::string& path) {
    FILE* file = fopen(path.c_str(), "rb");
    if (!file) return "";

    if (fseek(file, offsetof(pload_header7_t, apiVersion), SEEK_SET) != 0) {
        fclose(file);
        return "";
    }

    u16 apiVersion = 0;
    if (fread(&apiVersion, sizeof(apiVersion), 1, file) != 1) {
        fclose(file);
        return "";
    }
    fclose(file);

    return formatString("API %u", static_cast<unsigned>(apiVersion));
}

std::string wrapText(const std::string& text, u32 maxLineWidth) {
    std::string wrapped;
    size_t lineStart = 0;

    while (lineStart <= text.size()) {
        size_t lineEnd = text.find('\n', lineStart);
        size_t lineLength = (lineEnd == std::string::npos) ? text.size() - lineStart
                                                             : lineEnd - lineStart;
        if (lineLength == 0) {
            wrapped += "\n";
        } else {
            wrapped += font().breakLine(text.substr(lineStart, lineLength), maxLineWidth);
        }

        if (lineEnd == std::string::npos) break;
        lineStart = lineEnd + 1;
        if (lineStart == text.size()) wrapped += "\n";
    }

    return wrapped;
}

size_t countLines(const std::string& text) {
    if (text.empty()) return 0;

    size_t lines = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n') ++lines;
    }
    if (text[text.size() - 1] != '\n') ++lines;
    return lines;
}

std::string getLines(const std::string& text, size_t firstLine, size_t lineCount) {
    size_t start = 0;
    for (size_t line = 0; line < firstLine; ++line) {
        size_t newline = text.find('\n', start);
        if (newline == std::string::npos) return "";
        start = newline + 1;
    }

    size_t end = start;
    for (size_t line = 0; line < lineCount && end < text.size(); ++line) {
        size_t newline = text.find('\n', end);
        if (newline == std::string::npos) {
            end = text.size();
        } else {
            end = newline + 1;
        }
    }
    return text.substr(start, end - start);
}
}  // namespace

cAboutWnd::cAboutWnd(s32 x, s32 y, u32 w, u32 h, cWindow* parent, const std::string& text)
    : cForm(x, y, w, h, parent, text),
      _buttonOK(0, 0, 46, 18, this, "\x01 OK"),
      _firstVisibleLine(0),
      _lineCount(0),
      _visibleLineCount(0) {
    s16 buttonY = size().y - _buttonOK.size().y - 4;

    _buttonOK.setStyle(cButton::press);
    _buttonOK.setText("\x01 " + LANG("setting window", "ok"));
    _buttonOK.setTextColor(uis().buttonTextColor);
    _buttonOK.loadAppearance(SFN_BUTTON3);
    _buttonOK.clicked.connect(this, &cAboutWnd::onOK);
    addChildWindow(&_buttonOK);

    s16 nextButtonX = size().x;

    s16 buttonPitch = _buttonOK.size().x + 8;
    buttonPitch = _buttonOK.size().x + 8;
    nextButtonX -= buttonPitch;
    _buttonOK.setRelativePosition(cPoint(nextButtonX, buttonY));

    loadAppearance("");
    arrangeChildren();

    const std::string notAvailable = LANG("about window", "not available");
    const std::string yes = LANG("message box", "yes");
    const std::string no = LANG("message box", "no");

    const std::string ndsBootstrapReleasePath =
            fsManager().resolveSystemPath("/_nds/nds-bootstrap-release.nds");
    const std::string ndsBootstrapNightlyPath =
            fsManager().resolveSystemPath("/_nds/nds-bootstrap-nightly.nds");
    const std::string ndsBootstrapHbPath =
            fsManager().resolveSystemPath("/_nds/nds-bootstrap-hb-release.nds");
    std::string ndsBootstrapVersion =
            readVersionFile(fsManager().resolveSystemPath("/_nds/release-bootstrap.ver"));
    if (ndsBootstrapVersion.empty()) {
        ndsBootstrapVersion =
                readVersionFile(fsManager().resolveSystemPath("/_nds/nightly-bootstrap.ver"));
    }
    const bool ndsBootstrapInstalled =
            fileExists(ndsBootstrapReleasePath) || fileExists(ndsBootstrapNightlyPath) ||
            fileExists(ndsBootstrapHbPath) || !ndsBootstrapVersion.empty();
    if (ndsBootstrapVersion.empty()) ndsBootstrapVersion = notAvailable;

    const std::string picoLoader7Path = fsManager().resolveSystemPath("/_pico/picoLoader7.bin");
    const std::string picoLoader9Path = fsManager().resolveSystemPath("/_pico/picoLoader9.bin");
    const bool picoLoaderInstalled = fileExists(picoLoader7Path) && fileExists(picoLoader9Path);
    std::string picoLoaderVersion =
            picoLoaderInstalled ? readPicoLoaderApiVersion(picoLoader7Path) : "";
    if (picoLoaderVersion.empty()) picoLoaderVersion = notAvailable;

    const std::string mode = LANG("about window", isDSiMode() ? "dsi mode" : "ds mode");
    const std::string storage =
            isDSPicoFlashcart() ? LANG("about window", "dspico")
                                : (fsManager().isFlashcart() ? LANG("about window", "flashcart")
                                                             : LANG("about window", "sd card"));
    const std::string akmenuVersion =
            std::string(AKMENU_VERSION_MAIN) + "." + AKMENU_VERSION_SUB;

    _helpText += formatString(LANG("about window", "app version").c_str(), akmenuVersion.c_str());
    _helpText += "\n";
    _helpText += formatString(LANG("about window", "mode").c_str(), mode.c_str(), storage.c_str());
    _helpText += "\n\n";
    _helpText += LANG("about window", "installed loaders");
    _helpText += "\n";
    _helpText += formatString(LANG("about window", "nds-bootstrap").c_str(),
                              ndsBootstrapInstalled ? yes.c_str() : no.c_str(),
                              ndsBootstrapVersion.c_str());
    _helpText += "\n";
    _helpText += formatString(LANG("about window", "pico-loader").c_str(),
                              picoLoaderInstalled ? yes.c_str() : no.c_str(),
                              picoLoaderVersion.c_str());
    _helpText += "\n\n";
    _helpText += formatString(LANG("about window", "help").c_str(), kHelpUrl);
    _helpText += "\n";
    _helpText += formatString(LANG("about window", "source").c_str(), kSourceUrl);
    _helpText += "\n";
    _helpText += LANG("about window", "license");
    _helpText += "\n";
    _helpText += LANG("about window", "license details");
    _helpText += "\n\n";
    _helpText += LANG("about window", "credits");

    const u16 textWidth = size().x > kTextRightMargin ? size().x - kTextRightMargin : 1;
    _helpText = wrapText(_helpText, textWidth);
    _lineCount = countLines(_helpText);
}

cAboutWnd::~cAboutWnd() {}

void cAboutWnd::draw() {
    _renderDesc.draw(windowRectangle(), _engine);

    const s16 textX = position().x + 8;
    const s16 textY = position().y + 17 + uiSettings().thickness;
    const s16 textBottom = _buttonOK.position().y - 2;
    const u16 textWidth = size().x > kTextRightMargin ? size().x - kTextRightMargin : 1;
    const u16 textHeight = textBottom > textY ? textBottom - textY : 0;
    _visibleLineCount = textHeight / gs().fontHeight;

    const size_t maxFirstVisibleLine =
            _lineCount > _visibleLineCount ? _lineCount - _visibleLineCount : 0;
    if (_firstVisibleLine > maxFirstVisibleLine) _firstVisibleLine = maxFirstVisibleLine;

    gdi().setPenColor(uiSettings().formTextColor, _engine);
    const std::string visibleText = getLines(_helpText, _firstVisibleLine, _visibleLineCount);
    gdi().textOutRect(textX, textY, textWidth, textHeight, visibleText.c_str(), _engine);

    if (_lineCount > _visibleLineCount) {
        const s16 indicatorX = position().x + size().x - 16;
        const s16 indicatorY = textY + (textHeight - 12) / 2;
        const u16 activeColor = uis().spinBoxTextColor;
        const u16 inactiveColor = uis().spinBoxNormalColor;
        drawScrollChevron(indicatorX, indicatorY, true,
                          _firstVisibleLine > 0 ? activeColor : inactiveColor, _engine);
        drawScrollChevron(indicatorX, indicatorY + 8, false,
                          _firstVisibleLine < maxFirstVisibleLine ? activeColor : inactiveColor,
                          _engine);
    }
    cForm::draw();
}

bool cAboutWnd::process(const akui::cMessage& msg) {
    bool ret = false;

    ret = cForm::process(msg);

    if (!ret) {
        if (msg.id() > cMessage::keyMessageStart && msg.id() < cMessage::keyMessageEnd) {
            ret = processKeyMessage((cKeyMessage&)msg);
        }
    }
    return ret;
}

bool cAboutWnd::processKeyMessage(const cKeyMessage& msg) {
    bool ret = false;
    if (msg.id() == cMessage::keyDown) {
        switch (msg.keyCode()) {
            case cKeyMessage::UI_KEY_A:
            case cKeyMessage::UI_KEY_B:
                onOK();
                ret = true;
                break;
            case cKeyMessage::UI_KEY_UP:
                if (_firstVisibleLine > 0) --_firstVisibleLine;
                ret = true;
                break;
            case cKeyMessage::UI_KEY_DOWN:
                if (_visibleLineCount == 0) _visibleLineCount = 1;
                if (_firstVisibleLine + _visibleLineCount < _lineCount) ++_firstVisibleLine;
                ret = true;
                break;
            default: {
            }
        };
    }
    return ret;
}

cWindow& cAboutWnd::loadAppearance(const std::string& aFileName) {
    _renderDesc.loadData(SFN_FORM_TITLE_L, SFN_FORM_TITLE_R, SFN_FORM_TITLE_M);
    _renderDesc.setTitleText(_text);
    return *this;
}

void cAboutWnd::onOK() {
    cForm::onOK();
}

void cAboutWnd::onShow() {
    centerScreen();
}
