/*
 * Generic LCDC Panels support
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

#include <video/owldss.h>

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
static struct panel_config generic_lcdc_panels[] = {
	/* rgb/lvds lcd  */
	{
		//.config			= OWL_DSS_LCD_TFT,
		.power_on_delay		= 0,
		.power_off_delay	= 0,
		.name			= "lcd",
	},	
};

struct panel_drv_data {

	struct owl_dss_device *dssdev;

	struct panel_config *panel_config;
};

extern enum owl_display_type get_current_display_type(void);

static int generic_lcdc_panel_power_on(struct owl_dss_device *dssdev)
{
	int r;
	struct panel_drv_data *drv_data = dev_get_drvdata(&dssdev->dev);
	struct panel_config *panel_config = drv_data->panel_config;

	if (dssdev->state == OWL_DSS_DISPLAY_ACTIVE)
		return 0;

	r = owl_lcdc_display_enable(dssdev);
	
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
	owl_lcdc_display_disable(dssdev);
err0:
	return r;
}

static void generic_lcdc_panel_power_off(struct owl_dss_device *dssdev)
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

	owl_lcdc_display_disable(dssdev);
}

static int generic_lcdc_panel_probe(struct owl_dss_device *dssdev)
{

	struct panel_config *panel_config = NULL;
	struct panel_drv_data *drv_data = NULL;

	dev_dbg(&dssdev->dev, "probe\n");

	panel_config = &generic_lcdc_panels[0];
    
	if (!panel_config)
		return -EINVAL;

	/* temp in here, TODO */
	owl_lcdc_select_video_timings(dssdev, 0, &panel_config->timings);

	dssdev->timings = panel_config->timings;

	drv_data = kzalloc(sizeof(*drv_data), GFP_KERNEL);
	if (!drv_data)
		return -ENOMEM;

	drv_data->dssdev = dssdev;
	drv_data->panel_config = panel_config;

	dev_set_drvdata(&dssdev->dev, drv_data);

	return 0;
}

static void __exit generic_lcdc_panel_remove(struct owl_dss_device *dssdev)
{
	struct panel_drv_data *drv_data = dev_get_drvdata(&dssdev->dev);

	dev_dbg(&dssdev->dev, "remove\n");

	kfree(drv_data);

	dev_set_drvdata(&dssdev->dev, NULL);
}

extern void owl_backlight_set_onoff(int onoff);

static int generic_lcdc_panel_enable(struct owl_dss_device *dssdev)
{
	int r = 0;

	r = generic_lcdc_panel_power_on(dssdev);
	if (r)
		return r;

	dssdev->state = OWL_DSS_DISPLAY_ACTIVE;

	/* at last enable backlight */
	owl_backlight_set_onoff(1);

	return 0;
}

static void generic_lcdc_panel_disable(struct owl_dss_device *dssdev)
{
    /* disable backlight first */
    owl_backlight_set_onoff(0);

	generic_lcdc_panel_power_off(dssdev);

	dssdev->state = OWL_DSS_DISPLAY_DISABLED;
}

static int generic_lcdc_panel_dump(struct owl_dss_device *dssdev)
{
	owl_lcdc_display_dump();

    return 0;
}

static void generic_lcdc_panel_get_timings(struct owl_dss_device *dssdev,
                                           struct owl_video_timings *timings)
{
	*timings = dssdev->timings;
}

static int generic_lcdc_panel_get_effect_parameter(struct owl_dss_device *dssdev,
                                              enum owl_plane_effect_parameter parameter_id)
{
	return owl_lcdc_get_effect_parameter(dssdev, parameter_id);
}

static void generic_lcdc_panel_set_effect_parameter(struct owl_dss_device *dssdev,
                                           enum owl_plane_effect_parameter parameter_id ,int value)
{
	owl_lcdc_set_effect_parameter(dssdev, parameter_id,value);
}
static struct owl_dss_driver lcdc_driver = {
	.probe		= generic_lcdc_panel_probe,
	.remove		= __exit_p(generic_lcdc_panel_remove),

	.enable		= generic_lcdc_panel_enable,
	.disable	= generic_lcdc_panel_disable,
	.get_timings	= generic_lcdc_panel_get_timings,
	.get_effect_parameter   = generic_lcdc_panel_get_effect_parameter,
	.set_effect_parameter   = generic_lcdc_panel_set_effect_parameter,
	.dump       = generic_lcdc_panel_dump,

	.driver         = {
		.name   = "generic_lcdc_panel",
		.owner  = THIS_MODULE,
	},
};

static int __init generic_lcdc_panel_drv_init(void)
{
	owl_lcdc_init_platform();
	if(get_current_display_type()==OWL_DISPLAY_TYPE_LCD){
		printk("current type lcd \n");
		owl_dss_register_driver(&lcdc_driver);
	}
	return 0;
}

static void __exit generic_lcdc_panel_drv_exit(void)
{
	owl_dss_unregister_driver(&lcdc_driver);
	owl_lcdc_uninit_platform();
}

module_init(generic_lcdc_panel_drv_init);
module_exit(generic_lcdc_panel_drv_exit);
MODULE_LICENSE("GPL");
