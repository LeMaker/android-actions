/*
 * Asoc  ir driver
 *
 * Copyright (C) 2011 Actions Semiconductor, Inc
 * Author:	chenbo <chenbo@actions-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/mfd/atc260x/atc260x.h>
#include <linux/time.h>


#define ATC260X_PWM_INFO(fmt, args...)	printk(KERN_INFO "atc260x_pwm_drv: " fmt, ##args)

struct atc260x_pwm_dev {
	struct atc260x_dev *atc260x_dev;
};

void atc260x_pwm_reg_print(struct atc260x_pwm_dev *atc260x_pwm)
{
	ATC260X_PWM_INFO("\n\nfollowing list all atc260x pwm register's value!\n");
	ATC260X_PWM_INFO("register ATC2603C_PMU_MUX_CTL0(0x%08x) value is 0x%08x\n",
		ATC2603C_PMU_MUX_CTL0, atc260x_reg_read(atc260x_pwm->atc260x_dev,ATC2603C_PMU_MUX_CTL0));
	ATC260X_PWM_INFO("register ATC2603C_PWMCLK_CTL(0x%08x) value is 0x%08x\n",
		ATC2603C_PWMCLK_CTL, atc260x_reg_read(atc260x_pwm->atc260x_dev,ATC2603C_PWMCLK_CTL));
	ATC260X_PWM_INFO("register ATC2603C_PWM0_CTL(0x%08x) value is 0x%08x\n",
		ATC2603C_PWM0_CTL, atc260x_reg_read(atc260x_pwm->atc260x_dev,ATC2603C_PWM0_CTL));
	ATC260X_PWM_INFO("register ATC2603C_PWM1_CTL(0x%08x) value is 0x%08x\n",
		ATC2603C_PWM1_CTL, atc260x_reg_read(atc260x_pwm->atc260x_dev,ATC2603C_PWM1_CTL));
}

static void atc260x_pwm_on(struct atc260x_pwm_dev *atc260x_pwm)
{
	ATC260X_PWM_INFO("[%s start]\n",__func__);

	//set SGPIO4 sgpio out
	atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PMU_MUX_CTL0,0x0c00,0x0000);
	//atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWMCLK_CTL,0x3000,0x0010);
	//atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWMCLK_CTL,0x0fff,0x0010);
  //atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWM0_CTL,0xffff,0xff00);
  //atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWMCLK_CTL,0x3000,0x3010);	
  atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PMU_SGPIO_CTL3,0x2000,0x2000);
  atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PMU_SGPIO_CTL4,0x0010,0x0010);  

	ATC260X_PWM_INFO("[%s finished]\n",__func__);
}

static void atc260x_pwm_hold(struct atc260x_pwm_dev *atc260x_pwm)
{

	ATC260X_PWM_INFO("[%s start]\n",__func__);

	//set SGPIO4 pwm0
	atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PMU_MUX_CTL0,0x0c00,0x0C00);
	
	atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWMCLK_CTL,0x3000,0x0010);
	atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWMCLK_CTL,0x0fff,0x0010);
  atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWM0_CTL,0xffff,0xffff);
  atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWMCLK_CTL,0x3000,0x3010);	

	ATC260X_PWM_INFO("[%s finished]\n",__func__);
}

static void atc260x_pwm_stop(struct atc260x_pwm_dev *atc260x_pwm)
{

	ATC260X_PWM_INFO("[%s start]\n",__func__);

	atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PMU_MUX_CTL0,0x0c00,0x0000);
	atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PMU_SGPIO_CTL3,0x2000,0x0000);

	atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWMCLK_CTL,0xffff,0x0000);
  atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWM0_CTL,0xffff,0x0000);

	ATC260X_PWM_INFO("[%s finished]\n",__func__);
}

static int atc260x_pwm_probe(struct platform_device *pdev)
{
	struct atc260x_dev *atc260x_dev = dev_get_drvdata(pdev->dev.parent);
	struct atc260x_pwm_dev *atc260x_pwm;
	struct device_node *np;
	int ret = 0;

	dev_info(&pdev->dev, "atc260x_pwm Probing...\n");
	np = pdev->dev.of_node;

	atc260x_pwm = kzalloc(sizeof(struct atc260x_pwm_dev), GFP_KERNEL);
	if (atc260x_pwm == NULL) {
		dev_err(&pdev->dev, "failed to allocate atc260x_pwm driver data\n");
		ret = -ENOMEM;
		return ret;
	}
	platform_set_drvdata(pdev, atc260x_pwm);

	atc260x_pwm->atc260x_dev = atc260x_dev;

	/*undo for dts*/
	
	atc260x_pwm_on(atc260x_pwm);
	return 0;
}

static int atc260x_pwm_remove(struct platform_device *pdev)
{
  struct atc260x_pwm_dev *atc260x_pwm = platform_get_drvdata(pdev);
  ATC260X_PWM_INFO("[%s start]\n",__func__);

	platform_set_drvdata(pdev, NULL);
	kfree(atc260x_pwm);
    
	ATC260X_PWM_INFO("[%s finished]\n", __func__);
	return 0;
}

static void atc260x_pwm_shutdown(struct platform_device *pdev){
	struct atc260x_pwm_dev *atc260x_pwm = platform_get_drvdata(pdev);
	ATC260X_PWM_INFO("[%s start]\n",__func__);

	atc260x_pwm_stop(atc260x_pwm);
    
	ATC260X_PWM_INFO("[%s finished]\n", __func__);
}

static int atc260x_pwm_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct atc260x_pwm_dev *atc260x_pwm = platform_get_drvdata(pdev);
	atc260x_pwm_hold(atc260x_pwm);

  ATC260X_PWM_INFO("[%s finished]\n", __func__);
	return 0;
}

static int atc260x_pwm_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct atc260x_pwm_dev *atc260x_pwm = platform_get_drvdata(pdev);
	atc260x_pwm_on(atc260x_pwm);
	
  ATC260X_PWM_INFO("[%s finished]\n", __func__);
	return 0;
}

static const struct dev_pm_ops s_atc260x_pwm_pm_ops = {
	.suspend        = atc260x_pwm_suspend,
	.resume	        = atc260x_pwm_resume,
};

static const struct of_device_id atc260x_pwm_of_match[] = {
        {.compatible = "actions,atc2603a-pwm",},
        {.compatible = "actions,atc2603c-pwm",},
        {.compatible = "actions,atc2609a-pwm",},
        {}
};
MODULE_DEVICE_TABLE(of, atc260x_pwm_of_match);

static struct platform_driver atc260x_pwm_driver = {
        .driver         = {
                .name   = "atc260x-pwm",
                .owner  = THIS_MODULE,
                .pm     = &s_atc260x_pwm_pm_ops,
                .of_match_table = of_match_ptr(atc260x_pwm_of_match),
        },
	.probe		= atc260x_pwm_probe,
	.remove		= atc260x_pwm_remove,
  .shutdown = atc260x_pwm_shutdown,
};

static int __init atc260x_pwm_init(void)
{
	int ret;
	ret=platform_driver_register(&atc260x_pwm_driver);
	return ret;
}

static void __exit atc260x_pwm_exit(void)
{
	platform_driver_unregister(&atc260x_pwm_driver);
}

module_init(atc260x_pwm_init);
module_exit(atc260x_pwm_exit);

MODULE_DESCRIPTION("ATC260X PWM driver");
MODULE_AUTHOR("Actions Semi, Inc");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:atc260x-pwm");
