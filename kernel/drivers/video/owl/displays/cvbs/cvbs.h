/*
 * hdmi.h
 *
 * HDMI header definition for OWL IP.
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: Guo Long  <guolong@actions-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _OWL_CVBS_H
#define _OWL_CVBS_H
#include <video/owldss.h>

#define CVBS_PRINT
	#ifdef CVBS_PRINT
	#define DEBUG_CVBS(format, ...) \
		do { \
			printk(KERN_DEBUG "OWL_CVBS: " format, ## __VA_ARGS__); \
		} while (0)
	#else
	#define DEBUG_CVBS(format, ...)
	#endif

enum VIDEO_ID_TABLE {
	VID640x480P_60_4VS3 = 1,
	VID720x480P_60_4VS3,
	VID720x480P_60_16VS9,
	VID1280x720P_60_16VS9,
	VID1920x1080I_60_16VS9,
	VID720x480I_60_4VS3,
	VID720x480I_60_16VS9,
	VID1440x480P_60_4VS3 = 14,
	VID1440x480P_60_16VS9,
	VID1920x1080P_60_16VS9,
	VID720x576P_50_4VS3,
	VID720x576P_50_16VS9,
	VID1280x720P_50_16VS9,
	VID1920x1080I_50_16VS9,
	VID720x576I_50_4VS3,
	VID720x576I_50_16VS9,
	VID1440x576P_50_4VS3 = 29,
	VID1440x576P_50_16VS9,
	VID1920x1080P_50_16VS9,
	VID1920x1080P_24_16VS9,
	VID1920x1080P_25_16VS9,
	VID1920x1080P_30_16VS9,
	VID720x480P_59P94_4VS3 = 72,
	VID720x480P_59P94_16VS9,
	VID1280X720P_59P94_16VS9,
	VID1920x1080I_59P94_16VS9,
	VID720x480I_59P54_4VS3,
	VID720x480I_59P54_16VS9,
	VID1920x1080P_59P94_16VS9 = 86,
	VID1920x1080P_29P97_16VS9 = 104,
	VID_MAX
};

struct cvbs_info {
	struct platform_device  *pdev;
	void __iomem	*base;
	struct owl_dss_device   *dssdev;
	struct mutex lock;
	struct owl_videomode *cvbs_display_mode;
	int current_vid;
	int hot_plugin_enable;
	
	u16 overscan_width;
	u16 overscan_height;
	
	struct workqueue_struct *wq;
	bool is_init;
};
//struct cvbs_info cvbs;
extern struct cvbs_info cvbs;

enum TV_MODE_TYPE {
	TV_MODE_PAL = 0,
	TV_MODE_NTSC = 1,
	TV_MODE_MAX = 2
};

enum TV_ENCODER_TYPE {
	TV_ENCODER_INTERNAL,
	TV_ENCODER_EXTERNAL,
	TV_ENCODER_MAX
};

enum TVOUT_DEV_STATUS {
	TVOUT_DEV_PLUGOUT,
	TVOUT_DEV_PLUGIN,
	TVOUT_DEV_MAX
};
enum CVBS_PLUGGING {
	CVBS_PLUGOUT = 0,
	CVBS_PLUGIN,
	CVBS_PLUGGING_MAX
};

void enable_cvbs_output(void);
void disable_cvbs_output(void);
void dump_reg(void);
void cvbs_display_get_vid(struct owl_dss_device *dssdev, int *vid);
void cvbs_display_set_vid(struct owl_dss_device *dssdev, int vid);
int owl_cvbs_create_sysfs(struct device *dev);
int owldss_cvbs_suspend(struct owl_dss_device *dssdev);
int owldss_cvbs_resume(struct owl_dss_device *dssdev);
void cvbs_set_vid(struct owl_dss_device *dssdev,int vid);
void cvbs_display_set_overscan(struct owl_dss_device *dssdev,u16 over_scan_width,u16 over_scan_height);
void cvbs_display_get_overscan(struct owl_dss_device *dssdev, u16 * over_scan_width,u16 * over_scan_height);
void owldss_cvbs_display_disable(struct owl_dss_device *dssdev);
void owldss_cvbs_display_enable_hpd(struct owl_dss_device *dssdev, bool enable);

#endif
