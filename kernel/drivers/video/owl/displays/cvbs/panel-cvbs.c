/*
 * Generic EDP Panels support
 *
 * Copyright (C) 2010 Canonical Ltd.
 * Author: Bryan Wu <bryan.wu@canonical.com>
 *
 * LCD panel driver for Sharp LQ043T1DG01
 *
 * Copyright (C) 2009 Actions Inc
 * Author: Vaibhav Hiremath <hvaibhav@ti.com>
 *
 * LCD panel driver for Toppoly TDO35S
 *
 * Copyright (C) 2009 CompuLab, Ltd.
 * Author: Mike Rapoport <mike@compulab.co.il>
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: Hui Wang  <wanghui@actions-semi.com>
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
#include <mach/bootdev.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "../../dss/dss_features.h"
#include "cvbs.h"

static struct {
	struct mutex lock;
} cvbs_panel;


static void cvbs_panel_enable(struct owl_dss_device *dssdev)
{
	DEBUG_CVBS("cvbs_panel_enable\n");
	mutex_lock(&cvbs_panel.lock);
	
	owldss_cvbs_display_enable(dssdev);
	
	dssdev->state = OWL_DSS_DISPLAY_ACTIVE;
	mutex_unlock(&cvbs_panel.lock);

}

static void cvbs_panel_disable(struct owl_dss_device *dssdev)
{
	DEBUG_CVBS("cvbs_panel_disable\n");
	mutex_lock(&cvbs_panel.lock);
	
	if (dssdev->state == OWL_DSS_DISPLAY_ACTIVE) {
		
		owldss_cvbs_display_disable(dssdev);

		dssdev->state = OWL_DSS_DISPLAY_DISABLED;		
	}

	mutex_unlock(&cvbs_panel.lock);

}

static void cvbs_enable_hpd(struct owl_dss_device *dssdev,bool enable)
{
	DEBUG_CVBS("cvbs_enable_hpt\n");
	mutex_lock(&cvbs_panel.lock);
	
	owldss_cvbs_display_enable_hpd(dssdev, enable);

	mutex_unlock(&cvbs_panel.lock);
}
			
			
static void cvbs_panel_get_timings(struct owl_dss_device *dssdev,struct owl_video_timings *timings)
{	
	mutex_lock(&cvbs_panel.lock);
		
	*timings = dssdev->timings;
		
	mutex_unlock(&cvbs_panel.lock);
	
}

static void cvbs_panel_set_timings(struct owl_dss_device *dssdev,struct owl_video_timings *timings)
{		
	mutex_lock(&cvbs_panel.lock);
		
	dssdev->timings = *timings;
		
	mutex_unlock(&cvbs_panel.lock);

}

 void cvbs_set_vid(struct owl_dss_device *dssdev,
			int vid)
{
	DEBUG_CVBS("cvbs_set_vid\n");
	mutex_lock(&cvbs_panel.lock);
	
	cvbs_display_set_vid(dssdev,vid);
	
	mutex_unlock(&cvbs_panel.lock);
}

static void cvbs_get_vid(struct owl_dss_device *dssdev,
			int *vid)
{
	mutex_lock(&cvbs_panel.lock);
	
	cvbs_display_get_vid(dssdev, vid);
	
	mutex_unlock(&cvbs_panel.lock);
}
static void cvbs_get_over_scan(struct owl_dss_device *dssdev, 
			u16 * over_scan_width,u16 * over_scan_height)
{

	mutex_lock(&cvbs_panel.lock);
	
	cvbs_display_get_overscan(dssdev, over_scan_width,over_scan_height);
	
	mutex_unlock(&cvbs_panel.lock);
}

static void cvbs_set_over_scan(struct owl_dss_device *dssdev,
			u16 over_scan_width,u16 over_scan_height)
{

	mutex_lock(&cvbs_panel.lock);
	
	cvbs_display_set_overscan(dssdev, over_scan_width,over_scan_height);
	
	mutex_unlock(&cvbs_panel.lock);
}


static int cvbs_panel_suspend(struct owl_dss_device *dssdev)
{
	

	mutex_lock(&cvbs_panel.lock);
	
	owldss_cvbs_suspend(dssdev);
	
	mutex_unlock(&cvbs_panel.lock);
	return 0;
}

static int cvbs_panel_resume(struct owl_dss_device *dssdev)
{

	mutex_lock(&cvbs_panel.lock);
	
	owldss_cvbs_resume(dssdev);
	
	mutex_unlock(&cvbs_panel.lock);
	
	return 0;
}

static int cvbs_panel_probe(struct owl_dss_device *dssdev)
{

	
	cvbs.dssdev = dssdev;
	
	cvbs_display_set_vid(dssdev, cvbs.current_vid);

	DEBUG_CVBS("cvbs_panel_probe\n");
   
	return 0;
}

static void __exit cvbs_panel_remove(struct owl_dss_device *dssdev)
{
	struct panel_drv_data *drv_data = dev_get_drvdata(&dssdev->dev);

	kfree(drv_data);

	dev_set_drvdata(&dssdev->dev, NULL);
}



static struct owl_dss_driver cvbs_driver = {
	.probe		= cvbs_panel_probe,
	.remove		= cvbs_panel_remove,
	.suspend		  = cvbs_panel_suspend,
	.resume 		  = cvbs_panel_resume,
	.enable			  = cvbs_panel_enable,
	.disable		  = cvbs_panel_disable,
	.set_vid   	 	  = cvbs_set_vid,
	.get_vid    	  = cvbs_get_vid,
	.enable_hpd		  = cvbs_enable_hpd,
	.get_timings	= cvbs_panel_get_timings,
	.set_timings	= cvbs_panel_set_timings,
	.get_over_scan = cvbs_get_over_scan,
	.set_over_scan = cvbs_set_over_scan,
	.driver         = {
		.name   = "cvbs_panel",
		.owner  = THIS_MODULE,
	},
};

static int __init cvbs_panel_drv_init(void)
{
	DEBUG_CVBS("generic_cvbs_panel_drv_init \n");
	int r = -1; 
	mutex_init(&cvbs_panel.lock);
	
	r = owl_cvbs_init_platform();
	
	if(r){
		printk("generic_cvbs_panel_drv_init r %d \n",r);
		return r;
	}	 
	return owl_dss_register_driver(&cvbs_driver);
	
}

static void __exit cvbs_panel_drv_exit(void)
{
	owl_dss_unregister_driver(&cvbs_driver);

}

module_init(cvbs_panel_drv_init);
module_exit(cvbs_panel_drv_exit);
MODULE_LICENSE("GPL");
