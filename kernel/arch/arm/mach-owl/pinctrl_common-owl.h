/*
 * Pinctrl definitions for Actions SOC
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */
 
#ifndef __PINCTRL_COMMON_OWL_H__ 
#define __PINCTRL_COMMON_OWL_H__

#include "pinctrl_data-owl.h"


#if 0
#define PINCTRL_DBG(format, ...) \
	printk(KERN_NOTICE "owl pinctrl: " format, ## __VA_ARGS__)
#else
#define PINCTRL_DBG(format, ...)
#endif

#define PINCTRL_ERR(format, ...) \
	printk(KERN_ERR "owl pinctrl: " format, ## __VA_ARGS__)


/**
 * struct owl_pinctrl_soc_info - Actions SOC pin controller per-SoC configuration
 * @gpio_ranges: An array of GPIO ranges for this SoC
 * @gpio_num_ranges: The number of GPIO ranges for this SoC
 * @pins:	An array describing all pins the pin controller affects.
 *		All pins which are also GPIOs must be listed first within the
 *		array, and be numbered identically to the GPIO controller's
 *		numbering.
 * @npins:	The number of entries in @pins.
 * @functions:	The functions supported on this SoC.
 * @nfunction:	The number of entries in @functions.
 * @groups:	An array describing all pin groups the pin SoC supports.
 * @ngroups:	The number of entries in @groups.
 */
struct owl_pinctrl_soc_info {
	struct device *dev;
	struct pinctrl_gpio_range *gpio_ranges;
	unsigned gpio_num_ranges;
	const struct owl_pinconf_pad_info *padinfo;
	const struct pinctrl_pin_desc *pins;
	unsigned npins;
	const struct owl_pinmux_func *functions;
	unsigned nfunctions;
	const struct owl_group *groups;
	unsigned ngroups;
	struct owl_gpio_pad_data *owl_gpio_pad_data;
};

int owl_pinctrl_common_probe(struct platform_device *pdev,
				struct owl_pinctrl_soc_info *info);
int owl_pinctrl_common_remove(struct platform_device *pdev);

#endif /* __PINCTRL_COMMON_OWL_H__ */
