/*
 * drivers/gpio_test/gpio_test.c
 *
 * Copyright (c) 2012 ShenZhen artekmicro
 *	wanguihong <wanguihong@artekmicro.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
 
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/async.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/gpio.h>
#include <mach/gpio.h>


#define GPIO_TEST_SELFTEST 
#ifdef GPIO_TEST_SELFTEST
static struct delayed_work gpio_test_work;
static struct workqueue_struct *gpio_test_workqueue = NULL;
#endif

#define GPIO_TEST_MAJOR    160
#define GPIO_TEST_NAME    "gpio_owl_test"
#define GPIO_TEST_LABLE_1   "gpio_test_in"
#define GPIO_TEST_LABLE_O    "gpio_test_out"

#define CMD_SET_GPIOA_OUT  0
#define CMD_SET_GPIOB_OUT  1
#define CMD_SET_GPIOC_OUT  20
#define CMD_SET_GPIOD_OUT  3
#define CMD_SET_GPIOE_OUT  4
#define CMD_SET_GPIOA_IN  5
#define CMD_SET_GPIOB_IN  6
#define CMD_SET_GPIOC_IN  7
#define CMD_SET_GPIOD_IN  8
#define CMD_SET_GPIOE_IN  9

#define CMD_INIT  10
#define CMD_RELEASE  11
#define CMD_START_TEST  12
#define CMD_SET_ON_OFF  13
#define CMD_READ_ON_OFF  14

static unsigned int gpio_test_exist = 1;//0:not exist 1:exist

static unsigned GPIO_TEST_OUT = OWL_GPIO_PORTC(26);
static unsigned GPIO_TEST_IN = OWL_GPIO_PORTD(31);
unsigned long io_in = 31;

static void request_gpio(void);
static void free_gpio(void);
static int test_main(void);
static int set_onoff(int on_off);
static int read_onoff(void);
static long gpio_test_ioctl(struct file *file, unsigned int cmd, unsigned long arg)  
{  
    printk("--- gpio_test_ioctl---CMD %d ,arg %d\n",cmd,arg);
	
	long ret = 0;
    
    if(!gpio_test_exist){
        printk("[gpio_test] gpio_test isn't exist !!!\n");
        return -1;
    }

    switch (cmd) { 
    case CMD_SET_GPIOA_OUT:
        GPIO_TEST_OUT = OWL_GPIO_PORTA(arg);
	printk("GPIO_TEST_OUT = %d\n",GPIO_TEST_OUT);
        break;
	case CMD_SET_GPIOB_OUT:
        GPIO_TEST_OUT = OWL_GPIO_PORTB(arg);
printk("GPIO_TEST_OUT = %d\n",GPIO_TEST_OUT);
        break;
	case CMD_SET_GPIOC_OUT:
        GPIO_TEST_OUT = OWL_GPIO_PORTC(arg);
		printk("GPIO_TEST_OUT = %d\n",GPIO_TEST_OUT);
        break;
	case CMD_SET_GPIOD_OUT:	    
        GPIO_TEST_OUT = OWL_GPIO_PORTD(arg);
	printk("GPIO_TEST_OUT = %d\n",GPIO_TEST_OUT);
        break;
	case CMD_SET_GPIOE_OUT:
        GPIO_TEST_OUT = OWL_GPIO_PORTE(arg);
	printk("GPIO_TEST_OUT = %d\n",GPIO_TEST_OUT);
        break;
	case CMD_SET_GPIOA_IN:
        GPIO_TEST_IN = OWL_GPIO_PORTA(arg);
		io_in = arg;
        break;
	case CMD_SET_GPIOB_IN:
        GPIO_TEST_IN = OWL_GPIO_PORTB(arg);
		io_in = arg;
        break;
	case CMD_SET_GPIOC_IN:
        GPIO_TEST_IN = OWL_GPIO_PORTC(arg);
		io_in = arg;
        break;
	case CMD_SET_GPIOD_IN:	    
        GPIO_TEST_IN = OWL_GPIO_PORTD(arg);
		io_in = arg;
        break;
	case CMD_SET_GPIOE_IN:
        GPIO_TEST_IN = OWL_GPIO_PORTE(arg);
		io_in = arg;
        break;
	case CMD_INIT:
        request_gpio();
        break;
	case CMD_RELEASE:
        free_gpio();
        break;
	case CMD_START_TEST:
        ret = test_main();
        break;
	case CMD_SET_ON_OFF:
		ret=set_onoff(arg);
		break;
	case CMD_READ_ON_OFF:
		ret=read_onoff();
		break;
    default:
        ret = -1;  
        printk("[gpio_test] this cmd isn't support !!!\n");
        break;
    }
    return ret; 
}

static int gpio_test_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int gpio_test_release(struct inode *inode, struct file *file)
{    
    return 0;
}
static int set_onoff(int on_off){
	int ret = -1;
	printk("\n--- set gpio %d 0x%d ---\n",GPIO_TEST_OUT,on_off);
	ret=gpio_direction_output(GPIO_TEST_OUT, on_off);
	return ret;
}

static int read_onoff(){
	int ret = -1;
	ret = gpio_get_value(GPIO_TEST_IN);
	printk("\n--- gpio_get_value(%d) 1? 0x%d ---\n",GPIO_TEST_IN,ret);
	return ret;
}
static int test_main(void)
{
    int ret = 0;
		
	gpio_direction_output(GPIO_TEST_OUT, 1);
	msleep(10);	
	int i = gpio_get_value(GPIO_TEST_IN);
	printk("\n--- gpio_get_value 1? 0x%x ---\n",i);
	if(i == 0) 
	{
	    printk("--- fail ---\n");
		ret = -1;
	}
	
	gpio_direction_output(GPIO_TEST_OUT, 0);	
	msleep(10);
	i = gpio_get_value(GPIO_TEST_IN);
	printk("--- gpio_get_value 0? 0x%x ---\n",i);
	if(i != 0) 
	{
	    printk("--- fail ---\n");
		ret = -1;
	}
	
	gpio_direction_output(GPIO_TEST_OUT, 1);
	msleep(10);
	i = gpio_get_value(GPIO_TEST_IN);
	printk("--- gpio_get_value 1? 0x%x ---\n",i);
	if(i == 0) {
	    printk("--- fail ---\n");
		ret = -1;
	}
	
	return ret;
}

static void request_gpio()
{
    printk("--- request_gpio ---\n"); 
	if(gpio_test_exist){	
	    //config gpio out
        int ret = gpio_request(GPIO_TEST_OUT, GPIO_TEST_LABLE_O);
        if(ret < 0){
            printk("request gpio_test gpio OUT fail\n");
        }
        //为安全起见 先将输出管脚也配置为输入功能
        ret = gpio_direction_input(GPIO_TEST_IN);
       	if(ret < 0){
            printk("init gpio_test gpio OUT fail\n");
        }
        //ret = gpio_direction_output(GPIO_TEST_OUT, 0);
		//if(ret < 0){
        //    printk("init gpio_test gpio OUT fail\n");
        //}
		
		//config gpio in
		ret = gpio_request(GPIO_TEST_IN, GPIO_TEST_LABLE_1);
        if(ret < 0){
            printk("request gpio_test gpio IN fail\n");
        }
        ret = gpio_direction_input(GPIO_TEST_IN);
		if(ret < 0){
            printk("init gpio_test gpio IN fail\n");
        }
        
    }else {
        printk("[gpio_test] gpio_test isn't exist !!!\n");
    }
}

static void free_gpio()
{
   if(gpio_test_exist){
        gpio_free(GPIO_TEST_OUT);
		gpio_free(GPIO_TEST_IN);
	}
}

#ifdef GPIO_TEST_SELFTEST
static void gpio_test_worker()
{   
    static int temp =0 ;
    if (temp == 0)
    {
        temp =1;
        request_gpio();	
		test_main();				
		free_gpio();
    }
}
#endif

static const struct file_operations gpio_test_fops = {  
    .owner      = THIS_MODULE,    
    .unlocked_ioctl = gpio_test_ioctl, 	
    .open       = gpio_test_open,  
    .release    = gpio_test_release,  
}; 

static struct class *gpio_test_dev_class;
static struct device *gpio_test_class_device;

static int __init gpio_test_init(void)
{
    int ret;
    printk(KERN_ERR "%s: ==gpio_test_init==\n",__FILE__);

    ret = register_chrdev(GPIO_TEST_MAJOR, "gpio_test", &gpio_test_fops);  
    if (ret)  
        goto out;  
  
    gpio_test_dev_class = class_create(THIS_MODULE, GPIO_TEST_NAME);  
    if (IS_ERR(gpio_test_dev_class)) {  
        ret = PTR_ERR(gpio_test_dev_class);  
        goto out_unreg_chrdev;  
    }
    
    gpio_test_class_device = device_create(gpio_test_dev_class, NULL, MKDEV(GPIO_TEST_MAJOR, 0), NULL, GPIO_TEST_NAME);
    if (IS_ERR(gpio_test_class_device)) {
        ret = PTR_ERR(gpio_test_class_device);
        goto out_unreg_class;
    }

#ifdef GPIO_TEST_SELFTEST
    INIT_DELAYED_WORK(&gpio_test_work, gpio_test_worker);
    gpio_test_workqueue = create_singlethread_workqueue("gpio_test_workqueue");
    queue_delayed_work(gpio_test_workqueue, &gpio_test_work, 500);
#endif

    return 0;

out_unreg_class:  
    class_destroy(gpio_test_dev_class);  
out_unreg_chrdev:  
    unregister_chrdev(GPIO_TEST_MAJOR, "gpio_test");
out:  
    printk(KERN_ERR "%s: Driver Initialisation failed\n", __FILE__);  
    return ret;
}

static void __exit gpio_test_exit(void)
{
    printk("gpio_test_exit !!!\n");
    device_destroy(gpio_test_dev_class,MKDEV(GPIO_TEST_MAJOR, 0));  
    class_destroy(gpio_test_dev_class);  
    unregister_chrdev(GPIO_TEST_MAJOR,GPIO_TEST_NAME); 
#ifdef GPIO_TEST_SELFTEST        
    cancel_delayed_work_sync(&gpio_test_work);
    destroy_workqueue(gpio_test_workqueue);
#endif
}

module_init(gpio_test_init);
module_exit(gpio_test_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ActduinoTest gpio test driver");
MODULE_AUTHOR("wanguihong, wanguihong@artekmicro.com");
MODULE_ALIAS("platform:ActduinoTest");

