/*
 * atc260x_switch_ldo.c  --  Switch LDO driver for ATC260X
 *
 * Copyright 2011 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mfd/atc260x/atc260x.h>


#define PMU_SWITCH_CTL_SWITCH1_LDO_BIAS         (1 << 0)
#define PMU_SWITCH_CTL_SWITCH1_DISCHARGE_EN (1 << 1)
#define PMU_SWITCH_CTL_SWITCH1_LDO_SET(x)       (((x) & 0x3) << 3)
#define PMU_SWITCH_CTL_SWITCH1_LDO_GET(x)       (((x) & 0x18) >> 3)
#define PMU_SWITCH_CTL_SWITCH1_LDO_MASK         (0x3 << 3)
#define PMU_SWITCH_CTL_SWITCH1_MODE_MASK      (1 << 5)
#define PMU_SWITCH_CTL_SWITCH1_MODE_SWITCH  (0 << 5)
#define PMU_SWITCH_CTL_SWITCH1_MODE_LDO         (1 << 5)
#define PMU_SWITCH_CTL_SWITCH2_LDO_BIAS     (1 << 6)
#define PMU_SWITCH_CTL_SWITCH2_LDO_SET(x)       (((x) & 0xf) << 8)
#define PMU_SWITCH_CTL_SWITCH2_LDO_GET(x)       (((x) & 0xf00) >> 8)
#define PMU_SWITCH_CTL_SWITCH2_LDO_MASK         ( 0xf << 8)
#define PMU_SWITCH_CTL_SWITCH2_LDO_VOL_SEL_MASK (1 << 12)
#define PMU_SWITCH_CTL_SWITCH2_LDO_VOL_SEL_10_20V   (0 << 12)
#define PMU_SWITCH_CTL_SWITCH2_LDO_VOL_SEL_23_33V   (1 << 12)
#define PMU_SWITCH_CTL_SWITCH2_MODE_SWITCH      (0 << 13)
#define PMU_SWITCH_CTL_SWITCH2_MODE_LDO         (1 << 13)
#define PMU_SWITCH_CTL_SWITCH2_EN               (1 << 14)
#define PMU_SWITCH_CTL_SWITCH1_EN               (1 << 15)

#define ATC260X_SWITCH_LDO_MAX_NUM  2

static unsigned short switch2_SEL;

/* Supported voltage values for regulators (in milliVolts) */
static const unsigned int switch2_ldo_volcfg1_table[] = {
	1000000, 1100000, 1200000, 1300000,
	1400000, 1500000, 1600000, 1700000,
	1750000, 1800000, 1850000, 1900000,
	1950000, 2000000, 2000000, 2000000,
};

static const unsigned int switch2_ldo_volcfg2_table[] = {
	2300000, 2400000, 2500000, 2600000,
	2700000, 2800000, 2900000, 3000000,
	3050000, 3100000, 3150000, 3200000,
	3250000, 3300000, 3300000, 3300000,
};

static const unsigned int switch1_ldo_volcfg_table[] = {
	3000000, 3100000, 3200000, 3300000,
};

struct atc260x_switch_ldo_property
{
	int table_len;
	const unsigned int *table;
};

struct atc260x_swldo_dev {
	const char *name;
	struct regulator_desc desc;
	struct atc260x_dev *atc260x;
	struct regulator_dev *regulator;
	struct atc260x_switch_ldo_property property;
	uint ctrl_reg;
	uint pmic_type;
	uint pmic_ver;
	uint vsel_now;
};

static int atc260x_switch_ldo_list_voltage(struct regulator_dev *rdev,
					  unsigned int selector)
{
	struct atc260x_swldo_dev *ldo = rdev_get_drvdata(rdev);
	int id = rdev_get_id(rdev);

	if (id > ATC260X_SWITCH_LDO_MAX_NUM ||
					selector >= (ldo->property.table_len))
		return -EINVAL;

	return ldo->property.table[selector];
}

static int atc260x_switch_ldo_is_enabled(struct regulator_dev *rdev)
{
	struct atc260x_swldo_dev *ldo = rdev_get_drvdata(rdev);
	struct atc260x_dev *atc260x = ldo->atc260x;
	int id, ret, mask_bit;

	id = rdev_get_id(rdev);
	if (id > ATC260X_SWITCH_LDO_MAX_NUM)
		return -EINVAL;

	ret = atc260x_reg_read(atc260x, ldo->ctrl_reg);
	if (ret < 0)
		return ret;

	if (1 == id) {
		mask_bit = PMU_SWITCH_CTL_SWITCH1_EN | PMU_SWITCH_CTL_SWITCH1_MODE_LDO;
		ret &= mask_bit;
		if (ldo->pmic_type == ATC260X_ICTYPE_2603A &&
			ldo->pmic_ver <= ATC260X_ICVER_C) {
			/*ver A B C*/
			return (ret == mask_bit);
		} else {
			/*ver D*/
			return (ret == 0);
		}
	} else {
		mask_bit = PMU_SWITCH_CTL_SWITCH2_EN | PMU_SWITCH_CTL_SWITCH2_MODE_LDO;
		ret &= mask_bit;
		return (ret == mask_bit);
	}
}

static int atc260x_switch_ldo_enable(struct regulator_dev *rdev)
{
	struct atc260x_swldo_dev *ldo = rdev_get_drvdata(rdev);
	struct atc260x_dev *atc260x = ldo->atc260x;
	int id, ret;

	id = rdev_get_id(rdev);
	if (id > ATC260X_SWITCH_LDO_MAX_NUM)
		return -EINVAL;

	if (1 == id) {
		if (ldo->pmic_type == ATC260X_ICTYPE_2603A &&
			ldo->pmic_ver <= ATC260X_ICVER_C) {
			ret = atc260x_reg_setbits(atc260x, ldo->ctrl_reg,
						PMU_SWITCH_CTL_SWITCH1_EN, PMU_SWITCH_CTL_SWITCH1_EN);
			pr_info("enable switch 1, version A~C!\n");
		} else {
			/*ver D, enable is 0, disable is 1*/
			ret = atc260x_reg_setbits(atc260x, ldo->ctrl_reg,
						PMU_SWITCH_CTL_SWITCH1_EN, 0);
			pr_info("enable switch 1, version D!\n");
		}
	} else {
		ret = atc260x_reg_setbits(atc260x, ldo->ctrl_reg,
				PMU_SWITCH_CTL_SWITCH2_EN, PMU_SWITCH_CTL_SWITCH2_EN);
	}

	mdelay(2);

	return ret;
}

static int atc260x_switch_ldo_disable(struct regulator_dev *rdev)
{
	struct atc260x_swldo_dev *ldo = rdev_get_drvdata(rdev);
	struct atc260x_dev *atc260x = ldo->atc260x;
	int id, ret;

	id = rdev_get_id(rdev);
	if (id > ATC260X_SWITCH_LDO_MAX_NUM)
		return -EINVAL;

	if (1 == id) {
		if (ldo->pmic_type == ATC260X_ICTYPE_2603A &&
			ldo->pmic_ver <= ATC260X_ICVER_C) {
			/*ver A~C, enable is 1, disable is 0*/
			ret = atc260x_reg_setbits(atc260x, ldo->ctrl_reg,
					PMU_SWITCH_CTL_SWITCH1_EN, 0);
			pr_info("disable switch 1, version A~C!\n");
		} else {
			/*ver D, enable is 0, disable is 1*/
			ret = atc260x_reg_setbits(atc260x, ldo->ctrl_reg,
					PMU_SWITCH_CTL_SWITCH1_EN, PMU_SWITCH_CTL_SWITCH1_EN);
			pr_info("disable switch 1, version D!\n");
		}
	} else {
			ret = atc260x_reg_setbits(atc260x, ldo->ctrl_reg,
				PMU_SWITCH_CTL_SWITCH2_EN, 0);
	}

	return ret;
}

static int atc260x_switch_ldo_set_voltage(struct regulator_dev *rdev,
					 int min_uV, int max_uV, unsigned *selector)
{
	struct atc260x_swldo_dev *ldo = rdev_get_drvdata(rdev);
	struct atc260x_dev *atc260x = ldo->atc260x;
	int id, ret, vsel;
	unsigned int uV;

	id = rdev_get_id(rdev);
	if (id > ATC260X_SWITCH_LDO_MAX_NUM)
		return -EINVAL;

	if (min_uV < (rdev->constraints->min_uV) ||
				min_uV > (rdev->constraints->max_uV))
		return -EINVAL;
	if (max_uV < (rdev->constraints->min_uV) ||
				max_uV >(rdev->constraints->max_uV))
		return -EINVAL;

	for (vsel = 0; vsel < ldo->property.table_len; vsel++) {
		uV = ldo->property.table[vsel];

		/* Break at the first in-range value */
		if (min_uV <= uV && uV <= max_uV)
			break;
	}

	if (vsel == ldo->property.table_len)
		return -EINVAL;

	if (1 ==id) {
		ret = atc260x_reg_setbits(atc260x, ldo->ctrl_reg,
				PMU_SWITCH_CTL_SWITCH1_LDO_MASK,
				PMU_SWITCH_CTL_SWITCH1_LDO_SET(vsel));
	} else {
		ret = atc260x_reg_setbits(atc260x, ldo->ctrl_reg,
				PMU_SWITCH_CTL_SWITCH2_LDO_MASK,
				PMU_SWITCH_CTL_SWITCH2_LDO_SET(vsel));
	}

	if (0 == ret) {
		ldo->vsel_now = *selector = vsel;
		mdelay(1);
	}

	return ret;
}

static int atc260x_switch_ldo_get_voltage(struct regulator_dev *rdev)
{
	struct atc260x_swldo_dev *ldo = rdev_get_drvdata(rdev);
	struct atc260x_dev *atc260x = ldo->atc260x;
	int id, ret, vsel;

	id = rdev_get_id(rdev);
	if (id > ATC260X_SWITCH_LDO_MAX_NUM)
		return -EINVAL;

	ret = atc260x_reg_read(atc260x, ldo->ctrl_reg);
	if (ret < 0)
		return ret;

	if (1 == id)
		vsel = PMU_SWITCH_CTL_SWITCH1_LDO_GET(ret);
	else
		vsel = PMU_SWITCH_CTL_SWITCH2_LDO_GET(ret);

	ret = ldo->property.table[vsel];

	return ret;
}


static struct regulator_ops atc260x_switch_ldo_ops = {
	.list_voltage    = atc260x_switch_ldo_list_voltage,
	.get_voltage   = atc260x_switch_ldo_get_voltage,
	.set_voltage   = atc260x_switch_ldo_set_voltage,
	.is_enabled     = atc260x_switch_ldo_is_enabled,
	.enable           = atc260x_switch_ldo_enable,
	.disable          = atc260x_switch_ldo_disable,
};

static int ldo_init(struct atc260x_swldo_dev *ldo, struct regulator_init_data *init_data)
{
	int ret = 0, id;
	int min_uv =  init_data->constraints.min_uV;
	int max_uv = init_data->constraints.max_uV;

	id = ldo->desc.id;
	if (id == 1) {
		/* Switch LDO 1, diable  discharge*/
		atc260x_reg_setbits(ldo->atc260x, ldo->ctrl_reg,
			PMU_SWITCH_CTL_SWITCH1_DISCHARGE_EN, 0);

		if ((ldo->pmic_type == ATC260X_ICTYPE_2603A) &&
			(ldo->pmic_ver <= ATC260X_ICVER_C)) {
			/* 5302 ver A,B,C,  1- LDO, 0-- switch*/
			pr_warn("[SWITCH1] set switch 1 to ldo mode (2603a Version A~C!)\n");
			atc260x_reg_setbits(ldo->atc260x, ldo->ctrl_reg,
				PMU_SWITCH_CTL_SWITCH1_MODE_MASK,
				PMU_SWITCH_CTL_SWITCH1_MODE_LDO);
		} else {
			/* ver D, 0-- LDO, 1-- switch*/
			pr_warn("[SWITCH1] set switch 1 to ldo mode Version D!\n");
			atc260x_reg_setbits(ldo->atc260x, ldo->ctrl_reg,
				PMU_SWITCH_CTL_SWITCH1_MODE_MASK,
				0);
		}
		ldo->property.table = switch1_ldo_volcfg_table;
		ldo->property.table_len = ARRAY_SIZE(switch1_ldo_volcfg_table);;
	} else if (id == 2) {
		/* Switch LDO 2 */
		int table1_len = ARRAY_SIZE(switch2_ldo_volcfg1_table);
		int table2_len = ARRAY_SIZE(switch2_ldo_volcfg2_table);

		atc260x_reg_setbits(ldo->atc260x, ldo->ctrl_reg,
				PMU_SWITCH_CTL_SWITCH2_EN,
				0);

		if ((switch2_ldo_volcfg1_table[0] <= min_uv) &&
				(switch2_ldo_volcfg1_table[table1_len - 1] >= max_uv)) {
			atc260x_reg_setbits(ldo->atc260x, ldo->ctrl_reg,
				PMU_SWITCH_CTL_SWITCH2_LDO_VOL_SEL_MASK,
				PMU_SWITCH_CTL_SWITCH2_LDO_VOL_SEL_10_20V);
			ldo->property.table = switch2_ldo_volcfg1_table;
			ldo->property.table_len = table1_len;
			switch2_SEL = PMU_SWITCH_CTL_SWITCH2_LDO_VOL_SEL_10_20V;
		} else if ((switch2_ldo_volcfg2_table[0] <= min_uv) &&
				(switch2_ldo_volcfg2_table[table2_len - 1] >= max_uv)) {
			atc260x_reg_setbits(ldo->atc260x, ldo->ctrl_reg,
				PMU_SWITCH_CTL_SWITCH2_LDO_VOL_SEL_MASK,
				PMU_SWITCH_CTL_SWITCH2_LDO_VOL_SEL_23_33V);
			ldo->property.table = switch2_ldo_volcfg2_table;
			ldo->property.table_len = table2_len;
			switch2_SEL = PMU_SWITCH_CTL_SWITCH2_LDO_VOL_SEL_23_33V;
			} else {
				ret = -EINVAL;
			}
	} else {
		ret = -EINVAL;
	}

	return ret;
}

static int atc260x_switch_ldo_probe(struct platform_device *pdev)
{
	static const u16 sc_ctrl_reg_tbl[ATC260X_ICTYPE_CNT] = {
		[ATC260X_ICTYPE_2603A] = ATC2603A_PMU_SWITCH_CTL,
		[ATC260X_ICTYPE_2603C] = ATC2603C_PMU_SWITCH_CTL,
		[ATC260X_ICTYPE_2609A] = 0,
	};
	struct regulator_config config = { };
	struct regulator_init_data  *init_data;
	struct atc260x_dev *atc260x;
	struct atc260x_swldo_dev *ldo;
	uint ic_type;
	int id, ret;

	id = pdev->id;
	dev_info(&pdev->dev, "Probing %s, id=%u\n", pdev->name, id);

	atc260x = atc260x_get_parent_dev(&pdev->dev);

	init_data = of_get_regulator_init_data(&pdev->dev, pdev->dev.of_node);
	if (!init_data) {
		return -ENOMEM;
	}

	ldo = devm_kzalloc(&pdev->dev, sizeof(struct atc260x_swldo_dev), GFP_KERNEL);
	if (ldo == NULL) {
		dev_err(&pdev->dev, "Unable to allocate private data\n");
		return -ENOMEM;
	}

	ldo->atc260x = atc260x;
	ldo->desc.id = id;
	ldo->name = init_data->constraints.name;
	ldo->desc.name = ldo->name;

	ic_type = atc260x_get_ic_type(atc260x);
	BUG_ON(ic_type >= ARRAY_SIZE(sc_ctrl_reg_tbl));
	ldo->ctrl_reg = sc_ctrl_reg_tbl[ic_type];
	if (ldo->ctrl_reg == 0) {
		dev_err(&pdev->dev, "device not support\n");
		return -ENXIO;
	}
	ldo->pmic_type = atc260x_get_ic_type(atc260x);
	ldo->pmic_ver = atc260x_get_ic_ver(atc260x);
	dev_err(&pdev->dev, "pmic_ver=%u\n", ldo->pmic_ver);

	ret = ldo_init(ldo, init_data);
	if (ret) {
		pr_err("Faild to init Switch LDO%d", id);
		goto err;
	}

	ldo->desc.type = REGULATOR_VOLTAGE;
	ldo->desc.n_voltages = ldo->property.table_len;
	ldo->desc.ops = &atc260x_switch_ldo_ops;
	ldo->desc.owner = THIS_MODULE;

	config.dev = &pdev->dev;
	config.init_data = init_data;
	config.driver_data = ldo;
	config.of_node = pdev->dev.of_node;

	ldo->regulator = regulator_register(&ldo->desc, &config);
	if (IS_ERR(ldo->regulator)) {
		ret = PTR_ERR(ldo->regulator);
		dev_err(&pdev->dev, "Failed to register LDO%d: %d\n",
			id + 1, ret);
		goto err;
	}

	ret = atc260x_reg_read(atc260x, ldo->ctrl_reg);
	if (ret < 0)
		goto err2;
	if (1 == id)
		ldo->vsel_now = PMU_SWITCH_CTL_SWITCH1_LDO_GET(ret);
	else
		ldo->vsel_now = PMU_SWITCH_CTL_SWITCH2_LDO_GET(ret);

	platform_set_drvdata(pdev, ldo);

#if 0 && defined(CONFIG_REGULATOR_VIRTUAL_CONSUMER)
	/* for debug */
	platform_device_register_resndata(NULL, "reg-virt-consumer",
		30 + id,
		NULL, 0,
		ldo->name, strlen(ldo->name) + 1);
#endif
	return 0;

err2:
	regulator_unregister(ldo->regulator);
err:
	return ret;
}

static int atc260x_switch_ldo_remove(struct platform_device *pdev)
{
	struct atc260x_swldo_dev *ldo = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);

	regulator_unregister(ldo->regulator);

	return 0;
}

static int atc260x_switch_ldo_suspend_late(struct device *dev)
{
	struct atc260x_swldo_dev *ldo = dev_get_drvdata(dev);
	uint state = atc260x_switch_ldo_is_enabled(ldo->regulator);
	if (state) {
		dev_warn(dev, "%s() switch%d state : %d\n",
			__func__, ldo->desc.id, state);
	} else {
		dev_info(dev, "%s() switch%d state : %d\n",
			__func__, ldo->desc.id, state);
	}
	return 0;
}

static int atc260x_switch_ldo_resume_early(struct device *dev)
{
	struct atc260x_swldo_dev *ldo = dev_get_drvdata(dev);
	int id, ret;

	id = ldo->desc.id;

	dev_info(dev, "%s() ldo%d\n", __func__, ldo->desc.id);

	if ((ldo->vsel_now) >= (ldo->property.table_len))
		return -EINVAL;

	if (1 ==id) {
		ret = atc260x_reg_setbits(ldo->atc260x, ldo->ctrl_reg,
				PMU_SWITCH_CTL_SWITCH1_LDO_MASK,
				PMU_SWITCH_CTL_SWITCH1_LDO_SET(ldo->vsel_now));
	} else {
		ret = atc260x_reg_setbits(ldo->atc260x, ldo->ctrl_reg,
				PMU_SWITCH_CTL_SWITCH2_LDO_MASK |
					PMU_SWITCH_CTL_SWITCH2_LDO_VOL_SEL_MASK,
				PMU_SWITCH_CTL_SWITCH2_LDO_SET(ldo->vsel_now)) |
					switch2_SEL;
	}

	return 0;
}

static const struct dev_pm_ops s_atc260x_switch_ldo_pm_ops = {
	.suspend_late  = atc260x_switch_ldo_suspend_late,
	.resume_early  = atc260x_switch_ldo_resume_early,
	.freeze_late   = atc260x_switch_ldo_suspend_late,
	.thaw_early    = atc260x_switch_ldo_resume_early,
	.poweroff_late = atc260x_switch_ldo_suspend_late,
	.restore_early = atc260x_switch_ldo_resume_early,
};

static const struct of_device_id atc260x_switch_ldo_match[] = {
	/* only 2603a & 2603c have switchs */
	{ .compatible = "actions,atc2603a-switch", },
	{ .compatible = "actions,atc2603c-switch", },
	{},
};
MODULE_DEVICE_TABLE(of, atc260x_switch_ldo_match);

static struct platform_driver atc260x_switch_ldo_driver = {
	.driver     = {
		.name   = "atc260x-switch-ldo",
		.owner  = THIS_MODULE,
		.pm     = &s_atc260x_switch_ldo_pm_ops,
		.of_match_table = of_match_ptr(atc260x_switch_ldo_match),
	},
	.probe = atc260x_switch_ldo_probe,
	.remove = atc260x_switch_ldo_remove,
};

static int __init atc260x_switch_ldo_init(void)
{
	int ret;
	ret = platform_driver_register(&atc260x_switch_ldo_driver);
	if (ret != 0)
		pr_err("Failed to register ATC260X Switch LDO driver: %d\n", ret);

	return 0;
}
subsys_initcall(atc260x_switch_ldo_init);

static void __exit atc260x_switch_ldo_exit(void)
{
	platform_driver_unregister(&atc260x_switch_ldo_driver);
}
module_exit(atc260x_switch_ldo_exit);

/* Module information */
MODULE_AUTHOR("Actions Semi, Inc");
MODULE_DESCRIPTION("ATC260X Switch LDO driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:atc260x-switch-ldo");
