/*
    coverwnd.cpp
    Nintendo DS cover renderer.
    Copyright (C) 2026 coderkei

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "coverwnd.h"

#include <cstdio>
#include <cstring>

#include "gdi.h"
#include "globalsettings.h"
#include "systemfilenames.h"
#include "uisettings.h"

namespace {

const size_t kMaxCoverFileSize = 1024 * 1024;
const s16 kUnsetPosition = -32768;
const u32 kMaxSourceWidth = 1024;
const u32 kMaxSourceHeight = 1024;

bool readFile(const std::string& filename, std::vector<u8>& data) {
    FILE* file = fopen(filename.c_str(), "rb");
    if (!file) return false;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return false;
    }
    long size = ftell(file);
    if (size < 0 || (size_t)size > kMaxCoverFileSize) {
        fclose(file);
        return false;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return false;
    }
    data.resize((size_t)size);
    bool ok = data.empty() || fread(&data[0], 1, data.size(), file) == data.size();
    fclose(file);
    return ok;
}

u16 readLittleEndian16(const u8* value) {
    return (u16)(value[0] | ((u16)value[1] << 8));
}

u32 readLittleEndian32(const u8* value) {
    return (u32)value[0] | ((u32)value[1] << 8) | ((u32)value[2] << 16) | ((u32)value[3] << 24);
}

s32 readLittleEndianSigned32(const u8* value) {
    return (s32)readLittleEndian32(value);
}

bool isContiguousMask(u32 mask) {
    if (!mask) return true;
    while (!(mask & 1)) mask >>= 1;
    while (mask & 1) mask >>= 1;
    return !mask;
}

u8 componentFromMask(u32 value, u32 mask) {
    if (!mask) return 0;
    u32 shift = 0;
    while (!(mask & 1)) {
        ++shift;
        mask >>= 1;
    }
    u32 component = (value >> shift) & mask;
    return (u8)((component * 255 + mask / 2) / mask);
}

bool hasExtension(const std::string& filename, const char* extension) {
    size_t extensionLength = strlen(extension);
    if (filename.size() < extensionLength) return false;
    size_t start = filename.size() - extensionLength;
    for (size_t index = 0; index < extensionLength; ++index) {
        char value = filename[start + index];
        if (value >= 'A' && value <= 'Z') value = value - 'A' + 'a';
        if (value != extension[index]) return false;
    }
    return true;
}

bool isRomExtension(const std::string& filename) {
    return hasExtension(filename, ".nds") || hasExtension(filename, ".dsi") ||
           hasExtension(filename, ".srl") || hasExtension(filename, ".ids");
}

//trim off black padding that comes from the covers Pico-Cover outputs
void trimTrailingBlankColumns(std::vector<u16>& pixels, u16& width, u16 height) {
    const u16 kMinimumPaddingWidth = 4;
    u16 trimmedWidth = width;
    while (trimmedWidth > 1) {
        bool blank = true;
        for (u16 y = 0; y < height; ++y) {
            u16 pixel = pixels[(size_t)y * width + trimmedWidth - 1];
            if ((pixel & BIT(15)) && (pixel & 0x7fff)) {
                blank = false;
                break;
            }
        }
        if (!blank) break;
        --trimmedWidth;
    }

    if (width - trimmedWidth < kMinimumPaddingWidth) return;

    std::vector<u16> trimmedPixels((size_t)trimmedWidth * height);
    for (u16 y = 0; y < height; ++y) {
        memcpy(&trimmedPixels[(size_t)y * trimmedWidth], &pixels[(size_t)y * width],
               (size_t)trimmedWidth * sizeof(u16));
    }
    pixels.swap(trimmedPixels);
    width = trimmedWidth;
}

bool decodeBmp15(const std::string& filename, std::vector<u16>& pixels, u16& width, u16& height) {
    pixels.clear();
    width = height = 0;
    std::vector<u8> file;
    if (!readFile(filename, file) || file.size() < 54 || file[0] != 'B' || file[1] != 'M') return false;

    u32 pixelOffset = readLittleEndian32(&file[10]);
    u32 dibSize = readLittleEndian32(&file[14]);
    s32 sourceWidth = readLittleEndianSigned32(&file[18]);
    s32 sourceHeight = readLittleEndianSigned32(&file[22]);
    u16 planes = readLittleEndian16(&file[26]);
    u16 bitsPerPixel = readLittleEndian16(&file[28]);
    u32 compression = readLittleEndian32(&file[30]);
    const u32 kBiRgb = 0;
    const u32 kBiBitfields = 3;
    const u32 kBiAlphaBitfields = 6;
    if (dibSize < 40 || sourceWidth <= 0 || sourceHeight == 0 || sourceHeight == -2147483647 - 1 ||
        (size_t)dibSize > file.size() - 14 || pixelOffset < 14 + dibSize || planes != 1 ||
        (bitsPerPixel != 1 && bitsPerPixel != 4 && bitsPerPixel != 8 && bitsPerPixel != 16 &&
         bitsPerPixel != 24 && bitsPerPixel != 32) ||
        (compression != kBiRgb && compression != kBiBitfields && compression != kBiAlphaBitfields) ||
        ((compression == kBiBitfields || compression == kBiAlphaBitfields) &&
         bitsPerPixel != 16 && bitsPerPixel != 32))
        return false;

    bool bottomUp = sourceHeight > 0;
    u32 sourceHeightAbsolute = bottomUp ? (u32)sourceHeight : (u32)(-sourceHeight);
    if ((u32)sourceWidth > kMaxSourceWidth || sourceHeightAbsolute > kMaxSourceHeight) return false;
    u64 rowBits = (u64)(u32)sourceWidth * bitsPerPixel;
    u32 rowBytes = (u32)(((rowBits + 31) / 32) * 4);
    u64 imageSize = (u64)rowBytes * sourceHeightAbsolute;
    if (pixelOffset > file.size() || imageSize > file.size() - pixelOffset) return false;

    u32 redMask = 0, greenMask = 0, blueMask = 0, alphaMask = 0;
    if (bitsPerPixel == 16) {
        redMask = 0x7c00;
        greenMask = 0x03e0;
        blueMask = 0x001f;
    }
    if (compression == kBiBitfields || compression == kBiAlphaBitfields) {
        const size_t maskOffset = 14 + 40;
        if (maskOffset + 12 > file.size() || maskOffset + 12 > pixelOffset) return false;
        redMask = readLittleEndian32(&file[maskOffset]);
        greenMask = readLittleEndian32(&file[maskOffset + 4]);
        blueMask = readLittleEndian32(&file[maskOffset + 8]);
        if (dibSize >= 56 || compression == kBiAlphaBitfields) {
            if (maskOffset + 16 > file.size() || maskOffset + 16 > pixelOffset) return false;
            alphaMask = readLittleEndian32(&file[maskOffset + 12]);
        }
        if (!redMask || !greenMask || !blueMask || (redMask & greenMask) || (redMask & blueMask) ||
            (greenMask & blueMask) || !isContiguousMask(redMask) || !isContiguousMask(greenMask) ||
            !isContiguousMask(blueMask) || !isContiguousMask(alphaMask))
            return false;
    }

    size_t paletteOffset = 14 + dibSize;
    u32 paletteCount = 0;
    if (bitsPerPixel <= 8) {
        paletteCount = readLittleEndian32(&file[46]);
        if (!paletteCount) paletteCount = 1u << bitsPerPixel;
        if (paletteCount > (1u << bitsPerPixel) || paletteOffset > pixelOffset ||
            paletteCount > (pixelOffset - paletteOffset) / 4)
            return false;
    }

    width = (u16)(((u32)sourceWidth > SCREEN_WIDTH) ? SCREEN_WIDTH : sourceWidth);
    height = (u16)((sourceHeightAbsolute > SCREEN_HEIGHT) ? SCREEN_HEIGHT : sourceHeightAbsolute);
    pixels.assign((size_t)width * height, 0);

    bool useAlpha = false;
    if (bitsPerPixel == 32 && !alphaMask) {
        for (u32 row = 0; row < sourceHeightAbsolute && !useAlpha; ++row) {
            const u8* rowData = &file[pixelOffset + (size_t)row * rowBytes];
            for (u32 column = 0; column < (u32)sourceWidth; ++column) {
                if (rowData[column * 4 + 3]) {
                    useAlpha = true;
                    break;
                }
            }
        }
    }

    for (u32 row = 0; row < height; ++row) {
        u32 sourceRow = bottomUp ? sourceHeightAbsolute - row - 1 : row;
        const u8* rowData = &file[pixelOffset + (size_t)sourceRow * rowBytes];
        for (u32 column = 0; column < width; ++column) {
            u8 red = 0, green = 0, blue = 0, alpha = 255;
            if (bitsPerPixel <= 8) {
                u8 index = 0;
                if (bitsPerPixel == 8) {
                    index = rowData[column];
                } else if (bitsPerPixel == 4) {
                    index = (rowData[column >> 1] >> ((column & 1) ? 0 : 4)) & 0x0f;
                } else {
                    index = (rowData[column >> 3] >> (7 - (column & 7))) & 1;
                }
                if (index >= paletteCount) return false;
                const u8* color = &file[paletteOffset + (size_t)index * 4];
                blue = color[0];
                green = color[1];
                red = color[2];
            } else if (bitsPerPixel == 16 || compression == kBiBitfields ||
                       compression == kBiAlphaBitfields) {
                u32 value = bitsPerPixel == 16 ? readLittleEndian16(rowData + column * 2)
                                                : readLittleEndian32(rowData + column * 4);
                red = componentFromMask(value, redMask);
                green = componentFromMask(value, greenMask);
                blue = componentFromMask(value, blueMask);
                if (alphaMask) alpha = componentFromMask(value, alphaMask);
            } else {
                const u8* value = rowData + column * (bitsPerPixel >> 3);
                blue = value[0];
                green = value[1];
                red = value[2];
                if (bitsPerPixel == 32 && useAlpha) alpha = value[3];
            }
            u16 pixel = RGB15(red >> 3, green >> 3, blue >> 3);
            if (alpha) pixel |= BIT(15);
            pixels[(size_t)row * width + column] = pixel;
        }
    }
    return !pixels.empty();
}

}  // namespace

cCoverWnd::cCoverWnd() : _width(0), _height(0) {}

void cCoverWnd::clear() {
    _selectedPath.clear();
    _pixels.clear();
    _width = 0;
    _height = 0;
}

bool cCoverWnd::isSupportedDsRom(const std::string& selectedPath, DSRomInfo& romInfo) const {
    if (selectedPath.empty() || selectedPath[selectedPath.size() - 1] == '/') return false;
    return isRomExtension(selectedPath) && romInfo.isDSRom();
}

bool cCoverWnd::tryLoad(const std::string& filename) {
    std::vector<u16> pixels;
    u16 width = 0, height = 0;
    if (!decodeBmp15(filename, pixels, width, height)) return false;

    trimTrailingBlankColumns(pixels, width, height);
    _pixels.swap(pixels);
    _width = width;
    _height = height;
    return true;
}

bool cCoverWnd::loadCover(const std::string& selectedPath, DSRomInfo& romInfo) {
    char code[5] = {};
    memcpy(code, romInfo.saveInfo().gameCode, 4);
    bool validCode = code[0] && code[1] && code[2] && code[3] && !strchr(code, '/') && !strchr(code, '\\');
    if (validCode) {
        std::string codeBase = SFN_COVERS_CODE_DIRECTORY + std::string(code);
        if (tryLoad(codeBase + ".bmp")) return true;
    }

    size_t slash = selectedPath.find_last_of("/\\");
    std::string basename = selectedPath.substr(slash == std::string::npos ? 0 : slash + 1);
    size_t dot = basename.find_last_of('.');
    if (dot != std::string::npos) {
        basename.erase(dot);  // The checked ROM extension is the only removed suffix.
        std::string nameBase = SFN_COVERS_NAME_DIRECTORY + basename;
        if (tryLoad(nameBase + ".bmp")) return true;
    }

    if (validCode) {
        std::string picoCover = SFN_PICO_COVERS_NDS_DIRECTORY + std::string(code) + ".bmp";
        if (tryLoad(picoCover)) return true;
    }
    return false;
}

void cCoverWnd::update(const std::string& selectedPath, DSRomInfo& romInfo) {
    // Fix slow scrolling when covers are disabled or unsupported by the theme.
    if (!gs().showCovers || !uiSettings().supportsCovers) {
        clear();
        return;
    }
    if (_selectedPath == selectedPath) return;
    _selectedPath = selectedPath;
    _pixels.clear();
    _width = 0;
    _height = 0;
    if (isSupportedDsRom(selectedPath, romInfo)) loadCover(selectedPath, romInfo);
}

void cCoverWnd::drawBackdrop() const {
    if (!gs().showCovers || !uiSettings().supportsCovers || _pixels.empty() || !_width ||
        !_height)
        return;

    int darken = uiSettings().coverDarken;
    if (!darken) return;
    gdi().fillRectBlend(0, 0, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GE_SUB, darken);
}

void cCoverWnd::draw() const {
    if (!gs().showCovers || !uiSettings().supportsCovers || _pixels.empty() || !_width ||
        !_height)
        return;

    int destinationX = uiSettings().coverX;
    int destinationY = uiSettings().coverY;
    if (destinationX == kUnsetPosition) destinationX = (SCREEN_WIDTH - _width) / 2;
    if (destinationY == kUnsetPosition) destinationY = (SCREEN_HEIGHT - _height) / 2;

    int sourceX = 0, sourceY = 0;
    int drawWidth = _width, drawHeight = _height;
    if (destinationX < 0) {
        sourceX = -destinationX;
        drawWidth -= sourceX;
        destinationX = 0;
    }
    if (destinationY < 0) {
        sourceY = -destinationY;
        drawHeight -= sourceY;
        destinationY = 0;
    }
    if (destinationX + drawWidth > SCREEN_WIDTH) drawWidth = SCREEN_WIDTH - destinationX;
    if (destinationY + drawHeight > SCREEN_HEIGHT) drawHeight = SCREEN_HEIGHT - destinationY;
    if (drawWidth <= 0 || drawHeight <= 0) return;

    const u16* source = &_pixels[(size_t)sourceY * _width + sourceX];
    int fade = uiSettings().coverFade;
    if (fade) {
        gdi().blendBlt(source, _width, destinationX, destinationY, drawWidth, drawHeight, GE_SUB,
                       100 - fade);
    } else {
        gdi().maskBlt(source, _width, _height, destinationX, destinationY, drawWidth, drawHeight,
                      GE_SUB);
    }
}
