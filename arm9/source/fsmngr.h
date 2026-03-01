#pragma once
#include <nds.h>
#include <nds/arm9/dldi.h>
#include <fat.h>
#include <string>
#include <vector>
#include "singleton.h"


class cFSManager {
public:
    cFSManager();
    void init(int argc, char* argv[]);
    bool isFlashcart() const;
    bool isSDInserted() const;
    std::string resolveSystemPath(const char* path) const;
    std::string getFSRoot() const;

private:
    bool checkSDInserted() const;
    bool _isSDInserted;
    bool _isFlashcart;
    std::string _fsRoot;
};


typedef t_singleton<cFSManager> FSManager_s;
inline cFSManager& fsManager() {
    return FSManager_s::instance();
}
