/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
* sync from gs702c_4.4.4
******************************************************
* ActionsCode(author:huoysh, new_method) 
*
*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>

#include <linux/kdev_t.h>

#define LOG_TAG "vold"

#include <cutils/log.h>
#include <cutils/properties.h>

#include "Exfat.h"

extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);

int Exfat::identify(const char *fsPath) {
    int i, rc =-1, cnt = 0, fd = 0;
	unsigned char *block = NULL;

    if(!(block = (unsigned char *)malloc(512))){
        goto out;
    }
    if((fd = open(fsPath, O_RDONLY)) < 0)
    {
        SLOGE("Unable to open device '%s' (%s)", fsPath, strerror(errno));
        goto out;
    }
    if((cnt = read(fd, block, 512)) != 512)
	{
        SLOGE("Unable to read device partition table (%d, %s)", cnt, strerror(errno));
        goto out;
    }

	if (memcmp(block+3, EXFAT_OEM_ID, 8)) {
		SLOGI("exfat identify fail");
		goto out;
	}
	SLOGI("Exfat System(%s) Identify success.", fsPath);
	rc = 0;

out:
    if(block)
	    free(block);
	if(fd >= 0)
		close(fd);
    return rc;
}

int Exfat::doMount(const char *fsPath, const char *mountPoint,
                 bool ro, bool remount, bool executable, 
                 int ownerUid, int ownerGid, int permMask, bool createLost) {
    int rc;
    unsigned long flags;
    char mountData[255];

    flags = MS_NODEV | MS_NOSUID | MS_DIRSYNC | MS_NOATIME | MS_NODIRATIME;

    flags |= (executable ? 0 : MS_NOEXEC);
    flags |= (ro ? MS_RDONLY : 0);
    flags |= (remount ? MS_REMOUNT : 0);

    /*
     * Note: This is a temporary hack. If the sampling profiler is enabled,
     * we make the SD card world-writable so any process can write snapshots.
     *
     * TODO: Remove this code once we have a drop box in system_server.
     */
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.sampling_profiler", value, "");
    if (value[0] == '1') {
        SLOGW("The SD card is world-writable because the"
            " 'persist.sampling_profiler' system property is set to '1'.");
        permMask = 0;
    }
    /* FIXME force to world-writable */
    sprintf(mountData,
            "uid=%d,gid=%d,fmask=%o,dmask=%o",
            ownerUid, ownerGid, permMask, permMask);

    rc = mount(fsPath, mountPoint, "exfat", flags, mountData);

    if (rc && errno == EROFS) {
        SLOGE("%s appears to be a read only filesystem - retrying mount RO", fsPath);
        flags |= MS_RDONLY;
        rc = mount(fsPath, mountPoint, "exfat", flags, mountData);
    }

    if (rc == 0 && createLost) {
        char *lost_path;
        asprintf(&lost_path, "%s/LOST.DIR", mountPoint);
        if (access(lost_path, F_OK)) {
            /*
             * Create a LOST.DIR in the root so we have somewhere to put
             * lost cluster chains (fsck_msdos doesn't currently do this)
             */
            if (mkdir(lost_path, 0755)) {
                SLOGE("Unable to create LOST.DIR (%s)", strerror(errno));
            }
        }
        free(lost_path);
    }

    return rc;
}

/* Don't Support Format */
int Exfat::format(const char *fsPath, unsigned int numSectors) {
    return -1;
}
