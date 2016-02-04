/*
 * Copyright (C) 2014 Actions Corporation
 * Author: Hui Wang  <wanghui@actions-semi.com>
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

#ifndef __OWL_OWLDSS_H
#define __OWL_OWLDSS_H

#include <linux/list.h>
#include <linux/kobject.h>
#include <linux/device.h>

/* act TRM gives bitfields as start:end, where start is the higher bit number. For example 7:0 */
#define REG_MASK(start, end)	(((1 << ((start) - (end) + 1)) - 1) << (end))
#define REG_VAL(val, start, end) (((val) << (end)) & REG_MASK(start, end))
#define REG_GET_VAL(val, start, end) (((val) & REG_MASK(start, end)) >> (end))
#define REG_SET_VAL(orig, val, start, end) (((orig) & ~REG_MASK(start, end)) | REG_VAL(val, start, end))


struct owl_dss_device;
struct owl_overlay_manager;

enum owl_display_type {
	OWL_DISPLAY_TYPE_NONE		= 0,
	OWL_DISPLAY_TYPE_LCD		= 1 << 0,
	OWL_DISPLAY_TYPE_DSI		= 1 << 1,
	OWL_DISPLAY_TYPE_EDP		= 1 << 2,
	OWL_DISPLAY_TYPE_CVBS		= 1 << 3,
	OWL_DISPLAY_TYPE_YPBPR		= 1 << 4,
	OWL_DISPLAY_TYPE_HDMI		= 1 << 5,
};

enum owl_plane_effect_parameter {
	OWL_DSS_VIDEO_LIGHTNESS	 = 0,
	OWL_DSS_VIDEO_SATURATION = 1,
	OWL_DSS_VIDEO_CONSTRAST	 = 2,
	OWL_DSS_DEF_EFFECT       = 3,
};
enum owl_plane {
	OWL_DSS_VIDEO1	= 0,
	OWL_DSS_VIDEO2	= 1,
	OWL_DSS_VIDEO3	= 2,
	OWL_DSS_VIDEO4	= 3,
};

enum owl_de_path_id {	
	OWL_DSS_PATH1_ID	= 0,
	OWL_DSS_PATH2_ID = 1,	
};

enum owl_boot_mode {	
	OWL_DSS_BOOT_NORMAL   = 0,
	OWL_DSS_BOOT_CHARGER	= 1,
	OWL_DSS_BOOT_RECOVERY   = 2,
};

enum owl_color_mode {
	OWL_DSS_COLOR_RGB16	    = 0,  /* RGB16  */
	OWL_DSS_COLOR_BGR16	    = 1,  /* RGB16 */
	OWL_DSS_COLOR_ARGB32    = 4,  /* ARGB32*/
	OWL_DSS_COLOR_ABGR32    = 5,  /* ARGB32 */
	OWL_DSS_COLOR_RGBA32    = 6,  /* RGBA32 */
	OWL_DSS_COLOR_BGRA32	= 7,  /* RGBA32 */
	OWL_DSS_COLOR_NV21	    = 8,  /* YUV 4:2:0 sp */	
	OWL_DSS_COLOR_NU21	    = 9,  /* YVU 4:2:0 sp */
	OWL_DSS_COLOR_YU12	    = 10,  /* YUV 4:2:0 */
	OWL_DSS_COLOR_ARGB16	= 12,  /* ARGB16 */
	OWL_DSS_COLOR_ABGR16	= 13, /* ABGR16 */
	OWL_DSS_COLOR_RGBA16	= 14, /* RGBA16 */
	OWL_DSS_COLOR_BGRA16	= 15, /* BGRA16 */	
	OWL_DSS_COLOR_RGB24U	= 16,  /* RGB24, 32-bit container */
	OWL_DSS_COLOR_RGB24P	= 17, /* RGB24, 24-bit container */
	OWL_DSS_COLOR_RGBX32	= 18, /* RGBx32 */
	OWL_DSS_COLOR_NV12	= 19,  /* YVU 4:2:0  */
	OWL_DSS_COLOR_XBGR32	= 20, /* XBGR32 */
	OWL_DSS_COLOR_XRGB32 = 21,
};

enum owl_dither_mode {
	DITHER_24_TO_16 = 0,
	DITHER_24_TO_18,
};

enum owl_lcd_display_type {
	OWL_DSS_LCD_DISPLAY_RGB,
	OWL_DSS_LCD_DISPLAY_LVDS,
};

enum owl_dss_trans_key_type {
	OWL_DSS_COLOR_KEY_GFX_DST = 0,
	OWL_DSS_COLOR_KEY_VID_SRC = 1,
};


enum owl_display_caps {
	OWL_DSS_DISPLAY_CAP_MANUAL_UPDATE	= 1 << 0,
	OWL_DSS_DISPLAY_CAP_TEAR_ELIM		= 1 << 1,
};

enum owl_dss_display_state {
	OWL_DSS_DISPLAY_DISABLED = 0,
	OWL_DSS_DISPLAY_ACTIVE,
	OWL_DSS_DISPLAY_SUSPENDED,
};

/* XXX perhaps this should be removed */
enum owl_dss_managers_id {
	OWL_DSS_OVL_MGR_PRIMARY,
	OWL_DSS_OVL_MGR_EXTERNAL,
};

/* clockwise rotation angle */
enum owl_dss_rotation_angle {
	OWL_DSS_ROT_0   = 0,
	OWL_DSS_ROT_90  = 1,
	OWL_DSS_ROT_180 = 2,
	OWL_DSS_ROT_270 = 3,
};

enum owl_overlay_caps {
	OWL_DSS_OVL_CAP_SCALE = 1 << 0,
	OWL_DSS_OVL_CAP_GLOBAL_ALPHA = 1 << 1,
	OWL_DSS_OVL_CAP_PRE_MULT_ALPHA = 1 << 2,
	OWL_DSS_OVL_CAP_ZORDER = 1 << 3,
};

enum owl_overlay_manager_caps {
	OWL_DSS_DUMMY_VALUE, /* add a dummy value to prevent compiler error */
};

enum owl_dss_clk_source {
	OWL_DSS_CLK_SRC_DEV_PLL = 0,		
						
	OWL_DSS_CLK_SRC_DISPLAY_PLL,	
						
	OWL_DSS_CLK_SRC_NAND_PLL,	
};



enum owl_dss_signal_level {
	OWLDSS_SIG_ACTIVE_HIGH	= 0,
	OWLDSS_SIG_ACTIVE_LOW	= 1,
};

struct owl_video_timings {
	/* Unit: pixels */
	u16 x_res;
	/* Unit: pixels */
	u16 y_res;
	/* Unit: KHz */
	u32 pixel_clock;
	/* Unit: pixel clocks */
	u16 hsw;	/* Horizontal synchronization pulse width */
	/* Unit: pixel clocks */
	u16 hfp;	/* Horizontal front porch */
	/* Unit: pixel clocks */
	u16 hbp;	/* Horizontal back porch */
	/* Unit: line clocks */
	u16 vsw;	/* Vertical synchronization pulse width */
	/* Unit: line clocks */
	u16 vfp;	/* Vertical front porch */
	/* Unit: line clocks */
	u16 vbp;	/* Vertical back porch */
	
	/* Vsync logic level */
	enum owl_dss_signal_level vsync_level;
	/* Hsync logic level */
	enum owl_dss_signal_level hsync_level;
	/* Interlaced or Progressive timings */
	bool interlace;
	/*Vsync start line */
	int vstart;
	/*video data repetetion*/
	bool repeat;	

	u16 data_width;
};

struct owl_gamma_info {
	u8 value[256];
	bool enabled;
};

struct owl_ovl_priv_info {
	int 	lightness;
	int 	saturation;
	int 	contrast;
};


struct owl_overlay_info {
	unsigned long paddr;
	unsigned long vaddr;	/* for MMU */
	u64 buffer_id;			/* for ION MMU */
	bool enable_mmu;
	
	unsigned short screen_width;	
	enum owl_color_mode color_mode;		
	unsigned short img_width;
	unsigned short img_height;
		
	unsigned short xoff;	
	unsigned short yoff;
	unsigned short width;
	unsigned short height;
	
	unsigned char rotation;
	
	unsigned short pos_x;
	unsigned short pos_y;
	unsigned short out_width;	/* if 0, out_width == width */
	unsigned short out_height;	/* if 0, out_height == height */
	
	unsigned char lightness;
	unsigned char saturation;
	unsigned char contrast;
	bool global_alpha_en;
	unsigned char global_alpha;
	
	bool pre_mult_alpha_en;
	
	unsigned char zorder;
};

struct ovl_priv_data {

	bool user_info_dirty;
	struct owl_overlay_info user_info;

	bool info_dirty;
	struct owl_overlay_info info;

	bool shadow_info_dirty;

	bool extra_info_dirty;
	bool shadow_extra_info_dirty;

	bool enabled;
	enum owl_dss_managers_id channel;
	u32 fifo_low, fifo_high;

	/*
	 * True if overlay is to be enabled. Used to check and calculate configs
	 * for the overlay before it is enabled in the HW.
	 */
	bool enabling;
};

struct owl_overlay {
	struct kobject kobj;
	struct list_head list;

	/* static fields */
	const char *name;
	enum owl_plane id;
	enum owl_color_mode supported_modes;
	enum owl_overlay_caps caps;
	
	struct ovl_priv_data priv_data;

	/* dynamic fields */	
	struct owl_overlay_manager *manager;

	/*
	 * The following functions do not block:
	 *
	 * is_enabled
	 * set_overlay_info
	 * get_overlay_info
	 *
	 * The rest of the functions may block and cannot be called from
	 * interrupt context
	 */

	int (*enable)(struct owl_overlay *ovl);
	int (*disable)(struct owl_overlay *ovl);
	bool (*is_enabled)(struct owl_overlay *ovl);

	int (*set_manager)(struct owl_overlay *ovl,
		struct owl_overlay_manager *mgr);
	int (*unset_manager)(struct owl_overlay *ovl);

	int (*set_overlay_info)(struct owl_overlay *ovl,
			struct owl_overlay_info *info);
	void (*get_overlay_info)(struct owl_overlay *ovl,
			struct owl_overlay_info *info);
	int (*apply_overlay_info)(struct owl_overlay *ovl);
	
	int (*write_hw_regs)(struct owl_overlay *ovl);
	
	int (*enable_overlay_mmu)(struct owl_overlay *ovl,bool enable);

	int (*wait_for_go)(struct owl_overlay *ovl);
};

static inline struct ovl_priv_data *get_ovl_priv(struct owl_overlay *ovl)
{
	return &ovl->priv_data;
}

struct owl_cursor_info {
	bool enable;
	unsigned short pos_x;
	unsigned short pos_y;
	void * paddr;
	unsigned short stride;

};

struct owl_overlay_manager_info {
	u16 out_width;
	u16 out_height;		
	u32 default_color;
	
	enum owl_dss_trans_key_type trans_key_type;
	u32 trans_key;
	bool trans_enabled;
	
	bool partial_alpha_enabled;

};

#define MMU_STATE_NO_ENABLE              0
#define MMU_STATE_PRE_ENABLE             1
#define MMU_STATE_ENABLED                2
struct mgr_priv_data {

	bool user_info_dirty;
	struct owl_overlay_manager_info user_info;

	bool info_dirty;
	struct owl_overlay_manager_info info;
	
	bool user_cursor_dirty;
	struct owl_cursor_info user_cursor;
	
	bool cursor_dirty;
	struct owl_cursor_info cursor;

	bool shadow_info_dirty;
	
	/*this mark we need switch to mmu , in uboot mmu is disable 
	,before first frame we check if need switch to mmu
	,after first frame we make sure mmu is enabled
    */
	int mmu_state;

	/* If true, GO bit is up and shadow registers cannot be written.
	 * Never true for manual update displays */
	bool busy;	
    
	/* If true, dispc output is enabled */
	bool updating;

	/* If true, a display is enabled using this manager */
	bool enabled;
	
	/* this for hw vsync */
	struct device *dev; // this for vsync event 
	bool hw_sync_enable;
	ktime_t timestamp;				
	atomic_t vsync_avail;
	struct work_struct  vsync_work;	
	wait_queue_head_t wait_vsync;	
};

struct owl_overlay_manager {
	struct kobject kobj;

	/* static fields */
	const char *name;
	
	enum owl_dss_managers_id id;
	
	/* this depath config */
		
	enum owl_de_path_id de_path_id;
	
	enum owl_overlay_manager_caps caps;
	struct list_head overlays;
	enum owl_display_type supported_displays;
	
	struct mgr_priv_data priv_data;
	
	struct mutex apply_lock;
	/* dynamic fields */
	struct owl_dss_device * device;
	bool mirror_context;
	struct fb_info * link_fbi;
	/* 
         * gamma setting is not controlled by FCR bit,
	 * so we should wait for vsync to update gamma info
	 */
	struct owl_gamma_info gamma_info;
	bool gamma_info_dirty;
	struct mutex gamma_lock;

	/*
	 * The following functions do not block:
	 *
	 * set_manager_info
	 * get_manager_info
	 * apply
	 *
	 * The rest of the functions may block and cannot be called from
	 * interrupt context
	 */

	int (*set_device)(struct owl_overlay_manager *mgr,	struct owl_dss_device *dssdev);
	int (*unset_device)(struct owl_overlay_manager *mgr);

	int (*set_manager_info)(struct owl_overlay_manager *mgr, struct owl_overlay_manager_info *info);
	void (*get_manager_info)(struct owl_overlay_manager *mgr, struct owl_overlay_manager_info *info);
	int (*apply_manager_info)(struct owl_overlay_manager *mgr);
	
	int (*set_cursor_info)(struct owl_overlay_manager *mgr, struct owl_cursor_info *info);
	void (*get_cursor_info)(struct owl_overlay_manager *mgr, struct owl_cursor_info *info);

	int (*apply)(struct owl_overlay_manager *mgr);
	
	int (*set_mmu_state)(struct owl_overlay_manager *mgr, int mmu_state);
		
	int (*enable_hw_vsync)(struct owl_overlay_manager *mgr, bool enabled, struct device *dev);
	
	int (*wait_for_go)(struct owl_overlay_manager *mgr);
	
	int (*wait_for_vsync)(struct owl_overlay_manager *mgr,long long i64TimeStamp);
};
struct owl_dss_device {
	struct device dev;

	enum owl_display_type type;

	enum owl_dss_managers_id channel;
	
	struct owl_video_timings timings;
	
	u8 data_lines;

	const char *name;

	/* used to match device to driver */
	const char *driver_name;

	void *data;

	struct owl_dss_driver *driver;

	/* helper variable for driver suspend/resume */
	bool activate_after_resume;
	
	enum owl_display_caps caps;

	struct owl_overlay_manager *manager;

	enum owl_dss_display_state state;

	/* platform specific  */
	int (*platform_enable)(struct owl_dss_device *dssdev);
	void (*platform_disable)(struct owl_dss_device *dssdev);
	int (*set_backlight)(struct owl_dss_device *dssdev, int level);
	int (*get_backlight)(struct owl_dss_device *dssdev);
};
static inline struct mgr_priv_data *get_mgr_priv(struct owl_overlay_manager *mgr)
{
	return &mgr->priv_data;
}
struct owl_dss_driver {
	struct device_driver driver;

	int (*probe)(struct owl_dss_device *);
	void (*remove)(struct owl_dss_device *);

	int (*enable)(struct owl_dss_device *display);
	void (*disable)(struct owl_dss_device *display);
	
	int (*suspend)(struct owl_dss_device *display);
	int (*resume)(struct owl_dss_device *display);
	int (*run_test)(struct owl_dss_device *display, int test);

	void (*get_resolution)(struct owl_dss_device *dssdev,	u16 *xres, u16 *yres);
	void (*get_dimensions)(struct owl_dss_device *dssdev,	u32 *width, u32 *height);
	int (*get_recommended_bpp)(struct owl_dss_device *dssdev);

	int (*check_timings)(struct owl_dss_device *dssdev, struct owl_video_timings *timings);
	void (*set_timings)(struct owl_dss_device *dssdev,	struct owl_video_timings *timings);
	void (*get_timings)(struct owl_dss_device *dssdev,	struct owl_video_timings *timings);
	void (*get_over_scan)(struct owl_dss_device *dssdev, u16 * scan_width,  u16 * scan_height);
	void (*set_over_scan)(struct owl_dss_device *dssdev, u16 scan_width,  u16 scan_height);
	int (*get_effect_parameter)(struct owl_dss_device *dssdev,	enum owl_plane_effect_parameter parameter_id);
	void (*set_effect_parameter)(struct owl_dss_device *dssdev,	enum owl_plane_effect_parameter parameter_id,int value);
	void (*set_vid)(struct owl_dss_device *dssdev, int vid);
	void (*get_vid)(struct owl_dss_device *dssdev, int *vid);
	void (*enable_hpd)(struct owl_dss_device *dssdev, bool enable);
	void (*enable_hdcp)(struct owl_dss_device *dssdev, bool enable);
	int (*get_vid_cap)(struct owl_dss_device *dssdev, int *vid_cap);
	int (*get_cable_status)(struct owl_dss_device *dssdev);
	
	int (*read_edid)(struct owl_dss_device *dssdev, u8 *buf, int len);	
	int (*dump)(struct owl_dss_device *dssdev);
	
	int (*hot_plug_nodify)(struct owl_dss_device *dssdev, int state);
	
};

int owl_dss_register_driver(struct owl_dss_driver *);
void owl_dss_unregister_driver(struct owl_dss_driver *);

void owl_dss_get_device(struct owl_dss_device *dssdev);
void owl_dss_put_device(struct owl_dss_device *dssdev);
#define for_each_dss_dev(d) while ((d = owl_dss_get_next_device(d)) != NULL)
struct owl_dss_device *owl_dss_get_next_device(struct owl_dss_device *from);
struct owl_dss_device *owl_dss_find_device(void *data, int (*match)(struct owl_dss_device *dssdev, void *data));
struct owl_dss_device *owl_dss_find_device_by_type(enum owl_display_type type);

int owl_dss_start_device(struct owl_dss_device *dssdev);
void owl_dss_stop_device(struct owl_dss_device *dssdev);
int owl_dss_get_num_overlay_managers(void);
struct owl_overlay_manager *owl_dss_get_overlay_manager(int num);
bool owl_dss_is_devices_suspended(void);

int owl_dss_get_num_overlays(void);
struct owl_overlay *owl_dss_get_overlay(int num);

void owl_default_get_resolution(struct owl_dss_device *dssdev, 	u16 *xres, u16 *yres);
int owl_default_get_recommended_bpp(struct owl_dss_device *dssdev);

#define to_dss_driver(x) container_of((x), struct owl_dss_driver, driver)
#define to_dss_device(x) container_of((x), struct owl_dss_device, dev)

enum owl_display_type get_current_display_type(void);

int owl_lcdc_display_enable(struct owl_dss_device *dssdev);
void owl_lcdc_display_disable(struct owl_dss_device *dssdev);
void owl_lcdc_set_timings(struct owl_dss_device *dssdev,struct owl_video_timings *timings);
int owl_lcdc_check_timings(struct owl_dss_device *dssdev,	struct owl_video_timings *timings);
int owl_lcdc_init_platform(void);
int owl_lcdc_uninit_platform(void);
void owl_lcdc_display_dump(void);
void owl_lcdc_set_effect_parameter(struct owl_dss_device *dssdev,enum owl_plane_effect_parameter parameter_id ,int value);
int owl_lcdc_get_effect_parameter(struct owl_dss_device *dssdev, enum owl_plane_effect_parameter parameter_id);
void owl_lcdc_select_video_timings(struct owl_dss_device *dssdev, u32 num,
                                   struct owl_video_timings *timings);

int owl_edp_display_enable(struct owl_dss_device *dssdev);
void owl_edp_display_disable(struct owl_dss_device *dssdev);
int owl_edp_init_platform(void);
int owl_edp_uninit_platform(void);
void owl_edp_display_dump(void);


int owl_dsi_display_enable(struct owl_dss_device *dssdev);
void owl_dsi_display_disable(struct owl_dss_device *dssdev);
int owl_dsi_init_platform(void);
int owl_dsi_uninit_platform(void);
void owl_dsi_display_dump(void);
bool dss_check_channel_boot_inited(enum owl_de_path_id channel);

int owl_cvbs_init_platform(void);
int owldss_cvbs_display_enable(struct owl_dss_device *dssdev);
//void cvbs_panel_enable(struct owl_dss_device *dssdev);


void owl_de_get_histogram_info(u32 * hist, u32 * totaly);
int owl_de_get_boot_mode(void);
bool owl_de_is_atm7059tc(void);

void dss_mgr_set_gamma_table(enum owl_de_path_id channel,
				struct owl_gamma_info *gamma_info);
void dss_mgr_get_gamma_table(enum owl_de_path_id channel,
				struct owl_gamma_info *gamma_info);
void trace_buffer_release(void * args);

void trace_buffer_put_to_queue(void * args);

void trace_buffer_put_to_dehw(void * args);

void trace_vsync_point(int index, long long timestamp);

#define DEF_LIGHTNESS   0x80
#define DEF_SATURATION  0x07
#define DEF_CONTRAST    0x07

#endif
