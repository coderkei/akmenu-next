#include "fsmngr.h"
#include "fifotool.h"

cFSManager::cFSManager() : _isSDInserted(false), _isFlashcart(false), _fsRoot() {

}

void cFSManager::init(int argc, char* argv[]) {
    _isSDInserted = checkSDInserted();

    // We're always on a flashcart if we're running in NTR mode
    if (!isDSiMode()) {
        _isFlashcart = true;
    // If no SD is inserted, we can only be on a flashcart
    } else if (!isSDInserted()) {
        _isFlashcart = true;
    // If argv tells us we've launched from fat, we're on a flashcart
    }else if (argc > 0 && strncmp(argv[0], "fat:/", 5) == 0) {
        _isFlashcart = true;
    }

    // Mount devices
    const bool mounted = fatInitDefault();
    (void)mounted;

    // If we can mount fat, we're on a flashcart
    if ((_isFlashcart || argc == 0 || strncmp(argv[0], "sd:/", 4) != 0) &&
        chdir("fat:/") == 0) {
        _isFlashcart = true;
        _fsRoot = "fat:";
    }

    if (isSDInserted()) {
        if (!isFlashcart()) {
            chdir("sd:/");
            _fsRoot = "sd:";
        }
    }
}

bool cFSManager::isFlashcart() const {
    return _isFlashcart;
}

bool cFSManager::isDsiSdMode() const {
    return isDSiMode() && !_isFlashcart;
}

bool cFSManager::mountNand() const {
    static bool mounted = false;
    if (!mounted) mounted = nandInit(true);
    return mounted;
}

bool cFSManager::isSDInserted() const {
    return _isSDInserted;
}

std::string cFSManager::resolveSystemPath(const char* path) const {
    return _fsRoot + path;
}

std::string cFSManager::getFSRoot() const {
    return _fsRoot + "/";
}

bool cFSManager::checkSDInserted() const {
    fifoSendValue32(FIFO_USER_01, MENU_MSG_IS_SD_INSERTED);

    fifoWaitValue32(FIFO_USER_01);

    int result = fifoGetValue32(FIFO_USER_01);

    return result != 0;
}
