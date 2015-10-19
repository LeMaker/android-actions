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

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "../owldss.h"

struct panel_config {
	struct owl_video_timings timings;

	//enum owl_panel_config config;

	int power_on_delay;
	
	int power_off_delay;

	/*
	 * Used to match device to panel configuration
	 * when use generic panel driver
	 */
	const char *name;

	int (*platform_enable)(struct owl_dss_device *dssdev);
	
	void (*platform_disable)(struct owl_dss_device *dssdev);
};

/* Panel configurations */
static struct panel_config generic_edp_panels[] = {
	/* fpga rgb lcd  */
	{
		{
			.x_res		= 1280,
			.y_res		= 800,

			.pixel_clock	= 9000,

			.hsw		= 48,
			.hfp		= 23,
			.hbp		= 64,

			.vsw		= 3,
			.vfp		= 15,
			.vbp		= 12,
		},

		//.config			= OWL_DSS_LCD_TFT,
		.power_on_delay		= 50,
		.power_off_delay	= 100,
		.name			= "sharp_lq",
	},	
};

struct panel_drv_data {

	struct owl_dss_device *dssdev;

	struct panel_config *panel_config;
};


static int generic_edp_panel_power_on(struct owl_dss_device *dssdev)
{
	int r;
	struct panel_drv_data *drv_data = dev_get_drvdata(&dssdev->dev);
	struct panel_config *panel_config = drv_data->panel_config;

	if (dssdev->state == OWL_DSS_DISPLAY_ACTIVE)
		return 0;

	r = owl_edp_display_enable(dssdev);
	
	if (r)
		goto err0;

	/* wait couple of vsyncs until enabling the LCD */
	if (panel_config->power_on_delay)
		msleep(panel_config->power_on_delay);

	if (panel_config->platform_enable) {
		r = panel_config->platform_enable(dssdev);
		if (r)
			goto err1;
	}

	return 0;
err1:
	owl_edp_display_disable(dssdev);
err0:
	return r;
}

static void generic_edp_panel_power_off(struct owl_dss_device *dssdev)
{

	struct panel_drv_data *drv_data = dev_get_drvdata(&dssdev->dev);
	struct panel_config *panel_config = drv_data->panel_config;

	if (dssdev->state != OWL_DSS_DISPLAY_ACTIVE)
		return;

	if (panel_config->platform_disable)
		panel_config->platform_disable(dssdev);

	/* wait couple of vsyncs after disabling the LCD */
	if (panel_config->power_off_delay)
		msleep(panel_config->power_off_delay);

	owl_edp_display_disable(dssdev);
}

static int generic_edp_panel_probe(struct owl_dss_device *dssdev)
{

	struct panel_config *panel_config = NULL;
	struct panel_drv_data *drv_data = NULL;

	dev_dbg(&dssdev->dev, "probe\n");

    panel_config = &generic_edp_panels[0];
    
	if (!panel_config)
		return -EINVAL;

	dssdev->timings = panel_config->timings;

	drv_data = kzalloc(sizeof(*drv_data), GFP_KERNEL);
	if (!drv_data)
		return -ENOMEM;

	drv_data->dssdev = dssdev;
	drv_data->panel_config = panel_config;

	dev_set_drvdata(&dssdev->dev, drv_data);

	return 0;
}

static void __exit generic_edp_panel_remove(struct owl_dss_device *dssdev)
{
	struct panel_drv_data *drv_data = dev_get_drvdata(&dssdev->dev);

	dev_dbg(&dssdev->dev, "remove\n");

	kfree(drv_data);

	dev_set_drvdata(&dssdev->dev, NULL);
}

static int generic_edp_panel_enable(struct owl_dss_device *dssdev)
{
	int r = 0;

	r = generic_edp_panel_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OWL_DSS_DISPLAY_ACTIVE;

	return 0;
}

static void generic_edp_panel_disable(struct owl_dss_device *dssdev)
{
	generic_edp_panel_power_off(dssdev);

	dssdev->state = OWL_DSS_DISPLAY_DISABLED;
}

static void generic_edp_panel_get_timings(struct owl_dss_device *dssdev, struct owl_video_timings *timings)
{
	*timings = dssdev->timings;
}

static struct owl_dss_driver edp_driver = {
	.probe		= generic_edp_panel_probe,
	.remove		= __exit_p(generic_edp_panel_remove),

	.enable		= generic_edp_panel_enable,
	.disable	= generic_edp_panel_disable,
	.get_timings	= generic_edp_panel_get_timings,

	.driver         = {
		.name   = "generic_edp_panel",
		.owner  = THIS_MODULE,
	},
};

static int __init generic_edp_panel_drv_init(void)
{
	owl_edp_init_platform();
	return owl_dss_register_driver(&edp_driver);
}

static void __exit generic_edp_panel_drv_exit(void)
{
	owl_dss_unregister_driver(&edp_driver);
	owl_edp_uninit_platform();
}

module_init(generic_edp_panel_drv_init);
module_exit(generic_edp_panel_drv_exit);
MODULE_LICENSE("GPL");
