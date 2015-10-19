/*
 * linux/drivers/video/owl/dss/de_atm9009.c
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: lipeng<lipeng@actions-semi.com>
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

#include <mach/clkname.h>

#include <video/owldss.h>

#include "dss.h"
#include "dss_features.h"
#include "de.h"
#include "de_atm9009.h"

#define DE_ATM9009_PATH_NUM		(2)
#define DE_ATM9009_VIDEO_NUM		(4)

const static struct owl_de_path_pdata de_atm9009_paths[] = {
	{
		.id			= OWL_DSS_CHANNEL_LCD,
		.supported_displays	= OWL_DISPLAY_TYPE_LCD
					| OWL_DISPLAY_TYPE_DSI
					| OWL_DISPLAY_TYPE_EDP,
	},
	{
		.id			= OWL_DSS_CHANNEL_DIGIT,
		.supported_displays	= OWL_DISPLAY_TYPE_HDMI
					| OWL_DISPLAY_TYPE_LCD
					| OWL_DISPLAY_TYPE_DSI,
	},
};

const static struct owl_de_video_pdata de_atm9009_videos[] = {
	{
		.id			= OWL_DSS_VIDEO1,
		.supported_colors	=
				OWL_DSS_COLOR_RGB16 | OWL_DSS_COLOR_BGR16
        			| OWL_DSS_COLOR_ARGB32 | OWL_DSS_COLOR_ABGR32
        			| OWL_DSS_COLOR_RGBA32 | OWL_DSS_COLOR_BGRA32
        			| OWL_DSS_COLOR_YU12 | OWL_DSS_COLOR_NV12 | OWL_DSS_COLOR_NV21
        			| OWL_DSS_COLOR_ARGB16 | OWL_DSS_COLOR_ABGR16
        			| OWL_DSS_COLOR_RGBA16 | OWL_DSS_COLOR_BGRA16 ,
	},
	{
		.id			= OWL_DSS_VIDEO2,
		.supported_colors	=
				OWL_DSS_COLOR_RGB16 | OWL_DSS_COLOR_BGR16
				| OWL_DSS_COLOR_ARGB32 | OWL_DSS_COLOR_ABGR32
				| OWL_DSS_COLOR_RGBA32 | OWL_DSS_COLOR_BGRA32
				| OWL_DSS_COLOR_YU12 | OWL_DSS_COLOR_NV12 | OWL_DSS_COLOR_NV21
				| OWL_DSS_COLOR_ARGB16 | OWL_DSS_COLOR_ABGR16
				| OWL_DSS_COLOR_RGBA16 | OWL_DSS_COLOR_BGRA16,
	},
	{
		.id			= OWL_DSS_VIDEO3,
		.supported_colors	=
				OWL_DSS_COLOR_RGB16 | OWL_DSS_COLOR_BGR16
				| OWL_DSS_COLOR_ARGB32 | OWL_DSS_COLOR_ABGR32
				| OWL_DSS_COLOR_RGBA32 | OWL_DSS_COLOR_BGRA32
				| OWL_DSS_COLOR_YU12 | OWL_DSS_COLOR_NV12 | OWL_DSS_COLOR_NV21
				| OWL_DSS_COLOR_ARGB16 | OWL_DSS_COLOR_ABGR16
				| OWL_DSS_COLOR_RGBA16 | OWL_DSS_COLOR_BGRA16,
	},
	{
		.id			= OWL_DSS_VIDEO4,
		.supported_colors	=
				OWL_DSS_COLOR_RGB16 | OWL_DSS_COLOR_BGR16
				| OWL_DSS_COLOR_ARGB32 | OWL_DSS_COLOR_ABGR32
				| OWL_DSS_COLOR_RGBA32 | OWL_DSS_COLOR_BGRA32
				| OWL_DSS_COLOR_YU12 | OWL_DSS_COLOR_NV12 | OWL_DSS_COLOR_NV21
				| OWL_DSS_COLOR_ARGB16 | OWL_DSS_COLOR_ABGR16
				| OWL_DSS_COLOR_RGBA16 | OWL_DSS_COLOR_BGRA16,
	},
};

static struct de_regs_t		de_atm9009_regs[256];
static int			de_atm9009_regs_cnt = 0;

struct owl_de_pdata 		owl_de_atm9009;

static void inline de_write_reg(int idx, u32 val)
{
	writel(val, owl_de_atm9009.base + idx);
}

static u32 inline de_read_reg(int idx)
{
	return readl(owl_de_atm9009.base + idx);
}

static u32 de_irq_status_get(void)
{
	return de_read_reg(DE_IRQSTATUS);
}

static void de_irq_status_set(u32 status)
{
	de_write_reg(DE_IRQSTATUS, status);
}

static u32 de_irq_enable_get(void)
{
	return de_read_reg(DE_IRQENABLE);
}

static void de_irq_enable_set(u32 enable)
{
	de_write_reg(DE_IRQENABLE, enable);
}

static u32 de_irq_to_mask(enum de_irq_type irq)
{
	switch (irq) {
	case DE_IRQ_HDMI_PRE:
		return (1 << 0);
		break;
	case DE_IRQ_LCD_PRE:
		return (1 << 1);
		break;
	case DE_IRQ_DSI_PRE:
		return (1 << 2);
		break;
	case DE_IRQ_EDP_PRE:
		return (1 << 3);
		break;
	default:
		return 0;
		break;
	}
}

static u32 de_irq_mask_to_enable(u32 mask)
{
	/* good luck, enable bit is same as status bit */
	return mask;
}

static u32 de_irq_mask_to_vb_mask(u32 mask)
{
	return (mask << 8);
}

static void de_path_enable(enum owl_channel path, bool enable)
{
	u32 val;
	
	val = de_read_reg(DE_PATH_EN(path));
	val = REG_SET_VAL(val, enable, DE_PATH_ENABLE_BIT, DE_PATH_ENABLE_BIT);
	de_write_reg(DE_PATH_EN(path), val);
}

static bool de_path_is_enabled(enum owl_channel path)
{
	u32 val;

	val = de_read_reg(DE_PATH_EN(path));
	val = REG_GET_VAL(val, DE_PATH_ENABLE_BIT, DE_PATH_ENABLE_BIT);
	
	return val;
}

static void de_path_size_set(enum owl_channel path, u32 width, u32 height)
{
	u32 val;
	BUG_ON((width > DE_PATH_SIZE_WIDTH) || (height > DE_PATH_SIZE_HEIGHT));
	
	val = REG_VAL(height - 1, DE_PATH_SIZE_HEIGHT_END_BIT, DE_PATH_SIZE_HEIGHT_BEGIN_BIT)
	         | REG_VAL(width - 1, DE_PATH_SIZE_WIDTH_END_BIT, DE_PATH_SIZE_WIDTH_BEGIN_BIT);
	
	de_write_reg(DE_PATH_SIZE(path), val);
}
	
static void de_display_type_set(enum owl_channel path, enum owl_display_type type)
{
	u32 val;

	/* only valid for path2 */
	BUG_ON(OWL_DSS_CHANNEL_LCD == path);

	val = de_read_reg(DE_OUTPUT_CON);

	switch (type){
	case OWL_DISPLAY_TYPE_LCD:
		val = REG_SET_VAL(val, 0, DE_OUTPUT_PATH2_DEVICE_END_BIT,
                          DE_OUTPUT_PATH2_DEVICE_BEGIN_BIT);
		break;
	case OWL_DISPLAY_TYPE_DSI:
		val = REG_SET_VAL(val, 1, DE_OUTPUT_PATH2_DEVICE_END_BIT,
		DE_OUTPUT_PATH2_DEVICE_BEGIN_BIT);
		break;
	case OWL_DISPLAY_TYPE_EDP:
		val = REG_SET_VAL(val, 2, DE_OUTPUT_PATH2_DEVICE_END_BIT,
		DE_OUTPUT_PATH2_DEVICE_BEGIN_BIT);
		break;
	default:
    		BUG();
	}

	de_write_reg(DE_OUTPUT_CON, val);
}

/* TODO */
static void de_video_enable(enum owl_channel path, enum owl_plane video, bool enable)
{
	u32 val;
	if (enable) {
		if (path == OWL_DSS_CHANNEL_LCD) {
			val = de_read_reg(DE_PATH_CTL(OWL_DSS_CHANNEL_DIGIT));
			val = REG_SET_VAL(val, 0, DE_PATH_VIDEO_ENABLE_BEGIN_BIT + video,
						  DE_PATH_VIDEO_ENABLE_BEGIN_BIT + video);
                	de_write_reg(DE_PATH_CTL(OWL_DSS_CHANNEL_DIGIT), val);
        	} else {
			val = de_read_reg(DE_PATH_CTL(OWL_DSS_CHANNEL_LCD));
			val = REG_SET_VAL(val, 0, DE_PATH_VIDEO_ENABLE_BEGIN_BIT + video,
						  DE_PATH_VIDEO_ENABLE_BEGIN_BIT + video);
			de_write_reg(DE_PATH_CTL(OWL_DSS_CHANNEL_LCD), val);
		}
	}

	val = de_read_reg(DE_PATH_CTL(path));
	val = REG_SET_VAL(val, enable, DE_PATH_VIDEO_ENABLE_BEGIN_BIT + video,
			  	DE_PATH_VIDEO_ENABLE_BEGIN_BIT + video);   
	de_write_reg(DE_PATH_CTL(path), val);
}

static bool de_video_is_enabled(enum owl_channel path, enum owl_plane video)
{
	u32 val;

	val = de_read_reg(DE_PATH_CTL(path));
	val = REG_GET_VAL(val, DE_PATH_VIDEO_ENABLE_BEGIN_BIT + video,
			DE_PATH_VIDEO_ENABLE_BEGIN_BIT + video);
	
	return val;
}

static void de_path_fcr_set(enum owl_channel path)
{
	u32 val;
	
	val = de_read_reg(DE_PATH_FCR(path));
	val = REG_SET_VAL(val, 1, DE_PATH_FCR_BIT, DE_PATH_FCR_BIT);
	
	de_write_reg(DE_PATH_FCR(path), val);
}

static bool de_path_fcr_get(enum owl_channel path)
{
	u32 val;

	val = de_read_reg(DE_PATH_FCR(path));
	val = REG_GET_VAL(val, DE_PATH_FCR_BIT, DE_PATH_FCR_BIT);

	return val;
}

/* TODO */
static int de_owl_color_mode_to_hw_mode(enum owl_color_mode color_mode){
	int hw_format = 0;

	switch(color_mode){
	case OWL_DSS_COLOR_RGB16:
		hw_format = 0x0;
		break;
	case OWL_DSS_COLOR_BGR16:
		hw_format = 0x1;
		break;
	case OWL_DSS_COLOR_ARGB32:
		hw_format = 0x100;
		break;
	case OWL_DSS_COLOR_ABGR32:
		hw_format = 0x101;
		break;
	case OWL_DSS_COLOR_RGBA32:
		hw_format = 0x110;
		break;
	case OWL_DSS_COLOR_BGRA32:
		hw_format = 0x111;
		break;
	case OWL_DSS_COLOR_NV21:		
    		hw_format = 0x1000;
		break;
	case OWL_DSS_COLOR_NU21:
	case OWL_DSS_COLOR_NV12:
    		hw_format = 0x1001;
		break;
	case OWL_DSS_COLOR_YU12:
    		hw_format = 0x1010;
		break;

	case OWL_DSS_COLOR_ARGB16:
    		hw_format = 0x1100;
		break;
	case OWL_DSS_COLOR_ABGR16:
    		hw_format = 0x1101;
		break;
	case OWL_DSS_COLOR_RGBA16:
    		hw_format = 0x1110;
		break;
	case OWL_DSS_COLOR_BGRA16:
    		hw_format = 0x1111;
		break;
	default:
		/* TODO */
		break;
	}
	return hw_format;
}


static void de_video_format_set(enum owl_plane video, enum owl_color_mode color_mode)
{
	u32 val;
	int hw_format = 0;

	if (color_mode == OWL_DSS_COLOR_YU12
		|| color_mode == OWL_DSS_COLOR_NV12
		|| color_mode == OWL_DSS_COLOR_NV21
		|| color_mode == OWL_DSS_COLOR_NU21) {
		val = REG_SET_VAL(val, 3, 29, 28);
	}
	
	hw_format = de_owl_color_mode_to_hw_mode(color_mode);
	
	val = de_read_reg(DE_OVL_CFG(video));
	
	val = REG_SET_VAL(val, hw_format, DE_OVL_CFG_FMT_END_BIT, DE_OVL_CFG_FMT_BEGIN_BIT);
	
	de_write_reg(DE_OVL_CFG(video), val);
}

/* TODO */
static void de_video_fb_addr_set(enum owl_plane video, enum de_video_fb fb, void *paddr)
{
	if (fb == DE_VIDEO_FB0) {
    		de_write_reg(DE_OVL_BA0(video), (u32)paddr);
	} else if (fb == DE_VIDEO_FB1) {
    		de_write_reg(DE_OVL_BA1UV(video), (u32)paddr);
	} else if (fb == DE_VIDEO_FB2) {
    		de_write_reg(DE_OVL_BA2V(video), (u32)paddr);
	}
}

static void de_video_isize_set(enum owl_plane video, u32 width, u32 height)
{
	u32 val;

	BUG_ON((width > DE_PATH_SIZE_WIDTH) || (height > DE_PATH_SIZE_HEIGHT));
	
	val = REG_VAL(height - 1, DE_PATH_SIZE_HEIGHT_END_BIT, DE_PATH_SIZE_HEIGHT_BEGIN_BIT)
	         | REG_VAL(width - 1, DE_PATH_SIZE_WIDTH_END_BIT, DE_PATH_SIZE_WIDTH_BEGIN_BIT);

	de_write_reg(DE_OVL_ISIZE(video), val);
}

static void de_video_osize_set(enum owl_plane video, u32 width, u32 height)
{
	u32 val;

	BUG_ON((width > DE_PATH_SIZE_WIDTH) || (height > DE_PATH_SIZE_HEIGHT));
	
	val = REG_VAL(height - 1, DE_PATH_SIZE_HEIGHT_END_BIT, DE_PATH_SIZE_HEIGHT_BEGIN_BIT)
	         | REG_VAL(width - 1, DE_PATH_SIZE_WIDTH_END_BIT, DE_PATH_SIZE_WIDTH_BEGIN_BIT);

	de_write_reg(DE_OVL_OSIZE(video), val);
}

static void de_video_position_set(enum owl_channel path, enum owl_plane video,
				u32 x_pos, u32 y_pos)
{
	u32 val;

	BUG_ON((x_pos > DE_PATH_SIZE_WIDTH) || (y_pos > DE_PATH_SIZE_HEIGHT));

	val = REG_VAL(x_pos - 1, DE_PATH_SIZE_HEIGHT_END_BIT, DE_PATH_SIZE_HEIGHT_BEGIN_BIT)
	         | REG_VAL(y_pos - 1, DE_PATH_SIZE_WIDTH_END_BIT, DE_PATH_SIZE_WIDTH_BEGIN_BIT);

	de_write_reg(DE_OVL_COOR(path, video), val);
}

/*
 * scaling set, TODO
 */
static void de_ovl_set_scal_coef(enum owl_plane plane, u8 scalmode)
{
	switch(scalmode) {
	case DE_SCLCOEF_ZOOMIN:
		de_write_reg(DE_OVL_SCOEF0(plane), 0x00400000);
		de_write_reg(DE_OVL_SCOEF1(plane), 0xFC3E07FF);
		de_write_reg(DE_OVL_SCOEF2(plane), 0xFA3810FE);
		de_write_reg(DE_OVL_SCOEF3(plane), 0xF9301BFC);
		de_write_reg(DE_OVL_SCOEF4(plane), 0xFA2626FA);
		de_write_reg(DE_OVL_SCOEF5(plane), 0xFC1B30F9);
		de_write_reg(DE_OVL_SCOEF6(plane), 0xFE1038FA);
		de_write_reg(DE_OVL_SCOEF7(plane), 0xFF073EFC);
		break;
	case DE_SCLCOEF_HALF_ZOOMOUT:
		de_write_reg(DE_OVL_SCOEF0(plane), 0x00400000);
		de_write_reg(DE_OVL_SCOEF1(plane), 0x00380800);
		de_write_reg(DE_OVL_SCOEF2(plane), 0x00301000);
		de_write_reg(DE_OVL_SCOEF3(plane), 0x00281800);
		de_write_reg(DE_OVL_SCOEF4(plane), 0x00202000);
		de_write_reg(DE_OVL_SCOEF5(plane), 0x00182800);
		de_write_reg(DE_OVL_SCOEF6(plane), 0x00103000);
		de_write_reg(DE_OVL_SCOEF7(plane), 0x00083800);
		break;
	case DE_SCLCOEF_SMALLER_ZOOMOUT:
		de_write_reg(DE_OVL_SCOEF0(plane), 0x10201000);
		de_write_reg(DE_OVL_SCOEF1(plane), 0x0E1E1202);
		de_write_reg(DE_OVL_SCOEF2(plane), 0x0C1C1404);
		de_write_reg(DE_OVL_SCOEF3(plane), 0x0A1A1606);
		de_write_reg(DE_OVL_SCOEF4(plane), 0x08181808);
		de_write_reg(DE_OVL_SCOEF5(plane), 0x06161A0A);
		de_write_reg(DE_OVL_SCOEF6(plane), 0x04141C0C);
		de_write_reg(DE_OVL_SCOEF7(plane), 0x02121E0E);
		break;
	default:
		BUG();
	}   
}

static void de_video_scaling_set(enum owl_plane video, u32 orig_width, u32 orig_height,
				u32 out_width, u32 out_height, bool ilace)
{
	u16 w_factor;
	u16 h_factor;
	u16 factor;
	u16 scalmode;
	u32 val;    
	
	w_factor = (orig_width * DE_SCALE_CONST_VALUE  +  out_width - 1) / out_width;
	h_factor = (orig_height * DE_SCALE_CONST_VALUE  +  out_height - 1) / out_height;  
	   
	val = REG_VAL(h_factor, 31, 16) | REG_VAL(w_factor, 15, 0);
	        
	de_write_reg(DE_OVL_SR(video), val);
	
	factor = (orig_width * orig_height) / (out_width * out_height);
	
	if (factor <= 1) {
		scalmode = DE_SCLCOEF_ZOOMIN;        
	} else if (factor <= 4) {
		scalmode = DE_SCLCOEF_HALF_ZOOMOUT;        
	} else if(factor > 4) {
		scalmode = DE_SCLCOEF_SMALLER_ZOOMOUT;
	}
	       
	de_ovl_set_scal_coef(video, scalmode);
}

/* TODO */
static void de_video_rotation_set(enum owl_plane video, u8 rotation)
{
	u32 val;

	BUG_ON(rotation != 0 && rotation != 1 && rotation != 2 && rotation != 3);
	val = de_read_reg(DE_OVL_CFG(video));
	val = REG_SET_VAL(val, rotation, DE_OVL_CFG_FLIP_BIT + 1, DE_OVL_CFG_FLIP_BIT);

	de_write_reg(DE_OVL_CFG(video), val);
}

static void de_video_alpha_set(enum owl_channel path, enum owl_plane video,
				u8 alpha_value,bool alpha_en, bool pre_mult_en)
{
	u32 val;         

	val = de_read_reg(DE_OVL_ALPHA_CFG(path, video));    

	/* set global alpha */
	val = REG_SET_VAL(val, alpha_value, DE_OVL_ALPHA_CFG_VALUE_END_BIT,
				DE_OVL_ALPHA_CFG_VALUE_BEGIN_BIT);


	if (!alpha_en) {
		/* alpha disable */
		val = REG_SET_VAL(val, 0, DE_OVL_ALPHA_CFG_ENABLE_END_BIT,
        	      			DE_OVL_ALPHA_CFG_ENABLE_BEGIN_BIT);
	} else if (pre_mult_en) {
		/* permultied mode */
		val = REG_SET_VAL(val, 2, DE_OVL_ALPHA_CFG_ENABLE_END_BIT,
        	      			DE_OVL_ALPHA_CFG_ENABLE_BEGIN_BIT);
	} else {
		/* normal coverage mode */
		val = REG_SET_VAL(val, 1, DE_OVL_ALPHA_CFG_ENABLE_END_BIT,
        	      			DE_OVL_ALPHA_CFG_ENABLE_BEGIN_BIT);
	}

	de_write_reg(DE_OVL_ALPHA_CFG(path, video), val);
}

static void de_video_str_set(enum owl_plane video,
			  enum owl_color_mode color_mode, u32 width)
{
	u32 val;

	switch (color_mode) {
	case OWL_DSS_COLOR_RGB16:
	case OWL_DSS_COLOR_BGR16:
	case OWL_DSS_COLOR_ARGB16:
	case OWL_DSS_COLOR_ABGR16:
	case OWL_DSS_COLOR_RGBA16:
	case OWL_DSS_COLOR_BGRA16:
		val = width / 4;
		break;

	case OWL_DSS_COLOR_ARGB32:    
	case OWL_DSS_COLOR_ABGR32:
	case OWL_DSS_COLOR_RGBA32:
	case OWL_DSS_COLOR_BGRA32:
		val = width / 2;
		break;

	case OWL_DSS_COLOR_YU12:
	case OWL_DSS_COLOR_NV21:
		val = ((width / 4) >> 1) | (((width / 4) >> 2) << 11)
			| (((width / 4) >> 2) << 21);
		break;

	case OWL_DSS_COLOR_NV12:
	case OWL_DSS_COLOR_NU21: 
		val = ((width / 4) >> 1) | (((width / 4) >> 1) << 11);
		break;
	default:
		BUG();
		break;            
	}
	de_write_reg(DE_OVL_STR(video), val); 
}

static inline void __de_backup_reg(struct de_regs_t *p, int reg)
{
	p->reg      = reg;
	p->value    = de_read_reg(reg);
}

static void de_backup_regs(void)
{
	int i = 0, cnt = 0;
	struct de_regs_t *reg = de_atm9009_regs;
	
	DSSINFO("%s\n", __func__);
	
	__de_backup_reg(&reg[cnt++], DE_IRQENABLE);
	__de_backup_reg(&reg[cnt++], DE_MAX_OUTSTANDING);
	__de_backup_reg(&reg[cnt++], DE_OUTPUT_CON);
	__de_backup_reg(&reg[cnt++], DE_WB_CON);
	__de_backup_reg(&reg[cnt++], DE_WB_ADDR);
	
	for(i = 0 ; i < 2 ; i++) {
		__de_backup_reg(&reg[cnt++], DE_PATH_CTL(i));
		__de_backup_reg(&reg[cnt++], DE_PATH_BK(i));
		__de_backup_reg(&reg[cnt++], DE_PATH_SIZE(i));
		__de_backup_reg(&reg[cnt++], DE_PATH_GAMMA_IDX(i));
		__de_backup_reg(&reg[cnt++], DE_PATH_GAMMA_RAM(i));
	}

	for (i = 0 ; i < 4 ; i++) {
		__de_backup_reg(&reg[cnt++], DE_OVL_CFG(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_ISIZE(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_OSIZE(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_SR(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_SCOEF0(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_SCOEF1(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_SCOEF2(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_SCOEF3(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_SCOEF4(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_SCOEF5(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_SCOEF6(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_SCOEF7(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_BA0(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_BA1UV(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_BA2V(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_3D_RIGHT_BA0(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_3D_RIGHT_BA1UV(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_3D_RIGHT_BA2V(i));

		__de_backup_reg(&reg[cnt++], DE_OVL_STR(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_CRITICAL_CFG(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_REMAPPING(i));
		__de_backup_reg(&reg[cnt++], DE_OVL_COOR(0,i));
		__de_backup_reg(&reg[cnt++], DE_OVL_COOR(1,i));
		__de_backup_reg(&reg[cnt++], DE_OVL_ALPHA_CFG(0,i));
		__de_backup_reg(&reg[cnt++], DE_OVL_ALPHA_CFG(1,i));
	}

	de_atm9009_regs_cnt = cnt;
}

static void de_restore_regs(void)
{
	int i;
	void * noc_reg = 0;		
	DSSINFO("%s\n", __func__);
	
	for (i = 1; i < de_atm9009_regs_cnt; i++) {
	    de_write_reg(de_atm9009_regs[i].reg, de_atm9009_regs[i].value);
	}

	noc_reg = ioremap(0xb0500108, 4);
	DSSINFO("%s  de_write_reg  addr end %p \n", __func__, noc_reg);
	writel(0x0f, noc_reg);
	iounmap(noc_reg); 	  
	
	DSSINFO("%s end\n", __func__);
}

static void de_special_init(void)
{
	u32 val = 0;
	
	/* max outstanding num for read, TODO */
#if 0
	val = REG_VAL(0x20, 21, 16);
	de_write_reg(DE_MAX_OUTSTANDING, val);
#endif
}

static int de_suspend(struct platform_device *pdev, pm_message_t state)
{
	de_backup_regs();

	return 0;
}

static int de_resume(struct platform_device *pdev)
{
	de_restore_regs();

	return 0;
}



static void de_dump_regs(void)
{
	int i = 0;

#define DUMPREG(r) printk("%08x ~~~~ %08x ~~~ %s ~~ \n", r, de_read_reg(r), #r)
	DUMPREG(DE_IRQSTATUS);
	DUMPREG(DE_IRQENABLE);
	DUMPREG(DE_MAX_OUTSTANDING);
	DUMPREG(DE_MMU_EN);
	DUMPREG(DE_MMU_BASE);
	DUMPREG(DE_OUTPUT_CON);
	DUMPREG(DE_OUTPUT_STAT);
	DUMPREG(DE_PATH_DITHER);

	for (i = 0 ; i < 2 ; i++) {
		printk("\npath %d ------------------>\n", i);
		DUMPREG(DE_PATH_CTL(i));
		DUMPREG(DE_PATH_FCR(i));
		DUMPREG(DE_PATH_EN(i));
		DUMPREG(DE_PATH_BK(i));
		DUMPREG(DE_PATH_SIZE(i));
		DUMPREG(DE_PATH_GAMMA_IDX(i));
		DUMPREG(DE_PATH_GAMMA_RAM(i));
	}
	for (i = 0 ; i < 4 ; i++){
		printk("\nlayer %d ------------------>\n", i);
		DUMPREG(DE_OVL_CFG(i));
		DUMPREG(DE_OVL_ISIZE(i));
		DUMPREG(DE_OVL_OSIZE(i));
		DUMPREG(DE_OVL_SR(i));
		DUMPREG(DE_OVL_SCOEF0(i));
		DUMPREG(DE_OVL_SCOEF1(i));
		DUMPREG(DE_OVL_SCOEF2(i));
		DUMPREG(DE_OVL_SCOEF3(i));
		DUMPREG(DE_OVL_SCOEF4(i));
		DUMPREG(DE_OVL_SCOEF5(i));
		DUMPREG(DE_OVL_SCOEF6(i));
		DUMPREG(DE_OVL_SCOEF7(i));
		DUMPREG(DE_OVL_BA0(i));
		DUMPREG(DE_OVL_BA1UV(i));
		DUMPREG(DE_OVL_BA2V(i));
		DUMPREG(DE_OVL_3D_RIGHT_BA0(i));
		DUMPREG(DE_OVL_3D_RIGHT_BA1UV(i));
		DUMPREG(DE_OVL_3D_RIGHT_BA2V(i));

		DUMPREG(DE_OVL_STR(i));
		DUMPREG(DE_OVL_CRITICAL_CFG(i));
		DUMPREG(DE_OVL_REMAPPING(i));
		DUMPREG(DE_OVL_CSC(i));
		DUMPREG(DE_OVL_COOR(0,i));
		DUMPREG(DE_OVL_COOR(1,i));
		DUMPREG(DE_OVL_ALPHA_CFG(0,i));
		DUMPREG(DE_OVL_ALPHA_CFG(1,i));
	}
#undef DUMPREG
}

static ssize_t de_write_regs(const char *buf, size_t len)
{
	unsigned int reg, reg_val;
	char *end_ptr;
	
	reg = simple_strtoul(buf, &end_ptr, 16);
	
	if (*end_ptr++ == '=') {
		reg_val = simple_strtoul(end_ptr, NULL,16);        
		de_write_reg(reg, reg_val);
	}
	    
	printk("reg[0x%x] = 0x%x \n", reg, de_read_reg(reg));

	return len;
}

static const struct owl_de_hwops de_atm9009_ops = {
	/* global operations */
	.irq_status_get		= de_irq_status_get,
	.irq_status_set		= de_irq_status_set,

	.irq_enable_get		= de_irq_enable_get,
	.irq_enable_set		= de_irq_enable_set,
	.irq_to_mask		= de_irq_to_mask,
	.irq_mask_to_enable	= de_irq_mask_to_enable,
	.irq_mask_to_vb_mask	= de_irq_mask_to_vb_mask,

	.special_init		= de_special_init,

	.suspend		= de_suspend,
	.resume			= de_resume,

	/* path operations */
	.path_enable		= de_path_enable,
	.path_is_enabled	= de_path_is_enabled,

	.path_size_set		= de_path_size_set,

	.display_type_set	= de_display_type_set,

	.video_enable		= de_video_enable,
	.video_is_enabled	= de_video_is_enabled,
	.fcr_set		= de_path_fcr_set,
	.fcr_get		= de_path_fcr_get,

	/* video layer operations */
	.format_set		= de_video_format_set,

	.fb_addr_set		= de_video_fb_addr_set,

	.isize_set		= de_video_isize_set,
	.osize_set		= de_video_osize_set,
	.position_set		= de_video_position_set,

	.scaling_set		= de_video_scaling_set,
	.rotation_set		= de_video_rotation_set,
	.alpha_set		= de_video_alpha_set,
	.str_set		= de_video_str_set,

	/* debug oprations */
	.dump_regs		= de_dump_regs,
	.write_regs		= de_write_regs,
};

struct owl_de_pdata owl_de_atm9009 = {
	.num_paths		= 2,
	.num_videos		= 4,

	.path_pdata		= &de_atm9009_paths,
	.video_pdata		= &de_atm9009_videos,

	.hwops			= &de_atm9009_ops,
};
EXPORT_SYMBOL(owl_de_atm9009);
