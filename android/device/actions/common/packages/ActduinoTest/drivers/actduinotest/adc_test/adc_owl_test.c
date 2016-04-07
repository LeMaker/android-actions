/*
 * Asoc adc keypad driver
 *
 * Copyright (C) 2011 Actions Semiconductor, Inc
 * Author:  chenbo <chenbo@actions-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <asm/delay.h>
#include <asm/io.h>
#include <linux/fb.h>
#include <linux/hrtimer.h>
/*#include <mach/gl5203_gpio.h> */
#include <linux/mfd/atc260x/atc260x.h>
//#include <unistd.h>


#define ADC_TEST_MAJOR 163
#define ADC_TEST_NAME "adc_owl_test"
#define AUX0 0
#define AUX1 1
#define AUX2 2
#define AUX3 3
#define AUX4 4

#define REMCON 10
#define GETVAL 12


struct atc260x_dev *atc260x;
static int adc_channel_num=-1;
struct device *dev=NULL;

static int adc_test_open(struct inode * inode,struct file * file)
{

//	atc260x = atc260x_get_parent_dev(&pdev->dev);
	printk("success get atc260x  \n");

return 0;
}

static ssize_t adc_test_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret,tr_val;

printk("start get channle \n");


	switch (cmd){
	        case REMCON:
	            adc_channel_num =  atc260x_auxadc_find_chan(atc260x, "REMCON");
	            break;
				
	        case AUX0:  
	            adc_channel_num =  atc260x_auxadc_find_chan(atc260x, "AUX0");    
	            break;

	        case AUX1:
	            adc_channel_num =  atc260x_auxadc_find_chan(atc260x, "AUX1");       
	            break;

	        case AUX2:
	            adc_channel_num =  atc260x_auxadc_find_chan(atc260x, "AUX2");
	            break;

			case AUX3:
	            adc_channel_num =  atc260x_auxadc_find_chan(atc260x, "AUX3");
	            break;

			case AUX4:
	            adc_channel_num =  atc260x_auxadc_find_chan(atc260x, "AUX4");
	            break;

			case GETVAL:
				ret = atc260x_auxadc_get_translated(atc260x,adc_channel_num, &tr_val);
				if (ret) {
					dev_err(dev,
						"%s() failed to get raw value of auxadc channel #%u\n",
						__func__, adc_channel_num);
					return ret;
					}
				return tr_val;
				break;

	        default:
	            printk("%d cmd is not support\n",cmd);
	            ret = -1;
	            break;
	    }
					
				
		return adc_channel_num;
}

struct file_operations adc_test_fops = {
.owner = THIS_MODULE,
.open  = adc_test_open,
.unlocked_ioctl = adc_test_ioctl,
};
static struct class *adc_test_dev_class;
static struct device *adc_test_class_device;

static int atc260x_adc_test_probe(struct platform_device *pdev) {
	
	//dev=pdev->dev;
	int ret;
    printk(KERN_ERR "%s: ==adc_test init==\n",__FILE__);

    ret = register_chrdev(ADC_TEST_MAJOR, "adc_test", &adc_test_fops);  
    if (ret) { 
        goto out; 
    	}
  
    adc_test_dev_class = class_create(THIS_MODULE, ADC_TEST_NAME);  
    if (IS_ERR(adc_test_dev_class)) {  
        ret = PTR_ERR(adc_test_dev_class);  
        goto out_unreg_chrdev;  
    }
    
    adc_test_class_device = device_create(adc_test_dev_class, NULL, MKDEV(ADC_TEST_MAJOR, 0), NULL, ADC_TEST_NAME);
    if (IS_ERR(adc_test_class_device)) {
        ret = PTR_ERR(adc_test_class_device);
        goto out_unreg_class;
    }

	atc260x = atc260x_get_parent_dev(&pdev->dev);
	printk(KERN_ERR "%s:got atc260x==\n",__FILE__);

#ifdef ADC_TEST
    INIT_DELAYED_WORK(&adc_test_work, adc_test_worker);
    adc_test_workqueue = create_singlethread_workqueue("adc_test_workqueue");
    queue_delayed_work(adc_test_workqueue, &adc_test_work, 500);
#endif

	return 0;
out_unreg_class:  
    class_destroy(adc_test_dev_class);  
out_unreg_chrdev:  
    unregister_chrdev(ADC_TEST_MAJOR, ADC_TEST_NAME);  
out:  
    printk(KERN_ERR "%s: Driver Initialisation failed\n", __FILE__);  
    return ret;


	
}

static int atc260x_adc_test_remove(struct platform_device *pdev) {
	printk("adc test exit !!!\n");
    device_destroy(adc_test_dev_class,MKDEV(ADC_TEST_MAJOR, 0));  
    class_destroy(adc_test_dev_class);  
    unregister_chrdev(ADC_TEST_MAJOR,ADC_TEST_NAME); 
#ifdef ADC_TEST        
    cancel_delayed_work_sync(&adc_test_work);
    destroy_workqueue(adc_test_workqueue);
#endif

	return 0;
}


static int atc260x_adc_test_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	
	return 0;
}
static int atc260x_adc_test_resume(struct platform_device *pdev)
{
	
	return 0;
}


static const struct of_device_id atc260x_adcpwmtest_of_match[] = {
	 { .compatible = "actions,atc260c-adcpwmtest" },
	{}
};

static struct platform_driver atc260x_adc_test_driver = {
	.driver = {
		.name   = "atc260x-adcpwmtest",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(atc260x_adcpwmtest_of_match),
	},
	.probe = atc260x_adc_test_probe,
	.remove = atc260x_adc_test_remove,
	.suspend = atc260x_adc_test_suspend,
	.resume = atc260x_adc_test_resume,
};

static int __init adc_test_init(void) {	
	return platform_driver_register(&atc260x_adc_test_driver);
}
static void __exit adc_test_exit(void) {
    platform_driver_unregister(&atc260x_adc_test_driver);
}
module_init(adc_test_init);
module_exit(adc_test_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ActduinoTest adc test driver");
MODULE_AUTHOR("jiangjinzhang, jiangjinzhang@artekmicro.com");
MODULE_ALIAS("platform:ActduinoTest");