/*
    bmp15.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

//�

#include "bmp15.h"
#include <list>
#include <string>
#include "dbgtool.h"

cBMP15::cBMP15() : _width(0), _height(0), _pitch(0), _buffer(NULL) {}

cBMP15::cBMP15(u32 width, u32 height) : _width(0), _height(0), _pitch(0), _buffer(NULL) {
    _width = width;
    _height = height;
    _pitch = (width + (width & 1)) << 1;
    // u32 pitch = (((width*16)+31)>>5)<<2;            // 通用算法？
}

cBMP15::~cBMP15() {
    // dbg_printf( "cBMP15 %08x destructed\n", this );
}

cBMP15 createBMP15(u32 width, u32 height) {
    cBMP15 bmp(width, height);

    u32 pitch = bmp.pitch();  // 15bit bmp pitch 算法
    // dbg_printf( "pitch: %d bytes\n", pitch );
    // dbg_printf( "buffer %08x\n", bmp.buffer() );

    u32 bufferSize = height * pitch;
    if (bufferSize & 3)  // 如果 bufferSize 不是按4字节对齐，就把他调整到对齐
        bufferSize += 4 - (bufferSize & 3);
    bmp._buffer = new u32[bufferSize >> 2];
    return bmp;
}

typedef std::pair<std::string, cBMP15> str_bmp_pair;
typedef std::list<str_bmp_pair> str_bmp_list;
static str_bmp_list _bmpPool;

cBMP15 createBMP15FromMem(void* mem) {
    return cBMP15();
}

cBMP15 createBMP15FromFile(const std::string& filename) {
    // dbg_printf( "createBMP15FromFile (%s)\n", filename );

    str_bmp_list::iterator it;
    for (it = _bmpPool.begin(); it != _bmpPool.end(); ++it) {
        if (filename == it->first) {
            return it->second;
        }
    }

    FILE* f = fopen(filename.c_str(), "rb");
    if (NULL == f) {
        dbg_printf("(%s) file does not exist\n", filename.c_str());
        return cBMP15();
    }

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);

    if (fileSize < 54) {
        fclose(f);
        return cBMP15();
    }

    u16 bmMark = 0;
    fseek(f, 0, SEEK_SET);
    if (fread(&bmMark, 1, 2, f) != 2 || bmMark != 0x4d42) {  // 'B' 'M' header
        dbg_printf("not a bmp file\n");
        fclose(f);
        return cBMP15();
    }

    u32 width = 0;
    u32 height = 0;
    u16 planes = 0;
    u16 bitsPerPixel = 0;
    u32 compression = 0;
    u32 bmpDataOffset = 0;
    fseek(f, 0x12, SEEK_SET);
    if (fread(&width, 1, 4, f) != 4) {
        fclose(f);
        return cBMP15();
    }
    fseek(f, 0x16, SEEK_SET);
    if (fread(&height, 1, 4, f) != 4) {
        fclose(f);
        return cBMP15();
    }
    fseek(f, 0x1a, SEEK_SET);
    if (fread(&planes, 1, 2, f) != 2 || planes != 1) {
        fclose(f);
        return cBMP15();
    }
    fseek(f, 0x1c, SEEK_SET);
    if (fread(&bitsPerPixel, 1, 2, f) != 2 || bitsPerPixel != 16) {
        fclose(f);
        return cBMP15();
    }
    fseek(f, 0x1e, SEEK_SET);
    if (fread(&compression, 1, 4, f) != 4 || (compression != 0 && compression != 3)) {
        fclose(f);
        return cBMP15();
    }
    fseek(f, 0x0a, SEEK_SET);
    if (fread(&bmpDataOffset, 1, 4, f) != 4) {
        fclose(f);
        return cBMP15();
    }

    if (width == 0 || height == 0 || width > 1024 || height > 1024 || bmpDataOffset < 54 ||
        bmpDataOffset > (u32)fileSize) {
        fclose(f);
        return cBMP15();
    }

    u32 pitch = (width + (width & 1)) << 1;
    unsigned long long bitmapSize = (unsigned long long)pitch * height;
    if (bitmapSize > (unsigned long long)fileSize - bmpDataOffset) {
        fclose(f);
        return cBMP15();
    }

    cBMP15 bmp = createBMP15(width, height);
    if (!bmp.valid()) {
        fclose(f);
        return cBMP15();
    }

    long position = bmpDataOffset;
    fseek(f, position, SEEK_SET);
    u16* pbuffer = ((u16*)bmp.buffer()) + (bmp.pitch() >> 1) * height - (bmp.pitch() >> 1);

    for (u32 i = 0; i < height; ++i) {
        if (fread(pbuffer, 1, bmp.pitch(), f) != bmp.pitch()) {
            delete[] bmp._buffer;
            bmp._buffer = NULL;
            fclose(f);
            return cBMP15();
        }
        position += bmp.pitch();
        pbuffer -= bmp.pitch() >> 1;
        fseek(f, position, SEEK_SET);
    }
    fclose(f);

    pbuffer = (u16*)bmp.buffer();
    for (u32 i = 0; i < height; ++i) {
        for (u32 j = 0; j < (bmp.pitch() >> 1); ++j) {
            u16 pixelColor = pbuffer[i * (bmp.pitch() >> 1) + j];
            pixelColor = ((pixelColor & 0x7C00) >> 10) | ((pixelColor & 0x03E0)) |
                         ((pixelColor & 0x1F) << 10);
            pbuffer[i * (bmp.pitch() >> 1) + j] = pixelColor | (pixelColor ? BIT(15) : 0);
            // dbg_printf("%d               %d\n", j, i );
        }
    }

    str_bmp_pair bmpPoolItem(std::string(filename), bmp);
    _bmpPool.push_back(bmpPoolItem);

    // dbg_printf( "load bmp success, %08x\n", bmp.buffer() );
    return bmp;
}
