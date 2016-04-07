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
#define PWM_TEST_MAJOR 165
#define PWM_TEST_NAME "pwm_owl_test"
#define PWM0 0
#define PWM1 1
#define PWM2 2
#define PWM3 3
#define PWM4 4
#define PMU_PWM0 8

struct atc260x_pwm_dev {
	struct atc260x_dev *atc260x_dev;
};
struct atc260x_pwm_dev *atc260x_pwm;

static int pwm_test_open(struct inode * inode,struct file * file)
{
	printk("open success  \n");

	return 0;
}

static ssize_t pwm_test_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret,tr_val;

	printk("start config pwm = %d   arg = %d\n",cmd,arg);


	switch (cmd){

	        case PWM0:

	            break;

	        case PWM1:

	            break;

	        case PWM2:

	            break;

			case PWM3:

	            break;

			case PWM4:

	            break;

			case PMU_PWM0:
				printk("case PMU_PWM0\n");
				atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PMU_MUX_CTL0,0x0c00,0x0C00);
				atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWMCLK_CTL,0x3fff,0x0000);
				printk("ATC2603C_PWMCLK_CTL is %d \n",atc260x_reg_read(atc260x_pwm->atc260x_dev, ATC2603C_PWMCLK_CTL));
				//set clk
				atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWMCLK_CTL,0x0fff,0x0000);
				printk("ATC2603C_PWMCLK_CTL is %d \n",atc260x_reg_read(atc260x_pwm->atc260x_dev, ATC2603C_PWMCLK_CTL));
				//set period and duty
				atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWM0_CTL,0xffff,arg);
				printk("ATC2603C_PWM0_CTL is %d \n",atc260x_reg_read(atc260x_pwm->atc260x_dev, ATC2603C_PWM0_CTL));

				atc260x_set_bits(atc260x_pwm->atc260x_dev,ATC2603C_PWMCLK_CTL,0x3000,0x3000);
				printk("ATC2603C_PWMCLK_CTL is %d \n",atc260x_reg_read(atc260x_pwm->atc260x_dev, ATC2603C_PWMCLK_CTL));
				break;

	        default:
	            printk("%d cmd is not support\n",cmd);
	            ret = -1;
	            break;
	    }

		return 0;
}

struct file_operations pwm_test_fops = {
.owner = THIS_MODULE,
.open  = pwm_test_open,
.unlocked_ioctl = pwm_test_ioctl,
};
static struct class *pwm_test_dev_class;
static struct device *pwm_test_class_device;


static int atc260x_pwm_test_probe(struct platform_device *pdev)
{
	 printk(KERN_ERR "%s: ==pwm_test init==\n",__FILE__);
	struct atc260x_dev *atc260x_dev = dev_get_drvdata(pdev->dev.parent);
	printk(KERN_ERR "%s:got atc260x==%d\n",__FILE__,atc260x_dev);
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

    ret = register_chrdev(PWM_TEST_MAJOR, "pwm_test", &pwm_test_fops);
    if (ret) {
        goto out;
    	}

    pwm_test_dev_class = class_create(THIS_MODULE, PWM_TEST_NAME);
    if (IS_ERR(pwm_test_dev_class)) {
        ret = PTR_ERR(pwm_test_dev_class);
        goto out_unreg_chrdev;
    }

    pwm_test_class_device = device_create(pwm_test_dev_class, NULL, MKDEV(PWM_TEST_MAJOR, 0), NULL, PWM_TEST_NAME);
    if (IS_ERR(pwm_test_class_device)) {
        ret = PTR_ERR(pwm_test_class_device);
        goto out_unreg_class;
    }


#ifdef PWM_TEST
    INIT_DELAYED_WORK(&pwm_test_work, pwm_test_worker);
    pwm_test_workqueue = create_singlethread_workqueue("pwm_test_workqueue");
    queue_delayed_work(pwm_test_workqueue, &pwm_test_work, 500);
#endif
	return 0;
out_unreg_class:
    class_destroy(pwm_test_dev_class);
out_unreg_chrdev:
    unregister_chrdev(PWM_TEST_MAJOR, PWM_TEST_NAME);
out:
    printk(KERN_ERR "%s: Driver Initialisation failed\n", __FILE__);
    return ret;


}

static int atc260x_pwm_test_remove(struct platform_device *pdev)
{
	printk("pwm test exit !!!\n");
    device_destroy(pwm_test_dev_class,MKDEV(PWM_TEST_MAJOR, 0));
    class_destroy(pwm_test_dev_class);
    unregister_chrdev(PWM_TEST_MAJOR,PWM_TEST_NAME);
#ifdef PWM_TEST
    cancel_delayed_work_sync(&pwm_test_work);
    destroy_workqueue(pwm_test_workqueue);
#endif
  struct atc260x_pwm_dev *atc260x_pwm = platform_get_drvdata(pdev);
  ATC260X_PWM_INFO("[%s start]\n",__func__);

	platform_set_drvdata(pdev, NULL);
	kfree(atc260x_pwm);

	ATC260X_PWM_INFO("[%s finished]\n", __func__);
	return 0;
}



static int atc260x_pwm_test_suspend(struct device *dev)
{
	return 0;
}

static int atc260x_pwm_test_resume(struct device *dev)
{
	return 0;
}

static const struct of_device_id atc260x_pwmadctest_of_match[] = {
        {.compatible = "actions,atc260c-pwmadctest",},
        {}
};

static struct platform_driver atc260x_pwm_test_driver = {
        .driver         = {
                .name   = "atc260x-pwmadctest",
                .owner  = THIS_MODULE,
                .of_match_table = of_match_ptr(atc260x_pwmadctest_of_match),
        },
	.probe		= atc260x_pwm_test_probe,
	.remove		= atc260x_pwm_test_remove,
	.suspend = atc260x_pwm_test_suspend,
	.resume = atc260x_pwm_test_resume,
};

static int __init pwm_test_init(void)
{
	return platform_driver_register(&atc260x_pwm_test_driver);
}

static void __exit pwm_test_exit(void)
{
	platform_driver_unregister(&atc260x_pwm_test_driver);
}

module_init(pwm_test_init);
module_exit(pwm_test_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ActduinoTest pwm test driver");
MODULE_AUTHOR("jiangjinzhang, jiangjinzhang@artekmicro.com");
MODULE_ALIAS("platform:ActduinoTest");
