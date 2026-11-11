/*
    Copyright (C) 2007 Acekard, www.acekard.com

    SPDX-License-Identifier: MIT
*/

#include "stringtool.h"
#include <malloc.h>
#include <cstdarg>
#include <cstdio>

std::string formatString(const char* fmt, ...) {
    const char* f = fmt;
    va_list argList;
    va_start(argList, fmt);
    char* ptempStr = NULL;
    size_t max_len = vasprintf(&ptempStr, f, argList);
    std::string str(ptempStr);
    str.resize(max_len);
    free(ptempStr);
    va_end(argList);
    return str;
}
