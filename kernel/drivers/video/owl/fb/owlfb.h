/*
 * linux/drivers/video/owl/owlfb.h
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: Hui Wang  <wanghui@actions-semi.com>
 *
 * Some code and ideas taken from drivers/video/owl/ driver
 * by leopard.
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

#ifndef __DRIVERS_VIDEO_OWL2_OWLFB_H__
#define __DRIVERS_VIDEO_OWL2_OWLFB_H__

#ifdef CONFIG_FB_OWL2_DEBUG_SUPPORT
#define DEBUG
#endif

#include <linux/rwsem.h>

#include <video/owldss.h>

#define DEBUGx

#ifdef DEBUG
extern bool owlfb_debug;
#define DBG(format, ...) \
	do { \
		if (owlfb_debug) \
			printk(KERN_ERR "OWLFB: " format, ## __VA_ARGS__); \
	} while (0)
#else
#define DBG(format, ...)
#endif

#define FB2OFB(fb_info) ((struct owlfb_info *)(fb_info->par))

/* max number of overlays to which a framebuffer data can be direct */
#define OWLFB_NUM_FBS        2
#define OWLFB_MAX_OVL_PER_FB 3
#define OWLFB_NUM_BUFFERS_PER_FB 2

#define OWLFB_BUFFERS_MAX_XRES 1920
#define OWLFB_BUFFERS_MAX_YRES 1080

#define OWLFB_MAX_OVL_MEM_RESERVE_PER_OVL  (2048 * 2048 * 3 / 2)
#define OWLFB_MAX_OVL_MEM_RESERVE_NUM  1
#define OWLFB_MAX_CURSOR_MEM_RESERVE  (64 * 64 * 4)
#define OWLFB_CURSOR_OVL_ID 3
struct owlfb_mem_region {
	int             id;
	u32		paddr;
	void __iomem	*vaddr;

	unsigned long	size;
	u8		type;		/* OWLFB_PLANE_MEM_* */
	bool		alloc;		/* allocated by the driver */
	bool		map;		/* kernel mapped by the driver */
	atomic_t	map_count;
	struct rw_semaphore lock;
	atomic_t	lock_count;
};

/* appended to fb_info */
struct owlfb_info {
	int id;
	struct owlfb_mem_region *region;
	u32 overlay_mem_base;
	u32 overlay_free_mem_off;
	u32 overlay_free_mem_size;	
	int num_overlays;
	int used_overlay_mask;
	struct owl_overlay_manager * manager;
	struct owl_overlay *overlays[OWLFB_MAX_OVL_PER_FB];
	struct owlfb_device *fbdev;
	u8 rotation[OWLFB_MAX_OVL_PER_FB];
	bool mirror_to_hdmi;
};

struct owlfb_display_data {
	struct owlfb_device *fbdev;
	struct owl_dss_device *dssdev;
	u8 bpp_override;
	bool connected;
};

struct owlfb_device {
	struct device *dev;
	struct mutex  mtx;
	int mirror_fb_id;

	u32 pseudo_palette[17];

	int state;

	unsigned num_fbs;
	struct fb_info *fbs[10];
	struct owlfb_mem_region regions[10];

	unsigned num_displays;
	struct owlfb_display_data displays[10];
	unsigned num_overlays;
	struct owl_overlay *overlays[10];
	unsigned num_managers;
	struct owl_overlay_manager *managers[10];

	struct workqueue_struct *auto_update_wq;
	
	struct owl_dss_device *def_display;
	
	int xres;
	int yres;
	int bpp;
	int refresh;
	
};

struct owlfb_colormode {
	enum owl_color_mode dssmode;
	u32 bits_per_pixel;
	u32 nonstd;
	struct fb_bitfield red;
	struct fb_bitfield green;
	struct fb_bitfield blue;
	struct fb_bitfield transp;
};

void set_fb_fix(struct fb_info *fbi);
int check_fb_var(struct fb_info *fbi, struct fb_var_screeninfo *var);
int owlfb_realloc_fbmem(struct fb_info *fbi, unsigned long size, int type);
int owlfb_apply_changes(struct fb_info *fbi, int init);

int owlfb_create_sysfs(struct owlfb_device *fbdev);
void owlfb_remove_sysfs(struct owlfb_device *fbdev);

int owlfb_ioctl(struct fb_info *fbi, unsigned int cmd, unsigned long arg);

int owlfb_update_window(struct fb_info *fbi,
		u32 x, u32 y, u32 w, u32 h);

int dss_mode_to_fb_mode(enum owl_color_mode dssmode,
			struct fb_var_screeninfo *var);

int owlfb_setup_overlay(struct fb_info *fbi, struct owl_overlay *ovl,
		u16 posx, u16 posy, u16 outw, u16 outh);

void owlfb_start_auto_update(struct owlfb_device *fbdev,
		struct owl_dss_device *display);
void owlfb_stop_auto_update(struct owlfb_device *fbdev,
		struct owl_dss_device *display);
int owlfb_get_update_mode(struct fb_info *fbi, enum owlfb_update_mode *mode);
int owlfb_set_update_mode(struct fb_info *fbi, enum owlfb_update_mode mode);
int owlfb_free_all_fbmem_after_dc(struct owlfb_device *fbdev);
int owlfb_dc_init(struct owlfb_device *fbdev);

/* find the display connected to this fb, if any */
static inline struct owl_dss_device *fb2display(struct fb_info *fbi)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	int i;

	/* XXX: returns the display connected to first attached overlay */
	for (i = 0; i < ofbi->num_overlays; i++) {
		if (ofbi->overlays[i]->manager)
			return ofbi->overlays[i]->manager->device;
	}

	return NULL;
}

static inline struct owlfb_display_data *get_display_data(
		struct owlfb_device *fbdev, struct owl_dss_device *dssdev)
{
	int i;

	for (i = 0; i < fbdev->num_displays; ++i)
		if (fbdev->displays[i].dssdev == dssdev)
			return &fbdev->displays[i];

	/* This should never happen */
	BUG();
}

static inline void owlfb_lock(struct owlfb_device *fbdev)
{
	mutex_lock(&fbdev->mtx);
}

static inline void owlfb_unlock(struct owlfb_device *fbdev)
{
	mutex_unlock(&fbdev->mtx);
}

static inline int owlfb_overlay_enable(struct owl_overlay *ovl,
		int enable)
{
	if (enable)
		return ovl->enable(ovl);
	else
		return ovl->disable(ovl);
}

static inline struct owlfb_mem_region *
owlfb_get_mem_region(struct owlfb_mem_region *rg)
{
	down_read_nested(&rg->lock, rg->id);
	atomic_inc(&rg->lock_count);
	return rg;
}

static inline void owlfb_put_mem_region(struct owlfb_mem_region *rg)
{
	atomic_dec(&rg->lock_count);
	up_read(&rg->lock);
}

#endif
