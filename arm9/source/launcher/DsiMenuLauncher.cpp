#include "DsiMenuLauncher.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <nds.h>
#include "../fsmngr.h"
#include "fifotool.h"

bool hasDsiMenuLauncher() {
    if (!fsManager().isDsiSdMode()) return false;

    fifoSendValue32(FIFO_USER_01, MENU_MSG_IS_3DS);
    fifoWaitValue32(FIFO_USER_01);
    if (fifoGetValue32(FIFO_USER_01) || !fsManager().mountNand()) return false;

    int region = EOF;
    FILE* hwinfo = fopen("nand:/sys/HWINFO_S.dat", "rb");
    if (hwinfo) {
        if (fseek(hwinfo, 0x90, SEEK_SET) == 0) region = fgetc(hwinfo);
        fclose(hwinfo);
    }

    const char regions[] = "JEPUCK";
    for (int i = 0; i < 6; ++i) {
        if (region >= 0 && region < 6 && region != i) continue;
        char directory[64];
        snprintf(directory, sizeof(directory), "nand:/title/00030017/484e41%02x/content/",
                 static_cast<unsigned int>(regions[i]));
        const std::string contentDir = directory;
        FILE* tmd = fopen((contentDir + "title.tmd").c_str(), "rb");
        if (!tmd) continue;

        unsigned char contentId[4];
        const bool haveContent = fseek(tmd, 0x1E4, SEEK_SET) == 0 &&
                                 fread(contentId, 1, sizeof(contentId), tmd) == sizeof(contentId);
        fclose(tmd);
        if (!haveContent) continue;

        char filename[13];
        snprintf(filename, sizeof(filename), "%02x%02x%02x%02x.app",
                 static_cast<unsigned int>(contentId[0]), static_cast<unsigned int>(contentId[1]),
                 static_cast<unsigned int>(contentId[2]), static_cast<unsigned int>(contentId[3]));
        const std::string path = contentDir + filename;
        struct stat st;
        if (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0) return true;
    }
    return false;
}

void launchDsiMenu() {
    memset((void*)0x02000800, 0, 12);
    memset((void*)0x02000300, 0, 0x20);
    *(vu32*)0x02000300 = 0x434E4C54;
    *(vu16*)0x02000304 = 0x1801;
    *(vu32*)0x02000310 = 0x4D454E55;
    DC_FlushAll();
    fifoSendValue32(FIFO_USER_01, MENU_MSG_UNLAUNCH_BOOT);
    while (true) swiWaitForVBlank();
}
