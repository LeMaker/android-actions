#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/module.h> 
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/memblock.h>
#include <linux/device.h>
#include <mach/hardware.h>

#define GET_OWL_AFI_INFO 0

extern unsigned char *g_afinfo;

static int cdev_ret;
static dev_t misc_owl_devt;
static struct class *misc_owl_class;
static struct device *misc_owl_dev;
static struct cdev misc_owl_cdev;


static int misc_owl_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int misc_owl_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int g_board_opt = 0;
static int g_board_opt_flags = 0;

static int __init boardopt_setup(char *str)
{
    int ints[3];
    
    get_options(str, 3, ints);
    if (ints[0] != 2)
        return 0;
        
    g_board_opt = ints[1];
    g_board_opt_flags = ints[2];
    printk("g_board_opt=%d, g_board_opt_flags=0x%x\n", g_board_opt, g_board_opt_flags);
    return 1;
}
__setup("board_opt=", boardopt_setup);

int owl_get_board_opt(void)
{
    return g_board_opt;
}
EXPORT_SYMBOL_GPL(owl_get_board_opt);

int owl_get_board_opt_flags(void)
{
	return g_board_opt_flags;
}
EXPORT_SYMBOL_GPL(owl_get_board_opt_flags);


static long misc_owl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch(cmd) {
    case GET_OWL_AFI_INFO:
        if(copy_to_user((char *)arg, g_afinfo, 0x400) != 0) {
            printk("copy_to_user for owl misc failed\n");
            return -1;
        }
        break;
    
    default:
        return -1;
    }
    
    return 0;
}

const struct file_operations misc_owl_cdev_file_operations = {
    .open = misc_owl_open,
    .release = misc_owl_release,
    .unlocked_ioctl = misc_owl_ioctl,
};

static int __init misc_owl_init(void)
{
    int ret;

    misc_owl_devt = MKDEV(90, 0);
    ret = register_chrdev_region(misc_owl_devt, 1, "miscowl");
    if (ret != 0) {
        printk("misc_owl register_chrdev_region err: %d\n", ret);
        return -1;
    }

    cdev_init(&misc_owl_cdev, &misc_owl_cdev_file_operations);
    cdev_ret = cdev_add(&misc_owl_cdev, misc_owl_devt, 1);
    if (cdev_ret != 0) {
        printk("Unable to get misc_owl major %d\n", 92);
        goto err;
    }

    /* create your own class under /sysfs */
    misc_owl_class = class_create(THIS_MODULE, "miscowl");
    if (IS_ERR(misc_owl_class)) {
        printk("misc_owl: failed in creating class.\n");
        goto err;
    }

    /* register your own device in sysfs, and this will cause udev to create corresponding device node */
    misc_owl_dev = device_create(misc_owl_class, NULL, misc_owl_devt, NULL, "miscowl");
    if (IS_ERR(misc_owl_dev)) {
        printk("misc_owl: failed in creating device.\n");
        goto err;
    }

    return 0;
    
err:
    if (!IS_ERR(misc_owl_dev))
        device_del(misc_owl_dev);
    if (!IS_ERR(misc_owl_class))
        class_destroy(misc_owl_class);
    if (cdev_ret == 0)
        cdev_del(&misc_owl_cdev);
    unregister_chrdev_region(misc_owl_devt, 1);
    
    return -1;
}

late_initcall(misc_owl_init);

