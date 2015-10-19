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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

#include <linux/kdev_t.h>

#include "unicode/ucnv.h"
#include "unicode/ustring.h"
#define LOG_TAG "Vold"

#include <cutils/log.h>
#include <cutils/properties.h>

#include <logwrap/logwrap.h>

#include "Fat.h"
#include "VoldUtil.h"

static char FSCK_MSDOS_PATH[] = "/system/bin/fsck_msdos";
static char MKDOSFS_PATH[] = "/system/bin/newfs_msdos";
static char DOSFSLABEL_PATH[] = "/system/bin/dosfslabel";
extern "C" int logwrap(int argc, const char **argv, int background);
extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);

/**
* Read boot sector and volume info from a FAT filesystem
******************************************************
* ActionsCode(author:huoysh, new_method) 
*
*/ 
static int read_bootsectandvi (boot_sector *bs, fat_volume_info *volinfo,
				unsigned char *block, int *fatsize)
{
	fat_volume_info *vistart;
	memcpy(bs, block, sizeof(boot_sector));
	bs->reserved = FAT2CPU16(bs->reserved);
	bs->fat_length = FAT2CPU16(bs->fat_length);
	bs->secs_track = FAT2CPU16(bs->secs_track);
	bs->heads = FAT2CPU16(bs->heads);
	bs->total_sect = FAT2CPU32(bs->total_sect);

	/* FAT32 entries */
	if (bs->fat_length == 0) {
		/* Assume FAT32 */
		bs->fat32_length = FAT2CPU32(bs->fat32_length);
		bs->flags = FAT2CPU16(bs->flags);
		bs->root_cluster = FAT2CPU32(bs->root_cluster);
		bs->info_sector = FAT2CPU16(bs->info_sector);
		bs->backup_boot = FAT2CPU16(bs->backup_boot);
		vistart = (fat_volume_info *)(block + sizeof(boot_sector));
		*fatsize = 32;
	} else {
		vistart = (fat_volume_info *)&(bs->fat32_length);
		*fatsize = 0;
	}
	memcpy(volinfo, vistart, sizeof(fat_volume_info));

	if (*fatsize == 32) {
		if (strncmp(FAT32_SIGN, vistart->fs_type, SIGNLEN) == 0)
			return 0;
	} else {
		if (strncmp(FAT12_SIGN, vistart->fs_type, SIGNLEN) == 0) {
			*fatsize = 12;
			return 0;
		}
		if (strncmp(FAT16_SIGN, vistart->fs_type, SIGNLEN) == 0) {
			*fatsize = 16;
			return 0;
		}
	}
	return -1;
}
/**
* Read boot sector and volume info from a FAT filesystem
******************************************************
* ActionsCode(author:huoysh, new_method) 
*
*/
static size_t codec_convert( char *from_charset, char *to_charset,
                    char *inbuf, size_t inlen, char *outbuf, size_t outlen )
{
    size_t length = outlen;
    char *pin = inbuf;
    char *pout = outbuf;
    UErrorCode status = U_ZERO_ERROR;

    UConverter *conv = ucnv_open(from_charset, &status);
    if (U_FAILURE(status)) {
        ALOGE("could not create UConverter for %s\n", from_charset);
        return -1;
    }
    UConverter *destConv = ucnv_open(to_charset, &status);
    if (U_FAILURE(status)) {
        ALOGE("could not create UConverter for  for %s\n", to_charset);
        ucnv_close(conv);
        return -1;
    }
    ucnv_convertEx(destConv, conv, &pout, pout + outlen,
            (const char **)&pin, (const char *)pin + inlen, NULL, NULL, NULL, NULL, TRUE, TRUE, &status);
    if (U_FAILURE(status)) {
        ALOGE("ucnv_convertEx failed: %d\n", status);
    } else {
    // zero terminate
        *pout = 0;
    }
    ucnv_close(conv);
    ucnv_close(destConv);
    return 0;
}
/**
* Read boot sector and volume info from a FAT filesystem
******************************************************
* ActionsCode(author:huoysh, new_method) 
*
*/
int Fat::identify(const char *fsPath)
{
	boot_sector bs;
	fat_volume_info volinfo;
	int fd, size, fatsize;
	unsigned char block[FS_BLOCK_SIZE];
	int i;

	for(i=0; i<2; i++) {
		fd = open(fsPath, O_RDONLY);
		if(fd < 0) {
			SLOGE("Open device(%s) fail i=%d\n", fsPath, i);
			//usleep(1000);
		}
		else
			break;
	}
	//fd = open(fsPath, O_RDONLY);
	if (fd < 0) {
		SLOGE("Open device(%s) fail\n", fsPath);
		return -1;
	}

	size = read(fd, block, FS_BLOCK_SIZE);
	if (size < FS_BLOCK_SIZE) {
		SLOGE("Device(%s), Read data error\n", fsPath);
		close(fd);
		return -1;
	}
	close(fd);

	if (read_bootsectandvi(&bs, &volinfo, block, &fatsize)) {
		SLOGE("Device(%s) not Fat Fs\n", fsPath);
		return -1;
	} else {
		SLOGI("Device(%s) is Fat(%d)\n", fsPath, fatsize);
	}

	return 0;
}
/**
* 
******************************************************
* ActionsCode(author:huoysh, change_code) 
*
*/
int Fat::check(const char *fsPath) {
    bool rw = true;
    if (access(FSCK_MSDOS_PATH, X_OK)) {
        SLOGW("Skipping fs checks\n");
        return 0;
    }

    int pass = 1;
    int rc = 0;
    do {
        const char *args[4];
        int status;
        args[0] = FSCK_MSDOS_PATH;
        args[1] = "-p";
        args[2] = "-f";
        args[3] = fsPath;

        rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,
                false, true);
        if (rc != 0) {
            SLOGE("Filesystem check failed due to logwrap error");
            errno = EIO;
            return -1;
        }

        if (!WIFEXITED(status)) {
            SLOGE("Filesystem check did not exit properly");
            errno = EIO;
            return -1;
        }

        status = WEXITSTATUS(status);

        switch(status) {
        case 0:
            SLOGI("Filesystem check completed OK");
            return 0;
		//ActionsCode(huoysh, sync from gs702c_4.4.4) 
        case 1:
            SLOGI("Recoverable errors have been detected, completed OK");
            return 0;

        case 2:
            SLOGE("Filesystem check failed (not a FAT filesystem)");
            errno = ENODATA;
            return -1;

        case 4:
            if (pass++ <= 3) {
                SLOGW("Filesystem modified - rechecking (pass %d)",
                        pass);
                continue;
            }
            SLOGE("Failing check after too many rechecks");
            errno = EIO;
            return -1;

        default:
            SLOGE("Filesystem check failed (unknown exit code %d)", status);
            errno = EIO;
            return -1;
        }
    } while (0);

    return 0;
}
/**
* 
******************************************************
* ActionsCode(author:huoysh, change_code) 
*
*/
int Fat::doMount(const char *fsPath, const char *mountPoint,
                 bool ro, bool remount, bool executable,
                 int ownerUid, int ownerGid, int permMask, bool createLost) {
    int rc;
    unsigned long flags;
    char mountData[255];
	//ActionsCode(huoysh, sync from gs702c_4.4.4) 
    flags = MS_NODEV | MS_NOSUID | MS_DIRSYNC | MS_NOATIME;

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

    sprintf(mountData,
            "utf8,uid=%d,gid=%d,fmask=%o,dmask=%o,shortname=mixed",
            ownerUid, ownerGid, permMask, permMask);

    rc = mount(fsPath, mountPoint, "vfat", flags, mountData);

    if (rc && errno == EROFS) {
        SLOGE("%s appears to be a read only filesystem - retrying mount RO", fsPath);
        flags |= MS_RDONLY;
        rc = mount(fsPath, mountPoint, "vfat", flags, mountData);
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
/**
* 
******************************************************
* ActionsCode(author:huoysh, change_code) 
*
*/
int Fat::format(const char *fsPath, unsigned int numSectors, bool wipe) {
    int fd;
    const char *args[13];
    int rc;
    int status;

    if (wipe) {
        Fat::wipe(fsPath, numSectors);
    }

    args[0] = MKDOSFS_PATH;
    args[1] = "-F";
    args[2] = "32";
    args[3] = "-O";
    args[4] = "android";
    args[5] = "-c";
	//ActionsCode(huoysh, sync from gs702c_4.4.4) 
    args[6] = "32";//64//fix xp can't recognize unuse space;
    args[7] = "-A"; //sync from aosp 5.0
    args[8] = "-L";	
    args[9] = "NO NAME";

    if (numSectors) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%u", numSectors);
        const char *size = tmp;
		//ActionsCode(huoysh, sync from gs702c_4.4.4) 
        args[10] = "-s";
        args[11] = size;
        args[12] = fsPath;
        rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,
                false, true);
    } else {
		//ActionsCode(huoysh, sync from gs702c_4.4.4) 
        args[10] = fsPath;
        rc = android_fork_execvp(11, (char **)args, &status, false,
                true);
    }

    if (rc != 0) {
        SLOGE("Filesystem format failed due to logwrap error");
        errno = EIO;
        return -1;
    }

    if (!WIFEXITED(status)) {
        SLOGE("Filesystem format did not exit properly");
        errno = EIO;
        return -1;
    }

    status = WEXITSTATUS(status);

    if (status == 0) {
        SLOGI("Filesystem formatted OK");
        return 0;
    } else {
        SLOGE("Format failed (unknown exit code %d)", status);
        errno = EIO;
        return -1;
    }
    return 0;
}

int Fat::setLabel(const char *fsPath, const char *label) {
    int fd;
    const char *args[4];
    int rc;
    int status;
    char *utf8_lable;
    char ansi_label[255];
    
    // convert utf8 to ansi
    utf8_lable = (char*)label;
    codec_convert("UTF-8", "GBK", utf8_lable, strlen(utf8_lable)+1, 
                    ansi_label, sizeof(ansi_label));
    
    args[0] = DOSFSLABEL_PATH;
    args[1] = fsPath;
    args[2] = ansi_label;
    args[3] = NULL;

    rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,
            false, true);
	
    if (rc != 0) {
        SLOGE("Filesystem set label failed due to logwrap error");
        errno = EIO;
        return -1;
    }

    if (!WIFEXITED(status)) {
        SLOGE("Filesystem set label did not exit properly");
        errno = EIO;
        return -1;
    }

    status = WEXITSTATUS(status);

    if (status == 0) {
        SLOGI("Filesystem set label OK");
        return 0;
    } else {
        SLOGE("Set label failed (unknown exit code %d)", status);
        errno = EIO;
        return -1;
    }
    return 0;
}

void Fat::wipe(const char *fsPath, unsigned int numSectors) {
    int fd;
    unsigned long long range[2];

    fd = open(fsPath, O_RDWR);
    if (fd >= 0) {
        if (numSectors == 0) {
            numSectors = get_blkdev_size(fd);
        }
        if (numSectors == 0) {
            SLOGE("Fat wipe failed to determine size of %s", fsPath);
            close(fd);
            return;
        }
        range[0] = 0;
        range[1] = (unsigned long long)numSectors * 512;
        if (ioctl(fd, BLKDISCARD, &range) < 0) {
            SLOGE("Fat wipe failed to discard blocks on %s", fsPath);
        } else {
            SLOGI("Fat wipe %d sectors on %s succeeded", numSectors, fsPath);
        }
        close(fd);
    } else {
        SLOGE("Fat wipe failed to open device %s", fsPath);
    }
}
