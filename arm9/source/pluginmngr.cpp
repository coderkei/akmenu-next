#include "pluginmngr.h"

#include <sys/dir.h>
#include <algorithm>
#include <cctype>

#include "dbgtool.h"
#include "inifile.h"
#include "systemfilenames.h"

namespace {
std::string fileExtensionFromPath(const std::string& path) {
    size_t slashPos = path.find_last_of("/\\");
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos || (slashPos != std::string::npos && dotPos < slashPos)) {
        return "";
    }
    return path.substr(dotPos);
}

std::string fileStemFromPath(const std::string& path) {
    size_t slashPos = path.find_last_of("/\\");
    size_t startPos = (slashPos == std::string::npos) ? 0 : slashPos + 1;
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos || dotPos < startPos) {
        return path.substr(startPos);
    }
    return path.substr(startPos, dotPos - startPos);
}

bool hasPrefix(const std::string& value, const char* prefix) {
    return value.rfind(prefix, 0) == 0;
}
}  // namespace

void cPluginManager::loadPlugins() {
    _plugins.clear();
    _extensions.clear();

    DIR* dir = opendir(SFN_PLUGINS_DIRECTORY.c_str());
    if (!dir) {
        dbg_printf("plugin directory missing: %s\n", SFN_PLUGINS_DIRECTORY.c_str());
        return;
    }

    struct dirent* entry = NULL;
    while ((entry = readdir(dir)) != NULL) {
        std::string fileName(entry->d_name);
        if (fileName == "." || fileName == "..") {
            continue;
        }

        std::string iniExt = normalizeExtension(fileExtensionFromPath(fileName));
        if (iniExt != ".ini") {
            continue;
        }

        std::string iniPath = SFN_PLUGINS_DIRECTORY + fileName;
        CIniFile ini;
        if (!ini.LoadIniFile(iniPath)) {
            continue;
        }

        std::string extension = normalizeExtension(
                ini.GetString("plugin", "extension", "." + fileStemFromPath(fileName)));
        std::string launcherPath = resolveLauncherPath(
                ini.GetString("plugin", "path",
                              ini.GetString("plugin", "launcher",
                                            ini.GetString("plugin", "app", ""))));
        bool useArgv = ini.GetInt("plugin", "argv", 0) != 0;
        bool useNdsBootstrapHb = ini.GetInt("plugin", "bootstrap", 0) != 0;

        if (extension.empty() || launcherPath.empty()) {
            dbg_printf("skipping invalid plugin ini: %s\n", iniPath.c_str());
            continue;
        }
        std::string iconPath = SFN_PLUGIN_ICONS_DIRECTORY + extension.substr(1) + ".bin";

        PluginAssociation plugin = {extension, launcherPath, iconPath, useArgv, useNdsBootstrapHb};
        _plugins.push_back(plugin);
        if (std::find(_extensions.begin(), _extensions.end(), extension) == _extensions.end()) {
            _extensions.push_back(extension);
        }
    }

    closedir(dir);
}

const std::vector<std::string>& cPluginManager::extensions() const {
    return _extensions;
}

const cPluginManager::PluginAssociation* cPluginManager::findPlugin(const std::string& path) const {
    std::string extension = normalizeExtension(fileExtensionFromPath(path));
    if (extension.empty()) {
        return NULL;
    }

    for (size_t i = 0; i < _plugins.size(); ++i) {
        if (_plugins[i].extension == extension) {
            return &_plugins[i];
        }
    }

    return NULL;
}

std::string cPluginManager::normalizeExtension(const std::string& value) {
    if (value.empty()) {
        return "";
    }

    std::string extension = value;
    if (extension[0] != '.') {
        extension = "." + extension;
    }

    for (size_t i = 0; i < extension.size(); ++i) {
        extension[i] = std::tolower(static_cast<unsigned char>(extension[i]));
    }

    return extension;
}

std::string cPluginManager::resolveLauncherPath(const std::string& value) {
    if (value.empty()) {
        return "";
    }

    if (hasPrefix(value, "fat:/") || hasPrefix(value, "sd:/")) {
        return value;
    }

    if (value[0] == '/') {
        return fsManager().resolveSystemPath(value.c_str());
    }

    if (hasPrefix(value, "_nds/")) {
        return fsManager().getFSRoot() + value;
    }

    return SFN_PLUGINS_DIRECTORY + value;
}
