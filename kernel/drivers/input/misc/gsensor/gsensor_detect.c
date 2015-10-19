/*
 * gsensor_detect.c  --  auto detect gsensor device
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
#include "gsensor_detect.h"

#define CFG_GSENSOR_USE_CONFIG 1
#define CFG_GSENSOR_ADAP_ID          "gsensor.i2c_adap_id"
#define CFG_GSENSOR_REGULATOR        "gsensor.regulator"
#define CFG_GSENSOR_DETECT_LIST      "gsensor_detect_list"

//default value
#define GSENSOR_I2C_ADAPTER     (2)
#define GSENSOR_DETECT_NAME        "gsensor_detect"

static int scan_start = 0;

static unsigned int gsensor_adapter = 0;
static char gsensor_regulator[32];

struct i2c_client    *client;
struct i2c_adapter *adap = NULL;
static struct regulator *regulator = NULL;
static struct kobject *kobj; 

static char export_ko_name[32];
static int export_ko_offset = 0;

//GSENSOR POWER
#define GSENSOR_POWER_ID        ("ldo5")
#define GSENSOR_POWER_MIN_VOL    (3300000)
#define GSENSOR_POWER_MAX_VOL    (3300000)   


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

#if CFG_GSENSOR_USE_CONFIG
static int gsensor_of_data_get(struct platform_device *pdev)
{
    int ret = -1;
    int idx = 0;
    struct gsensor_device *dev = NULL;
    char cfg_name[32];
    u32 cfg_detect = 0;
    struct device_node *of_node;
    struct device_node *list_node;
    
    of_node = pdev->dev.of_node;
    ret = of_property_read_u32(of_node, "i2c_adapter_id", &gsensor_adapter);
    if (ret == 0) {
        gsensor_adapter = GSENSOR_I2C_ADAPTER;
    }
    
    list_node = of_find_compatible_node(NULL, NULL, "gsensor_detect_list");
    
    /* get detect list config */
    for (idx=0; idx < ARRAY_SIZE(gsensor_device_list); idx++) {
        dev = &gsensor_device_list[idx];        
        sprintf(cfg_name, "%s",  dev->name);
        ret = of_property_read_u32(list_node, cfg_name, &cfg_detect);
        if (ret == 0) {
            dev->need_detect = (bool)cfg_detect;
        }
        printk("%s,%d, ctp_detect:%d\n", __func__, __LINE__, cfg_detect);
    }

    return ret;
}
#endif


int gsensor_init(void)
{
    int ret=0;    
    
    if(gsensor_regulator[0] != '\0') {
        regulator = regulator_init(gsensor_regulator, GSENSOR_POWER_MIN_VOL, GSENSOR_POWER_MAX_VOL);
        if ( !regulator ) {
            printk("gsensor init power failed");
            ret = -EINVAL;
        }
    }
    
    return ret;
}

int gsensor_deinit(void)
{
    int ret=0;
    
    if (regulator != NULL) {
        regulator_deinit(regulator);
    }
    
    return ret;
}

bool test_i2c(struct gsensor_device *dev)
{
    int ret = -1, retry = 0;
    uint8_t test_data[1] = { 0 };
    
    if (dev->need_detect == false) {
        //printk("Skip the gsensor:%s,no need detect.\n", dev->name);
        return false;
    }
    
    if (dev->has_chipid == false) {
        for (retry = 0; retry < 3; retry++) {
            client->addr = dev->i2c_addr;
            ret = i2c_master_send(client, test_data, 1);
            if (ret == 1)
                break;
                
            if (dev->has_sa0 == true) {
                client->addr = dev->i2c_addr + 1;
                ret = i2c_master_send(client, test_data, 1);
                if (ret == 1)
                    break;
            }            
            msleep(5);
        }
        
        if (ret == 1) {
            printk(KERN_ERR"Find u,u are:%s\n", dev->name);
            return true;
        }else{
            printk(KERN_ERR"Is not the gsensor:%s,skip it.\n",dev->name);
            return false;
        }
    } else {
        for (retry = 0; retry < 3; retry++) {
            client->addr = dev->i2c_addr;
            ret = i2c_smbus_read_byte_data(client, dev->chipid_reg);
            if (ret >= 0)
                break;
            
            if (dev->has_sa0 == true) {
                client->addr = dev->i2c_addr + 1;
                ret = i2c_smbus_read_byte_data(client, dev->chipid_reg);
                if (ret >= 0)
                    break;
            }       
            msleep(5);
        }
        printk("%s: read the chipid is: 0x%x\n", dev->name, ret);
        
        if (ret == dev->chipid[0] || ret == dev->chipid[1]) {
            printk("Find u,u are:%s\n",dev->name);
            return true;
        } else {
            printk("Is not the gsensor:%s,skip it.\n",dev->name);
            return false;
        }
    }

    return false;
}

int gsensor_detect(void)
{
    int idx = 0;
   
    client = kzalloc(sizeof *client, GFP_KERNEL);
    if (!client)
        return -1;
    
    adap = i2c_get_adapter(gsensor_adapter);
    if(!adap){
        printk(KERN_ERR"err:%s, %d.\n", __func__, __LINE__);
        return -1;
    }
    client->adapter = adap;
    if ( scan_start > 0 && scan_start < ARRAY_SIZE(gsensor_device_list) ) {
        if (true == test_i2c(&gsensor_device_list[scan_start])) {
            return scan_start;
        }
    }
    for(idx = 0; idx < ARRAY_SIZE(gsensor_device_list); idx++) {
        if (true == test_i2c(&gsensor_device_list[idx])) {
            return idx;
        }
    }
    return -1;
}

static ssize_t name_show(struct device *dev,  struct device_attribute *attr,  
        char *buf)  
{  
    return strlcpy(buf, export_ko_name, sizeof(export_ko_name)); 
}  

static ssize_t offset_show(struct device *dev,  struct device_attribute *attr,  
        char *buf)  
{  
    return sprintf(buf, "%d", export_ko_offset); 
}  

static DEVICE_ATTR(name, 0400, name_show, NULL); 
static DEVICE_ATTR(offset, 0400, offset_show, NULL); 

static int gsensor_detect_probe(struct platform_device *pdev)
{
    int ret = 0;

#if CFG_GSENSOR_USE_CONFIG
    gsensor_of_data_get(pdev);
#else
    gsensor_adapter = GSENSOR_I2C_ADAPTER;
    strcpy(gsensor_regulator, GSENSOR_POWER_ID);
#endif

    ret=gsensor_init();
    if(ret){
        printk(KERN_ERR"gsensor init failed.\n");
        goto exit;
    }
    
    kobj = kobject_create_and_add("gsensor_detect", NULL);  
    if (kobj == NULL) {  
        printk("kobject_create_and_add failed.\n"); 
        ret = -ENOMEM;  
        goto kobject_create_err;  
    }  
    
    ret = sysfs_create_file(kobj,&dev_attr_name.attr);  
    if (ret < 0){
        printk(KERN_ERR"sysfs_create_file failed.\n");
        goto sysfs_create_err;
    }

    ret = sysfs_create_file(kobj,&dev_attr_offset.attr);  
    if (ret < 0){
        printk(KERN_ERR"sysfs_create_file failed.\n");
        goto sysfs_create_err;
    }
    
    ret = gsensor_detect();
    if (ret < 0) {
        printk(KERN_ERR"Sorry,no one suit!!!\n");
    } else {
        strlcpy(export_ko_name, gsensor_device_list[ret].ko_name, strlen(gsensor_device_list[ret].ko_name)+1);
        
        if (gsensor_device_list[ret].has_chipid == true) {
            export_ko_offset = ret;
        } else {
            export_ko_offset = 0;
        }
    }
    gsensor_deinit();

    return 0;
    
sysfs_create_err:  
    kobject_del(kobj);
kobject_create_err:
    gsensor_deinit();
exit:
    return ret;
}

static int gsensor_detect_remove(struct platform_device *pdev)
{
    //printk("==gsensor_detect_remove==\n");
    sysfs_remove_file(kobj,&dev_attr_name.attr);
    kobject_del(kobj);
    kfree(client);
    return 0;
}

static struct of_device_id gsensor_detect_of_match[] = {
	{ .compatible = "owl-gsensor-detect" },
	{ }
};

static struct platform_driver detect_driver = {
    .driver = {
        .name = GSENSOR_DETECT_NAME,
        .owner = THIS_MODULE,
        .of_match_table	= of_match_ptr(gsensor_detect_of_match),
    },
    .probe = gsensor_detect_probe,
    .remove = gsensor_detect_remove,
};

#if 0
static void detect_device_release(struct device * dev)
{
    return;
}

static struct platform_device detect_device = {
    .name = GSENSOR_DETECT_NAME,
    .dev = {
        .release = detect_device_release,
    }
};
#endif

static int __init gsensor_detect_init(void)
{
    int ret;
    printk("==gsensor_detect_init==\n");

    //ret=platform_device_register(&detect_device);
    //if(ret) return ret;
    ret=platform_driver_register(&detect_driver);
    return ret;
}

static void __exit gsensor_detect_exit(void)
{
    printk("==gsensor_detect_exit==\n");
    
    platform_driver_unregister(&detect_driver);
    //platform_device_unregister(&detect_device);
    return;
}

module_init(gsensor_detect_init);
module_exit(gsensor_detect_exit);

module_param(scan_start, int, S_IRUSR);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GSENSOR auto detect driver");
MODULE_AUTHOR("Actions Semi, Inc");
MODULE_ALIAS("platform:gsensor_detect");

