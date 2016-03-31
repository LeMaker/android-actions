/*
 * hdmi_ip.c
 *
 * HDMI OWL IP driver Library
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/clk.h>
#include <mach/clkname.h>
#include <mach/module-owl.h>

#include "hdmi_ip.h"
#include "hdmi.h"

static inline void hdmi_write_reg(struct hdmi_ip_data *ip_data,	const u16 idx, u32 val)
{
	//HDMI_DEBUG("write base=%x  offset=%x  val=%x\n", ip_data->base, idx, val);
	writel(val, ip_data->base + idx);
}

static inline u32 hdmi_read_reg(struct hdmi_ip_data *ip_data,const u16 idx)
{
	//HDMI_DEBUG("read base=%x  offset=%x \n", ip_data->base, idx);
	return readl(ip_data->base + idx);
}

/* HDMI_CORE_VIDEO_CONFIG */
static void hdmi_core_config_deepcolor_mode(struct hdmi_ip_data *ip_data)
{
	u32 val = 0;	
	val = hdmi_read_reg(ip_data, HDMI_SCHCR);	
	val = REG_SET_VAL(val,ip_data->settings.deep_color, 17, 16);
	hdmi_write_reg(ip_data, HDMI_SCHCR, val);
	return;
}

static void hdmi_core_config_pixel_fomat(struct hdmi_ip_data *ip_data)
{
	u32 val = 0;	
	val = hdmi_read_reg(ip_data, HDMI_SCHCR);	
	val = REG_SET_VAL(val,ip_data->settings.pixel_encoding, 5, 4);
	hdmi_write_reg(ip_data, HDMI_SCHCR, val);
	return;
}

static void hdmi_core_config_3d_mode(struct hdmi_ip_data *ip_data)
{
	u32 val = 0;	
	val = hdmi_read_reg(ip_data, HDMI_SCHCR);	
	val = REG_SET_VAL(val,ip_data->settings.enable_3d, 8, 8);
	hdmi_write_reg(ip_data, HDMI_SCHCR, val);
	return;

}

static void hdmi_core_config_mode(struct hdmi_ip_data *ip_data)
{
	u32 val = 0;	
	val = hdmi_read_reg(ip_data, HDMI_SCHCR);	
	val = REG_SET_VAL(val,ip_data->settings.hdmi_mode, hdmi.hdmihw_diff->mode_end, hdmi.hdmihw_diff->mode_start);
	hdmi_write_reg(ip_data, HDMI_SCHCR, val);
	return;		
}

static void hdmi_core_config_invert(struct hdmi_ip_data *ip_data)
{
	u32 val = 0;	
	val = hdmi_read_reg(ip_data, HDMI_SCHCR);	
	val = REG_SET_VAL(val,ip_data->settings.bit_invert, 28, 28);
	val = REG_SET_VAL(val,ip_data->settings.channel_invert, 29, 29);
	hdmi_write_reg(ip_data, HDMI_SCHCR, val);
	return;		
}

static void hdmi_core_config_colordepth_value(struct hdmi_ip_data *ip_data)
{
	u32 val = 0;
	
	u32 mode = ip_data->settings.deep_color;	
	
	val = hdmi_read_reg(ip_data, HDMI_GCPCR);	
	
	val = REG_SET_VAL(val,mode, 7, 4);
	
	val = REG_SET_VAL(val,1, 31, 31);
	
	if(mode > HDMI_PACKETMODE24BITPERPIXEL){

		val = REG_SET_VAL(val,1, 30, 30);

	}else{

		val = REG_SET_VAL(val,0, 30, 30);

	}
	// clear specify avmute flag in gcp packet 
	val = REG_SET_VAL(val,1, 1, 1);	
	
	hdmi_write_reg(ip_data, HDMI_GCPCR, val);
	
	return;

}

static void hdmi_core_config_input_src(struct hdmi_ip_data *ip_data)
{
	u32 val;
		
	val = hdmi_read_reg(ip_data, HDMI_ICR);
	
	if(ip_data->settings.hdmi_src==VITD){
		
		val = REG_SET_VAL(val,0x01, 24, 24);
		
		val = REG_SET_VAL(val,hdmi.ip_data.settings.vitd_color, 23, 0);
		
	}else{
		
		val = REG_SET_VAL(val,0x00, 24, 24);
		
	}
		
	hdmi_write_reg(ip_data, HDMI_ICR, val);
	
	return;
}

static void hdmi_video_init_format(struct hdmi_video_format *video_fmt,
	struct owl_video_timings *timings, struct hdmi_config *param)
{
	HDMI_DEBUG("Enter hdmi_video_init_format\n");
	video_fmt->packing_mode = HDMI_PACK_24b_RGB_YUV444_YUV422;
	video_fmt->y_res = param->timings.y_res;
	video_fmt->x_res = param->timings.x_res;

    timings->x_res = param->timings.x_res;
    timings->y_res = param->timings.y_res;
	timings->hbp = param->timings.hbp;
	timings->hfp = param->timings.hfp;
	timings->hsw = param->timings.hsw;
	timings->vbp = param->timings.vbp;
	timings->vfp = param->timings.vfp;
	timings->vsw = param->timings.vsw;
	timings->vstart = param->timings.vstart;
	timings->repeat = param->timings.repeat;
	
	timings->vsync_level = param->timings.vsync_level;
	timings->hsync_level = param->timings.hsync_level;
	timings->interlace = param->timings.interlace;
	timings->repeat = param->timings.repeat;
}

static void hdmi_video_config_format(struct hdmi_ip_data *ip_data,
		struct owl_video_timings *timings)
{
	u32 val = 0;
	u32 val_hp = timings->x_res + timings->hbp + timings->hfp + timings->hsw;
	u32 val_vp = timings->y_res + timings->vbp + timings->vfp + timings->vsw;
	HDMI_DEBUG("x %d %d %d %d\n", timings->x_res, timings->hbp, timings->hfp, timings->hsw);
	HDMI_DEBUG("x %d %d %d %d\n", timings->y_res, timings->vbp, timings->vfp, timings->vsw);

	val = hdmi_read_reg(ip_data, HDMI_VICTL);
	val = REG_SET_VAL(val ,val_hp-1, hdmi.hdmihw_diff->hp_end, hdmi.hdmihw_diff->hp_start);
	if(timings->interlace == 0){
		val = REG_SET_VAL(val ,val_vp- 1, hdmi.hdmihw_diff->vp_end, hdmi.hdmihw_diff->vp_start);
	}else{
		val = REG_SET_VAL(val ,val_vp * 2, hdmi.hdmihw_diff->vp_end, hdmi.hdmihw_diff->vp_start);
	}

	HDMI_DEBUG("hdmi_video_config_format val = %x hp = %x vp=%x\n", val, val_hp, val_vp);	
	hdmi_write_reg(ip_data, HDMI_VICTL, val);
}

static void hdmi_video_config_interface(struct hdmi_ip_data *ip_data, struct owl_video_timings *timings)
{

	u32 val;
	HDMI_DEBUG("hdmi_video_config_interface timings->interlace %x\n", timings->interlace);
	if(timings->interlace == 0){	
		val = 0;
		hdmi_write_reg(ip_data, HDMI_VIVSYNC, val);
		
	    val = hdmi_read_reg(ip_data, HDMI_VIVHSYNC);
		if(timings->vstart!=1){
			val = REG_SET_VAL(val ,timings->hsw - 1, 8, 0);
			val = REG_SET_VAL(val ,timings->vstart - 2, 23, 12);
			val = REG_SET_VAL(val ,timings->vstart + timings->vsw - 2, 27, 24);		
		}else{
			val = REG_SET_VAL(val ,timings->hsw - 1, 8, 0);
			val = REG_SET_VAL(val ,timings->y_res + timings->vbp + timings->vfp + timings->vsw - 1, 23, 12);
			val = REG_SET_VAL(val ,timings->vsw - 1, 27, 24);		
		}
		hdmi_write_reg(ip_data, HDMI_VIVHSYNC, val);
		
		HDMI_DEBUG("hdmi_video_config_interface HDMI_VIVHSYNC %x\n", val);
		
		//VIALSEOF = (y_res + vbp + vsp - 1)  |  ((vbp + vfp - 1) << 12)
		val = hdmi_read_reg(ip_data, HDMI_VIALSEOF);
		val = REG_SET_VAL(val ,timings->vstart -1 + timings->vsw + timings->vbp + timings->y_res - 1, 23, 12);
		val = REG_SET_VAL(val ,timings->vstart -1 + timings->vsw + timings->vbp - 1, 10, 0);
		hdmi_write_reg(ip_data, HDMI_VIALSEOF, val);
		
		HDMI_DEBUG("hdmi_video_config_interface HDMI_VIALSEOF %x\n", val);
		
	    val = 0;
		hdmi_write_reg(ip_data, HDMI_VIALSEEF, val);
	
		//VIADLSE =  (x_res + hbp + hsp - 1)  |  ((hbp + hsw - 1) << 12)
		val = hdmi_read_reg(ip_data, HDMI_VIADLSE);
		val = REG_SET_VAL(val ,timings->hbp +  timings->hsw - 1, 11, 0);
		val = REG_SET_VAL(val ,timings->x_res + timings->hbp + timings->hsw - 1, 28, 16);
		hdmi_write_reg(ip_data, HDMI_VIADLSE, val);
		HDMI_DEBUG("hdmi_video_config_interface HDMI_VIADLSE %x\n", val);
	}else{
		val = 0;
		hdmi_write_reg(ip_data, HDMI_VIVSYNC, val);
		
		//VIVHSYNC  = (hsw -1 ) | ((y_res + vsw + vfp + vbp - 1 ) << 12) | (vfp -1 << 24)
	    val = hdmi_read_reg(ip_data, HDMI_VIVHSYNC);
		val = REG_SET_VAL(val ,timings->hsw - 1, 8, 0);
		val = REG_SET_VAL(val ,(timings->y_res + timings->vbp + timings->vfp + timings->vsw) * 2, 22, 12);
		val = REG_SET_VAL(val ,timings->vfp * 2, 22, 12);
		hdmi_write_reg(ip_data, HDMI_VIVHSYNC, val);
		HDMI_DEBUG("hdmi_video_config_interface HDMI_VIVHSYNC %x\n", val);
		
		//VIALSEOF = (y_res + vbp + vfp - 1)  |  ((vbp + vfp - 1) << 12)
		val = hdmi_read_reg(ip_data, HDMI_VIALSEOF);
		val = REG_SET_VAL(val ,timings->vbp + timings->vfp  - 1, 22, 12);
		val = REG_SET_VAL(val ,(timings->y_res + timings->vbp + timings->vfp)*2, 10, 0);
		hdmi_write_reg(ip_data, HDMI_VIALSEOF, val);
		HDMI_DEBUG("hdmi_video_config_interface HDMI_VIALSEOF %x\n", val);
		
	    val = 0;
		hdmi_write_reg(ip_data, HDMI_VIALSEEF, val);
		
		//VIADLSE =  (x_res + hbp + hsp - 1)  |  ((hbp + hsw - 1) << 12)
		val = hdmi_read_reg(ip_data, HDMI_VIADLSE);
		val = REG_SET_VAL(val ,timings->hbp +  timings->hsw - 1, 27, 16);
		val = REG_SET_VAL(val ,timings->x_res + timings->hbp + timings->hsw - 1, 11, 0);
		hdmi_write_reg(ip_data, HDMI_VIADLSE, val);
		HDMI_DEBUG("hdmi_video_config_interface HDMI_VIADLSE %x\n", val);
	}

}

static void hdmi_video_interval_packet(struct hdmi_ip_data *ip_data, struct owl_video_timings *timings)
{
	u32 val;
	HDMI_DEBUG("hdmi_video_interval_packet\n");
	switch (ip_data->cfg.cm.code)
	{
		case VID640x480P_60_4VS3:
		case VID720x480P_60_4VS3:
		case VID720x576P_50_4VS3:
			val = 0x701;
			break;		
		case VID1280x720P_60_16VS9:
		case VID1280x720P_50_16VS9:
		case VID1920x1080P_50_16VS9:
		case VID1280x720P_60_DVI:
			val = 0x1107;
			break;
		case VID1920x1080P_60_16VS9:
			val = 0x1105;
			break;
		default:
			val = 0x1107;
			break;
	}
	hdmi_write_reg(ip_data, HDMI_DIPCCR, val);	
}

static void hdmi_video_config_timing(struct hdmi_ip_data *ip_data, struct owl_video_timings *timings)
{

	bool vsync_pol, hsync_pol ,interlace, repeat;
	u32 val;
	vsync_pol = timings->vsync_level == OWLDSS_SIG_ACTIVE_LOW ;
	hsync_pol = timings->hsync_level == OWLDSS_SIG_ACTIVE_LOW ;
	
	interlace = timings->interlace;
	repeat    = timings->repeat; 
	
	val = hdmi_read_reg(ip_data, HDMI_SCHCR);
	val = REG_SET_VAL(val , hsync_pol, 1, 1);
	val = REG_SET_VAL(val , vsync_pol, 2, 2);
	hdmi_write_reg(ip_data, HDMI_SCHCR, val);

	val = hdmi_read_reg(ip_data, HDMI_VICTL);
	val = REG_SET_VAL(val ,interlace, 28, 28);
	val = REG_SET_VAL(val ,repeat, 29, 29);
	hdmi_write_reg(ip_data, HDMI_VICTL, val);	
}

static void ip_hdmi_devclken(struct hdmi_ip_data *ip_data, bool enable)
{
	if(enable){
		module_clk_enable(MOD_ID_HDMI);	
	}else{
		module_clk_disable(MOD_ID_HDMI);
	}
}

static void ip_hdmi_clk24Men(struct hdmi_ip_data *ip_data, bool enable)
{
	if(enable){
		module_clk_enable(MOD_ID_TV24M);	
	}else{
		module_clk_disable(MOD_ID_TV24M);	
	}	
}

static void ip_hdmi_reset(struct hdmi_ip_data *ip_data)
{
	HDMI_DEBUG("[%s start]\n", __func__);

	HDMI_DEBUG("~~~~~module_reset \n");
	module_reset(MOD_ID_HDMI);
/*	
	val = hdmi_read_reg(ip_data, HDMI_TX_1);
	val = REG_SET_VAL(val ,1, 23, 23);
	hdmi_write_reg(ip_data, HDMI_TX_1, val);
	HDMI_DEBUG("[%s finished]\n", __func__);
*/
}

static void ip_hdmi_pmds_ldo_enable(struct hdmi_ip_data *ip_data ,bool enable)
{
	u32 val;
	if(!enable){
		
		// tx pll power off
	    val = hdmi_read_reg(ip_data, HDMI_TX_1);
		val = REG_SET_VAL(val ,0, 23, 23);
		hdmi_write_reg(ip_data, HDMI_TX_1, val);
		
		// tmds ldo power off
	    val = hdmi_read_reg(ip_data, HDMI_TX_2);
		val = REG_SET_VAL(val ,0, 27, 27);
		hdmi_write_reg(ip_data, HDMI_TX_2, val);
		
	}else{
		// only open tmds ldo power on 
		val = 0x18f80f89;
		
		val = REG_SET_VAL(val ,0, 17, 17);	
			
		val = REG_SET_VAL(val ,0, 11, 8);
				
		hdmi_write_reg(ip_data, HDMI_TX_2,val);
		
		udelay(500);
		
		// tx pll power on
		switch (ip_data->cfg.cm.code)
		{
			case VID640x480P_60_4VS3:			
				hdmi_write_reg(ip_data, HDMI_TX_1, 0x819c2983);	
				break;	
			case VID720x576P_50_4VS3:
			case VID720x480P_60_4VS3:					
				hdmi_write_reg(ip_data, HDMI_TX_1, 0x819c2983);	
				break;		
			case VID1280x720P_60_16VS9:
			case VID1280x720P_50_16VS9:
			case VID1280x720P_60_DVI:						
				hdmi_write_reg(ip_data, HDMI_TX_1, 0x81942983);	
				break;
			case VID1920x1080P_60_16VS9:
			case VID1920x1080P_50_16VS9:						
				hdmi_write_reg(ip_data, HDMI_TX_1, 0x81902983);
				break;
			default:
				DEBUG_ERR("!!!no surpport this vid %d\n", ip_data->cfg.cm.code);
		}		
	}
}

static int ip_hdmi_pll_enable(struct hdmi_ip_data *ip_data)
{
	u32 pix_rate = 0;
	int ret;
	struct clk *tvout_clk = NULL;
	HDMI_DEBUG("PLL locked!\n");
	switch (ip_data->cfg.cm.code)
	{
		case VID640x480P_60_4VS3:
			pix_rate = 25200000;
			break;	
		case VID720x576P_50_4VS3:
		case VID720x480P_60_4VS3:
			pix_rate = 27000000;
			break;		
		case VID1280x720P_60_16VS9:
		case VID1280x720P_50_16VS9:
		case VID1280x720P_60_DVI:
			pix_rate = 74250000;
			break;
		case VID1920x1080P_60_16VS9:
		case VID1920x1080P_50_16VS9:
			pix_rate = 148500000;
			break;
		default:
			return -EINVAL;
	}
	
	tvout_clk = clk_get(NULL, CLKNAME_TVOUTPLL);
	ret = clk_set_rate(tvout_clk, pix_rate);
	if (ret < 0) {
		DEBUG_ERR("pixel rate set error!\n");
		return ret;
	}		
	msleep(5);
	clk_prepare(tvout_clk);
	clk_enable(tvout_clk);
	msleep(10);
	return 0;
}

/* hdmi pll dsiable */

static void ip_hdmi_pll_disable(struct hdmi_ip_data *ip_data)
{
	struct clk *tvout_clk = NULL;
	HDMI_DEBUG("PLL Disable!\n");
	tvout_clk = clk_get(NULL, CLKNAME_TVOUTPLL);
    clk_disable(tvout_clk);	
	clk_unprepare(tvout_clk);

}

static void ip_hdmi_write_reg(struct hdmi_ip_data *ip_data,	const u16 idx, u32 val)
{
	hdmi_write_reg(ip_data, idx, val);
}

static int ip_hdmi_read_reg(struct hdmi_ip_data *ip_data, const u16 idx)
{
	return hdmi_read_reg(ip_data, idx);
}

static bool ip_hdmi_cable_check_state(struct hdmi_ip_data *ip_data)
{
	bool hpd;
	
	/*HDMI_DEBUG("hdmi_read_reg(ip_data, HDMI_CR)=%x\n",hdmi_read_reg(ip_data, HDMI_CR));*/
	
	/*hpd = (hdmi_read_reg(ip_data, HDMI_CR)&(1<<29)?1:0);*/
	
	if((hdmi_read_reg(ip_data, HDMI_CR)&(1<<29))&&(hdmi_read_reg(ip_data, CEC_DDC_HPD)&(3<<12))){
		if((hdmi_read_reg(ip_data, CEC_DDC_HPD)&(3<<8))||
			(hdmi_read_reg(ip_data, CEC_DDC_HPD)&(3<<10))||
			(hdmi_read_reg(ip_data, CEC_DDC_HPD)&(3<<12))||
			(hdmi_read_reg(ip_data, CEC_DDC_HPD)&(3<<14))
			){
			hpd = 1;
		}else{
			hpd = 0;
		}	
	}else{
		hpd = 0;
	}

	return hpd;
}

static bool ip_hdmi_detect(struct hdmi_ip_data *ip_data)
{
	u32 val;
		
	val = hdmi_read_reg(ip_data, HDMI_CR);

	return (REG_GET_VAL(val,30,30) == 1);
}

static u32 ip_hdmi_get_irq_state(struct hdmi_ip_data *ip_data)
{
	u32 val;
	u32 irq_state = 0;

	val = hdmi_read_reg(ip_data, HDMI_CR);

	if(REG_GET_VAL(val,30,30) == 1)
	{
		irq_state |= HDMI_HPD_IRQ;
	}
	
	val = hdmi_read_reg(ip_data, HDMI_CECTXCR);

	if(REG_GET_VAL(val,6,6) == 1)
	{
		irq_state |= HDMI_CEC_IRQ;
	}

	val = hdmi_read_reg(ip_data, HDMI_CECRXCR);

	if(REG_GET_VAL(val,6,6) == 1)
	{
		irq_state |= HDMI_CEC_IRQ;
	}
	
	return irq_state;
}

static void ip_hdmi_hpd_enable(struct hdmi_ip_data *ip_data,bool enable)
{

	u32 val;
	
	val = hdmi_read_reg(ip_data, HDMI_CR);
	
	if(enable){	
		//set hotplug debounce ,default used 0x0f	
		val = REG_SET_VAL(val, 0x0f, 27, 24);
		
		//enable hotplug interrupt 	
		val = REG_SET_VAL(val ,0x01, 31, 31);
		
		//enable hotplug function 	
		val = REG_SET_VAL(val ,0x01, 28, 28);	
		
		//no clear hotplug panding bit 	
		val = REG_SET_VAL(val ,0x00, 30, 30);
	}else{
		//enable hotplug interrupt 	
		val = REG_SET_VAL(val ,0x00, 31, 31);
		
		//enable hotplug function 	
		val = REG_SET_VAL(val ,0x00, 28, 28);
		
		//clear hotplug panding bit 	
		val = REG_SET_VAL(val ,0x01, 30, 30);
	}
	
	hdmi_write_reg(ip_data, HDMI_CR, val);

}

static void ip_hdmi_clear_plugstatus(struct hdmi_ip_data *ip_data)
{

	u32 val;
	
	val = hdmi_read_reg(ip_data, HDMI_CR);

	//clear hotplug panding bit 	
	val = REG_SET_VAL(val ,0x01, 30, 30);

	hdmi_write_reg(ip_data, HDMI_CR, val);
	
}

static void ip_hdmi_phy_enable(struct hdmi_ip_data *ip_data)
{

/*	u32 val;
	// tdms enable
	val = hdmi_read_reg(ip_data, TMDS_EODR0);
	val = REG_SET_VAL(val ,1, 31, 31);
	hdmi_write_reg(ip_data, TMDS_EODR0, val);	
*/	
	if(hdmi.ip_data.txrx_cfg.drv_from_dts)
	{
		switch (ip_data->cfg.cm.code)
		{
			case VID640x480P_60_4VS3:
				hdmi_write_reg(ip_data, HDMI_TX_1, hdmi.ip_data.txrx_cfg.vid480p_tx1);	
				hdmi_write_reg(ip_data, HDMI_TX_2, hdmi.ip_data.txrx_cfg.vid480p_tx2);	
				break;	
			case VID720x576P_50_4VS3:
			case VID720x480P_60_4VS3:
				hdmi_write_reg(ip_data, HDMI_TX_1, hdmi.ip_data.txrx_cfg.vid576p_tx1);	
				hdmi_write_reg(ip_data, HDMI_TX_2, hdmi.ip_data.txrx_cfg.vid576p_tx2);	
				break;		
			case VID1280x720P_60_16VS9:
			case VID1280x720P_50_16VS9:
			case VID1280x720P_60_DVI:
				hdmi_write_reg(ip_data, HDMI_TX_1, hdmi.ip_data.txrx_cfg.vid720p_tx1);	
				hdmi_write_reg(ip_data, HDMI_TX_2, hdmi.ip_data.txrx_cfg.vid720p_tx2);	
				break;
			case VID1920x1080P_60_16VS9:
			case VID1920x1080P_50_16VS9:
				hdmi_write_reg(ip_data, HDMI_TX_1, hdmi.ip_data.txrx_cfg.vid1080p_tx1);	
				hdmi_write_reg(ip_data, HDMI_TX_2, hdmi.ip_data.txrx_cfg.vid1080p_tx2);	
				break;
			default:
				DEBUG_ERR("!!!no surpport this vid %d\n", ip_data->cfg.cm.code);
		}		
	}else{
		if(hdmi.hdmihw_diff->ic_type == IC_TYPE_ATM7059A){	
			switch (ip_data->cfg.cm.code)
			{
				case VID640x480P_60_4VS3:
					hdmi_write_reg(ip_data, HDMI_TX_1, 0x819c2983);	
					hdmi_write_reg(ip_data, HDMI_TX_2, 0x18f80f89);	
					break;	
				case VID720x576P_50_4VS3:
				case VID720x480P_60_4VS3:
					hdmi_write_reg(ip_data, HDMI_TX_1, 0x819c2983);	
					hdmi_write_reg(ip_data, HDMI_TX_2, 0x18f80f89);	
					break;		
				case VID1280x720P_60_16VS9:
				case VID1280x720P_50_16VS9:
				case VID1280x720P_60_DVI:
					hdmi_write_reg(ip_data, HDMI_TX_1, 0x81942983);	
					hdmi_write_reg(ip_data, HDMI_TX_2, 0x18f80f89);	
					break;
				case VID1920x1080P_60_16VS9:
				case VID1920x1080P_50_16VS9:
					hdmi_write_reg(ip_data, HDMI_TX_1, 0x81902983);	
					hdmi_write_reg(ip_data, HDMI_TX_2, 0x18f80f89);	
					break;
				default:
					DEBUG_ERR("!!!no surpport this vid %d\n", ip_data->cfg.cm.code);
			}
		}else{
			switch (ip_data->cfg.cm.code)
			{
				case VID640x480P_60_4VS3:
					hdmi_write_reg(ip_data, HDMI_TX_1, 0x819c0986);	
					hdmi_write_reg(ip_data, HDMI_TX_2, 0x18f80f89);	
					break;	
				case VID720x576P_50_4VS3:
				case VID720x480P_60_4VS3:
					hdmi_write_reg(ip_data, HDMI_TX_1, 0x819c0986);	
					hdmi_write_reg(ip_data, HDMI_TX_2, 0x18f80f89);	
					break;		
				case VID1280x720P_60_16VS9:
				case VID1280x720P_50_16VS9:
				case VID1280x720P_60_DVI:
					hdmi_write_reg(ip_data, HDMI_TX_1, 0x81982986);	
					hdmi_write_reg(ip_data, HDMI_TX_2, 0x18f80f89);
					break;
				case VID1920x1080P_60_16VS9:
				case VID1920x1080P_50_16VS9:
					hdmi_write_reg(ip_data, HDMI_TX_1, 0x81940986);	
					hdmi_write_reg(ip_data, HDMI_TX_2, 0x18f80f89);	
					break;
				default:
					DEBUG_ERR("!!!no surpport this vid %d\n", ip_data->cfg.cm.code);
			}	
		}
	}
}

static void ip_hdmi_phy_disable(struct hdmi_ip_data *ip_data)
{
	u32 val;

	// LDO_TMDS power off
    val = hdmi_read_reg(ip_data, HDMI_TX_2);
	val = REG_SET_VAL(val ,0, 17, 17);
	val = REG_SET_VAL(val ,0x0, 11, 8);
	hdmi_write_reg(ip_data, HDMI_TX_2, val);	
}

static void ip_hdmi_video_start(struct hdmi_ip_data *ip_data)
{
	u32 val;
	
	val = hdmi_read_reg(ip_data, TMDS_EODR0);
	val = REG_SET_VAL(val ,1, 31, 31);
	hdmi_write_reg(ip_data, TMDS_EODR0, val);	
	
	val = hdmi_read_reg(ip_data, HDMI_CR);
	val = REG_SET_VAL(val ,1, 0, 0);
	hdmi_write_reg(ip_data, HDMI_CR, val);

 /*        val = hdmi_read_reg(ip_data, HDMI_ICR);
         val = REG_SET_VAL(val ,1, 25, 25);
         hdmi_write_reg(ip_data, HDMI_ICR, val);
*/
	
}

static void ip_hdmi_video_stop(struct hdmi_ip_data *ip_data)
{
         u32 val;

         val = hdmi_read_reg(ip_data, HDMI_ICR);
         val = REG_SET_VAL(val ,0, 25, 25);
         hdmi_write_reg(ip_data, HDMI_ICR, val);

         val = hdmi_read_reg(ip_data, HDMI_CR);
         val = REG_SET_VAL(val ,0, 0, 0);
         hdmi_write_reg(ip_data, HDMI_CR, val);
         
}


static void ip_hdmi_dump(struct hdmi_ip_data *ip_data)
{
#define DUMPREG(name,r) DEBUG_ON("%s %08x\n",name,hdmi_read_reg(ip_data, r))
	
    DUMPREG("HDMI_VICTL  value is ", HDMI_VICTL);
    DUMPREG("HDMI_VIVSYNC  value is ", HDMI_VIVSYNC);
    DUMPREG("HDMI_VIVHSYNC  value is ", HDMI_VIVHSYNC);
    DUMPREG("HDMI_VIALSEOF  value is ", HDMI_VIALSEOF);
    DUMPREG("HDMI_VIALSEEF  value is ", HDMI_VIALSEEF);
    DUMPREG("HDMI_VIADLSE  value is ", HDMI_VIADLSE);
    DUMPREG("HDMI_VR  value is ", HDMI_VR);
    DUMPREG("HDMI_CR  value is ", HDMI_CR);
    DUMPREG("HDMI_SCHCR  value is ", HDMI_SCHCR);
    DUMPREG("HDMI_ICR  value is ", HDMI_ICR);
    DUMPREG("HDMI_SCR  value is ", HDMI_SCR);
    DUMPREG("HDMI_LPCR  value is ", HDMI_LPCR);
    DUMPREG("HDCP_CR  value is ", HDCP_CR);
    DUMPREG("HDCP_SR  value is ", HDCP_SR);
    DUMPREG("HDCP_ANLR  value is ", HDCP_ANLR);
    DUMPREG("HDCP_ANMR  value is ", HDCP_ANMR);
    DUMPREG("HDCP_ANILR  value is ", HDCP_ANILR);
    DUMPREG("HDCP_ANIMR  value is ", HDCP_ANIMR);
    DUMPREG("HDCP_DPKLR  value is ", HDCP_DPKLR);
    DUMPREG("HDCP_DPKMR  value is ", HDCP_DPKMR);
    DUMPREG("HDCP_LIR  value is ", HDCP_LIR);
    DUMPREG("HDCP_SHACR  value is ", HDCP_SHACR);
    DUMPREG("HDCP_SHADR  value is ", HDCP_SHADR);
    DUMPREG("HDCP_ICR  value is ", HDCP_ICR);
    DUMPREG("HDCP_KMMR  value is ", HDCP_KMMR);
    DUMPREG("HDCP_KMLR  value is ", HDCP_KMLR);
    DUMPREG("HDCP_MILR  value is ", HDCP_MILR);
    DUMPREG("HDCP_MIMR  value is ", HDCP_MIMR);
    DUMPREG("HDCP_KOWR  value is ", HDCP_KOWR);
    DUMPREG("HDCP_OWR  value is ", HDCP_OWR);

    DUMPREG("TMDS_STR0  value is ", TMDS_STR0);
    DUMPREG("TMDS_STR1  value is ", TMDS_STR1);
    DUMPREG("TMDS_EODR0  value is ", TMDS_EODR0);
    DUMPREG("TMDS_EODR1  value is ", TMDS_EODR1);
    DUMPREG("HDMI_ASPCR  value is ", HDMI_ASPCR);
    DUMPREG("HDMI_ACACR  value is ", HDMI_ACACR);
    DUMPREG("HDMI_ACRPCR  value is ", HDMI_ACRPCR);
    DUMPREG("HDMI_ACRPCTSR  value is ", HDMI_ACRPCTSR);
    DUMPREG("HDMI_ACRPPR value is ", HDMI_ACRPPR);
    DUMPREG("HDMI_GCPCR  value is ", HDMI_GCPCR);
    DUMPREG("HDMI_RPCR  value is ", HDMI_RPCR);
    DUMPREG("HDMI_RPRBDR  value is ", HDMI_RPRBDR);
    DUMPREG("HDMI_OPCR  value is ", HDMI_OPCR);
    DUMPREG("HDMI_DIPCCR  value is ", HDMI_DIPCCR);
    DUMPREG("HDMI_ORP6PH  value is ", HDMI_ORP6PH);
    DUMPREG("HDMI_ORSP6W0  value is ", HDMI_ORSP6W0);
    DUMPREG("HDMI_ORSP6W1  value is ", HDMI_ORSP6W1);
    DUMPREG("HDMI_ORSP6W2  value is ", HDMI_ORSP6W2);
    DUMPREG("HDMI_ORSP6W3  value is ", HDMI_ORSP6W3);
    DUMPREG("HDMI_ORSP6W4  value is ", HDMI_ORSP6W4);
    DUMPREG("HDMI_ORSP6W5  value is ", HDMI_ORSP6W5);
    DUMPREG("HDMI_ORSP6W6v  value is ", HDMI_ORSP6W6);
    DUMPREG("HDMI_ORSP6W7  value is ", HDMI_ORSP6W7);
    DUMPREG("HDMI_CECCR  value is ", HDMI_CECCR);
    DUMPREG("HDMI_CECRTCR  value is ", HDMI_CECRTCR);
    DUMPREG("HDMI_CRCCR  value is ", HDMI_CRCCR);
    DUMPREG("HDMI_CRCDOR  value is ", HDMI_CRCDOR);
    DUMPREG("HDMI_TX_1  value is ", HDMI_TX_1);
    DUMPREG("HDMI_TX_2  value is ", HDMI_TX_2);
    DUMPREG("CEC_DDC_HPD  value is ", CEC_DDC_HPD);

}

static void ip_hdmi_basic_configure(struct hdmi_ip_data *ip_data)
{

	/* HDMI */
	struct owl_video_timings video_timing;
	struct hdmi_video_format video_format;

	/* HDMI core */
	struct hdmi_config *cfg = &ip_data->cfg;

	/* video config */
	hdmi_video_init_format(&video_format, &video_timing, cfg);	

	/*HDMI_SCHCR vinster hinster bit 2:1
	HDMI_VICTL POR*/
	hdmi_video_config_timing(ip_data, &video_timing);
	/*HDMI_VICTL*/
	hdmi_video_config_format(ip_data, &video_timing);
	/*HDMI_VIVSYNC
	HDMI_VIVHSYNC
	HDMI_VIALSEOF
	HDMI_VIALSEEF
	HDMI_VIADLSE*/
	hdmi_video_config_interface(ip_data,&video_timing);	
	/*DIPCCR*/
	hdmi_video_interval_packet(ip_data,&video_timing);	
	/*HDMI_ICR*/	
	hdmi_core_config_input_src(ip_data);
	/*HDMI_SCHCR*/	
	hdmi_core_config_pixel_fomat(ip_data);
	/*HDMI_SCHCR*/	
	hdmi_core_config_deepcolor_mode(ip_data);
	/*HDMI_SCHCR*/	
	hdmi_core_config_mode(ip_data);
	/*HDMI_SCHCR*/	
	hdmi_core_config_invert(ip_data);
	/*HDMI_GCPCR*/	
	hdmi_core_config_colordepth_value(ip_data);
	/*HDMI_SCHCR*/	
	hdmi_core_config_3d_mode(ip_data);	
	
	asoc_hdmi_gen_infoframe(ip_data);
	/*ip_hdmi_dump(ip_data);*/
}

static const struct owl_hdmi_ip_ops owl_hdmi_functions = {
	.hdmi_devclken		=	ip_hdmi_devclken,
	.hdmi_clk24Men		=	ip_hdmi_clk24Men,
	.hdmi_reset 		=   ip_hdmi_reset,
	.pmds_ldo_enable    =   ip_hdmi_pmds_ldo_enable,
	.pll_enable			=	ip_hdmi_pll_enable,
	.pll_disable		=	ip_hdmi_pll_disable,
	.read_reg 			=   ip_hdmi_read_reg,
	.write_reg 			=   ip_hdmi_write_reg,
	.cable_check		=	ip_hdmi_cable_check_state,
	.detect			    =	ip_hdmi_detect,
	.hpd_enable		    =	ip_hdmi_hpd_enable,
	.hpd_clear_plug     =   ip_hdmi_clear_plugstatus,
	.phy_enable		    =	ip_hdmi_phy_enable,
	.phy_disable		=	ip_hdmi_phy_disable,	
	.video_enable		=	ip_hdmi_video_start,
	.video_disable		=	ip_hdmi_video_stop,	
	.dump_hdmi		    =	ip_hdmi_dump,
	.video_configure	=	ip_hdmi_basic_configure,
	.get_irq_state      =   ip_hdmi_get_irq_state,

};

void dss_init_hdmi_ip_ops(struct hdmi_ip_data *ip_data)
{
	ip_data->ops = &owl_hdmi_functions;

	WARN_ON(ip_data->ops == NULL);
}
