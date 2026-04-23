#pragma once

#include <string>
#include <vector>
#include "singleton.h"

class cPluginManager {
  public:
    struct PluginAssociation {
        std::string extension;
        std::string launcherPath;
        bool useArgv;
    };

    void loadPlugins();
    const std::vector<std::string>& extensions() const;
    const PluginAssociation* findPlugin(const std::string& path) const;

  private:
    static std::string normalizeExtension(const std::string& value);
    static std::string resolveLauncherPath(const std::string& value);

  private:
    std::vector<PluginAssociation> _plugins;
    std::vector<std::string> _extensions;
};

typedef t_singleton<cPluginManager> PluginManager_s;
inline cPluginManager& pluginManager() {
    return PluginManager_s::instance();
}
