#pragma once

struct OverlayLanguage {
    const char* code;
    const char* label;
    const char* menuDirectory = nullptr;
};

inline constexpr OverlayLanguage overlayLanguages[] = {
    {"auto", "Auto"},
    {"ar", "Arabic"},
    {"ca", "Catalan"},
    {"cs", "Czech"},
    {"da", "Danish"},
    {"de", "German", "Deutsch"},
    {"el", "Greek"},
    {"en", "English", "English"},
    {"es", "Spanish", "Español"},
    {"fi", "Finnish"},
    {"fr", "French", "Français"},
    {"he", "Hebrew"},
    {"hu", "Hungarian"},
    {"id", "Indonesian"},
    {"it", "Italian", "Italiano"},
    {"ja", "Japanese", "日本語 (JP)"},
    {"ko", "Korean"},
    {"nl", "Dutch"},
    {"no", "Norwegian"},
    {"pl", "Polish"},
    {"pt", "Portuguese"},
    {"pt-BR", "Portuguese (BR)"},
    {"ro", "Romanian"},
    {"ru", "Russian", "Русский"},
    {"ry", "Rusyn"},
    {"sv", "Swedish"},
    {"tr", "Turkish"},
    {"uk", "Ukrainian"},
    {"val", "Valencian"},
    {"vi", "Vietnamese"},
    {"zh-CN", "Chinese (CN)", "简体中文 (CN)"},
    {"zh-TW", "Chinese (TW)", "繁體中文 (ZH)"},
};
