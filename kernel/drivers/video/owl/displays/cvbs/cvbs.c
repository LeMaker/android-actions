/*
 * linux/drivers/video/owl/dss/lcdc.c
 *
 * Copyright (C) 2009 Actions Corporation
 * Author: Xieshsh <xieshsh@artekmicro.com>
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

#define DSS_SUBSYS_NAME "CVBS"

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

#include <video/owldss.h>

#include "../../dss/dss_features.h"
#include "../../dss/dss.h"
#include "../../dss/de.h"
#include "cvbs_ip.h"
#include "cvbs_reg.h"
#include "cvbs.h"
 


int preline = 8;


struct switch_dev cdev = {
	.name = "cvbs",
};

void  *CMU_TVOUTPLL = 0;

struct cvbs_info cvbs;


#define CVBS_IN  	1
#define CVBS_OUT	2

static bool first_status = false;
static bool first_hpt = true;
atomic_t cvbs_connected_state = ATOMIC_INIT(0);
static DEFINE_MUTEX(cvbs_setting_mutex);

static struct work_struct cvbs_in_work;
static struct work_struct cvbs_out_work;
//static struct work_struct cvbs_check_work;

static struct delayed_work cvbs_check_work;

#define    OWL_TV_MOD_PAL                  8
#define    OWL_TV_MOD_NTSC                 9

#define NUM_CVBS                     1

struct data_fmt_param {
    const char *name;
    s32 data_fmt;
};

static struct data_fmt_param date_fmts[] = {
	{"PAL", OWL_TV_MOD_PAL},
	{"NTSC", OWL_TV_MOD_NTSC},	
};

static const struct owl_video_timings cvbs_display_modes[]=
{
	
	{ 720, 576, 13500, 64, 16, 39, 5, 64, 5,
	OWLDSS_SIG_ACTIVE_LOW, OWLDSS_SIG_ACTIVE_LOW,
	true, 1, 0, },
				
	{ 720, 480, 27000, 60, 16, 30, 9, 62, 6,
	OWLDSS_SIG_ACTIVE_LOW, OWLDSS_SIG_ACTIVE_LOW,
	true, 1, 0, },	
};

void io_cmap(void)
{
	CMU_TVOUTPLL = ioremap(0xb0160018, 4);	
}

inline void cvbs_write_reg(const u32 index, u32 val)
{
	writel(val, cvbs.base+index);
}

inline u32 cvbs_read_reg(const u32 index)
{
	return readl(cvbs.base + index);
}
void dump_reg(void)
{
	#define DUMPREG(name,r) printk("%s %08x\n",name,cvbs_read_reg(r))
	
     DUMPREG("TVOUT_EN  	value is ", TVOUT_EN);
     DUMPREG("TVOUT_OCR  	value is ", TVOUT_OCR);
     DUMPREG("TVOUT_STA  	value is ", TVOUT_STA);
     DUMPREG("TVOUT_CCR    value is ", TVOUT_CCR);
     DUMPREG("TVOUT_BCR  	value is ", TVOUT_BCR);
     DUMPREG("TVOUT_CSCR  	value is ", TVOUT_CSCR);
     DUMPREG("TVOUT_PRL  	value is ", TVOUT_PRL);
     
     DUMPREG("TVOUT_VFALD  	value is ", TVOUT_VFALD);
     DUMPREG("CVBS_MSR  	value is ", CVBS_MSR);    
     DUMPREG("CVBS_AL_SEPO  	value is ", CVBS_AL_SEPO);
     DUMPREG("CVBS_AL_SEPE  	value is ", CVBS_AL_SEPE);
     DUMPREG("CVBS_AD_SEP  value is ", CVBS_AD_SEP);
     DUMPREG("CVBS_HUECR  value is ", CVBS_HUECR);     
     DUMPREG("CVBS_SCPCR  value is ", CVBS_SCPCR);
     DUMPREG("CVBS_SCFCR  value is ", CVBS_SCFCR);
     DUMPREG("CVBS_CBACR  value is ", CVBS_CBACR);     
     DUMPREG("CVBS_SACR  value is ", CVBS_SACR);
     DUMPREG("TVOUT_DCR  value is ", TVOUT_DCR);
     DUMPREG("TVOUT_DDCR  value is ", TVOUT_DDCR);
     DUMPREG("TVOUT_DCORCTL  value is ", TVOUT_DCORCTL);
     DUMPREG("TVOUT_DRCR  value is ", TVOUT_DRCR);
	
}

static void ip_cvbs_reset(void)
{
	DEBUG_CVBS("[%s start]\n", __func__);
	DEBUG_CVBS("~~~~~module_reset \n");
	module_reset(MOD_ID_TVOUT);
}



/*****************tvout_reset***********************************/
static void tvout_reset(void)
{
	
	DEBUG_CVBS("[%s start]\n", __func__);
	module_reset(MOD_ID_TVOUT);
	DEBUG_CVBS("[%s start] TVOUT_DDCR=0x%x\n", __func__,TVOUT_DDCR);
	cvbs_write_reg(TVOUT_DDCR,0x110050);
	
	/* disable before registering irq handler */
	cvbs_write_reg(TVOUT_OCR,0x0);
	/* clear pendings before registering irq handler */
	cvbs_write_reg( TVOUT_STA,cvbs_read_reg(TVOUT_STA));

}



static void cvbs_irq_enable(int flag,bool enable)
{
	u32 val;
	if (flag==CVBS_IN)
	{
		val = cvbs_read_reg( TVOUT_OCR);
		val = REG_SET_VAL(val,enable, 12, 12);
		cvbs_write_reg(TVOUT_OCR,val);	
	}
	else if(flag==CVBS_OUT)
	{
		val = cvbs_read_reg(TVOUT_OCR);
		val = REG_SET_VAL(val,enable, 11, 11);
		cvbs_write_reg( TVOUT_OCR,val);
	}

}

static bool cvbs_pending(int flag)
{
	u32 val;
	if (flag==CVBS_IN)
	{
		val = cvbs_read_reg( TVOUT_STA);
		return (REG_GET_VAL(val,3,3) == 1);
	}
	if(flag==CVBS_OUT)
	{
		val = cvbs_read_reg(TVOUT_STA);
		return (REG_GET_VAL(val,7,7) == 1);
	}
	return;
}

static void cvbs_clear_pending(int flag)
{
	u32 val;
	if (flag==CVBS_IN)
	{
		val = cvbs_read_reg( TVOUT_STA);		
		val = REG_SET_VAL(val,1, 3, 3);
		cvbs_write_reg(val, TVOUT_STA);

			
	}
	else if(flag==CVBS_OUT)
	{
		val = cvbs_read_reg(TVOUT_STA); 
		val = REG_SET_VAL(val,1, 7, 7);
		cvbs_write_reg( TVOUT_STA,val);			
	}
}

static void enable_cvbs_internal(int flag)
{
	u32 val = 0;	

	val = cvbs_read_reg(TVOUT_OCR);	
	val = REG_SET_VAL(val,1, 10, 10);
	cvbs_write_reg(TVOUT_OCR,val);	
}

static void hdac_cvbs_enable()
{
	u32 val = 0;	

	val = cvbs_read_reg(TVOUT_OCR);	
	val = REG_SET_VAL(val,1, 3, 3);
	cvbs_write_reg(TVOUT_OCR,val);
}

static void auto_detect_bit(int flag)
{
	u32 val = 0;	
	if(flag==CVBS_IN)
	{
		val = cvbs_read_reg( TVOUT_OCR);	
		val = REG_SET_VAL(val,1, 8, 8);
		cvbs_write_reg( TVOUT_OCR,val);
	}
	else if(flag==CVBS_OUT)
	{
		val = cvbs_read_reg( TVOUT_OCR);	
		val = REG_SET_VAL(val,1, 9, 9);
		cvbs_write_reg( TVOUT_OCR,val);
	}
}

void enable_cvbs_output(void)
{
	
	cvbs_write_reg(TVOUT_EN,cvbs_read_reg(TVOUT_EN) | TVOUT_EN_CVBS_EN);
	cvbs_write_reg(TVOUT_OCR,(cvbs_read_reg(TVOUT_OCR) | TVOUT_OCR_DAC3 | TVOUT_OCR_INREN) &
		~TVOUT_OCR_DACOUT);

}

void disable_cvbs_output(void)
{	
	cvbs_write_reg(TVOUT_OCR,cvbs_read_reg(TVOUT_OCR) & ~(TVOUT_OCR_DAC3 | TVOUT_OCR_INREN));
	cvbs_write_reg(TVOUT_EN,cvbs_read_reg(TVOUT_EN) &  ~TVOUT_EN_CVBS_EN);
	

}



static irqreturn_t cvbs_irq_handler(int irq)
{
	DEBUG_CVBS("[%s start]\n", __func__);
	
	if (cvbs_pending(CVBS_IN))
	{
		DEBUG_CVBS("CVBS is in \n");
		cvbs_irq_enable(CVBS_IN,false);
		cvbs_irq_enable(CVBS_OUT,true);
						
		cvbs_clear_pending(CVBS_IN); 

		schedule_work(&cvbs_in_work);
		atomic_set(&cvbs_connected_state,1);			
	
		auto_detect_bit(CVBS_OUT);
	}
 if (cvbs_pending(CVBS_OUT))
	{
		DEBUG_CVBS("CVBS is out \n"); 
		cvbs_irq_enable(CVBS_OUT,false);
		cvbs_irq_enable(CVBS_IN,true);	
							 
		cvbs_clear_pending(CVBS_OUT);
		
		schedule_work(&cvbs_out_work);
		atomic_set(&cvbs_connected_state,0);
		
		auto_detect_bit(CVBS_IN);	
					
	}
	
	DEBUG_CVBS("[%s end]\n", __func__);	
	return IRQ_HANDLED;
}


static int  cvbs_uevent_state = -1;
static void set_cvbs_status(struct switch_dev *cdev, int state)
{	
	if(cvbs_uevent_state == state){
		return; 
	}
	
	switch_set_state(cdev, state);
	
#ifdef CONFIG_FB_MAP_TO_DE	
	if(cvbs.dssdev != NULL 
		&& cvbs.dssdev->driver != NULL 
		&& cvbs.dssdev->driver->hot_plug_nodify){
		cvbs.dssdev->driver->hot_plug_nodify(cvbs.dssdev,state);	
	}
#endif 

	cvbs_uevent_state = state;
}
static void do_cvbs_in(struct work_struct *work) 
{
	DEBUG_CVBS("[%s start]\n", __func__);
	if(cvbs.hot_plugin_enable)
	{
		set_cvbs_status(&cdev, 1);
	}		
}

static void do_cvbs_out(struct work_struct *work) 
{
		DEBUG_CVBS("[%s start]\n", __func__);	
		if(cvbs.hot_plugin_enable)
		{
			set_cvbs_status(&cdev, 0);	
		}
}

static void cvbs_check_status (struct work_struct *work) 
{
	if (first_hpt)
	{
		first_hpt=false;
		auto_detect_bit(CVBS_IN);
		cvbs_irq_enable(CVBS_IN,true);
	}
}



unsigned long  owl_tvoutpll1_set_rate(void)
{
	struct clk *cvbs_clk = NULL;
	unsigned int cvbs_rate;
	int ret;

	cvbs_clk = clk_get(NULL, CLKNAME_CVBSPLL);
	clk_prepare(cvbs_clk);
	clk_enable(cvbs_clk);
	cvbs_rate = 432000000;
	ret = clk_set_rate(cvbs_clk, cvbs_rate);
	if (ret < 0) {
		DEBUG_CVBS(KERN_ERR "cvbs clk set error!\n");
		return ret;
	}
	return ret;
}
  
  


void configure_pal(void)//pal(576i),pll1:432M,pll0:594M
{


	owl_tvoutpll1_set_rate();
	cvbs_write_reg(CVBS_MSR,CVBS_MSR_CVBS_PAL_D | CVBS_MSR_CVCKS);   //ÉèÖÃPAL_DºÍclk

	cvbs_write_reg(CVBS_AL_SEPO,(cvbs_read_reg(CVBS_AL_SEPO) & (~CVBS_AL_SEPO_ALEP_MASK)) |
			   CVBS_AL_SEPO_ALEP(0x136));
	cvbs_write_reg(CVBS_AL_SEPO,(cvbs_read_reg(CVBS_AL_SEPO) & (~CVBS_AL_SEPO_ALSP_MASK)) |
			   CVBS_AL_SEPO_ALSP(0x17));		   

	cvbs_write_reg(CVBS_AL_SEPE,(cvbs_read_reg(CVBS_AL_SEPE) & (~CVBS_AL_SEPE_ALEPEF_MASK)) |
			   CVBS_AL_SEPE_ALEPEF(0x26f));
	cvbs_write_reg(CVBS_AL_SEPE,(cvbs_read_reg(CVBS_AL_SEPE) & (~CVBS_AL_SEPE_ALSPEF_MASK)) |
			   CVBS_AL_SEPE_ALSPEF(0x150));

	cvbs_write_reg(CVBS_AD_SEP,(cvbs_read_reg(CVBS_AD_SEP) & (~CVBS_AD_SEP_ADEP_MASK)) |
			   CVBS_AD_SEP_ADEP(0x2cf));
	cvbs_write_reg(CVBS_AD_SEP,(cvbs_read_reg(CVBS_AD_SEP) & (~CVBS_AD_SEP_ADSP_MASK)) |
			   CVBS_AD_SEP_ADSP(0x0));

}



void configure_ntsc(void)//ntsc(480i),pll1:432M,pll0:594/1.001
{


	owl_tvoutpll1_set_rate();

	
	cvbs_write_reg(CVBS_MSR,CVBS_MSR_CVBS_NTSC_M | CVBS_MSR_CVCKS);
	
	cvbs_write_reg(CVBS_AL_SEPO,(cvbs_read_reg(CVBS_AL_SEPO) & (~CVBS_AL_SEPO_ALEP_MASK)) |
			    CVBS_AL_SEPO_ALEP(0xfe)); //0xfe  0x106
	cvbs_write_reg(CVBS_AL_SEPO,(cvbs_read_reg(CVBS_AL_SEPO) & (~CVBS_AL_SEPO_ALSP_MASK)) |
			   CVBS_AL_SEPO_ALSP(0x15));	

	cvbs_write_reg(CVBS_AL_SEPE,(cvbs_read_reg(CVBS_AL_SEPE) & (~CVBS_AL_SEPE_ALEPEF_MASK)) |
			   CVBS_AL_SEPE_ALEPEF(0x205)); //0x20b 0x20d 208
	cvbs_write_reg(CVBS_AL_SEPE,(cvbs_read_reg(CVBS_AL_SEPE) & (~CVBS_AL_SEPE_ALSPEF_MASK)) |
			   CVBS_AL_SEPE_ALSPEF(0x11c));

	cvbs_write_reg(CVBS_AD_SEP,(cvbs_read_reg(CVBS_AD_SEP) & (~CVBS_AD_SEP_ADEP_MASK)) |
			   CVBS_AD_SEP_ADEP(0x2cf));
	cvbs_write_reg(CVBS_AD_SEP,(cvbs_read_reg(CVBS_AD_SEP) & (~CVBS_AD_SEP_ADSP_MASK)) |
			   CVBS_AD_SEP_ADSP(0x0));
}


int configure_cvbs(int vid)
{

	switch (vid) {
	case OWL_TV_MOD_PAL:
		configure_pal();
		break;
		
	case OWL_TV_MOD_NTSC:
		configure_ntsc();
		break;
		
	default:
		return -EINVAL;
	}
	cvbs.current_vid=vid;
	return 0;
}

void cvbs_show_colorbar()
{
	DEBUG_CVBS("[%s start]\n", __func__);

	configure_cvbs(OWL_TV_MOD_PAL);
	
	/*enable color bar ,cvbs HDAC*/
	cvbs_write_reg(TVOUT_OCR,cvbs_read_reg(TVOUT_OCR) | TVOUT_OCR_DACOUT | TVOUT_OCR_DAC3 
		| TVOUT_OCR_INACEN | TVOUT_OCR_INREN);
	writel(readl(CMU_TVOUTPLL) | CMU_TVOUTPLL_PLL1EN |
		CMU_TVOUTPLL_TK0SS | CMU_TVOUTPLL_CVBS_PLL1FSS(0x4),CMU_TVOUTPLL);

	/*eable cvbs output*/
	cvbs_write_reg(TVOUT_EN,cvbs_read_reg(TVOUT_EN) | TVOUT_EN_CVBS_EN);

  DEBUG_CVBS("[%s finished]\n", __func__);
}

int cvbs_set_preline(int num_preline)
{
	//struct cvbs_info *c_inf = cvbs_get_data(pdev); 
	int temp;
	int hpn;
	preline = num_preline;
	hpn = (num_preline - 1) << 8;
	cvbs_write_reg(TVOUT_PRL,0xf<<8);
	temp = cvbs_read_reg(TVOUT_PRL);
	cvbs_write_reg( TVOUT_PRL,hpn|temp);
	return 1;
}



/**
 * @tv_mode: see enum TV_MODE_TYPE
 * This function sets register for CVBS(PAL/NTSC)
 */



void cvbs_display_set_vid(struct owl_dss_device *dssdev, int vid)
{
	struct owl_video_timings *timings;
	int id;
	DEBUG_CVBS("[%s start]\n", __func__);
	timings = &dssdev->timings;	
	cvbs.current_vid=vid;
	
	mutex_lock(&cvbs.lock);
	if (vid==OWL_TV_MOD_PAL)
	{
		id=0;
	}
	else if(vid==OWL_TV_MOD_NTSC)
	{
		id=1;
	}
	
	 	timings->x_res 			= 	cvbs_display_modes[id].x_res;
		timings->y_res 		 	= 	cvbs_display_modes[id].y_res;
		timings->pixel_clock 	= 	cvbs_display_modes[id].pixel_clock;
		timings->hsw 		 	= 	cvbs_display_modes[id].hsw;
		timings->hfp 			= 	cvbs_display_modes[id].hfp;
		timings->hbp 		 	= 	cvbs_display_modes[id].hbp;
		timings->vsw 			= 	cvbs_display_modes[id].vsw;
		timings->vfp 			= 	cvbs_display_modes[id].vfp;
		timings->vbp 			= 	cvbs_display_modes[id].vbp;	
	
	DEBUG_CVBS("[%s finished]\n", __func__);
	mutex_unlock(&cvbs.lock);
	
}

void cvbs_display_get_vid(struct owl_dss_device *dssdev, int *vid)
{
  	int i;
   mutex_lock(&cvbs.lock);
   
   	i=cvbs.current_vid;
   	
   	*vid = i;   	
   	
   	mutex_unlock(&cvbs.lock);
}

void cvbs_display_set_overscan(struct owl_dss_device *dssdev,u16 over_scan_width,u16 over_scan_height)
{	
	mutex_lock(&cvbs.lock);
	cvbs.overscan_width = over_scan_width;
	cvbs.overscan_height = over_scan_height;
	mutex_unlock(&cvbs.lock);
	
}
void cvbs_display_get_overscan(struct owl_dss_device *dssdev, u16 * over_scan_width,u16 * over_scan_height)
{
	mutex_lock(&cvbs.lock);
	*over_scan_width = cvbs.overscan_width;
	*over_scan_height = cvbs.overscan_height;
	mutex_unlock(&cvbs.lock);
}
static void cvbs_boot_inited(void)
{
   // int enable = cvbs_read_reg(TVOUT_EN);
    int msr    = cvbs_read_reg(CVBS_MSR);
       
    if(msr == 0x14)
    	{ //PAL
    			cvbs.current_vid=OWL_TV_MOD_PAL;           
       } else if (msr == 0x10)
       { //NTSC
        	cvbs.current_vid=OWL_TV_MOD_NTSC;        
       }      
}

static int  cvbs_hpd_state = -1;
void owldss_cvbs_display_enable_hpd(struct owl_dss_device *dssdev, bool enable)
{
	int val;
		if(cvbs_hpd_state == enable){
		return; 
	}
	
	mutex_lock(&cvbs.lock);
	if (enable)
	{	 

			val = cvbs_read_reg(TVOUT_OCR);	
			
			cvbs_write_reg( TVOUT_OCR,0x0);
			 
			cvbs_write_reg(TVOUT_OCR , TVOUT_OCR_PI_ADEN | TVOUT_OCR_PO_ADEN);
		
			mdelay(600);

			cvbs_write_reg(TVOUT_OCR,0x300);
			cvbs_irq_enable(CVBS_IN,true);				
	}else
		{	
			msleep(500);
			set_cvbs_status(&cdev, 0);
			cvbs_irq_enable(CVBS_IN,false);	
			cvbs_irq_enable(CVBS_OUT,false);			
		}
	mutex_unlock(&cvbs.lock);
	cvbs_hpd_state = enable;
}


int owldss_cvbs_display_enable(struct owl_dss_device *dssdev)
{

	struct owl_overlay_manager *mgr = dssdev->manager;

	struct owl_video_timings *timings;

	int r = 0,val,enable;
	
	timings = &dssdev->timings;	

	DEBUG_CVBS("ENTER cvbs_display_enable\n");
	if(cvbs_read_reg(TVOUT_EN))
		{
			dss_mgr_enable(mgr);
			return 0;
		}
	mutex_lock(&cvbs.lock);

    	if (mgr == NULL) {
		DEBUG_CVBS("failed to enable display: no manager\n");
		r = -ENODEV;

	}

	r = owl_dss_start_device(dssdev);
	
	if (r) {
		DEBUG_CVBS("failed to start device\n");
		goto err0;
	}
		
	
	if(atomic_read(&cvbs_connected_state) == 1)
	{
		configure_cvbs(cvbs.current_vid);
		msleep(500);
		enable_cvbs_output();
		DEBUG_CVBS("cvbs_boot_inited vid  =%d\n",cvbs.current_vid);
		
		r = dss_mgr_enable(mgr);
	
	if (r)
		{
		DEBUG_CVBS("failed to dss_mgr_enable device\n");
		}
	}
	mutex_unlock(&cvbs.lock);
		
	return 0;
	
err0:	
	return r;
}


void owldss_cvbs_display_disable(struct owl_dss_device *dssdev)
{
	
	struct owl_overlay_manager *mgr = dssdev->manager;

	DEBUG_CVBS("ENTER owldss_cvbs_display_disable\n");
	mutex_lock(&cvbs.lock);
	disable_cvbs_output();
		
	dss_mgr_disable(mgr);

	owl_dss_stop_device(dssdev);
	msleep(350);
	mutex_unlock(&cvbs.lock);	
}

int owldss_cvbs_resume(struct owl_dss_device *dssdev)
{
	if(cvbs.is_init){
		DEBUG_CVBS("owldss_cvbs_resume33 \n");
		tvout_reset();
		cvbs_clear_pending(CVBS_IN);
		cvbs_clear_pending(CVBS_OUT);
		auto_detect_bit(CVBS_IN);
		auto_detect_bit(CVBS_OUT);
		cvbs_irq_enable(CVBS_IN,true);	
		cvbs_irq_enable(CVBS_OUT,true);	
		
	}
	return 0;
}

int owldss_cvbs_suspend(struct owl_dss_device *dssdev)
{
	DEBUG_CVBS("owldss_cvbs_suspend \n");
	if(cvbs.is_init){
		cvbs_irq_enable(CVBS_IN,false);
		cvbs_irq_enable(CVBS_OUT,false);
		cancel_work_sync(&cvbs_in_work);
		cancel_work_sync(&cvbs_out_work);
		
	}
	return 0;
}


static struct of_device_id owl_cvbs_of_match[] = {
    { .compatible = "actions,atm7059a-cvbs",},
      { },
};

MODULE_DEVICE_TABLE(of, owl_cvbs_of_match);


static u32 string_to_data_fmt(const char *name)
{
	int i;
	DEBUG_CVBS("ARRAY_SIZE=%d\n",ARRAY_SIZE(date_fmts));
	for (i = 0; i < ARRAY_SIZE(date_fmts); i++) {
		if (!strcmp(date_fmts[i].name, name))
			return date_fmts[i].data_fmt;
	}

	return -1;
}

static int get_cvbs_data(struct platform_device *pdev)
{
	struct device_node *of_node;
	
	char const *default_mode;
	char const * over_scan;
	int index = 1;

	DEBUG_CVBS("%s\n", __func__);

	of_node = pdev->dev.of_node;
	
	if (of_property_read_string(of_node, "default_mode", &default_mode)) {
		default_mode = NULL;
			DEBUG_CVBS("get default_mode node error\n");
	}
	
	if (of_property_read_u32_array(of_node, "hotplugable", &cvbs.hot_plugin_enable , 1)) {
		cvbs.hot_plugin_enable = 1;
		DEBUG_CVBS("get hot_plugin_enable node error\n");
	}
	if (!of_property_read_string(of_node, "over_scan", &over_scan)) {		
		if(over_scan != NULL)
		{
			printk("over_scan \n %s",over_scan);
			sscanf(over_scan,"%d,%d",&cvbs.overscan_width,&cvbs.overscan_height);		
			printk("cvbs ->overscan_width %d  ,hdmi_data->overscan_height %d \n",cvbs.overscan_width,cvbs.overscan_height);
		}
		
	}
	
	 DEBUG_CVBS("get hot_plugin_enable node hotplugable =%d\n",cvbs.hot_plugin_enable);

	index = string_to_data_fmt(default_mode);
	if(index <= 0 )
	{
		index = 1;
	}
	cvbs.current_vid = index;
	
	return 0;
}

static int owl_cvbs_probe(struct platform_device *pdev)
{
	int r,val,enable;
	struct resource * cvbs_mem;
	const struct of_device_id 	*match;
	
	cvbs.pdev = pdev;
	io_cmap();	
	cvbs_mem = platform_get_resource(cvbs.pdev, IORESOURCE_MEM, 0);

	if (!cvbs_mem) {
		DEBUG_CVBS("cvbs_mem platform_get_resource ERROR\n");
		r = -EINVAL;
		goto err1;
	}
	
	cvbs.base =  ioremap(cvbs_mem->start, resource_size(cvbs_mem));

	if (!cvbs.base){	
		DEBUG_CVBS("cvbs.cfo.base error\n");
		r=  -ENOMEM;
		goto err_ioremap;
	}	
	DEBUG_CVBS("cvbs_mem->start  ~~~ %x  resource_size %x\n",cvbs_mem ->start, resource_size(cvbs_mem ));	
	DEBUG_CVBS("cvbs->base  ~~~ %p \n",cvbs.base);

	mutex_init(&cvbs.lock);
	
  	owl_cvbs_create_sysfs(&pdev->dev);

	
	DEBUG_CVBS("rest probe cvbs  ~~\n");
	
	
	cvbs_irq_enable(CVBS_IN,false);

	r = request_irq(OWL_IRQ_TVOUT, cvbs_irq_handler, 0, "cvbsdev", NULL);	
	if (r) {
		DEBUG_CVBS(" register irq failed!\n");
		return r;
	} else {
		DEBUG_CVBS(" register irq ON!\n");
	}		
	
	if(get_cvbs_data(pdev)){
		DEBUG_CVBS("get_cvbs_data error\n");
		return -1;
	}
	r = switch_dev_register(&cdev);
	if (r)
		goto err1;
	
	cvbs_boot_inited();
	
	DEBUG_CVBS("get vid is %d\n",cvbs.current_vid);
	
	enable = cvbs_read_reg(TVOUT_EN);
	
	if(!enable)
	{
		first_status = true;
		atomic_set(&cvbs_connected_state,0);
		cvbs_uevent_state=0;
		switch_set_state(&cdev, 0);
	}else{
		atomic_set(&cvbs_connected_state,1);
		cvbs_uevent_state=1;
		switch_set_state(&cdev, 1);
		
	}
	cvbs.wq = create_workqueue("atm705a-cvbs");
	
	
	INIT_WORK(&cvbs_in_work, do_cvbs_in);
	INIT_WORK(&cvbs_out_work, do_cvbs_out);
	
	INIT_DELAYED_WORK(&cvbs_check_work, cvbs_check_status);
	
	queue_delayed_work(cvbs.wq, &cvbs_check_work,
				msecs_to_jiffies(2000));
	
	DEBUG_CVBS(" owl_cvbs_probe is OK!\n");
	cvbs.is_init = true;
	return 0;
err_ioremap:
err1:
	return r;	
	
}

static void owl_cvbs_remove(struct platform_device *pdev)
{ 
	DEBUG_CVBS("%s start!\n", __func__);
	disable_cvbs_output();
	DEBUG_CVBS("%s finished\n", __func__);
}

static struct platform_driver owl_cvbs_driver = {
    .driver = {
        .name           = "owl_cvbs",
        .owner          = THIS_MODULE,
        .of_match_table = owl_cvbs_of_match,
    },
    .probe              = owl_cvbs_probe,
    .remove             = owl_cvbs_remove,
};

int owl_cvbs_init_platform(void)
{
    int ret = 0;
    ret = platform_driver_register(&owl_cvbs_driver);
    
    if (ret) {
        DSSERR("Failed to initialize dsi platform driver ret=%d\n",ret);
        return ret;
    }
    if(!cvbs.is_init){
    	return -1;
    }
    return 0;
}


