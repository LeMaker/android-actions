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

#include <hardware/hardware.h>

#include <fcntl.h>
#include <errno.h>

#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cutils/ashmem.h>
#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include <linux/fb.h>

#include "display.h"


#define OWL_SETTING		"/data/setting"
#define OWL_SETTING_HDMI_ENABLE	"/data/setting/setting_hdmi_enable"
#define OWL_SETTING_HDMI_VID	"/data/setting/setting_hdmi_vid"
#define OWL_SETTING_HDMI_SIZE	"/data/setting/setting_hdmi_size"

#define OWL_HDMI_ENABLE		"/sys/devices/e0250000.hdmi/enable"
#define OWL_HDMI_PLUG		"/sys/devices/e0250000.hdmi/plug"
#define OWL_HDMI_VID		"/sys/devices/e0250000.hdmi/vid"
#define OWL_HDMI_AVAIL_VIDS	"/sys/devices/e0250000.hdmi/avail_vids"
#define OWL_HDMI_SIZE		"/sys/devices/e0250000.hdmi/scale_factor"

struct disp_manager_context_t {
	
	struct owldisp_device_t device;
	
	int mDispFd;
	
	int mDispNum;
		
	struct owlfb_disp_device * mDisplays[MAX_DISPLAY_NUMBER];	
};

/*
 * in order to merge to GS705A, I have to
 * switch the vid between  tv_mode(used by GS705A) 
 * and vid(used by GS900A).
 */
static int tv_mode_to_vid(int tv_mode)
{
	switch (tv_mode) {
		case OWL_TV_MOD_720P_50HZ: return 19;
		case OWL_TV_MOD_720P_60HZ: return 4;
		case OWL_TV_MOD_1080P_50HZ: return 31;
		case OWL_TV_MOD_1080P_60HZ: return 16;
		case OWL_TV_MOD_576P: return 17;
		case OWL_TV_MOD_480P: return 2;
		case OWL_TV_MOD_720P_60HZ_DVI: return 126;
		case OWL_TV_MOD_3840_2160P_30HZ: return 95;
		case OWL_TV_MOD_4096_2160P_30HZ: return 100;
		case OWL_TV_MOD_3840_1080P_60HZ: return 127;
		default: return 0;
	}
}

static int vid_to_tv_mode(int vid)
{
	switch (vid) {
		case 19: return OWL_TV_MOD_720P_50HZ;
		case 4: return OWL_TV_MOD_720P_60HZ;
		case 31: return OWL_TV_MOD_1080P_50HZ;
		case 16: return OWL_TV_MOD_1080P_60HZ;
		case 17: return OWL_TV_MOD_576P;
		case 2: return OWL_TV_MOD_480P;
		case 126: return OWL_TV_MOD_720P_60HZ_DVI;
		case 95: return OWL_TV_MOD_3840_2160P_30HZ;
		case 100: return OWL_TV_MOD_4096_2160P_30HZ;
		case 127: return OWL_TV_MOD_3840_1080P_60HZ;
		default: return 0;
	}
}


/**
 * Common hardware methods
 */
static int open_display_manager(const struct hw_module_t* module,
		const char* name, struct hw_device_t** device);

static struct hw_module_methods_t owldisp_module_methods = {
	open: open_display_manager,
};

/* The hal module for de */
/*****************************************************************************/

struct owldisp_module_t HAL_MODULE_INFO_SYM =
{
	common:
	{
		tag: HARDWARE_MODULE_TAG,
		version_major: 1,
		version_minor: 0, 
		id: DM_HARDWARE_MODULE_ID,
		name: "actions display manager",
		author: "ywwang",
		methods: &owldisp_module_methods,
		dso:0,
		reserved:
		{	0,}
	},
};

static int write_file_int(int fd, int value)
{
	char buf[256] = {0};

	sprintf(buf, "%d\n", value);
	return write(fd, buf, strlen(buf));
}

static int read_file_int(int fd, int *value)
{
	char buf[256] = {0};

	read(fd, buf, 256);
	return sscanf(buf, "%d\n", value);
}

/**/
static int owldisp_get_disp_info(struct owldisp_device_t *dev,int disp,int * info){
	struct disp_manager_context_t* ctx = (struct disp_manager_context_t*) dev;
	int rc = 0;
	int i = 0;
	int offset = 1;
	ALOGD("owldisp_get_disp_info called ctx->mDispNum %d disp %d",ctx->mDispNum,disp);
	for(i = 0 ; i < ctx->mDispNum; i++){
		struct owlfb_disp_device * newInfo = ctx->mDisplays[i];
		if(newInfo != NULL && newInfo->mType == disp){
			rc = ioctl(ctx->mDispFd, OWLFB_GET_DISPLAY_INFO, newInfo);
			if(rc < 0){
				ALOGE("owldisp_get_disp_info error rc %d",rc);
				return rc ;
			}
			//info[offset++] = newInfo->mType; 
			info[offset++] = newInfo->mState; 		
			info[offset++] = newInfo->mPluginState; 
			info[offset++] = newInfo->mWidth; 		
			info[offset++] = newInfo->mHeight; 
			info[offset++] = newInfo->mRefreshRate; 		
			info[offset++] = newInfo->mWidthScale; 
			info[offset++] = newInfo->mHeightScale;
			info[offset++] = newInfo->mCmdMode;
			info[offset++] = newInfo->mIcType;
						   
//			ALOGE("owldisp_get_disp_info ~~~ mType %d mState %d mPluginState %d mWidth %d mHeight %d ",newInfo->mType,newInfo->mState,newInfo->mPluginState,newInfo->mWidth,newInfo->mHeight); 		
			ALOGE("owldisp_get_disp_info ~~~ PD_LCD_TYPE_OFF %d \n PD_LCD_LIGHTENESS_OFF %d\n PD_LCD_SATURATION_OFF %d\n PD_LCD_CONSTRAST_OFF %d\n \
					PD_LCD_DITHER_OFF %d\n PD_LCD_GAMMA_OFF %d\n PD_LCD_GAMMA_RX %d\n PD_LCD_GAMMA_GX %d\n PD_LCD_GAMMA_BX %d\n PD_LCD_GAMMA_RY %d\n PD_LCD_GAMMA_GY %d\n PD_LCD_GAMMA_BY %d\n",newInfo->mPrivateInfo[0],newInfo->mPrivateInfo[1],\
					newInfo->mPrivateInfo[2],newInfo->mPrivateInfo[3],newInfo->mPrivateInfo[4],newInfo->mPrivateInfo[5],newInfo->mPrivateInfo[6],newInfo->mPrivateInfo[7],\
					newInfo->mPrivateInfo[8],newInfo->mPrivateInfo[9],newInfo->mPrivateInfo[10],newInfo->mPrivateInfo[11]); 		
			memcpy((void *)&info[offset],(void *)&newInfo->mPrivateInfo[0],MAX_PRIVATE_DATA_SIZE * 4);

		}		
	}
	return rc;	
}



static int owldisp_set_disp_info(struct owldisp_device_t *dev , int disp,int * info){
	struct disp_manager_context_t* ctx = (struct disp_manager_context_t*) dev;
	int rc = 0;
	int i = 0;
	int offset = 1;
	ALOGD("owldisp_set_disp_info called disp %d",disp);
	for(i = 0 ; i < ctx->mDispNum; i++){
		struct owlfb_disp_device *newInfo = ctx->mDisplays[i];
		if(newInfo != NULL && newInfo->mType == disp){
			newInfo->mState = info[offset++]; 		
			newInfo->mPluginState = info[offset++]; 
			newInfo->mWidth =info[offset++]; 		
			newInfo->mHeight = info[offset++]; 
			newInfo->mRefreshRate = info[offset++]; 		
			newInfo->mWidthScale = info[offset++]; 
			newInfo->mHeightScale = info[offset++]; 
			newInfo->mCmdMode = info[offset++];
			newInfo->mIcType = info[offset++];
			memcpy((void *)&newInfo->mPrivateInfo[0],(void *)&info[offset],MAX_PRIVATE_DATA_SIZE*4);			
			rc = ioctl(ctx->mDispFd, OWLFB_SET_DISPLAY_INFO, newInfo);
			if(rc < 0){
				return rc ;
			}
		}
		
	}
	return rc;
}
static int init_display_device(struct disp_manager_context_t *dev){
	int i = 0;
	int rc = 0;
	struct disp_manager_context_t* ctx = (struct disp_manager_context_t*) dev;
	struct owlfb_disp_device info;
	ctx->mDispNum = 0;
	for(i = 0 ; i < MAX_DISPLAY_NUMBER; i++){
		info.mType = (1 << i);
		
		rc = ioctl(ctx->mDispFd, OWLFB_GET_DISPLAY_INFO, &info);
		
		if (rc < 0) {
		    ALOGW("failed to get mdisplays info rc :%d\n",rc);
		    continue;
		}
						
		struct owlfb_disp_device * newInfo = static_cast<struct owlfb_disp_device *>(malloc(sizeof(struct owlfb_disp_device)));
		
		newInfo->mType = info.mType; 
		newInfo->mState = info.mState; 		
		newInfo->mPluginState = info.mPluginState; 
		newInfo->mWidth = info.mWidth; 		
		newInfo->mHeight = info.mHeight; 
		newInfo->mRefreshRate = info.mRefreshRate; 		
		newInfo->mWidthScale = info.mWidthScale; 
		newInfo->mHeightScale = info.mHeightScale; 		
		memcpy(&info.mPrivateInfo[0],&newInfo->mPrivateInfo[0],MAX_PRIVATE_DATA_SIZE);
		ctx->mDisplays[ctx->mDispNum++] = newInfo;
		ALOGE("init_display_device i %d",i);
	}
	return 0;
}

static int owldisp_set_hdmi_enable(struct owldisp_device_t *dev, bool enable)
{
	ALOGD("owldisp_set_hdmi_enable ~~~ enable %d", enable);

	int fd1 = open(OWL_SETTING_HDMI_ENABLE, O_WRONLY);	
	int fd2 = open(OWL_HDMI_ENABLE, O_WRONLY);	
	if (fd1 < 0 || fd2 < 0){
		ALOGE("open file(%s or %s) error",
			OWL_SETTING_HDMI_ENABLE, OWL_HDMI_ENABLE);
		return -1;
	}
	
	write_file_int(fd1, enable ? 1 : 0);
	write_file_int(fd2, enable ? 1 : 0);
	
	close(fd1);
	close(fd2);

	return 0;
}

static int owldisp_get_hdmi_enable(struct owldisp_device_t *dev)
{
	int enable = 0;

	ALOGD("owldisp_get_hdmi_enable ~~~");

	int fd = open(OWL_SETTING_HDMI_ENABLE, O_RDONLY);	
	if (fd < 0) {
		ALOGE("open file(%s) error", OWL_SETTING_HDMI_ENABLE);
		return 0;
	}
	read_file_int(fd, &enable);
	close(fd);
	
	ALOGD("owldisp_get_hdmi_enable ~~~%d", enable);

	return enable;
}

   
static int owldisp_set_hdmi_vid(struct owldisp_device_t *dev, int vid)
{
	int fd1, fd2;

	vid = tv_mode_to_vid(vid);

	ALOGD("owldisp_set_hdmi_vid ~~~ vid %d", vid);

	if (vid <= 0) {
		ALOGD("err: owldisp_set_hdmi_vid ~~~ vid %d", vid);
		return -1;
	}

	fd1 = open(OWL_SETTING_HDMI_VID, O_WRONLY);	
	fd2 = open(OWL_HDMI_VID, O_WRONLY);	
	if (fd1 < 0 || fd2 < 0){
		ALOGE("open file(%s or %s) error",
			OWL_SETTING_HDMI_VID, OWL_HDMI_VID);
		return -1;
	}
	
	write_file_int(fd1, vid);
	write_file_int(fd2, vid);
	
	close(fd1);
	close(fd2);
	return 0;
}

static int owldisp_get_hdmi_vid(struct owldisp_device_t *dev)
{
	int vid_file = 0;	
	int vid_driver = 0;	
	int fd1;
	int fd2;

	ALOGD("owldisp_get_hdmi_vid ~~~");

	fd1 = open(OWL_SETTING_HDMI_VID, O_RDWR);	
	fd2 = open(OWL_HDMI_VID, O_RDONLY);	
	if (fd1 < 0 || fd2 < 0){
		ALOGE("open file(%s or %s) error",
			OWL_SETTING_HDMI_VID, OWL_HDMI_VID);
		return -1;
	}

	read_file_int(fd1, &vid_file);
	read_file_int(fd2, &vid_driver);

	ALOGD("owldisp_get_hdmi_vid ~~~%d/%d", vid_file, vid_driver);

	if (vid_file != vid_driver && vid_driver > 0) {
		vid_file = vid_driver;
		write_file_int(fd1, vid_file);
	}

	close(fd1);
	close(fd2);

	return vid_to_tv_mode(vid_file);
}

static int owldisp_get_hdmi_supported_vid_list(struct owldisp_device_t *dev, int *vidlist)
{
	struct disp_manager_context_t* ctx = (struct disp_manager_context_t*) dev;
	int offset = 0, cnt = 0;
	char buf[256] = {0}, *p;	
	int fd;	
	int i;

	ALOGD("owldisp_get_hdmi_supported_vid_list");

	if ((fd = open(OWL_HDMI_AVAIL_VIDS, O_RDONLY)) < 0){
		ALOGE("open file(%s) error", OWL_HDMI_AVAIL_VIDS);
		return 0;
	}
	read(fd, buf, 256);
	close(fd);
	
	p = buf;
	while (sscanf(p, "%d %n", &vidlist[cnt], &offset) == 1) {
		ALOGD("%s: %d", __func__, vidlist[cnt]);
		p += offset;
		cnt++;
	}

	for (i = 0; i < cnt; i++)
		vidlist[i] = vid_to_tv_mode(vidlist[i]);

	return cnt;
} 

static int owldisp_get_hdmi_cable_state(struct owldisp_device_t *dev)
{
	struct disp_manager_context_t* ctx = (struct disp_manager_context_t*)dev;
	int state = 0;
	int fd;

	ALOGD("owldisp_get_hdmi_cable_state");

	if ((fd = open(OWL_HDMI_PLUG, O_RDONLY)) < 0) {
		ALOGE("open file(%s) error", OWL_HDMI_PLUG);
		return 0;
	}
	read_file_int(fd, &state);
	close(fd);
	
	ALOGD("%s ~~~%d", __func__, state);
	return state;
}

static int owldisp_set_hdmi_fitscreen (struct owldisp_device_t *dev,int value)
{
	ALOGD("owldisp_set_hdmi_fitscreen value %d ",value);
	return 0;
}

static int owldisp_get_hdmi_fitscreen (struct owldisp_device_t *dev)
{
	ALOGD("owldisp_get_hdmi_fitscreen ");
	return 0;
}

static int owldisp_set_hdmi_size(struct owldisp_device_t *dev, int xres, int yres)
{
	char buf[256] = {0};
	int fd1, fd2;	

	ALOGD("owldisp_set_hdmi_size ~~~ xres %d, yres %d", xres, yres);

	fd1 = open(OWL_SETTING_HDMI_SIZE, O_WRONLY);	
	fd2 = open(OWL_HDMI_SIZE, O_WRONLY);	
	if (fd1 < 0 || fd2 < 0) {
		ALOGE("open file(%s or %s) error",
			OWL_SETTING_HDMI_SIZE, OWL_HDMI_SIZE);
		return -1;
	}
	
	sprintf(buf, "%d %d\n", xres, yres);
	write(fd1, buf, strlen(buf));

	sprintf(buf, "%d %d\n", 50 + xres, 50 + yres);
	write(fd2, buf, strlen(buf));
	
	close(fd1);
	close(fd2);
	
	return 0;
}

static int owldisp_get_hdmi_size(struct owldisp_device_t *dev, int* xres_yres)
{
	char buf[256] = {0};
	int fd = open(OWL_SETTING_HDMI_SIZE, O_RDONLY);	
	if (fd < 0) {
		ALOGE("open file(%s) error", OWL_SETTING_HDMI_SIZE);
		return -1;
	}

	read(fd, buf, 256);
	close(fd);	
	
	sscanf((char*)buf, "%d %d\n", &xres_yres[0], &xres_yres[1]);
	
	ALOGD("owldisp_get_hdmi_size ~~~ xres %d ,yres %d",xres_yres[0],xres_yres[1]);
	return 0;
}

static void owldisp_init_check_and_set(struct owldisp_device_t *dev)
{
	int vid = owldisp_get_hdmi_vid(dev);
	int enable = owldisp_get_hdmi_enable(dev);
	int xres_yres[2];

	owldisp_get_hdmi_size(dev, xres_yres);

	/* 
	 * set default vid, maybe invalid, in this case,
	 * driver will choose a default one
	 */
	owldisp_set_hdmi_vid(dev, vid);

	owldisp_set_hdmi_size(dev, xres_yres[0], xres_yres[1]);

	if (enable == 1) {
		/* HW always disabled, enable it if need */
		owldisp_set_hdmi_enable(dev, 1);
	}
}
 
/* Close mdisplays manager device */
static int close_display_manager(struct hw_device_t *dev) {	
	struct disp_manager_context_t* ctx = (struct disp_manager_context_t*) dev;
	if (ctx) {
		close(ctx->mDispFd);
		free(ctx);
	}
	return 0;
}

/* Open mdisplays manager device */

static int open_display_manager(const struct hw_module_t* module,
		const char* name, struct hw_device_t** device) {
			
	int status = -EINVAL;
	struct disp_manager_context_t *ctx = 
		static_cast<struct disp_manager_context_t *>(malloc(sizeof(struct disp_manager_context_t)));
		
	memset(ctx, 0, sizeof(*ctx));
	
	ALOGD("enter open_display_manager\n");
	
	ctx->device.common.tag = HARDWARE_DEVICE_TAG;
	ctx->device.common.version = 0;
	ctx->device.common.module = const_cast<struct hw_module_t *>(module);
	ctx->device.common.close = close_display_manager;
	//ctx->device.get_disp_num = get_disp_num;
	ctx->device.get_disp_info = owldisp_get_disp_info;
	ctx->device.set_disp_info = owldisp_set_disp_info;
	
 	ctx->device.set_hdmi_enable = owldisp_set_hdmi_enable;    
	ctx->device.get_hdmi_enable = owldisp_get_hdmi_enable;  
	ctx->device.set_hdmi_vid = owldisp_set_hdmi_vid;  
	ctx->device.get_hdmi_vid =  owldisp_get_hdmi_vid; 
	ctx->device.set_hdmi_size = owldisp_set_hdmi_size;
	ctx->device.get_hdmi_size = owldisp_get_hdmi_size;
	ctx->device.get_hdmi_supported_vid_list = owldisp_get_hdmi_supported_vid_list;
	ctx->device.get_hdmi_cable_state = owldisp_get_hdmi_cable_state;
	ctx->device.set_hdmi_fitscreen = owldisp_set_hdmi_fitscreen;
	ctx->device.get_hdmi_fitscreen = owldisp_get_hdmi_fitscreen;

	mkdir(OWL_SETTING, S_IRWXU | S_IRWXG);
	if ((access(OWL_SETTING_HDMI_ENABLE, F_OK)) < 0) {
		int fd = open(OWL_SETTING_HDMI_ENABLE, O_RDWR | O_CREAT,
					S_IRWXU | S_IRWXG);
		if (fd < 0) {
			ALOGE("create %s failed\n", OWL_SETTING_HDMI_ENABLE);
			return fd;
		}
		write_file_int(fd, 1);	/* set default value */
	}
	if ((access(OWL_SETTING_HDMI_VID, F_OK)) < 0) {
		int fd = open(OWL_SETTING_HDMI_VID, O_RDWR | O_CREAT,
					S_IRWXU | S_IRWXG);
		if (fd < 0) {
			ALOGE("create %s failed\n", OWL_SETTING_HDMI_VID);
			return fd;
		}
		write_file_int(fd, 19);	/* set default value */
	}
	if ((access(OWL_SETTING_HDMI_SIZE, F_OK)) < 0) {
		char buf[256] = {0};
		int fd = open(OWL_SETTING_HDMI_SIZE, O_RDWR | O_CREAT,
					S_IRWXU | S_IRWXG);
		if (fd < 0) {
			ALOGE("create %s failed\n", OWL_SETTING_HDMI_SIZE);
			return fd;
		}

		/* set default value */
		sprintf(buf, "%d %d\n", 50, 50);
		write(fd, buf, strlen(buf));
	}

	owldisp_init_check_and_set(&ctx->device);

#if 0
	ctx->mDispFd = open(DM_HARDWARE_DEVICE, O_RDWR, 0);

	init_display_device(ctx);

	if (ctx->mDispFd >= 0) {
		ALOGI("open de successfully! fd: %d", ctx->mDispFd);
		*device = &ctx->device.common;
		status = 0;
	} else {
		status = errno;
		close_display_manager(&ctx->device.common);
		ALOGE("ctx->mDispFd:%d,Error open mdisplays manager failed: %d %s", ctx->mDispFd,status, strerror(status));
		status = -status;
	}
#else
	ALOGI("open de successfully! fd: %d", ctx->mDispFd);
	*device = &ctx->device.common;
	status = 0;
#endif
	return status;
}

