/*
 * linux/drivers/video/owl/owlfb-ioctl.c
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

#include <linux/fb.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/export.h>
#include <linux/delay.h>

#include <video/owldss.h>
#include <video/owlfb.h>

#include "owlfb.h"
extern atomic_t want_close_tv_devices;
static u8 get_mem_idx(struct owlfb_info *ofbi)
{
	if (ofbi->id == ofbi->region->id)
		return 0;

	return OWLFB_MEM_IDX_ENABLED | ofbi->region->id;
}

static struct owlfb_mem_region *get_mem_region(struct owlfb_info *ofbi,
						 u8 mem_idx)
{
	struct owlfb_device *fbdev = ofbi->fbdev;

	if (mem_idx & OWLFB_MEM_IDX_ENABLED)
		mem_idx &= OWLFB_MEM_IDX_MASK;
	else
		mem_idx = ofbi->id;

	if (mem_idx >= fbdev->num_fbs)
		return NULL;

	return &fbdev->regions[mem_idx];
}

static int owlfb_setup_plane(struct fb_info *fbi, struct owlfb_plane_info *pi)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owl_overlay *ovl;
	struct owl_overlay_info old_info;
	struct owlfb_mem_region *old_rg, *new_rg;
	int r = 0;

	DBG("owlfb_setup_plane\n");

	if (ofbi->num_overlays != 1) {
		r = -EINVAL;
		goto out;
	}

	/* XXX uses only the first overlay */
	ovl = ofbi->overlays[0];

	old_rg = ofbi->region;
	new_rg = get_mem_region(ofbi, pi->mem_idx);
	if (!new_rg) {
		r = -EINVAL;
		goto out;
	}

	/* Take the locks in a specific order to keep lockdep happy */
	if (old_rg->id < new_rg->id) {
		owlfb_get_mem_region(old_rg);
		owlfb_get_mem_region(new_rg);
	} else if (new_rg->id < old_rg->id) {
		owlfb_get_mem_region(new_rg);
		owlfb_get_mem_region(old_rg);
	} else
		owlfb_get_mem_region(old_rg);

	if (pi->enabled && !new_rg->size) {
		/*
		 * This plane's memory was freed, can't enable it
		 * until it's reallocated.
		 */
		r = -EINVAL;
		goto put_mem;
	}

	ovl->get_overlay_info(ovl, &old_info);

	if (old_rg != new_rg) {
		ofbi->region = new_rg;
		set_fb_fix(fbi);
	}

	if (!pi->enabled) {
		r = ovl->disable(ovl);
		if (r)
			goto undo;
	}

	if (pi->enabled) {
		r = owlfb_setup_overlay(fbi, ovl, pi->pos_x, pi->pos_y,
			pi->out_width, pi->out_height);
		if (r)
			goto undo;
	} else {
		struct owl_overlay_info info;

		ovl->get_overlay_info(ovl, &info);

		info.pos_x = pi->pos_x;
		info.pos_y = pi->pos_y;
		info.out_width = pi->out_width;
		info.out_height = pi->out_height;

		r = ovl->set_overlay_info(ovl, &info);
		if (r)
			goto undo;
	}

	if (ovl->manager)
		ovl->manager->apply(ovl->manager);

	if (pi->enabled) {
		r = ovl->enable(ovl);
		if (r)
			goto undo;
	}

	/* Release the locks in a specific order to keep lockdep happy */
	if (old_rg->id > new_rg->id) {
		owlfb_put_mem_region(old_rg);
		owlfb_put_mem_region(new_rg);
	} else if (new_rg->id > old_rg->id) {
		owlfb_put_mem_region(new_rg);
		owlfb_put_mem_region(old_rg);
	} else
		owlfb_put_mem_region(old_rg);

	return 0;

 undo:
	if (old_rg != new_rg) {
		ofbi->region = old_rg;
		set_fb_fix(fbi);
	}

	ovl->set_overlay_info(ovl, &old_info);
 put_mem:
	/* Release the locks in a specific order to keep lockdep happy */
	if (old_rg->id > new_rg->id) {
		owlfb_put_mem_region(old_rg);
		owlfb_put_mem_region(new_rg);
	} else if (new_rg->id > old_rg->id) {
		owlfb_put_mem_region(new_rg);
		owlfb_put_mem_region(old_rg);
	} else
		owlfb_put_mem_region(old_rg);
 out:
	dev_err(fbdev->dev, "setup_plane failed\n");

	return r;
}

static int owlfb_query_plane(struct fb_info *fbi, struct owlfb_plane_info *pi)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);

	if (ofbi->num_overlays != 1) {
		memset(pi, 0, sizeof(*pi));
	} else {
		struct owl_overlay *ovl;
		struct owl_overlay_info ovli;

		ovl = ofbi->overlays[0];
		ovl->get_overlay_info(ovl, &ovli);

		pi->pos_x = ovli.pos_x;
		pi->pos_y = ovli.pos_y;
		pi->enabled = ovl->is_enabled(ovl);
		pi->channel_out = 0; /* xxx */
		//pi->mirror = 0;
		pi->mem_idx = get_mem_idx(ofbi);
		pi->out_width = ovli.out_width;
		pi->out_height = ovli.out_height;
	}

	return 0;
}

static int owlfb_setup_mem(struct fb_info *fbi, struct owlfb_mem_info *mi)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owlfb_mem_region *rg;
	int r = 0, i;
	size_t size;

	if (mi->type > OWLFB_MEMTYPE_MAX)
		return -EINVAL;

	size = PAGE_ALIGN(mi->size);

	rg = ofbi->region;

	down_write_nested(&rg->lock, rg->id);
	atomic_inc(&rg->lock_count);

	if (atomic_read(&rg->map_count)) {
		r = -EBUSY;
		goto out;
	}

	for (i = 0; i < fbdev->num_fbs; i++) {
		struct owlfb_info *ofbi2 = FB2OFB(fbdev->fbs[i]);
		int j;

		if (ofbi2->region != rg)
			continue;

		for (j = 0; j < ofbi2->num_overlays; j++) {
			struct owl_overlay *ovl;
			ovl = ofbi2->overlays[j];
			if (ovl->is_enabled(ovl)) {
				r = -EBUSY;
				goto out;
			}
		}
	}

	if (rg->size != size || rg->type != mi->type) {
		r = owlfb_realloc_fbmem(fbi, size, mi->type);
		if (r) {
			dev_err(fbdev->dev, "realloc fbmem failed\n");
			goto out;
		}
	}

 out:
	atomic_dec(&rg->lock_count);
	up_write(&rg->lock);

	return r;
}

static int owlfb_query_mem(struct fb_info *fbi, struct owlfb_mem_info *mi)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_mem_region *rg;

	rg = owlfb_get_mem_region(ofbi->region);
	memset(mi, 0, sizeof(*mi));

	mi->size = rg->size;
	mi->type = rg->type;

	owlfb_put_mem_region(rg);

	return 0;
}

static int owlfb_update_window_nolock(struct fb_info *fbi,
		u32 x, u32 y, u32 w, u32 h)
{
#if 0
	struct owl_dss_device *display = fb2display(fbi);
	u16 dw, dh;

	if (!display)
		return 0;

	if (w == 0 || h == 0)
		return 0;

	display->driver->get_resolution(display, &dw, &dh);

	if (x + w > dw || y + h > dh)
		return -EINVAL;

	return display->driver->update(display, x, y, w, h);
#endif 
	return -EINVAL;
}

/* This function is exported for SGX driver use */
int owlfb_update_window(struct fb_info *fbi,
		u32 x, u32 y, u32 w, u32 h)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	int r;

	if (!lock_fb_info(fbi))
		return -ENODEV;
	owlfb_lock(fbdev);

	r = owlfb_update_window_nolock(fbi, x, y, w, h);

	owlfb_unlock(fbdev);
	unlock_fb_info(fbi);

	return r;
}
EXPORT_SYMBOL(owlfb_update_window);

int owlfb_set_update_mode(struct fb_info *fbi,
				   enum owlfb_update_mode mode)
{
	struct owl_dss_device *display = fb2display(fbi);
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owlfb_display_data *d;
	int r;

	if (!display)
		return -EINVAL;

	if (mode != OWLFB_AUTO_UPDATE && mode != OWLFB_MANUAL_UPDATE)
		return -EINVAL;

	owlfb_lock(fbdev);

	d = get_display_data(fbdev, display);

	if (d->update_mode == mode) {
		owlfb_unlock(fbdev);
		return 0;
	}

	r = 0;

	if (display->caps & OWL_DSS_DISPLAY_CAP_MANUAL_UPDATE) {
		if (mode == OWLFB_AUTO_UPDATE)
			owlfb_start_auto_update(fbdev, display);
		else /* MANUAL_UPDATE */
			owlfb_stop_auto_update(fbdev, display);

		d->update_mode = mode;
	} else { /* AUTO_UPDATE */
		if (mode == OWLFB_MANUAL_UPDATE)
			r = -EINVAL;
	}

	owlfb_unlock(fbdev);

	return r;
}

int owlfb_get_update_mode(struct fb_info *fbi,
		enum owlfb_update_mode *mode)
{
	struct owl_dss_device *display = fb2display(fbi);
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owlfb_display_data *d;

	if (!display)
		return -EINVAL;

	owlfb_lock(fbdev);

	d = get_display_data(fbdev, display);

	*mode = d->update_mode;

	owlfb_unlock(fbdev);

	return 0;
}

/* XXX this color key handling is a hack... */
static struct owlfb_color_key owlfb_color_keys[2];

static int _owlfb_set_color_key(struct owl_overlay_manager *mgr,
		struct owlfb_color_key *ck)
{
	struct owl_overlay_manager_info info;
	enum owl_dss_trans_key_type kt;
	int r;

	mgr->get_manager_info(mgr, &info);

	if (ck->key_type == OWLFB_COLOR_KEY_DISABLED) {
		info.trans_enabled = false;
		owlfb_color_keys[mgr->id] = *ck;

		r = mgr->set_manager_info(mgr, &info);
		if (r)
			return r;

		r = mgr->apply(mgr);

		return r;
	}

	switch (ck->key_type) {
	case OWLFB_COLOR_KEY_GFX_DST:
		kt = OWL_DSS_COLOR_KEY_GFX_DST;
		break;
	case OWLFB_COLOR_KEY_VID_SRC:
		kt = OWL_DSS_COLOR_KEY_VID_SRC;
		break;
	default:
		return -EINVAL;
	}

	info.default_color = ck->background;
	info.trans_key = ck->trans_key;
	info.trans_key_type = kt;
	info.trans_enabled = true;

	owlfb_color_keys[mgr->id] = *ck;

	r = mgr->set_manager_info(mgr, &info);
	if (r)
		return r;

	r = mgr->apply(mgr);

	return r;
}

static int owlfb_set_color_key(struct fb_info *fbi,
		struct owlfb_color_key *ck)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	int r;
	int i;
	struct owl_overlay_manager *mgr = NULL;

	owlfb_lock(fbdev);

	for (i = 0; i < ofbi->num_overlays; i++) {
		if (ofbi->overlays[i]->manager) {
			mgr = ofbi->overlays[i]->manager;
			break;
		}
	}

	if (!mgr) {
		r = -EINVAL;
		goto err;
	}

	r = _owlfb_set_color_key(mgr, ck);
err:
	owlfb_unlock(fbdev);

	return r;
}

static int owlfb_get_color_key(struct fb_info *fbi,
		struct owlfb_color_key *ck)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owl_overlay_manager *mgr = NULL;
	int r = 0;
	int i;

	owlfb_lock(fbdev);

	for (i = 0; i < ofbi->num_overlays; i++) {
		if (ofbi->overlays[i]->manager) {
			mgr = ofbi->overlays[i]->manager;
			break;
		}
	}

	if (!mgr) {
		r = -EINVAL;
		goto err;
	}

	*ck = owlfb_color_keys[mgr->id];
err:
	owlfb_unlock(fbdev);

	return r;
}

static int owlfb_memory_read(struct fb_info *fbi,
		struct owlfb_memory_read *mr)
{
	int r = 0;
/* TODO */
#if 0 
	struct owl_dss_device *display = fb2display(fbi);
	void *buf;
	if (!display || !display->driver->memory_read)
		return -ENOENT;

	if (!access_ok(VERIFY_WRITE, mr->buffer, mr->buffer_size))
		return -EFAULT;

	if (mr->w * mr->h * 3 > mr->buffer_size)
		return -EINVAL;

	buf = vmalloc(mr->buffer_size);
	if (!buf) {
		DBG("vmalloc failed\n");
		return -ENOMEM;
	}

	r = display->driver->memory_read(display, buf, mr->buffer_size,
			mr->x, mr->y, mr->w, mr->h);

	if (r > 0) {
		if (copy_to_user(mr->buffer, buf, mr->buffer_size))
			r = -EFAULT;
	}

	vfree(buf);
#endif 
	return r;
}

static int owlfb_pan_display(struct fb_info *fbi, struct owlfb_disp_content_info * disp_content)
{
	int r = 0;
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owl_dss_device *display = fb2display(fbi);
	struct owl_overlay *ovl;
	int i;
	
	
	for (i = 0; i < ofbi->num_overlays; i++) {
		struct owl_overlay_info info;
		
		ovl = ofbi->overlays[i];

		DBG("apply_changes, fb %d, ovl %d\n", ofbi->id, ovl->id);
        #if 0
		if (disp_content->overlay[i].paddr == 0) {
			/* the fb is not available. disable the overlay */
			owlfb_overlay_enable(ovl, 0);
			if (ovl->manager)
				ovl->manager->apply(ovl->manager);
			continue;
		}	
		#endif	
		DBG("apply_changes, fb %d, ovl %d\n", ofbi->id, ovl->id);
		ovl->get_overlay_info(ovl, &info);
		
		memcpy(&info,&disp_content->overlay[i],sizeof(info));		

	    r = ovl->set_overlay_info(ovl, &info);

		DBG("ovl->manager->apply, fb %d, ovl %d\n", ofbi->id, ovl->id);

		if (ovl->manager)
			ovl->manager->apply(ovl->manager);
	}
 
	if(display && display->driver->dump){
		display->driver->dump(display);
	}
	//de_dump_regs();
	return r;
}

static int owlfb_get_ovl_colormode(struct owlfb_device *fbdev,
			     struct owlfb_ovl_colormode *mode)
{
	int ovl_idx = mode->overlay_idx;
	int mode_idx = mode->mode_idx;
	struct owl_overlay *ovl;
	enum owl_color_mode supported_modes;
	struct fb_var_screeninfo var;
	int i;

	if (ovl_idx >= fbdev->num_overlays)
		return -ENODEV;
	ovl = fbdev->overlays[ovl_idx];
	supported_modes = ovl->supported_modes;

	mode_idx = mode->mode_idx;

	for (i = 0; i < sizeof(supported_modes) * 8; i++) {
		if (!(supported_modes & (1 << i)))
			continue;
		/*
		 * It's possible that the FB doesn't support a mode
		 * that is supported by the overlay, so call the
		 * following here.
		 */
		if (dss_mode_to_fb_mode(1 << i, &var) < 0)
			continue;

		mode_idx--;
		if (mode_idx < 0)
			break;
	}

	if (i == sizeof(supported_modes) * 8)
		return -ENOENT;

	mode->bits_per_pixel = var.bits_per_pixel;
	mode->nonstd = var.nonstd;
	mode->red = var.red;
	mode->green = var.green;
	mode->blue = var.blue;
	mode->transp = var.transp;

	return 0;
}

static int owlfb_wait_for_go(struct fb_info *fbi)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	int r = 0;
	int i;

	for (i = 0; i < ofbi->num_overlays; ++i) {
		struct owl_overlay *ovl = ofbi->overlays[i];
		r = ovl->wait_for_go(ovl);
		if (r)
			break;
	}

	return r;
}

extern int owl_de_get_ictype(void);
int owlfb_get_effect_parameter(struct owlfb_disp_device * disp_info)
{	
	struct owl_overlay_manager * lcd_mgr = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_LCD);	
	struct display_private_info *new_private_info = (struct display_private_info *)&(disp_info->mPrivateInfo[0]);	
	printk("get_info~~\n");	
	if(lcd_mgr->device 
		&& lcd_mgr->device->driver
		&& lcd_mgr->device->driver->get_effect_parameter){
			disp_info->mIcType = owl_de_get_ictype();	
			new_private_info->LCD_TYPE = lcd_mgr->device->type;			
			new_private_info->LCD_LIGHTENESS = lcd_mgr->device->driver->get_effect_parameter(lcd_mgr->device ,OWL_DSS_VIDEO_LIGHTNESS);
			new_private_info->LCD_SATURATION = lcd_mgr->device->driver->get_effect_parameter(lcd_mgr->device ,OWL_DSS_VIDEO_SATURATION);
			new_private_info->LCD_CONSTRAST = lcd_mgr->device->driver->get_effect_parameter(lcd_mgr->device ,OWL_DSS_VIDEO_CONSTRAST);
	}
	return 0;
}

int owlfb_set_effect_parameter(struct owlfb_disp_device * disp_info)
{	
	struct owl_overlay_manager * lcd_mgr = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_LCD);	
	struct display_private_info *new_private_info = (struct display_private_info *)&(disp_info->mPrivateInfo[0]);	
	int 	lightness = new_private_info->LCD_LIGHTENESS;
	int 	saturation = new_private_info->LCD_SATURATION;
	int 	contrast = new_private_info->LCD_CONSTRAST;
	printk("set_info~~\n");
	if(lcd_mgr->device 
		&& lcd_mgr->device->driver
		&& lcd_mgr->device->driver->set_effect_parameter){

		switch(disp_info->mCmdMode){
			case SET_LIGHTENESS:
				  lcd_mgr->device->driver->set_effect_parameter(lcd_mgr->device ,OWL_DSS_VIDEO_LIGHTNESS, lightness);
				  printk("set_info~~ new_private_info->LCD_LIGHTENESS  %d\n", new_private_info->LCD_LIGHTENESS);
				  break;	
			case SET_SATURATION:
				  lcd_mgr->device->driver->set_effect_parameter(lcd_mgr->device ,OWL_DSS_VIDEO_SATURATION, saturation);
				  printk("set_info~~ new_private_info->LCD_SATURATION  %d\n", new_private_info->LCD_SATURATION);
				  break;	
			case SET_CONSTRAST:
				  lcd_mgr->device->driver->set_effect_parameter(lcd_mgr->device ,OWL_DSS_VIDEO_CONSTRAST, contrast);
				  printk("set_info~~ new_private_info->LCD_CONSTRAST  %d\n", new_private_info->LCD_CONSTRAST);
				  break;	
			case SET_DEFAULT:
				  lcd_mgr->device->driver->set_effect_parameter(lcd_mgr->device ,OWL_DSS_DEF_EFFECT, 0);
				  printk("set_info~~ new_private_info->LCD_CONSTRAST  %d\n", new_private_info->LCD_CONSTRAST);
				  break;			
		}
	}
	return 0;
}
int owlfb_ioctl(struct fb_info *fbi, unsigned int cmd, unsigned long arg)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owl_dss_device *display = fb2display(fbi);

	union {
		struct owlfb_update_window_old	uwnd_o;
		struct owlfb_update_window	uwnd;
		struct owlfb_plane_info	plane_info;
		struct owlfb_caps		caps;
		struct owlfb_mem_info          mem_info;
		struct owlfb_color_key		color_key;
		struct owlfb_ovl_colormode	ovl_colormode;
		enum owlfb_update_mode		update_mode;
		int test_num;
		struct owlfb_memory_read	memory_read;
		struct owlfb_disp_content_info		content_info;
		struct owlfb_sync_info sync_info;
		struct owlfb_disp_histogram_info histogram_info;
		struct owl_gamma_info gamma_info;
		struct owlfb_display_info display_info;
		struct owlfb_hdmi_vid_info hdmi_vid;
		u32				crt;
	} p;

	int r = 0;

	switch (cmd) {
	case OWLFB_SYNC_GFX:
		DBG("ioctl SYNC_GFX\n");
#if 0
		if (!display || !display->driver->sync) {
			/* DSS1 never returns an error here, so we neither */
			/*r = -EINVAL;*/
			break;
		}

		r = display->driver->sync(display);
#endif 
		break;

	case OWLFB_UPDATE_WINDOW_OLD:
		DBG("ioctl UPDATE_WINDOW_OLD\n");
#if 0
		if (!display || !display->driver->update) {
			r = -EINVAL;
			break;
		}

		if (copy_from_user(&p.uwnd_o,
					(void __user *)arg,
					sizeof(p.uwnd_o))) {
			r = -EFAULT;
			break;
		}

		r = owlfb_update_window_nolock(fbi, p.uwnd_o.x, p.uwnd_o.y,
				p.uwnd_o.width, p.uwnd_o.height);
#endif 
		break;

	case OWLFB_UPDATE_WINDOW:
		DBG("ioctl UPDATE_WINDOW\n");
#if 0
		if (!display || !display->driver->update) {
			r = -EINVAL;
			break;
		}

		if (copy_from_user(&p.uwnd, (void __user *)arg,
					sizeof(p.uwnd))) {
			r = -EFAULT;
			break;
		}

		r = owlfb_update_window_nolock(fbi, p.uwnd.x, p.uwnd.y,
				p.uwnd.width, p.uwnd.height);
#endif 
		break;

	case OWLFB_SETUP_PLANE:
		DBG("ioctl SETUP_PLANE\n");
		if (copy_from_user(&p.plane_info, (void __user *)arg,
					sizeof(p.plane_info)))
			r = -EFAULT;
		else
			r = owlfb_setup_plane(fbi, &p.plane_info);
		break;

	case OWLFB_QUERY_PLANE:
		DBG("ioctl QUERY_PLANE\n");
		r = owlfb_query_plane(fbi, &p.plane_info);
		if (r < 0)
			break;
		if (copy_to_user((void __user *)arg, &p.plane_info,
					sizeof(p.plane_info)))
			r = -EFAULT;
		break;

	case OWLFB_SETUP_MEM:
		DBG("ioctl SETUP_MEM\n");
		if (copy_from_user(&p.mem_info, (void __user *)arg,
					sizeof(p.mem_info)))
			r = -EFAULT;
		else
			r = owlfb_setup_mem(fbi, &p.mem_info);
		break;

	case OWLFB_QUERY_MEM:
		DBG("ioctl QUERY_MEM\n");
		r = owlfb_query_mem(fbi, &p.mem_info);
		if (r < 0)
			break;
		if (copy_to_user((void __user *)arg, &p.mem_info,
					sizeof(p.mem_info)))
			r = -EFAULT;
		break;

	case OWLFB_GET_CAPS:
		DBG("ioctl GET_CAPS\n");
		if (!display) {
			r = -EINVAL;
			break;
		}

		memset(&p.caps, 0, sizeof(p.caps));
		if (display->caps & OWL_DSS_DISPLAY_CAP_MANUAL_UPDATE)
			p.caps.ctrl |= OWLFB_CAPS_MANUAL_UPDATE;
		if (display->caps & OWL_DSS_DISPLAY_CAP_TEAR_ELIM)
			p.caps.ctrl |= OWLFB_CAPS_TEARSYNC;

		if (copy_to_user((void __user *)arg, &p.caps, sizeof(p.caps)))
			r = -EFAULT;
		break;

	case OWLFB_GET_OVERLAY_COLORMODE:
		DBG("ioctl GET_OVERLAY_COLORMODE\n");
		if (copy_from_user(&p.ovl_colormode, (void __user *)arg,
				   sizeof(p.ovl_colormode))) {
			r = -EFAULT;
			break;
		}
		r = owlfb_get_ovl_colormode(fbdev, &p.ovl_colormode);
		if (r < 0)
			break;
		if (copy_to_user((void __user *)arg, &p.ovl_colormode,
				 sizeof(p.ovl_colormode)))
			r = -EFAULT;
		break;

	case OWLFB_SET_UPDATE_MODE:
		DBG("ioctl SET_UPDATE_MODE\n");
		if (get_user(p.update_mode, (int __user *)arg))
			r = -EFAULT;
		else
			r = owlfb_set_update_mode(fbi, p.update_mode);
		break;

	case OWLFB_GET_UPDATE_MODE:
		DBG("ioctl GET_UPDATE_MODE\n");
		r = owlfb_get_update_mode(fbi, &p.update_mode);
		if (r)
			break;
		if (put_user(p.update_mode,
					(enum owlfb_update_mode __user *)arg))
			r = -EFAULT;
		break;

	case OWLFB_SET_COLOR_KEY:
		DBG("ioctl SET_COLOR_KEY\n");
		if (copy_from_user(&p.color_key, (void __user *)arg,
				   sizeof(p.color_key)))
			r = -EFAULT;
		else
			r = owlfb_set_color_key(fbi, &p.color_key);
		break;

	case OWLFB_GET_COLOR_KEY:
		DBG("ioctl GET_COLOR_KEY\n");
		r = owlfb_get_color_key(fbi, &p.color_key);
		if (r)
			break;
		if (copy_to_user((void __user *)arg, &p.color_key,
				 sizeof(p.color_key)))
			r = -EFAULT;
		break;

	case OWLFB_WAITFORVSYNC: {
			long long i64TimeStamp;
			DBG("ioctl WAITFORVSYNC\n");
			if (copy_from_user(&i64TimeStamp, (void __user *)arg, sizeof(i64TimeStamp))){
				r = -EFAULT;
				break;
			}
			if (!display) {
				r = -EINVAL;
				break;
			}
			unlock_fb_info(fbi);
			r = display->manager->wait_for_vsync(display->manager,i64TimeStamp);
			lock_fb_info(fbi);		
			break;
		}
	case OWLFB_WAITFORGO:
		DBG("ioctl WAITFORGO\n");
		if (!display) {
			r = -EINVAL;
			break;
		}

		r = owlfb_wait_for_go(fbi);
		break;

	/* LCD and CTRL tests do the same thing for backward
	 * compatibility */
	case OWLFB_LCD_TEST:
		DBG("ioctl LCD_TEST\n");
		if (get_user(p.test_num, (int __user *)arg)) {
			r = -EFAULT;
			break;
		}
		if (!display || !display->driver->run_test) {
			r = -EINVAL;
			break;
		}

		r = display->driver->run_test(display, p.test_num);

		break;

	case OWLFB_CTRL_TEST:
		DBG("ioctl CTRL_TEST\n");
		if (get_user(p.test_num, (int __user *)arg)) {
			r = -EFAULT;
			break;
		}
		if (!display || !display->driver->run_test) {
			r = -EINVAL;
			break;
		}

		r = display->driver->run_test(display, p.test_num);

		break;

	case OWLFB_MEMORY_READ:
		DBG("ioctl MEMORY_READ\n");

		if (copy_from_user(&p.memory_read, (void __user *)arg,
					sizeof(p.memory_read))) {
			r = -EFAULT;
			break;
		}

		r = owlfb_memory_read(fbi, &p.memory_read);

		break;

	case OWLFB_SET_PAN_DISPLAY: {
		if (copy_from_user(&p.content_info, (void __user *)arg,
					sizeof(p.content_info))) {
			r = -EFAULT;
			break;
		}
		owlfb_pan_display(fbi,&p.content_info);
		break;
	}
	case OWLFB_GET_MAIN_DISPLAY_RES: {
		u16 xres, yres;
		struct owl_video_timings timings;
		printk("ioctl GET_DISPLAY_INFO display %p \n",display);

		if (display == NULL) {
			r = -ENODEV;
			break;
		}		
		if(display->driver->get_timings){
			display->driver->get_timings(display,&timings);
			p.display_info.xres = timings.x_res;
			p.display_info.yres = timings.y_res;
			p.display_info.xres_ext = timings.hsw + timings.hfp + timings.hbp ;
			p.display_info.yres_ext = timings.vsw + timings.vfp + timings.vbp ;
			p.display_info.pixclock = timings.pixel_clock;
			printk(KERN_ERR"hsw %d  hfp %d hbp %d \n",timings.hsw, timings.hfp,timings.hbp);
			printk(KERN_ERR"vsw %d  vfp %d vbp %d \n",timings.vsw, timings.vfp,timings.vbp);
			printk(KERN_ERR"timings.pixel_clock %d \n",timings.pixel_clock);
		}else{			
			display->driver->get_resolution(display, &xres, &yres);
			p.display_info.xres = xres;
			p.display_info.yres = yres;			
		}
		if (display->driver->get_dimensions) {
			u32 w, h;
			display->driver->get_dimensions(display, &w, &h);
			p.display_info.width = w;
			p.display_info.height = h;
		} else {
			p.display_info.width = 0;
			p.display_info.height = 0;
		}

		if (copy_to_user((void __user *)arg, &p.display_info,
					sizeof(p.display_info)))
			r = -EFAULT;
		
		break;
	}
	
	case OWLFB_GET_HISTOGRAM_INFO: {
		DBG("ioctl OWLFB_GET_HISTOGRAM_INFO\n");
		owl_de_get_histogram_info((__u32 *)&p.histogram_info.hist,
                                    (__u32 *)&p.histogram_info.totaly);
		if (copy_to_user((void __user *)arg, &p.histogram_info,
					sizeof(p.histogram_info)))
			r = -EFAULT;
		break;
	}
	case OWLFB_SET_GAMMA_INFO: {
		DBG("ioctl OWLFB_SET_GAMMA_INFO, &p.gamma_info %p\n",
			&p.gamma_info);
		if (copy_from_user(&p.gamma_info, (void __user *)arg,
					sizeof(p.gamma_info))) {
			r = -EFAULT;
			break;
		}

		dss_mgr_set_gamma_table(OWL_DSS_CHANNEL_LCD, &p.gamma_info);
		break;
	}
	case OWLFB_GET_GAMMA_INFO: {
		DBG("ioctl OWLFB_SET_GAMMA_INFO, &p.gamma_info %p\n",&p.gamma_info);				
		dss_mgr_get_gamma_table(OWL_DSS_CHANNEL_LCD, &p.gamma_info);

		if (copy_to_user((void __user *)arg, &p.gamma_info,
			sizeof(p.gamma_info))) {
			r = -EFAULT;
		}
		break;
	}
	case OWLFB_VSYNC_EVENT_EN:{
		if (copy_from_user(&p.sync_info, (void __user *)arg, sizeof(p.sync_info))) {
			r = -EFAULT;
			break;
		}
		DBG("ioctl OWLFB_VSYNC_EVENT_EN display %d  enable %d\n",p.sync_info.disp_id,p.sync_info.enabled);
		if (!display) {
			r = -EINVAL;
			break;
		}
		
		r = display->manager->enable_hw_vsync(display->manager,p.sync_info.enabled,fbdev->dev);
		
		break;
	}
	case OWLFB_HDMI_SUPPORT_MODE:{
		int support_vid[9];
		int i = 0;
		int k = 0;
		struct owl_overlay_manager * tv_mgr = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_TV);		
		if (copy_from_user(&p.hdmi_vid, (void __user *)arg, sizeof(p.hdmi_vid))) {
			r = -EFAULT;
			break;
		}
		DBG("ioctl OWLFB_HDMI_SUPPORT_MODE  vid %d mode %d \n",p.hdmi_vid.vid,p.hdmi_vid.mode);	
		memset(support_vid, 0, sizeof(support_vid));
		if(tv_mgr->device 
			&& tv_mgr->device->driver
			&& tv_mgr->device->driver->get_vid_cap){
			k = tv_mgr->device->driver->get_vid_cap(tv_mgr->device, support_vid);	
		}
		
		for(i = 0 ; i < k; i++){
			if(p.hdmi_vid.vid == support_vid[i]){		
				return 1;
			}
		}
		r = 0;		
		break;
	}	
	case OWLFB_HDMI_GET_CABLE_STATUS:{
		int i = 0;
		struct owl_overlay_manager * tv_mgr = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_TV);			
		if(tv_mgr->device 
			&& tv_mgr->device->driver
			&& tv_mgr->device->driver->get_vid_cap){
			i = tv_mgr->device->driver->get_cable_status(tv_mgr->device);	
		}
		
		if (copy_to_user((void __user *)arg, &i,
					sizeof(int)))
			r = -EFAULT;
		break;
	}
	case OWLFB_HDMI_SET_MODE:{		
		struct owl_overlay_manager * tv_mgr = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_TV);

		if (copy_from_user(&p.hdmi_vid, (void __user *)arg, sizeof(p.hdmi_vid))) {
			r = -EFAULT;
			break;
		}
		DBG("ioctl OWLFB_HDMI_SET_MODE vid %d \n",p.hdmi_vid.vid);		
		if(tv_mgr->device 
			&& tv_mgr->device->driver
			&& tv_mgr->device->driver->set_vid){
			tv_mgr->device->driver->set_vid(tv_mgr->device,p.hdmi_vid.vid);	
		}
		break;
	}	
	case OWLFB_HDMI_ON:{
		struct owl_overlay_manager * tv_mgr = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_TV);
		DBG("ioctl OWLFB_HDMI_ON\n");
		tv_mgr->device->driver->enable(tv_mgr->device);
		atomic_set(&want_close_tv_devices,false);
		unlock_fb_info(fbi);
		msleep(500);
		lock_fb_info(fbi);
		break;
	}	
	case OWLFB_HDMI_OFF:{
		DBG("ioctl OWLFB_HDMI_OFF\n");
		//struct owl_overlay_manager * tv_mgr = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_TV);
		//tv_mgr->device->driver->disable(tv_mgr->device);
		atomic_set(&want_close_tv_devices,true);
		break;
	}
	case OWLFB_HDMI_DISABLE:{
		int enable = 0;
		struct owl_overlay_manager * tv_mgr = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_TV);
		DBG("ioctl OWLFB_HDMI_DISABLE\n");
		if (copy_from_user(&enable, (void __user *)arg, sizeof(int))) {
			r = -EFAULT;
			break;
		}
		if(enable){
			if(tv_mgr->device 
				&& tv_mgr->device->driver
				&& tv_mgr->device->driver->enable_hpd){
				tv_mgr->device->driver->enable_hpd(tv_mgr->device, 1);
			}			
		}else{
			if(tv_mgr->device 
				&& tv_mgr->device->driver
				&& tv_mgr->device->driver->enable_hpd){
				tv_mgr->device->driver->enable_hpd(tv_mgr->device, 0);
			}		
		}
		

		break;
	}
	case OWLFB_HDMI_GET_VID:{
		int vid = -1;
		struct owl_overlay_manager * tv_mgr = owl_dss_get_overlay_manager(OWL_DSS_OVL_MGR_TV);			
		if(tv_mgr->device 
			&& tv_mgr->device->driver
			&& tv_mgr->device->driver->get_vid){
			tv_mgr->device->driver->get_vid(tv_mgr->device, (int *)&vid);	
		}
		
		if (copy_to_user((void __user *)arg, &vid,
					sizeof(int)))
			r = -EFAULT;
		break;
	}	
	case OWLFB_LAYER_REQUEST:{
		DBG("ioctl OWLFB_LAYER_REQUEST\n");
		break;
	}		
	case OWLFB_GET_DISPLAY_INFO:{
	    	struct owlfb_disp_device info;
	    	if(copy_from_user(&info,(void *)arg,sizeof(struct owlfb_disp_device))){
	             return -EFAULT;
	        }
	        owlfb_get_effect_parameter(&info);
	        if(copy_to_user((void *)arg,(void *)&info,sizeof(struct owlfb_disp_device))){
	            return -EFAULT;
	        }
    	    
		break;
	}	
	case OWLFB_SET_DISPLAY_INFO:{
		struct owlfb_disp_device info;
		if(copy_from_user(&info,(void *)arg,sizeof(struct owlfb_disp_device))){
			return -EFAULT;
		}
		owlfb_set_effect_parameter(&info);
		break;	
	}
	default:
		dev_err(fbdev->dev, "Unknown ioctl 0x%x\n", cmd);
		r = -EINVAL;
	}

	if (r < 0)
		DBG("ioctl failed: %d\n", r);

	return r;
}


