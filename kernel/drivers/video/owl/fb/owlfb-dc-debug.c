/*
 * linux/drivers/video/owl/owlfb-main.c
 *
 * Copyright (C) 2014 Actions Corporation
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
 
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <video/owldss.h>
#include <video/owlfb.h>
#include <video/owldisplay.h>

#include "owlfb.h"

#define CREATE_TRACE_POINTS
#include <trace/events/owlfb-trace-events.h>

void trace_buffer_release(void * args)
{
	trace_owlfb_dc_out(args);
}

void trace_buffer_put_to_queue(void * args)
{
	trace_owlfb_dc_in(args);
}

void trace_buffer_put_to_dehw(void * args)
{
	trace_owlfb_dc_to_de(args);
}

void trace_vsync_point(int index, long long timestamp)
{
	trace_owlfb_vsync(index,timestamp);
}


static ssize_t show_dc_debug(struct device *dev,struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static ssize_t store_dc_debug(struct device *dev,struct device_attribute *attr, const char *buf, size_t count)
{
	int dc_debug_flags;
	int r;


	r = strtobool(buf, (bool *)&dc_debug_flags);
		
	if (r)
		return r;		
	
	r = count;

	return r;
}

static struct device_attribute owlfb_dc_attrs[] = {
	__ATTR(dc_debug, S_IRUGO | S_IWUSR, show_dc_debug, store_dc_debug),
};


int owlfb_dc_create_debug_sysfs(struct owlfb_device *fbdev)
{
	int i;
	int r;
	DBG("create sysfs for fbs\n");

	for (i = 0; i < fbdev->num_fbs; i++) {
		int t;
		for (t = 0; t < ARRAY_SIZE(owlfb_dc_attrs); t++) {
			r = device_create_file(fbdev->fbs[i]->dev,&owlfb_dc_attrs[t]);

			if (r) {
				dev_err(fbdev->dev, "failed to create sysfs "
						"file\n");
				return r;
			}
		}
	}

	return 0;
}
