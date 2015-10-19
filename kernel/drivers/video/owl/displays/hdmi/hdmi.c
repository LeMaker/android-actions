/*
 * hdmi.c
 *
 * HDMI interface DSS driver setting for TI's OWL4 family of processor.
 * Copyright (C) 2010-2011 Texas Instruments Incorporated - http://www.ti.com/
 * Authors: Yong Zhi
 *	Mythri pk <mythripk@ti.com>
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

#define DSS_SUBSYS_NAME "HDMI"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/switch.h>
#include <mach/irqs.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/earlysuspend.h>
#include <video/owldisplay.h>
#include <asm/atomic.h>
#include <linux/sched.h>    
#include <linux/kthread.h> 
#include <linux/timer.h>
#include <linux/jiffies.h>

#include <mach/clkname.h>
#include <mach/module-owl.h>

#include "../../dss/dss_features.h"
#include "../../dss/dss.h"

#include "hdmi_ip.h"
#include "hdmi.h"

#define SUSPEND_IN_DSS

struct hdmi_core hdmi;

struct timer_list hdmi_timer;

struct switch_dev hdev = {
	.name = "hdmi",
};
struct switch_dev hdev_audio = {
	.name = "audio_hdmi",
};
struct hdmi_property
{
	int hdcp_onoff;
	int channel_invert;
	int bit_invert;
	
	int lightness;
	int saturation;
	int contrast;
} ;

struct hdmi_property hdmi_data;
static struct work_struct irq_work;
static struct delayed_work hdmi_cable_check_work;

atomic_t hdmi_status = ATOMIC_INIT(0);
atomic_t need_change_timeing = ATOMIC_INIT(0);

struct ic_info {
	int ic_type;
};

static struct ic_info  atm7039c_data = {
   .ic_type = IC_TYPE_ATM7039C,
};

static  struct ic_info atm7059tc_data = {
   .ic_type = IC_TYPE_ATM7059TC,
};
 
static  struct ic_info atm7059a_data = {
   .ic_type = IC_TYPE_ATM7059A,
};

static const struct of_device_id atm70xx_hdmi_of_match[] = {

	{.compatible = "actions,atm7039c-hdmi", .data = &atm7039c_data},
	
	{.compatible = "actions,atm7059tc-hdmi", .data = &atm7059tc_data},

	{.compatible = "actions,atm7059a-hdmi", .data = &atm7059a_data},

	{}
};

static struct hw_diff hdmi_atm7039c = {
	.ic_type = IC_TYPE_ATM7039C,
	.hp_start = 5,
	.hp_end = 5,
};

static struct hw_diff hdmi_atm7059tc = {
	.ic_type = IC_TYPE_ATM7059TC,
	.hp_start 	= 16,
	.hp_end	 	= 28,
	.vp_start 	= 4,
	.vp_end 	= 15,
	.mode_start = 0,
	.mode_end 	= 0,
};

static struct hw_diff hdmi_atm7059a = {
	.ic_type = IC_TYPE_ATM7059A,
	.hp_start 	= 16,
	.hp_end	 	= 28,
	.vp_start 	= 4,
	.vp_end 	= 15,
	.mode_start = 0,
	.mode_end 	= 0,
};

MODULE_DEVICE_TABLE(of, atm70xx_hdmi_of_match);

/*
 * Logic for the below structure :
 * user enters the CEA or VESA timings by specifying the HDMI code.
 * There is a correspondence between CEA/VESA timing and code, please
 * refer to section 6.3 in HDMI 1.3 specification for timing code.
 */
struct data_fmt_param {
    const char *name;
    s32 data_fmt;
};

static struct data_fmt_param date_fmts[] = {
	{"720P50HZ", OWL_TV_MOD_720P_50HZ},
	{"720P60HZ", OWL_TV_MOD_720P_60HZ},
	{"1080P50HZ", OWL_TV_MOD_1080P_50HZ},
	{"1080P60HZ", OWL_TV_MOD_1080P_60HZ},
	{"576P", OWL_TV_MOD_576P},
	{"580P", OWL_TV_MOD_480P},
	{"DVI", OWL_TV_MOD_DVI},
	{"PAL", OWL_TV_MOD_PAL},
	{"NTSC", OWL_TV_MOD_NTSC},
	{"4K30HZ", OWL_TV_MOD_4K_30HZ},
};

static const struct hdmi_config cea_timings[] = {
	{
		{ 720, 480, 27027, 62, 16, 60, 6, 9, 30,
			OWLDSS_SIG_ACTIVE_LOW, OWLDSS_SIG_ACTIVE_LOW,
			false, 7, 0, },
		{ 2, HDMI_HDMI },
	},
	{
		{ 1280, 720, 74250, 40, 110, 220, 5, 5, 20,
			OWLDSS_SIG_ACTIVE_HIGH, OWLDSS_SIG_ACTIVE_HIGH,
			false, 1, 0, },
		{ 4, HDMI_HDMI },
	},
	{
		{ 1920, 1080, 148500, 44, 88, 148, 5, 4, 36,
			OWLDSS_SIG_ACTIVE_HIGH, OWLDSS_SIG_ACTIVE_HIGH,
			false, 1, 0, },
		{ 16, HDMI_HDMI },
	},
	{
		{ 720, 576, 27000, 64, 12, 68, 5, 5, 39,
			OWLDSS_SIG_ACTIVE_LOW, OWLDSS_SIG_ACTIVE_LOW,
			false, 1, 0, },
		{ 17, HDMI_HDMI },
	},
	{
		{ 1280, 720, 74250, 40, 440, 220, 5, 5, 20,
			OWLDSS_SIG_ACTIVE_HIGH, OWLDSS_SIG_ACTIVE_HIGH,
			false, 1, 0, },
		{ 19, HDMI_HDMI },
	},
	{
		{ 1920, 1080, 148500, 44, 528, 148, 5, 4, 36,
			OWLDSS_SIG_ACTIVE_HIGH, OWLDSS_SIG_ACTIVE_HIGH,
			false, 1, 0, },
		{ 31, HDMI_HDMI },
	},
	{
		{ 1280, 720, 74250, 40, 110, 220, 5, 5, 20,
			OWLDSS_SIG_ACTIVE_HIGH, OWLDSS_SIG_ACTIVE_HIGH,
			false, 1, 0, },
		{ 126, HDMI_DVI },
	},
};

const int hdmi_vid_table[OWL_TV_MODE_NUM + 1]= {-1,19,4,31,16,17,2,126,0};

#if 0
static void hdmi_print_info(struct owl_dss_device *dssdev)
{
	HDMI_DEBUG("~~~~~~~~~hdmi_print_info\n");

	HDMI_DEBUG("x_res		= %d  %d\n",dssdev->timings.x_res, hdmi.ip_data.cfg.timings.x_res);
	HDMI_DEBUG("y_res		= %d  %d\n",dssdev->timings.y_res, hdmi.ip_data.cfg.timings.y_res);
	HDMI_DEBUG("pixel_clock	= %d  %d\n",dssdev->timings.pixel_clock, hdmi.ip_data.cfg.timings.pixel_clock);
	HDMI_DEBUG("hsw			= %d  %d\n",dssdev->timings.hsw, hdmi.ip_data.cfg.timings.hsw);
	HDMI_DEBUG("hfp			= %d  %d\n",dssdev->timings.hfp, hdmi.ip_data.cfg.timings.hfp);
	HDMI_DEBUG("hbp			= %d  %d\n",dssdev->timings.hbp, hdmi.ip_data.cfg.timings.hbp);
	HDMI_DEBUG("vsw			= %d  %d\n",dssdev->timings.vsw, hdmi.ip_data.cfg.timings.vsw);
	HDMI_DEBUG("vfp			= %d  %d\n",dssdev->timings.vfp, hdmi.ip_data.cfg.timings.vfp);
	HDMI_DEBUG("vbp         = %d  %d\n",dssdev->timings.vbp, hdmi.ip_data.cfg.timings.vbp);	
	HDMI_DEBUG("vsync_level = %d  %d\n",dssdev->timings.vsync_level, hdmi.ip_data.cfg.timings.vsync_level);
	HDMI_DEBUG("hsync_level = %d  %d\n",dssdev->timings.hsync_level, hdmi.ip_data.cfg.timings.hsync_level);
	HDMI_DEBUG("interlace	= %d  %d\n",dssdev->timings.interlace, hdmi.ip_data.cfg.timings.interlace);
	HDMI_DEBUG("code		= %d	\n",hdmi.ip_data.cfg.cm.code);
	HDMI_DEBUG("mode		= %d	\n",hdmi.ip_data.cfg.cm.mode);
	HDMI_DEBUG("hdmi_src	= %d	\n",hdmi.ip_data.settings.hdmi_src);
}
#endif

static int hdmi_init_display(struct owl_dss_device *dssdev)
{

	HDMI_DEBUG("init_display\n");

	dss_init_hdmi_ip_ops(&hdmi.ip_data);

	return 0;
}

static const struct hdmi_config *hdmi_find_timing(
					const struct hdmi_config *timings_arr,
					int len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (timings_arr[i].cm.code == hdmi.ip_data.cfg.cm.code)
			return &timings_arr[i];
	}
	return NULL;
}

static const struct hdmi_config *hdmi_get_timings(void)
{
       const struct hdmi_config *arr;
       int len;
	   arr = cea_timings;
	   len = ARRAY_SIZE(cea_timings);
       return hdmi_find_timing(arr, len);
}

static bool hdmi_timings_compare(struct owl_video_timings *timing1,
				const struct owl_video_timings *timing2)
{
	int timing1_vsync, timing1_hsync, timing2_vsync, timing2_hsync;

	if ((DIV_ROUND_CLOSEST(timing2->pixel_clock, 1000) ==
			DIV_ROUND_CLOSEST(timing1->pixel_clock, 1000)) &&
		(timing2->x_res == timing1->x_res) &&
		(timing2->y_res == timing1->y_res)) {

		timing2_hsync = timing2->hfp + timing2->hsw + timing2->hbp;
		timing1_hsync = timing1->hfp + timing1->hsw + timing1->hbp;
		timing2_vsync = timing2->vfp + timing2->vsw + timing2->vbp;
		timing1_vsync = timing2->vfp + timing2->vsw + timing2->vbp;

		HDMI_DEBUG("timing1_hsync = %d timing1_vsync = %d"\
			"timing2_hsync = %d timing2_vsync = %d\n",
			timing1_hsync, timing1_vsync,
			timing2_hsync, timing2_vsync);

		if ((timing1_hsync == timing2_hsync) &&
			(timing1_vsync == timing2_vsync)) {
			return true;
		}
	}
	return false;
}

static struct hdmi_cm hdmi_get_code(struct owl_video_timings *timing)
{
	int i;
	struct hdmi_cm cm = {-1};
	HDMI_DEBUG("hdmi_get_code\n");

	for (i = 0; i < ARRAY_SIZE(cea_timings); i++) {
		if (hdmi_timings_compare(timing, &cea_timings[i].timings)) {
			cm = cea_timings[i].cm;
			goto end;
		}
	}

end:	return cm;

}
static bool  is_hdmi_power_on(void)
{
	return ((hdmihw_read_reg(HDMI_CR) & 0x01) != 0);
}

#define DECLK2_MIN		150000000
static unsigned long de_clk2_rate = 0;
void  save_declk_and_switch_for_hdmi()
{
	struct clk      *de2_clk;
	unsigned long  de2_new_clk = 0;
	
	de2_clk     = clk_get(NULL, CLKNAME_DE2_CLK);
	
	//save clk 
	de_clk2_rate = clk_get_rate(de2_clk);
	
	if(de_clk2_rate < DECLK2_MIN)
	{
		//set new clk 	
		de2_new_clk = clk_round_rate(de2_clk, DECLK2_MIN);
		
		clk_set_rate(de2_clk,de2_new_clk);
		
		clk_prepare(de2_clk);
		
	    clk_enable(de2_clk);
	    printk("save_declk_and_switch_for_hdmi de2_new_clk %d MHZ\n " ,de2_new_clk);	
	}	
}
void  restore_declk_for_hdmi()
{
	struct clk      *de2_clk;
	unsigned long  de2_new_clk = 0;
	
	de2_clk     = clk_get(NULL, CLKNAME_DE2_CLK);
	printk("restore_declk_for_hdmi de_clk2_rate %d MHZ\n " ,de_clk2_rate);
	if(de_clk2_rate != 0 && clk_get_rate(de2_clk) != de_clk2_rate)
	{
	
		de2_new_clk = clk_round_rate(de2_clk, de_clk2_rate);
		
		clk_set_rate(de2_clk,de2_new_clk);
		
		clk_prepare(de2_clk);
		
	    clk_enable(de2_clk);	
	}	
}
static int hdmi_power_on_full(struct owl_dss_device *dssdev)
{
	int r;
	struct owl_video_timings *timings;
	struct owl_overlay_manager *mgr = dssdev->manager;
	timings = &dssdev->timings;
	if(atomic_read(&need_change_timeing)==1){
		timings->x_res 			= 	hdmi.ip_data.cfg.timings.x_res;
		timings->y_res 		 	= 	hdmi.ip_data.cfg.timings.y_res;
		timings->pixel_clock 	= 	hdmi.ip_data.cfg.timings.pixel_clock;
		timings->hsw 		 	= 	hdmi.ip_data.cfg.timings.hsw;
		timings->hfp 			= 	hdmi.ip_data.cfg.timings.hfp;
		timings->hbp 		 	= 	hdmi.ip_data.cfg.timings.hbp;
		timings->vsw 			= 	hdmi.ip_data.cfg.timings.vsw;
		timings->vfp 			= 	hdmi.ip_data.cfg.timings.vfp;
		timings->vbp 			= 	hdmi.ip_data.cfg.timings.vbp;	
		atomic_set(&need_change_timeing, 0);
	}
			
	if(hdmi.ip_data.cfg.cm.code == VID1280x720P_60_DVI){
		hdmi.ip_data.settings.hdmi_mode = HDMI_DVI;
	}else{
		hdmi.ip_data.settings.hdmi_mode = hdmi.edid.isHDMI;
	}
    if(is_hdmi_power_on())
    {
    	return 0;
    }

	//hdmi.ip_data.ops->hdmi_devclken(&hdmi.ip_data, 1);

	hdmi.ip_data.ops->hdmi_clk24Men(&hdmi.ip_data, 1);	
	
	hdmi.ip_data.ops->pll_enable(&hdmi.ip_data);
	
	hdmi.ip_data.ops->hdmi_reset(&hdmi.ip_data);
	
	hdmi.ip_data.ops->hpd_clear_plug(&hdmi.ip_data);	
	
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, 1);
	
	r = dss_mgr_enable(mgr);
	if (r)
		goto err_mgr_enable;
	DEBUG_ON("dss_mgr_enable end\n");	
		
	hdmi.ip_data.ops->video_configure(&hdmi.ip_data);

	hdmi.ip_data.ops->video_enable(&hdmi.ip_data);
	
	hdmi.ip_data.ops->phy_enable(&hdmi.ip_data);		
	DEBUG_ON("hdmi_power_on_full end\n");	

	return 0;

err_mgr_enable:
	hdmi.ip_data.ops->video_disable(&hdmi.ip_data);
	hdmi.ip_data.ops->phy_disable(&hdmi.ip_data);
	return -EIO;
}

static void hdmi_power_off_full(struct owl_dss_device *dssdev)
{
	struct owl_overlay_manager *mgr = dssdev->manager;
	
	hdmi.ip_data.ops->phy_disable(&hdmi.ip_data);	
	hdmi.ip_data.ops->video_disable(&hdmi.ip_data);
		
	dss_mgr_disable(mgr);
	
	hdmi.ip_data.ops->pll_disable(&hdmi.ip_data);
	hdmi.ip_data.ops->hdmi_clk24Men(&hdmi.ip_data, 0);	
	
	restore_declk_for_hdmi();	
}

static int hdmi_set_settings(struct hdmi_ip_data *ip_data)
{
	ip_data->settings.hdmi_src = DE;
	ip_data->settings.vitd_color = 0xff0000;
	ip_data->settings.enable_3d = NOT_3D;
	ip_data->settings.present_3d = NOT_3D;
	ip_data->settings.pixel_encoding = RGB444;
	ip_data->settings.color_xvycc = 0;
	ip_data->settings.deep_color = color_mode_24bit;
	ip_data->settings.hdmi_mode = HDMI_HDMI;
	
	ip_data->settings.channel_invert = hdmi_data.channel_invert;
	ip_data->settings.bit_invert = hdmi_data.bit_invert;
	return 0;	
}

static void hdmi_display_set_dssdev(struct owl_dss_device *dssdev)
{
	hdmi.dssdev = dssdev;
}

static int hdmi_probe_pdata(struct platform_device *pdev)
{
	struct owl_dss_device *dssdev;
	int r;
	
	r = hdmi_init_display(dssdev);
	if (r) {
		DEBUG_ERR("device %s init failed: %d\n", dssdev->name, r);
		return r;
	}

	return 0;
}

void hdmi_send_uevent(bool data)
{
#ifndef CONFIG_VIDEO_OWL_NO_DVI
	mutex_lock(&hdmi.ip_data.lock);
	/*HDMI_DEBUG("~~~hdmi_send_uevent %d  %d\n", data, atomic_read(&hdmi_status));*/
	if(data){
		if(atomic_read(&hdmi_status)==0){
			HDMI_DEBUG("parse_edid start\n");
			parse_edid(&hdmi.edid);
			HDMI_DEBUG("parse_edid end\n");		
			switch_set_state(&hdev, 1);	
			atomic_set(&hdmi_status, 1);
		}
	}else{
		if((atomic_read(&hdmi_status)==1)&&hdmi.data.send_uevent){
			switch_set_state(&hdev, 0);	
			atomic_set(&hdmi_status, 0);
		}	
	}
	mutex_unlock(&hdmi.ip_data.lock);
#endif
}

void hdmi_cable_check(struct work_struct *work)
{
	/*HDMI_DEBUG("hdmi_cable_check  ~~~~~~~~\n");*/	
	if(hdmi.data.hpd_en&&hdmi.data.cable_check_onoff){
		if(hdmi.ip_data.ops->cable_check(&hdmi.ip_data)){		
			hdmi_send_uevent(1);
		}else{
			hdmi_send_uevent(0);
		}
	}
	queue_delayed_work(hdmi.ip_data.hdcp.wq, &hdmi_cable_check_work,
				msecs_to_jiffies(2000));
}

int hdmi_cable_check_init(void)
{
	queue_delayed_work(hdmi.ip_data.hdcp.wq, &hdmi_cable_check_work,
				msecs_to_jiffies(2000));
	return 0;
}

static irqreturn_t hpd_irq_handler(int irq, void *data)
{
	DEBUG_ON("[%s start]\n", __func__);
	hdmi.data.hpd_pending  = hdmi.ip_data.ops->detect(&hdmi.ip_data);
	hdmi.data.cable_status = hdmi.ip_data.ops->cable_check(&hdmi.ip_data);
	hdmi.data.hdcp_ri      = check_ri_irq();	
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, 0);		
	schedule_work(&irq_work);	
	DEBUG_ON("[%s end]\n", __func__);	
	return IRQ_HANDLED;
}

static void do_hpd_irq(struct work_struct *work) 
{
	bool sta_new;	
	DEBUG_ON("~~~~~~~~~~~~~irq_handler  %d\n", hdmi.data.hdmi_sta);
	if(hdmi.data.hpd_pending){
		if(hdmi_data.hdcp_onoff){
			msleep(10);
		}else{
			msleep(50);
		}
		sta_new = hdmi.ip_data.ops->cable_check(&hdmi.ip_data);
		DEBUG_ON("sta_new %d old %d\n", sta_new, hdmi.data.cable_status);
		if(sta_new==hdmi.data.cable_status){
			if(sta_new){
				DEBUG_ON("~~~~~~~~~~~~~hdmi plug in\n");					
				hdmi_send_uevent(1);
			}else{
				DEBUG_ON("~~~~~~~~~~~~~hdmi plug out\n");
				hdmi_send_uevent(0);
			}
		}
		hdmi.data.hpd_pending = false;
	}
	
	mutex_lock(&hdmi.lock);	
	hdmi.ip_data.ops->hpd_clear_plug(&hdmi.ip_data);	
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, 1);
	mutex_unlock(&hdmi.lock);	
	
	if((hdmi_data.hdcp_onoff) && (hdmi.data.hdcp_ri)){
        hdcp_check_ri();
	}
	
}

int owldss_hdmi_display_check_timing(struct owl_dss_device *dssdev,
					struct owl_video_timings *timings)
{
	struct hdmi_cm cm;

	cm = hdmi_get_code(timings);
	if (cm.code == -1) {
		return -EINVAL;
	}

	return 0;

}

void owldss_hdmi_display_set_timing(struct owl_dss_device *dssdev,	struct owl_video_timings *timings)
{
	const struct hdmi_config *t;

	mutex_lock(&hdmi.lock);
	
	t = hdmi_get_timings();
	if (t != NULL)
		hdmi.ip_data.cfg = *t;
		
	mutex_unlock(&hdmi.lock);
}

void owldss_hdmi_display_set_vid(struct owl_dss_device *dssdev,	struct owl_video_timings *timings, int vid)
{
	const struct hdmi_config *t;

	mutex_lock(&hdmi.lock);

	hdmi.ip_data.cfg.cm.code = hdmi_vid_table[vid];
	
	DEBUG_ON("~~~~~~~~~~~~owldss_hdmi_display_set_vid %d\n", hdmi.ip_data.cfg.cm.code);
	
	t = hdmi_get_timings();
	if (t != NULL){
		hdmi.ip_data.cfg = *t;
		atomic_set(&need_change_timeing, 1);
	}

	hdmi.ip_data.cfg.cm.mode = hdmi.edid.isHDMI;

	mutex_unlock(&hdmi.lock);
}

void owldss_hdmi_display_get_vid(struct owl_dss_device *dssdev, int *vid)
{
    int i = 0;
	mutex_lock(&hdmi.lock);
	
	for(i = 0 ; i < OWL_TV_MODE_NUM; i++){
		if(hdmi_vid_table[i] == hdmi.ip_data.cfg.cm.code){
			break;
		}
	}
	*vid = i;
	
	mutex_unlock(&hdmi.lock);
}

void owldss_hdmi_display_enable_hotplug(struct owl_dss_device *dssdev, bool enable)
{
	mutex_lock(&hdmi.lock);	
	HDMI_DEBUG("owldss_hdmi_display_enable_hotplug  enable %d\n", enable);
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, enable);
	if(enable){
		if(hdmi.ip_data.ops->cable_check(&hdmi.ip_data)){
			HDMI_DEBUG("owldss_hdmi_display_enable_hotplug  switch_set_state 1\n");
			hdmi_send_uevent(1);
		}
		hdmi.data.hpd_en = 1;
	}else{
		if(hdmi.ip_data.ops->cable_check(&hdmi.ip_data)){
			HDMI_DEBUG("owldss_hdmi_display_enable_hotplug  switch_set_state 0\n");
			hdmi_send_uevent(0);
		}
		hdmi.data.hpd_en = 0;
	}
	mutex_unlock(&hdmi.lock);
}

void owldss_hdmi_display_enable_hdcp(struct owl_dss_device *dssdev, bool enable)
{
	mutex_lock(&hdmi.lock);	
	
	if(enable){
		hdmi_data.hdcp_onoff = 1;
	}else{
		hdmi_data.hdcp_onoff = 0;
	}
	mutex_unlock(&hdmi.lock);
}

int owldss_hdmi_display_get_vid_cap(struct owl_dss_device *dssdev, int *vid_cap)
{
	int i=0;
	mutex_lock(&hdmi.lock);
	if(hdmi.edid.video_formats[0]==0){
		if(hdmi.ip_data.ops->cable_check(&hdmi.ip_data)){
			HDMI_DEBUG("parse_edid start\n");
			parse_edid(&hdmi.edid);
			HDMI_DEBUG("parse_edid end\n");	
		}else{
			hdmi.edid.video_formats[0]=0x80090014;
			DEBUG_ERR("NO HDMI CABLE\n");
		}	
	}
	for(i = 1 ; i < OWL_TV_MODE_NUM + 1; i++){
		if(((hdmi.edid.video_formats[0]>>hdmi_vid_table[i])&0x01)==1){			
			*vid_cap = i;		
			vid_cap++;	
		}
	}
	
	if(hdmi.edid.read_ok==0){
		*vid_cap = OWL_TV_MOD_DVI;		
		vid_cap++;				
	}
	
	mutex_unlock(&hdmi.lock);
	return 8;
}

int owldss_hdmi_display_get_cable_status(struct owl_dss_device *dssdev)
{
	int r;
	mutex_lock(&hdmi.lock);
	r = hdmi.ip_data.ops->cable_check(&hdmi.ip_data);
	HDMI_DEBUG("ENTER owldss_hdmi_display_get_cable_status %d\n",r);
	mutex_unlock(&hdmi.lock);
	return r;
}

int owldss_hdmi_display_enable(struct owl_dss_device *dssdev)
{

	struct owl_overlay_manager *mgr = dssdev->manager;
	int r = 0;

	DEBUG_ON("ENTER hdmi_display_enable\n");

	mutex_lock(&hdmi.lock);

    if (mgr == NULL) {
		DEBUG_ERR("failed to enable display: no manager\n");
		r = -ENODEV;
		goto err0;
	}
	   
        save_declk_and_switch_for_hdmi();
    
	r = owl_dss_start_device(dssdev);
	
	if (r) {
		DEBUG_ERR("failed to start device\n");
		goto err0;
	}
	HDMI_DEBUG("owl_dss_start_device end\n");	
	
	r = hdmi_power_on_full(dssdev);
	if (r) {
		DEBUG_ERR("failed to power on device\n");
		goto err1;
	}	

	if(hdmi_data.hdcp_onoff){
		hdcp_init();

		queue_delayed_work(hdmi.ip_data.hdcp.wq, &hdmi.ip_data.hdcp.hdcp_work,
					msecs_to_jiffies(50));
	}			
	if(hdmi.ip_data.settings.hdmi_mode == HDMI_HDMI){
		switch_set_state(&hdev_audio, 1);
	}
	/*hdmi_dump_regs();*/
	
	mutex_unlock(&hdmi.lock);
	return 0;

err1:
	owl_dss_stop_device(dssdev);
err0:
	restore_declk_for_hdmi();
	mutex_unlock(&hdmi.lock);
	return r;
}

int owldss_hdmi_hotplug_debug(struct owl_dss_device *dssdev,int enable)
{
	if(enable){
		DEBUG_ON("~~~~~~~~~~~~~hdmi plug in\n");					
		hdmi_send_uevent(1);
	}else{
		DEBUG_ON("~~~~~~~~~~~~~hdmi plug out\n");
		hdmi_send_uevent(0);
	}
	return 0;
}

int owldss_hdmi_cable_debug(struct owl_dss_device *dssdev,int enable)
{
	if(enable){
		hdmi.data.cable_check_onoff = 1;
	}else{
		hdmi.data.cable_check_onoff = 0;
	}
	DEBUG_ON("~~~~~~~~~~~~~owldss_hdmi_cable_debug %d\n", hdmi.data.cable_check_onoff );
	return 0;
}

int owldss_hdmi_uevent_debug(struct owl_dss_device *dssdev,int enable)
{
	if(enable){
		hdmi.data.send_uevent = 1;
	}else{
		hdmi.data.send_uevent = 0;
	}	
	DEBUG_ON("~~~~~~~~~~~~~owldss_hdmi_uevent_debug %d\n", hdmi.data.send_uevent );
	return 0;
}

void owldss_hdmi_display_disable(struct owl_dss_device *dssdev)
{
	DEBUG_ON("Enter hdmi_display_disable\n");

	mutex_lock(&hdmi.lock);

	hdmi_power_off_full(dssdev);

	owl_dss_stop_device(dssdev);
	
	if(hdmi.ip_data.settings.hdmi_mode == HDMI_HDMI){
		switch_set_state(&hdev_audio, 2);
	}

	mutex_unlock(&hdmi.lock);
}

int owl_hdmi_get_effect_parameter(struct owl_dss_device *dssdev, enum owl_plane_effect_parameter parameter_id)
{

    switch(parameter_id){
    	case OWL_DSS_VIDEO_LIGHTNESS:
    		return hdmi_data.lightness;
    	case OWL_DSS_VIDEO_SATURATION:
    		return hdmi_data.saturation;
    	case OWL_DSS_VIDEO_CONSTRAST:
    		return hdmi_data.contrast;
    	default:
    		printk("invalid plane effect parameter \n");
    		return -1;
    }
}

void owl_hdmi_set_effect_parameter(struct owl_dss_device *dssdev,enum owl_plane_effect_parameter parameter_id ,int value)
{
     switch(parameter_id){
    	case OWL_DSS_VIDEO_LIGHTNESS:
    		hdmi_data.lightness = value;
    		break;
    	case OWL_DSS_VIDEO_SATURATION:
    		hdmi_data.saturation = value;
    		break;
    	case OWL_DSS_VIDEO_CONSTRAST:
    		hdmi_data.contrast = value;
    		break;
    	default:
    		printk("invalid plane effect parameter parameter_id %d value %d\n",parameter_id,value);
    		break;
    }

}

int owldss_hdmi_panel_init(struct owl_dss_device *dssdev)
{	
	int r;
	HDMI_DEBUG("owl_dss_device  hdmi  p->0x%p\n", dssdev);
	hdmi_display_set_dssdev(dssdev);
	hdmi_set_settings(&hdmi.ip_data);	
	HDMI_DEBUG("hpd_enable start\n");
	
	if(hdmi.hdmihw_diff->ic_type == IC_TYPE_ATM7059TC){		
		hdmi.ip_data.ops->hdmi_devclken(&hdmi.ip_data, 1);
		hdmi.ip_data.ops->hdmi_reset(&hdmi.ip_data);
	}
	
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, 0);
	
	r = request_irq(OWL_IRQ_HDMI, hpd_irq_handler, 0, "hdmidev", NULL);	
	
	if (r) {
		DEBUG_ERR(" register irq failed!\n");
		return r;
	} else {
		DEBUG_ON(" register irq ON!\n");
	}		
	hdmi_cable_check_init();
	return 0;
}

int owldss_hdmi_panel_suspend(struct owl_dss_device *dssdev)
{
	DEBUG_ON("owldss_hdmi_panel_suspend \n");
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, 0);
	cancel_delayed_work_sync(&hdmi_cable_check_work);
	cancel_delayed_work_sync(&hdmi.ip_data.hdcp.hdcp_work);
	cancel_work_sync(&irq_work);
	//flush_workqueue(hdmi.ip_data.hdcp.wq);
	//flush_scheduled_work();
	return 0;
}

int owldss_hdmi_panel_resume(struct owl_dss_device *dssdev)
{
	DEBUG_ON("owldss_hdmi_panel_resume \n");
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, hdmi.data.hpd_en);

	queue_delayed_work(hdmi.ip_data.hdcp.wq, &hdmi_cable_check_work,
				msecs_to_jiffies(2000));
	return 0;
}
static u32 string_to_data_fmt(const char *name)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(date_fmts); i++) {
		if (!strcmp(date_fmts[i].name, name))
			return date_fmts[i].data_fmt;
	}

	return -1;
}
static int of_get_hdmi_data(struct platform_device *pdev, struct hdmi_property *hdmi_data)
{
	struct device_node *of_node;
	char const *default_resulation;
	int index = 1;

	HDMI_DEBUG("%s\n", __func__);

	of_node = pdev->dev.of_node;

	if (of_property_read_u32_array(of_node, "hdcp_onoff", &hdmi_data->hdcp_onoff , 1)) {
		hdmi_data->hdcp_onoff = 0;
		//DEBUG_ON("cat not get hdcp_onoff  %d\n", hdmi_data->hdcp_onoff);
	}
	if (of_property_read_u32_array(of_node, "channel_invert", &hdmi_data->channel_invert , 1)) {
		hdmi_data->channel_invert = 0;
		//DEBUG_ON("cat not get channel_invert  %d\n", hdmi_data->channel_invert);
	}
	if (of_property_read_u32_array(of_node, "bit_invert", &hdmi_data->bit_invert , 1)) {
		hdmi_data->bit_invert = 0;
		//DEBUG_ON("cat not get bit_invert  %d\n", hdmi_data->bit_invert);
	}
	if (of_property_read_string(of_node, "default_resolution", &default_resulation)) {
		default_resulation = NULL;
		//DEBUG_ON("cat not get bit_invert  %d\n", hdmi_data->bit_invert);
	}
	index = string_to_data_fmt(default_resulation);
	
	if(index <= 0 )
	{
		index = 1;
	}
	
	hdmi.ip_data.cfg.cm.code = hdmi_vid_table[index];

	if (of_property_read_u32_array(of_node, "lightness", &hdmi_data->lightness , 1)) {
		hdmi_data->lightness = DEF_LIGHTNESS;
		//DEBUG_ON("cat not get lightness  %d\n", hdmi_data->lightness);
	}
	if (of_property_read_u32_array(of_node, "saturation", &hdmi_data->saturation , 1)) {
		hdmi_data->saturation = DEF_SATURATION;
		//DEBUG_ON("cat not get saturation  %d\n", hdmi_data->saturation);
	}
	if (of_property_read_u32_array(of_node, "contrast", &hdmi_data->contrast , 1)) {
		hdmi_data->contrast = DEF_CONTRAST;
		//DEBUG_ON("cat not get contrast  %d\n", hdmi_data->contrast);
	}
	
	hdmi.ip_data.txrx_cfg.drv_from_dts = 0;
	
	if (of_property_read_u32_array(of_node, "vid480p_tx1", &hdmi.ip_data.txrx_cfg.vid480p_tx1, 1)) {
		DEBUG_ERR("cat not get vid480p_tx1 \n");
		goto read_err;
	}
	if (of_property_read_u32_array(of_node, "vid480p_tx2", &hdmi.ip_data.txrx_cfg.vid480p_tx2, 1)) {
		DEBUG_ERR("cat not get vid480p_tx2 \n");
		goto read_err;
	}
	if (of_property_read_u32_array(of_node, "vid576p_tx1", &hdmi.ip_data.txrx_cfg.vid576p_tx1, 1)) {
		DEBUG_ERR("cat not get vid576p_tx1 \n");
		goto read_err;
	}	
	if (of_property_read_u32_array(of_node, "vid576p_tx2", &hdmi.ip_data.txrx_cfg.vid576p_tx2, 1)) {
		DEBUG_ERR("cat not get vid576p_tx2 \n");
		goto read_err;
	}	
	if (of_property_read_u32_array(of_node, "vid720p_tx1", &hdmi.ip_data.txrx_cfg.vid720p_tx1, 1)) {
		DEBUG_ERR("cat not get vid720p_tx1 \n");
		goto read_err;
	}	
	if (of_property_read_u32_array(of_node, "vid720p_tx2", &hdmi.ip_data.txrx_cfg.vid720p_tx2, 1)) {
		DEBUG_ERR("cat not get vid720p_tx2 \n");
		goto read_err;
	}		
	if (of_property_read_u32_array(of_node, "vid1080p_tx1", &hdmi.ip_data.txrx_cfg.vid1080p_tx1, 1)) {
		DEBUG_ERR("cat not get vid1080p_tx1 \n");
		goto read_err;
	}	
	if (of_property_read_u32_array(of_node, "vid1080p_tx2", &hdmi.ip_data.txrx_cfg.vid1080p_tx2, 1)) {
		DEBUG_ERR("cat not get vid1080p_tx2 \n");
		goto read_err;
	}	

	hdmi.ip_data.txrx_cfg.drv_from_dts = 1;
read_err:
	
	return 0;
}

static bool first_read_hdcpkey = true;

void hdcp_read_key_handle(void)
{	
		if(hdcp_read_key()==-EAGAIN){
			
				queue_delayed_work(hdmi.ip_data.hdcp.wq, &hdmi.ip_data.hdcp.hdcp_read_key_work,
					msecs_to_jiffies(2000));			
		}
				
}

/* HDMI HW IP initialisation */
static int platform_hdmihw_probe(struct platform_device *pdev)
{
	int r;
	struct resource * hdmihw_mem;
	const struct of_device_id *id = of_match_device(atm70xx_hdmi_of_match, &pdev->dev);	
	DEBUG_ON("Enter owldss_hdmihw_probe\n");
	if(id != NULL){
		struct ic_info * info = (struct ic_info *)id->data;	
		if(info != NULL){
			HDMI_DEBUG("hdmi info ic_type 0x%x \n",info->ic_type);
			if(info->ic_type == IC_TYPE_ATM7039C){
				hdmi.hdmihw_diff = &hdmi_atm7039c;
			}else if(info->ic_type == IC_TYPE_ATM7059TC){	
				hdmi.hdmihw_diff = &hdmi_atm7059tc;
			}
			else if(info->ic_type == IC_TYPE_ATM7059A){	
				hdmi.hdmihw_diff = &hdmi_atm7059a;
			}
		}else{
			HDMI_DEBUG("hdmi info is null\n");
		}
	}
	
	if(of_get_hdmi_data(pdev, &hdmi_data)){
		DEBUG_ERR("of_get_hdmi_data error\n");
		return -1;
	}

	hdmi.pdev = pdev;

	mutex_init(&hdmi.lock);
	mutex_init(&hdmi.ip_data.lock);

	/* Base address taken from platform */
	hdmihw_mem = platform_get_resource(hdmi.pdev, IORESOURCE_MEM, 0);
	if (!hdmihw_mem) {
		DEBUG_ERR("hdmihw_mem platform_get_resource ERROR\n");
		r = -EINVAL;
		goto err1;
	}
	
	hdmi.ip_data.base = devm_ioremap_resource(&pdev->dev, hdmihw_mem);
	if (IS_ERR(hdmi.ip_data.base)){	
		DEBUG_ERR("hdmi.ip_data.base error\n");
		return PTR_ERR(hdmi.ip_data.base);
	}	
	HDMI_DEBUG("base=%p\n", hdmi.ip_data.base);		
	r = hdmi_probe_pdata(pdev);
	if (r) {
		return r;
	}
		
	owl_hdmi_create_sysfs(&pdev->dev);	
	
    hdmi.ip_data.hdcp.wq = create_workqueue("atm705a-hdmi-hdcp");
	
	INIT_DELAYED_WORK(&hdmi.ip_data.hdcp.hdcp_work, hdcp_check_handle);
	
	INIT_DELAYED_WORK(&hdmi.ip_data.hdcp.hdcp_read_key_work, hdcp_read_key_handle);
	
	INIT_DELAYED_WORK(&hdmi_cable_check_work, hdmi_cable_check);
	
	INIT_WORK(&irq_work, do_hpd_irq);

	
	
		queue_delayed_work(hdmi.ip_data.hdcp.wq, &hdmi.ip_data.hdcp.hdcp_read_key_work,
					msecs_to_jiffies(50));
 
	r = switch_dev_register(&hdev);
	if (r)
		goto err1;
	r = switch_dev_register(&hdev_audio);
	if (r)
		goto err2;	
		
	ddc_init();
	
	hdmi.data.cable_check_onoff = 1;
	hdmi.data.send_uevent = 1;
	hdmi.edid.isHDMI = HDMI_HDMI;

	return 0;
	
err2:
	switch_dev_unregister(&hdev);
err1:
	return r;	
}

static int platform_hdmihw_remove(struct platform_device *pdev)
{
	DEBUG_ON("platform_hdmihw_remove \n");
	return 0;
}

#ifndef SUSPEND_IN_DSS
static s32 hdmi_diver_suspend(struct platform_device *pdev, pm_message_t mesg)
{
#ifndef CONFIG_HAS_EARLYSUSPEND
	DEBUG_ON("hdmi_diver_suspend \n");
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, 0);
#endif
	return 0;
}

static s32 hdmi_diver_resume(struct platform_device *pdev)
{
#ifndef CONFIG_HAS_EARLYSUSPEND
	DEBUG_ON("hdmi_diver_resume \n");

	hdmi.ip_data.ops->hdmi_reset(&hdmi.ip_data);
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, hdmi.data.hpd_en);
#endif
	return 0;
}
#endif 

#ifndef SUSPEND_IN_DSS
static void hdmi_early_suspend(struct early_suspend *h)
{
	DEBUG_ON("hdmi_early_suspend \n");
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, 0);
}
static void hdmi_late_resume(struct early_suspend *h)
{
	DEBUG_ON("hdmi_late_resume \n");

	hdmi.ip_data.ops->hdmi_reset(&hdmi.ip_data);
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, hdmi.data.hpd_en);
}

static struct early_suspend hdmi_early_suspend_desc = {
#ifndef SUSPEND_IN_DSS
	.suspend = hdmi_early_suspend,
	.resume  = hdmi_late_resume,
#endif
};
#endif

static struct platform_driver owldss_hdmihw_driver = {
	.probe		= platform_hdmihw_probe,
	.remove         = platform_hdmihw_remove,
#ifndef SUSPEND_IN_DSS
	.suspend = hdmi_diver_suspend,
	.resume = hdmi_diver_resume,
#endif
	.driver         = {
		.name   = "owl_hdmihw",
		.owner  = THIS_MODULE,
		.of_match_table = atm70xx_hdmi_of_match,
	},
};

int owl_hdmi_init_platform(void)
{
	int r;
	r = platform_driver_register(&owldss_hdmihw_driver);
	
#ifndef SUSPEND_IN_DSS
	register_early_suspend(&hdmi_early_suspend_desc);
#endif	

	if (r) {
		DEBUG_ERR("Failed to initialize hdmi platform driver\n");
		goto err_driver;
	}
	return 0;
err_driver:
	return r;
}

int owl_hdmi_uninit_platform(void)
{   
	platform_driver_unregister(&owldss_hdmihw_driver);
	
    return 0;
}

void hdmihw_write_reg(u32 val, const u16 idx)
{
	hdmi.ip_data.ops->write_reg(&hdmi.ip_data, idx, val);
}
EXPORT_SYMBOL(hdmihw_write_reg); 

int hdmihw_read_reg(const u16 idx)
{
	return hdmi.ip_data.ops->read_reg(&hdmi.ip_data, idx);
}
EXPORT_SYMBOL(hdmihw_read_reg); 