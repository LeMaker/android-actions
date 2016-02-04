/*
 * linux/drivers/video/owl/dss/display.c
 *
 * Copyright (C) 2009 Actions Corporation
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

#define DSS_SUBSYS_NAME "DISPLAY"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/of.h>

#include <video/owldss.h>

#include "dss.h"
#include "dss_features.h"

static ssize_t display_enabled_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct owl_dss_device *dssdev = to_dss_device(dev);
	bool enabled = dssdev->state != OWL_DSS_DISPLAY_DISABLED;

	return snprintf(buf, PAGE_SIZE, "%d\n", enabled);
}

static ssize_t display_enabled_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct owl_dss_device *dssdev = to_dss_device(dev);
	int r;
	bool enabled;

	r = strtobool(buf, &enabled);
	if (r)
		return r;

	if (enabled != (dssdev->state != OWL_DSS_DISPLAY_DISABLED)) {
		if (enabled) {
			r = dssdev->driver->enable(dssdev);
			if (r)
				return r;
		} else {
			dssdev->driver->disable(dssdev);
		}
	}

	return size;
}

static ssize_t display_timings_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct owl_dss_device *dssdev = to_dss_device(dev);
	struct owl_video_timings t;

	if (!dssdev->driver->get_timings)
		return -ENOENT;

	dssdev->driver->get_timings(dssdev, &t);

	return snprintf(buf, PAGE_SIZE, "%u,%u/%u/%u/%u,%u/%u/%u/%u\n",
			t.pixel_clock,
			t.x_res, t.hfp, t.hbp, t.hsw,
			t.y_res, t.vfp, t.vbp, t.vsw);
}

static ssize_t display_timings_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct owl_dss_device *dssdev = to_dss_device(dev);
	struct owl_video_timings t;
	int r, found;

	if (!dssdev->driver->set_timings || !dssdev->driver->check_timings)
		return -ENOENT;

	found = 0;

	if (!found && sscanf(buf, "%u,%hu/%hu/%hu/%hu,%hu/%hu/%hu/%hu",
				&t.pixel_clock,
				&t.x_res, &t.hfp, &t.hbp, &t.hsw,
				&t.y_res, &t.vfp, &t.vbp, &t.vsw) != 9)
		return -EINVAL;

	r = dssdev->driver->check_timings(dssdev, &t);
	if (r)
		return r;

	dssdev->driver->set_timings(dssdev, &t);

	return size;
}

static DEVICE_ATTR(enabled, S_IRUGO|S_IWUSR,
		display_enabled_show, display_enabled_store);
static DEVICE_ATTR(timings, S_IRUGO|S_IWUSR,
		display_timings_show, display_timings_store);

static struct device_attribute *display_sysfs_attrs[] = {
	&dev_attr_enabled,
	&dev_attr_timings,
	NULL
};

void owl_default_get_resolution(struct owl_dss_device *dssdev,
			u16 *xres, u16 *yres)
{
	*xres = dssdev->timings.x_res;
	*yres = dssdev->timings.y_res;
}
EXPORT_SYMBOL(owl_default_get_resolution);

int owl_default_get_recommended_bpp(struct owl_dss_device *dssdev)
{
	switch (dssdev->type) {
	case OWL_DISPLAY_TYPE_LCD:
	case OWL_DISPLAY_TYPE_DSI:
	case OWL_DISPLAY_TYPE_EDP:
		if (dssdev->data_lines == 24)
			return 24;
		else
			return 16;
	case OWL_DISPLAY_TYPE_CVBS:
	case OWL_DISPLAY_TYPE_YPBPR:
	case OWL_DISPLAY_TYPE_HDMI:
		if (dssdev->data_lines == 24)
			return 24;
		else
			return 24;
	default:
		BUG();
	}
}
EXPORT_SYMBOL(owl_default_get_recommended_bpp);

/* Checks if replication logic should be used. Only use for active matrix,
 * when overlay is in RGB12U or RGB16 mode, and LCD interface is
 * 18bpp or 24bpp */
bool dss_use_replication(struct owl_dss_device *dssdev,
		enum owl_color_mode mode)
{
	int bpp;
	switch (dssdev->type) {
	case OWL_DISPLAY_TYPE_LCD:
	case OWL_DISPLAY_TYPE_DSI:
	case OWL_DISPLAY_TYPE_EDP:
		bpp = dssdev->data_lines;
		break;
	case OWL_DISPLAY_TYPE_HDMI:
	case OWL_DISPLAY_TYPE_CVBS:
	case OWL_DISPLAY_TYPE_YPBPR:
		bpp = dssdev->data_lines;
		break;
	default:
		BUG();
	}

	return bpp > 16;
}

void dss_init_device(struct platform_device *pdev,
		struct owl_dss_device *dssdev)
{
	struct device_attribute *attr;
	int i = 0;
	int r = 0;

	/* create device sysfs files */
	while ((attr = display_sysfs_attrs[i++]) != NULL) {
		r = device_create_file(&dssdev->dev, attr);
		if (r)
			DSSERR("failed to create sysfs file\n");
	}

	/* create display? sysfs links */
	r = sysfs_create_link(&pdev->dev.kobj, &dssdev->dev.kobj,
			dev_name(&dssdev->dev));
	if (r)
		DSSERR("failed to create sysfs display link\n");
}

void dss_uninit_device(struct platform_device *pdev,
		struct owl_dss_device *dssdev)
{
	struct device_attribute *attr;
	int i = 0;

	sysfs_remove_link(&pdev->dev.kobj, dev_name(&dssdev->dev));

	while ((attr = display_sysfs_attrs[i++]) != NULL)
		device_remove_file(&dssdev->dev, attr);

	if (dssdev->manager)
		dssdev->manager->unset_device(dssdev->manager);
}
atomic_t devices_suspended = ATOMIC_INIT(false);
int dss_suspend_all_devices(void)
{
    struct owl_dss_device *dssdev = NULL;
    
	atomic_set(&devices_suspended,true);
	
    for_each_dss_dev(dssdev) {
        if (!dssdev->driver)
            continue;
			
        if (dssdev->state == OWL_DSS_DISPLAY_ACTIVE) {
            if (dssdev->driver->disable) {
                dssdev->driver->disable(dssdev);
            }
            dssdev->activate_after_resume = true;
        } else {
            dssdev->activate_after_resume = false;
        }		
	
		if (dssdev->driver->suspend) {
			dssdev->driver->suspend(dssdev);
		}

    }

	return 0;
}

int dss_resume_all_devices(void)
{
    struct owl_dss_device *dssdev = NULL;
	
    for_each_dss_dev(dssdev) {
        if (!dssdev->driver)
            continue;
			
		if (dssdev->driver->resume) {
			dssdev->driver->resume(dssdev);
		}
		
        if (dssdev->activate_after_resume) {
            if (dssdev->driver->enable) {
                dssdev->driver->enable(dssdev);
            }
            dssdev->activate_after_resume = false;
        }
    }
	atomic_set(&devices_suspended,false);
	
    return 0;
}

static int dss_disable_device(struct device *dev, void *data)
{
	struct owl_dss_device *dssdev = to_dss_device(dev);

	if (dssdev->state != OWL_DSS_DISPLAY_DISABLED)
		dssdev->driver->disable(dssdev);

	return 0;
}

void dss_disable_all_devices(void)
{
	struct bus_type *bus = dss_get_bus();
	bus_for_each_dev(bus, NULL, NULL, dss_disable_device);
}

#define max_display_cnt 3
char lcd_node_compatible[max_display_cnt][32]={
	"actions,owl-lcd",
	"actions,owl-dsi",
	"actions,owl-edp",
};

enum owl_display_type get_current_display_type(void)
{
	int i;
	const char              *portname;
	struct device_node *np=NULL;
	enum owl_display_type display_type;
	
	display_type = OWL_DISPLAY_TYPE_NONE;
	
	for(i=0; i<max_display_cnt; i++) {
		np = of_find_compatible_node(NULL, NULL, lcd_node_compatible[i]);
		if (!np) {
			//DSSERR("failed to find %s node\n", lcd_node_compatible[i]);
			continue ;
		} else {
			if (!of_property_read_string(np, "port_type", &portname)) {
				printk(" %d portname = %s\n", i, portname);
				if(!strcmp("rgb", portname)){
					display_type = OWL_DISPLAY_TYPE_LCD;	
				}else if(!strcmp("lvds", portname)){
					display_type = OWL_DISPLAY_TYPE_LCD;	
				}else if(!strcmp("dsi", portname)){
					display_type = OWL_DISPLAY_TYPE_DSI;		
				}else if(!strcmp("edp", portname)){
					display_type = OWL_DISPLAY_TYPE_EDP;	
				}
				break;
			}		
		}
	}
	
	return display_type;
}
EXPORT_SYMBOL(get_current_display_type);

void owl_dss_get_device(struct owl_dss_device *dssdev)
{
	get_device(&dssdev->dev);
}
EXPORT_SYMBOL(owl_dss_get_device);

void owl_dss_put_device(struct owl_dss_device *dssdev)
{
	put_device(&dssdev->dev);
}
EXPORT_SYMBOL(owl_dss_put_device);

/* ref count of the found device is incremented. ref count
 * of from-device is decremented. */
struct owl_dss_device *owl_dss_get_next_device(struct owl_dss_device *from)
{
	struct device *dev;
	struct device *dev_start = NULL;
	struct owl_dss_device *dssdev = NULL;

	int match(struct device *dev, void *data)
	{
		return 1;
	}

	if (from)
		dev_start = &from->dev;
	dev = bus_find_device(dss_get_bus(), dev_start, NULL, match);
	if (dev)
		dssdev = to_dss_device(dev);
	if (from)
		put_device(&from->dev);

	return dssdev;
}
EXPORT_SYMBOL(owl_dss_get_next_device);

struct owl_dss_device *owl_dss_find_device(void *data,
		int (*match)(struct owl_dss_device *dssdev, void *data))
{
	struct owl_dss_device *dssdev = NULL;

	while ((dssdev = owl_dss_get_next_device(dssdev)) != NULL) {
		if (match(dssdev, data))
			return dssdev;
	}

	return NULL;
}
EXPORT_SYMBOL(owl_dss_find_device);


struct owl_dss_device *owl_dss_find_device_by_type(enum owl_display_type type)
{
	struct owl_dss_device *dssdev = NULL;

	while ((dssdev = owl_dss_get_next_device(dssdev)) != NULL) {
		if (dssdev->type == type)
			return dssdev;
	}
	return NULL;
}
EXPORT_SYMBOL(owl_dss_find_device_by_type);

int owl_dss_start_device(struct owl_dss_device *dssdev)
{
	if (!dssdev->driver) {
		DSSDBG("no driver\n");
		return -ENODEV;
	}

	if (!try_module_get(dssdev->dev.driver->owner)) {
		return -ENODEV;
	}

	return 0;
}
EXPORT_SYMBOL(owl_dss_start_device);

void owl_dss_stop_device(struct owl_dss_device *dssdev)
{
	module_put(dssdev->dev.driver->owner);
}
EXPORT_SYMBOL(owl_dss_stop_device);

bool owl_dss_is_devices_suspended(void)
{
	return atomic_read(&devices_suspended);
}
EXPORT_SYMBOL(owl_dss_is_devices_suspended);

