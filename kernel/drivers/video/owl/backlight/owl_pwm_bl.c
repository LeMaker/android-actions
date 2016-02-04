/*
 * linux/drivers/video/backlight/pwm_bl.c
 *
 * simple PWM based backlight control, board code has to setup
 * 1) pin configuration so PWM waveforms can output
 * 2) platform_data casts to the PWM id (0/1/2/3 on PXA)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <mach/hardware.h>
#include <mach/pwm-owl.h>

#include <linux/mfd/atc260x/atc260x.h>

#if 0
#define BL_PRINT(fmt, args...) printk(KERN_INFO fmt, ##args)
#else
#define BL_PRINT(fmt, args...)
#endif

struct pwm_en_gpio {
    int             gpio;
    int             active_low;
};

struct owl_pwm_bl_platform_data {
    int             pwm_id;
    unsigned int    total_steps;
    unsigned int    max_brightness;
    unsigned int    min_brightness;
    unsigned int    dft_brightness;
    unsigned int    pwm_period_ns;
    unsigned int    polarity_inversed;

    u32             delay_before_pwm;
    u32             delay_after_pwm;

    struct pwm_en_gpio en_gpio;

    int (*init) (struct device *dev);
    int (*notify) (int brightness);
    void (*exit) (struct device *dev);
};

/*
 * period,          PWM period in ns
 * total_steps,     the total brightness levels in ns, max brightness
 * threshold_step,  the start step in ns, another word, min brightness
 * duty(ns) = brightness / total steps *  period
 */
struct owl_pwm_bl {
    struct pwm_device   *pwm;
    unsigned int        period;
    unsigned int        total_steps;
    unsigned int        threshold_step;
    unsigned int        polarity_inversed;
    int                 (*notify)(int brightness);

    struct pwm_en_gpio  en_gpio;

    int                 dev_enable;

    u32                 delay_before_pwm;
    u32                 delay_after_pwm;
	bool 				need_update_bl;
};

static struct backlight_device *owl_pwm_bl_device = NULL;

static int owl_pwm_bl_update_status(struct backlight_device *bl) {
    struct owl_pwm_bl *pb = dev_get_drvdata(&bl->dev);

    int brightness      = bl->props.brightness;
    int total_steps     = pb->total_steps;

    int old_dev_enable  = pb->dev_enable;

    /* the percent of backlight, used to adjust charging cuurent */
    int brightness_percent;

    BL_PRINT("owl pwm bl update status\n");

    if (bl->props.power != FB_BLANK_UNBLANK
        || bl->props.fb_blank != FB_BLANK_UNBLANK) {
        brightness = 0;
		pb->need_update_bl = 1;
    }

    /* modified for the threshold_step */
    if (brightness > 0) {
        brightness += pb->threshold_step;
    }

    if (pb->notify) {
        BL_PRINT("brightness from  notify\n");
        brightness = pb->notify(brightness);
    }

    BL_PRINT("bl->props.power = %d\n", bl->props.power);
    BL_PRINT("bl->props.fb_blank = %d\n", bl->props.fb_blank);
    BL_PRINT("pb->polarity_inversed = %d\n", pb->polarity_inversed);

    if (brightness > total_steps) {
        return -EINVAL;
    }

    /*
     * adjust the charger current according to brightness
     */
    if (brightness > 0) {
        brightness_percent = bl->props.brightness * 100 / bl->props.max_brightness;
    } else {
        brightness_percent = 0;
    }
    config_inner_charger_current(DEV_CHARGER_PRE_CONFIG,
                                 DEV_CHARGER_CURRENT_LCD,
                                 brightness_percent);

    if (brightness == 0) {
        if (old_dev_enable == 0) {
            goto config_out;
        }

        BL_PRINT("disable device, turn off backlight\n");

        pb->dev_enable = 0;

        if (gpio_is_valid(pb->en_gpio.gpio)) {
            gpio_set_value(pb->en_gpio.gpio, pb->en_gpio.active_low);
        }

        /*
         * First, ic's bug, backlight cannot be closed
         * when polarity=1, duty=0.
         * Second, pwm framework refuse to call
         * pwm_set_polarity when pwm is enabled.
         */
        pb->pwm->chip->ops->set_polarity(pb->pwm->chip,
                                         pb->pwm,
                                         !pb->polarity_inversed);
        pwm_config(pb->pwm, pb->period, pb->period);
        msleep_interruptible(10);
        pwm_disable(pb->pwm);
    } else {
        BL_PRINT("turn on backlight\n");
        BL_PRINT("duty_ns = %x\n", brightness * pb->period / total_steps);
        BL_PRINT("period_ns = %x\n", pb->period);

        pwm_set_polarity(pb->pwm, pb->polarity_inversed);
        pwm_config(pb->pwm, brightness * pb->period / total_steps,
                   pb->period);
        pwm_enable(pb->pwm);

        if (old_dev_enable == 0) {
            if (gpio_is_valid(pb->en_gpio.gpio)) {
                if (pb->delay_after_pwm) {
                    BL_PRINT("delay %d ms after pwm cfg\n", pb->delay_after_pwm);
                    msleep_interruptible(pb->delay_after_pwm);
                }
                gpio_set_value(pb->en_gpio.gpio, !pb->en_gpio.active_low);
            }
        }

        pb->dev_enable = 1;
    }

config_out:
    config_inner_charger_current(DEV_CHARGER_POST_CONFIG,
            DEV_CHARGER_CURRENT_LCD,
            brightness_percent);

    return 0;
}

static int owl_pwm_bl_get_brightness(struct backlight_device *bl) {
    return bl->props.brightness;
}

static int owl_pwm_bl_check_fb(struct backlight_device *bl,
                                  struct fb_info *fbi) {
    return 0;
}

static const struct backlight_ops owl_pwm_bl_ops = {
    .update_status  = owl_pwm_bl_update_status,
    .get_brightness = owl_pwm_bl_get_brightness,
    .check_fb       = owl_pwm_bl_check_fb,
};

static int of_parse_pwm_gpio(struct device_node *of_node,
                             const char *propname,
                             struct pwm_en_gpio *gpio) {
    enum of_gpio_flags flags;
    int gpio_num;

    gpio_num = of_get_named_gpio_flags(of_node, propname, 0, &flags);
    if (gpio_num >= 0) {
        gpio->gpio = gpio_num;
    } else {
        gpio->gpio = -1;
    }

    gpio->active_low = flags & OF_GPIO_ACTIVE_LOW;

    BL_PRINT("%s, gpio = %d\n", __func__, gpio->gpio);
    BL_PRINT("%s, active low = %d\n", __func__, gpio->active_low);
    return 0;
}

static int owl_pwm_bl_of_data_get(
        struct platform_device *pdev,
        struct owl_pwm_bl_platform_data *data) {
    struct device_node *of_node;

    memset(data, 0, sizeof(struct owl_pwm_bl_platform_data));

    of_node = pdev->dev.of_node;

    if (of_property_read_u32(of_node,
        "total_steps", &data->total_steps)) {
        return -EINVAL;
    }

    if (of_property_read_u32(of_node,
        "max_brightness", &data->max_brightness)) {
        return -EINVAL;
    }

    if (of_property_read_u32(of_node,
        "min_brightness", &data->min_brightness)) {
        return -EINVAL;
    }

    if (of_property_read_u32(of_node,
        "dft_brightness", &data->dft_brightness)) {
        return -EINVAL;
    }

    if (of_property_read_u32(of_node,
        "delay_bf_pwm", &data->delay_before_pwm)) {
        data->delay_before_pwm = 0;
    }

    if (of_property_read_u32(of_node,
        "delay_af_pwm", &data->delay_after_pwm)) {
        data->delay_after_pwm = 0;
    }

    of_parse_pwm_gpio(of_node, "backlight_en_gpios", &data->en_gpio);

    return 0;
}


static bool boot_pwm_inited;

static inline bool is_pwm_enabled(int hwpwm) {
    /* 
     * CMU_DEVCLKEN1 is 0xB0160070, FIXME
     * return !!(act_readl(CMU_PWM0CLK + 4 * hwpwm));
     */
    return !!(readl(ioremap(0xB0160070 + 4 * hwpwm, 4)));
}

static void check_boot_pwm_inited(int hwpwm) {
    /* 
     * CMU_DEVCLKEN1 is 0xB01600A4, FIXME
     * boot_pwm_inited = (act_readl(CMU_DEVCLKEN1) & (1 << (23 + hwpwm)));
     */
    boot_pwm_inited = (readl(ioremap(0xB01600A4, 4)) & (1 << (23 + hwpwm)));

    if (boot_pwm_inited) {
        boot_pwm_inited = (boot_pwm_inited && is_pwm_enabled(hwpwm));
    }

    BL_PRINT("PWM%d INITED FROM UBOOT??  %d\n", hwpwm, boot_pwm_inited);

    return ;
}

static ssize_t show_total_steps(struct device *device,
                                struct device_attribute *attr,
                                char *buf) {
    struct owl_pwm_bl *pb = dev_get_drvdata(device);

    return snprintf(buf, PAGE_SIZE, "%d\n", pb->total_steps);
}

static ssize_t show_threshold_step(struct device *device,
                                   struct device_attribute *attr,
                                   char *buf) {
    struct owl_pwm_bl *pb = dev_get_drvdata(device);

    return snprintf(buf, PAGE_SIZE, "%d\n", pb->threshold_step);
}

static struct device_attribute owl_pwm_bl_attrs[] = {
    __ATTR(total_steps, S_IRUGO, show_total_steps, NULL),
    __ATTR(threshold_step, S_IRUGO, show_threshold_step, NULL),
};

int owl_pwm_bl_init_attr(struct device *dev) {
    int i, error = 0;

    BL_PRINT("init att\n");
    for (i = 0; i < ARRAY_SIZE(owl_pwm_bl_attrs); i++) {
        error = device_create_file(dev, &owl_pwm_bl_attrs[i]);

        if (error) {
            BL_PRINT("init attr err\n");
            break;
        }
    }

    if (error) {
        while (--i >= 0) {
            device_remove_file(dev, &owl_pwm_bl_attrs[i]);
        }
    }
    return 0;
}

int owl_pwm_bl_remove_attr(struct device *dev) {
    /*TODO*/
    return 0;
}

static int owl_pwm_bl_probe(struct platform_device *pdev) {
    struct owl_pwm_bl_platform_data  *data = pdev->dev.platform_data;
    struct owl_pwm_bl_platform_data  of_data;
    struct backlight_device             *bl;
    struct backlight_properties         props;
    struct owl_pwm_bl                *pb;
    int                                 ret;

    /* has the Open Firmare data from DT */
    int                                 has_of_data = 0;

    printk("%s: name = %s\n", __func__, pdev->name);

    BL_PRINT("%s: get platform data\n", __func__);
    if (!data) {
        /* pdev has no platform data, get them from DT */
        ret = owl_pwm_bl_of_data_get(pdev, &of_data);
        if (ret < 0) {
            dev_err(&pdev->dev, "failed to find platform data\n");
            return ret;
        }

        data        = &of_data;
        has_of_data = 1;
    }

    BL_PRINT("%s: data->init\n", __func__);
    if (data->init) {
        if ((ret = data->init(&pdev->dev)) < 0) {
            return ret;
        }
    }

    BL_PRINT("%s: alloc pb\n", __func__);
    pb = kzalloc(sizeof(*pb), GFP_KERNEL);
    if (!pb) {
        dev_err(&pdev->dev, "no memory for state\n");
        ret = -ENOMEM;
        goto err_alloc;
    }

    BL_PRINT("%s: init pb\n", __func__);

    pb->total_steps     = data->total_steps;
    pb->threshold_step  = data->min_brightness;
    pb->notify          = data->notify;
	pb->need_update_bl = 0;

    BL_PRINT("%s: get pwm device\n", __func__);
    pb->pwm = devm_pwm_get(&pdev->dev, NULL);
    if (IS_ERR(pb->pwm)) {
        dev_err(&pdev->dev, "unable to request PWM, trying legacy API\n");

        pb->pwm = pwm_request(data->pwm_id, "pwm-backlight");
        if (IS_ERR(pb->pwm)) {
            dev_err(&pdev->dev, "unable to request legacy PWM\n");
            ret = PTR_ERR(pb->pwm);
            goto err_pwm;
        }
    }

    pb->delay_before_pwm    = data->delay_before_pwm;
    pb->delay_after_pwm     = data->delay_after_pwm;
    pb->en_gpio.gpio        = data->en_gpio.gpio;
    pb->en_gpio.active_low  = data->en_gpio.active_low;

    gpio_request(pb->en_gpio.gpio, NULL);

    check_boot_pwm_inited(pb->pwm->hwpwm);

    BL_PRINT("%s: get period\n", __func__);

    pb->polarity_inversed = data->polarity_inversed;
    /*
     * The DT case will set the pwm_period_ns field to 0 and store the
     * period, parsed from the DT, in the PWM device. For the non-DT case,
     * set the period from platform data.
     */
    if (has_of_data) {
        pb->polarity_inversed = owl_pwm_get_polarity(pb->pwm);
    } else {
        pwm_set_period(pb->pwm, data->pwm_period_ns);
    }
    pb->period = pwm_get_period(pb->pwm);

    BL_PRINT("%s: register backlight\n", __func__);

    memset(&props, 0, sizeof(props));
    props.type              = BACKLIGHT_RAW;
    props.max_brightness    = data->max_brightness - pb->threshold_step;
    bl = backlight_device_register(pdev->name, &pdev->dev, pb,
                                   &owl_pwm_bl_ops, &props);
    if (IS_ERR(bl)) {
        dev_err(&pdev->dev, "failed to register backlight\n");
        ret = PTR_ERR(bl);
        goto err_bl;
    }

    BL_PRINT("%s: set default brightess\n", __func__);

    bl->props.brightness = data->dft_brightness - pb->threshold_step;

    if (boot_pwm_inited) {
        gpio_direction_output(pb->en_gpio.gpio, !pb->en_gpio.active_low);

        bl->props.power = FB_BLANK_UNBLANK;
        pb->dev_enable  = 1;
    } else {
        gpio_direction_output(pb->en_gpio.gpio, pb->en_gpio.active_low);

        bl->props.power = FB_BLANK_POWERDOWN;
        pb->dev_enable = 0;
    }

    owl_pwm_bl_device = bl;
    platform_set_drvdata(pdev, bl);

    owl_pwm_bl_init_attr(&bl->dev);

    return 0;

err_bl:
    gpio_free(pb->en_gpio.gpio);
    pwm_free(pb->pwm);
err_pwm:
    kfree(pb);
err_alloc:
    if (data->exit) {
        data->exit(&pdev->dev);
    }
    return ret;
}

static int owl_pwm_bl_remove(struct platform_device *pdev) {
    struct owl_pwm_bl_platform_data *data = pdev->dev.platform_data;
    struct backlight_device             *bl = platform_get_drvdata(pdev);
    struct owl_pwm_bl                *pb = dev_get_drvdata(&bl->dev);

    owl_pwm_bl_device = NULL;

    backlight_device_unregister(bl);
    gpio_free(pb->en_gpio.gpio);
    pwm_config(pb->pwm, 0, pb->period);
    pwm_disable(pb->pwm);
    pwm_free(pb->pwm);
    kfree(pb);

    if (data->exit) {
        data->exit(&pdev->dev);
    }
    return 0;
}

#ifdef CONFIG_PM
static int owl_pwm_bl_suspend(struct platform_device *pdev,
                                 pm_message_t state) {
#if 0
    struct backlight_device *bl = platform_get_drvdata(pdev);
    struct owl_pwm_bl *pb = dev_get_drvdata(&bl->dev);

    bl->props.power = FB_BLANK_POWERDOWN;
    backlight_update_status(bl);
#endif
    return 0;
}

static int owl_pwm_bl_resume(struct platform_device *pdev) {
#if 0
    struct backlight_device *bl = platform_get_drvdata(pdev);

    bl->props.power = FB_BLANK_UNBLANK;
    backlight_update_status(bl);
#endif
    return 0;
}
#else
#define owl_pwm_bl_suspend    NULL
#define owl_pwm_bl_resume    NULL
#endif

static struct of_device_id owl_pwm_bl_of_match[] = {
    { .compatible = "actions,owl-pwm-backlight" },
    { }
};

static struct platform_driver owl_pwm_bl_driver = {
    .driver = {
        .name   = "owl_pwm_backlight",
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(owl_pwm_bl_of_match),
    },
    .probe      = owl_pwm_bl_probe,
    .remove     = owl_pwm_bl_remove,
    .suspend    = owl_pwm_bl_suspend,
    .resume     = owl_pwm_bl_resume,
};

static int __init owl_pwm_bl_init(void) {
    return platform_driver_register(&owl_pwm_bl_driver);
}

#ifdef MODULE
module_init(owl_pwm_bl_init);

static void __exit owl_pwm_bl_exit(void) {
    platform_driver_unregister(&owl_pwm_bl_driver);
}
module_exit(owl_pwm_bl_exit);

MODULE_DESCRIPTION("OWL PWM based Backlight Driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform: owl_pwm_backlight");
#else
subsys_initcall(owl_pwm_bl_init);
#endif


/*
 * return backlight's on/off status,
 * 0 is off, > 0 is on
 */
int owl_backlight_is_on(void) {
    int stat, bright, power;
    struct backlight_device *bl = owl_pwm_bl_device;

    if (NULL == bl) {
        BL_PRINT("%s/%s: get backlight failed!\n", __FILE__, __func__);
        return 1;
    }

    power   = bl->props.power;
    bright  = bl->props.brightness;

    stat    = bright && (!power);

    BL_PRINT("%s: state = %d\n", __func__, stat);

    return stat;
}
EXPORT_SYMBOL(owl_backlight_is_on);

/*
 * set backlight on/off, 0 is off, 1 is on.
 *
 * NOTE: this interface SHOULD ONLY change the on/off
 *       status, CAN NOT change the brightness value.
 */
static void owl_turnon_backlight_function(struct work_struct *work)
{
    struct backlight_device *bl = owl_pwm_bl_device;
	struct owl_pwm_bl *pb = dev_get_drvdata(&bl->dev);
    bl->props.power = FB_BLANK_UNBLANK;	
	if(pb->need_update_bl){
		BL_PRINT("owl_turnon_backlight_function\n");
		backlight_update_status(owl_pwm_bl_device);	
		pb->need_update_bl = 0;
	}
    BL_PRINT("backlight turn on \n"); 
}

static DECLARE_DELAYED_WORK(owl_turnon_backlight_work, owl_turnon_backlight_function);

void owl_backlight_set_onoff(int onoff) {
    struct backlight_device *bl = owl_pwm_bl_device;
	struct owl_pwm_bl *pb = NULL;
    if (NULL == bl) {
        printk("ERROR! backlight is no exist!\n");
    }
    pb = dev_get_drvdata(&bl->dev)

    BL_PRINT("%s: onoff = %d\n", __func__, onoff); 

    /* 
     * do not update status for backlight on.
     */
    if (onoff == 0)
    {
    	bl->props.power =  FB_BLANK_POWERDOWN;
    	backlight_update_status(owl_pwm_bl_device);
    }
    else    	
    {
    	schedule_delayed_work(&owl_turnon_backlight_work, msecs_to_jiffies(pb->delay_before_pwm));
    }

    return;
}
EXPORT_SYMBOL(owl_backlight_set_onoff);
