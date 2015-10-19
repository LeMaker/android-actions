/*
 * gl520x_mmc.c - SD/MMC host controller driver
 *
 * Copyright (C) 2012, Actions Semiconductor Co. LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/cpufreq.h>
#include <linux/genhd.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/clk.h>

#include "gl520x_wifi_test.h"

static int __init acts_mmc_init(void)
{
	int ret;

	ret = acts_wifi_init();
	if (unlikely(ret < 0)) {
		pr_err("SDIO: Failed to register the power control driver.\n");
		return -1;
	}

	return 0;
}

static void __exit acts_mmc_exit(void)
{
	acts_wifi_cleanup();
}

module_init(acts_mmc_init);
module_exit(acts_mmc_exit);

MODULE_AUTHOR("Actions");
MODULE_DESCRIPTION("MMC/SD host controller driver");
MODULE_LICENSE("GPL");
