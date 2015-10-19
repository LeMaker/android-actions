/*
 * Copyright (C) 2008 Nokia Corporation
 * Author: Tomi Valkeinen <tomi.valkeinen@nokia.com>
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

#ifndef __DSS_OWL_H__
#define __DSS_OWL_H__

#include <linux/list.h>
#include <linux/kobject.h>
#include <linux/device.h>
#include <linux/atomic.h>

#include <mach/display-owl.h>
#include <mach/dss_user-owl.h>

struct owl_overlay_manager;

#define OWL_DISP_CHANNEL_NUM	2
#define MAX_CHANNEL_NUM	OWL_DISP_CHANNEL_NUM

enum disp_dev_id {
	LCD_DISPLAYER = 0x1,
	HDMI_DISPLAYER = 0x2,
	TV_CVBS_DISPLAYER = 0x4,
	TV_YPbPr_DISPLAYER = 0x8,
	LCD1_DISPLAYER = 0x10,
	DSI_DISPLAYER = 0x20,
};

#define MAX_NUM_DISP_DEV	4

#ifdef CONFIG_ACT_EBOX
#define OWL_INIT_DISPLAYERS_MDSB		\
	(HDMI_DISPLAYER | TV_CVBS_DISPLAYER)
#endif

/*
enum owl_layer_id {
	GL5201DE_GRAPHIC_LAYER	= 0,
	GL5201DE_VIDEO_LAYER		= 1,
	MAX_OVERLAY_NUM
};
*/

#define OWL_MAX_LAYER_REGION_NUM		1
#define MAX_LAYER_REGION_NUM		OWL_MAX_LAYER_REGION_NUM

enum owl_overlay_manager_id {
	OWL_OVERLAY_MANAGER_PRIMARY	= 0,
	OWL_OVERLAY_MANAGER_OVERLAY	= 1,
};

enum owl_overlay_caps {
	OWL_DSS_OVL_CAP_SCALE = 1 << 0,
	OWL_DSS_OVL_CAP_REGIONS = 1 << 1,
	OWL_DSS_OVL_CAP_2X = 1 << 2,

	/*rotate stuff*/
	OWL_DSS_OVL_CAP_ROT_180 = 1 << 3,
};

enum owl_overlay_manager_caps {
	OWL_DSS_OVL_MGR_CAP_DISPC = 1 << 0,
};

enum owl_channel_caps {
	OWL_DSS_CHANNEL_CAP_MDSB = 1 << 0,
};

enum owl_arrange_option {
	/*no scale, preserve aspect*/
	ARR_ORIGINAL_SIZE		= 0x0,

	/*fill the whole screen,ignore aspect*/
	ARR_SCALE_FULL_SIZE		= 0x1,

	/*scale the image to fit the screen, centred , preserve aspect*/
	ARR_SCALE_FIT_SIZE		= 0x2,

	/*if original image is smaller than screen, then original size*/
	ARR_SCALE_ORIGINAL_OR_FIT_SIZE	= 0x3,

	ARR_SCALE_MAX,
	ARR_BASE_MASK = 0xf,

	/*extra arrange methods*/
	ARR_2X_MODE			= 0x10,
	ARR_ALL_CUSTOM		= 0x40,


	ARR_EXTRA_MASK		= 0xf0,

	ARR_TYPES_MASK		=
			(ARR_BASE_MASK | ARR_EXTRA_MASK),


	/*user defined cor&size, need to remap according to scale option*/
	ARR_REMAP_FLAG	= 0x100,
	ARR_SHUTDOWN_FLAG	= 0x200,

	ARR_FLAGS_MASK	= 0xf00,

};

typedef union {
		u32 rgb565;
		struct {u32 y, u, v; } yuv_p;
		struct {u32 y, uv; } yuv_sp;
		u32 yuv_interleaved;
		struct {u32 yc, uvc, ylut, uvlut; } yuv_spc;
		struct {u32 ly, luv, ry, ruv; } yuv_sp3d;
} pixel_t;

struct rgb_color {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};


enum COLORKEY_MATCH_RULE {
	MATCH_ALWAYS  = 0x00,
	MATCH_BETWEEN = 0x01,
	MATCH_EXCEPT  = 0x02,
};

struct colorkey {
	struct rgb_color min;
	struct rgb_color max;

	unsigned int  green_match_rule;
	unsigned int  red_match_rule;
	unsigned int  blue_match_rule;

};

struct owl_channel {
	struct list_head list;

	int id;

	u32 caps;

	int num_display;
	u32 display_ids;
	struct owl_display_device *display[MAX_NUM_DISP_DEV];

	int num_layers;
	int layers_bitmap;
	int prim_ovl_id;

	int preline_exptime_us;

	struct rgb_color background;

/*this "enable" member cannot be cached*/
/*the channel enable/disable ops will flush it immediately*/
	bool enable;
};

struct owl_region {
	struct kobject kobj;
	struct owl_overlay *ovl;
	int id;
/*****config info*******************/
	bool enable;

	/*input information*/
	u16 xoff;
	u16 yoff;
	u16 input_width;
	u16 input_height;

	/* output information*/
	u16 xcor;
	u16 ycor;
	u16 output_width;
	u16 output_height;

/****states*********************/
	pixel_t data_buffer;

};

struct owl_overlay_info {
	u32 paddr;
	void __iomem *vaddr;
	u32 width;
	u32 height;
	enum owl_pixel_format pixfmt;
	u32 rotation;
	bool global_alpha_enable;
	u8 global_alpha;

	bool trans_enable;
	struct colorkey trans_key;

	bool ilace;
	u32 field;

	pixel_t src_w;		/*line length in pixel*/

	const struct fb_videomode *required_modes[MAX_NUM_DISP_DEV];
	int required_mdsb_table_subs;

	/*states, users don't need to set them, */
	/*dss will encode them using infomations above*/
	pixel_t bpp;
	pixel_t pitch;			/*line length in bytes*/

/*CURRENTLY, we assume all regions are controlled by the layer*/
/*which means the overlay info will be set to every region*/
/*but things may happen that some regions will controlled seperately, TODO*/
	u32 arrange;
	u32 w_scale_rate;
	u32 h_scale_rate;


	u32 umap_xoff;
	u32 umap_yoff;
	u32 umap_input_w;
	u32 umap_input_h;
	u32 virt_scr_w;
	u32 virt_scr_h;
	u32 umap_xcor;
	u32 umap_ycor;
	u32 umap_output_w;
	u32 umap_output_h;

	int num_regions;
	struct owl_region region[MAX_LAYER_REGION_NUM];
};

struct overlay_stack {
	struct owl_overlay *ovls[MAX_OVERLAY_NUM];
	int ovl_num;
};

enum owl_overlay_stack_state {
	OVL_STACK_NONE = 0,
	OVL_STACK_SLAVE = 1,
	OVL_STACK_PRIM= 1,
};

struct owl_overlay {
	struct kobject kobj;
	struct list_head list;

	/* static fields */
	char name[32];
	int id;
	u32 supported_pixfmts;
	u32 caps;

	const char *owner_name;

	/* dynamic fields */
	struct owl_overlay_manager *manager;

	bool channel_changed;
	struct owl_channel *channel;

	bool wakeup;
	bool state;

	bool enable;
	bool info_dirty;
	struct owl_overlay_info info;

	char master_dev_type[20];
	struct platform_device *master_pdev;

	struct overlay_stack *slave_ovls;
	u32 stack_state; /*SEE OVL_STACK_*/

	int (*set_manager)(struct owl_overlay *ovl,
		struct owl_overlay_manager *mgr, bool init);
	int (*unset_manager)(struct owl_overlay *ovl);

	int (*set_overlay_info)(struct owl_overlay *ovl,
			struct owl_overlay_info *info);
	void (*get_overlay_info)(struct owl_overlay *ovl,
			struct owl_overlay_info *info);
	int (*set_overlay_flip)(struct owl_overlay *ovl,
			u32 paddr, void __iomem *vaddr, u32 field);

/*	int (*wait_for_go)(struct owl_overlay *ovl);*/

	struct mutex ovl_lock;
};


struct owl_global_info {
	bool info_dirty;

};

struct owl_overlay_manager {
	struct kobject kobj;
	struct list_head list;

	/* static fields */
	char name[32];
	int id;
	u32 caps;

	/* dynamic fields */
	struct owl_global_info *info;

	int (*set_device)(struct owl_overlay *ovl, u32 disp_dev_ids);
	int (*unset_device)(struct owl_overlay *ovl);

	int (*set_manager_info)(struct owl_overlay_manager *mgr,
			struct owl_global_info *info);
	void (*get_manager_info)(struct owl_overlay_manager *mgr,
			struct owl_global_info *info);
	int (*write_colreg)(struct owl_overlay_manager *mgr,
			unsigned regno, unsigned int colreg_data);
	int (*channel_apply)(struct owl_overlay *ovl);
	int (*apply)(struct owl_overlay *ovl, bool force);
	int (*overlay_stack_apply)(struct owl_overlay *ovl, bool force);
	int (*flip)(struct owl_overlay *ovl, u32 paddr,
		void __iomem *vaddr, u32 field);
	int (*overlay_stack_flip)(struct owl_overlay *ovl, u32 paddr[],
		void __iomem *vaddr[], u32 field[], int num_ovls);
	int (*wait_for_go)(struct owl_overlay *ovl);
	int (*wait_for_vsync)(struct owl_overlay *ovl);

	int (*enable)(struct owl_overlay *ovl);
	int (*disable)(struct owl_overlay *ovl);
	int (*sleep)(struct owl_overlay *ovl);
	int (*wakeup)(struct owl_overlay *ovl);
};

static inline int mode_string(char *buf, unsigned int offset,
		       const struct fb_videomode *mode)
{
	char m = 'U';
	char v = 'p';

	if (mode->flag & FB_MODE_IS_DETAILED)
		m = 'D';
	if (mode->flag & FB_MODE_IS_VESA)
		m = 'V';
	if (mode->flag & FB_MODE_IS_STANDARD)
		m = 'S';

	if (mode->vmode & FB_VMODE_INTERLACED)
		v = 'i';
	if (mode->vmode & FB_VMODE_DOUBLE)
		v = 'd';

	return snprintf(&buf[offset], PAGE_SIZE - offset, "%c:%dx%d%c-%d\n",
		m, mode->xres, mode->yres, v, mode->refresh);
}

extern unsigned int snd_displayers_st_string(char *buf,
	int plugged_disp_dev_ids, int registered_disp_dev_ids);
extern unsigned int snd_displayers_string(char *buf, int disp_dev_ids);
extern unsigned int rev_displayer_string(int *disp_dev_ids, const char *name);

extern struct fb_videomode *fb_find_mode_by_str(
	const char *buf, int len, struct list_head *head);

extern int string_to_arrange_id(const char *name);


int owl_dss_get_num_overlay_managers(void);
struct owl_overlay_manager *owl_dss_get_overlay_manager(int num);

int owl_dss_get_num_overlays(void);
struct owl_overlay *owl_dss_get_overlay(int num);
int owl_dss_grab_overlay(struct owl_overlay *ovl, struct device *owner);
int owl_dss_release_overlay(struct owl_overlay *ovl, struct device *owner);
bool is_overlay_inited(struct owl_overlay *ovl);

struct owl_channel *owl_dss_get_channel(int num);

extern int owl_compare_overlay_stack(struct owl_overlay *prim,
		int ovl_ids[], int ovl_num,
		int remove_ovl_ids[], int *remove_ovl_num,
		int add_ovl_ids[], int *add_ovl_num);
extern int owl_add_overlay_stack(
		struct owl_overlay *prim, int ovl_id[], int ovl_num);
extern int owl_remove_overlay_stack(
		struct owl_overlay *prim, int ovl_id[], int ovl_num);
extern void owl_destory_overlay_stack(struct owl_overlay *prim);
extern int _owl_overlay_manager_set(struct owl_overlay *ovl,
	struct owl_overlay_manager *mgr, bool init);
extern int owl_overlay_manager_set(struct owl_overlay *ovl,
	struct owl_overlay_manager *mgr, bool init);
extern void owl_get_overlay_info(struct owl_overlay *ovl,
	struct owl_overlay_info *info);
extern int owl_set_overlay_info(struct owl_overlay *ovl,
	struct owl_overlay_info *info);

extern int owl_dss_update_display_device_state(struct owl_channel *channel);

extern int owl_overlay_channel_set(struct owl_overlay *ovl,
	struct owl_channel *channel,
	struct owl_overlay_manager *mgr, bool init);
extern int owl_disp_device_update(struct owl_overlay *ovl);
extern int owl_disp_device_set(struct owl_overlay *ovl, u32 disp_dev_ids);
extern int owl_overlay_channel_apply(struct owl_overlay *ovl);
extern int owl_overlay_apply(struct owl_overlay *ovl);
extern int owl_overlay_flip(struct owl_overlay *ovl,
	u32 paddr, void __iomem *vaddr, u32 field);
extern int owl_overlay_stack_apply(struct owl_overlay *ovl);
extern int owl_overlay_stack_flip(struct owl_overlay *ovl,
		u32 paddr[], void __iomem *vaddr[], u32 field[], int num_ovls);
extern int owl_overlay_wait_for_go(struct owl_overlay *ovl);
extern int owl_overlay_wait_for_vsync(struct owl_overlay *ovl);
extern int owl_overlay_enable(struct owl_overlay *ovl);
extern int owl_overlay_disable(struct owl_overlay *ovl);
extern int owl_overlay_sleep(struct owl_overlay *ovl);
extern int owl_overlay_wakeup(struct owl_overlay *ovl);
const char *owl_overlay_master_dev_get(struct owl_overlay *ovl);
extern int owl_overlay_master_dev_set(struct owl_overlay *ovl,
	const char *dev_type, void *pdata);
extern int owl_overlay_rotate_check(struct owl_overlay *ovl, u32 rotation);
extern int owl_get_overlay_stack_state(struct owl_overlay *ovl);
extern int owl_set_overlay_stack_state(struct owl_overlay *ovl,
	u32 stack_state);



static inline bool ovl_enable_encode(struct owl_overlay *ovl)
{
	return ovl->wakeup && ovl->state;
}

/*****de irq callback register*********************/
#define OWL_DE_ISR_TYPE_ONESHOT		0
#define OWL_DE_ISR_TYPE_STICK		1

typedef void (*owl_de_isr_t) (void *arg, u32 mask);
typedef void (*owl_de_work_t) (void *arg);

extern int owl_de_register_isr(u32 channel_id,
	owl_de_isr_t isr, void *arg, u32 mask, u32 type);
extern int owl_de_unregister_isr(u32 channel_id,
	owl_de_isr_t isr, void *arg, u32 mask, u32 type);
extern int owl_de_register_isr_work(u32 channel_id,
		owl_de_isr_t isr, void *arg_isr,
		owl_de_work_t work, void *arg_work,
		u32 mask, u32 type);
extern int owl_de_unregister_isr_work(u32 channel_id,
		owl_de_isr_t isr, void *arg_isr,
		owl_de_work_t work, void *arg_work,
		u32 mask, u32 type);
/**************************/

#define OWL_DSS_EVENT_OVL_DISPLAY_SET	1
#define OWL_DSS_EVENT_OVL_CHANNEL_SET	2
#define OWL_DSS_EVENT_OVL_DISPLAY_UPDATE	3
#define OWL_DSS_EVENT_OVL_DISPLAY_PLUG	4
#define OWL_DSS_EVENT_OVL_ENABLE_REQ	5

struct owl_dss_channlevt {
	u32 channel_id;
	u32 changed_ovl_id;
};

struct owl_dss_plugevt {
	u32 changed_disp_dev_ids;
	u32 plugged_disp_dev_ids;
};

extern int owl_dss_register_client(struct notifier_block *nb);
extern int owl_dss_unregister_client(struct notifier_block *nb);
extern int owl_dss_notifier_call_chain(unsigned long val, void *v);


extern void print_fb_videomode(const struct fb_videomode *mode);
extern void owl_copy_modelist(struct list_head *to , struct list_head *from);


#ifdef CONFIG_OWL_DSS_MDSB
extern int mdsb_info_get(struct owl_display_device *displays[],
					const struct fb_videomode *modes[],
					int *mdsb_table_subs);
extern int mdsb_set(int mdsb_table_subs);
extern int mdsb_apply(void);
extern int mdsb_hw_enable(void);
extern int mdsb_hw_disable(void);
extern int mdsb_enable(void);
extern int mdsb_disable(void);
extern int mdsb_init(void);
#else
static inline int mdsb_info_get(struct owl_display_device *displays[],
					const struct fb_videomode *modes[],
					int *mdsb_table_subs){
	return 0;
}

static inline int mdsb_set(int mdsb_table_subs)
{
	return 0;
}

static inline int mdsb_apply(void)
{
	return 0;
}

static inline int mdsb_hw_enable(void)
{
	return 0;
}

static inline int mdsb_hw_disable(void)
{
	return 0;
}

static inline int mdsb_enable(void)
{
	return -EINVAL;
}

static inline int mdsb_disable(void)
{
	return 0;
}

static inline int mdsb_init(void)
{
	return 0;
}

#endif


/* actions framebuffer platform data*/
struct owl_fb_pdata {
	int overlay_id;
	bool show_logo;
	bool inited;
	bool use_rsvmem;

	int bpp;
	int img_w;
	int img_h;
};

#define OWL_FBDRV_NAME		"owlfb"
#define OWL_VOUTDRV_NAME	"gl5201_vout"

/*framebuffer reserve mem block*/
extern unsigned int owl_fb_rsvmem_start;
extern unsigned int owl_fb_rsvmem_size;

/**/

/******lcd gamma stuff************/

extern unsigned int lcdgamma_rsvmem_start;
extern unsigned int lcdgamma_rsvmem_size;

extern u32 lcdgamma_table[];
extern int lcdgamma_table_valid;

/**/

/*dss init state from u-boot*/

/**/

#define PICOS2KHZ_ROUND(a) (DIV_ROUND_CLOSEST(1000000000UL, a))
#define KHZ2PICOS_ROUND(a) (DIV_ROUND_CLOSEST(1000000000UL, a))

#define PICOS2KHZ_ROUND_UP(a) (DIV_ROUND_UP(1000000000UL, a))
#define KHZ2PICOS_ROUND_UP(a) (DIV_ROUND_UP(1000000000UL, a))

#ifdef CONFIG_OWL_DSS
extern int dss_bl_disable_charge(void);
#else
static inline int dss_bl_disable_charge(void)
{
	return 0;
}
#endif

#endif
