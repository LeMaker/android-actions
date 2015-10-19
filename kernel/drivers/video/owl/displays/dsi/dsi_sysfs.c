/*
 * dsi_sysfs.c
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
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/poll.h>


#include "../../dss/dss_features.h"
#include "../../dss/dss.h"

#include "dsihw.h"
#include "dsi.h"

static ssize_t show_dsi_dump(struct device *dev, struct device_attribute *attr, char *buf)
{
	dsihw_fs_dump_regs(dev);
	return 0;
}

static ssize_t test_dsi_fs(struct device *dev, struct device_attribute *attr, char *buf)
{
	test_fs_dsi(dev);
	return 0;
}

static ssize_t test_fs_dsi_cmd(struct device *dev, struct device_attribute *attr, char *buf)
{
	test_fs_longcmd(dev);
	return 0;
}

static struct device_attribute asoc_dsi_attrs[] = {
	__ATTR(reg_dump, S_IRUGO, show_dsi_dump, NULL),
	__ATTR(test_dsi, S_IRUGO, test_dsi_fs, NULL),
	__ATTR(longcmd_test, S_IRUGO, test_fs_dsi_cmd, NULL),
};

int owl_dsi_create_sysfs(struct device *dev)
{
	int r, t;

	for (t = 0; t < ARRAY_SIZE(asoc_dsi_attrs); t++) 
	{
		r = device_create_file(dev, &asoc_dsi_attrs[t]);

		if (r) {
			dev_err(dev, "failed to create sysfs file\n");
			return r;
		}
	}

	return 0;
}

void asoc_dsi_remove_sysfs(struct device *dev)
{
	int  t;
	
	for (t = 0; t < ARRAY_SIZE(asoc_dsi_attrs); t++) 
	{
		device_remove_file(dev, &asoc_dsi_attrs[t]);
	}
}

