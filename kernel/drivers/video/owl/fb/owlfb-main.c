/*
 * linux/drivers/video/owl/owlfb-main.c
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

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <mach/dss_user-owl.h>
#include <linux/of_device.h>
#include <video/owldss.h>
#include <video/owlfb.h>
#include <mach/bootdev.h>
#include "owlfb.h"

#define MODULE_NAME     "owlfb"

#define OWLFB_PLANE_XRES_MIN		8
#define OWLFB_PLANE_YRES_MIN		8

#ifdef DEBUG
bool owlfb_debug = true;
module_param_named(debug, owlfb_debug, bool, 0644);
static bool owlfb_test_pattern = true;
module_param_named(test, owlfb_test_pattern, bool, 0644);
#endif

static int owlfb_fb_init(struct owlfb_device *fbdev, struct fb_info *fbi);
static int owlfb_get_recommended_bpp(struct owlfb_device *fbdev,
		struct owl_dss_device *dssdev);

#ifdef DEBUG
static void draw_pixel(struct fb_info *fbi, int x, int y, unsigned color)
{
	struct fb_var_screeninfo *var = &fbi->var;
	struct fb_fix_screeninfo *fix = &fbi->fix;
	void __iomem *addr = fbi->screen_base;
	const unsigned bytespp = var->bits_per_pixel >> 3;
	const unsigned line_len = fix->line_length / bytespp;

	int r = (color >> 16) & 0xff;
	int g = (color >> 8) & 0xff;
	int b = (color >> 0) & 0xff;

	if (var->bits_per_pixel == 16) {
		u16 __iomem *p = (u16 __iomem *)addr;
		p += y * line_len + x;

		r = r * 32 / 256;
		g = g * 64 / 256;
		b = b * 32 / 256;

		__raw_writew((r << 11) | (g << 5) | (b << 0), p);
	} else if (var->bits_per_pixel == 24) {
		u8 __iomem *p = (u8 __iomem *)addr;
		p += (y * line_len + x) * 3;

		__raw_writeb(b, p + 0);
		__raw_writeb(g, p + 1);
		__raw_writeb(r, p + 2);
	} else if (var->bits_per_pixel == 32) {
		u32 __iomem *p = (u32 __iomem *)addr;
		p += y * line_len + x;
		__raw_writel(color, p);
	}
}

static void fill_fb(struct fb_info *fbi)
{
	struct fb_var_screeninfo *var = &fbi->var;
	const short w = var->xres_virtual;
	const short h = var->yres_virtual;
	void __iomem *addr = fbi->screen_base;
	int y, x;

	if (!addr)
		return;

	DBG("fill_fb %dx%d, line_len %d bytes\n", w, h, fbi->fix.line_length);

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			if (x < 20 && y < 20)
				draw_pixel(fbi, x, y, 0xffffff);
			else if (x < 20 && (y > 20 && y < h - 20))
				draw_pixel(fbi, x, y, 0xff);
			else if (y < 20 && (x > 20 && x < w - 20))
				draw_pixel(fbi, x, y, 0xff00);
			else if (x > w - 20 && (y > 20 && y < h - 20))
				draw_pixel(fbi, x, y, 0xff0000);
			else if (y > h - 20 && (x > 20 && x < w - 20))
				draw_pixel(fbi, x, y, 0xffff00);
			else if (x == 20 || x == w - 20 ||
					y == 20 || y == h - 20)
				draw_pixel(fbi, x, y, 0xffffff);
			else if (x == y || w - x == h - y)
				draw_pixel(fbi, x, y, 0xff00ff);
			else if (w - x == y || x == h - y)
				draw_pixel(fbi, x, y, 0x00ffff);
			else if (x > 20 && y > 20 && x < w - 20 && y < h - 20) {
				int t = x * 3 / w;
				unsigned r = 0, g = 0, b = 0;
				unsigned c;
				if (var->bits_per_pixel == 16) {
					if (t == 0)
						b = (y % 32) * 256 / 32;
					else if (t == 1)
						g = (y % 64) * 256 / 64;
					else if (t == 2)
						r = (y % 32) * 256 / 32;
				} else {
					if (t == 0)
						b = (y % 256);
					else if (t == 1)
						g = (y % 256);
					else if (t == 2)
						r = (y % 256);
				}
				c = (r << 16) | (g << 8) | (b << 0);
				draw_pixel(fbi, x, y, c);
			} else {
				draw_pixel(fbi, x, y, 0);
			}
		}
	}
}
#endif

static u32 owlfb_get_region_paddr(const struct owlfb_info *ofbi)
{
	return ofbi->region->paddr;
}

static void __iomem *owlfb_get_region_vaddr(const struct owlfb_info *ofbi)
{
	return ofbi->region->vaddr;
}

static struct owlfb_colormode owlfb_colormodes[] = {
   {
		.dssmode = OWL_DSS_COLOR_ARGB16,
		.bits_per_pixel = 16,
		.red	= { .length = 4, .offset = 8, .msb_right = 0 },
		.green	= { .length = 4, .offset = 4, .msb_right = 0 },
		.blue	= { .length = 4, .offset = 0, .msb_right = 0 },
		.transp	= { .length = 4, .offset = 12, .msb_right = 0 },
	}, {
		.dssmode = OWL_DSS_COLOR_RGB16,
		.bits_per_pixel = 16,
		.red	= { .length = 5, .offset = 11, .msb_right = 0 },
		.green	= { .length = 6, .offset = 5, .msb_right = 0 },
		.blue	= { .length = 5, .offset = 0, .msb_right = 0 },
		.transp	= { .length = 0, .offset = 0, .msb_right = 0 },
	}, {
		.dssmode = OWL_DSS_COLOR_RGB24P,
		.bits_per_pixel = 24,
		.red	= { .length = 8, .offset = 16, .msb_right = 0 },
		.green	= { .length = 8, .offset = 8, .msb_right = 0 },
		.blue	= { .length = 8, .offset = 0, .msb_right = 0 },
		.transp	= { .length = 0, .offset = 0, .msb_right = 0 },
	}, {
		.dssmode = OWL_DSS_COLOR_RGB24U,
		.bits_per_pixel = 32,
		.red	= { .length = 8, .offset = 16, .msb_right = 0 },
		.green	= { .length = 8, .offset = 8, .msb_right = 0 },
		.blue	= { .length = 8, .offset = 0, .msb_right = 0 },
		.transp	= { .length = 0, .offset = 0, .msb_right = 0 },
	}, {
		.dssmode = OWL_DSS_COLOR_ARGB32,
		.bits_per_pixel = 32,
		.red	= { .length = 8, .offset = 16, .msb_right = 0 },
		.green	= { .length = 8, .offset = 8, .msb_right = 0 },
		.blue	= { .length = 8, .offset = 0, .msb_right = 0 },
		.transp	= { .length = 8, .offset = 24, .msb_right = 0 },
	}, {
		.dssmode = OWL_DSS_COLOR_RGBA32,
		.bits_per_pixel = 32,
		.red	= { .length = 8, .offset = 24, .msb_right = 0 },
		.green	= { .length = 8, .offset = 16, .msb_right = 0 },
		.blue	= { .length = 8, .offset = 8, .msb_right = 0 },
		.transp	= { .length = 8, .offset = 0, .msb_right = 0 },
	}, {
		.dssmode = OWL_DSS_COLOR_RGBX32,
		.bits_per_pixel = 32,
		.red	= { .length = 8, .offset = 24, .msb_right = 0 },
		.green	= { .length = 8, .offset = 16, .msb_right = 0 },
		.blue	= { .length = 8, .offset = 8, .msb_right = 0 },
		.transp	= { .length = 0, .offset = 0, .msb_right = 0 },
	}, {
		.dssmode = OWL_DSS_COLOR_XBGR32,
		.bits_per_pixel = 32,
		.red	= { .length = 8, .offset = 0, .msb_right = 0 },
		.green	= { .length = 8, .offset = 8, .msb_right = 0 },
		.blue	= { .length = 8, .offset = 16, .msb_right = 0 },
		.transp	= { .length = 0, .offset = 24, .msb_right = 0 },
	}, {
		.dssmode = OWL_DSS_COLOR_XRGB32,
		.bits_per_pixel = 32,
		.red	= { .length = 8, .offset = 16, .msb_right = 0 },
		.green	= { .length = 8, .offset = 8, .msb_right = 0 },
		.blue	= { .length = 8, .offset = 0, .msb_right = 0 },
		.transp	= { .length = 0, .offset = 24, .msb_right = 0 },
	},
};

static bool cmp_var_to_colormode(struct fb_var_screeninfo *var,
		struct owlfb_colormode *color)
{
	bool cmp_component(struct fb_bitfield *f1, struct fb_bitfield *f2)
	{
		return f1->length == f2->length &&
			f1->offset == f2->offset &&
			f1->msb_right == f2->msb_right;
	}

	if (var->bits_per_pixel == 0 ||
			var->red.length == 0 ||
			var->blue.length == 0 ||
			var->green.length == 0)
		return 0;

	return var->bits_per_pixel == color->bits_per_pixel &&
		cmp_component(&var->red, &color->red) &&
		cmp_component(&var->green, &color->green) &&
		cmp_component(&var->blue, &color->blue) &&
		cmp_component(&var->transp, &color->transp);
}

static void assign_colormode_to_var(struct fb_var_screeninfo *var,
		struct owlfb_colormode *color)
{
	var->bits_per_pixel = color->bits_per_pixel;
	var->nonstd = color->nonstd;
	var->red = color->red;
	var->green = color->green;
	var->blue = color->blue;
	var->transp = color->transp;
}

static int fb_mode_to_dss_mode(struct fb_var_screeninfo *var,
		enum owl_color_mode *mode)
{
	enum owl_color_mode dssmode;
	int i;

	/* first match with nonstd field */
	if (var->nonstd) {
		for (i = 0; i < ARRAY_SIZE(owlfb_colormodes); ++i) {
			struct owlfb_colormode *m = &owlfb_colormodes[i];
			if (var->nonstd == m->nonstd) {
				assign_colormode_to_var(var, m);
				*mode = m->dssmode;
				return 0;
			}
		}

		return -EINVAL;
	}

	/* then try exact match of bpp and colors */
	for (i = 0; i < ARRAY_SIZE(owlfb_colormodes); ++i) {
		struct owlfb_colormode *m = &owlfb_colormodes[i];
		if (cmp_var_to_colormode(var, m)) {
			assign_colormode_to_var(var, m);
			*mode = m->dssmode;
			return 0;
		}
	}

	/* match with bpp if user has not filled color fields
	 * properly */
	switch (var->bits_per_pixel) {
	case 16:
		dssmode = OWL_DSS_COLOR_RGB16;
		break;
	case 24:
		dssmode = OWL_DSS_COLOR_RGB24P;
		break;
	case 32:
		/* 
		 * android minui only support RGBX(which is XBGR in DE),
		 * so we set FB's default color mode to XBGR,
		 * pls see "android/bootable/recovery/minui/graphics_fbdev.c"
		 */
#ifdef CONFIG_FB_COLOR_MODE_XRGB32
		dssmode = OWL_DSS_COLOR_XRGB32;
#else
		dssmode = OWL_DSS_COLOR_XBGR32;
#endif
		break;
	default:
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(owlfb_colormodes); ++i) {
		struct owlfb_colormode *m = &owlfb_colormodes[i];
		if (dssmode == m->dssmode) {
			assign_colormode_to_var(var, m);
			*mode = m->dssmode;
			return 0;
		}
	}

	return -EINVAL;
}

static int check_fb_res_bounds(struct fb_var_screeninfo *var)
{
	int xres_min = OWLFB_PLANE_XRES_MIN;
	int xres_max = 2048;
	int yres_min = OWLFB_PLANE_YRES_MIN;
	int yres_max = 2048;

	/* XXX: some applications seem to set virtual res to 0. */
	if (var->xres_virtual == 0)
		var->xres_virtual = var->xres;

	if (var->yres_virtual == 0)
		var->yres_virtual = var->yres;

	if (var->xres_virtual < xres_min || var->yres_virtual < yres_min)
		return -EINVAL;

	if (var->xres < xres_min)
		var->xres = xres_min;
	if (var->yres < yres_min)
		var->yres = yres_min;
	if (var->xres > xres_max)
		var->xres = xres_max;
	if (var->yres > yres_max)
		var->yres = yres_max;

	if (var->xres > var->xres_virtual)
		var->xres = var->xres_virtual;
	if (var->yres > var->yres_virtual)
		var->yres = var->yres_virtual;

	return 0;
}

static void shrink_height(unsigned long max_frame_size,
		struct fb_var_screeninfo *var)
{
	DBG("can't fit FB into memory, reducing y\n");
	var->yres_virtual = max_frame_size /
		(var->xres_virtual * var->bits_per_pixel >> 3);

	if (var->yres_virtual < OWLFB_PLANE_YRES_MIN)
		var->yres_virtual = OWLFB_PLANE_YRES_MIN;

	if (var->yres > var->yres_virtual)
		var->yres = var->yres_virtual;
}

static void shrink_width(unsigned long max_frame_size,
		struct fb_var_screeninfo *var)
{
	DBG("can't fit FB into memory, reducing x\n");
	var->xres_virtual = max_frame_size / var->yres_virtual /
		(var->bits_per_pixel >> 3);

	if (var->xres_virtual < OWLFB_PLANE_XRES_MIN)
		var->xres_virtual = OWLFB_PLANE_XRES_MIN;

	if (var->xres > var->xres_virtual)
		var->xres = var->xres_virtual;
}


static int check_fb_size(const struct owlfb_info *ofbi,
		struct fb_var_screeninfo *var)
{
	unsigned long max_frame_size = ofbi->region->size;
	int bytespp = var->bits_per_pixel >> 3;
	unsigned long line_size = var->xres_virtual * bytespp;

	DBG("max frame size %lu, line size %lu\n", max_frame_size, line_size);

	if (line_size * var->yres_virtual > max_frame_size)
		shrink_height(max_frame_size, var);

	if (line_size * var->yres_virtual > max_frame_size) {
		shrink_width(max_frame_size, var);
		line_size = var->xres_virtual * bytespp;
	}

	if (line_size * var->yres_virtual > max_frame_size) {
		DBG("cannot fit FB to memory\n");
		return -EINVAL;
	}

	return 0;
}

int dss_mode_to_fb_mode(enum owl_color_mode dssmode,
			struct fb_var_screeninfo *var)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(owlfb_colormodes); ++i) {
		struct owlfb_colormode *mode = &owlfb_colormodes[i];
		if (dssmode == mode->dssmode) {
			assign_colormode_to_var(var, mode);
			return 0;
		}
	}
	return -ENOENT;
}

void set_fb_fix(struct fb_info *fbi)
{
	struct fb_fix_screeninfo *fix = &fbi->fix;
	struct fb_var_screeninfo *var = &fbi->var;
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_mem_region *rg = ofbi->region;

	DBG("set_fb_fix\n");

	/* used by open/write in fbmem.c */
	fbi->screen_base = (char __iomem *)owlfb_get_region_vaddr(ofbi);

	/* used by mmap in fbmem.c */
	
	fix->line_length =
			(var->xres_virtual * var->bits_per_pixel) >> 3;
	fix->smem_len = rg->size;

	fix->smem_start = owlfb_get_region_paddr(ofbi);

	fix->type = FB_TYPE_PACKED_PIXELS;

	if (var->nonstd)
		fix->visual = FB_VISUAL_PSEUDOCOLOR;
	else {
		switch (var->bits_per_pixel) {
		case 32:
		case 24:
		case 16:
		case 12:
			fix->visual = FB_VISUAL_TRUECOLOR;
			/* 12bpp is stored in 16 bits */
			break;
		case 1:
		case 2:
		case 4:
		case 8:
			fix->visual = FB_VISUAL_PSEUDOCOLOR;
			break;
		}
	}

	fix->accel = FB_ACCEL_NONE;

	fix->xpanstep = 1;
	fix->ypanstep = 1;
}

/* check new var and possibly modify it to be ok */
int check_fb_var(struct fb_info *fbi, struct fb_var_screeninfo *var)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owl_dss_device *display = fb2display(fbi);
	enum owl_color_mode mode = 0;
	int i;
	int r;

	DBG("check_fb_var %d\n", ofbi->id);

	WARN_ON(!atomic_read(&ofbi->region->lock_count));

	r = fb_mode_to_dss_mode(var, &mode);
	if (r) {
		DBG("cannot convert var to owl dss mode\n");
		return r;
	}

	for (i = 0; i < ofbi->num_overlays; ++i) {
		if ((ofbi->overlays[i]->supported_modes & mode) == 0 && mode != 0) {
			DBG("invalid mode\n");
			return -EINVAL;
		}
	}

	if (var->rotate > 3)
		return -EINVAL;

	if (check_fb_res_bounds(var))
		return -EINVAL;

	/* When no memory is allocated ignore the size check */
	if (ofbi->region->size != 0 && check_fb_size(ofbi, var))
		return -EINVAL;

	if (var->xres + var->xoffset > var->xres_virtual)
		var->xoffset = var->xres_virtual - var->xres;
	if (var->yres + var->yoffset > var->yres_virtual)
		var->yoffset = var->yres_virtual - var->yres;

	DBG("xres = %d, yres = %d, vxres = %d, vyres = %d\n",
			var->xres, var->yres,
			var->xres_virtual, var->yres_virtual);

	if (display && display->driver->get_dimensions) {
		u32 w, h;
		display->driver->get_dimensions(display, &w, &h);
		var->width = DIV_ROUND_CLOSEST(w, 1000);
		var->height = DIV_ROUND_CLOSEST(h, 1000);
	} else {
		var->height = -1;
		var->width = -1;
	}

	var->grayscale          = 0;
 
	return 0;
}

/*
 * ---------------------------------------------------------------------------
 * fbdev framework callbacks
 * ---------------------------------------------------------------------------
 */
static int owlfb_open(struct fb_info *fbi, int user)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	if(fbdev->mirror_fb_id != 0 && ofbi->id != 0)
	{
		return -1;
	}	
	return 0;
}

static int owlfb_release(struct fb_info *fbi, int user)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	if(fbdev->mirror_fb_id != 0 && ofbi->id != 0)
	{
		return -1;
	}	
	return 0;
}

static unsigned calc_offset_dma(const struct fb_var_screeninfo *var,
		const struct fb_fix_screeninfo *fix)
{
	unsigned offset;

	offset = var->yoffset * fix->line_length +
		var->xoffset * (var->bits_per_pixel >> 3);

	return offset;
}


static void owlfb_calc_addr(const struct owlfb_info *ofbi,
			     const struct fb_var_screeninfo *var,
			     const struct fb_fix_screeninfo *fix, u32 *paddr)
{
	u32 data_start_p;
	int offset;

	data_start_p = owlfb_get_region_paddr(ofbi);

	offset = calc_offset_dma(var, fix);

	data_start_p += offset;

	if (offset)
		DBG("offset %d, %d = %d\n",
		    var->xoffset, var->yoffset, offset);

	DBG("paddr %x\n", data_start_p);

	*paddr = data_start_p;
}

/* setup overlay according to the fb */
int owlfb_setup_overlay(struct fb_info *fbi, struct owl_overlay *ovl,
		u16 posx, u16 posy, u16 outw, u16 outh)
{
	int r = 0;
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct fb_var_screeninfo *var = &fbi->var;
	struct fb_fix_screeninfo *fix = &fbi->fix;
	enum owl_color_mode mode = 0;
	u32 data_start_p = 0;
	struct owl_overlay_info info;
	int xres, yres;

	int rotation = var->rotate;
	int i;

	WARN_ON(!atomic_read(&ofbi->region->lock_count));

	DBG("setup_overlay %d, posx %d, posy %d, outw %d, outh %d\n", ofbi->id,
			posx, posy, outw, outh);

	xres = var->xres;
	yres = var->yres;

	if (ofbi->region->size)
		owlfb_calc_addr(ofbi, var, fix, &data_start_p);

	r = fb_mode_to_dss_mode(var, &mode);
	if (r) {
		DBG("fb_mode_to_dss_mode failed");
		goto err;
	}

	ovl->get_overlay_info(ovl, &info);

	info.paddr = data_start_p;
	info.screen_width = xres;	
	info.img_width = xres;
	info.img_height = yres;
	info.width = xres;
	info.height = yres;
	info.color_mode = mode;
	info.rotation = rotation;

	info.pos_x = posx;
	info.pos_y = posy;
	info.out_width = outw;
	info.out_height = outh;

	r = ovl->set_overlay_info(ovl, &info);
	if (r) {
		DBG("ovl->setup_overlay_info failed\n");
		goto err;
	}

	return 0;

err:
	DBG("setup_overlay failed\n");
	return r;
}

/* apply var to the overlay */
int owlfb_apply_changes(struct fb_info *fbi, int init)
{
	int r = 0;
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct fb_var_screeninfo *var = &fbi->var;
	struct owl_dss_device *dssdev = NULL;
	struct owl_overlay *ovl;
	u16 overscan_width, overscan_height;
	u16 posx, posy;
	u16 outw, outh;
	int i;

#ifdef DEBUG
	if (owlfb_test_pattern)
		fill_fb(fbi);
#endif

	if(fbdev->mirror_fb_id == 1 &&  ofbi->id == 1)
	{
		return 0;
	}

	WARN_ON(!atomic_read(&ofbi->region->lock_count));

	for (i = 0; i < ofbi->num_overlays; i++) {
		ovl = ofbi->overlays[i];

		DBG("apply_changes, fb %d, ovl %d var->xres %d ,var->yres %d \n", ofbi->id, ovl->id,var->xres,var->yres);

		if (ofbi->region->size == 0) {
			/* the fb is not available. disable the overlay */
			owlfb_overlay_enable(ovl, 0);
			if (!init && ovl->manager)
				ovl->manager->apply(ovl->manager);
			continue;
		}

		posx = 0;
		posy = 0;
		
		dssdev = ovl->manager->device;
		if(dssdev && dssdev->driver != NULL){
			dssdev->driver->get_resolution(dssdev, &outw, &outh);
		}
		
		if(outw == 0 || outh == 0)
		{
			outw = var->xres;
			outh = var->yres;
		}

		if(dssdev->driver->get_over_scan){			
			dssdev->driver->get_over_scan(dssdev, &overscan_width, &overscan_height);
			outw = outw - overscan_height * 2;
			outh = outh - overscan_height * 2;
			posx = overscan_width;
			posy = overscan_height;
		}

		DBG("apply_changes, init %d ,posx %d ,posy %d ,outw %d ,outh %d \n",init,posx,posy,outw,outh );
		r = owlfb_setup_overlay(fbi, ovl, posx, posy, outw, outh);
		
		if (r)
			goto err;

		if (!init && ovl->manager)
			ovl->manager->apply(ovl->manager);
		
		if(fbdev->mirror_fb_id == 1 && i == 0){			
			struct owl_overlay * dest_ovl;
			struct owl_overlay_info src_info;
			struct owl_overlay_info dest_info;			
			struct owl_cursor_info src_cursor;
			struct owl_cursor_info dest_cursor;
			dest_ovl = owl_dss_get_overlay(3);
			
			dest_ovl->get_overlay_info(dest_ovl, &dest_info);
			
			ovl->get_overlay_info(ovl,&src_info);

			dest_info.paddr = src_info.paddr;		
			dest_info.width = src_info.width;
			dest_info.height = src_info.height;
			dest_info.color_mode = src_info.color_mode;
			dest_info.rotation = src_info.rotation;
			dest_info.img_width = src_info.img_width;
			dest_info.img_height = src_info.img_height;
			dest_info.pos_x = 0;
			dest_info.pos_y = 0;
				
			dssdev = dest_ovl->manager->device;
			if(dssdev != NULL && dssdev->driver){
				if(dssdev->driver->get_resolution){
					dssdev->driver->get_resolution(dssdev, &outw, &outh);
				}
				
				if(dssdev->driver->get_over_scan){
					dssdev->driver->get_over_scan(dssdev, &overscan_width, &overscan_height);
					outw -= overscan_width * 2;
					outh -= overscan_height * 2;
					dest_info.pos_x = overscan_width;
					dest_info.pos_y = overscan_height;
				}
					
			}

			dest_info.out_width = outw;
			dest_info.out_height = outh;
			
			r = dest_ovl->set_overlay_info(dest_ovl, &dest_info);
			if (r)
				goto err;			
				
			if(!dest_ovl->is_enabled(dest_ovl)){	
				dest_ovl->enable(dest_ovl);
			}
			//printk(KERN_ERR "  ovl->manager->apply hdmi , outw %d outh %d\n",outw,outh);
			dest_ovl->manager->apply(dest_ovl->manager);
		}
	}
	return 0;
err:
	DBG("apply_changes failed\n");
	return r;
}

/* checks var and eventually tweaks it to something supported,
 * DO NOT MODIFY PAR */
static int owlfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fbi)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	int r;

	DBG("check_var(%d)\n", FB2OFB(fbi)->id);

	owlfb_get_mem_region(ofbi->region);

	r = check_fb_var(fbi, var);

	owlfb_put_mem_region(ofbi->region);

	return r;
}
static void fb_videomode_to_owl_timings(struct fb_videomode *m,
		struct owl_video_timings *t)
{
	t->x_res = m->xres;
	t->y_res = m->yres;
	t->pixel_clock = PICOS2KHZ(m->pixclock);
	t->hsw = m->hsync_len;
	t->hfp = m->right_margin;
	t->hbp = m->left_margin;
	t->vsw = m->vsync_len;
	t->vfp = m->lower_margin;
	t->vbp = m->upper_margin;
}

static int owlfb_set_display_mode(struct fb_info *fbi)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct fb_videomode mode;
	struct owl_video_timings t;
	struct owl_dss_device *display = fb2display(fbi);
	int r;
	
#ifdef CONFIG_FB_MAP_TO_DE	
	fb_var_to_videomode(&mode,&fbi->var);
	
	fb_videomode_to_owl_timings(&mode,&t);
	
	if(display->driver 
		&& display->driver->check_timings
		&& display->driver->set_timings
		&& display->driver->get_timings){
		if(!display->driver->check_timings(display,&t)){
			display->driver->disable(display);			
			display->driver->set_timings(display,&t);			
			msleep(500);			
			display->driver->enable(display);
		}
	}
#endif 	

	return r;
}

/* set the video mode according to info->var */
static int owlfb_set_par(struct fb_info *fbi)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	int r;

	printk("set_par(%d)\n", FB2OFB(fbi)->id);
	
	owlfb_set_display_mode(fbi);

	owlfb_get_mem_region(ofbi->region);

	set_fb_fix(fbi);

	r = owlfb_apply_changes(fbi, 0);

	owlfb_put_mem_region(ofbi->region);

	return r;
}

static int owlfb_pan_display(struct fb_var_screeninfo *var,
		struct fb_info *fbi)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct fb_var_screeninfo new_var;
	int r;

	DBG("pan_display(%d) var->xoffset %x ,var->yoffset %x fbi->var.xoffset %x ,fbi->var.yoffset%x \n", FB2OFB(fbi)->id,var->xoffset,var->yoffset,fbi->var.xoffset,fbi->var.yoffset);

	if (var->xoffset == fbi->var.xoffset &&
	    var->yoffset == fbi->var.yoffset)
		//return 0;

	new_var = fbi->var;
	new_var.xoffset = var->xoffset;
	new_var.yoffset = var->yoffset;

	fbi->var = new_var;

	owlfb_get_mem_region(ofbi->region);

	r = owlfb_apply_changes(fbi, 0);

	owlfb_put_mem_region(ofbi->region);

	return r;
}

static void mmap_user_open(struct vm_area_struct *vma)
{
	struct owlfb_mem_region *rg = vma->vm_private_data;

	owlfb_get_mem_region(rg);
	atomic_inc(&rg->map_count);
	owlfb_put_mem_region(rg);
}

static void mmap_user_close(struct vm_area_struct *vma)
{
	struct owlfb_mem_region *rg = vma->vm_private_data;

	owlfb_get_mem_region(rg);
	atomic_dec(&rg->map_count);
	owlfb_put_mem_region(rg);
}

static struct vm_operations_struct mmap_user_ops = {
	.open = mmap_user_open,
	.close = mmap_user_close,
};

static int owlfb_mmap(struct fb_info *fbi, struct vm_area_struct *vma)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct fb_fix_screeninfo *fix = &fbi->fix;
	struct owlfb_mem_region *rg;
	unsigned long off;
	unsigned long start;
	u32 len;
	int r = -EINVAL;
	struct owl_dss_device * dssdev = ofbi->manager->device;
	struct owl_overlay *ovl = ofbi->overlays[0];

	if (vma->vm_end - vma->vm_start == 0)
		return 0;
	if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
		return -EINVAL;
	off = vma->vm_pgoff << PAGE_SHIFT;

	rg = owlfb_get_mem_region(ofbi->region);

	start = owlfb_get_region_paddr(ofbi);
	len = fix->smem_len;
	if (off >= len)
		goto error;
	if ((vma->vm_end - vma->vm_start + off) > len)
		goto error;

	off += start;

	DBG("user mmap region start %lx, len %d, off %lx\n", start, len, off);

	vma->vm_pgoff = off >> PAGE_SHIFT;
	//vma->vm_flags |= VM_IO | VM_RESERVED;
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	vma->vm_ops = &mmap_user_ops;
	vma->vm_private_data = rg;
	if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
			       vma->vm_end - vma->vm_start,
			       vma->vm_page_prot)) {
		r = -EAGAIN;
		goto error;
	}

	/* vm_ops.open won't be called for mmap itself. */
	atomic_inc(&rg->map_count);


DBG("Apply overlay changes ofbi->num_overlays %d \n", ofbi->num_overlays);
	
#if CONFIG_FB_MAP_TO_DE
	if(ofbi->manager != NULL && ofbi->manager->device != NULL && ofbi->manager->device->driver != NULL)
	{
		
		if(dssdev->driver->get_vid != 0 && dssdev->driver->set_vid != 0)
		{
			int vid = 0;
			dssdev->driver->get_vid(dssdev,&vid);
			dssdev->driver->set_vid(dssdev,vid);
		}
		if(dssdev->driver->enable){
			dssdev->driver->enable(dssdev);	
		}		

	    r = owlfb_overlay_enable(ovl, 1);		
		
		r = owlfb_apply_changes(fbi, 0);
 	}
#endif

	owlfb_put_mem_region(rg);
	

	return 0;

 error:
	owlfb_put_mem_region(ofbi->region);

	return r;
}

/* Store a single color palette entry into a pseudo palette or the hardware
 * palette if one is available. For now we support only 16bpp and thus store
 * the entry only to the pseudo palette.
 */
static int _setcolreg(struct fb_info *fbi, u_int regno, u_int red, u_int green,
		u_int blue, u_int transp, int update_hw_pal)
{
	/*struct owlfb_info *ofbi = FB2OFB(fbi);*/
	/*struct owlfb_device *fbdev = ofbi->fbdev;*/
	struct fb_var_screeninfo *var = &fbi->var;
	int r = 0;

	enum owlfb_color_format mode = OWLFB_COLOR_RGB24U; /* XXX */

	/*switch (plane->color_mode) {*/
	switch (mode) {
	case OWLFB_COLOR_YUV420:
	case OWLFB_COLOR_YVU420:
	case OWLFB_COLOR_YUV420SP:
		r = -EINVAL;
		break;	
	case OWLFB_COLOR_RGB565:
	case OWLFB_COLOR_RGB444:
	case OWLFB_COLOR_RGB24P:
	case OWLFB_COLOR_RGB24U:
		if (r != 0)
			break;

		if (regno < 16) {
			u16 pal;
			pal = ((red >> (16 - var->red.length)) <<
					var->red.offset) |
				((green >> (16 - var->green.length)) <<
				 var->green.offset) |
				(blue >> (16 - var->blue.length));
			((u32 *)(fbi->pseudo_palette))[regno] = pal;
		}
		break;
	default:
		BUG();
	}
	return r;
}

static int owlfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		u_int transp, struct fb_info *info)
{
	DBG("setcolreg\n");

	return _setcolreg(info, regno, red, green, blue, transp, 1);
}

static int owlfb_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	int count, index, r;
	u16 *red, *green, *blue, *transp;
	u16 trans = 0xffff;

	DBG("setcmap\n");

	red     = cmap->red;
	green   = cmap->green;
	blue    = cmap->blue;
	transp  = cmap->transp;
	index   = cmap->start;

	for (count = 0; count < cmap->len; count++) {
		if (transp)
			trans = *transp++;
		r = _setcolreg(info, index++, *red++, *green++, *blue++, trans,
				count == cmap->len - 1);
		if (r != 0)
			return r;
	}

	return 0;
}

static int owlfb_blank(int blank, struct fb_info *fbi)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owl_dss_device *display = fb2display(fbi);
	struct owlfb_display_data *d;
	int r = 0;

	if (!display)
		return -EINVAL;
	
	DBG("owlfb_blank~~~~~~~~~~~~~~~blank %d \n",blank);
	owlfb_lock(fbdev);

	d = get_display_data(fbdev, display);

	switch (blank) {
	case FB_BLANK_UNBLANK:
		if (display->state != OWL_DSS_DISPLAY_SUSPENDED)
			goto exit;

		if (display->driver->resume)
			r = display->driver->resume(display);

		break;

	case FB_BLANK_NORMAL:
		/* FB_BLANK_NORMAL could be implemented.
		 * Needs DSS additions. */
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_POWERDOWN:
		if (display->state != OWL_DSS_DISPLAY_ACTIVE)
			goto exit;

		if (display->driver->suspend)
			r = display->driver->suspend(display);

		break;

	default:
		r = -EINVAL;
	}

exit:
	owlfb_unlock(fbdev);

	return r;
}

static struct fb_ops owlfb_ops = {
	.owner          = THIS_MODULE,
	.fb_open        = owlfb_open,
	.fb_release     = owlfb_release,
#if 1
	.fb_fillrect    = cfb_fillrect,
	.fb_copyarea    = cfb_copyarea,
	.fb_imageblit   = cfb_imageblit,
#endif
	.fb_blank       = owlfb_blank,
	.fb_ioctl       = owlfb_ioctl,
	.fb_check_var   = owlfb_check_var,
	.fb_set_par     = owlfb_set_par,
	.fb_pan_display = owlfb_pan_display,
	.fb_mmap	= owlfb_mmap,
	.fb_setcolreg	= owlfb_setcolreg,
	.fb_setcmap	= owlfb_setcmap,
	/*.fb_write	= owlfb_write,*/
};

static void owlfb_free_fbmem(struct fb_info *fbi)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_mem_region *rg;

	rg = ofbi->region;

	WARN_ON(atomic_read(&rg->map_count));

	if (rg->paddr) {
        /* paddr free? TODO */
    }

	if (rg->vaddr)
		iounmap(rg->vaddr);

	rg->vaddr = NULL;
	rg->paddr = 0;
	rg->alloc = 0;
	rg->size = 0;
}

static void clear_fb_info(struct fb_info *fbi)
{
	memset(&fbi->var, 0, sizeof(fbi->var));
	memset(&fbi->fix, 0, sizeof(fbi->fix));
	strlcpy(fbi->fix.id, MODULE_NAME, sizeof(fbi->fix.id));
}

int owlfb_free_all_fbmem_after_dc(struct owlfb_device *fbdev)
{
	int i;   
	for (i = 0; i < fbdev->num_fbs; i++) {
		struct fb_info *fbi = fbdev->fbs[i];
		struct owlfb_info *ofbi = FB2OFB(fbi);
		struct owlfb_mem_region *rg;
		rg = ofbi->region;	
		if (rg->paddr) {
	        dma_free_coherent(NULL, rg->size, rg->vaddr, rg->paddr);
	        DBG("owlfb_free_all_fbmem_after_dc size 0x%x\n",rg->size);
	    }
	}
	return 0;
}

static int owlfb_free_all_fbmem(struct owlfb_device *fbdev)
{
	int i;

	DBG("free all fbmem\n");

	for (i = 0; i < fbdev->num_fbs; i++) {
		struct fb_info *fbi = fbdev->fbs[i];
		owlfb_free_fbmem(fbi);
		clear_fb_info(fbi);
	}

	return 0;
}

static int owlfb_alloc_fbmem(struct fb_info *fbi, unsigned long size)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_mem_region *rg;
	void __iomem *vaddr = NULL;
	unsigned long paddr;

	rg = ofbi->region;

	rg->paddr = 0;
	rg->vaddr = NULL;
	rg->size = 0;
	rg->type = 0;
	rg->alloc = false;
	rg->map = false;

	size = PAGE_ALIGN(size);

	DBG("allocating %lu bytes for fb %d\n", size, ofbi->id);
		
     /* really cast me? TODO */
	vaddr = dma_alloc_coherent(NULL, size,
                                   (dma_addr_t *)&paddr, GFP_KERNEL);
	
    if (!vaddr) {
        printk(KERN_ERR "fail to allocate fb mem (size: %ldK))\n", size / 1024);
        return -ENOMEM;
    }

	//SetPageReserved(pfn_to_page(((unsigned long) paddr) >> PAGE_SHIFT));

	DBG("allocated VRAM paddr %lx, vaddr %p\n", paddr, vaddr);

	rg->paddr = paddr;
	rg->vaddr = vaddr;
	rg->size = size;
	rg->alloc = 1;

	return 0;
}

/* allocate fbmem using display resolution as reference */
static int owlfb_alloc_fbmem_display(struct fb_info *fbi, unsigned long size)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owl_dss_device *display;
	int bytespp;

	display =  fb2display(fbi);

	if (!display)
		return 0;

	switch (owlfb_get_recommended_bpp(fbdev, display)) {
	case 16:
		bytespp = 2;
		break;
	case 24:
		bytespp = 4;
		break;
	default:
		bytespp = 4;
		break;
	}

	if (!size) {
		u16 w, h;

		display->driver->get_resolution(display, &w, &h);

		size = w * h * bytespp * OWLFB_NUM_BUFFERS_PER_FB;

	}

	if (!size)
		return 0;

	return owlfb_alloc_fbmem(fbi, size);
}
static int owlfb_allocate_all_fbs(struct owlfb_device *fbdev)
{
	int i, r;
	unsigned long size = 0;

	for (i = 0; i < fbdev->num_fbs; i++) {
		/* allocate memory automatically only for fb0, or if
		 * excplicitly defined with vram or plat data option */
		size = 0;
		if(i == 0){
			size = OWLFB_BUFFERS_MAX_XRES * OWLFB_BUFFERS_MAX_YRES * fbdev->bpp * OWLFB_NUM_BUFFERS_PER_FB;
#if CONFIG_FB_MAP_TO_DE	
			size += OWLFB_MAX_OVL_MEM_RESERVE_PER_OVL * OWLFB_MAX_OVL_MEM_RESERVE_NUM + OWLFB_MAX_CURSOR_MEM_RESERVE;		
#endif 
		}
		DBG("owlfb_allocate_all_fbs allocating %lu bytes for fb %d\n", size,i);
		r = owlfb_alloc_fbmem_display(fbdev->fbs[i],size);
		if (r)
			return r;		
	}

	for (i = 0; i < fbdev->num_fbs; i++) {
		struct owlfb_info *ofbi = FB2OFB(fbdev->fbs[i]);
		struct owlfb_mem_region *rg;
		rg = ofbi->region;
		
		if(i == 0){
			ofbi->overlay_free_mem_size = OWLFB_MAX_OVL_MEM_RESERVE_PER_OVL * OWLFB_MAX_OVL_MEM_RESERVE_NUM;
			ofbi->overlay_free_mem_off = 0;
			ofbi->overlay_mem_base = fbdev->xres * fbdev->yres * fbdev->bpp * OWLFB_NUM_BUFFERS_PER_FB;
			
			DBG("ofbi->overlay_free_mem_size 0x%x ofbi->overlay_mem_base 0x%x \n",ofbi->overlay_free_mem_size,ofbi->overlay_mem_base);
		}
		ofbi->used_overlay_mask |= 0x01;

		DBG("region%d phys %08x virt %p size=%lu\n",
				i,
				rg->paddr,
				rg->vaddr,
				rg->size);
	}

	return 0;
}

int owlfb_realloc_fbmem(struct fb_info *fbi, unsigned long size, int type)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owlfb_mem_region *rg = ofbi->region;
	unsigned long old_size = rg->size;
	unsigned long old_paddr = rg->paddr;
	int old_type = rg->type;
	int r;

	if (type > OWLFB_MEMTYPE_MAX)
		return -EINVAL;

	size = PAGE_ALIGN(size);

	if (old_size == size && old_type == type)
		return 0;

	owlfb_free_fbmem(fbi);

	if (size == 0) {
		clear_fb_info(fbi);
		return 0;
	}

	r = owlfb_alloc_fbmem(fbi, size);

	if (r) {
		if (old_size)
			owlfb_alloc_fbmem(fbi, old_size);

		if (rg->size == 0)
			clear_fb_info(fbi);

		return r;
	}

	if (old_size == size)
		return 0;

	if (old_size == 0) {
		DBG("initializing fb %d\n", ofbi->id);
		r = owlfb_fb_init(fbdev, fbi);
		if (r) {
			DBG("owlfb_fb_init failed\n");
			goto err;
		}
		r = owlfb_apply_changes(fbi, 1);
		if (r) {
			DBG("owlfb_apply_changes failed\n");
			goto err;
		}
	} else {
		struct fb_var_screeninfo new_var;
		memcpy(&new_var, &fbi->var, sizeof(new_var));
		r = check_fb_var(fbi, &new_var);
		if (r)
			goto err;
		memcpy(&fbi->var, &new_var, sizeof(fbi->var));
		set_fb_fix(fbi);
		
	}

	return 0;
err:
	owlfb_free_fbmem(fbi);
	clear_fb_info(fbi);
	return r;
}

struct fb_videomode cvbs_mode_list[2] ={
	/* 720x576i @ 50 Hz, 15.625 kHz hsync (PAL RGB) */
	[0] = {
		NULL, 50, 720, 576, 74074, 64, 16, 39, 5, 64, 5, 0,
		FB_VMODE_INTERLACED
	},
	/* #3: 720x480i@59.94/60Hz */
	[1] = {
		NULL, 60, 720, 480, 37037, 60, 16, 30, 9, 62, 6, 0,
		FB_VMODE_INTERLACED, 0,
	},
};
static int owlfb_build_modelist(struct fb_info *fbi,struct owl_dss_device *display)
{
	struct fb_monspecs *specs;
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	u8 *edid;
	int r, i, len;	
	if (!display->driver->read_edid){
		if(display->type == OWL_DISPLAY_TYPE_CVBS){
			for(i = 0 ; i < 2 ; i++){
				fb_add_videomode(&cvbs_mode_list[i],&fbi->modelist);
			}
			return 0;			
		}else{
			return -ENODEV;
		}		
	}
	
	len = 0x80 * 2;
	edid = kmalloc(len, GFP_KERNEL);

	r = display->driver->read_edid(display, edid, len);
	if (r < 0){
		printk("read_edid r %d \n",r);
		goto err1;
	}
	
	fb_destroy_modelist(&fbi->modelist);
	
	memset(&fbi->monspecs, 0, sizeof(fbi->monspecs));

	specs = kzalloc(sizeof(*specs), GFP_KERNEL);

	fb_edid_to_monspecs(edid, specs);

	if (edid[126] > 0)
		fb_edid_add_monspecs(edid + 0x80, specs);

	DBG("specs->modedb_len %d \n",specs->modedb_len);
	for (i = 0; i < specs->modedb_len; ++i) {
		struct fb_videomode *m;
		struct owl_video_timings t;

		m = &specs->modedb[i];

		if (m->pixclock == 0)
			continue;
			
		if (m->vmode & FB_VMODE_INTERLACED ||
				m->vmode & FB_VMODE_DOUBLE)
			continue;

		fb_videomode_to_owl_timings(m, &t);		
					
		r = display->driver->check_timings(display, &t);
		if (r == 0 ) {
			fb_add_videomode(m,	&fbi->modelist);
		}
	}
	r = 0;

err2:
	fb_destroy_modedb(specs->modedb);
	kfree(specs);
err1:
	kfree(edid);

	return r;
}

/* initialize fb_info, var, fix to something sane based on the display */
static int owlfb_fb_init(struct owlfb_device *fbdev, struct fb_info *fbi)
{
	struct fb_var_screeninfo *var = &fbi->var;
	struct owl_dss_device *display = fb2display(fbi);
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct fb_videomode * best_mode = NULL;
	int r = 0;

	fbi->fbops = &owlfb_ops;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->pseudo_palette = fbdev->pseudo_palette;

	if (ofbi->region->size == 0) {
		clear_fb_info(fbi);
		return 0;
	}

	var->nonstd = 0;
	var->bits_per_pixel = 0;
	
	
	switch (owlfb_get_recommended_bpp(fbdev, display)) {
	case 16:
		var->bits_per_pixel = 16;
		break;
	case 24:
		var->bits_per_pixel = 32;
		break;
	default:
		var->bits_per_pixel = 32;
		break;
	}
	
	fbdev->bpp = var->bits_per_pixel / 8;
	

	if(owlfb_build_modelist(fbi,display) == 0)
	{
		best_mode =  fb_find_best_display(&fbi->monspecs, &fbi->modelist);
		best_mode = NULL;
				
	}	
	if(best_mode != NULL){		
		
		fb_videomode_to_var(var, best_mode);
				
	}else{
		/*is primary fb or display is null ,we used fbdev res as ofbi res */
		if(display != NULL && display->driver != NULL && display->driver->get_timings)
		{
			struct owl_video_timings timings;
			display->driver->get_timings(display,&timings);			
			var->xres = timings.x_res;
			var->yres = timings.y_res;
			var->xres_virtual = timings.x_res;
			var->yres_virtual = timings.y_res;
			var->xoffset = 0;
			var->yoffset = 0;
			var->pixclock = KHZ2PICOS(timings.pixel_clock);
			var->left_margin = timings.hfp;
			var->right_margin = timings.hbp;
			var->upper_margin = timings.vfp;
			var->lower_margin = timings.vbp;
			var->hsync_len = timings.hsw;
			var->vsync_len = timings.vsw;			
		}else{
			var->xres = fbdev->xres;
			var->yres = fbdev->yres;
			var->xres_virtual = var->xres;
			var->yres_virtual = var->yres;
			var->bits_per_pixel = fbdev->bpp * 8;	
		}	
	}	
	
	r = check_fb_var(fbi, var);
	
	if (r)
		goto err;

	set_fb_fix(fbi);

	r = fb_alloc_cmap(&fbi->cmap, 256, 0);
	if (r)
		dev_err(fbdev->dev, "unable to allocate color map memory\n");

	DBG("obfi %d  var:xres %d ,yres %d ,bit_per_pixel %d pixclock %d \n",ofbi->id,var->xres,var->yres,var->bits_per_pixel,var->pixclock);	
err:
	return r;
}

static void owlfb_hotplug_notify(struct owl_dss_device *dssdev, int state)
{
	
	struct owl_overlay_manager *external_mgr = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_EXTERNAL);
	int vid ;		 
	printk("owlfb_hotplug_notify %d ~~~~~~ \n",state);	
	if(state == 1){
		dssdev->driver->disable(dssdev);
		dssdev->driver->get_vid(dssdev,&vid);
		dssdev->driver->set_vid(dssdev,vid);
		dssdev->driver->enable(dssdev);
			
	}else{
		dssdev->driver->disable(dssdev);
	}	
#ifdef CONFIG_FB_MAP_TO_DE
	printk("owlfb_hotplug_notify owlfb_apply_changes \n");
	if(external_mgr != NULL && external_mgr->link_fbi != NULL){	
		if(state)
			{
				struct owlfb_info *ofbi = FB2OFB(external_mgr->link_fbi);
				struct owlfb_device *fbdev = ofbi->fbdev;
				struct fb_info *fbi = fbdev->fbs[0];
				
				if(external_mgr->device->type == dssdev->type)
				{
					fbi=fbdev->fbs[1];
					printk("external_mgr is fb1\n");
				}
				owlfb_build_modelist(fbi,dssdev);	
			}	
		
			owlfb_get_mem_region(FB2OFB(external_mgr->link_fbi)->region);
			owlfb_apply_changes(external_mgr->link_fbi, 0);
			owlfb_put_mem_region(FB2OFB(external_mgr->link_fbi)->region);	
	}
#endif

}

static void owlfb_register_hotplug_notify(struct owl_dss_device *dssdev)
{
	if(dssdev && dssdev->driver){
		if(!dssdev->driver->hot_plug_nodify){
			dssdev->driver->hot_plug_nodify = owlfb_hotplug_notify;
			DBG("owlfb_register_hotplug_notify  dssdev %s  \n",dssdev->name);
		}
	}
}


static void fbinfo_cleanup(struct owlfb_device *fbdev, struct fb_info *fbi)
{
	fb_dealloc_cmap(&fbi->cmap);
}


static void owlfb_free_resources(struct owlfb_device *fbdev)
{
	int i;

	DBG("free_resources\n");

	if (fbdev == NULL)
		return;

	for (i = 0; i < fbdev->num_fbs; i++)
		unregister_framebuffer(fbdev->fbs[i]);

	/* free the reserved fbmem */
	owlfb_free_all_fbmem(fbdev);

	for (i = 0; i < fbdev->num_fbs; i++) {
		fbinfo_cleanup(fbdev, fbdev->fbs[i]);
		framebuffer_release(fbdev->fbs[i]);
	}

	for (i = 0; i < fbdev->num_displays; i++) {
		struct owl_dss_device *dssdev = fbdev->displays[i].dssdev;

		if (dssdev->state != OWL_DSS_DISPLAY_DISABLED)
			dssdev->driver->disable(dssdev);

		owl_dss_put_device(dssdev);
	}

	dev_set_drvdata(fbdev->dev, NULL);
	kfree(fbdev);
}

static int owlfb_create_framebuffers(struct owlfb_device *fbdev)
{
	int r, i ,k = 0 , j = 0;

	fbdev->num_fbs = 0;

	DBG("create %d framebuffers\n",	OWLFB_NUM_FBS);

	/* allocate fb_infos */
	for (i = 0; i < OWLFB_NUM_FBS; i++) {
		struct fb_info *fbi;
		struct owlfb_info *ofbi;

		fbi = framebuffer_alloc(sizeof(struct owlfb_info),
				fbdev->dev);

		if (fbi == NULL) {
			dev_err(fbdev->dev,
				"unable to allocate memory for plane info\n");
			return -ENOMEM;
		}

		clear_fb_info(fbi);

		fbdev->fbs[i] = fbi;

		ofbi = FB2OFB(fbi);
		ofbi->fbdev = fbdev;
		ofbi->id = i;

		ofbi->region = &fbdev->regions[i];
		ofbi->region->id = i;
		init_rwsem(&ofbi->region->lock);
		INIT_LIST_HEAD(&fbi->modelist);
		fbdev->num_fbs++;
	}

	printk("fb_infos allocated\n");
	
	/* assign managers for the fbs */
	DBG("assign managers for the fbs fbdev->def_display %p \n",fbdev->def_display);
	for (i = 0; i < min(fbdev->num_fbs, fbdev->num_overlays); i++) {
		struct owlfb_info *ofbi = FB2OFB(fbdev->fbs[i]);
		ofbi->manager =  owl_dss_get_overlay_manager(i);
		
		if(ofbi->manager->device){
			ofbi->manager->unset_device(ofbi->manager);
		}		
		/* connected display to managers */
		if(fbdev->def_display != NULL && i == 0){
			ofbi->manager->set_device(ofbi->manager,fbdev->def_display);
			owlfb_register_hotplug_notify(fbdev->def_display);
		}else{
			for( j = 0 ; j < fbdev->num_displays ; j++){
				if(!fbdev->displays[j].connected){
					fbdev->displays[j].connected = true;
					ofbi->manager->set_device(ofbi->manager,fbdev->displays[j].dssdev);

					DBG("owlfb_register_hotplug_notify allocated %s \n",fbdev->displays[j].dssdev->name);
					owlfb_register_hotplug_notify(fbdev->displays[j].dssdev);
					break;
				}
			}			
		}
	}	

	/* assign overlays for the fbs */
	DBG(" assign overlays for the fbs  \n");
	for (i = 0; i < min(fbdev->num_fbs, fbdev->num_overlays); i++) {
		struct owlfb_info *ofbi = FB2OFB(fbdev->fbs[i]);
		/*this primary fb , we used ovl 1,2*/
		if(i == 0)
		{			
			ofbi->num_overlays = 2;
			for(j = 0 ; j < ofbi->num_overlays; j++)
			{
				ofbi->overlays[j] = fbdev->overlays[k++];
				DBG("assign ovl %d  for fbs  %d \n", k,i);
			}
		}else{
			/*this external fb , we used ovl 4*/
			ofbi->num_overlays = 2;
			for(j = 0 ; j < ofbi->num_overlays; j++)
			{
				ofbi->overlays[j] = fbdev->overlays[k++];
				DBG("assign ovl %d  for fbs  %d \n", k,i);
			}
		}
	}
	
	/* check overlay and manager for ofbi*/
	DBG(" check overlay and manager for ofbi \n");
	for (i = 0; i < fbdev->num_fbs; i++) {		
		struct owlfb_info *ofbi = FB2OFB(fbdev->fbs[i]);
		DBG(" ofbi %d , ofbi->num_overlays %d  \n",i,ofbi->num_overlays);
		for( j = 0 ; j < ofbi->num_overlays;j++){
			struct owl_overlay *ovl = ofbi->overlays[j];	
			if (ovl != NULL && ovl->manager != NULL
				&& ovl->manager->id != ofbi->manager->id) {
					ovl->disable(ovl);
			    	ovl->unset_manager(ovl);		    	
			}
			ovl->set_manager(ovl,ofbi->manager); 
			DBG("ovl %s set to manager %s ",ovl->name,ofbi->manager->name);
		}		
	}		
	/* allocate fb memories */
	r = owlfb_allocate_all_fbs(fbdev);
	if (r) {
		dev_err(fbdev->dev, "failed to allocate fbmem\n");
		return r;
	}

	DBG("fbmems allocated\n");

	/* setup fb_infos */
    DBG(" setup fb_infos %d \n",fbdev->num_fbs);
	for (i = 0; i < fbdev->num_fbs; i++) {
		struct fb_info *fbi = fbdev->fbs[i];
		struct owlfb_info *ofbi = FB2OFB(fbi);

		owlfb_get_mem_region(ofbi->region);
		r = owlfb_fb_init(fbdev, fbi);
		owlfb_put_mem_region(ofbi->region);

		if (r) {
			dev_err(fbdev->dev, "failed to setup fb_info\n");
			return r;
		}
	}

	DBG("fb_infos initialized\n");

	for (i = 0; i < fbdev->num_fbs; i++) {
		r = register_framebuffer(fbdev->fbs[i]);
		if (r != 0) {
			dev_err(fbdev->dev,
				"registering framebuffer %d failed\n", i);
			return r;
		}
	}

	DBG("framebuffers registered\n");

	for (i = 0; i < fbdev->num_fbs; i++) {
		struct fb_info *fbi = fbdev->fbs[i];
		struct owlfb_info *ofbi = FB2OFB(fbi);

		owlfb_get_mem_region(ofbi->region);
		r = owlfb_apply_changes(fbi, 1);
		owlfb_put_mem_region(ofbi->region);

		if (r) {
			dev_err(fbdev->dev, "failed to change mode\n");
			return r;
		}
	}

	/* Enable fb0 */
	if (fbdev->num_fbs > 0) {
		struct owlfb_info *ofbi = FB2OFB(fbdev->fbs[0]);

		if (ofbi->num_overlays > 0) {
			struct owl_overlay *ovl = ofbi->overlays[0];

			r = owlfb_overlay_enable(ovl, 1);

			if (r) {
				dev_err(fbdev->dev,
						"failed to enable overlay\n");
				return r;
			}
		}
	}

	DBG("create_framebuffers done\n");

	return 0;
}

static int owlfb_mode_to_timings(const char *mode_str,
		struct owl_video_timings *timings, u8 *bpp)
{
	struct fb_info *fbi;
	struct fb_var_screeninfo *var;
	struct fb_ops *fbops;
	int r;

	/* this is quite a hack, but I wanted to use the modedb and for
	 * that we need fb_info and var, so we create dummy ones */

	*bpp = 0;
	fbi = NULL;
	var = NULL;
	fbops = NULL;

	fbi = kzalloc(sizeof(*fbi), GFP_KERNEL);
	if (fbi == NULL) {
		r = -ENOMEM;
		goto err;
	}

	var = kzalloc(sizeof(*var), GFP_KERNEL);
	if (var == NULL) {
		r = -ENOMEM;
		goto err;
	}

	fbops = kzalloc(sizeof(*fbops), GFP_KERNEL);
	if (fbops == NULL) {
		r = -ENOMEM;
		goto err;
	}

	fbi->fbops = fbops;

	r = fb_find_mode(var, fbi, mode_str, NULL, 0, NULL, 24);
	if (r == 0) {
		r = -EINVAL;
		goto err;
	}

	timings->pixel_clock = PICOS2KHZ(var->pixclock);
	timings->hbp = var->left_margin;
	timings->hfp = var->right_margin;
	timings->vbp = var->upper_margin;
	timings->vfp = var->lower_margin;
	timings->hsw = var->hsync_len;
	timings->vsw = var->vsync_len;
	timings->x_res = var->xres;
	timings->y_res = var->yres;

	switch (var->bits_per_pixel) {
	case 16:
		*bpp = 16;
		break;
	case 24:
	case 32:
	default:
		*bpp = 24;
		break;
	}

	r = 0;

err:
	kfree(fbi);
	kfree(var);
	kfree(fbops);

	return r;
}

static int owlfb_set_def_mode(struct owlfb_device *fbdev,
		struct owl_dss_device *display, char *mode_str)
{
	int r;
	u8 bpp;
	struct owl_video_timings timings, temp_timings;
	struct owlfb_display_data *d;

	r = owlfb_mode_to_timings(mode_str, &timings, &bpp);
	if (r)
		return r;

	d = get_display_data(fbdev, display);
	d->bpp_override = bpp;

	if (display->driver->check_timings) {
		r = display->driver->check_timings(display, &timings);
		if (r)
			return r;
	} else {
		/* If check_timings is not present compare xres and yres */
		if (display->driver->get_timings) {
			display->driver->get_timings(display, &temp_timings);

			if (temp_timings.x_res != timings.x_res ||
				temp_timings.y_res != timings.y_res)
				return -EINVAL;
		}
	}

	if (display->driver->set_timings)
			display->driver->set_timings(display, &timings);

	return 0;
}

static int owlfb_get_recommended_bpp(struct owlfb_device *fbdev,
		struct owl_dss_device *dssdev)
{
	struct owlfb_display_data *d;

	BUG_ON(dssdev->driver->get_recommended_bpp == NULL);

	d = get_display_data(fbdev, dssdev);

	if (d->bpp_override != 0)
		return d->bpp_override;

	return dssdev->driver->get_recommended_bpp(dssdev);
}
static int owlfb_init_display(struct owlfb_device *fbdev,
		struct owl_dss_device *dssdev)
{
	struct owl_dss_driver *dssdrv = dssdev->driver;
	struct owlfb_display_data *d;
	int r;

	r = dssdrv->enable(dssdev);
	if (r) {
		dev_warn(fbdev->dev, "Failed to enable display '%s'\n",
				dssdev->name);
		return r;
	}

	d = get_display_data(fbdev, dssdev);

	d->fbdev = fbdev;
		
	return 0;
}


static int owlfb_parse_params(struct platform_device *pdev,struct owlfb_device * fbdev)
{
	struct device_node      *of_node;
    char                    propname[20];
    char * def_display_name = &propname[0];
    u16 xres , yres , bpp;
    int i ;
    
    of_node = pdev->dev.of_node;
    
    of_property_read_string(of_node, "def_display", &def_display_name);
    DBG("def_display is %s \n",def_display_name);
    for (i = 0; i < fbdev->num_displays; ++i) {
			if (strcmp(fbdev->displays[i].dssdev->name,
						def_display_name) == 0) {
				fbdev->def_display = fbdev->displays[i].dssdev;
				fbdev->displays[i].connected = true;
				DBG("fbdev->def_display is %p \n",fbdev->def_display);
				break;
			}
    }
    
    if(fbdev->def_display == NULL){
    	fbdev->def_display = fbdev->displays[0].dssdev;
    	fbdev->displays[0].connected = true;
    }
    
    if( fbdev->def_display != NULL 
    	&& fbdev->def_display->driver != NULL 
    	&& fbdev->def_display->driver->get_timings != NULL){
    	struct owl_video_timings timings;
    	if(fbdev->def_display->driver->get_timings){
    		fbdev->def_display->driver->get_timings(fbdev->def_display,&timings);
    		fbdev->xres = timings.x_res;
			fbdev->yres = timings.y_res;
			fbdev->refresh = timings.pixel_clock * 1000 
					/ ((timings.x_res + timings.hsw + timings.hfp + timings.hbp) 
					* (timings.y_res + timings.vsw + timings.vfp + timings.vbp));
    	}
    }
        
    if(!of_property_read_u32(of_node, "xres", &xres) 
    	&& !of_property_read_u32(of_node, "yres", &yres)) {    	
    	fbdev->xres = xres;
    	fbdev->yres = yres;
    	if(fbdev->refresh == 0)
    	{
    		fbdev->refresh = 60;
    	}    	
    } 
    fbdev->bpp = 4;       
	printk("fbdev info : xres %d yres %d refresh %d name %s \n",fbdev->xres,fbdev->yres,fbdev->refresh,fbdev->def_display->name);
}
static struct of_device_id owl_fb_of_match[] = {
	{
		.compatible	= "actions,framebuffer",
	},
	{},
};
static int owlfb_probe(struct platform_device *pdev)
{
	struct owlfb_device *fbdev = NULL;
	const struct of_device_id *match;
	int r = 0;
	int i;
	struct owl_overlay *ovl;
	struct owl_dss_device *dssdev;
	struct device *dev = &pdev->dev;
	DBG("owlfb_probe\n");
	
	match = of_match_device(of_match_ptr(owl_fb_of_match), dev);
	if (!match) {
		dev_err(dev, "Error: No device match found\n");
		return -ENODEV;
	}	

	fbdev = kzalloc(sizeof(struct owlfb_device), GFP_KERNEL);
	if (fbdev == NULL) {
		r = -ENOMEM;
		goto err0;
	}

	mutex_init(&fbdev->mtx);

	fbdev->dev = &pdev->dev;
	platform_set_drvdata(pdev, fbdev);

	r = 0;
	fbdev->num_displays = 0;
	dssdev = NULL;
	for_each_dss_dev(dssdev) {
		struct owlfb_display_data *d;

		owl_dss_get_device(dssdev);

		if (!dssdev->driver) {
			owl_dss_put_device(dssdev);
			continue;
		}

		d = &fbdev->displays[fbdev->num_displays++];
		d->dssdev = dssdev;
	}

	if (r)
		goto cleanup;

	if (fbdev->num_displays == 0) {
		dev_err(&pdev->dev, "no displays\n");
		r = -EINVAL;
		goto cleanup;
	}

	fbdev->num_overlays = owl_dss_get_num_overlays();
	for (i = 0; i < fbdev->num_overlays; i++)
		fbdev->overlays[i] = owl_dss_get_overlay(i);

	fbdev->num_managers = owl_dss_get_num_overlay_managers();
	
	for (i = 0; i < fbdev->num_managers; i++)
		fbdev->managers[i] = owl_dss_get_overlay_manager(i);
		
	fbdev->mirror_fb_id	 = 0;
		
	owlfb_parse_params(pdev,fbdev);  
	
	r = owlfb_create_framebuffers(fbdev);
	if (r)
		goto cleanup;

	if (fbdev->def_display) {
	    if(owl_get_boot_mode() != OWL_BOOT_MODE_UPGRADE){
				r = owlfb_init_display(fbdev, fbdev->def_display);
				if (r) {
					dev_err(fbdev->dev,
							"failed to initialize default "
							"display\n");
					goto cleanup;
				}
			}else{
			    printk(" upgrade mode not open display \n");
			}
	}

#ifdef CONFIG_FB_MAP_TO_DE
	for (i = 0; i < fbdev->num_managers; i++) {
		struct owl_overlay_manager *mgr;
		mgr = fbdev->managers[i];
		r = mgr->apply(mgr);
		if (r)
			dev_warn(fbdev->dev, "failed to apply dispc config\n");
	}
#endif

	DBG("create sysfs for fbs\n");
	r = owlfb_create_sysfs(fbdev);
	if (r) {
		dev_err(fbdev->dev, "failed to create sysfs entries\n");
		goto cleanup;
	}
	
	owlfb_dc_init(fbdev);
	
	return 0;

cleanup:
	owlfb_free_resources(fbdev);
err0:
	dev_err(&pdev->dev, "failed to setup owlfb\n");
	return r;
}

static int owlfb_remove(struct platform_device *pdev)
{
	struct owlfb_device *fbdev = platform_get_drvdata(pdev);

	/* FIXME: wait till completion of pending events */

	owlfb_remove_sysfs(fbdev);

	owlfb_free_resources(fbdev);

	return 0;
}

static struct platform_driver owlfb_driver = {
	.probe          = owlfb_probe,
	.remove         = owlfb_remove,
	.driver         = {
		.name   = "framebuffer",
		.owner  = THIS_MODULE,
		.of_match_table	= owl_fb_of_match,
	},
};

static int __init owlfb_init(void)
{
	DBG("owlfb_init\n");

	if (platform_driver_register(&owlfb_driver)) {
		printk(KERN_ERR "failed to register owlfb driver\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit owlfb_exit(void)
{
	DBG("owlfb_exit\n");

	platform_driver_unregister(&owlfb_driver);
}

/* late_initcall to let panel/ctrl drivers loaded first.
 * I guess better option would be a more dynamic approach,
 * so that owlfb reowl to new panels when they are loaded */
late_initcall(owlfb_init);
/*module_init(owlfb_init);*/
module_exit(owlfb_exit);

MODULE_AUTHOR("Hui Wang  <wanghui@actions-semi.com>");
MODULE_DESCRIPTION("OWL2/3 Framebuffer");
MODULE_LICENSE("GPL v2");
