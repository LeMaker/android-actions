/*
 * arch/arm/mach-owl/pinctrl-owl.c
 *
 * Pinctrl driver based on Actions SOC pinctrl
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>

#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinconf-generic.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/gpio.h>
#include "pinctrl_common-owl.h"
#include "pinctrl_data-owl.h"

#define DRIVER_NAME "pinctrl-owl"

static struct pinctrl_gpio_range owl_gpio_ranges[] = {
	{
		.name = "owl-gpio",
		.id = 0,
		.base = 0,
		.pin_base = 0,
		.npins = NR_OWL_GPIO,
	},
};

static struct owl_pinctrl_soc_info atm7059_pinctrl_info = {
	.gpio_ranges = owl_gpio_ranges,
	.gpio_num_ranges = ARRAY_SIZE(owl_gpio_ranges),
	.padinfo = atm7059_pad_tab,
	.pins = (const struct pinctrl_pin_desc *)atm7059_pads,
//	.npins = atm7059_num_pads,
	.functions = atm7059_functions,
//	.nfunctions = atm7059_num_functions,
	.groups = atm7059_groups,
//	.ngroups = atm7059_num_groups,
	.owl_gpio_pad_data = &atm7059_gpio_pad_data,
};

static struct of_device_id owl_pinctrl_of_match[] = {
	{ .compatible = "actions,atm7059a-pinctrl", .data = &atm7059_pinctrl_info},
	{ },
};

static int owl_pinctrl_probe(struct platform_device *pdev)
{
	struct owl_pinctrl_soc_info *owl_pinctrl_info;

	const struct of_device_id *id = of_match_device(owl_pinctrl_of_match, &pdev->dev);
	if(id == NULL)
		return -EINVAL;

	owl_pinctrl_info = (struct owl_pinctrl_soc_info *)id->data;
	if(owl_pinctrl_info == &atm7059_pinctrl_info)
	{
		owl_pinctrl_info->npins = atm7059_num_pads;
		owl_pinctrl_info->nfunctions = atm7059_num_functions;
		owl_pinctrl_info->ngroups = atm7059_num_groups;
	}
	else
		return -EINVAL;

	return owl_pinctrl_common_probe(pdev, owl_pinctrl_info);
}


static struct platform_driver owl_pinctrl_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = owl_pinctrl_of_match,
	},
	.probe = owl_pinctrl_probe,
	.remove = owl_pinctrl_common_remove,
};

static int __init owl_pinctrl_init(void)
{
	return platform_driver_register(&owl_pinctrl_driver);
}
arch_initcall(owl_pinctrl_init);

static void __exit owl_pinctrl_exit(void)
{
	platform_driver_unregister(&owl_pinctrl_driver);
}
module_exit(owl_pinctrl_exit);

MODULE_AUTHOR("Actions Semi Inc.");
MODULE_DESCRIPTION("Pin control driver for Actions SOC");
MODULE_LICENSE("GPL");
