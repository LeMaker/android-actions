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
	int hotplugable;
	
	u16 overscan_width;
	u16 overscan_height;
	
	int lightness;
	int saturation;
	int contrast;
} ;

struct hdmi_property hdmi_data;
static struct work_struct irq_work;
static struct delayed_work hdmi_cable_check_work;

static int boot_hdmi_vid = -1;
static int user_config_vid = -1;

module_param(boot_hdmi_vid, int, 0644);

atomic_t hdmi_status = ATOMIC_INIT(0);

static bool is_probe_called = false;

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
	{"1280x720p-50", OWL_TV_MOD_720P_50HZ},
	{"1280x720p-60", OWL_TV_MOD_720P_60HZ},
	{"1920x1080p-50", OWL_TV_MOD_1080P_50HZ},
	{"1920x1080p-60", OWL_TV_MOD_1080P_60HZ},
	{"720x576p-60", OWL_TV_MOD_576P},
	{"720x480p-60", OWL_TV_MOD_480P},
	{"DVI", OWL_TV_MOD_DVI},
	{"PAL", OWL_TV_MOD_PAL},
	{"NTSC", OWL_TV_MOD_NTSC},
	{"4K30HZ", OWL_TV_MOD_4K_30HZ},
};

static struct hdmi_config cea_timings[] = {
	{
		{ 1920, 1080, 148500, 44, 88, 148, 5, 4, 36,
			OWLDSS_SIG_ACTIVE_HIGH, OWLDSS_SIG_ACTIVE_HIGH,
			false, 1, 0, },
		{ 16, HDMI_HDMI,OWL_TV_MOD_1080P_60HZ},
	},
	{
		{ 1920, 1080, 74250, 44, 528, 148, 5, 4, 36,
			OWLDSS_SIG_ACTIVE_HIGH, OWLDSS_SIG_ACTIVE_HIGH,
			false, 1, 0, },
		{ 31, HDMI_HDMI,OWL_TV_MOD_1080P_50HZ },
	},
	{
		{ 1280, 720, 74250, 40, 110, 220, 5, 5, 20,
			OWLDSS_SIG_ACTIVE_HIGH, OWLDSS_SIG_ACTIVE_HIGH,
			false, 1, 0, },
		{ 4, HDMI_HDMI,OWL_TV_MOD_720P_60HZ},
	},
	{
		{ 1280, 720, 74250, 40, 440, 220, 5, 5, 20,
			OWLDSS_SIG_ACTIVE_HIGH, OWLDSS_SIG_ACTIVE_HIGH,
			false, 1, 0, },
		{ 19, HDMI_HDMI,OWL_TV_MOD_720P_50HZ},
	},
	{
		{ 720, 576, 27000, 64, 12, 68, 5, 5, 39,
			OWLDSS_SIG_ACTIVE_LOW, OWLDSS_SIG_ACTIVE_LOW,
			false, 1, 0, },
		{ 17, HDMI_HDMI,OWL_TV_MOD_576P},
	},	
	{
		{ 720, 480, 27000, 62, 16, 60, 6, 9, 30,
			OWLDSS_SIG_ACTIVE_LOW, OWLDSS_SIG_ACTIVE_LOW,
			false, 7, 0, },
		{ 2, HDMI_HDMI,OWL_TV_MOD_480P},
	},		
	{
		{ 1280, 720, 74250, 40, 110, 220, 5, 5, 20,
			OWLDSS_SIG_ACTIVE_HIGH, OWLDSS_SIG_ACTIVE_HIGH,
			false, 1, 0, },
		{ 126, HDMI_DVI,OWL_TV_MOD_DVI},
	},
};

#if 0
static void hdmi_print_info(struct owl_dss_device *dssdev)
{
	HDMI_DEBUG("~~~~~~~~~hdmi_print_info\n");

	HDMI_DEBUG("x_res		= %d  \n",dssdev->timings.x_res);
	HDMI_DEBUG("y_res		= %d  \n",dssdev->timings.y_res);
	HDMI_DEBUG("pixel_clock	= %d  \n",dssdev->timings.pixel_clock);
	HDMI_DEBUG("hsw			= %d  \n",dssdev->timings.hsw);
	HDMI_DEBUG("hfp			= %d  \n",dssdev->timings.hfp);
	HDMI_DEBUG("hbp			= %d  \n",dssdev->timings.hbp);
	HDMI_DEBUG("vsw			= %d  \n",dssdev->timings.vsw);
	HDMI_DEBUG("vfp			= %d  \n",dssdev->timings.vfp);
	HDMI_DEBUG("vbp         = %d  \n",dssdev->timings.vbp);	
	HDMI_DEBUG("vsync_level = %d  \n",dssdev->timings.vsync_level);
	HDMI_DEBUG("hsync_level = %d  \n",dssdev->timings.hsync_level);
	HDMI_DEBUG("interlace	= %d  \n",dssdev->timings.interlace);
	HDMI_DEBUG("code		= %d  \n",hdmi.ip_data.cfg.cm.code);
	HDMI_DEBUG("mode		= %d  \n",hdmi.ip_data.cfg.cm.mode);
	HDMI_DEBUG("hdmi_src	= %d  \n",hdmi.ip_data.settings.hdmi_src);
}
#endif

static int hdmi_init_display(struct owl_dss_device *dssdev)
{

	HDMI_DEBUG("init_display\n");

	dss_init_hdmi_ip_ops(&hdmi.ip_data);

	return 0;
}

static struct owl_video_timings *hdmi_get_timings_by_vid(int vid)
{
     int i = 0;
     for (i = 0; i < ARRAY_SIZE(cea_timings); i++) {
		if (cea_timings[i].cm.vid == vid){
			return &cea_timings[i].timings;
		}
	 }
	 return NULL;
}

int owl_times_is_equal(const struct owl_video_timings *timings1,
		     const struct owl_video_timings *timings2)
{
	return (timings1->x_res         == timings2->x_res &&
		timings1->y_res         ==timings2->y_res &&
		timings1->pixel_clock     == timings2->pixel_clock &&
		timings1->hsw    == timings2->hsw &&
		timings1->hfp    == timings2->hfp &&
		timings1->hbp  == timings2->hbp &&
		timings1->vsw == timings2->vsw &&
		timings1->vfp == timings2->vfp &&
		timings1->vbp == timings2->vbp);
}

static int hdmi_get_vid_by_times(struct owl_video_timings * times)
{
     int i ;
     for (i = 0; i < ARRAY_SIZE(cea_timings); i++) {     	
		if (owl_times_is_equal(&cea_timings[i].timings,times)){
			return cea_timings[i].cm.vid ;
		}
	 }
	 return -1;
}

static bool hdmi_check_vid_available(int vid)
{
	return (((hdmi.edid.video_formats[0] >> vid) & 0x01)==1);
}
static int hdmi_get_code_by_vid(int vid)
{
     int i = 0;
     for (i = 0; i < ARRAY_SIZE(cea_timings); i++) {
		if (cea_timings[i].cm.vid == vid){
			return cea_timings[i].cm.code;
		}
	 }
	 return -1;
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
	struct owl_video_timings *timings = &dssdev->timings;
	struct owl_overlay_manager *mgr = dssdev->manager;
			
	if(hdmi.ip_data.cfg.cm.code == VID1280x720P_60_DVI){
		hdmi.ip_data.settings.hdmi_mode = HDMI_DVI;
	}else{
		hdmi.ip_data.settings.hdmi_mode = hdmi.edid.isHDMI;
	}
	
    if(is_hdmi_power_on())
    {
    	dss_mgr_enable(mgr);
    	return 0;
    }

	//hdmi.ip_data.ops->hdmi_devclken(&hdmi.ip_data, 1);
	hdmi.ip_data.ops->hdmi_reset(&hdmi.ip_data);
	
	hdmi.ip_data.ops->pmds_ldo_enable(&hdmi.ip_data,true);

	hdmi.ip_data.ops->hdmi_clk24Men(&hdmi.ip_data, 1);	
	
	hdmi.ip_data.ops->pll_enable(&hdmi.ip_data);	
	
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
		
	hdmi.ip_data.ops->pmds_ldo_enable(&hdmi.ip_data,false);
	
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

static int check_best_vid_from_edid(struct hdmi_edid *edid)
{
	int best_vid;
	int i = 0;	
	
	if(boot_hdmi_vid != -1)	{
		
		best_vid =  boot_hdmi_vid;
		boot_hdmi_vid = -1;
		
	}else if(hdmi.edid.read_ok){
		
		if(user_config_vid != -1
			&& hdmi_check_vid_available(user_config_vid)){
			best_vid = user_config_vid;
		}else{
			for (i = 0; i < ARRAY_SIZE(cea_timings); i++) {	
				if(hdmi_check_vid_available(cea_timings[i].cm.vid)){			
					best_vid = cea_timings[i].cm.vid;
					break;
				}
			}
		}		
	}else{
		if(user_config_vid != -1){
			best_vid = user_config_vid;
		}else{
			best_vid = OWL_TV_MOD_DVI;
		}
	}
		
	hdmi.ip_data.cfg.cm.vid = best_vid;
	
	return best_vid;
}
void hdmi_send_uevent(bool data)
{
	mutex_lock(&hdmi.ip_data.lock);
	/*HDMI_DEBUG("~~~hdmi_send_uevent %d  %d\n", data, atomic_read(&hdmi_status));*/
	if(data){
		if(atomic_read(&hdmi_status)==0){
			HDMI_DEBUG("parse_edid start\n");
			parse_edid(&hdmi.edid);
			check_best_vid_from_edid(&hdmi.edid);
			HDMI_DEBUG("parse_edid end\n");		
			if(hdmi.data.hpd_en){	
				switch_set_state(&hdev, 1);	
				atomic_set(&hdmi_status, 1);			
			}else{
				if(hdmi.dssdev != NULL 
					&& hdmi.dssdev->driver != NULL 
					&& hdmi.dssdev->driver->hot_plug_nodify){
					hdmi.dssdev->driver->hot_plug_nodify(hdmi.dssdev,data);	
				}
			}
		}
	}else{
		if((atomic_read(&hdmi_status)==1)&&hdmi.data.send_uevent){
			if(hdmi.data.hpd_en){
				switch_set_state(&hdev, 0);
				atomic_set(&hdmi_status, 0);					
			}else{
				if(hdmi.dssdev != NULL 
					&& hdmi.dssdev->driver != NULL 
					&& hdmi.dssdev->driver->hot_plug_nodify){

					hdmi.dssdev->driver->hot_plug_nodify(hdmi.dssdev,data);	
				
				}
			}		
		}	
	}		
	mutex_unlock(&hdmi.ip_data.lock);

}
static bool old_hotplug_state = false;
void hdmi_cable_check(struct work_struct *work)
{
	/*HDMI_DEBUG("hdmi_cable_check  ~~~~~~~~\n");*/	
	if(hdmi.data.cable_check_onoff){
		bool new_hotplug_state = hdmi.ip_data.ops->cable_check(&hdmi.ip_data);
		if(old_hotplug_state != new_hotplug_state){			
			hdmi_send_uevent(new_hotplug_state);
			
			if(!hdmi.data.hpd_en){
				if(hdmi.ip_data.settings.hdmi_mode == HDMI_HDMI){
					switch_set_state(&hdev_audio, new_hotplug_state ? 1: 2);
				}
			}
			
			old_hotplug_state = new_hotplug_state;
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

extern void hdmi_cec_irq_handler();

static irqreturn_t hdmi_irq_handle(int irq, void *data)
{
	//DEBUG_ON("[%s start]\n", __func__);
	u32 status = hdmi.ip_data.ops->get_irq_state(&hdmi.ip_data);
	if(status & HDMI_HPD_IRQ)
	{
		hdmi.data.hpd_pending  = hdmi.ip_data.ops->detect(&hdmi.ip_data);
		hdmi.data.cable_status = hdmi.ip_data.ops->cable_check(&hdmi.ip_data);
		hdmi.data.hdcp_ri      = check_ri_irq();	
		hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, 0);		
		schedule_work(&irq_work);
	}else{
		hdmi_cec_irq_handler();
	}	
	//DEBUG_ON("[%s end]\n", __func__);	
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
	
	int i = 0;
	for (i = 0; i < ARRAY_SIZE(cea_timings); i++) {
		if (owl_times_is_equal(&cea_timings[i].timings,timings)){
			return 0;
		}
	}
	return -1;
}
void owldss_hdmi_display_set_timing(struct owl_dss_device *dssdev,struct owl_video_timings *timings)
{
	struct owl_video_timings *dss_timings = &dssdev->timings;
	int vid = hdmi_get_vid_by_times(timings);
	
	timings = hdmi_get_timings_by_vid(vid);
	
	mutex_lock(&hdmi.lock);
	
	if (timings != NULL){
			memcpy(dss_timings,timings,sizeof(struct owl_video_timings));
			memcpy(&hdmi.ip_data.cfg.timings,timings,sizeof(struct owl_video_timings));			
	}else{
		printk("owldss_hdmi_display_set_timing NULL  \n");
	}	
	hdmi.ip_data.cfg.cm.vid = vid;
	hdmi.ip_data.cfg.cm.code = hdmi_get_code_by_vid(vid);
	hdmi.ip_data.cfg.cm.mode = hdmi.edid.isHDMI;	
	
	mutex_unlock(&hdmi.lock);
}

void owldss_hdmi_display_set_vid(struct owl_dss_device *dssdev,int vid)
{
	struct owl_video_timings *timings;
	struct owl_video_timings *dss_timings = &dssdev->timings;

	mutex_lock(&hdmi.lock);
	
	timings = hdmi_get_timings_by_vid(vid);
	printk("timings : xres %d yres %d size of %d %d \n",timings->x_res,timings->y_res,sizeof(struct owl_video_timings),sizeof(struct owl_video_timings*));
	if (timings != NULL){
			memcpy(dss_timings,timings,sizeof(struct owl_video_timings));
			memcpy(&hdmi.ip_data.cfg.timings,timings,sizeof(struct owl_video_timings));			
	}else{
		printk("hdmi_get_timings NULL  \n");
	}	
	hdmi.ip_data.cfg.cm.vid = vid;
	hdmi.ip_data.cfg.cm.code = hdmi_get_code_by_vid(vid);
	hdmi.ip_data.cfg.cm.mode = hdmi.edid.isHDMI;	
	DEBUG_ON("owldss_hdmi_display_set_vid vid %d ,code %d mode %d  \n",hdmi.ip_data.cfg.cm.vid,hdmi.ip_data.cfg.cm.code,hdmi.ip_data.cfg.cm.mode);
	mutex_unlock(&hdmi.lock);
}

void owldss_hdmi_display_get_vid(struct owl_dss_device *dssdev, int *vid)
{
    int i = 0;
    
	mutex_lock(&hdmi.lock);
	*vid = hdmi.ip_data.cfg.cm.vid;
	DEBUG_ON("owldss_hdmi_display_get_vid %d \n",*vid);
	mutex_unlock(&hdmi.lock);
}

void owldss_hdmi_display_enable_hotplug(struct owl_dss_device *dssdev, bool enable)
{
	if(!hdmi_data.hotplugable)
	{
		 return;
	}
	mutex_lock(&hdmi.lock);	
	HDMI_DEBUG("owldss_hdmi_display_enable_hotplug  enable %d\n", enable);
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, enable);
	if(enable){
		hdmi.data.hpd_en = 1;
		if(hdmi.ip_data.ops->cable_check(&hdmi.ip_data)){
			HDMI_DEBUG("owldss_hdmi_display_enable_hotplug  switch_set_state 1\n");
			hdmi_send_uevent(1);
		}
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
	int vid = 0;
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
	for (i = 0; i < ARRAY_SIZE(cea_timings); i++) {
		vid =  cea_timings[i].cm.vid;			
		if(hdmi_check_vid_available(vid)){			
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

void owldss_hdmi_display_set_overscan(struct owl_dss_device *dssdev,u16 over_scan_width,u16 over_scan_height)
{	
	mutex_lock(&hdmi.lock);
	hdmi_data.overscan_width = over_scan_width;
	hdmi_data.overscan_height = over_scan_height;
	mutex_unlock(&hdmi.lock);
	
}
void owldss_hdmi_display_get_overscan(struct owl_dss_device *dssdev, u16 * over_scan_width,u16 * over_scan_height)
{
	mutex_lock(&hdmi.lock);
	*over_scan_width = hdmi_data.overscan_width;
	*over_scan_height = hdmi_data.overscan_height;
	mutex_unlock(&hdmi.lock);
}

int owldss_hdmi_read_edid(struct owl_dss_device *dssdev, u8 * buffer , int len)
{
	
  int r = owldss_hdmi_display_get_cable_status(dssdev);
  if(!r){
         printk("owldss_hdmi_read_edid ~~~ error hdmi not connected \n");
         return r;
       }
    r = read_edid(buffer,len);
	if(r <= 0){
		printk("owldss_hdmi_read_edid ~~~ error\n");
	}
	return r;
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
	struct owl_video_timings *timings = &dssdev->timings;
	HDMI_DEBUG("owl_dss_device  hdmi  p->0x%p\n", dssdev);
	hdmi_display_set_dssdev(dssdev);
	hdmi_set_settings(&hdmi.ip_data);	
	HDMI_DEBUG("hpd_enable start\n");
	
	if(hdmi.hdmihw_diff->ic_type == IC_TYPE_ATM7059TC){		
		hdmi.ip_data.ops->hdmi_devclken(&hdmi.ip_data, 1);
		hdmi.ip_data.ops->hdmi_reset(&hdmi.ip_data);
	}
	
	owldss_hdmi_display_set_vid(dssdev,hdmi.ip_data.cfg.cm.vid);	
	
	
	hdmi.ip_data.ops->hpd_enable(&hdmi.ip_data, 0);
	
	r = request_irq(OWL_IRQ_HDMI, hdmi_irq_handle, 0, "hdmidev", NULL);	
	
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
	char const *over_scan;
	

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
	
	if (of_property_read_u32_array(of_node, "hotplugable", &hdmi_data->hotplugable , 1)) {
		hdmi_data->hotplugable = 1;		
		//DEBUG_ON("cat not get hotplugable  %d\n", hdmi_data->hotplugable);
	}
	hdmi.data.hpd_en = hdmi_data->hotplugable;			
	
	if (!of_property_read_string(of_node, "over_scan", &over_scan)) {		
		if(over_scan != NULL)
		{
			printk("over_scan \n %s",over_scan);
			sscanf(over_scan,"%d,%d",&hdmi_data->overscan_width,&hdmi_data->overscan_height);		
			printk("hdmi_data->overscan_width %d  ,hdmi_data->overscan_height %d \n",hdmi_data->overscan_width,hdmi_data->overscan_height);
		}
		
	}
	
	if (of_property_read_string(of_node, "default_resolution", &default_resulation)) {
		default_resulation = NULL;
		//DEBUG_ON("cat not get bit_invert  %d\n", hdmi_data->bit_invert);
	}
	
	user_config_vid = string_to_data_fmt(default_resulation);
	
	if(boot_hdmi_vid == -1)
	{
		boot_hdmi_vid = user_config_vid;
	}
		
	hdmi.ip_data.cfg.cm.vid = boot_hdmi_vid;
	
	if(hdmi.ip_data.cfg.cm.vid == -1){
		
		hdmi.ip_data.cfg.cm.vid = OWL_TV_MOD_720P_60HZ;
		
	}
	
	DEBUG_ON("of_get_hdmi_data ~~~~~~~~~~~~~ hdmi.ip_data.cfg.cm.code %d \n",hdmi.ip_data.cfg.cm.code);

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
		//DEBUG_ERR("cat not get vid480p_tx1 \n");
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
	is_probe_called = true;
	if(is_hdmi_power_on){
	 	old_hotplug_state = true;
	}
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
	if(!is_probe_called){
		r = -1;
	}
err_driver:
	return r;
}

int owl_hdmi_uninit_platform(void)
{   
	platform_driver_unregister(&owldss_hdmihw_driver);
	
    return 0;
}

static int __init boot_hdmi_vid_setup(char *str)
{
	
	sscanf(str,"%d",&boot_hdmi_vid);
	printk("-------- boot_hdmi_vid_setup %d --------\n",boot_hdmi_vid);
	return 1;
}
__setup("hdmi_vid=", boot_hdmi_vid_setup);
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
