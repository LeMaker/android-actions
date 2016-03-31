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
extern atomic_t want_close_external_devices;

static int owlfb_request_overlay(struct fb_info *fbi, struct owlfb_overlay_args * request)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	int ovl_id = -1;
	int i;	
	
	if(request->overlay_type == OWLFB_OVERLAY_VIDEO){
		for (i = 1; i < ofbi->num_overlays; ++i) {
			if((ofbi->used_overlay_mask & (1 << i)) == 0){
				ovl_id = i;
				ofbi->used_overlay_mask |= (1 << i);
				break;
			}		
		}		
		if(OWLFB_MAX_OVL_MEM_RESERVE_PER_OVL <= ofbi->overlay_free_mem_size){
			request->overlay_mem_base = ofbi->overlay_mem_base + OWLFB_MAX_CURSOR_MEM_RESERVE + ofbi->overlay_free_mem_off;
			request->overlay_mem_size = OWLFB_MAX_OVL_MEM_RESERVE_PER_OVL;
			ofbi->overlay_free_mem_off += OWLFB_MAX_OVL_MEM_RESERVE_PER_OVL;
			ofbi->overlay_free_mem_size -= OWLFB_MAX_OVL_MEM_RESERVE_PER_OVL;
			request->overlay_id = ovl_id;	
		}
		else
		{
			ovl_id = -1;
		}
	}else if(request->overlay_type == OWLFB_OVERLAY_CURSOR){
		request->overlay_id = OWLFB_CURSOR_OVL_ID;
		ovl_id = OWLFB_CURSOR_OVL_ID;
		ofbi->used_overlay_mask |= (1 << 3);
		request->overlay_mem_base = ofbi->overlay_mem_base + ofbi->overlay_free_mem_off;
		request->overlay_mem_size = OWLFB_MAX_CURSOR_MEM_RESERVE;
	}
	
	DBG("owlfb_request_overlay ovl_id %d request->overlay_mem_base 0x%x,request->overlay_mem_size 0x%x\n",ovl_id,request->overlay_mem_base,request->overlay_mem_size);
	return ovl_id;
}

static int owlfb_release_overlay(struct fb_info *fbi, int ovl_id)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	DBG("owlfb_release_overlay ovl_id %d \n",ovl_id);
	if((ofbi->used_overlay_mask & (1 << ovl_id)) != 0)
	{
		ofbi->used_overlay_mask &= (~(1 << ovl_id));
	}
	return 0;
}

static int owlfb_set_overlay_enable(struct fb_info *fbi,int ovl_id ,bool enable)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owl_overlay *ovl = NULL;
	struct owlfb_device *fbdev = ofbi->fbdev;
	int rc = -1;
	
	if(ovl_id == OWLFB_CURSOR_OVL_ID)
	{
		struct owl_cursor_info cursor;
		
		ofbi->manager->get_cursor_info(ofbi->manager,&cursor);
		
		cursor.enable = enable;
		
		ofbi->manager->set_cursor_info(ofbi->manager,&cursor);
		
		ofbi->manager->apply(ofbi->manager);
		
		if(fbdev->mirror_fb_id){
			struct fb_info * link_fbi = fbdev->fbs[fbdev->mirror_fb_id];
			struct owlfb_info *link_ofbi = FB2OFB(link_fbi);
			link_ofbi->manager->get_cursor_info(link_ofbi->manager,&cursor);
			cursor.enable = enable;		
			link_ofbi->manager->set_cursor_info(link_ofbi->manager,&cursor);
			link_ofbi->manager->apply(link_ofbi->manager);
		}		
	}else{
		if(ovl_id > ofbi->num_overlays){
			return -1;
		}
		
		ovl = ofbi->overlays[ovl_id];
		DBG("owlfb_set_overlay_enable ovl_id %d enable %d \n",ovl_id,enable);
		if(enable){
			if(!ovl->is_enabled(ovl)){
				ovl->enable(ovl);
				ovl->manager->apply(ovl->manager);
				//ovl->manager->wait_for_go(ovl->manager);
				rc = 0;
			}
		}else{
			if(ovl->is_enabled(ovl)){
				ovl->disable(ovl);
				ovl->manager->apply(ovl->manager);
				//ovl->manager->wait_for_go(ovl->manager);
				rc = 0;
			}
		}
	}	
	return 0;
}

static int owlfb_get_overlay_info(struct fb_info *fbi,int ovl_id, struct owlfb_overlay_info * userinfo)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owl_overlay *ovl = NULL;
	struct owl_overlay_info info;
	
	if(ovl_id == OWLFB_CURSOR_OVL_ID)
	{
		struct owl_cursor_info cursor;
		
		ofbi->manager->get_cursor_info(ofbi->manager,&cursor);

		userinfo->pos_x = cursor.pos_x;
		userinfo->pos_y = cursor.pos_y;
		
	}else{	
		if(ovl_id > ofbi->num_overlays){
			return -1;
		}

		ovl = ofbi->overlays[ovl_id];
			
		ovl->get_overlay_info(ovl, &info);
		
		userinfo->color_mode = info.color_mode;
		userinfo->img_width = info.img_width;
		userinfo->img_height = info.img_height;
		
		userinfo->xoff = info.xoff;
		userinfo->yoff = info.yoff;
		userinfo->width = info.width;
		userinfo->height = info.height;
			
		userinfo->pos_x = info.pos_x;
		userinfo->pos_y = info.pos_y;
		userinfo->out_width = info.out_width;
		userinfo->out_height = info.out_height;	
					
		userinfo->rotation = info.rotation;	
		
		userinfo->global_alpha_en = info.global_alpha_en;
		userinfo->global_alpha = info.global_alpha; 	
		userinfo->pre_mult_alpha_en = info.pre_mult_alpha_en;
		DBG("owlfb_get_overlay_info  ovl_id %d info: color_mode 0x%x img_width 0x%x info.img_height 0x%x \n",ovl_id,info.color_mode,info.img_width,info.img_height);	
		DBG(" info: xoff 0x%x yoff 0x%x width 0x%x height 0x%x \n",info.xoff,info.yoff,info.width,info.height);	
		DBG(" info: pos_x 0x%x pos_y 0x%x out_width 0x%x out_height 0x%x \n",info.pos_x,info.pos_y,info.out_width,info.out_height);	
		DBG(" info: global_alpha_en 0x%x global_alpha 0x%x pre_mult_alpha_en 0x%x \n",info.global_alpha_en,info.global_alpha,info.pre_mult_alpha_en);	
	}
	return 0;
}

static int owlfb_set_overlay_info(struct fb_info *fbi,int ovl_id , struct owlfb_overlay_info * userinfo)
{
	struct owlfb_info *ofbi = FB2OFB(fbi);
	struct owlfb_device *fbdev = ofbi->fbdev;
	struct owl_overlay *ovl = NULL;
	struct owl_overlay_info info;
	struct owl_dss_device * src_dssdev = ofbi->manager->device;
	u16 src_out_w, src_out_h;
	
	if(ovl_id == OWLFB_CURSOR_OVL_ID)
	{
		struct owl_cursor_info cursor;
		
		ofbi->manager->get_cursor_info(ofbi->manager,&cursor);
	
		cursor.pos_x  = userinfo->pos_x;
		cursor.pos_y  = userinfo->pos_y;
		
		if(src_dssdev != NULL && src_dssdev->driver != NULL){
			u16 overscan_w, overscan_h;
			src_dssdev->driver->get_resolution(src_dssdev, &src_out_w, &src_out_h);
			if(src_dssdev->driver->get_over_scan)
			{
				src_dssdev->driver->get_over_scan(src_dssdev,&overscan_w,&overscan_h);
				if(overscan_w != 0 || overscan_h != 0){
					overscan_w = src_out_w - overscan_w * 2;
					overscan_h = src_out_h - overscan_h * 2;
					cursor.pos_x = src_out_w / 2 - overscan_w * (src_out_w / 2 - cursor.pos_x) / src_out_w ;
					cursor.pos_y = src_out_h / 2 - overscan_h * (src_out_h / 2 - cursor.pos_y) / src_out_h ;
					
					if(cursor.pos_x > (src_out_w + overscan_w)/ 2){
						cursor.pos_x = (src_out_w + overscan_w)/ 2;
					}					
					if(cursor.pos_y > (src_out_h + overscan_h)/ 2){
						cursor.pos_y =  (src_out_h + overscan_h)/ 2;
					}
				}				
			}
		}
		
		if(cursor.pos_x > src_out_w - 64){
			cursor.pos_x = src_out_w - 64;
		}
		
		if(cursor.pos_y > src_out_h - 64){
			cursor.pos_y = src_out_h - 64;
		}
		
		if(src_dssdev->type == OWL_DISPLAY_TYPE_CVBS)
		{
			cursor.pos_y = (cursor.pos_y & (~0x01));					
		}
		
		cursor.paddr = 	userinfo->mem_off + ofbi->region->paddr;
		cursor.stride = userinfo->img_width * 4 / 8;
		
		DBG("owlfb_set_overlay_info OWLFB_CURSOR_OVL_ID cursor.pos_x %d cursor.pos_y %d cursor.paddr 0x%x cursor.stride %d \n",cursor.pos_x,cursor.pos_y,cursor.paddr,cursor.stride);
		
		ofbi->manager->set_cursor_info(ofbi->manager,&cursor);
		
		ofbi->manager->apply(ofbi->manager);
		
		if(fbdev->mirror_fb_id){
			u16 link_out_w, link_out_h;
			u16 link_overscan_w, link_overscan_h;
			struct fb_info * link_fbi = fbdev->fbs[fbdev->mirror_fb_id];
			struct owlfb_info *link_ofbi = FB2OFB(link_fbi);
			struct owl_cursor_info link_cursor;			
			struct owl_dss_device * link_dssdev = link_ofbi->manager->device;
			
			link_ofbi->manager->get_cursor_info(link_ofbi->manager,&link_cursor);
			
			if(link_dssdev != NULL && link_dssdev->driver != NULL){				
				link_dssdev->driver->get_resolution(link_dssdev, &link_out_w, &link_out_h);
				if(link_dssdev->driver->get_over_scan != NULL){
					link_dssdev->driver->get_over_scan(link_dssdev,&link_overscan_w,&link_overscan_h);
					link_overscan_w = link_out_w - link_overscan_w * 2;
					link_overscan_h = link_out_h - link_overscan_h * 2;
				}else{
					link_overscan_w = link_out_w;
					link_overscan_h = link_out_h;
				}
			}	
			
			link_cursor.pos_x = userinfo->pos_x	* link_out_w / src_out_w ;			
			link_cursor.pos_y = userinfo->pos_y * link_out_h / src_out_h ;	
			
			link_cursor.pos_x = link_out_w / 2 - link_overscan_w * (link_out_w / 2 - link_cursor.pos_x) / link_out_w ;
			link_cursor.pos_y = link_out_h / 2 - link_overscan_h * (link_out_h / 2 - link_cursor.pos_y) / link_out_h ;
			
			
			if(link_cursor.pos_x > (link_out_w + link_overscan_w) / 2 ){
				link_cursor.pos_x = (link_out_w + link_overscan_w) / 2;				
			}
			
			if(link_cursor.pos_y > (link_out_h + link_overscan_h) / 2 ){
				link_cursor.pos_y = (link_out_h + link_overscan_h) / 2;
			}
			
			
			if(link_cursor.pos_x > link_out_w - 64 ){
				link_cursor.pos_x = link_out_w - 64;
			}					
			if(link_cursor.pos_y > link_out_h - 64){
				link_cursor.pos_y = link_out_h - 64;
			}	
			
			if(link_dssdev->type == OWL_DISPLAY_TYPE_CVBS)
			{
				link_cursor.pos_y = (link_cursor.pos_y & (~0x01));					
			}			
			printk("link_cursor.pos_x %d  link_cursor.pos_y  %d\n",link_cursor.pos_x,link_cursor.pos_y);
			link_cursor.paddr = cursor.paddr;
			link_cursor.stride =  cursor.stride;
			link_cursor.enable =  cursor.enable;		
			link_ofbi->manager->set_cursor_info(link_ofbi->manager,&link_cursor);
			link_ofbi->manager->apply(link_ofbi->manager);
		}		
	}else{
		if(ovl_id > ofbi->num_overlays){
			return -1;
		}	
		
		ovl = ofbi->overlays[ovl_id];
			
		ovl->get_overlay_info(ovl, &info);
		
		info.paddr = userinfo->mem_off + ofbi->region->paddr;
		
		info.color_mode = userinfo->color_mode;
		info.img_width = userinfo->img_width;
		info.img_height = userinfo->img_height;
		
		info.xoff =  userinfo->xoff;
		info.yoff =  userinfo->yoff;
		info.width =  userinfo->width;
		info.height =  userinfo->height;
			
		info.pos_x = userinfo->pos_x;
		info.pos_y = userinfo->pos_y;
		info.out_width = userinfo->out_width;
		info.out_height = userinfo->out_height;	
					
		info.rotation =	userinfo->rotation;	
		
		info.global_alpha_en = userinfo->global_alpha_en;
		info.global_alpha =  userinfo->global_alpha; 
		info.pre_mult_alpha_en =  userinfo->pre_mult_alpha_en;
		
		DBG(" owlfb_set_overlay_info ovl_id %d info: color_mode 0x%x img_width 0x%x info.img_height 0x%x \n",ovl_id,info.color_mode,info.img_width,info.img_height);	
		DBG(" info: xoff 0x%x yoff 0x%x width 0x%x height 0x%x \n",info.xoff,info.yoff,info.width,info.height);	
		DBG(" info: pos_x 0x%x pos_y 0x%x out_width 0x%x out_height 0x%x \n",info.pos_x,info.pos_y,info.out_width,info.out_height);	
		DBG(" info: global_alpha_en 0x%x global_alpha 0x%x pre_mult_alpha_en 0x%x \n",info.global_alpha_en,info.global_alpha,info.pre_mult_alpha_en);	
		
		ovl->set_overlay_info(ovl, &info);
		
		if(!ovl->is_enabled(ovl)){
			ovl->enable(ovl);
		}		
		ovl->manager->apply(ovl->manager);		
		//ovl->manager->wait_for_go(ovl->manager);		
	}	
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
	struct owl_dss_device * device = owl_dss_find_device_by_type(get_current_display_type()); 
	struct display_private_info *new_private_info = (struct display_private_info *)&(disp_info->mPrivateInfo[0]);	
	DBG("get_info~~\n");	
	if(device 
		&& device->driver
		&& device->driver->get_effect_parameter){
			disp_info->mIcType = owl_de_get_ictype();	
			new_private_info->LCD_TYPE = device->type;			
			new_private_info->LCD_LIGHTENESS = device->driver->get_effect_parameter(device ,OWL_DSS_VIDEO_LIGHTNESS);
			new_private_info->LCD_SATURATION = device->driver->get_effect_parameter(device ,OWL_DSS_VIDEO_SATURATION);
			new_private_info->LCD_CONSTRAST = device->driver->get_effect_parameter(device ,OWL_DSS_VIDEO_CONSTRAST);
	}
	return 0;
}

int owlfb_set_effect_parameter(struct owlfb_disp_device * disp_info)
{	
	struct owl_dss_device * device = owl_dss_find_device_by_type(get_current_display_type()); 	
	struct display_private_info *new_private_info = (struct display_private_info *)&(disp_info->mPrivateInfo[0]);	
	int 	lightness = new_private_info->LCD_LIGHTENESS;
	int 	saturation = new_private_info->LCD_SATURATION;
	int 	contrast = new_private_info->LCD_CONSTRAST;
	DBG("set_info~~\n");
	if(device 
		&& device->driver
		&& device->driver->set_effect_parameter){
		switch(disp_info->mCmdMode){
			case SET_LIGHTENESS:
				  device->driver->set_effect_parameter(device ,OWL_DSS_VIDEO_LIGHTNESS, lightness);
				  printk("set_info~~ new_private_info->LCD_LIGHTENESS  %d\n", new_private_info->LCD_LIGHTENESS);
				  break;	
			case SET_SATURATION:
				  device->driver->set_effect_parameter(device ,OWL_DSS_VIDEO_SATURATION, saturation);
				  printk("set_info~~ new_private_info->LCD_SATURATION  %d\n", new_private_info->LCD_SATURATION);
				  break;	
			case SET_CONSTRAST:
				  device->driver->set_effect_parameter(device ,OWL_DSS_VIDEO_CONSTRAST, contrast);
				  printk("set_info~~ new_private_info->LCD_CONSTRAST  %d\n", new_private_info->LCD_CONSTRAST);
				  break;	
			case SET_DEFAULT:
				  device->driver->set_effect_parameter(device ,OWL_DSS_DEF_EFFECT, 0);
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
		struct owlfb_overlay_args overlay_args;
		struct owlfb_sync_info sync_info;
		struct owlfb_disp_histogram_info histogram_info;
		struct owl_gamma_info gamma_info;
		struct owlfb_display_info display_info;
		struct owlfb_hdmi_vid_info hdmi_vid;
		u32				crt;
	} p;

	int r = 0;

	switch (cmd) {
	/*this for linux overlay functions */
	case OWLFB_SYNC_GFX:
		DBG("ioctl SYNC_GFX\n");
		break;
	case OWLFB_OVERLAY_REQUEST:{		
		DBG("ioctl OWLFB_OVERLAY_REQUEST \n");
		if (copy_from_user(&p.overlay_args, (void __user *)arg, sizeof(p.overlay_args))){
			r = -EFAULT;
			break;
		}
				
		owlfb_request_overlay(fbi,&p.overlay_args);
		
		if(p.overlay_args.overlay_id >= 0){
			if (copy_to_user((void __user *)arg, &p.overlay_args,sizeof(struct owlfb_overlay_args))){
				 return -EFAULT;
			}
		}
		break;
	}		
	case OWLFB_OVERLAY_RELEASE:{
		if (copy_from_user(&p.overlay_args, (void __user *)arg, sizeof(p.overlay_args)) ){
			r = -EFAULT;
			break;
		}
		r = owlfb_release_overlay(fbi,p.overlay_args.overlay_id);
		DBG("ioctl OWLFB_OVERLAY_RELEASE \n"); 
		break;
	}
	case OWLFB_OVERLAY_ENABLE:{
		DBG("ioctl OWLFB_OVERLAY_ENABLE \n"); 
		if (copy_from_user(&p.overlay_args, (void __user *)arg, sizeof(p.overlay_args))) {
			r = -EFAULT;
			break;
		}
		r = owlfb_set_overlay_enable(fbi,p.overlay_args.overlay_id,true);
		break;
	}
	case OWLFB_OVERLAY_DISABLE:{
		DBG("ioctl OWLFB_OVERLAY_DISABLE \n"); 
		if (copy_from_user(&p.overlay_args, (void __user *)arg, sizeof(p.overlay_args))) {
			r = -EFAULT;
			break;
		}
		r = owlfb_set_overlay_enable(fbi,p.overlay_args.overlay_id,false);
		break;
	}
	case OWLFB_OVERLAY_GETINFO:{
		struct owlfb_overlay_info   overlay_info;
		if (copy_from_user(&p.overlay_args, (void __user *)arg, sizeof(p.overlay_args))) {
			r = -EFAULT;
			break;
		}
		
		DBG("ioctl OWLFB_OVERLAY_GETINFO \n");
		
		if(owlfb_get_overlay_info(fbi,p.overlay_args.overlay_id,&overlay_info) >= 0)
		{
			if(copy_to_user((void __user *)p.overlay_args.uintptr_overly_info,(void *)&overlay_info,sizeof(struct owlfb_overlay_info))){
	            return -EFAULT;
	        }
		} 
		break;	
	}
	case OWLFB_OVERLAY_SETINFO:{
		struct owlfb_overlay_info   overlay_info;
		DBG("ioctl OWLFB_OVERLAY_SETINFO \n"); 
		if (copy_from_user(&p.overlay_args, (void __user *)arg, sizeof(p.overlay_args))) {
			r = -EFAULT;
			break;
		}
		if (copy_from_user(&overlay_info, (void __user *)p.overlay_args.uintptr_overly_info, sizeof(struct owlfb_overlay_info))) {
			r = -EFAULT;
			break;
		}
		r = owlfb_set_overlay_info(fbi,p.overlay_args.overlay_id,&overlay_info);		
		break;
	}
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
			
	case OWLFB_GET_MAIN_DISPLAY_RES: {
		u16 xres, yres;	

		if (display == NULL) {
			r = -ENODEV;
			break;
		}
				
		/*if(display->driver->get_resolution){
			display->driver->get_resolution(display, &p.display_info.xres, &p.display_info.yres);
		}else{*/
			p.display_info.xres = fbdev->xres;
			p.display_info.yres = fbdev->yres;
		/*}*/
		
		p.display_info.virtual_xres = fbdev->xres;
		p.display_info.virtual_yres = fbdev->yres;		
		p.display_info.refresh_rate = fbdev->refresh;
		
		p.display_info.disp_type = display->type;
		
		printk("ioctl GET_DISPLAY_INFO display %p %s (%d x %d @ %d MHZ)\n",display, display->name,p.display_info.xres,p.display_info.yres,p.display_info.refresh_rate);
		
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

		dss_mgr_set_gamma_table(OWL_DSS_PATH1_ID, &p.gamma_info);
		break;
	}
	case OWLFB_GET_GAMMA_INFO: {
		DBG("ioctl OWLFB_SET_GAMMA_INFO, &p.gamma_info %p\n",&p.gamma_info);				
		dss_mgr_get_gamma_table(OWL_DSS_PATH1_ID, &p.gamma_info);

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
		struct owl_dss_device * device = owl_dss_find_device_by_type(OWL_DISPLAY_TYPE_HDMI);
		
		if(!device || !device->driver){
			dev_err(fbdev->dev, "device is NULL or no driver device %p \n",device);
			r = -EFAULT;
			break;
		}
		
		DBG("ioctl OWLFB_HDMI_SUPPORT_MODE  vid %d mode %d \n",p.hdmi_vid.vid,p.hdmi_vid.mode);	
		memset(support_vid, 0, sizeof(support_vid));
		
		if(device->driver->get_vid_cap){
			
			k = device->driver->get_vid_cap(device, support_vid);	
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
		struct owl_dss_device * device = owl_dss_find_device_by_type(OWL_DISPLAY_TYPE_HDMI);
		
		if(!device || !device->driver){
			dev_err(fbdev->dev, "device is NULL or no driver device %p \n",device);
			r = -EFAULT;
			break;
		}
				
		if(device->driver->get_cable_status){
			i = device->driver->get_cable_status(device);	
		}
		
		if (copy_to_user((void __user *)arg, &i, sizeof(int))){
			r = -EFAULT;
		}
		break;
	}
	case OWLFB_HDMI_SET_MODE:{		
		struct owl_dss_device * device = owl_dss_find_device_by_type(OWL_DISPLAY_TYPE_HDMI);
		
		if(!device || !device->driver){
			dev_err(fbdev->dev, "device is NULL or no driver device %p \n",device);
			r = -EFAULT;
			break;
		}

		if (copy_from_user(&p.hdmi_vid, (void __user *)arg, sizeof(p.hdmi_vid))) {
			r = -EFAULT;
			break;
		}
		printk("ioctl OWLFB_HDMI_SET_MODE vid %d \n",p.hdmi_vid.vid);
				
		if(device->driver->set_vid){
			device->driver->set_vid(device,p.hdmi_vid.vid);	
		}
		break;
	}	
	case OWLFB_HDMI_ON:{
		struct owl_dss_device * device = owl_dss_find_device_by_type(OWL_DISPLAY_TYPE_HDMI);
		
		if(!device || !device->driver){
			dev_err(fbdev->dev, "device is NULL or no driver device %p \n",device);
			r = -EFAULT;
			break;
		}
		DBG("ioctl OWLFB_HDMI_ON\n");
		if(device->driver->enable){
			device->driver->enable(device);
		}
		atomic_set(&want_close_external_devices,false);
		unlock_fb_info(fbi);
		msleep(500);
		lock_fb_info(fbi);
		break;
	}	
	case OWLFB_HDMI_OFF:{
		DBG("ioctl OWLFB_HDMI_OFF\n");
		atomic_set(&want_close_external_devices,true);
		break;
	}
	case OWLFB_CVBS_ON:{
		struct owl_dss_device * device = owl_dss_find_device_by_type(OWL_DISPLAY_TYPE_CVBS);
		atomic_set(&want_close_external_devices,false);
		if(!device || !device->driver){
			dev_err(fbdev->dev, "device is NULL or no driver device %p \n",device);
			r = -EFAULT;
			break;
		}
		printk("ioctl OWLFB_CVBS_ON\n");
		if(device->driver->enable){
			device->driver->enable(device);
		}
		
		break;
	}	
	case OWLFB_CVBS_OFF:{
		printk("ioctl OWLFB_CVBS_OFF\n");
		atomic_set(&want_close_external_devices,true);
		break;
	}
	
	case OWLFB_HDMI_DISABLE:{
		int enable = 0;
		struct owl_dss_device * device = owl_dss_find_device_by_type(OWL_DISPLAY_TYPE_HDMI);
		
		if(!device || !device->driver){
			dev_err(fbdev->dev, "device is NULL or no driver \n");
			r = -EFAULT;
			break;
		}
		
		DBG("ioctl OWLFB_HDMI_DISABLE\n");
		if (copy_from_user(&enable, (void __user *)arg, sizeof(int))) {
			r = -EFAULT;
			break;
		}
		if(enable){
			if(device->driver->enable_hpd){
				device->driver->enable_hpd(device, 1);
			}			
		}else{
			if(device->driver->enable_hpd){
				device->driver->enable_hpd(device, 0);
			}		
		}
		break;
	}
	case OWLFB_HDMI_GET_VID:{
		int vid = -1;		
		struct owl_dss_device * device = owl_dss_find_device_by_type(OWL_DISPLAY_TYPE_HDMI);
		
		if(!device || !device->driver){
			dev_err(fbdev->dev, "device is NULL or no driver \n");
			r = -EFAULT;
			break;
		}
		
		if(device->driver->get_vid){
			device->driver->get_vid(device, (int *)&vid);	
		}		
		printk("OWLFB_HDMI_GET_VID vid %d ",vid);
		if (copy_to_user((void __user *)arg, &vid, sizeof(int))){
			r = -EFAULT;
		}
		break;
	}	
	/**********************CVBS***************************/
	case OWLFB_CVBS_ENABLE:{
		int enable = -1;
		struct owl_dss_device * device = owl_dss_find_device_by_type(OWL_DISPLAY_TYPE_CVBS);
		
		if(!device || !device->driver){
			dev_err(fbdev->dev, "device is NULL or no driver\n");
			r = -EFAULT;
			break;
		}
		
		if (copy_from_user(&enable, (void __user *)arg, sizeof(int))){
			r = -EFAULT;
			break;
		}
	
		if(enable){
			if(device->driver->enable_hpd){
				device->driver->enable_hpd(device, 1);
			}			
		}else{
			if(device->driver->enable_hpd){
				device->driver->enable_hpd(device, 0);
			}		
		}
		break;
	}

	case OWLFB_CVBS_GET_VID:{
		int vid = -1;
		DBG("enter OWLFB_CVBS_GET_VID\n");
		struct owl_dss_device * device = owl_dss_find_device_by_type(OWL_DISPLAY_TYPE_CVBS);
		
		if(device && device->driver && device->driver->get_vid){
			device->driver->get_vid(device, (int *)&vid);	
		}
		
		if (copy_to_user((void __user *)arg, &vid, sizeof(int))){
			r = -EFAULT;
		}
		break;
	}	
	
	case OWLFB_CVBS_SET_VID:{
		int vid = -1;
		struct owl_dss_device * device = owl_dss_find_device_by_type(OWL_DISPLAY_TYPE_CVBS);
		
		if(!device || !device->driver){
			dev_err(fbdev->dev, "device is NULL or no driver\n");
			r = -EFAULT;
			break;
		}

		if (copy_from_user(&vid, (void __user *)arg, sizeof(int))){			
			r = -EFAULT;
			break;
		}	
		DBG("OWLFB_CVBS_SET_VID  vid %d \n",vid);
		
		if(device->driver->set_vid){
			device->driver->set_vid(device, vid);	
		}
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


