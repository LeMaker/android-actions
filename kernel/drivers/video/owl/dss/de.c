/*
 * linux/drivers/video/owl/dss/de.c
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: Lipeng  <lipeng@actions-semi.com>
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
#define DSS_SUBSYS_NAME "DE"

#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/export.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/hardirq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/of_device.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>

#include <mach/clkname.h>
#include <mach/module-owl.h>

#include <video/owldss.h>

#include "dss.h"
#include "dss_features.h"
#include "de.h"


/* some convenient pointers */
static struct owl_de_pdata		*de_pdata = NULL;
static const struct owl_de_hwops	*de_ops = NULL;

static bool 				de_boot_inited = false;

/*===================={ internal interfaces======================*/

bool de_channel_check_boot_inited(enum owl_de_path_id path)
{
	return de_ops->path_is_enabled(path);
}

/*****************TODO*********************/
#define DECLK1_MAX		300000000
#define DECLK2_MAX		150000000
#define DECLK_TARGET		300000000

static void de_clk_init(void)
{
	struct clk      *parent_clk;
	struct clk      *de1_clk;
	struct clk      *de2_clk;
	struct clk      *nic_clk;

	unsigned long   parent_rate;
	unsigned long   declk1_rate;
	unsigned long   declk2_rate;
	unsigned long   nic_rate;

	/* CLKNAME_DEVPLL is used, TODO(CLKNAME_DISPLAYPLL) */
	parent_clk  = clk_get(NULL, CLKNAME_DEVPLL);
	parent_rate = clk_get_rate(parent_clk);
	DSSINFO("de parent pll is %ldhz\n", parent_rate);

	nic_clk     = clk_get(NULL, CLKNAME_NIC_CLK);
	nic_rate    = clk_get_rate(nic_clk);

	de1_clk     = clk_get(NULL, CLKNAME_DE1_CLK);
	clk_set_parent(de1_clk, parent_clk);

	de2_clk     = clk_get(NULL, CLKNAME_DE2_CLK);
	clk_set_parent(de2_clk, parent_clk);

#ifdef DECLK_TARGET
	if (nic_rate < DECLK_TARGET) {
		printk(KERN_ERR "DE clk > nic clk!!\n");
		printk(KERN_ERR "DE clk target %dhz\n", DECLK_TARGET);
		printk(KERN_ERR "nic clk %lu\n", nic_rate);
	}
	declk1_rate = DECLK_TARGET;
#else
	/*de clk1 will be a little lower than nic clk*/
	if (nic_rate > DECLK1_MAX) {
		declk1_rate = DECLK1_MAX;
	} else {
		declk1_rate = nic_rate - 1;
	}
#endif

	/* de clk2 will not be higher than de clk1*/
	declk1_rate = clk_round_rate(de1_clk, declk1_rate);
	if (declk1_rate > DECLK2_MAX) {
		declk2_rate = DECLK2_MAX;
	} else {
		declk2_rate = declk1_rate;
	}

	clk_set_rate(de1_clk, declk1_rate);
	clk_set_rate(de2_clk, declk2_rate);

	clk_prepare(parent_clk);
	clk_enable(parent_clk);

	clk_prepare(de1_clk);
	clk_enable(de1_clk);

	clk_prepare(de2_clk);
	clk_enable(de2_clk);

	module_clk_enable(MODULE_CLK_DE);
}

#if 0
static void de_clk_deinit(void)
{
	/* TODO */
}
#endif

static inline void de_clk_enable(void)
{
	module_clk_enable(MODULE_CLK_DE);
}

static inline void de_clk_disable(void)
{
	DSSINFO("%s\n", __func__);
	module_clk_disable(MODULE_CLK_DE);
}

static inline void de_clk_reset(void)
{
    DSSDBG("##DE RESET DE!!!######\n");
    module_reset(MODULE_RST_DE);
}

static void de_set_irqs(void)
{
	int 			i;
	u32 			mask, old_mask, enable;
	struct de_isr_data	*isr_data;
	
	mask = 0;

	for (i = 0; i < DE_MAX_NR_ISRS; i++) {
		isr_data = &de_pdata->registered_isr[i];

		if (isr_data->isr == NULL) {
			continue;
		}

		mask |= isr_data->mask;
	}

	old_mask = de_ops->irq_status_get();

	/* clear the irqstatus for newly enabled irqs */
	de_ops->irq_status_set((mask ^ old_mask) & mask);

	de_pdata->irq_mask = mask;
	enable = de_ops->irq_mask_to_enable(mask);

	de_ops->irq_enable_set(enable);
}

static irqreturn_t de_irq_handler(int irq, void *dev)
{
	int			i;
	u32 			irqstatus, vb_mask, masks, val;
	u32 			handledirqs = 0;
	u32 			unhandled_errors;
	struct de_isr_data 	*isr_data;
	struct de_isr_data 	registered_isr[DE_MAX_NR_ISRS];
	
	irqstatus = de_ops->irq_status_get();
	
	//printk("de_irq_handler irqstatus 0x%x \n",irqstatus);

	spin_lock(&de_pdata->irq_lock);

	masks = de_pdata->irq_mask;

	/* IRQ is not for us */
	if (!(irqstatus & masks)) {
		spin_unlock(&de_pdata->irq_lock);
		return IRQ_NONE;
	}

	/* make a copy and unlock, so that isrs can unregister themselves */
	memcpy(registered_isr, de_pdata->registered_isr, sizeof(registered_isr));
	

	spin_unlock(&de_pdata->irq_lock);


	/* 
	 * Ack the interrupt. Do it here before clocks are possibly turned off.
	 * NOTE: do not clear VB bits
	 */
	de_ops->irq_status_set(masks & irqstatus);
	/* flush posted write */
	de_ops->irq_status_get();
	
	/* wait for VB according to PRELINE irq status */
	vb_mask = de_ops->irq_mask_to_vb_mask(masks & irqstatus);

	i = 100;
	while (i--) {
		val = de_ops->irq_status_get() & vb_mask;
		if (val == vb_mask) {
			break;
		}
		udelay(5);
	};
	de_pdata->vb_timeout_cnt += (val != vb_mask);

	/* handling preline irq */
	for (i = 0; i < DE_MAX_NR_ISRS; i++) {
		isr_data = &registered_isr[i];

		if (!isr_data->isr) {
			continue;
		}

		if (isr_data->mask & irqstatus) {
		    isr_data->isr(isr_data->irq, isr_data->arg);
		    handledirqs |= isr_data->mask;
		}
	}


	unhandled_errors = irqstatus & masks & ~handledirqs;
	if (unhandled_errors) {
		DSSINFO("unhandled irq errors: %x\n", unhandled_errors);
	}

	return IRQ_HANDLED;
}

static int de_irq_init(void)
{
	unsigned long flags;

	spin_lock_irqsave(&de_pdata->irq_lock, flags);

	memset(de_pdata->registered_isr, 0, sizeof(de_pdata->registered_isr));
	
	de_set_irqs();
	
	/* TODO */
	if (request_irq(OWL_IRQ_DE, de_irq_handler ,IRQF_DISABLED, "asoc_de", NULL) != 0) {
		DSSERR("DE request interrupt %d failed\n", OWL_IRQ_DE);
		return -EBUSY;
	}
	de_pdata->irq = OWL_IRQ_DE;

	DSSINFO("de irq init ok\n");
	spin_unlock_irqrestore(&de_pdata->irq_lock, flags);

	return 0;
}

static void de_irq_deinit(void)
{
	free_irq(de_pdata->irq, de_pdata->pdev);
}



/*===================internal interfaces end }===================*/

/*==================={ external interfaces=======================*/
int owl_de_register_isr(enum de_irq_type irq, owl_de_isr_t isr, void *arg)
{
	int 			i, ret;
	unsigned long 		flags;
	struct de_isr_data 	*isr_data;

	/* convert irq to mask */
	u32			mask = de_ops->irq_to_mask(irq);

	if (isr == NULL) {
		return -EINVAL;
	}

	spin_lock_irqsave(&de_pdata->irq_lock, flags);

	/* check for duplicate entry */
	for (i = 0; i < DE_MAX_NR_ISRS; i++) {
		isr_data = &de_pdata->registered_isr[i];
		if (isr_data->irq == irq && isr_data->isr == isr
			&& isr_data->arg == arg) {
			ret = -EINVAL;
			goto err;
		}
	}

	isr_data = NULL;
	ret = -EBUSY;

	for (i = 0; i < DE_MAX_NR_ISRS; i++) {
		isr_data = &de_pdata->registered_isr[i];

		if (isr_data->isr != NULL) {
			continue;
		}

		isr_data->irq = irq;
		isr_data->isr = isr;
		isr_data->arg = arg;
		isr_data->mask = mask;
		ret = 0;

		break;
	}

	if (ret) {
		goto err;
	}

	de_set_irqs();

	spin_unlock_irqrestore(&de_pdata->irq_lock, flags);

	return 0;
err:
	spin_unlock_irqrestore(&de_pdata->irq_lock, flags);

	return 0;
}

int owl_de_unregister_isr(enum de_irq_type irq, owl_de_isr_t isr, void *arg)
{
	int 			i;
	unsigned long 		flags;
	int			ret = -EINVAL;
	struct de_isr_data	*isr_data;

	spin_lock_irqsave(&de_pdata->irq_lock, flags);

	for (i = 0; i < DE_MAX_NR_ISRS; i++) {
		isr_data = &de_pdata->registered_isr[i];
		if (isr_data->irq != irq || isr_data->isr != isr
			|| isr_data->arg != arg) {
			continue;
		}

		/* found the correct isr */
		isr_data->irq = DE_IRQ_MAX;
		isr_data->isr = NULL;
		isr_data->arg = NULL;
		isr_data->mask = 0;

		ret = 0;
		break;
	}

	if (ret == 0) {
		de_set_irqs();
	}

	spin_unlock_irqrestore(&de_pdata->irq_lock, flags);

	return 0;
}

enum de_irq_type de_mgr_get_vsync_irq(enum owl_display_type type){
	switch(type){
		case OWL_DISPLAY_TYPE_LCD:
			return DE_IRQ_LCD_PRE;
		case OWL_DISPLAY_TYPE_DSI:
			return DE_IRQ_DSI_PRE;
		case OWL_DISPLAY_TYPE_HDMI:
			return DE_IRQ_HDMI_PRE;
		case	OWL_DISPLAY_TYPE_CVBS:
			return DE_IRQ_CVBS_PRE;
		default:
			return DE_IRQ_LCD_PRE;
	}
}

static DECLARE_COMPLETION(completion);

static void de_irq_wait_handler(enum de_irq_type irq, void *data)
{
	complete((struct completion *)data);
}

void dehw_mgr_wait_for_go(enum owl_de_path_id path_id)
{
	ktime_t expires ,end ;
	u32 val;
	int i = 70;
	ktime_t being = ktime_get();
	do{				
		val = de_ops->fcr_get(path_id);
		
		if(val == 0 || i == 0){
			break;
		}
						
        expires = ktime_add_ns(ktime_get(), 200 * 1000);   

	    set_current_state(TASK_UNINTERRUPTIBLE);
	    
	    i--;
	    
	    while(schedule_hrtimeout(&expires, HRTIMER_MODE_ABS) != 0);
	    
    }while(val); 
    end = ktime_get();
    if(ktime_to_ns(end) - ktime_to_ns(being) > 10000000){
    	//printk("dehw_mgr_wait_for_go ~~ %lld (ns)\n",(ktime_to_ns(end) - ktime_to_ns(being)));   
	}
}


int de_wait_for_irq_interruptible_timeout(enum de_irq_type irq,enum owl_de_path_id path_id, unsigned long timeout)
{
	int r;
	//DSSINFO("de_wait_for_irq_interruptible_timeout path_id %d  irq 0x%x  irq status 0x%x\n",path_id,irq,de_ops->irq_status_get());	
	r = owl_de_register_isr(irq, de_irq_wait_handler, &completion);
	
	if (r) {
			return r;
	}
	
	timeout = wait_for_completion_interruptible_timeout(&completion, timeout);
	
	owl_de_unregister_isr(irq, de_irq_wait_handler, &completion);
	
	if (timeout == 0) {
		return -ETIMEDOUT;
	}
	
	if (timeout == -ERESTARTSYS) {
		return -ERESTARTSYS;
	}
	
	
	dehw_mgr_wait_for_go(path_id);	
	return 0;
}

void owl_de_get_histogram_info(u32 * hist, u32 * totaly)
{
}
EXPORT_SYMBOL(owl_de_get_histogram_info);

bool de_is_vb_valid(enum owl_de_path_id path, enum owl_display_type type)
{
	u32 mask, vb_mask;

	mask = de_ops->irq_to_mask(de_mgr_get_vsync_irq(type));
	vb_mask = de_ops->irq_mask_to_vb_mask(mask);

	return (de_ops->irq_status_get() & vb_mask);
}

bool owl_de_is_atm7059tc(void)
{
	return (DE_HW_ID_ATM7059TC == de_pdata->id);
}

int owl_de_get_ictype(void)
{
	if(DE_HW_ID_ATM7059A == de_pdata->id){
		return 0x705A;
	}else if(DE_HW_ID_ATM9009A == de_pdata->id){
		return 0x900A;
	}else{
		return 0x705A;
	}
}
EXPORT_SYMBOL(owl_de_get_ictype);

void de_set_gamma_table(enum owl_de_path_id channel, u32 *gamma)
{
	DSSDBG("set gamma\n");
	de_ops->set_gamma_table(channel, gamma);
}

void de_get_gamma_table(enum owl_de_path_id channel, u32 *gamma)
{
	de_ops->get_gamma_table(channel, gamma);
}

void de_enable_gamma_table(enum owl_de_path_id channel, bool enable)
{
	DSSDBG("enable gamma, %d\n", enable);
	de_ops->enable_gamma_table(channel, enable);
}


void de_mgr_set_path_size(enum owl_de_path_id channel, u16 width, u16 height)
{
	de_ops->path_size_set(channel, width, height);
}

void de_mgr_set_device_type(enum owl_de_path_id channel,enum owl_display_type type)
{
	/* TODO */
	de_ops->display_type_set(channel, type);
}

void de_mgr_enable(enum owl_de_path_id channel, bool enable)
{
	de_ops->path_enable(channel, enable);
}

void de_mgr_set_dither(enum owl_de_path_id channel, enum owl_dither_mode mode)
{
	de_ops->dither_set(channel, mode);
}

void de_mgr_enable_dither(enum owl_de_path_id channel, bool enable)
{
	de_ops->dither_enable(channel, enable);
}

bool de_mgr_is_enabled(enum owl_de_path_id channel)
{
	return de_ops->path_is_enabled(channel);
}

void de_mgr_go(enum owl_de_path_id channel)
{
	de_ops->fcr_set(channel);
}

void de_mgr_setup(enum owl_de_path_id channel, struct owl_overlay_manager_info *info)
{
	/* TODO */
}	

void de_mgr_cursor_setup(enum owl_de_path_id channel, struct owl_cursor_info *info)
{
	if(!info->enable){
		de_ops->curosr_enable(channel,false);
	}else{
		printk("de_mgr_cursor_setup info->pos_x 0x%x info->pos_y 0x%x\n ",info->pos_x,info->pos_y);
		de_ops->curosr_set_position(channel,info->pos_x,info->pos_y);
		de_ops->curosr_set_addr(channel,info->paddr);
		de_ops->curosr_set_str(channel,info->stride);
		de_ops->curosr_enable(channel,true);
	}	
}	

int de_ovl_setup(enum owl_de_path_id channel, enum owl_plane plane,
	   	 struct owl_overlay_info *oi, bool ilace)
{
    	bool mmu_enable = false;

	/* 
	 * some soc does not support RGBX format, 
	 * we can use global alpha with 0xff to emulate 'X'
	 */
	bool alpha_en = false;

    	u16 outw, outh;
	int ret;

	struct owl_overlay *ovl = owl_dss_get_overlay(plane);

	DSSDBGF("channel %d, plnae %d, pa %lx, pa_uv %x, sw %d, %d,%d, %dx%d -> "
	    "%dx%d, cmode %x, rot %d, mir %d, ilace %d\n",
	    channel, plane, oi->paddr, 0,
	    oi->screen_width, oi->pos_x, oi->pos_y, oi->width, oi->height,
	    oi->out_width, oi->out_height, oi->color_mode, oi->rotation,
	    0, ilace);

	if (oi->paddr == 0 && (oi->vaddr != 0 || oi->buffer_id >= 0)) {
		/* MMU */
		u32 da = 0;

		if (oi->vaddr != 0)
			ret = mmu_va_to_da(oi->vaddr, oi->img_width
					* oi->img_height * 3 / 2, &da);		
		else
			ret = mmu_fd_to_da(oi->buffer_id, &da);
		
	
		/* MMU map error, or we do not support MMU, BUG */
		if (ret) {
			DSSERR("Do not support MMU or MMU error!\n");
			BUG();
		}
		
		//owl_de_mmu_enable(plane, true);
		oi->paddr = da;
		mmu_enable = true;
		DSSDBG("mmu_enable %d , oi->vaddr 0x%lx da 0x%x\n",
			mmu_enable, oi->vaddr, da);
	}


	outw = (oi->out_width == 0 ? oi->width : oi->out_width);
	outh = (oi->out_height == 0 ? oi->height : oi->out_height);

	if (ilace) {
		oi->height 	/= 2;
		oi->pos_y 	/= 2;
		outh 		/= 2;
		DSSDBG("adjusting for ilace: height %d, pos_y %d, out_height %d\n",
			oi->height, oi->pos_y, outh);
	}
	DSSDBG("dss_feat_color_mode_supported plane %d oi->color_mode %d\n",
		plane, oi->color_mode);
	if (!dss_feat_color_mode_supported(plane, oi->color_mode)) {
		return -EINVAL;
	}

	alpha_en = de_ops->format_set(plane, oi->color_mode);

	/* set fb addr and bypass according to color mode */
	switch(oi->color_mode) {
	case OWL_DSS_COLOR_RGB16:
	case OWL_DSS_COLOR_BGR16:
	case OWL_DSS_COLOR_ARGB16:
	case OWL_DSS_COLOR_ABGR16:
	case OWL_DSS_COLOR_RGBA16:
	case OWL_DSS_COLOR_BGRA16:
		de_ops->fb_addr_set(plane, DE_VIDEO_FB0,
			(void *)oi->paddr + oi->yoff * oi->img_width * 2
					+ oi->xoff * 2);

		/* TODO if we need effective adjust */
		if(oi->lightness != 0x80 || oi->saturation != 0x07 || oi->contrast != 0x07){
		    de_ops->bypass_enable(plane, false);
		}else{
		    de_ops->bypass_enable(plane, true);
		}
		break;
	case OWL_DSS_COLOR_ARGB32:
	case OWL_DSS_COLOR_ABGR32:
	case OWL_DSS_COLOR_RGBA32:
	case OWL_DSS_COLOR_BGRA32:
	case OWL_DSS_COLOR_RGBX32:
	case OWL_DSS_COLOR_XBGR32:
    case OWL_DSS_COLOR_XRGB32:
		de_ops->fb_addr_set(plane, DE_VIDEO_FB0,
			(void *)oi->paddr + oi->yoff * oi->img_width * 4
					+ oi->xoff * 4);

		/* TODO if we need effective adjust */
		if(oi->lightness != 0x80 || oi->saturation != 0x07 || oi->contrast != 0x07){
		    de_ops->bypass_enable(plane, false);
		}else{
		    de_ops->bypass_enable(plane, true);
		}
		break;
	case OWL_DSS_COLOR_YU12:
	case OWL_DSS_COLOR_NV21:
	{
 		
 		void *ba0 = (void *)oi->paddr;
		void *ba1 = ba0 + oi->img_width * oi->img_height;
		void *ba2 = ba1 + oi->img_width * oi->img_height / 4;
		/* adjust for offset */
		ba0 += (oi->yoff * oi->img_width + oi->xoff);
		ba1 += (oi->yoff * oi->img_width / 4 + oi->xoff / 2);
		ba2 += (oi->yoff * oi->img_width / 4 + oi->xoff / 2);

		de_ops->fb_addr_set(plane, DE_VIDEO_FB0, ba0);

		if (oi->color_mode == OWL_DSS_COLOR_NV12) {
			de_ops->fb_addr_set(plane, DE_VIDEO_FB1, ba2);
			de_ops->fb_addr_set(plane, DE_VIDEO_FB2, ba1);
		} else {
			de_ops->fb_addr_set(plane, DE_VIDEO_FB1, ba1);
			de_ops->fb_addr_set(plane, DE_VIDEO_FB2, ba2);
		}

		de_ops->bypass_enable(plane, false);
		break;
	}
	case OWL_DSS_COLOR_NV12:
	case OWL_DSS_COLOR_NU21:
	{
		void *ba0 = (void *)oi->paddr;
		void *ba1 = ba0 + oi->img_width * oi->img_height;

		/* adjust for offset */
		ba0 += (oi->yoff * oi->img_width + oi->xoff);
		ba1 += (oi->yoff * oi->img_width / 2 + oi->xoff);

		de_ops->fb_addr_set(plane, DE_VIDEO_FB0, ba0);
		de_ops->fb_addr_set(plane, DE_VIDEO_FB1, ba1);

		de_ops->bypass_enable(plane, false);
		break;
	}
	default:
		DSSERR("fomat %d not support\n", oi->color_mode);
		BUG();
		break;
	}

	DSSDBG("set isize plane %d oi->width %d  oi->height %d \n",
	    	plane, oi->width, oi->height);
	de_ops->isize_set(plane, oi->width, oi->height);

	DSSDBG("set str plane %d oi->color_mode %d oi->img_width %d \n",
		plane, oi->color_mode, oi->img_width);
	de_ops->str_set(plane, oi->color_mode,oi->img_width);

	DSSDBG("set pos plane %d oi->pos_x %d oi->pos_y %d \n",
		plane, oi->pos_x, oi->pos_y);
    	de_ops->position_set(channel, plane, oi->pos_x, oi->pos_y);

	if (ovl->caps & OWL_DSS_OVL_CAP_SCALE) {
		DSSDBG("de_ovl_set_scaling \n");
		de_ops->scaling_set(plane, oi->width, oi->height, outw, outh, ilace);
	}

	DSSDBG("set osize plane %d oi->width %d  oi->height %d \n", plane, outw, outh);
	de_ops->osize_set(plane, outw, outh);

	DSSDBG("set plane %d oi->rotation %d \n", plane, oi->rotation);
	de_ops->rotation_set(plane, oi->rotation);
	
	DSSDBG("set plane %d lightness %d \n", plane, oi->lightness);
	de_ops->lightness_set(plane, oi->lightness);
	
	DSSDBG("set plane %d  saturation %d \n", plane, oi->saturation);
	de_ops->saturation_set(plane, oi->saturation);
	
	DSSDBG("set plane %d contrast %d \n", plane, oi->contrast);
	de_ops->contrast_set(plane, oi->contrast);

	if (alpha_en) {
		oi->global_alpha_en = true;
		oi->global_alpha = 0xff;
		oi->pre_mult_alpha_en = false;
	}
	DSSDBG("set alpha %d value %d is premult %d\n", plane,
			oi->global_alpha, oi->pre_mult_alpha_en);
	de_ops->alpha_set(channel, plane, oi->global_alpha,
				oi->global_alpha_en, oi->pre_mult_alpha_en);
	
	de_ops->critical_set(channel, plane);
	
	DSSDBG("de_ovl_setup ok\n");
	return 0;
}

int owl_de_mmu_config(u32 base_addr)
{
	return de_ops->mmu_config(base_addr);
}

int owl_de_mmu_enable(enum owl_plane plane, bool enable)
{
	return de_ops->mmu_enable(plane, enable);
}

int de_ovl_enable(enum owl_de_path_id channel, enum owl_plane plane, bool enable)
{
    	de_ops->video_enable(channel, plane, enable);

	return 0;
}


void owl_de_suspend(void)
{
	DSSINFO("%s\n", __func__);
	de_ops->suspend(de_pdata->pdev, PMSG_SUSPEND);

}

/* 
 * temp in here, for DSS's suspend/resume
 * I have to treat differently for normal boot and mini-charger boot,
 * it's an awlful design, should be discarded later!
 */
static int bootmode = 0;

static int __init bootmode_setup(char *str)
{
	if (strcmp(str, "charger") == 0) {
		pr_info("mini charger boot\n");
		bootmode = OWL_DSS_BOOT_CHARGER;
	}else if(strcmp(str, "recovery1") == 0 || strcmp(str, "recovery2") == 0 ){
		pr_info("recovery boot\n");
		bootmode = OWL_DSS_BOOT_RECOVERY;
	}else{
		pr_info("Normal boot\n");
		bootmode = OWL_DSS_BOOT_NORMAL;
	}

	return 1;
}
__setup("androidboot.mode=", bootmode_setup);
 
int owl_de_get_boot_mode(void)
{
	return bootmode;
} 
EXPORT_SYMBOL(owl_de_get_boot_mode);
void owl_de_resume(void)
{
	DSSINFO("%s\n", __func__);

	de_ops->resume(de_pdata->pdev);

	/* make sure gamma is disabled, awful design */
	de_enable_gamma_table(OWL_DSS_PATH1_ID, false);
	de_enable_gamma_table(OWL_DSS_PATH2_ID, false);

	/* mini charger boot, set path0's FCR, TODO */
	if (bootmode == 1)
		de_ops->fcr_set(0);
}


/*===================external interfaces end }====================*/

/*==================={ driver attributes====================*/
/*
 * regitsers dump and write
 */
static ssize_t de_regs_show(struct device_driver *drv, char *buf)
{
	de_ops->dump_regs();

	return 0;
}

static ssize_t de_regs_store(struct device_driver *drv,
				const char *buf, size_t count)
{
	return de_ops->write_regs(buf, count);
}
static DRIVER_ATTR(regs, S_IWUSR | S_IRUGO,
		de_regs_show, de_regs_store);

static ssize_t de_vb_timout_cnt_show(struct device_driver *ddp, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", de_pdata->vb_timeout_cnt);
}
static DRIVER_ATTR(vb_timeout_cnt, S_IRUGO, de_vb_timout_cnt_show, NULL);



/*===================driver attributes end }==================*/


/* TODO */
static const struct of_device_id owl_de_match[] = {
#ifdef CONFIG_VIDEO_OWL_DE_ATM7059
	{
		.compatible	= "actions,atm7059tc-de",
		.data		= &owl_de_atm7059,
	},
	{
		.compatible	= "actions,atm7059a-de",
		.data		= &owl_de_atm7059,
	},
#endif
#ifdef CONFIG_VIDEO_OWL_DE_ATM9009
	{
		.compatible	= "actions,atm9009a-de",
		.data		= &owl_de_atm9009,
	},
#endif
};

static int __init owl_de_probe(struct platform_device *pdev) {
	struct device 			*dev = &pdev->dev;
	struct resource 		*res;
	int				ret = 0;

	const struct of_device_id 	*match;

	DSSINFO("%s, pdev = 0x%p\n", __func__, pdev);

	match = of_match_device(of_match_ptr(owl_de_match), dev);
	if (!match) {
		DSSERR("Error: No device match found\n");
		return -ENODEV;
	}


	/* get DE's private data pointer */
	de_pdata = (struct owl_de_pdata *)match->data;
	de_ops = de_pdata->hwops;
	if (!de_pdata) {
		DSSERR("private data is NULL\n");
		return -EINVAL;
	}

	/* set de hw id */
	if (strcmp(match->compatible, "actions,atm7059tc-de") == 0)
		de_pdata->id = DE_HW_ID_ATM7059TC;
	else if (strcmp(match->compatible, "actions,atm7059a-de") == 0)
		de_pdata->id = DE_HW_ID_ATM7059A;
	else if (strcmp(match->compatible, "actions,atm9009a-de") == 0)
		de_pdata->id = DE_HW_ID_ATM9009A;

	de_pdata->pdev 	= pdev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		DSSERR("can't get IORESOURCE_MEM\n");
		return -ENODEV;
	}

	de_pdata->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(de_pdata->base)) {
		DSSERR("can't remap IORESOURCE_MEM\n");
		return PTR_ERR(de_pdata->base);
	}
	DSSDBG("de base is 0x%p\n", de_pdata->base);
	
	de_boot_inited = de_channel_check_boot_inited(OWL_DSS_PATH1_ID) 
	                 | de_channel_check_boot_inited(OWL_DSS_PATH2_ID);
	
	DSSINFO("DE INITED FROM UBOOT ?? %d \n", de_boot_inited);

	if (!de_boot_inited) {
		de_clk_init();
	}

	ret = de_irq_init();
	if (ret) {
		return ret;
	}

	mmu_init();

	return 0;
}

static int __exit owl_de_remove(struct platform_device *pdev)
{
	de_irq_deinit();

	de_clk_disable();

	return 0;
}


static struct platform_driver owl_de_driver = {
	.driver = {
		.name		= "de-owl",
		.owner		= THIS_MODULE,
		.of_match_table	= owl_de_match,
	},
	.probe			= owl_de_probe,
	.remove			= owl_de_remove,
};

int __init owl_de_init(void) {
	int ret = 0;
	
	ret = platform_driver_register(&owl_de_driver);
	if (ret) {
		DSSERR("Failed to register de platform driver\n");
		return ret;
	}

	/* and regs attribute */
	ret = driver_create_file(&owl_de_driver.driver, &driver_attr_regs);
	ret |= driver_create_file(&owl_de_driver.driver, &driver_attr_vb_timeout_cnt);
	if (ret) {
		DSSERR("driver_create_file failed!!\n");
	}

	return 0;
}

void __exit owl_de_exit(void) {
	platform_driver_unregister(&owl_de_driver);

	driver_remove_file(&owl_de_driver.driver, &driver_attr_regs);
	driver_remove_file(&owl_de_driver.driver, &driver_attr_vb_timeout_cnt);
}
