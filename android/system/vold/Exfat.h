#ifndef EXFAT_H
#define EXFAT_H

#include <unistd.h>

#define EXFAT_OEM_ID		"EXFAT   "

class Exfat {
public:
    static int identify(const char *fsPath);
    static int check(const char *fsPath);
    static int doMount(const char *fsPath, const char *mountPoint,
                 bool ro, bool remount, bool executable, 
                 int ownerUid, int ownerGid, int permMask, bool createLost);
    static int format(const char *fsPath, unsigned int numSectors);
};

#endif
