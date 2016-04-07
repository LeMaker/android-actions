/*
 * atc260x-hwmon.c  --  hardware monitoring for ATC260X
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
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/slab.h>

#include <linux/mfd/atc260x/atc260x.h>

struct atc260x_hwmon_dev {
	struct device *dev;
	struct atc260x_dev *atc260x;
	struct device *classdev;
};

static const char * sc_atc260x_hwmon_alt_ch_name_tbl[][2] = {
	{"BATV",        "bat_voltage" },
	{"BATI",        "bat_current" },
	{"VBUSV",       "vbus_voltage" },
	{"VBUSI",       "vbus_current" },
	{"SYSPWRV",     "syspower_voltage" },
	{"WALLV",       "wall_voltage" },
	{"WALLI",       "wall_current" },
	{"CHGI",        "charge_current" },
	{"IREF",        "current_ref" },
	{"REMCON",      "remote_control" },
	{"ICTEMP",      "ic_temperature" },
	{"BAKBATV",     "backupbat_voltage" },
	{"AUX0",        "aux0" },
	{"AUX1",        "aux1" },
	{"AUX2",        "aux2" },
	{"AUX3",        "aux3" },
	{"ICM",         "icm_current" },
	{"SVCC",        "svcc_voltage" },
};

static ssize_t show_name(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	strcpy(buf, "atc260x\n");
	return strlen(buf);
}

static ssize_t show_value(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct atc260x_hwmon_dev *hwmon = dev_get_drvdata(dev);
	struct sensor_device_attribute *sen_attr;
	int channel, ret;
	s32 tr_value;
	const char *ch_unit_name;

	sen_attr = to_sensor_dev_attr(attr);
	channel = sen_attr->index;
	if (channel < 0) {
		const char *core_ch_name;
		uint i;
		for (i = 0; i < ARRAY_SIZE(sc_atc260x_hwmon_alt_ch_name_tbl); i++) {
			if (strcmp(sen_attr->dev_attr.attr.name,
					sc_atc260x_hwmon_alt_ch_name_tbl[i][1]) == 0) {
				core_ch_name = sc_atc260x_hwmon_alt_ch_name_tbl[i][0];
				channel = atc260x_auxadc_find_chan(hwmon->atc260x, core_ch_name);
				sen_attr->index = channel; /* save it. */
				break;
			}
		}
	}

	if (channel < 0) {
		strcpy(buf, "<channel not found>\n");
		return strlen(buf);
	}

	ret = atc260x_auxadc_get_translated(hwmon->atc260x, channel, &tr_value);
	if (ret != 0) {
		return scnprintf(buf, PAGE_SIZE, "<translate error, ret=%d>\n", ret);
	} else {
		ch_unit_name = atc260x_auxadc_channel_unit_name(hwmon->atc260x, channel);
		return scnprintf(buf, PAGE_SIZE, "%d %s\n", tr_value, ch_unit_name);
	}
}

static DEVICE_ATTR(name, S_IRUGO, show_name, NULL);

static SENSOR_DEVICE_ATTR(wall_voltage, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(vbus_voltage, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(bat_voltage, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(syspower_voltage, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(backupbat_voltage, S_IRUGO, show_value, NULL, -1);

static SENSOR_DEVICE_ATTR(wall_current, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(vbus_current, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(bat_current, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(charge_current, S_IRUGO, show_value, NULL, -1);

static SENSOR_DEVICE_ATTR(current_ref, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(ic_temperature, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(remote_control, S_IRUGO, show_value, NULL, -1);

static SENSOR_DEVICE_ATTR(aux0, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(aux1, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(aux2, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(aux3, S_IRUGO, show_value, NULL, -1);

static SENSOR_DEVICE_ATTR(icm_current, S_IRUGO, show_value, NULL, -1);
static SENSOR_DEVICE_ATTR(svcc_voltage, S_IRUGO, show_value, NULL, -1);

static struct attribute *atc260x_attributes[] = {
	&dev_attr_name.attr,

	&sensor_dev_attr_wall_voltage.dev_attr.attr,
	&sensor_dev_attr_vbus_voltage.dev_attr.attr,
	&sensor_dev_attr_bat_voltage.dev_attr.attr,
	&sensor_dev_attr_syspower_voltage.dev_attr.attr,
	&sensor_dev_attr_backupbat_voltage.dev_attr.attr,

	&sensor_dev_attr_wall_current.dev_attr.attr,
	&sensor_dev_attr_vbus_current.dev_attr.attr,
	&sensor_dev_attr_bat_current.dev_attr.attr,
	&sensor_dev_attr_charge_current.dev_attr.attr,

	&sensor_dev_attr_current_ref.dev_attr.attr,
	&sensor_dev_attr_ic_temperature.dev_attr.attr,
	&sensor_dev_attr_remote_control.dev_attr.attr,

	&sensor_dev_attr_aux0.dev_attr.attr,
	&sensor_dev_attr_aux1.dev_attr.attr,
	&sensor_dev_attr_aux2.dev_attr.attr,
	&sensor_dev_attr_aux3.dev_attr.attr,

	&sensor_dev_attr_icm_current.dev_attr.attr,
	&sensor_dev_attr_svcc_voltage.dev_attr.attr,

	NULL
};

static const struct attribute_group atc260x_attr_group = {
	.attrs  = atc260x_attributes,
};

static int atc260x_hwmon_probe(struct platform_device *pdev)
{
	struct atc260x_dev *atc260x = dev_get_drvdata(pdev->dev.parent);
	struct atc260x_hwmon_dev *hwmon;
	int ret;

	dev_info(&pdev->dev, "Probing %s\n", pdev->name);

	hwmon = devm_kzalloc(&pdev->dev, sizeof(struct atc260x_hwmon_dev), GFP_KERNEL);
	if (!hwmon)
		return -ENOMEM;

	hwmon->dev = &pdev->dev;
	hwmon->atc260x = atc260x;

	ret = sysfs_create_group(&pdev->dev.kobj, &atc260x_attr_group);
	if (ret)
		goto err;

	hwmon->classdev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(hwmon->classdev)) {
		ret = PTR_ERR(hwmon->classdev);
		goto err_sysfs;
	}

	platform_set_drvdata(pdev, hwmon);
	return 0;

err_sysfs:
	sysfs_remove_group(&pdev->dev.kobj, &atc260x_attr_group);
err:
	return ret;
}

static int atc260x_hwmon_remove(struct platform_device *pdev)
{
	struct atc260x_hwmon_dev *hwmon = platform_get_drvdata(pdev);
	hwmon_device_unregister(hwmon->classdev);
	sysfs_remove_group(&pdev->dev.kobj, &atc260x_attr_group);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static const struct of_device_id atc260x_hwmon_match[] = {
	{ .compatible = "actions,atc2603a-hwmon", },
	{ .compatible = "actions,atc2603c-hwmon", },
	{ .compatible = "actions,atc2609a-hwmon", },
	{},
};
MODULE_DEVICE_TABLE(of, atc260x_hwmon_match);

static struct platform_driver atc260x_hwmon_driver = {
	.probe = atc260x_hwmon_probe,
	.remove = atc260x_hwmon_remove,
	.driver = {
		.name = "atc260x-hwmon",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(atc260x_hwmon_match),
	},
};

module_platform_driver(atc260x_hwmon_driver)

/*static int __init atc260x_hwmon_init(void) */
/*{ */
/*    return platform_driver_register(&atc260x_hwmon_driver); */
/*} */
/*//subsys_initcall(atc260x_hwmon_init); */
/*late_initcall(atc260x_hwmon_init); */
/* */
/*static void __exit atc260x_hwmon_exit(void) */
/*{ */
/*    platform_driver_unregister(&atc260x_hwmon_driver); */
/*} */
/*module_exit(atc260x_hwmon_exit); */

MODULE_AUTHOR("Actions Semi, Inc");
MODULE_DESCRIPTION("ATC260X Hardware Monitoring");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:atc260x-hwmon");

