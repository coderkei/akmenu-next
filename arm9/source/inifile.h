/*
    inifile.h
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009-2010 yellow wood goblin

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <map>
#include <string>
#include <vector>

class CIniFile {
  public:
    CIniFile();
    CIniFile(const std::string& filename);
    virtual ~CIniFile();

  public:
    bool LoadIniFile(const std::string& FileName);
    bool SaveIniFile(const std::string& FileName);
    bool SaveIniFileModified(const std::string& FileName);
    bool HasSection(const std::string& Section) const;

    std::string GetString(const std::string& Section, const std::string& Item,
                          const std::string& DefaultValue);
    void SetString(const std::string& Section, const std::string& Item, const std::string& Value);
    int GetInt(const std::string& Section, const std::string& Item, int DefaultValue);
    void SetInt(const std::string& Section, const std::string& Item, int Value);
    void GetStringVector(const std::string& Section, const std::string& Item,
                         std::vector<std::string>& strings, char delimiter = ',');
    void SetStringVector(const std::string& Section, const std::string& Item,
                         std::vector<std::string>& strings, char delimiter = ',');

  protected:
    std::string m_sFileName;
    typedef std::vector<std::string> cStringArray;
    cStringArray m_FileContainer;
    bool m_bLastResult;
    bool m_bModified;
    bool m_bReadOnly;
    typedef std::map<std::string, size_t> cSectionCache;
    cSectionCache m_Cache;

    bool InsertLine(size_t line, const std::string& str);
    bool ReplaceLine(size_t line, const std::string& str);

    void SetFileString(const std::string& Section, const std::string& Item,
                       const std::string& Value);
    std::string GetFileString(const std::string& Section, const std::string& Item);

    std::string GetString(const std::string& Section, const std::string& Item);
    int GetInt(const std::string& Section, const std::string& Item);
};
