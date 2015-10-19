/*
 * atc260x-gpio.c  --  gpiolib support for ATC260X
 *
 * Copyright 2011 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>

#include <linux/mfd/atc260x/atc260x.h>


struct atc260x_gpio_mfp_map {
	u16 mfp_reg;
	u16 mask;
	u16 value;
};

struct atc260x_gpio_hwinfo {
	const struct atc260x_gpio_mfp_map *mfp_map_tbl;
	u16 reg_outen_tbl[4];
	u16 reg_inen_tbl[4];
	u16 reg_data_tbl[4];
	u16 gpio_nr_per_reg;
	u16 gpio_nr;
};

struct atc260x_gpio_dev {
	struct device *dev;
	struct atc260x_dev *atc260x;
	const struct atc260x_gpio_hwinfo *hwinfo;
	struct gpio_chip gpio_chip;
};


/* HWinfo for atc2603a ------------------------------------------------------ */

static const struct atc260x_gpio_mfp_map sc_atc2603a_gpio_mfp_map[] = {
	/* reg                mask        val */
	{ATC2603A_MFP_CTL1,   0x0c00,     0x0800},    /* GPIO0 */
	{ATC2603A_MFP_CTL1,   0x0c00,     0x0800},    /* GPIO1 */
	{ATC2603A_MFP_CTL1,   0x3000,     0x2000},    /* GPIO2 */
	{ATC2603A_MFP_CTL1,   0x3000,     0x2000},    /* GPIO3 */
	{ATC2603A_MFP_CTL1,   0xc000,     0x8000},    /* GPIO4 */
	{ATC2603A_MFP_CTL1,   0xc000,     0x8000},    /* GPIO5 */
	{ATC2603A_MFP_CTL0,   0x0010,     0x0010},    /* GPIO6 */
	{ATC2603A_MFP_CTL0,   0x0010,     0x0010},    /* GPIO7 */
	{ATC2603A_MFP_CTL0,   0x000c,     0x0004},    /* GPIO8 */
	{ATC2603A_MFP_CTL0,   0x0020,     0x0020},    /* GPIO9 */
	{ATC2603A_MFP_CTL0,   0x0020,     0x0020},    /* GPIO10 */
	{ATC2603A_MFP_CTL0,   0x0020,     0x0020},    /* GPIO11 */
	{ATC2603A_MFP_CTL1,   0x0001,     0x0001},    /* GPIO12 */
	{ATC2603A_MFP_CTL1,   0x0002,     0x0002},    /* GPIO13 */
	{ATC2603A_MFP_CTL1,   0x0004,     0x0004},    /* GPIO14 */
	{ATC2603A_MFP_CTL1,   0x0008,     0x0008},    /* GPIO15 */
	{ATC2603A_MFP_CTL1,   0x0010,     0x0010},    /* GPIO16 */
	{ATC2603A_MFP_CTL1,   0x00e0,          0},    /* GPIO17, reserved for TP */
	{ATC2603A_MFP_CTL1,   0x00e0,          0},    /* GPIO18, reserved for TP */
	{ATC2603A_MFP_CTL1,   0x00e0,          0},    /* GPIO19, reserved for TP */
	{ATC2603A_MFP_CTL1,   0x00e0,          0},    /* GPIO20, reserved for TP */
	{ATC2603A_MFP_CTL0,   0x0003,     0x0001},    /* GPIO21 */
	{ATC2603A_MFP_CTL0,   0x0003,     0x0001},    /* GPIO22 */
	{ATC2603A_MFP_CTL0,   0x0003,     0x0001},    /* GPIO23 */
	{ATC2603A_MFP_CTL0,   0x0003,     0x0001},    /* GPIO24 */
	{ATC2603A_MFP_CTL0,   0x0003,     0x0001},    /* GPIO25 */
	{ATC2603A_MFP_CTL0,   0x0003,     0x0001},    /* GPIO26 */
	{ATC2603A_MFP_CTL0,   0x0003,     0x0001},    /* GPIO27 */
	{ATC2603A_MFP_CTL0,   0x0040,     0x0040},    /* GPIO28 */
	{ATC2603A_MFP_CTL0,   0x0080,     0x0080},    /* GPIO29 */
	{                0,        0,          0},    /* GPIO30, dedicated GPIO */
	{                0,        0,          0},    /* GPIO31, dedicated GPIO */
};

static const struct atc260x_gpio_hwinfo sc_atc2603a_gpio_hwinfo = {
	.mfp_map_tbl = sc_atc2603a_gpio_mfp_map,
	.reg_outen_tbl = {ATC2603A_GPIO_OUTEN0, ATC2603A_GPIO_OUTEN1},
	.reg_inen_tbl  = {ATC2603A_GPIO_INEN0, ATC2603A_GPIO_INEN1},
	.reg_data_tbl  = {ATC2603A_GPIO_DAT0, ATC2603A_GPIO_DAT1},
	.gpio_nr_per_reg = 16,
	.gpio_nr = 32,
};

/* HWinfo for atc2603c ------------------------------------------------------ */

static const struct atc260x_gpio_mfp_map sc_atc2603c_gpio_mfp_map[] = {
	/* reg                mask        val */
	{ATC2603C_MFP_CTL,    0x0060,     0x0020},    /* GPIO0 */
	{ATC2603C_MFP_CTL,    0x0060,     0x0020},    /* GPIO1 */
	{ATC2603C_MFP_CTL,    0x0018,     0x0008},    /* GPIO2 */
	{ATC2603C_MFP_CTL,    0x0180,     0x0080},    /* GPIO3 */
	{ATC2603C_MFP_CTL,    0x0180,     0x0080},    /* GPIO4 */
	{ATC2603C_MFP_CTL,    0x0180,     0x0080},    /* GPIO5 */
	{               0,         0,          0},    /* GPIO6 */
	{               0,         0,          0},    /* GPIO7 */
};

static const struct atc260x_gpio_hwinfo sc_atc2603c_gpio_hwinfo = {
	.mfp_map_tbl = sc_atc2603c_gpio_mfp_map,
	.reg_outen_tbl = {ATC2603C_GPIO_OUTEN, },
	.reg_inen_tbl  = {ATC2603C_GPIO_INEN, },
	.reg_data_tbl  = {ATC2603C_GPIO_DAT, },
	.gpio_nr_per_reg = 8,
	.gpio_nr = 8,
};

/* HWinfo for atc2609a ------------------------------------------------------ */

static const struct atc260x_gpio_mfp_map sc_atc2609a_gpio_mfp_map[] = {
	/* reg                mask        val */
	{ATC2609A_MFP_CTL,    0x0060,     0x0020},    /* GPIO0 */
	{ATC2609A_MFP_CTL,    0x0060,     0x0020},    /* GPIO1 */
	{ATC2609A_MFP_CTL,    0x0018,     0x0008},    /* GPIO2 */
	{ATC2609A_MFP_CTL,    0x0180,     0x0080},    /* GPIO3 */
	{ATC2609A_MFP_CTL,    0x0180,     0x0080},    /* GPIO4 */
	{ATC2609A_MFP_CTL,    0x0180,     0x0080},    /* GPIO5 */
	{               0,         0,          0},    /* GPIO6 */
	{               0,         0,          0},    /* GPIO7 */
};

static const struct atc260x_gpio_hwinfo sc_atc2609a_gpio_hwinfo = {
	.mfp_map_tbl = sc_atc2609a_gpio_mfp_map,
	.reg_outen_tbl = {ATC2609A_GPIO_OUTEN, },
	.reg_inen_tbl  = {ATC2609A_GPIO_INEN, },
	.reg_data_tbl  = {ATC2609A_GPIO_DAT, },
	.gpio_nr_per_reg = 8,
	.gpio_nr = 8,
};


/* code --------------------------------------------------------------------- */

static inline struct atc260x_gpio_dev *_get_atc260x_gpio_dev(struct gpio_chip *chip)
{
	return dev_get_drvdata(chip->dev);
}

static int _atc260x_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	struct atc260x_gpio_dev *atc260x_gpio;
	struct atc260x_gpio_hwinfo const *hwinfo;
	uint mfp_reg, mfp_reg_msk, mfp_reg_val;
	int ret;

	atc260x_gpio = _get_atc260x_gpio_dev(chip);
	hwinfo = atc260x_gpio->hwinfo;

	if (offset >= hwinfo->gpio_nr)
		return -EINVAL;

	if (hwinfo->mfp_map_tbl == NULL) { /* need to touch MFP ? */
		mfp_reg_msk = (hwinfo->mfp_map_tbl[offset]).mask;
		if (mfp_reg_msk != 0) { /* need to touch MFP ? */
			mfp_reg = (hwinfo->mfp_map_tbl[offset]).mfp_reg;
			mfp_reg_val = (hwinfo->mfp_map_tbl[offset]).value;

			/* once GPIO MFP set, no way (no need) to revert */
			ret = atc260x_reg_setbits(atc260x_gpio->atc260x, mfp_reg, mfp_reg_msk, mfp_reg_val);
			if (ret)
				return ret;
		}
	}

	dev_info(atc260x_gpio->dev, "requested GPIO #%u\n", offset);
	return 0;
}

static int _atc260x_gpio_get_direction(struct gpio_chip *chip, unsigned offset)
{
	struct atc260x_gpio_dev *atc260x_gpio;
	struct atc260x_gpio_hwinfo const *hwinfo;
	uint reg_bank, reg_msk;
	int ret;

	atc260x_gpio = _get_atc260x_gpio_dev(chip);
	hwinfo = atc260x_gpio->hwinfo;

	if (offset >= hwinfo->gpio_nr)
		return -EINVAL;

	reg_bank = offset / hwinfo->gpio_nr_per_reg;
	reg_msk = 1U << (offset % hwinfo->gpio_nr_per_reg);

	ret = atc260x_reg_read(atc260x_gpio->atc260x, hwinfo->reg_outen_tbl[reg_bank]);
	if (ret < 0)
		return ret;
	if (ret & reg_msk)
		return GPIOF_DIR_OUT;

	ret = atc260x_reg_read(atc260x_gpio->atc260x, hwinfo->reg_inen_tbl[reg_bank]);
	if (ret < 0)
		return ret;
	if (ret & reg_msk)
		return GPIOF_DIR_IN;

	return -ENXIO;
}

static int _atc260x_gpio_direction_in(struct gpio_chip *chip, unsigned offset)
{
	struct atc260x_gpio_dev *atc260x_gpio;
	struct atc260x_gpio_hwinfo const *hwinfo;
	uint reg_bank, reg_msk;
	int ret;

	atc260x_gpio = _get_atc260x_gpio_dev(chip);
	hwinfo = atc260x_gpio->hwinfo;

	if (offset >= hwinfo->gpio_nr)
		return -EINVAL;

	reg_bank = offset / hwinfo->gpio_nr_per_reg;
	reg_msk = 1U << (offset % hwinfo->gpio_nr_per_reg);

	ret = atc260x_reg_setbits(atc260x_gpio->atc260x,
			hwinfo->reg_inen_tbl[reg_bank], reg_msk, reg_msk);
	if (ret == 0) {
		ret = atc260x_reg_setbits(atc260x_gpio->atc260x,
				hwinfo->reg_outen_tbl[reg_bank], reg_msk, 0);
	}
	return ret;
}

static int _atc260x_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	struct atc260x_gpio_dev *atc260x_gpio;
	struct atc260x_gpio_hwinfo const *hwinfo;
	uint reg_bank, reg_msk;
	int ret;

	atc260x_gpio = _get_atc260x_gpio_dev(chip);
	hwinfo = atc260x_gpio->hwinfo;

	if (offset >= hwinfo->gpio_nr)
		return -EINVAL;

	reg_bank = offset / hwinfo->gpio_nr_per_reg;
	reg_msk = 1U << (offset % hwinfo->gpio_nr_per_reg);

	ret = atc260x_reg_read(atc260x_gpio->atc260x, hwinfo->reg_data_tbl[reg_bank]);
	if (ret < 0)
		return ret;

	return ((ret & reg_msk) != 0);
}

static int _atc260x_gpio_direction_out(struct gpio_chip *chip, unsigned offset, int value)
{
	struct atc260x_gpio_dev *atc260x_gpio;
	struct atc260x_gpio_hwinfo const *hwinfo;
	uint reg_bank, reg_msk;
	int ret;

	atc260x_gpio = _get_atc260x_gpio_dev(chip);
	hwinfo = atc260x_gpio->hwinfo;

	if (offset >= hwinfo->gpio_nr)
		return -EINVAL;

	reg_bank = offset / hwinfo->gpio_nr_per_reg;
	reg_msk = 1U << (offset % hwinfo->gpio_nr_per_reg);

	ret = atc260x_reg_setbits(atc260x_gpio->atc260x,
			hwinfo->reg_data_tbl[reg_bank],
			reg_msk,
			((value != 0) ? reg_msk : 0));
	if (ret == 0) {
		ret = atc260x_reg_setbits(atc260x_gpio->atc260x,
				hwinfo->reg_outen_tbl[reg_bank], reg_msk, reg_msk);
		if (ret == 0) {
			ret = atc260x_reg_setbits(atc260x_gpio->atc260x,
					hwinfo->reg_inen_tbl[reg_bank], reg_msk, 0);
		}
	}
	return ret;
}

static void _atc260x_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct atc260x_gpio_dev *atc260x_gpio;
	struct atc260x_gpio_hwinfo const *hwinfo;
	uint reg_bank, reg_msk;
	int ret;

	atc260x_gpio = _get_atc260x_gpio_dev(chip);
	hwinfo = atc260x_gpio->hwinfo;

	if (offset >= hwinfo->gpio_nr)
		return;

	reg_bank = offset / hwinfo->gpio_nr_per_reg;
	reg_msk = 1U << (offset % hwinfo->gpio_nr_per_reg);

	ret = atc260x_reg_setbits(atc260x_gpio->atc260x,
			hwinfo->reg_data_tbl[reg_bank],
			reg_msk,
			((value != 0) ? reg_msk : 0));
	if (ret) {
		dev_err(atc260x_gpio->dev, "failed to set GPIO%u state to %d, ret=%d\n",
			offset, value, ret);
	}
}

static const struct gpio_chip sc_atc260x_gpiochip_template = {
	.label              = "atc260x-gpio-chip",
	.owner              = THIS_MODULE,
	.request            = _atc260x_gpio_request,
	.free               = NULL,
	.get_direction      = _atc260x_gpio_get_direction,
	.direction_input    = _atc260x_gpio_direction_in,
	.get                = _atc260x_gpio_get,
	.direction_output   = _atc260x_gpio_direction_out,
	.set                = _atc260x_gpio_set,
	.to_irq             = NULL,
	.set_debounce       = NULL,
	.dbg_show           = NULL,
	.can_sleep          = 1,
};

static int atc260x_gpio_probe(struct platform_device *pdev)
{
	static const struct atc260x_gpio_hwinfo * const sc_hwinfo_tbl[ATC260X_ICTYPE_CNT] = {
		[ATC260X_ICTYPE_2603A] = &sc_atc2603a_gpio_hwinfo,
		[ATC260X_ICTYPE_2603C] = &sc_atc2603c_gpio_hwinfo,
		[ATC260X_ICTYPE_2609A] = &sc_atc2609a_gpio_hwinfo,
	};
	struct atc260x_dev *atc260x;
	struct atc260x_gpio_dev *atc260x_gpio;
	uint ic_type;
	int ret;

	dev_info(&pdev->dev, "Probing...\n");

	atc260x = atc260x_get_parent_dev(&pdev->dev);

	atc260x_gpio = devm_kzalloc(&pdev->dev, sizeof(*atc260x_gpio), GFP_KERNEL);
	if (atc260x_gpio == NULL) {
		dev_err(&pdev->dev, "no mem\n");
		return -ENOMEM;
	}

	atc260x_gpio->dev = &pdev->dev;
	atc260x_gpio->atc260x = atc260x;

	ic_type = atc260x_get_ic_type(atc260x);
	BUG_ON(ic_type >= ARRAY_SIZE(sc_hwinfo_tbl));
	atc260x_gpio->hwinfo = sc_hwinfo_tbl[ic_type];

	atc260x_gpio->gpio_chip = sc_atc260x_gpiochip_template;
	atc260x_gpio->gpio_chip.dev = &pdev->dev;
	atc260x_gpio->gpio_chip.base = -1; /* use a dynamic range, user get GPIO via DTS bindings */
	atc260x_gpio->gpio_chip.ngpio = atc260x_gpio->hwinfo->gpio_nr;

	ret = gpiochip_add(&atc260x_gpio->gpio_chip);
	if (ret < 0) {
		dev_err(&pdev->dev, "Failed to register gpiochip, ret=%d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, atc260x_gpio);

	return 0;
}

static int atc260x_gpio_remove(struct platform_device *pdev)
{
	struct atc260x_gpio_dev *atc260x_gpio;
	int ret;

	atc260x_gpio = platform_get_drvdata(pdev);

	ret = gpiochip_remove(&atc260x_gpio->gpio_chip);
	if (ret == 0)
		dev_err(&pdev->dev, "Failed to unregister gpiochip, ret=%d\n", ret);

	return ret;
}

static const struct of_device_id atc260x_gpio_match[] = {
	{ .compatible = "actions,atc2603a-gpio", },
	{ .compatible = "actions,atc2603c-gpio", },
	{ .compatible = "actions,atc2609a-gpio", },
	{},
};
MODULE_DEVICE_TABLE(of, atc260x_gpio_match);

static struct platform_driver atc260x_gpio_driver = {
	.probe      = atc260x_gpio_probe,
	.remove     = atc260x_gpio_remove,
	.driver = {
		.name = "atc260x-gpio",
		.owner = THIS_MODULE,
		.of_match_table = atc260x_gpio_match,
	},
};

static int __init atc260x_gpio_init(void)
{
	return platform_driver_register(&atc260x_gpio_driver);
}
subsys_initcall(atc260x_gpio_init);

static void __exit atc260x_gpio_exit(void)
{
	platform_driver_unregister(&atc260x_gpio_driver);
}
module_exit(atc260x_gpio_exit);

MODULE_AUTHOR("Actions Semi, Inc");
MODULE_DESCRIPTION("GPIO interface for ATC260X PMIC");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:atc260x-gpio");
