/*
 * bluetooth.c
 * Copyright (C) Actions Semi
 * Copyright (C) 2010 Samsung Electronics Co., Ltd.
 * Copyright (C) 2008 Google, Inc.
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	    bluetooth "driver"
 *
 */

/* Control bluetooth power for asoc platform */

#define DEBUG

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/wakelock.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/init.h>

#ifdef CONFIG_OF_GPIO
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif

#include <mach/hardware.h>

#define DRV_NAME		"bt_rfkill"
#define MODULE_TAG		DRV_NAME

static struct rfkill *bt_rfk;
static const char bt_name[] = "ap6210";

static int bt_en_pin, /* BT enable Pin */
		   bt_wifi_power_pin;	/* WiFi & BT Power Pin */
		   
#ifdef HAVE_LPM
static int bt_wake_pin, bt_hwake_pin;
#endif

/* For Dts/pre_cfg */
#define OF_BT_NODE			"wifi,bt,power,ctl"
#define BT_HWAKE_PIN_NAME	"bt_host_wake_gpio"
#define BT_EN_PIN_NAME		"bt_en_gpios"
#define BT_WAKE_PIN_NAME	"bt_wake_gpio"
#define BT_POWER_PIN_NAME	"wifi_bt_power_gpios"
#define BT_WIFI_PIN_NAME	BT_POWER_PIN_NAME

/*
 * Host wake up: indicate to the host that chip needs attentation
 * GPIO_BT_WAKE: Bluetooth device wake-up
 */
#define GPIO_BT_WAKE_ID		"bt_wake"
#define GPIO_BT_HWAKE_ID	"bt_hwake"
#define GPIO_BT_nRST_ID		"bt_nRest"

extern void acts_wlan_bt_power(int on);

static int bt_powered_on;
static int acts_wlan_bt_ref_count;

static void acts_wlan_bt_power_control(int on)
{
	if (on) {
		if (acts_wlan_bt_ref_count > 0)
			return;
		acts_wlan_bt_ref_count++;
	} else {
		if (acts_wlan_bt_ref_count <= 0)
			return;
		acts_wlan_bt_ref_count--;
	}
	pr_info("(BT & WiFi) power control, at %s wlan_bt_ref_count=%d",
		__func__, acts_wlan_bt_ref_count);
	acts_wlan_bt_power(on);
}

int bt_power_on(void)
{
	bt_powered_on = 1;
    pr_info(MODULE_TAG "mt_bt_power_on ++ %d\n", bt_powered_on);
    
	acts_wlan_bt_power_control(1);
	msleep(50);

	gpio_direction_output(bt_en_pin, 1);
	/*
	 * 50msec, delay after bt rst
	 * (bcm4329 powerup sequence)
	 */
	msleep(50);

	return 0;
}

void bt_power_off(void)
{
    pr_info(MODULE_TAG "mt_bt_power_off ++ %d\n", bt_powered_on);
	gpio_direction_output(bt_en_pin, 0);

	acts_wlan_bt_power_control(0);
	msleep(50);

	bt_powered_on = 0;
}

static int bluetooth_set_power(void *data, enum rfkill_user_states state)
{
	switch (state) {
	case RFKILL_USER_STATE_UNBLOCKED:
		printk("[BT] Device Powering ON\n");
		bt_power_on();
		break;

	case RFKILL_USER_STATE_SOFT_BLOCKED:
		printk("[BT] Device Powering OFF\n");
		bt_power_off();
		break;

	default:
		pr_err("[BT] Bad bluetooth rfkill state %d\n", state);
	}

	return 0;
}

static int bt_rfkill_set_block(void *data, bool blocked)
{
	unsigned int ret = 0;

	ret = bluetooth_set_power(data, blocked ?
			RFKILL_USER_STATE_SOFT_BLOCKED :
			RFKILL_USER_STATE_UNBLOCKED);

	return ret;
}

static const struct rfkill_ops bt_rfkill_ops = {
	.set_block = bt_rfkill_set_block,
};

static int bt_rfkill_probe(struct platform_device *pdev)
{
	int ret;

	/* Init GPIO pin */
	struct device_node *np;
	enum of_gpio_flags flags;

	pr_debug("[BT]in rfkill probe");	

	/*parse dt from root*/
	np = of_find_compatible_node(NULL, NULL, OF_BT_NODE);
	if (NULL == np) {
		pr_err(MODULE_TAG "No bluetooth node found in dts\n");
		return -ENOENT;
	}

	bt_wifi_power_pin = of_get_named_gpio_flags(np, BT_POWER_PIN_NAME, 0, &flags);
	bt_en_pin = of_get_named_gpio_flags(np, BT_EN_PIN_NAME, 0, &flags);
#ifdef HAVE_LPM
	bt_wake_pin = of_get_named_gpio_flags(np, BT_WAKE_PIN_NAME, 0, &flags);
	bt_hwake_pin = of_get_named_gpio_flags(np, BT_HWAKE_PIN_NAME, 0, &flags);
	pr_info("bluetooth gpio: hwake: %d, en: %d, wake: %d, power: %d\n", bt_hwake_pin, bt_en_pin, bt_wake_pin, bt_wifi_power_pin);
#else
	pr_info("bluetooth gpio: (en: %d, power: %d)\n", bt_en_pin, bt_wifi_power_pin);
#endif


#ifdef HAVE_LPM

	/*  request gpio */
	ret = gpio_request(bt_hwake_pin, GPIO_BT_HWAKE_ID);
	if (ret < 0) {
		pr_err("brcm bt: fail to request gpio for HWAKE\n");
		goto err_req_gpio_bt_hwake;
	}

	ret = gpio_request(bt_wake_pin, GPIO_BT_WAKE_ID);
	if (ret < 0) {
		pr_err("brcm bt: fail to request gpio for WAKE\n");
		goto err_req_gpio_bt_wake;
	}
#endif
	ret = gpio_request(bt_en_pin, GPIO_BT_nRST_ID);
	if (ret < 0) {
		pr_err("brcm bt: fail to request gpio for nRST\n");
		goto err_req_gpio_bt_en;
	}

	bt_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			&bt_rfkill_ops, NULL);

	if (!bt_rfk) {
		pr_err("[BT] bt_rfk : rfkill_alloc is failed\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	rfkill_init_sw_state(bt_rfk, 0);

	pr_debug("[BT] rfkill_register(bt_rfk)\n");

	ret = rfkill_register(bt_rfk);
	if (ret) {
		pr_err("********ERROR IN REGISTERING THE bt_rfk********\n");
		goto err_register;
	}

	rfkill_set_sw_state(bt_rfk, 1);

	return ret;

err_register:
	rfkill_destroy(bt_rfk);

err_alloc:
	gpio_free(bt_en_pin);

err_req_gpio_bt_en:
#ifdef HAVE_LPM
	gpio_free(bt_wake_pin);

err_req_gpio_bt_wake:
	gpio_free(bt_hwake_pin);

err_req_gpio_bt_hwake:
#endif
	return ret;
}

static int bt_rfkill_remove(struct platform_device *pdev)
{
	if (bt_rfk) {
		rfkill_unregister(bt_rfk);
		rfkill_destroy(bt_rfk);
	}
	bt_rfk = NULL;

	bt_rfkill_set_block(NULL, true);

#ifdef HAVE_LPM
	gpio_free(bt_hwake_pin);
	gpio_free(bt_wake_pin);
#endif
	gpio_free(bt_en_pin);

	return 0;
}

static int rfkill_suspend(struct platform_device *dev, pm_message_t state)
{        
	printk(KERN_INFO "<--%s\n", __func__);
	return 0;
}

static int rfkill_resume(struct platform_device  *dev)
{    
	/* 
	 * Because after STR, android bill power on 
	 * bluetooth chip, it doesn't need to power on bt
	 * here.
	 */
#if 0
	if (bt_powered_on)
	{
		bluetooth_set_power(NULL, RFKILL_USER_STATE_UNBLOCKED);
	}
#endif
	printk(KERN_INFO "<--%s\n", __func__);
	return 0;
}

static void rfkill_shut_down(struct platform_device * _dev)
{
	printk("bluetooth shut down\n");
	bluetooth_set_power(NULL, RFKILL_USER_STATE_SOFT_BLOCKED);
}

static struct platform_driver bt_device_rfkill = {
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
	},
	.probe = bt_rfkill_probe,
	.remove = bt_rfkill_remove,
	.suspend = rfkill_suspend,    
	.resume = rfkill_resume,
	.shutdown = rfkill_shut_down, 	
};

static int __init bt_rfkill_init(void)
{
	int ret = 0;
	ret = platform_driver_register(&bt_device_rfkill);
	if (ret) {
		pr_err("register brcm platform driver error!\n");
		return ret;
	}	
	platform_device_register_simple(DRV_NAME, -1, NULL, 0);		

	return ret;
}

static void __exit bt_rfkill_exit(void)
{
	printk(KERN_INFO "-->%s\n", __func__);
	platform_driver_unregister(&bt_device_rfkill);

}

module_init(bt_rfkill_init);
module_exit(bt_rfkill_exit);

MODULE_AUTHOR("fwang@actions");
MODULE_DESCRIPTION("bluetooth rfkill");
MODULE_LICENSE("GPL");

