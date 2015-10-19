/*
 * ctp_detect.c  --  auto detect ctp device
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/gpio.h>
#include <asm/prom.h>
#include <mach/gpio.h>
#include <linux/of.h>
#include <linux/of_i2c.h>
#include <linux/of_gpio.h>

#include "ctp_detect.h"

#define CFG_TP_USE_CONFIG 1
//default value
#define CTP_RESET_PIN       OWL_GPIO_PORTB(3)
#define CTP_I2C_ADAPTER     (1)
#define CTP_DETECT_NAME		"ctp_detect"
#define CFG_CTP_DETECT_LIST "ctp_detect_list"
static int scan_start = 0;

struct ctp_cfg {
	unsigned int sirq;
	unsigned int gpio_reset;
	unsigned int i2cAddr;
	char const* regulator; 
	unsigned int vol_max;
	unsigned int vol_min;
}; 
static struct ctp_cfg ctp_cfg;
static unsigned ctp_adapter = CTP_I2C_ADAPTER;

struct i2c_client	*client;
struct i2c_adapter *adap;
struct delayed_work *ctp_detect_work;

static char export_ko_name[50];

static struct kobject *kobj; 

static int export_ko_offset = 0;
//CTP POWER
static struct regulator *tp_regulator = NULL;
#define CTP_POWER_ID		("ldo5")
#define CTP_POWER_MIN_VOL	(3300000)
#define CTP_POWER_MAX_VOL	(3300000)   

static struct regulator *regulator_init(const char *name, int minvol, int maxvol)
{
	struct regulator *power;
	int ret;

	power = regulator_get(NULL,name);
    	if (IS_ERR(power)) {
		printk("err,regulator_get fail\n!!!");
		return NULL;
	}
 
	if (regulator_set_voltage(power, minvol, maxvol)) {
        	printk("err,cannot set voltage\n!!!");
        	regulator_put(power);
		return NULL;
	}
	ret = regulator_enable(power);
	return (power);
}

static inline void regulator_deinit(struct regulator *power)
{
	regulator_disable(power);
	regulator_put(power);
}

#if CFG_TP_USE_CONFIG
static int ctp_of_data_get(struct platform_device *pdev)
{
    int ret = -1;
    int idx = 0;
    struct ctp_device *dev = NULL;
    char cfg_name[32];
    u32 cfg_detect = 0;
    enum of_gpio_flags flags;
    unsigned int scope[2];

    struct device_node *of_node;
    struct device_node *list_node;
    
    of_node = pdev->dev.of_node;

    /* load gpio info */
    if (!of_find_property(of_node, "rst_gpios", NULL)) {
        printk(KERN_ERR"<isp>err: no config gpios\n");
	ctp_cfg.gpio_reset = CTP_RESET_PIN;
    }else{
        ctp_cfg.gpio_reset = of_get_named_gpio_flags(of_node, "rst_gpios", 0, &flags);
    }

    /* load tp regulator */
    if (of_find_property(of_node, "tp_vcc", NULL)) {
        ret = of_property_read_string(of_node, "tp_vcc", &ctp_cfg.regulator);
        if (ret < 0) {
            printk(KERN_ERR"can not read tp_vcc power source\n");
        }

        if (of_property_read_u32_array(of_node, "vol_range", scope, 2)) {
            printk(KERN_ERR" failed to get voltage range\n");
        }
        ctp_cfg.vol_min=scope[0];
        ctp_cfg.vol_max=scope[1];
    }

    
    ret = of_property_read_u32(of_node, "adapter_id", &ctp_adapter);
    if (ret == 0) {
        ctp_adapter = CTP_I2C_ADAPTER;
    }
    
    list_node = of_find_compatible_node(NULL, NULL, "ctp_detect_list");
    
    /* get detect list config */
    for (idx=0; idx < ARRAY_SIZE(ctp_device_list); idx++) {
        dev = &ctp_device_list[idx];        

        sprintf(cfg_name, "%s",  dev->name);
        ret = of_property_read_u32(list_node, dev->name, &cfg_detect);
        if (ret == 0) {
            dev->need_detect = (bool)cfg_detect;
        }
        printk(KERN_ERR" ctp detect list: %s=%d, cfg_detect = %d\n", dev->name, dev->need_detect, cfg_detect);
    }

    return ret;
}
#endif


int ctp_init(void)
{
	int ret=0;
	printk("in %s.\n",__FUNCTION__);
	tp_regulator = regulator_init(CTP_POWER_ID, CTP_POWER_MIN_VOL, CTP_POWER_MAX_VOL);
	if ( !tp_regulator ) {
		printk("ctp init power failed");
		ret = -EINVAL;
		return ret;
	}

	ret=gpio_request(ctp_cfg.gpio_reset, CTP_DETECT_NAME);
	if(ret) return ret;
	
	gpio_direction_output(ctp_cfg.gpio_reset, 1);
	msleep(20); 
	gpio_direction_output(ctp_cfg.gpio_reset, 0);
	msleep(50); 
	gpio_direction_output(ctp_cfg.gpio_reset, 1);
	msleep(50); 

	return ret;
}

int ctp_deinit(void)
{
	int ret=0;
	printk("in %s.\n",__FUNCTION__);
	if ( tp_regulator )
		regulator_deinit(tp_regulator);

	gpio_free(ctp_cfg.gpio_reset);
	return ret;
}

bool test_i2c(struct ctp_device *dev)
{
	int ret=-1, retry=0;
	uint8_t test_data[1] = { 0 };
	
	if(dev->need_detect==false){
		printk("Skip the ctp:%s,no need detect.\n",dev->name);
		return false;
	}
//need_detect==true
	client->addr=dev->i2c_addr;
	if(dev->has_chipid==false){
		for(retry=0;retry<3;retry++){
			ret = i2c_master_send(client, test_data, 1);
			if(ret==1)
				break;
			msleep(5);
		}
		if(ret==1){
			printk("Find u,u are:%s\n",dev->name);
			return true;
		}
		else{
			printk("Is not the ctp:%s,skip it.\n",dev->name);
			return false;
		}
	}
	else if(dev->has_chipid==true){
		for(retry=0;retry<3;retry++){
			ret = i2c_smbus_read_byte_data(client,dev->chipid_req);
			if (ret >= 0)
				break;
			msleep(5);
		}
		printk("read the chipid is:0x%x\n",ret);
		if(ret==dev->chipid){
			printk("Find u,u are:%s\n",dev->name);
			return true;
		}
		else{
			printk("Is not the ctp:%s,skip it.\n",dev->name);
			return false;
		}
	}

	return false;
}

int ctp_detect(void)
{
	int idx=0;
	printk("%s,line:%d.\n",__func__,__LINE__);
	client = kzalloc(sizeof *client, GFP_KERNEL);
	if (!client)
		return -ENOMEM;
	
	adap = i2c_get_adapter(ctp_adapter);
	client->adapter = adap;
	
	if ( scan_start > 0 && scan_start < ARRAY_SIZE(ctp_device_list) ) {
        if (true == test_i2c(&ctp_device_list[scan_start])) {
            return scan_start;
        }
    }

	for(idx=0;idx<sizeof(ctp_device_list)/sizeof(struct ctp_device);idx++){
		if(true==test_i2c(&ctp_device_list[idx]))
			return idx;
	}

	return -1;
}

static ssize_t name_show(struct device *dev,  struct device_attribute *attr,  
		char *buf)  
{  
    return strlcpy(buf, export_ko_name,sizeof(export_ko_name)); 
}  
static ssize_t offset_show(struct device *dev,  struct device_attribute *attr,  
        char *buf)  
{  
    return sprintf(buf, "%d", export_ko_offset); 
}  

static DEVICE_ATTR(name,0400,name_show,NULL); 
static DEVICE_ATTR(offset, 0400, offset_show, NULL); 

static void ctp_monitor(struct work_struct *work)
{
	int ret=-1;
	ret=ctp_init();
	if(ret){
		printk("ctp init failed.\n");
		goto loop;
	}

	ret=ctp_detect();
	if(ret<0){
		printk("Sorry,no one suit!!!\n");
		goto deinit;
	}
	else{
		ctp_deinit();
		strlcpy(export_ko_name,ctp_device_list[ret].ko_name,strlen(ctp_device_list[ret].ko_name)+1);
		if (ctp_device_list[ret].has_chipid == true) {
			export_ko_offset = ret;
		} else {
			export_ko_offset = 0;
		}
		cancel_delayed_work_sync(ctp_detect_work);
		return;
	}
	
deinit:
	ctp_deinit();
loop:	
	/* reschedule for the next time */
    schedule_delayed_work(ctp_detect_work, 300);
}

static int ctp_detect_probe(struct platform_device *pdev)
{
	int ret = 0;
	printk("in %s.\n",__FUNCTION__);

#if CFG_TP_USE_CONFIG
    ctp_of_data_get(pdev);
#else
	ctp_cfg.gpio_reset = CTP_RESET_PIN;
	ctp_adapter = CTP_I2C_ADAPTER;
#endif

	kobj = kobject_create_and_add("ctp_detect", NULL);  
	if (kobj == NULL) {  
		printk("kobject_create_and_add failed.\n"); 
		ret = -ENOMEM;  
		goto err_kobject_create;
	} 
	
	ret = sysfs_create_file(kobj,&dev_attr_name.attr);  
	if (ret < 0){
		printk("sysfs_create_file name failed.\n");
		goto err_sysfs_create;
	}
		
	ret = sysfs_create_file(kobj,&dev_attr_offset.attr);  
	if (ret < 0){
		printk("sysfs_create_file offset failed.\n");
		goto err_sysfs_create;
	}
	ctp_detect_work = kzalloc(sizeof(struct delayed_work), GFP_KERNEL);
    if (ctp_detect_work == NULL)
        goto err_sysfs_create;
	INIT_DELAYED_WORK(ctp_detect_work, ctp_monitor);
    schedule_delayed_work(ctp_detect_work, 100);

	return 0;
	
err_sysfs_create:	
	kobject_del(kobj);
err_kobject_create:
	return ret;
}
static int ctp_detect_remove(struct platform_device *pdev)
{
	printk("in %s.\n",__FUNCTION__);

	sysfs_remove_file(kobj,&dev_attr_name.attr);
	kobject_del(kobj);
	kfree(client);
	kfree(ctp_detect_work);
	return 0;
}

static struct of_device_id ctp_detect_of_match[] = {
	{ .compatible = "owl-ctp-detect" },
	{ }
};

static struct platform_driver detect_driver = {
    .driver = {
        .name = CTP_DETECT_NAME,
        .owner = THIS_MODULE,
        .of_match_table	= of_match_ptr(ctp_detect_of_match),
    },
    .probe = ctp_detect_probe,
    .remove = ctp_detect_remove,
};

#if 0
static int ctp_detect_suspend(struct platform_device *pdev, pm_message_t m)
{
	printk("in %s.\n",__FUNCTION__);
	if(delayed_work_pending(ctp_detect_work)){
		cancel_delayed_work_sync(ctp_detect_work);
	}
	return 0;
}
static int ctp_detect_resume(struct platform_device *pdev)
{
	printk("in %s.\n",__FUNCTION__);
	schedule_delayed_work(ctp_detect_work, 100);
	return 0;
}
static struct platform_driver detect_driver = {
	.driver = {
		.name = CTP_DETECT_NAME,
		.owner = THIS_MODULE,
	},
	.probe = ctp_detect_probe,
	.remove = ctp_detect_remove,
	.suspend = ctp_detect_suspend,
	.resume = ctp_detect_resume,
};

static void detect_device_release(struct device * dev)
{
    return;
}
static struct platform_device detect_device = {
    .name = CTP_DETECT_NAME,
	.dev = {
        .release = detect_device_release,
    }
};
#endif
static int __init ctp_detect_init(void)
{
	int ret;
	printk("==ctp_detect_init==\n");

	//ret=platform_device_register(&detect_device);
	//if(ret) return ret;
	ret=platform_driver_register(&detect_driver);
	return ret;
}

static void __exit ctp_detect_exit(void)
{
	printk("==ctp_detect_exit==\n");
	
	platform_driver_unregister(&detect_driver);
	//platform_device_unregister(&detect_device);

	return;
}

module_init(ctp_detect_init);
module_exit(ctp_detect_exit);

module_param(scan_start, int, S_IRUSR);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("CTP auto detect driver");
MODULE_AUTHOR("Actions Semi, Inc");
MODULE_ALIAS("platform:ctp_detect");

