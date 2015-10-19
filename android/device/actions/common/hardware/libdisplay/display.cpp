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

struct disp_manager_context_t {
	
	struct owldisp_device_t device;
	
	int mDispFd;
	
	int mDispNum;
		
	struct owlfb_disp_device * mDisplays[MAX_DISPLAY_NUMBER];	
};

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
static int owldisp_set_hdmi_enable (struct owldisp_device_t *dev,bool enable)
{
	char buf[256]={0};
	ALOGD("owldisp_set_hdmi_enable ~~~ enable %d",enable);
	int fd = open("/data/setting/setting_hdmi_enable", O_WRONLY);	
	if (fd < 0){
		ALOGE("open file(%s) error", "/data/setting/setting_hdmi_enable");
		return -1;
	}
	
	sprintf(buf, "%d ",enable?1:0);
	
	write(fd, buf, strlen(buf));
	
	close(fd);

	return 0;
}

static int owldisp_get_hdmi_enable (struct owldisp_device_t *dev)
{
	char buf[256]={0};
	int enable = 0;
	ALOGD("owldisp_get_hdmi_enable ~~~");
	int fd = open("/data/setting/setting_hdmi_enable", O_RDONLY);	
	if (fd < 0){
		ALOGE("open file(%s) error", "/data/setting/setting_hdmi_enable");
		return 0;
	}
	
	read(fd, buf, sizeof(int));
	
	close(fd);
	
	sscanf(buf, "%d",&enable);
	
	ALOGD("owldisp_get_hdmi_enable ~~~%d", enable);
	
	return enable;
}
    
static int owldisp_set_hdmi_vid(struct owldisp_device_t *dev, int vid)
{
	char buf[256]={0};
	ALOGD("owldisp_set_hdmi_vid ~~~ vid %d",vid);
	if(vid == -1){
		ALOGD("err : owldisp_set_hdmi_vid ~~~ vid %d",vid);
		return -1;
	}
	int fd = open("/data/setting/setting_hdmi_mode", O_WRONLY);	
	if (fd < 0){
		ALOGE("open file(%s) error", "/data/setting/setting_hdmi_mode");
		return -1;
	}
	
	sprintf(buf, "%d ",vid);
	
	write(fd, buf, strlen(buf));
	
	close(fd);

	return 0;
}
static int owldisp_get_hdmi_vid(struct owldisp_device_t *dev)
{
	struct disp_manager_context_t* ctx = (struct disp_manager_context_t*) dev;
	char buf[256]={0};	
	int vid = -1;	
	int i;
	int rc;
	int fd = open("/data/setting/setting_hdmi_mode", O_RDONLY);	
	if (fd < 0)
	{
		ALOGE("open file(%s) error", "/data/setting/setting_hdmi_mode");
		return -1;
	}
	
	read(fd, buf, sizeof(int));
	
	close(fd);	
		
	sscanf(buf, "%d",&vid);
	
	rc = ioctl(ctx->mDispFd, OWLFB_HDMI_GET_VID, &i);
	if(rc < 0){
		ALOGE("owldisp_get_hdmi_vid error rc %d",rc);
		return vid;
	}

	if(vid != i){
		ALOGE("owldisp_get_hdmi_vid vid is not current vid %d current vid %d", vid, i);
		vid = i;
	}
	ALOGD("owldisp_get_hdmi_vid ~~~ vid %d",vid);
	return vid;
}

static int owldisp_get_hdmi_supported_vid_list (struct owldisp_device_t *dev,int * vidlist)
{
	struct disp_manager_context_t* ctx = (struct disp_manager_context_t*) dev;
	ALOGD("owldisp_get_hdmi_supported_vid_list");
	int j = 0;
	int i = 0;
	for(i = 1 ; i < OWL_TV_MODE_NUM ; i++){
	  	if (ioctl(ctx->mDispFd, OWLFB_HDMI_SUPPORT_MODE, &i)){		    
			vidlist[j++] = i;
		}
	}	
	return j;
} 

static int owldisp_get_hdmi_cable_state (struct owldisp_device_t *dev)
{
	struct disp_manager_context_t* ctx = (struct disp_manager_context_t*) dev;
	ALOGD("owldisp_get_hdmi_cable_state");
	int i = 0;
	int rc = 0;
	rc = ioctl(ctx->mDispFd, OWLFB_HDMI_GET_CABLE_STATUS, &i);
	if(rc < 0){
		ALOGE("owldisp_get_hdmi_cable_state error rc %d",rc);
		return 0;
	}
	return i;
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

static int owldisp_set_hdmi_size(struct owldisp_device_t *dev, int xres , int yres)
{
	char buf[256]={0};
	ALOGD("owldisp_set_hdmi_size ~~~ xres %d ,yres %d",xres,yres);
	int fd = open("/data/setting/setting_hdmi_size", O_WRONLY);	
	if (fd < 0)
	{
		ALOGE("open file(%s) error", "/data/setting/setting_hdmi_size");
		return -1;
	}
	
	sprintf(buf, "%d %d ", xres, yres);
	
	write(fd, buf, strlen(buf));
	
	close(fd);
	
	return 0;
}

static int owldisp_get_hdmi_size(struct owldisp_device_t *dev, int* xres_yres)
{
	char buf[256]={0};
	int fd = open("/data/setting/setting_hdmi_size", O_RDONLY);	
	if (fd < 0)
	{
		ALOGE("open file(%s) error", "/data/setting/setting_hdmi_size");
		return -1;
	}
		
	read(fd, buf, 256);
	
	close(fd);	
	
	sscanf((char*)buf, "%d %d", &xres_yres[0],&xres_yres[1]);
	
	ALOGD("owldisp_get_hdmi_size ~~~ xres %d ,yres %d",xres_yres[0],xres_yres[1]);
	return 0;
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
    ctx->device.get_hdmi_supported_vid_list =owldisp_get_hdmi_supported_vid_list;
	ctx->device.get_hdmi_cable_state =owldisp_get_hdmi_cable_state;
    ctx->device.set_hdmi_fitscreen = owldisp_set_hdmi_fitscreen;
    ctx->device.get_hdmi_fitscreen = owldisp_get_hdmi_fitscreen;
    
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
	return status;
}

