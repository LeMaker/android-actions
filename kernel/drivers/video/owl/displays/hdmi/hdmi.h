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

#ifndef _OWL_HDMI_H
#define _OWL_HDMI_H
#include <video/owldss.h>

#define PRINGK_ON
#ifdef PRINGK_ON
#define DEBUG_ON(format, ...) \
	do { \
		printk(KERN_INFO "OWL_HDMI: " format, ## __VA_ARGS__); \
	} while (0)
#else
#define DEBUG_ON(format, ...)
#endif

#define ERR_DEBUG
#ifdef ERR_DEBUG
#define DEBUG_ERR(format, ...) \
	do { \
		printk(KERN_ERR "OWL_HDMI_ERR: " format, ## __VA_ARGS__); \
	} while (0)
#else
#define DEBUG_ERR(format, ...)
#endif

#define DEBUG
#ifdef DEBUG
#define HDMI_DEBUG(format, ...) \
	do { \
		printk(KERN_DEBUG "OWL_HDMI_DEBUG: " format, ## __VA_ARGS__); \
	} while (0)
#else
#define HDMI_DEBUG(format, ...)
#endif

#define DEBUG_HDCPa
#ifdef DEBUG_HDCP
#define HDCP_DEBUG(format, ...) \
	do { \
		printk(KERN_INFO "OWL_HDCP: " format, ## __VA_ARGS__); \
	} while (0)
#else
#define HDCP_DEBUG(format, ...)
#endif

#define DEBUG_EDIDa
#ifdef DEBUG_EDID
#define EDID_DEBUG(format, ...) \
	do { \
		printk(KERN_INFO "OWL_EDID: " format, ## __VA_ARGS__); \
	} while (0)
#else
#define EDID_DEBUG(format, ...)
#endif

#define HDCP_CASE(format, ...) \
	do { \
		printk(KERN_INFO "HDCP_CASE: " format, ## __VA_ARGS__); \
	} while (0)
	
/*add macro to diff IC*/
#define IC_TYPE_ATM7039C	 0x01  
#define IC_TYPE_ATM7059TC	 0x02
#define IC_TYPE_ATM7059A	 0x03 

struct hdmi_ip_data;

enum VIDEO_ID_TABLE {
	VID640x480P_60_4VS3 = 1,
	VID720x480P_60_4VS3,
	VID1280x720P_60_16VS9 = 4,
	VID1920x1080I_60_16VS9,
	VID720x480I_60_4VS3,
	VID1920x1080P_60_16VS9 = 16,
	VID720x576P_50_4VS3,
	VID1280x720P_50_16VS9 = 19,
	VID1920x1080I_50_16VS9,
	VID720x576I_50_4VS3,
	VID1440x576P_50_4VS3 = 29,
	VID1920x1080P_50_16VS9 = 31,
	VID1920x1080P_24_16VS9,
	VID1280x720P_60_DVI = 126,
	VID_MAX
}; 

enum hdmi_core_hdmi_dvi {
	HDMI_DVI = 0,
	HDMI_HDMI = 1
};

enum hdmi_core_irq_state {
	HDMI_HPD_IRQ = 0x1,
	HDMI_CEC_IRQ = 0x2,
};
enum hdmi_clk_refsel {
	HDMI_REFSEL_PCLK = 0,
	HDMI_REFSEL_REF1 = 1,
	HDMI_REFSEL_REF2 = 2,
	HDMI_REFSEL_SYSCLK = 3
};

enum SRC_SEL {
	VITD = 0,
	DE,
	SRC_MAX
};

enum FORMAT_3D{
	NOT_3D= 0,
	IS_3D,
	FORMAT_3D_MAX
};

enum hdmi_core_packet_mode {
	HDMI_PACKETMODERESERVEDVALUE = 0,
	HDMI_PACKETMODE24BITPERPIXEL = 4,
	HDMI_PACKETMODE30BITPERPIXEL = 5,
	HDMI_PACKETMODE36BITPERPIXEL = 6,
	HDMI_PACKETMODE48BITPERPIXEL = 7
};

enum hdmi_packing_mode {
	HDMI_PACK_10b_RGB_YUV444 = 0,
	HDMI_PACK_24b_RGB_YUV444_YUV422 = 1,
	HDMI_PACK_20b_YUV422 = 2,
	HDMI_PACK_ALREADYPACKED = 7
};

enum hdmi_pixel_foemat{
	RGB444 = 0,
	YUV444 = 2
};

enum hdmi_deep_color{
	color_mode_24bit = 0,
	color_mode_30bit = 1,
	color_mode_36bit = 2,
	color_mode_48bit = 3
};

struct hw_diff {
	int ic_type;
	int hp_start;
	int hp_end;
	int vp_start;
	int vp_end;
	int mode_start;
	int mode_end;
};

struct hdmi_hdcp {
	int hdcpOper_state;
	int hdcp_a2;
	int hdcpOper_retry;
	int repeater;
	int Ri; 
	int Ri_Read;
	int retry_times_for_set_up_5_second;

	int hdcp_check_status;
	int need_to_delay;
	int check_state;
	int hot_plug_pin_status;
	int hdcp_have_goin_authentication;
	int hdcp_have_open_authentication;
	int hdcp_have_authentication_finished;
	int hdcp_error_timer_open;

	unsigned char  Bksv[5];
	unsigned char  Bstatus[2];
	unsigned char  ksvList[128*5];
	unsigned char  Vp[20];
	unsigned char  hdcpOper_M0[8];

	int ri_counter;
	int Ri_Read_Error;
	int i_error;
	int i2c_error;
	int hdcp_fail_times;

	struct workqueue_struct *wq;
	struct delayed_work hdcp_work;
	struct delayed_work hdcp_err_work;
	
	struct delayed_work hdcp_read_key_work;
	
	bool read_hdcp_success;
	bool hdcp_authentication_success;
};

struct hdmi_video_format {
	enum hdmi_packing_mode	packing_mode;
	u32			y_res;	/* Line per panel */
	u32			x_res;	/* pixel per line */
};

struct hdmi_cm {
	int	code;
	int	mode;
	int vid;
};

struct hdmi_settings {
	int hdmi_src;
	int vitd_color;
	int enable_3d;
	int present_3d;
	int pixel_encoding;
	int color_xvycc;
	int deep_color;
	int hdmi_mode;
	int channel_invert;
	int bit_invert;
};

struct hdmi_config {
	struct owl_video_timings timings;
	struct hdmi_cm cm;
};

struct data_flag {
	bool hpd_en;
	bool cable_status;
	bool hpd_pending;
	bool hdmi_sta;
	bool hdcp_ri;
	bool cable_check_onoff;
	bool send_uevent;
};

struct drv_cfg{
	int drv_from_dts;
	
	int vid480p_tx1;
	int vid480p_tx2;
	int vid576p_tx1;
	int vid576p_tx2;
	int vid720p_tx1;
	int vid720p_tx2;
	int vid1080p_tx1;
	int vid1080p_tx2;
	
};

struct hdmi_ip_data {
	
	void __iomem	*base;	/* HDMI*/
	
	const struct owl_hdmi_ip_ops *ops;
	
	struct hdmi_config cfg;

	struct hdmi_settings settings;
	
	struct drv_cfg txrx_cfg;
	
	struct hdmi_hdcp hdcp;
		
	struct mutex lock;
};

struct hdmi_edid{

	u8		EDID_Buf[1024];
	u8 		Device_Support_VIC[512];
	u8      isHDMI;
	u8      YCbCr444_Support;
	u32     video_formats[4];
	bool    read_ok;
};

struct hdmi_core{
	struct mutex lock;
	
	struct device *dev;
	
	struct platform_device *pdev;

	struct hdmi_ip_data ip_data;
	
	struct hdmi_edid edid;
	
	struct data_flag data;
	
	struct hw_diff *hdmihw_diff;
	
	struct owl_dss_device *dssdev;

};
extern struct hdmi_core hdmi;

struct owl_hdmi_ip_ops {
	
	void (*pmds_ldo_enable)(struct hdmi_ip_data *ip_data, bool enable);

	void (*hdmi_devclken)(struct hdmi_ip_data *ip_data, bool enable);
	
	void (*hdmi_clk24Men)(struct hdmi_ip_data *ip_data, bool enable);

	void (*hdmi_reset)(struct hdmi_ip_data *ip_data);
	
	int (*pll_enable)(struct hdmi_ip_data *ip_data);

	void (*pll_disable)(struct hdmi_ip_data *ip_data);
	
	int (*read_reg)(struct hdmi_ip_data *ip_data,	const u16 idx);
	
	void (*write_reg)(struct hdmi_ip_data *ip_data,	const u16 idx, u32 val);
	
	bool (*cable_check)(struct hdmi_ip_data *ip_data);
	
	bool (*detect)(struct hdmi_ip_data *ip_data);	
	
	void (*hpd_enable)(struct hdmi_ip_data *ip_data,bool enable);

	void (*hpd_clear_plug)(struct hdmi_ip_data *ip_data);

	void (*phy_enable)(struct hdmi_ip_data *ip_data);

	void (*phy_disable)(struct hdmi_ip_data *ip_data);

	void (*video_enable)(struct hdmi_ip_data *ip_data);

	void (*video_disable)(struct hdmi_ip_data *ip_data);	

	void (*dump_hdmi)(struct hdmi_ip_data *ip_data);	
	
	void (*video_configure)(struct hdmi_ip_data *ip_data);
	
	u32 (*get_irq_state)(struct hdmi_ip_data *ip_data);
};

/**************************
hdmi_panel.c
**************************/

int hdmi_panel_init(void);
void hdmi_panel_exit(void);

/**************************
hdmi.c
**************************/
void hdmi_send_uevent(bool data);

int owldss_hdmi_display_enable(struct owl_dss_device *dssdev);
void owldss_hdmi_display_disable(struct owl_dss_device *dssdev);
void owldss_hdmi_display_set_timing(struct owl_dss_device *dssdev,	struct owl_video_timings *timings);
void owldss_hdmi_display_set_vid(struct owl_dss_device *dssdev,	int vid);
void owldss_hdmi_display_get_vid(struct owl_dss_device *dssdev, int *vid);
void owldss_hdmi_display_set_overscan(struct owl_dss_device *dssdev,u16 over_scan_width,u16 over_scan_height);
void owldss_hdmi_display_get_overscan(struct owl_dss_device *dssdev, u16 * over_scan_width,u16 * over_scan_height);
void owldss_hdmi_display_enable_hotplug(struct owl_dss_device *dssdev, bool enable);
void owldss_hdmi_display_enable_hdcp(struct owl_dss_device *dssdev, bool enable);
int owldss_hdmi_display_get_vid_cap(struct owl_dss_device *dssdev, int *vid_cap);
int owldss_hdmi_display_get_cable_status(struct owl_dss_device *dssdev);
int owldss_hdmi_read_edid(struct owl_dss_device *dssdev, u8 * buffer , int len);
int owldss_hdmi_display_check_timing(struct owl_dss_device *dssdev, struct owl_video_timings *timings);
int owldss_hdmi_panel_init(struct owl_dss_device *dssdev);
int owl_hdmi_init_platform(void);
int owl_hdmi_uninit_platform(void);

int owldss_hdmi_panel_suspend(struct owl_dss_device *dssdev);
int owldss_hdmi_panel_resume(struct owl_dss_device *dssdev);

int owldss_hdmi_hotplug_debug(struct owl_dss_device *dssdev,int enable);
int owldss_hdmi_cable_debug(struct owl_dss_device *dssdev,int enable);
int owldss_hdmi_uevent_debug(struct owl_dss_device *dssdev,int enable);

void owl_hdmi_set_effect_parameter(struct owl_dss_device *dssdev,enum owl_plane_effect_parameter parameter_id ,int value);
int owl_hdmi_get_effect_parameter(struct owl_dss_device *dssdev, enum owl_plane_effect_parameter parameter_id);
void hdmihw_write_reg(u32 val, const u16 idx);
int hdmihw_read_reg(const u16 idx);
/**************************
hdmi_ip.c
**************************/
void dss_init_hdmi_ip_ops(struct hdmi_ip_data *ip_data);

/**************************
hdmi_sysfs.c
**************************/
int owl_hdmi_create_sysfs(struct device *dev);
/**************************
hdmi_edid.c
**************************/
int ddc_init(void);
int parse_edid(struct hdmi_edid *edid);
int read_edid(u8 * edid , int len);
int i2c_hdcp_write(const char *buf, unsigned short offset, int count) ;
int i2c_hdcp_read(char *buf, unsigned short offset, int count) ;
/**************************
hdcp.c
**************************/
void hdcp_check_handle(struct work_struct *work);
int hdcp_read_key(void);
void hdcp_init(void);
int hdcp_check_ri(void);
int check_ri_irq(void);
/**************************
hdmi_packet.c
**************************/
int  asoc_hdmi_gen_infoframe(struct hdmi_ip_data *ip_data);

#endif
