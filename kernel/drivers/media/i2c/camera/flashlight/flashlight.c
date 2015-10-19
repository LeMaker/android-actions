/*
 * arch/arm/mach-msm/flashlight.c - flashlight driver
 *
 *  Copyright (C) 2009 raymond wang <wangjiaqi@actions-semi.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/wakelock.h>
#include <linux/hrtimer.h>
#include <linux/leds.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/of_gpio.h>

#include "flashlight.h"

#define GPIO_NAME_FLASHLIGHT	"flashlight_gpio"

struct flashlight_struct {
	struct led_classdev fl_lcdev;
	struct early_suspend early_suspend_flashlight;
	spinlock_t spin_lock;
	struct hrtimer timer;
	int brightness;
	int gpio_pin;
	int flash_duration_ms;
};

static struct flashlight_struct the_fl;
//static struct gpio_pre_cfg gpio_cfg;
static bool gpio_act_level=1;
static int gpio_pin;
static bool flashlight_exist = false;
    
#if 0
static inline void toggle(void)
{
	gpio_direction_output(the_fl.gpio_torch, 0);
	udelay(2);
	gpio_direction_output(the_fl.gpio_torch, 1);
	udelay(2);
}

static void flashlight_hw_command(uint8_t addr, uint8_t data)
{
	int i;

	for (i = 0; i < addr + 17; i++)
		toggle();
	udelay(500);

	for (i = 0; i < data; i++)
		toggle();
	udelay(500);
}
#endif

static enum hrtimer_restart flashlight_timeout(struct hrtimer *timer)
{
	unsigned long flags;

	pr_debug("%s\n", __func__);

	spin_lock_irqsave(&the_fl.spin_lock, flags);
	gpio_direction_output(gpio_pin, 0);
	the_fl.brightness = LED_OFF;
	spin_unlock_irqrestore(&the_fl.spin_lock, flags);

	return HRTIMER_NORESTART;
}

int flashlight_control(int mode)
{
	int ret = 0;
	unsigned long flags;
	printk("[flashlight] function:%s , mode:%d \n",__func__,mode);
	

	spin_lock_irqsave(&the_fl.spin_lock, flags);

	/*if (!flashlight_exist) {
		ret = -EINVAL;
		goto done;
	}*/
	
	the_fl.brightness = mode;

	switch (mode) {
	case FLASHLIGHT_TORCH:
#if 0
		pr_info("%s: half\n", __func__);
		/* If we are transitioning from flash to torch, make sure to
		 * cancel the flash timeout timer, otherwise when it expires,
		 * the torch will go off as well.
		 */
		hrtimer_cancel(&the_fl.timer);
		flashlight_hw_command(2, 4);
#else
		gpio_direction_output(gpio_pin, gpio_act_level);
#endif

		break;

	case FLASHLIGHT_FLASH:
       	gpio_direction_output(gpio_pin, gpio_act_level);

		break;

	case FLASHLIGHT_OFF:
       	gpio_direction_output(gpio_pin, gpio_act_level ^ 0x1);
		break;

	default:
		pr_err("%s: unknown flash_light flags: %d\n", __func__, mode);
		ret = -EINVAL;
		goto done;
	}

done:
	spin_unlock_irqrestore(&the_fl.spin_lock, flags);
	return ret;
}
EXPORT_SYMBOL(flashlight_control);

static void fl_lcdev_brightness_set(struct led_classdev *led_cdev,
		enum led_brightness brightness)
{
	int level;

	switch (brightness) {
	case LED_HALF:
		level = FLASHLIGHT_TORCH;
		break;
	case LED_FULL:
		level = FLASHLIGHT_FLASH;
		break;
	case LED_OFF:
	default:
		level = FLASHLIGHT_OFF;
	};

	flashlight_control(level);
}

static void flashlight_early_suspend(struct early_suspend *handler)
{
	flashlight_control(FLASHLIGHT_OFF);
}

static int flashlight_init_gpio(struct flashlight_platform_data *fl_pdata,struct platform_device *pdev)
{
	int ret;
	enum of_gpio_flags flags;

	printk("[flashlight] function:%s\n",__func__);
    flashlight_exist = false;
	/*ret = gpio_get_pre_cfg(GPIO_NAME_FLASHLIGHT, &gpio_cfg);
	if (ret != 0) {
		goto out;
	}

	gpio_pin = ASOC_GPIO_PORT(gpio_cfg.iogroup, gpio_cfg.pin_num);*/
	
	gpio_pin = of_get_named_gpio_flags(pdev->dev.of_node, "flashlight-gpios", 0, &flags);
	printk("flashlight GPIO: %d \n",gpio_pin);
	gpio_act_level = !(flags & OF_GPIO_ACTIVE_LOW);
	ret = gpio_request(gpio_pin, GPIO_NAME_FLASHLIGHT);
    if (ret == 0) {
        gpio_direction_output(gpio_pin, gpio_act_level ^ 0x1);
        flashlight_exist = true;            
    } else {
		goto out;
    }

out:
	return ret;
}


static int flashlight_probe(struct platform_device *pdev)
{
	struct flashlight_platform_data *fl_pdata = pdev->dev.platform_data;
	int err = 0;
	printk("[flashlight] function:%s ,line:%d \n",__func__,__LINE__);
	err = flashlight_init_gpio(fl_pdata,pdev);
	if (err < 0) {
		pr_err("%s: setup GPIO failed\n", __func__);
		goto fail_free_mem;
	}
	spin_lock_init(&the_fl.spin_lock);
	the_fl.fl_lcdev.name = pdev->name;
	the_fl.fl_lcdev.brightness_set = fl_lcdev_brightness_set;
	the_fl.fl_lcdev.brightness = LED_OFF;

	err = led_classdev_register(&pdev->dev, &the_fl.fl_lcdev);
	if (err < 0) {
		pr_err("failed on led_classdev_register\n");
		goto fail_free_gpio;
	}

	hrtimer_init(&the_fl.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	the_fl.timer.function = flashlight_timeout;

#ifdef CONFIG_HAS_EARLYSUSPEND
	the_fl.early_suspend_flashlight.suspend = flashlight_early_suspend;
	the_fl.early_suspend_flashlight.resume = NULL;
	register_early_suspend(&the_fl.early_suspend_flashlight);
#endif

	return 0;

fail_free_gpio:
	if (gpio_pin)
		gpio_free(gpio_pin);
fail_free_mem:
	return err;
}

static int flashlight_remove(struct platform_device *pdev)
{

	hrtimer_cancel(&the_fl.timer);
	unregister_early_suspend(&the_fl.early_suspend_flashlight);
	flashlight_control(FLASHLIGHT_OFF);
	led_classdev_unregister(&the_fl.fl_lcdev);
	if (gpio_pin)
		gpio_free(gpio_pin);
	
	return 0;
}

static struct of_device_id flashlight_of_match[] = {
    { .compatible = "flashlight" },
    { }
};

static struct platform_driver flashlight_driver = {
	.driver	= {
		.name = "flashlight",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(flashlight_of_match),
	},
	.probe		= flashlight_probe,
	.remove		= flashlight_remove,
};

static int __init flashlight_init(void)
{
	int ret;

    printk("[flash light] %s version: %s, 2015-01-08\n", THIS_MODULE->name, THIS_MODULE->version);

	ret = platform_driver_register(&flashlight_driver);

	return ret;
}

static void __exit flashlight_exit(void)
{
	platform_driver_unregister(&flashlight_driver);
}

module_init(flashlight_init);
module_exit(flashlight_exit);

MODULE_AUTHOR("Actions Semi, Inc");
MODULE_DESCRIPTION("flash light driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
