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

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <hardware/hardware.h>

#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#define DM_HARDWARE_DEVICE   "/dev/graphics/fb0"
#define MAX_DISPLAY_NUMBER 4
#define MAX_PRIVATE_DATA_SIZE 40
/*****************************************************************************/

struct owldisp_module_t {
   struct hw_module_t common;
};

#define HDMI_SUPPORT_MODE_NUMER 12

struct owlfb_hdmi_mode{
	const char * mode;
	int vid;
};

typedef enum
{
	OWL_TV_MOD_720P_50HZ = 1,
	OWL_TV_MOD_720P_60HZ,
	OWL_TV_MOD_1080P_50HZ,
	OWL_TV_MOD_1080P_60HZ, 
	OWL_TV_MOD_576P,
	OWL_TV_MOD_480P,
	OWL_TV_MOD_720P_60HZ_DVI,
	OWL_TV_MOD_PAL,
	OWL_TV_MOD_NTSC,
	OWL_TV_MOD_3840_2160P_30HZ,
	OWL_TV_MOD_4096_2160P_30HZ,
	OWL_TV_MOD_3840_1080P_60HZ,
	OWL_TV_MODE_NUM = OWL_TV_MOD_NTSC, /* NOTE: NUM must equal to last one */
}__owl_tv_mode_t;


const struct owlfb_hdmi_mode  owlfb_hdmi_mode_table[HDMI_SUPPORT_MODE_NUMER] = {
	{
		.mode = "1280x720p-50HZ",
		.vid = OWL_TV_MOD_720P_50HZ,
	},
	{
		.mode = "1280x720p-60HZ",
		.vid = OWL_TV_MOD_720P_60HZ,
	},
	{
		.mode = "1920x1080p-50HZ",
		.vid = OWL_TV_MOD_1080P_50HZ,
	},
	{
		.mode = "1920x1080p-60HZ",
		.vid = OWL_TV_MOD_1080P_60HZ,
	},
	{
		.mode = "720x576p-50HZ",
		.vid = OWL_TV_MOD_576P,
	},
	{
		.mode = "720x480p-60HZ",
		.vid = OWL_TV_MOD_480P,
	},
	{
		.mode = "1280x720p-60HZ-DVI",
		.vid = OWL_TV_MOD_720P_60HZ_DVI,
	},
		{
		.mode = "PAL",
		.vid = OWL_TV_MOD_PAL,
	},
	{
		.mode = "NTSC",
		.vid = OWL_TV_MOD_NTSC,
	},
	{
		.mode = "3840x2160p-30HZ",
		.vid = OWL_TV_MOD_3840_2160P_30HZ,
	},
	{
		.mode = "4096x2160p-30HZ",
		.vid = OWL_TV_MOD_4096_2160P_30HZ,
	},
	{
		.mode = "3840x1080p-60HZ",
		.vid = OWL_TV_MOD_3840_1080P_60HZ,
	},
};

struct owlfb_disp_device{
    int mType;
    int mState;
    int mPluginState;
    int mWidth;
    int mHeight;
    int mRefreshRate;
    int mWidthScale;
    int mHeightScale;
    int mCmdMode;
    int mIcType;
    int mPrivateInfo[MAX_PRIVATE_DATA_SIZE];
};

struct owlfb_disp_gamma_info {
	char gamma_value[256];
	char enabled;
};

struct owlfb_hdmi_vid_info {
	__u8 vid;
	__u8 mode;
	__u16 reserved2;
};

#define OWL_IOW(num, dtype)	_IOW('O', num, dtype)
#define OWL_IOR(num, dtype)	_IOR('O', num, dtype)
#define OWL_IOWR(num, dtype)	_IOWR('O', num, dtype)
#define OWL_IO(num)		_IO('O', num)

#define OWLFB_SET_GAMMA_INFO	      OWL_IOW(65, struct owlfb_disp_gamma_info)
#define OWLFB_GET_GAMMA_INFO	      OWL_IOR(66, struct owlfb_disp_gamma_info)

#define OWLFB_HDMI_SUPPORT_MODE       OWL_IOR(68, struct owlfb_hdmi_vid_info)

#define OWLFB_GET_DISPLAY_INFO		  OWL_IOW(74,struct owlfb_disp_device)
#define OWLFB_SET_DISPLAY_INFO		  OWL_IOW(75,struct owlfb_disp_device)

#define OWLFB_HDMI_GET_CABLE_STATUS	  OWL_IOR(76, int)
#define OWLFB_HDMI_GET_VID			  OWL_IOR(77, int)

#define OWLFB_CVBS_GET_VID			  OWL_IOR(78, int)
#define OWLFB_CVBS_SET_VID			  OWL_IOR(79, int)
#define OWLFB_CVBS_ENABLE			    OWL_IOR(80, int)

struct owldisp_device_t {
	
	struct hw_device_t common;
	
    /*new de control infterface */
    
    int (*get_disp_num)(struct owldisp_device_t *dev);
    
    int (*get_disp_info)(struct owldisp_device_t *dev,int disp,int * info );
    
    int (*set_disp_info)(struct owldisp_device_t *dev,int disp,int * info );
    
    int (*set_hdmi_enable)(struct owldisp_device_t *dev,bool enable);
	
	int (*get_hdmi_enable)(struct owldisp_device_t *dev);
    
    int (*set_hdmi_vid)(struct owldisp_device_t *dev, int vid);
    
    int (*get_hdmi_vid)(struct owldisp_device_t *dev);
    
    int (*get_hdmi_supported_vid_list)(struct owldisp_device_t *dev,int * vidlist);
	
	int (*get_hdmi_cable_state)(struct owldisp_device_t *dev);
    
    int (*set_hdmi_size)(struct owldisp_device_t *dev, int xres , int yres);
    
    int (*get_hdmi_size)(struct owldisp_device_t *dev, int* xres_yres);
    
    int (*set_hdmi_fitscreen)(struct owldisp_device_t *dev, int value);
    
    int (*get_hdmi_fitscreen)(struct owldisp_device_t *dev);
    
    int (*set_cvbs_vid)(struct owldisp_device_t *dev, int vid);
    
    int (*get_cvbs_vid)(struct owldisp_device_t *dev);   
    
    int (*set_cvbs_enable)(struct owldisp_device_t *dev,bool enable);
	
	int (*get_cvbs_enable)(struct owldisp_device_t *dev);
    
};

/*****************************************************************************/
#define DM_HARDWARE_MODULE_ID "libdisplay"

/** helper APIs */
static inline int owldisp_manager_open(const struct hw_module_t* module,
		struct owldisp_device_t** device) {
	return module->methods->open(module, DM_HARDWARE_MODULE_ID,
			(struct hw_device_t**) device);
}

#endif

