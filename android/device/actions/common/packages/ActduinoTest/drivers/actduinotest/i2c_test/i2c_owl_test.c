/*
 * drivers/i2c/i2c_owl_dev.c
 *
 * Copyright (c) 2012 ShenZhen artekmicro
 *	Leo Zhang <leo.zhang@artekmicro.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/pm_runtime.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>

//#define I2CDEV_TEST
//#define UNKNOWN_DEV_ADDR

#define FM24C256
#ifdef  FM24C256
#define ADDR_REG	2
#define DEFAULT_ADDR    0x400    //15 bit addr

#define FLASH_CAPACITY  0x8000   //256k bit 
#define FLASH_PAGE_SIZE 0x40     //64 byte
#define TWR_DELAY       5
#endif

#define I2CDEV_DEBUG
#ifdef I2CDEV_DEBUG
#define i2c_dbg(fmt, args...)   \
    printk(KERN_INFO fmt, ##args)
#else
#define i2c_dbg(fmt, args...)   \
    do {} while(0)
#endif
 
struct i2c_dev {
        struct i2c_client *client;
        struct device *dev;
    #ifdef I2CDEV_TEST
        struct delayed_work work;
        struct workqueue_struct *wq;
    #endif
};
static struct i2c_client *i2cdev_client = NULL;


static int test_i2c(struct i2c_client *client, char addr)
{
    int ret = 0;
    char tmp[3] = {0};
    int i;

    i2c_dbg("adap = %d, addr = 0x%x \n",client->adapter->nr, client->addr);
    if(addr != 0xFF)
        client->addr = addr;
   
    /* test for write */
    tmp[0] = ((u8)(DEFAULT_ADDR >> 8));
    tmp[1] = ((u8)(DEFAULT_ADDR));
    tmp[2] = 0x55;
    ret = i2c_master_send(client, tmp, 3);  
    if(ret < 0)
        return ret;

    msleep(TWR_DELAY);
    /* test for read */
    tmp[0] = ((u8)(DEFAULT_ADDR >> 8));
    tmp[1] = ((u8)(DEFAULT_ADDR));
    ret = i2c_master_send(client, tmp, 2);
    if(ret < 0)
        return ret;

    memset(tmp, 0, sizeof(tmp));
    ret = i2c_master_recv(client, tmp, 1);
    if(ret < 0)
        return ret;			

    for ( i = 0; i < 3; i++)
    {
        i2c_dbg("i2c read data tmp[%d] = 0x%x:\n", i, tmp[i]);
    }

    return 0;
}

static ssize_t i2cdev_read(struct file *file, char __user *buf, size_t count,  
        loff_t *offset)  
{  
    char *rx_buf;
    char tmp[2] = {0};  
    int ret,page_nr;  
    struct i2c_client *client = file->private_data;  
    int addr = DEFAULT_ADDR;

    if(addr > FLASH_CAPACITY){
        printk(KERN_ERR "%s: invalid addr:0x%x > flash capacity \n",__FILE__,addr);
        return -EFAULT;
    }

    if ((count + DEFAULT_ADDR) > FLASH_CAPACITY){
        printk(KERN_ERR "%s: addr add len beyond the scope of flash capacity !!!\n", __FILE__);
        return -EFAULT;
    }  
  
    rx_buf = kmalloc(count, GFP_KERNEL);  
    if (tmp == NULL)  
        return -ENOMEM;  
  
    i2c_dbg("i2c-dev: i2c-%d reading %zu bytes./n",  
        iminor(file->f_path.dentry->d_inode), count);  
    
    page_nr = 0;
    do{
        tmp[0] = ((u8)(addr >> 8));
        tmp[1] = ((u8)(addr));
        ret = i2c_master_send(client, tmp, ADDR_REG);
        if(ret < 0){
	    printk(KERN_ERR "i2cdev_read send addr fail \n");
	    goto fail_i2c;
        }

        if(count <= FLASH_PAGE_SIZE){
            ret = i2c_master_recv(client, &rx_buf[(page_nr * FLASH_PAGE_SIZE)], count);
            break;
        }     
        ret = i2c_master_recv(client, &rx_buf[(page_nr * FLASH_PAGE_SIZE)], FLASH_PAGE_SIZE);
        if(ret < 0)
            break;

        page_nr += 1;
        addr += FLASH_PAGE_SIZE;
        count -= FLASH_PAGE_SIZE;

        msleep(TWR_DELAY);
    }while(1);  
    
    if (ret >= 0)  
        ret = copy_to_user(buf, rx_buf, (count + page_nr * FLASH_PAGE_SIZE)) ? -EFAULT : ret;  
	 printk(KERN_ERR "rx_buf=%s size =%d \n",rx_buf,strlen(rx_buf));


fail_i2c:
    kfree(rx_buf);  
    return ret;  
} 

static ssize_t i2cdev_write(struct file *file, const char __user *buf,  
        size_t count, loff_t *offset)  
{  
    int ret=0;
		int page_nr;
    char *tmp;  
    struct i2c_client *client = file->private_data;  
    int addr = DEFAULT_ADDR;
	printk(KERN_ERR "buf=%s strlen(buf)=%d  count =%d\n",buf,strlen(buf),count);

    if(addr > FLASH_CAPACITY){
        printk(KERN_ERR "%s: invalid addr:0x%x > flash capacity \n",__FILE__,addr);
        return -EFAULT;
    }

    if ((count + DEFAULT_ADDR) > FLASH_CAPACITY){  
        printk(KERN_ERR "%s: addr add len beyond the scope of flash capacity !!!\n",__FILE__);
        return -EFAULT;
    }
  
    tmp = kmalloc((count + ADDR_REG), GFP_KERNEL);
    if (tmp == NULL)
        return -ENOMEM;

    if(copy_from_user(&tmp[ADDR_REG],buf,count)){
        kfree(tmp);
        return -EFAULT;
    } 

    page_nr = 0;
    do{
        tmp[0] = ((u8)(addr >> 8));
        tmp[1] = ((u8)(addr));

	if(count <= FLASH_PAGE_SIZE){
            ret  += i2c_master_send(client, tmp, count + ADDR_REG);
            break;
	}
        ret  += i2c_master_send(client, tmp, FLASH_PAGE_SIZE + ADDR_REG);
        if(ret < 0)
            break;

        page_nr += 1;
        addr += FLASH_PAGE_SIZE;
        count -= FLASH_PAGE_SIZE;

        memcpy(&tmp[ADDR_REG], &tmp[(ADDR_REG + (page_nr * FLASH_PAGE_SIZE))], FLASH_PAGE_SIZE);
     
        msleep(TWR_DELAY);
    }while(1);

    kfree(tmp);  
    return ret;  
} 

static int i2cdev_check(struct device *dev, void *addrp)
{
    struct i2c_client *client = i2c_verify_client(dev);
    
    if (!client || client->addr != *(unsigned int *)addrp)
        return 0;

    return dev->driver ? -EBUSY : 0;
}
/* walk up mux tree */
static int i2cdev_check_mux_parents(struct i2c_adapter *adapter, int addr)
{
    struct i2c_adapter *parent = i2c_parent_is_i2c_adapter(adapter);
    int result;

    result = device_for_each_child(&adapter->dev, &addr, i2cdev_check);
    if (!result && parent)
        result = i2cdev_check_mux_parents(parent, addr);

    return result;
}
/* recurse down mux tree */
static int i2cdev_check_mux_children(struct device *dev, void *addrp)
{
    int result;

    if (dev->type == &i2c_adapter_type)
        result = device_for_each_child(dev, addrp,
             i2cdev_check_mux_children);
    else
        result = i2cdev_check(dev, addrp);

    return result;
}
static int i2cdev_check_addr(struct i2c_adapter *adapter, unsigned int addr)
{
    struct i2c_adapter *parent = i2c_parent_is_i2c_adapter(adapter);
    int result = 0;

    if (parent)
        result = i2cdev_check_mux_parents(parent, addr);

    if (!result)
        result = device_for_each_child(&adapter->dev, &addr,
            i2cdev_check_mux_children);

    return result;
}

static long i2cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)  
{  
    struct i2c_client *client = file->private_data;  
    unsigned long funcs;  
  
    switch (cmd) {  
    case I2C_SLAVE:  
    case I2C_SLAVE_FORCE:  
        /* NOTE:  devices set up to work with "new style" drivers 
         * can't use I2C_SLAVE, even when the device node is not 
         * bound to a driver.  Only I2C_SLAVE_FORCE will work. 
         * 
         * Setting the PEC flag here won't affect kernel drivers, 
         * which will be using the i2c_client node registered with 
         * the driver model core.  Likewise, when that client has 
         * the PEC flag already set, the i2c-dev driver won't see 
         * (or use) this setting. 
        */
        if ((arg > 0x3ff) ||  
            (((client->flags & I2C_M_TEN) == 0) && arg > 0x7f)){    
            return -EINVAL;
        }  
        if (cmd == I2C_SLAVE && i2cdev_check_addr(client->adapter, arg))
            return -EBUSY;  
        /* REVISIT: address could become busy later */ 
        i2c_dbg("--- valid addr : 0x%x ---\n",(u32)arg); 
        client->addr = arg;  
        return 0;
    case I2C_TENBIT:  
        if (arg)  
            client->flags |= I2C_M_TEN;  
        else  
            client->flags &= ~I2C_M_TEN;  
        return 0;  
    case I2C_PEC:  
        if (arg)  
            client->flags |= I2C_CLIENT_PEC;  
        else  
            client->flags &= ~I2C_CLIENT_PEC;  
        return 0;  
    case I2C_FUNCS:  
        funcs = i2c_get_functionality(client->adapter);  
        return put_user(funcs, (unsigned long __user *)arg);  
  
    case I2C_RDWR: 
        printk(KERN_ERR "%s: ---ioctl : I2C_RDWR cmd non't support \n",__FILE__);
        return -ENOTTY; 
        //return i2cdev_ioctl_rdrw(client, arg);  
  
    case I2C_SMBUS:
        printk(KERN_ERR "%s: ---ioctl : I2C_SMBUS cmd non't support \n",__FILE__);
        return -ENOTTY;  
        //return i2cdev_ioctl_smbus(client, arg);  
  
    case I2C_RETRIES:  
        client->adapter->retries = arg;  
        break;  
    case I2C_TIMEOUT:  
        /* For historical reasons, user-space sets the timeout 
         * value in units of 10 ms. 
         */  
        client->adapter->timeout = msecs_to_jiffies(arg * 10);  
        break;  
    default:  
        /* NOTE:  returning a fault code here could cause trouble 
         * in buggy userspace code.  Some old kernel bugs returned 
         * zero in this case, and userspace code might accidentally 
         * have depended on that bug. 
         */  
        printk(KERN_ERR "%s: ---ioctl : cmd[%d] isn't support \n",__FILE__,cmd);
        return -ENOTTY;  
    }  
    return 0;  
}  

static int i2cdev_open(struct inode *inode, struct file *file)
{
    struct i2c_client *client = i2cdev_client;
    
    file->private_data = client;
   
    return 0;
}

static int i2cdev_release(struct inode *inode, struct file *file)
{
   
    file->private_data = NULL;
   
    return 0;
}

static const struct file_operations i2cdev_fops = {  
    .owner      = THIS_MODULE,  
    .read       = i2cdev_read,  
    .write      = i2cdev_write,  
    .unlocked_ioctl = i2cdev_ioctl,  
    .open       = i2cdev_open,  
    .release    = i2cdev_release,  
}; 

#ifdef I2CDEV_TEST
static void i2cdev_test_work(struct work_struct *work)
{
    struct i2c_dev *i2cdev = container_of(work, struct i2c_dev, work);
    struct inode *inode;
    struct file *file = "/dev/i2c-2";
    int tmp0[10] = {1,2,3,4,5,6,7,8,9,0};
    int tmp1[10] = {0};

#if 1
    i2cdev_open(inode, file);
    i2cdev_write(file, tmp0, 5, 0);
    msleep(5);
    i2cdev_read(file, tmp1, 5, 0);
//    i2cdev_ioctl(file, I2C_SLAVE, 0x03);
//    i2cdev_ioctl(file, 0, 0xf3);
    i2cdev_release(inode, file);
#endif
    queue_delayed_work(i2cdev->wq, &i2cdev->work, 500);
}

#endif
/*******************************************************************************/
static struct class *i2c_dev_class;

static int i2c_dev_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
    struct i2c_dev *i2cdev;
    int ret;

    i2c_dbg("===i2c_dev_probe start===\n");

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(&client->dev, "I2C functionality not supported\n");
        return -ENODEV;
    }

    i2cdev = kzalloc(sizeof(*i2cdev), GFP_KERNEL);
    if (!i2cdev){
        printk(KERN_ERR "%s: kzalloc failed.\n",__FILE__);
        return -ENOMEM;
    }

    i2cdev->dev = device_create(i2c_dev_class, NULL, MKDEV(I2C_MAJOR, client->adapter->nr), 
				NULL,"i2c-%d",client->adapter->nr);
    if (IS_ERR(i2cdev->dev)) {		
        ret = PTR_ERR(i2cdev->dev);		
        goto dev_create_err;	
    }

    i2cdev->client = client;
    i2cdev_client = client;
    client->flags &= ~I2C_M_TEN;        //support 7bit addr only
    client->flags &= ~I2C_CLIENT_TEN;
    i2c_set_clientdata(client, i2cdev);

#ifdef UNKNOWN_DEV_ADDR
    char i;
    for(i = 0; i <= 0x7f; i++){
        ret = test_i2c(i2cdev->client, i);
        if( ret == 0){
            i2c_dbg("i2c dev addr is 0x%x\n", i);
            break;
        }
	msleep(1);
    }
    if(i > 0x7f){
        printk(KERN_ERR "%s: i2c test fail\n",__FILE__);
        goto i2c_fail;
    }
#else
    ret = test_i2c(i2cdev->client, 0xff);
    if(ret < 0){
        printk(KERN_ERR "%s: i2c test fail\n",__FILE__);
        goto i2c_fail;
    }
#endif

#ifdef I2CDEV_TEST
    INIT_DELAYED_WORK(&i2cdev->work, i2cdev_test_work);
    i2cdev->wq = create_singlethread_workqueue("i2cdev_test_workqueue");
    queue_delayed_work(i2cdev->wq, &i2cdev->work, 50);
#endif

    i2c_dbg("===i2c_dev_probe  ok ===\n");	
    return 0;

i2c_fail:
    device_destroy(i2c_dev_class,MKDEV(I2C_MAJOR, client->adapter->nr));
dev_create_err:
    kfree(i2cdev);
    return ret;
}

static int i2c_dev_remove(struct i2c_client *client)
{
    struct i2c_dev *i2cdev = i2c_get_clientdata(client);

    i2c_dbg("===i2c_dev_remove===\n");
#ifdef I2CDEV_TEST        
    cancel_delayed_work_sync(&i2cdev->work);
    destroy_workqueue(i2cdev->wq);
#endif
    device_destroy(i2c_dev_class,MKDEV(I2C_MAJOR, client->adapter->nr));
    kfree(i2cdev);
    return 0;
}

static const struct of_device_id i2cdev_dt_ids[] = {
    { .compatible = "actions,i2cdev" },
    {},
};

MODULE_DEVICE_TABLE(of, i2cdev_dt_ids);

static const struct i2c_device_id i2c_dev_id[] = {
    {"i2cdev", 0},
    {},
};
MODULE_DEVICE_TABLE(i2c, i2c_dev_id);

static struct i2c_driver i2c_dev_driver = {
    .driver = {
        .name = "i2cdev",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(i2cdev_dt_ids),
    },
    .probe	= i2c_dev_probe,
    .remove	= i2c_dev_remove,
    .id_table   = i2c_dev_id,
};

static int __init i2c_dev_init(void)
{
    int ret;
    i2c_dbg("i2c dev init !!!\n");
    ret = register_chrdev(I2C_MAJOR, "i2c", &i2cdev_fops);  
    if (ret)  
        goto out;  
  
    i2c_dev_class = class_create(THIS_MODULE, "i2cdev");  
    if (IS_ERR(i2c_dev_class)) {  
        ret = PTR_ERR(i2c_dev_class);  
        goto out_unreg_chrdev;  
    }

    ret = i2c_add_driver(&i2c_dev_driver);
    if (ret != 0)  
        goto out_i2c_driver;
    return ret;

out_i2c_driver:
    class_destroy(i2c_dev_class);  
out_unreg_chrdev:  
    unregister_chrdev(I2C_MAJOR, "i2c");  
out:  
    printk(KERN_ERR "%s: Driver Initialisation failed\n", __FILE__);  
    return ret;
}

static void __exit i2c_dev_exit(void)
{
    i2c_dbg("i2c dev exit !!!\n");
    i2c_del_driver(&i2c_dev_driver); 
    class_destroy(i2c_dev_class);  
    unregister_chrdev(I2C_MAJOR,"i2cdev"); 
}

module_init(i2c_dev_init);
module_exit(i2c_dev_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ActduinoTest i2c test driver");
MODULE_AUTHOR("Leo Zhang, leo.zhang@artekmicro.com");
MODULE_ALIAS("platform:ActduinoTest");

