/*
 * File: include/linux/owlfb.h
 *
 * Framebuffer driver for ACTS OWL boards
 *
 * Copyright (C) 2004 Nokia Corporation
 * Author: Imre Deak <imre.deak@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __LINUX_OWLFB_H__
#define __LINUX_OWLFB_H__

#include <linux/fb.h>
#include <linux/ioctl.h>
#include <linux/types.h>

#include "owldss.h"

/* IOCTL commands. */

#define OWL_IOW(num, dtype)	_IOW('O', num, dtype)
#define OWL_IOR(num, dtype)	_IOR('O', num, dtype)
#define OWL_IOWR(num, dtype)	_IOWR('O', num, dtype)
#define OWL_IO(num)		_IO('O', num)

#define OWLFB_MIRROR		          OWL_IOW(31, int)
#define OWLFB_SYNC_GFX		          OWL_IO(37)
#define OWLFB_VSYNC		              OWL_IO(38)

#define OWLFB_OVERLAY_REQUEST	      OWL_IOR(41, struct owlfb_overlay_args)
#define OWLFB_OVERLAY_RELEASE		  OWL_IOR(42, struct owlfb_overlay_args)
#define OWLFB_OVERLAY_ENABLE	      OWL_IOR(43, struct owlfb_overlay_args)
#define OWLFB_OVERLAY_DISABLE		  OWL_IOR(45, struct owlfb_overlay_args)
#define OWLFB_OVERLAY_GETINFO	      OWL_IOW(46, struct owlfb_overlay_args)
#define OWLFB_OVERLAY_SETINFO         OWL_IOW(47, struct owlfb_overlay_args)

#define OWLFB_SET_COLOR_KEY	          OWL_IOW(50, struct owlfb_color_key)
#define OWLFB_GET_COLOR_KEY	          OWL_IOW(51, struct owlfb_color_key)
#define OWLFB_SETUP_PLANE	          OWL_IOW(52, struct owlfb_plane_info)
#define OWLFB_QUERY_PLANE	          OWL_IOW(53, struct owlfb_plane_info)
#define OWLFB_UPDATE_WINDOW	          OWL_IOW(54, struct owlfb_update_window)
#define OWLFB_SETUP_MEM	              OWL_IOW(55, struct owlfb_mem_info)
#define OWLFB_QUERY_MEM	              OWL_IOW(56, struct owlfb_mem_info)
#define OWLFB_WAITFORVSYNC	          OWL_IOW(57,long long)
#define OWLFB_MEMORY_READ	          OWL_IOR(58, struct owlfb_memory_read)
#define OWLFB_GET_OVERLAY_COLORMODE   OWL_IOR(59, struct owlfb_ovl_colormode)
#define OWLFB_WAITFORGO	              OWL_IO(60)
#define OWLFB_SET_PAN_DISPLAY	      OWL_IOW(61, struct owlfb_disp_content_info)
#define OWLFB_SET_TEARSYNC	          OWL_IOW(62, struct owlfb_tearsync_info)

#define OWLFB_GET_MAIN_DISPLAY_RES	  OWL_IOR(63, struct owlfb_display_info)
#define OWLFB_GET_HISTOGRAM_INFO	  OWL_IOR(64, struct owlfb_disp_histogram_info)
#define OWLFB_SET_GAMMA_INFO	      OWL_IOW(65, struct owl_gamma_info)
#define OWLFB_GET_GAMMA_INFO	      OWL_IOR(66, struct owl_gamma_info)
#define OWLFB_VSYNC_EVENT_EN	      OWL_IOW(67, struct owlfb_sync_info)
#define OWLFB_HDMI_SUPPORT_MODE       OWL_IOR(68, struct owlfb_hdmi_vid_info)
#define OWLFB_HDMI_SET_MODE           OWL_IOW(69, struct owlfb_hdmi_vid_info)
#define OWLFB_HDMI_ON                 OWL_IO(70)
#define OWLFB_HDMI_OFF                OWL_IO(71)
#define OWLFB_HDMI_DISABLE            OWL_IOW(72,int)
#define OWLFB_LAYER_REQUEST           OWL_IOW(73,int)
#define OWLFB_GET_DISPLAY_INFO		  OWL_IOW(74,struct owlfb_disp_device)
#define OWLFB_SET_DISPLAY_INFO		  OWL_IOW(75,struct owlfb_disp_device)

#define OWLFB_HDMI_GET_CABLE_STATUS	  OWL_IOR(76, int)
#define OWLFB_HDMI_GET_VID			  OWL_IOR(77, int)
#define OWLFB_CVBS_GET_VID			  OWL_IOR(78, int)
#define OWLFB_CVBS_SET_VID			  OWL_IOR(79, int)
#define OWLFB_CVBS_ENABLE			    OWL_IOR(80, int)
#define OWLFB_CVBS_ON                 OWL_IO(81)
#define OWLFB_CVBS_OFF                OWL_IO(82)

#define OWLFB_CAPS_GENERIC_MASK	0x00000fff
#define OWLFB_CAPS_LCDC_MASK		0x00fff000
#define OWLFB_CAPS_PANEL_MASK		0xff000000

#define OWLFB_CAPS_MANUAL_UPDATE	0x00001000
#define OWLFB_CAPS_TEARSYNC		0x00002000
#define OWLFB_CAPS_PLANE_RELOCATE_MEM	0x00004000
#define OWLFB_CAPS_PLANE_SCALE		0x00008000
#define OWLFB_CAPS_WINDOW_PIXEL_DOUBLE	0x00010000
#define OWLFB_CAPS_WINDOW_SCALE	0x00020000
#define OWLFB_CAPS_WINDOW_OVERLAY	0x00040000
#define OWLFB_CAPS_WINDOW_ROTATE	0x00080000
#define OWLFB_CAPS_SET_BACKLIGHT	0x01000000

/* Values from DSP must map to lower 16-bits */
#define OWLFB_FORMAT_MASK		0x00ff
#define OWLFB_FORMAT_FLAG_DOUBLE	0x0100
#define OWLFB_FORMAT_FLAG_TEARSYNC	0x0200
#define OWLFB_FORMAT_FLAG_FORCE_VSYNC	0x0400
#define OWLFB_FORMAT_FLAG_ENABLE_OVERLAY	0x0800
#define OWLFB_FORMAT_FLAG_DISABLE_OVERLAY	0x1000

#define OWLFB_MEMTYPE_SDRAM		0
#define OWLFB_MEMTYPE_SRAM		1
#define OWLFB_MEMTYPE_MAX		1

#define OWLFB_MEM_IDX_ENABLED	0x80
#define OWLFB_MEM_IDX_MASK	0x7f

enum owlfb_color_format {
	OWLFB_COLOR_RGB565 = 0,
	OWLFB_COLOR_YUV420,
	OWLFB_COLOR_YVU420,
	OWLFB_COLOR_RGB444,
	OWLFB_COLOR_YUV420SP,

	OWLFB_COLOR_ARGB16,
	OWLFB_COLOR_RGB24U,	/* RGB24, 32-bit container */
	OWLFB_COLOR_RGB24P,	/* RGB24, 24-bit container */
	OWLFB_COLOR_ARGB32,
	OWLFB_COLOR_RGBA32,
	OWLFB_COLOR_RGBX32,
};


enum owlfb_overlay_type {
	OWLFB_OVERLAY_VIDEO = 1,
	OWLFB_OVERLAY_CURSOR,	
};

struct owlfb_update_window {
	__u32 x, y;
	__u32 width, height;
	__u32 format;
	__u32 out_x, out_y;
	__u32 out_width, out_height;
	__u32 reserved[8];
};

struct owlfb_update_window_old {
	__u32 x, y;
	__u32 width, height;
	__u32 format;
};

enum owlfb_plane {
	OWLFB_PLANE_VID1,
	OWLFB_PLANE_VID2,
	OWLFB_PLANE_VID3,
	OWLFB_PLANE_VID4,
};

enum owlfb_channel_out {
	OWLFB_CHANNEL_OUT_LCD = 0,
	OWLFB_CHANNEL_OUT_DIGIT,
};

struct owlfb_plane_info {
	__u32 pos_x;
	__u32 pos_y;
	__u8  enabled;
	__u8  channel_out;
	__u8  mirror;
	__u8  mem_idx;
	__u32 out_width;
	__u32 out_height;
	__u32 reserved2[12];
};

struct owlfb_overlay_info {
    __u32 mem_off;
    __u32 mem_size;
	
	__u32 screen_width;	
	enum owl_color_mode color_mode;		
	__u32 img_width;
	__u32 img_height;
		
	__u32 xoff;	
	__u32 yoff;
	__u32 width;
	__u32 height;
	
	__u8 rotation;
	
	__u32 pos_x;
	__u32 pos_y;
	__u32 out_width;	/* if 0, out_width == width */
	__u32 out_height;	/* if 0, out_height == height */
	
	__u8 lightness;
	__u8 saturation;
	__u8 contrast;
	bool global_alpha_en;
	__u8 global_alpha;
	
	bool pre_mult_alpha_en;	
	__u8 zorder;
};

struct owlfb_mem_info {
	__u32 size;
	__u8  type;
	__u8  reserved[3];
};

struct owlfb_caps {
	__u32 ctrl;
	__u32 plane_color;
	__u32 wnd_color;
};

enum owlfb_color_key_type {
	OWLFB_COLOR_KEY_DISABLED = 0,
	OWLFB_COLOR_KEY_GFX_DST,
	OWLFB_COLOR_KEY_VID_SRC,
};

struct owlfb_color_key {
	__u8  channel_out;
	__u32 background;
	__u32 trans_key;
	__u8  key_type;
};

enum owlfb_update_mode {
	OWLFB_UPDATE_DISABLED = 0,
	OWLFB_AUTO_UPDATE,
	OWLFB_MANUAL_UPDATE
};

struct owlfb_memory_read {
	__u16 x;
	__u16 y;
	__u16 w;
	__u16 h;
	size_t buffer_size;
	void __user *buffer;
};

struct owlfb_overlay_args {
	__u16 fb_id;
	__u16 overlay_id;
	__u16 overlay_type;
	__u32 overlay_mem_base;
	__u32 overlay_mem_size;
	__u32 uintptr_overly_info;
};

struct owlfb_ovl_colormode {
	__u8 overlay_idx;
	__u8 mode_idx;
	__u32 bits_per_pixel;
	__u32 nonstd;
	struct fb_bitfield red;
	struct fb_bitfield green;
	struct fb_bitfield blue;
	struct fb_bitfield transp;
};

struct owlfb_disp_content_info {
	struct owl_overlay_info overlay[4];
};

struct owlfb_disp_histogram_info {
	__u32 hist[32];
	__u32 totaly;
};

struct owlfb_sync_info {
	__u8 enabled;
	__u8 disp_id;
	__u16 reserved2;
};

struct owlfb_display_info {
	__u16 xres;
	__u16 yres;
	__u16 virtual_xres;
	__u16 virtual_yres;
	__u32 refresh_rate;
	__u32 width;	/* phys width of the display in micrometers */
	__u32 height;	/* phys height of the display in micrometers */
	__u16 disp_type;
	__u32 reserved[2];
};


struct owlfb_hdmi_vid_info {
	__u8 vid;
	__u8 mode;
	__u16 reserved2;
};
#define MAX_PRIVATE_DATA_SIZE 40
struct owlfb_disp_device{
    __u32 mType;
    __u32 mState;
    __u32 mPluginState;
    __u32 mWidth;
    __u32 mHeight;
    __u32 mRefreshRate;
    __u32 mWidthScale;
    __u32 mHeightScale;
    __u32 mCmdMode;
    __u32 mIcType;
    __u32 mPrivateInfo[MAX_PRIVATE_DATA_SIZE];
};

struct display_private_info
{
	int LCD_TYPE;
	int	LCD_LIGHTENESS;
	int LCD_SATURATION;
	int LCD_CONSTRAST;
};

enum CmdMode
{
    SET_LIGHTENESS = 0,
    SET_SATURATION = 1,
    SET_CONSTRAST = 2,
	SET_DEFAULT = 3,

};

#endif /* __OWLFB_H */
