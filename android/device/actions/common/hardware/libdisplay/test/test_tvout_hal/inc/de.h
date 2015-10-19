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

#ifndef __DE_H__
#define __DE_H__
#include <hardware/hardware.h>

#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#define DE_HARDWARE_DEVICE   "/dev/de"


struct displayer_config {
    int displayer_id;

};
#define    HAL_DE_LCD_DISPLAYER		       	 0x1
#define	HAL_DE_HDMI_DISPLAYER		 0x2
#define	HAL_DE_CVBS_DISPLAYER                 0x4

//keep sync with device/actions/common/hardware/libdisplay/de_drv.h
#define HAL_HDMI_CABLE_STATUS_BIT     2
#define HAL_CVBS_CABLE_STATUS_BIT       0


/*tvout ui display related*/
#define HAL_DEFAULT_TVOUT_VIRTUAL_WIDTH     960
#define HAL_DEFAULT_TVOUT_VIRTUAL_HEIGHT    540
#define HAL_DEFAULT_TVOUT_TV_WIDTH                1920
#define HAL_DEFAULT_TVOUT_TV_HEIGHT               1080
#define HAL_TVOUT_TOP_GAP_DEFAULT                   73
#define HAL_TVOUT_BOTTOM_GAP_DEFAULT                         73


#define DEIO_SET_SCALE_RATE_FULL_SCREEN_MIN_X  0
#define DEIO_SET_SCALE_RATE_FULL_SCREEN_MIN_Y  0
#define DEIO_SET_SCALE_RATE_FULL_SCREEN_MAX_X  50
#define DEIO_SET_SCALE_RATE_FULL_SCREEN_MAX_Y  50
/*****************************************************************************/

struct de_module_t {
   struct hw_module_t common;
};

struct de_control_device_t {
	struct hw_device_t common;
	/* supporting control APIs go here */
	int (*set_displayer)(struct de_control_device_t *dev, displayer_config *cfg);
	int (*get_displayer)(struct de_control_device_t *dev, int  *displayer);
	int (*de_set_content_output_tv)(struct de_control_device_t *dev, int set);
	int (*de_set_tv_display_scale)(struct de_control_device_t *dev, int  xscale, int yscale);
	int (*de_get_tv_display_scale)(struct de_control_device_t *dev, int  *xscale, int *yscale);
	int (*de_get_tv_cable_status)(struct de_control_device_t *dev, int *status);
	void (*de_set_display_mode_single)(struct de_control_device_t *dev, int mode);
	void(*de_set_display_mode)(struct de_control_device_t *dev, int mode);
};

/*****************************************************************************/


#define DE_HARDWARE_MODULE_ID "displayengine"

#endif

