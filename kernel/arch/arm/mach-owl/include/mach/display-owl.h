/*
 *
 *  Copyright (C) 2006 Luming Yu <luming.yu@intel.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#ifndef _LINUX_OWL_DISPLAY_H
#define _LINUX_OWL_DISPLAY_H
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#include <mach/dss-owl.h>

#define	DUMMY_DISPLAYER		0x40000000

struct owl_display_device;
//struct fb_info;

//struct fb_videomode;

struct display_ops {
	int (*update_status)(struct owl_display_device *);
	int (*get_status)(struct owl_display_device *);
	int (*update_mode)(struct owl_display_device *);
	int (*set_preline)(struct owl_display_device *, int num_preline);
	int (*get_bpp)(struct owl_display_device *);

	/*if src_clk_hz == 0, it means using the dev current src clk*/
	/*if mode == NULL, it means using the dev current mode*/
	unsigned int (*get_devclk_khz)(struct owl_display_device *, unsigned int src_clk_khz, const struct fb_videomode *mode);
	unsigned int (*get_tvoutpll1_khz)(struct owl_display_device *, const struct fb_videomode *mode);

	int (*check_fb)(struct fb_info *);
};

struct owl_videomode{
	int valid;
	struct fb_videomode mode;
};

struct owl_display_device {
/***parameters that disp dev provides, framebuffer driver will never change them******/
	int display_id;		/*LCD_DISPLAYER,HDMI_DISPLAYER... ...*/
	struct list_head modelist;
	const struct owl_videomode *modes;
	int num_modes;
	const struct fb_videomode *disp_cur_mode;
	int is_plugged;
/***********************************************************************/

/*******parameters that framebuffer driver may change******************/	
	const struct fb_videomode *fb_cur_mode;
	int state;
	int power;
	int fb_blank;
/***************************************************************/
	
	struct mutex lock;
	struct display_ops *ops;
	struct device dev;

	struct notifier_block fb_notif;
	struct notifier_block cpufreq_notif;
	struct list_head entry;

};
#define to_owl_display_device(obj) container_of(obj, struct owl_display_device, dev)

/* update current mode for each device */
static inline int update_current_device_mode(struct owl_display_device *disp_dev)
{
	u32 i;
	const struct owl_videomode *pmode = disp_dev->modes;
	u32 vid = disp_dev->fb_cur_mode->vid;

	u32 mode_num = disp_dev->num_modes;
	for (i = 0; i < mode_num; i++, pmode++) {
		if (vid == pmode->mode.vid) {
			disp_dev->disp_cur_mode = &pmode->mode;
			return 0;
		}
	}
	disp_dev->disp_cur_mode = NULL;
	return -EINVAL;
}



static inline void owl_display_update_status(struct owl_display_device *disp_dev)
{
	mutex_lock(&disp_dev->lock);
	if (disp_dev->ops && disp_dev->ops->update_status){
		disp_dev->ops->update_status(disp_dev);
	}
	mutex_unlock(&disp_dev->lock);
}

static inline void owl_display_update_mode(struct owl_display_device *disp_dev)
{
	mutex_lock(&disp_dev->lock);
	if (disp_dev->ops && disp_dev->ops->update_mode){
		update_current_device_mode(disp_dev);
		disp_dev->ops->update_mode(disp_dev);
	}
	mutex_unlock(&disp_dev->lock);	
}

static inline void owl_display_set_preline(
	struct owl_display_device *disp_dev,
	int num_preline)
{
	mutex_lock(&disp_dev->lock);
	if (disp_dev->ops && disp_dev->ops->set_preline)
		disp_dev->ops->set_preline(disp_dev, num_preline);

	mutex_unlock(&disp_dev->lock);
}

static inline int owl_display_get_bpp(struct owl_display_device *disp_dev)
{
	int ret;
	mutex_lock(&disp_dev->lock);
	if (disp_dev->ops && disp_dev->ops->get_bpp)
		ret = disp_dev->ops->get_bpp(disp_dev);
	else
		ret = -EINVAL;
	mutex_unlock(&disp_dev->lock);

	return ret;
}

static inline unsigned int owl_display_get_devclk_khz(struct owl_display_device *disp_dev, 
											int src_clk_hz,
											const struct fb_videomode *mode)
{
	unsigned int devclk_khz = 0;
//	mutex_lock(&disp_dev->lock);
	if (disp_dev->ops && disp_dev->ops->get_devclk_khz)
		devclk_khz = disp_dev->ops->get_devclk_khz(disp_dev, src_clk_hz, mode);
//	mutex_unlock(&disp_dev->lock);	
	return devclk_khz;
}

static inline unsigned int owl_display_get_tvoutpll1_khz(struct owl_display_device *disp_dev, 
											const struct fb_videomode *mode)
{
	unsigned int tvoutpll1_khz = 0;
//	mutex_lock(&disp_dev->lock);
	if (disp_dev->ops && disp_dev->ops->get_tvoutpll1_khz)
		tvoutpll1_khz = disp_dev->ops->get_tvoutpll1_khz(disp_dev, mode);
//	mutex_unlock(&disp_dev->lock);	
	return tvoutpll1_khz;
}

extern int owl_display_register_client(struct notifier_block *nb);
extern int owl_display_unregister_client(struct notifier_block *nb);
extern int owl_display_notifier_call_chain(unsigned long val, void *v);

enum owl_display_notifiy_val{
	OWL_DISPLAY_NOTIF_PLUG			= 0,
	OWL_DISPLAY_NOTIF_REGISTER		= 1,
};


extern struct owl_display_device *get_owl_display(int display_id);
extern struct owl_display_device *owl_display_connect(int display_id);
extern void owl_display_disconnect(struct owl_display_device* disp_dev);
extern int owl_display_check_display_devices(int *plugged_disp_dev_ids, int *registered_disp_dev_ids);
extern struct owl_display_device *owl_display_device_register(const char *name,
	struct device *dev,
	void *devdata,
	int display_id,
	const struct owl_videomode *modes,
	u32 num_modes,
	struct display_ops *ops);
extern void owl_display_device_unregister(struct owl_display_device *dev);
extern int owl_display_device_store_modes(struct owl_display_device * dev,
	const struct owl_videomode *modes,
	u32 num_modes);
extern void owl_videomode_to_modelist(const struct owl_videomode *modedb, int num,
			      struct list_head *head);

/****lcd interface convertion ic************/
enum {
	LCDIC_TYPE_LVDS2EDP = 0,
	LCDIC_TYPE_MAX,
};

struct lcdi_convertion;

struct lcdi_convertion_ops {
	int (*enable)(struct lcdi_convertion *);	
	int (*disable)(struct lcdi_convertion *);
};

struct lcdi_convertion {
	struct device dev;

	int type;
	struct lcdi_convertion_ops *ops;
};

extern struct lcdi_convertion *lcdi_convertion_register(const char *name,
				struct device *dev,
				void *devdata,
				int type,
				struct lcdi_convertion_ops *ops);

extern void lcdi_convertion_free(struct lcdi_convertion *lcdic);
extern struct lcdi_convertion *lcdi_convertion_get(int type);

static inline int lcdi_convertion_enable(struct lcdi_convertion *lcdic)
{
	int ret = 0;
	if (lcdic && lcdic->ops->enable)
		ret = lcdic->ops->enable(lcdic);

	return ret;
}

static inline int lcdi_convertion_disable(struct lcdi_convertion *lcdic)
{
	int ret = 0;
	if (lcdic && lcdic->ops->disable)
		ret = lcdic->ops->disable(lcdic);

	return ret;
}

#endif
