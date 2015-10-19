/*
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include <linux/gpio.h>
#include <linux/ioport.h>
//#include <mach/board.h>

//#include <mach/gpio.h>

#include <asm/gpio.h>
#include <mach/hardware.h>
#ifdef CONFIG_OF_GPIO
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

#define MODULE_TAG         "[BT-PLAT] "
#define GPIO_CONFIG_NUM 1

extern void acts_wlan_bt_power(int on);
static int acts_wlan_bt_ref_count;

struct gpio_set {
	unsigned int gpio;
	int requested;
	char *label;
};

static struct rfkill *bt_rfk;
static const char bt_name[] = "bluetooth";

static struct gpio_set bt_gpio_set[GPIO_CONFIG_NUM] = {
	{ 0,  0, "BT_PWR_EN"  }
};

//static  unsigned long bt_reset;

static struct platform_device *wmt_device;

unsigned int GPIO_BT_PWR_EN_PIN;
unsigned int GPIO_MUTI_PWR_PIN;

static int bt_8723as_powered_on = 0;

static void acts_wlan_bt_power_control(int on)
{
	//printk("===> rfkill %s,%d, on:%d\n", __FUNCTION__, __LINE__, on);

	if (on) {
		if (acts_wlan_bt_ref_count > 0)
			return;
		acts_wlan_bt_ref_count++;
	} else {
		if (acts_wlan_bt_ref_count <= 0)
			return;
		acts_wlan_bt_ref_count--;
	}
	pr_info("(BT & WiFi) power control, at %s wlan_bt_ref_count=%d\n",
		__func__, acts_wlan_bt_ref_count);
	acts_wlan_bt_power(on);

	
	//printk("<=== rfkill %s,%d\n", __FUNCTION__, __LINE__);
}

int rt_bt_power_on(void)
{
	//printk("===> rfkill %s,%d\n", __FUNCTION__, __LINE__);

	bt_8723as_powered_on = 1;
    printk(KERN_INFO MODULE_TAG "mt_bt_power_on ++ %d\n", bt_8723as_powered_on);
    
	acts_wlan_bt_power_control(1);
	msleep(300);

	gpio_direction_output(GPIO_BT_PWR_EN_PIN, 1);
	msleep(300);
	
	printk("<=== rfkill %s,%d\n", __FUNCTION__, __LINE__);

	return 0;
}


void rt_bt_power_off(void)
{
    printk(KERN_INFO MODULE_TAG "mt_bt_power_off ++ %d\n", bt_8723as_powered_on);
    //printk(KERN_INFO MODULE_TAG "mt_bt_power_off ++:\n");
	gpio_direction_output(GPIO_BT_PWR_EN_PIN, 0);

	acts_wlan_bt_power_control(0);
	msleep(300);

	bt_8723as_powered_on = 0;
}

#ifdef CONFIG_OF_GPIO
static int bluetooth_parse_dt(void)
{
	struct device_node *np;
	enum of_gpio_flags flags;

	//printk("===> %s,%d\n", __FUNCTION__, __LINE__);


	/*parse dt from root*/
	np = of_find_compatible_node(NULL, NULL, "wifi,bt,power,ctl");
	if (NULL == np) {
		pr_err(MODULE_TAG "No bluetooth node found in dts\n");
		return -1;
	}

	GPIO_MUTI_PWR_PIN = of_get_named_gpio_flags(np, "wifi_bt_power_gpios", 0, &flags);

	GPIO_BT_PWR_EN_PIN = of_get_named_gpio_flags(np, "bt_en_gpios", 0, &flags);
	//GPIO_MUTI_PWR_PIN = ASOC_GPIO_PORTA(20);
	//GPIO_BT_PWR_EN_PIN = ASOC_GPIO_PORTA(23);
	pr_info("rtl8723as gpio: (%d, %d)\n", GPIO_MUTI_PWR_PIN, GPIO_BT_PWR_EN_PIN);

	
	//printk("<=== %s,%d\n", __FUNCTION__, __LINE__);
	return 0;
}
#endif

void rt_bt_gpio_init(void)
{
	//printk("===> %s,%d\n", __FUNCTION__, __LINE__);

	int i;
#ifndef CONFIG_OF_GPIO
	struct gpio_pre_cfg pcfg;
	char *name;

	name = "rtk8723_bt_pmu_en";
	memset((void *)&pcfg, 0, sizeof(struct gpio_pre_cfg));
	gpio_get_pre_cfg(name, &pcfg);
	GPIO_BT_PWR_EN_PIN = ASOC_GPIO_PORT(pcfg.iogroup, pcfg.pin_num);

	name = "wifi_8723_en";
	memset((void *)&pcfg, 0, sizeof(struct gpio_pre_cfg));
	gpio_get_pre_cfg(name, &pcfg);
	GPIO_MUTI_PWR_PIN = ASOC_GPIO_PORT(pcfg.iogroup, pcfg.pin_num);
#else
	if (bluetooth_parse_dt()) 
		return;
#endif

	acts_wlan_bt_power_control(1);

	for(i = 0; i < GPIO_CONFIG_NUM; i++){
		if (i == 0)
			bt_gpio_set[i].gpio = GPIO_BT_PWR_EN_PIN;
		if (gpio_request(bt_gpio_set[i].gpio, bt_gpio_set[i].label)) {
			// GPIO pin is already requested, check who is using now 
            printk(KERN_ALERT MODULE_TAG "GPIO%d is busy!\n", bt_gpio_set[i].gpio);
		} else{
            printk(KERN_INFO MODULE_TAG "GPIO%d is requested as %s\n", bt_gpio_set[i].gpio, bt_gpio_set[i].label);
			bt_gpio_set[i].requested = 1;
		}
	}

	
	//printk("<=== %s,%d\n", __FUNCTION__, __LINE__);
}

void rt_bt_gpio_release(void)
{
	int i;
	for(i = 0; i < GPIO_CONFIG_NUM; i++){
		if (bt_gpio_set[i].requested){
			/* GPIO pin is requested by BT, release it */
			gpio_free(bt_gpio_set[i].gpio);
			bt_gpio_set[i].requested = 0;
		}
	}
}

static int bluetooth_set_power(void *data, bool blocked)
{
	//printk("%s: block=%d\n",__func__, blocked);
	//printk("===> rfkill %s,%d, block=%d\n", __FUNCTION__, __LINE__, blocked);

	if (!blocked) {
		//gpio_direction_output(bt_reset, 1);
		rt_bt_power_on();
		mdelay(10);
	} else {
		rt_bt_power_off();
		//gpio_direction_output(bt_reset, 0);
		mdelay(15);
	}
	//printk("<=== rfkill %s,%d\n", __FUNCTION__, __LINE__);

	return 0;
}

static struct rfkill_ops rfkill_bluetooth_ops = {
	.set_block = bluetooth_set_power,
};

static void rfkill_gpio_init(void)
{
	rt_bt_gpio_init();
}

static void rfkill_gpio_deinit(void)
{
	rt_bt_gpio_release();
}

static int rfkill_bluetooth_probe(struct platform_device *pdev)
{

	int rc = 0;
	bool default_state = true;
	
	//printk("===> rfkill %s,%d\n", __FUNCTION__, __LINE__);

	//printk(KERN_INFO "-->%s\n", __func__);

	bt_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			&rfkill_bluetooth_ops, NULL);
	if (!bt_rfk) {
		rc = -ENOMEM;
		goto err_rfkill_alloc;
	}

	//printk("      %s,---> rfkill_gpio_init\n", __FUNCTION__);
	rfkill_gpio_init();
	//printk("      %s,<--- rfkill_gpio_init\n", __FUNCTION__);
	
	/* userspace cannot take exclusive control */
	rfkill_init_sw_state(bt_rfk,false);
	rc = rfkill_register(bt_rfk);
	if (rc)
		goto err_rfkill_reg;

	rfkill_set_sw_state(bt_rfk,true);
	bluetooth_set_power(NULL, default_state);

	//printk(KERN_INFO "<--%s\n", __func__);

	
	//printk("<=== rfkill %s,%d\n", __FUNCTION__, __LINE__);
	return 0;

err_rfkill_reg:
	rfkill_destroy(bt_rfk);
err_rfkill_alloc:
	return rc;
}

static int rfkill_bluetooth_remove(struct platform_device *dev)
{
	printk(KERN_INFO "-->%s\n", __func__);
	rfkill_gpio_deinit();
	rfkill_unregister(bt_rfk);
	rfkill_destroy(bt_rfk);
	printk(KERN_INFO "<--%s\n", __func__);
	return 0;
}

static int rfkill_suspend(struct platform_device *dev, pm_message_t state)
{        
	//mt_bt_power_off();
    printk(KERN_INFO "<--%s\n", __func__);
	return 0;
}

static int rfkill_resume(struct platform_device  *dev)
{
	// Because after STR, android bill power on 
	// bluetooth chip, it doesn't need to power on bt
	// here.
#if 0
	if (bt_8723as_powered_on)
	{
		rt_bt_power_on();
	}
#endif
    printk(KERN_INFO "<--%s\n", __func__);
	return 0;
}

static void rfkill_shut_down(struct platform_device * _dev)
{
    printk("rtk8723 shut down\n");
	rt_bt_power_off();
}

static struct platform_driver rfkill_bluetooth_driver = {
	.probe  = rfkill_bluetooth_probe,
	.remove = rfkill_bluetooth_remove,
	.suspend =rfkill_suspend,    
	.resume = rfkill_resume,
	.shutdown  = rfkill_shut_down,    
	.driver = {
		.name = "rfkill",
		.owner = THIS_MODULE,
	},
};

static int __init rfkill_bluetooth_init(void)
{
	printk(KERN_INFO "-->%s\n", __func__);

#ifdef CONFIG_PLATFORM_UBUNTU
	/* ubuntu system always keep power on */
	rfkill_gpio_init();
	rt_bt_power_on();
#else
	printk("===> %s,%d\n", __FUNCTION__, __LINE__);
	platform_driver_register(&rfkill_bluetooth_driver);
	wmt_device = platform_device_register_simple("rfkill", -1, NULL, 0);	
#endif

	printk("<=== %s,%d\n", __FUNCTION__, __LINE__);

	return 0;
}

static void __exit rfkill_bluetooth_exit(void)
{
	printk(KERN_INFO "-->%s\n", __func__);
#ifdef CONFIG_PLATFORM_UBUNTU
	rt_bt_power_off();
#else
	platform_driver_unregister(&rfkill_bluetooth_driver);
#endif
}

late_initcall(rfkill_bluetooth_init);
module_exit(rfkill_bluetooth_exit);
MODULE_DESCRIPTION("bluetooth rfkill");
MODULE_AUTHOR("rs <wn@realsil.com.cn>");
MODULE_LICENSE("GPL");

