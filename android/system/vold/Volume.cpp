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
#include <sys/param.h>

#include <linux/kdev_t.h>

#include <cutils/properties.h>

#include <diskconfig/diskconfig.h>

#include <private/android_filesystem_config.h>

#define LOG_TAG "Vold"

#include <cutils/fs.h>
#include <cutils/log.h>

#include <string>

#include "Volume.h"
#include "VolumeManager.h"
#include "ResponseCode.h"
#include "Fat.h"
#include "Ntfs.h"
#include "Exfat.h"
#include "Process.h"
#include "cryptfs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>

extern "C" void dos_partition_dec(void const *pp, struct dos_partition *d);
extern "C" void dos_partition_enc(void *pp, struct dos_partition *d);


/*
 * Media directory - stuff that only media_rw user can see
 */
const char *Volume::MEDIA_DIR           = "/mnt/media_rw";

/*
 * Fuse directory - location where fuse wrapped filesystems go
 */
const char *Volume::FUSE_DIR           = "/storage";

/*
 * Path to external storage where *only* root can access ASEC image files
 */
const char *Volume::SEC_ASECDIR_EXT   = "/mnt/secure/asec";

/*
 * Path to internal storage where *only* root can access ASEC image files
 */
const char *Volume::SEC_ASECDIR_INT   = "/data/app-asec";

/*
 * Path to where secure containers are mounted
 */
const char *Volume::ASECDIR           = "/mnt/asec";

/*
 * Path to where OBBs are mounted
 */
const char *Volume::LOOPDIR           = "/mnt/obb";

const char *Volume::BLKID_PATH = "/system/bin/blkid";

static const char *stateToStr(int state) {
    if (state == Volume::State_Init)
        return "Initializing";
    else if (state == Volume::State_NoMedia)
        return "No-Media";
    else if (state == Volume::State_Idle)
        return "Idle-Unmounted";
    else if (state == Volume::State_Pending)
        return "Pending";
    else if (state == Volume::State_Mounted)
        return "Mounted";
    else if (state == Volume::State_Unmounting)
        return "Unmounting";
    else if (state == Volume::State_Checking)
        return "Checking";
    else if (state == Volume::State_Formatting)
        return "Formatting";
    else if (state == Volume::State_Shared)
        return "Shared-Unmounted";
    else if (state == Volume::State_SharedMnt)
        return "Shared-Mounted";
    else
        return "Unknown-Error";
}
/**
* sync from gs702c_4.4.4
******************************************************
* ActionsCode(author:huoysh, new_method) 
*
*/ 

static int sync_ptable(int fd){
	ALOGE("START sync_ptable");
	struct stat stat;
	int rv;
	sync();
	if (fstat(fd, &stat)) {
		ALOGE("Cannot stat, errno=%d.", errno); 
		return -1;
	}
	if (S_ISBLK(stat.st_mode) && ((rv = ioctl(fd, BLKRRPART, NULL)) < 0)) {
		ALOGE("Could not re-read partition table. REBOOT!. (errno=%d)", errno);
		return -1;
	}
	ALOGD("END sync_ptable");
	return 0;
}

Volume::Volume(VolumeManager *vm, const fstab_rec* rec, int flags) {
    mVm = vm;
    mDebug = false;
    mLabel = strdup(rec->label);
    mUuid = NULL;
    mUserLabel = NULL;
    mState = Volume::State_Init;
    mFlags = flags;
    mCurrentlyMountedKdev = -1;
    mPartIdx = rec->partnum;
    mRetryMount = false;
	//ActionsCode(huoysh, sync from gs702c_4.4.4) 
	mRetry = 0;
    mIsDiskPlugOut	= 0;
}

Volume::~Volume() {
    free(mLabel);
    free(mUuid);
    free(mUserLabel);
}

void Volume::setDebug(bool enable) {
    mDebug = enable;
}

dev_t Volume::getDiskDevice() {
    return MKDEV(0, 0);
};

dev_t Volume::getShareDevice() {
    return getDiskDevice();
}

void Volume::handleVolumeShared() {
}

void Volume::handleVolumeUnshared() {
}

int Volume::handleBlockEvent(NetlinkEvent * /*evt*/) {
    errno = ENOSYS;
    return -1;
}

void Volume::setUuid(const char* uuid) {
    char msg[256];

    if (mUuid) {
        free(mUuid);
    }

    if (uuid) {
        mUuid = strdup(uuid);
        snprintf(msg, sizeof(msg), "%s %s \"%s\"", getLabel(),
                getFuseMountpoint(), mUuid);
    } else {
        mUuid = NULL;
        snprintf(msg, sizeof(msg), "%s %s", getLabel(), getFuseMountpoint());
    }

    mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeUuidChange, msg,
            false);
}

void Volume::setUserLabel(const char* userLabel) {
    char msg[256];

    if (mUserLabel) {
        free(mUserLabel);
    }

    if (userLabel) {
        mUserLabel = strdup(userLabel);
        snprintf(msg, sizeof(msg), "%s %s \"%s\"", getLabel(),
                getFuseMountpoint(), mUserLabel);
    } else {
        mUserLabel = NULL;
        snprintf(msg, sizeof(msg), "%s %s", getLabel(), getFuseMountpoint());
    }

    mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeUserLabelChange,
            msg, false);
}
/**
* sync from gs702c_4.4.4
******************************************************
* ActionsCode(author:huoysh, new_method) 
*
*/ 
bool Volume::isPrimaryStorage()
{
    const char* externalStorage = getenv("EXTERNAL_STORAGE") ? : "/storage/sdcard";
    return !strcmp(getFuseMountpoint(), externalStorage);
}
void Volume::setState(int state) {
    char msg[255];
    int oldState = mState;

    if (oldState == state) {
        SLOGW("Duplicate state (%d)\n", state);
        return;
    }

    if ((oldState == Volume::State_Pending) && (state != Volume::State_Idle)) {
        mRetryMount = false;
    }

    mState = state;

    SLOGD("Volume %s state changing %d (%s) -> %d (%s)", mLabel,
         oldState, stateToStr(oldState), mState, stateToStr(mState));
    snprintf(msg, sizeof(msg),
             "Volume %s %s state changed from %d (%s) to %d (%s)", getLabel(),
             getFuseMountpoint(), oldState, stateToStr(oldState), mState,
             stateToStr(mState));

    mVm->getBroadcaster()->sendBroadcast(ResponseCode::VolumeStateChange,
                                         msg, false);
}

int Volume::createDeviceNode(const char *path, int major, int minor) {
    mode_t mode = 0660 | S_IFBLK;
    dev_t dev = (major << 8) | minor;
    if (mknod(path, mode, dev) < 0) {
        if (errno != EEXIST) {
            return -1;
        }
    }
    return 0;
}
/**
* 
******************************************************
* ActionsCode(author:huoysh, change_code) 
*
*/
int Volume::formatVol(bool wipe) {
	//ActionsCode(huoysh, sync from gs702c_4.4.4) 
    bool primaryStorage = isPrimaryStorage();
    if (getState() == Volume::State_NoMedia) {
        errno = ENODEV;
        return -1;
    } else if (getState() != Volume::State_Idle) {
        errno = EBUSY;
        return -1;
    }

    if (isMountpointMounted(getMountpoint())) {
        SLOGW("Volume is idle but appears to be mounted - fixing");
        setState(Volume::State_Mounted);
        // mCurrentlyMountedKdev = XXX
        errno = EBUSY;
        return -1;
    }

    bool formatEntireDevice = (mPartIdx == -1);
    char devicePath[255];
    dev_t diskNode = getDiskDevice();
    dev_t deviceNodes[16];
    getDeviceNodes((dev_t *) &deviceNodes, 16);
    int partition_minor =  minor(deviceNodes[0]) ;		
   //ActionsCode(huoysh, fix BUG00233123:minor err,can't format vol) 
   // dev_t partNode =
   //     MKDEV(MAJOR(diskNode),
   //         MINOR(diskNode) + (formatEntireDevice ? 1 : mPartIdx));
    dev_t partNode = MKDEV(MAJOR(diskNode), (formatEntireDevice ? MINOR(diskNode) : partition_minor));

    setState(Volume::State_Formatting);

    int ret = -1;
    // Only initialize the MBR if we are formatting the entire device
    if (formatEntireDevice) {
        sprintf(devicePath, "/dev/block/vold/%d:%d",
                major(diskNode), minor(diskNode));

        if (initializeMbr(devicePath)) {
            SLOGE("Failed to initialize MBR (%s)", strerror(errno));
            goto err;
        }
    }

    sprintf(devicePath, "/dev/block/vold/%d:%d",
            major(partNode), minor(partNode));

    if (mDebug) {
        SLOGI("Formatting volume %s (%s)", getLabel(), devicePath);
    }

    if (Fat::format(devicePath, 0, wipe)) {
        SLOGE("Failed to format (%s)", strerror(errno));
        goto err;
    }
	//ActionsCode(huoysh, clear label flag if format primary storage) 
    if (primaryStorage) {
        property_set("persist.vold.set_label_done", "0");
    }

    ret = 0;

err:
    setState(Volume::State_Idle);
    return ret;
}

bool Volume::isMountpointMounted(const char *path) {
    char device[256];
    char mount_path[256];
    char rest[256];
    FILE *fp;
    char line[1024];

    if (!(fp = fopen("/proc/mounts", "r"))) {
        SLOGE("Error opening /proc/mounts (%s)", strerror(errno));
        return false;
    }

    while(fgets(line, sizeof(line), fp)) {
        line[strlen(line)-1] = '\0';
        sscanf(line, "%255s %255s %255s\n", device, mount_path, rest);
        if (!strcmp(mount_path, path)) {
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    return false;
}
/**
* 
******************************************************
* ActionsCode(author:huoysh, change_code) 
*
*/
int Volume::mountVol() {
    dev_t deviceNodes[16];
    int n, i, rc = 0;
    char errmsg[255];
    //ActionsCode(huoysh, sync from gs702c_4.4.4) 	
    bool primaryStorage = isPrimaryStorage();
    int flags = getFlags();
    bool providesAsec = (flags & VOL_PROVIDES_ASEC) != 0;
    char mpoint[256];
    int multiPartMountedNum = 0; // determin is we have mount some partitions successfully.

    // TODO: handle "bind" style mounts, for emulated storage

    char decrypt_state[PROPERTY_VALUE_MAX];
    char crypto_state[PROPERTY_VALUE_MAX];
    char encrypt_progress[PROPERTY_VALUE_MAX];

    property_get("vold.decrypt", decrypt_state, "");
    property_get("vold.encrypt_progress", encrypt_progress, "");

    /* Don't try to mount the volumes if we have not yet entered the disk password
     * or are in the process of encrypting.
     */
    SLOGE("mountVol(getstate=%d,%s,%s)",getState() ,mLabel,getLabel());
    if ((getState() == Volume::State_NoMedia) ||
        ((!strcmp(decrypt_state, "1") || encrypt_progress[0]) && providesAsec)) {
        SLOGW("mountVol %c %d %d failed\n",decrypt_state,encrypt_progress[0],providesAsec);
        snprintf(errmsg, sizeof(errmsg),
                 "Volume %s %s mount failed - no media",
                 getLabel(), getFuseMountpoint());
        mVm->getBroadcaster()->sendBroadcast(
                                         ResponseCode::VolumeMountFailedNoMedia,
                                         errmsg, false);
        errno = ENODEV;
        return -1;
    } else if (getState() != Volume::State_Idle) {
        errno = EBUSY;
        if (getState() == Volume::State_Pending) {
            mRetryMount = true;
        }
        return -1;
    }

    if (isMountpointMounted(getMountpoint())) {
        SLOGW("Volume is idle but appears to be mounted - fixing 1");
        setState(Volume::State_Mounted);
        // mCurrentlyMountedKdev = XXX
        return 0;
    }
    //ActionsCode(huoysh, sync from gs702c_4.4.4) 
    n = getDeviceNodes((dev_t *) &deviceNodes, 16);
    if (!n) {
        SLOGE("Failed to get device nodes (%s)\n", strerror(errno));
        return -1;
    }
	
    //ActionsCode(huoysh, sync from GS703D,add multi partition ) 	
    if((n> 1)&&(!primaryStorage)){ // multi-parts
        if (isMountpointMounted(getFuseMountpoint())) {
            SLOGW("Volume is idle but appears to be mounted - fixing 2");
            for (i = 0; i < n; i++) {
                memset(mpoint, 0, sizeof(mpoint));
                snprintf(mpoint, sizeof(mpoint), "%s/partition%d", getFuseMountpoint(), i);
                if (isMountpointMounted(mpoint)) {
                    if (doUnmount(mpoint, true)) {
                        SLOGE("Failed to unmount %s (%s)", mpoint, strerror(errno));
                        setState(Volume::State_Mounted);
                        return 0;
                    }
                    mount("rootfs", "/", NULL, MS_REMOUNT, NULL);
                    rmdir(mpoint);
                    mount("rootfs", "/", NULL, MS_REMOUNT | MS_RDONLY, NULL);
                }
            }
            if (doUnmount(getFuseMountpoint(), true)) {
                SLOGE("Failed to unmount %s (%s)", getFuseMountpoint(), strerror(errno));
                setState(Volume::State_Mounted);
                return 0;
            }
        }
    }

    /* If we're running encrypted, and the volume is marked as encryptable and nonremovable,
     * and also marked as providing Asec storage, then we need to decrypt
     * that partition, and update the volume object to point to it's new decrypted
     * block device
     */
    property_get("ro.crypto.state", crypto_state, "");
    if (providesAsec &&
        ((flags & (VOL_NONREMOVABLE | VOL_ENCRYPTABLE))==(VOL_NONREMOVABLE | VOL_ENCRYPTABLE)) &&
        !strcmp(crypto_state, "encrypted") && !isDecrypted()) {
       char new_sys_path[MAXPATHLEN];
       char nodepath[256];
       int new_major, new_minor;

       if (n != 1) {
           /* We only expect one device node returned when mounting encryptable volumes */
           SLOGE("Too many device nodes returned when mounting %s\n", getMountpoint());
           return -1;
       }

       if (cryptfs_setup_volume(getLabel(), MAJOR(deviceNodes[0]), MINOR(deviceNodes[0]),
                                new_sys_path, sizeof(new_sys_path),
                                &new_major, &new_minor)) {
           SLOGE("Cannot setup encryption mapping for %s\n", getMountpoint());
           return -1;
       }
       /* We now have the new sysfs path for the decrypted block device, and the
        * majore and minor numbers for it.  So, create the device, update the
        * path to the new sysfs path, and continue.
        */
        snprintf(nodepath,
                 sizeof(nodepath), "/dev/block/vold/%d:%d",
                 new_major, new_minor);
        if (createDeviceNode(nodepath, new_major, new_minor)) {
            SLOGE("Error making device node '%s' (%s)", nodepath,
                                                       strerror(errno));
        }

        // Todo: Either create sys filename from nodepath, or pass in bogus path so
        //       vold ignores state changes on this internal device.
        updateDeviceInfo(nodepath, new_major, new_minor);
        //ActionsCode(huoysh, Get the device nodes again, because they just changed) 
        n = getDeviceNodes((dev_t *) &deviceNodes, 16);
        if (!n) {
            SLOGE("Failed to get device nodes (%s)\n", strerror(errno));
            return -1;
        }
    }
	
    //ActionsCode(huoysh, sync from GS703D,add multi partition ) 
    if((n > 1)&&(!primaryStorage)){ // multi-parts
        flags = MS_NOATIME | MS_NOSUID | MS_NODEV;
        if (mount("tmpfs", getFuseMountpoint(), "tmpfs", flags, "size=1048576,mode=755,uid=1000,gid=1000")) {
            SLOGE("Failed to tmpfs on (%s)<-(%s)", getFuseMountpoint(), strerror(errno));
            return -1;
        }
    }
    for (i = 0; i < n; i++) {
        char devicePath[255];

        sprintf(devicePath, "/dev/block/vold/%d:%d", major(deviceNodes[i]),
                minor(deviceNodes[i]));

        SLOGI("%s being considered for volume %s\n", devicePath, getLabel());

        errno = 0;
        setState(Volume::State_Checking);

        /*
	  * ActionsCode(huoysh,) 
         * Mount the device on our internal staging mountpoint so we can
         * muck with it before exposing it to non priviledged users.
         */
        errno = 0;
        int uid, gid, perm_mask=0;
        memset(mpoint, 0, sizeof(mpoint));
        if ((n > 1)&&(!primaryStorage)) {
            snprintf(mpoint, sizeof(mpoint), "%s/partition%d", getFuseMountpoint(), i);
            SLOGD("mpoint = %s", mpoint);
            if (mount("rootfs", "/", NULL, MS_REMOUNT, NULL)){
                SLOGD("mount remount failed: %s", strerror(errno));
            }
            if (mkdir(mpoint, 0755)) {
                SLOGD("mkdir failed: %s", strerror(errno));
            }
            if (mount("rootfs", "/", NULL, MS_REMOUNT | MS_RDONLY, NULL)){
                SLOGD("mount ro failed: %s", strerror(errno));
            }
            uid = AID_SYSTEM;
            gid = AID_SDCARD_RW;
            perm_mask = 0002;
        } else {
            snprintf(mpoint, sizeof(mpoint), "%s", getMountpoint());
            uid = AID_MEDIA_RW;
            gid = AID_MEDIA_RW;
            perm_mask = 0007;
        }
        //ActionsCode(huoysh,Identify FS type and mount) 
        if (Fat::identify(devicePath) == 0) {
            if (Fat::check(devicePath)) {
                if (errno == ENODATA) {
                    SLOGW("%s does not contain a FAT filesystem\n", devicePath);
                    continue;
                }
                errno = EIO;
                /* Badness - abort the mount */
                SLOGE("%s failed FS checks (%s)", devicePath, strerror(errno));
                setState(Volume::State_Idle);
                return -1;
            }


            if (Fat::doMount(devicePath, mpoint, false, false, false,
                    uid, gid, perm_mask, true)) {
                SLOGE("%s failed to mount via VFAT (%s)\n", devicePath, strerror(errno));
                continue;
            }
            
            /* primary storage: only set volume label once */
            if (primaryStorage) {
                char value[PROPERTY_VALUE_MAX];
                property_get("persist.vold.set_label_done", value, "0");
                if (value[0] == '0') {
                    /* get label string */
                    property_get("ro.usbdevice.volumelabel", value, "");

                    /* set Fat volume label */
                    if (Fat::setLabel(devicePath, value)) {
                        SLOGE("%s failed to set label via VFAT (%s)\n", devicePath, strerror(errno));
                        /* retry set Fat volume label */
                        if (Fat::setLabel(devicePath, value)) {
                            SLOGE("retry: %s failed to set label via VFAT (%s)\n", devicePath, strerror(errno));
                        } else {
                            /* set label flag if success*/
                            property_set("persist.vold.set_label_done", "1");
                        }
                    } else {
                        /* set label flag if success*/
                        property_set("persist.vold.set_label_done", "1");
                    }
                }
            }
        } else if (Ntfs::identify(devicePath) == 0) {
        if (Ntfs::doMount(devicePath, mpoint, false, false, false,
                    uid, gid, perm_mask, true)) {
                SLOGE("%s failed to mount via NTFS (%s)\n", devicePath, strerror(errno));
                continue;
            }
        } else if (Exfat::identify(devicePath) == 0) {
            if (Exfat::doMount(devicePath, mpoint, false, false, false,
                    uid, gid, perm_mask, true)) {
                SLOGE("%s failed to mount via EXFAT (%s)\n", devicePath, strerror(errno));
                continue;
            }
        } else {
            continue;
        }
        //ActionsCode(huoysh,Unmount the volumes if disk had been removed)
        if (getState() == Volume::State_NoMedia) {
            errno = ENODEV;
            SLOGE("Disk removed after mounted (%s)", strerror(errno));
            umount(mpoint);
            snprintf(errmsg, sizeof(errmsg),
                     "Volume %s %s mount failed - no media",
                     getLabel(), mpoint);
            mVm->getBroadcaster()->sendBroadcast(
                                             ResponseCode::VolumeMountFailedNoMedia,
                                             errmsg, false);
            setState(Volume::State_Idle);
            return -1;
        }

        SLOGD("Device %s, target %s mounted @ /mnt/secure/staging", devicePath, mpoint);

        extractMetadata(devicePath);

        if (providesAsec && mountAsecExternal() != 0) {
            SLOGE("Failed to mount secure area (%s)", strerror(errno));
			//ActionsCode(huoysh,force mount for read-only ntfs)
//            umount(getMountpoint());
//            setState(Volume::State_Idle);
//            return -1;
        }

        SLOGD("mountVol. %d parts mounted", i);
        if ( n ==  1) {
            char service[64];
            struct statfs  stat;
            int timeout_cnt = 0;

            snprintf(service, 64, "fuse_%s", getLabel());
            property_set("ctl.start", service);

            //ActionsCode(hmwei, wait fuse service finish)
            memset(&stat, 0, sizeof(struct statfs));
            do {
                if(mIsDiskPlugOut){
                    SLOGD(" while waiting fuse service disk pluged out,exit!! \n");
                    break;			
                }
                statfs(getFuseMountpoint(), &stat);
                SLOGD("stat.f_blocks:%lld, timeout_cnt:%d\n", stat.f_blocks, timeout_cnt);
                usleep(1000*100);	//100ms
                timeout_cnt++;
            } while((stat.f_blocks == 0) && (timeout_cnt != 50));
        }
        multiPartMountedNum++;
        mCurrentlyMountedKdev = deviceNodes[i];
        if (i == n - 1) {
            setState(Volume::State_Mounted);
            mRetry=0;
            return 0;
        }
    }

	// if multi-patition, and have mounted some, then assume the disk is OK,		
	// this can fix the problem that hissens reported, the last parition is invalid 
	// which caused the entier disk unusable.
    if((n > 1)&&(!primaryStorage)){
        if(multiPartMountedNum > 0) {
            setState(Volume::State_Mounted);
            mRetry=0;
            return 0;
        }
    }
    //ActionsCode(huoysh,add by wgh BUG00106449)
    if(mRetry<3){
        dev_t d = getShareDevice();
        if ((MAJOR(d) == 0) && (MINOR(d) == 0)) {
            // This volume does not support raw disk access
            return 0;
        }
        char nodepath[255];
        snprintf(nodepath,
	             sizeof(nodepath), "/dev/block/vold/%d:%d",
	             major(d), minor(d));
        SLOGD("nodepath:%s",nodepath);
        int fd_table = open(nodepath, O_RDONLY);
        if (fd_table >= 0) {
            sync_ptable(fd_table);
            close(fd_table);
            mRetry++;
        }
    }

	
    SLOGE("Volume %s found no suitable devices for mounting :(\n", getLabel());
    setState(Volume::State_Idle);

    return -1;
}

int Volume::mountAsecExternal() {
    char legacy_path[PATH_MAX];
    char secure_path[PATH_MAX];

    snprintf(legacy_path, PATH_MAX, "%s/android_secure", getMountpoint());
    snprintf(secure_path, PATH_MAX, "%s/.android_secure", getMountpoint());

    // Recover legacy secure path
    if (!access(legacy_path, R_OK | X_OK) && access(secure_path, R_OK | X_OK)) {
        if (rename(legacy_path, secure_path)) {
            SLOGE("Failed to rename legacy asec dir (%s)", strerror(errno));
        }
    }

    if (fs_prepare_dir(secure_path, 0770, AID_MEDIA_RW, AID_MEDIA_RW) != 0) {
        SLOGW("fs_prepare_dir failed: %s", strerror(errno));
        return -1;
    }

    if (mount(secure_path, SEC_ASECDIR_EXT, "", MS_BIND, NULL)) {
        SLOGE("Failed to bind mount points %s -> %s (%s)", secure_path,
                SEC_ASECDIR_EXT, strerror(errno));
        return -1;
    }

    return 0;
}

int Volume::doUnmount(const char *path, bool force) {
    int retries = 10;

    if (mDebug) {
        SLOGD("Unmounting {%s}, force = %d", path, force);
    }

    while (retries--) {
        if (!umount(path) || errno == EINVAL || errno == ENOENT) {
            SLOGI("%s sucessfully unmounted", path);
            return 0;
        }

        int action = 0;

        if (force) {
            if (retries == 1) {
                action = 2; // SIGKILL
            } else if (retries == 2) {
                action = 1; // SIGHUP
            }
        }

        SLOGW("Failed to unmount %s (%s, retries %d, action %d)",
                path, strerror(errno), retries, action);

        Process::killProcessesWithOpenFiles(path, action);
        usleep(1000*1000);
    }
    errno = EBUSY;
    SLOGE("Giving up on unmount %s (%s)", path, strerror(errno));
    return -1;
}

int Volume::unmountVol(bool force, bool revert) {
    dev_t deviceNodes[16];
    int n,i, rc,k;
    char mpoint[256];
    bool primaryStorage = isPrimaryStorage();

    int flags = getFlags();
    bool providesAsec = (flags & VOL_PROVIDES_ASEC) != 0;

    if (getState() != Volume::State_Mounted) {
        SLOGE("Volume %s unmount request when not mounted", getLabel());
        errno = EINVAL;
        return UNMOUNT_NOT_MOUNTED_ERR;
    }

    setState(Volume::State_Unmounting);
    usleep(1000 * 1000); // Give the framework some time to react

    //if (mDiskNumParts == 0)
        //mDiskNumParts++;
    n = getDeviceNodes((dev_t *) &deviceNodes, 16);
    if (!n) {
        SLOGE("Failed to get device nodes (%s)\n", strerror(errno));
        return -1;
    }
    SLOGI("unmountVol parts(%d)", n);
    if ( n ==  1) {
        char service[64];
        snprintf(service, 64, "fuse_%s", getLabel());
        property_set("ctl.stop", service);
        /* Give it a chance to stop.  I wish we had a synchronous way to determine this... */
        sleep(1);
    }

    // TODO: determine failure mode if FUSE times out

    if (providesAsec && doUnmount(Volume::SEC_ASECDIR_EXT, force) != 0) {
        SLOGE("Failed to unmount secure area on %s (%s)", getMountpoint(), strerror(errno));
        goto out_mounted;
    }

    /* Now that the fuse daemon is dead, unmount it */
    if (n == 1 && doUnmount(getFuseMountpoint(), force) != 0) {
        SLOGE("Failed to unmount %s (%s)", getFuseMountpoint(), strerror(errno));
        goto fail_remount_secure;
    }

    for (i = 0; i < n; i++) {
        memset(mpoint, 0, sizeof(mpoint));
        if (n > 1) {
            snprintf(mpoint, sizeof(mpoint), "%s/partition%d", getFuseMountpoint(), i);
        } else {
            snprintf(mpoint, sizeof(mpoint), "%s", getMountpoint());
        }
        /* Unmount the real sd card */
        if (doUnmount(mpoint, force) != 0) {
            SLOGE("Failed to unmount %s (%s)", mpoint, strerror(errno));
            goto fail_remount_secure;

        }
	 if(!((n ==1)&&(mPartMajors[i] ==179)))//some sd-ext have 1 partition
        	mPartMinors[i] = -1;

        if (n > 1) {
            mount("rootfs", "/", NULL, MS_REMOUNT, NULL);
            rmdir(mpoint);
            mount("rootfs", "/", NULL, MS_REMOUNT | MS_RDONLY, NULL);
        }
    }
    if((n > 1)&&(!primaryStorage)){ // multi-parts
        if (doUnmount(getFuseMountpoint(), force)) {
            SLOGE("Failed to unmount %s (%s)", getFuseMountpoint(), strerror(errno));
            goto fail_remount_secure;
        }
    }

    SLOGI("%s unmounted successfully", getMountpoint());

    /* If this is an encrypted volume, and we've been asked to undo
     * the crypto mapping, then revert the dm-crypt mapping, and revert
     * the device info to the original values.
     */
    if (revert && isDecrypted()) {
        cryptfs_revert_volume(getLabel());
        revertDeviceInfo();
        SLOGI("Encrypted volume %s reverted successfully", getMountpoint());
    }

    setUuid(NULL);
    setUserLabel(NULL);
    setState(Volume::State_Idle);
    mCurrentlyMountedKdev = -1;
    return 0;

fail_remount_secure:
    if (providesAsec && mountAsecExternal() != 0) {
        SLOGE("Failed to remount secure area (%s)", strerror(errno));
        goto out_nomedia;
    }

out_mounted:
    setState(Volume::State_Mounted);
    return -1;

out_nomedia:
    setState(Volume::State_NoMedia);
    return -1;
}
/**
* 
******************************************************
* ActionsCode(author:huoysh, change_code) 
*
*/
int Volume::initializeMbr(const char *deviceNode) {
    struct disk_info dinfo;

    memset(&dinfo, 0, sizeof(dinfo));

    if (!(dinfo.part_lst = (struct part_info *) malloc(MAX_NUM_PARTS * sizeof(struct part_info)))) {
        SLOGE("Failed to malloc prt_lst");
        return -1;
    }

    memset(dinfo.part_lst, 0, MAX_NUM_PARTS * sizeof(struct part_info));
    dinfo.device = strdup(deviceNode);
    dinfo.scheme = PART_SCHEME_MBR;
    dinfo.sect_size = 512;
    dinfo.skip_lba = 2048;
    dinfo.num_lba = 0;
	//ActionsCode(huoysh, sync from gs702c_4.4.4) 
//    dinfo.num_parts = 1;
	dinfo.num_parts = 0;

    struct part_info *pinfo = &dinfo.part_lst[0];

    pinfo->name = strdup("android_sdcard");
    pinfo->flags |= PART_ACTIVE_FLAG;
    pinfo->type = PC_PART_TYPE_FAT32;
    pinfo->len_kb = -1;

    int rc = apply_disk_config(&dinfo, 0);

    if (rc) {
        SLOGE("Failed to apply disk configuration (%d)", rc);
        goto out;
    }

 out:
    free(pinfo->name);
    free(dinfo.device);
    free(dinfo.part_lst);

    return rc;
}

/*
 * Use blkid to extract UUID and label from device, since it handles many
 * obscure edge cases around partition types and formats. Always broadcasts
 * updated metadata values.
 */
int Volume::extractMetadata(const char* devicePath) {
    int res = 0;

    std::string cmd;
    cmd = BLKID_PATH;
    cmd += " -c /dev/null ";
    cmd += devicePath;

    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) {
        ALOGE("Failed to run %s: %s", cmd.c_str(), strerror(errno));
        res = -1;
        goto done;
    }

    char line[1024];
    char value[128];
    if (fgets(line, sizeof(line), fp) != NULL) {
        ALOGD("blkid identified as %s", line);

        char* start = strstr(line, "UUID=");
        if (start != NULL && sscanf(start + 5, "\"%127[^\"]\"", value) == 1) {
            setUuid(value);
        } else {
            setUuid(NULL);
        }

        start = strstr(line, "LABEL=");
        if (start != NULL && sscanf(start + 6, "\"%127[^\"]\"", value) == 1) {
            setUserLabel(value);
        } else {
            setUserLabel(NULL);
        }
    } else {
        ALOGW("blkid failed to identify %s", devicePath);
        res = -1;
    }

    pclose(fp);

done:
    if (res == -1) {
        setUuid(NULL);
        setUserLabel(NULL);
    }
    return res;
}
