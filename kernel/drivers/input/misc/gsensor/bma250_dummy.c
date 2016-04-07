
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/uaccess.h>

// dummy sensor rotation
// 1 -- 0 degree, 2 -- 90 degree
// 3- - 180 degree, 4 -- 270 degree
#define SENSOR_ROTATION            3

// device info
#define SENSOR_NAME                   "bma250"
#define SENSOR_I2C_ADDR            0x18
#define ABSMIN                              -512
#define ABSMAX                             512
#define LSG                                   256
#define MAX_DELAY                       200

struct bma250_acc{
    s16    x;
    s16    y;
    s16    z;
} ;

struct bma250_data {
    struct i2c_client *bma250_client;
    struct input_dev *input;
    atomic_t delay;
    atomic_t enable;
    struct mutex enable_mutex;
    struct delayed_work work;
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend early_suspend;
#endif
    atomic_t position;
};

// cfg data : 1-- used
#define CFG_GSENSOR_USE_CONFIG  1

/*******************************************
* for xml cfg
*******************************************/
#define CFG_GSENSOR_ADAP_ID          "gsensor.i2c_adap_id"
#define CFG_GSENSOR_POSITION         "gsensor.position"
#define CFG_GSENSOR_CALIBRATION      "gsensor.calibration"

extern int get_config(const char *key, char *buff, int len);
/*******************************************
* end for xml cfg
*******************************************/

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma250_early_suspend(struct early_suspend *h);
static void bma250_early_resume(struct early_suspend *h);
#endif

static int bma250_read_data(struct i2c_client *client, struct bma250_acc *acc)
{
    static int count = 0;
    int noise;
    
    // generate noise
    noise = count / 4 - 1;
    count = (count + 1)  % 12;
    
    // default orientation: screen upright
    acc->x =  0;
    acc->y =  LSG + noise;
    acc->z =  0; 
    
    return 0;
}

static int bma250_axis_remap(struct i2c_client *client, struct bma250_acc *acc)
{
    s16 swap, cnt;
    struct bma250_data *bma250 = i2c_get_clientdata(client);
    int position = atomic_read(&bma250->position);

    for (cnt = 1; cnt < abs(position); cnt ++) { 
        // rotate 90 degree
        swap = acc->x;
        acc->x = -(acc->y);
        acc->y = swap;
    }
    
    return 0;
}

static ssize_t bma250_value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct input_dev *input = to_input_dev(dev);
    struct bma250_data *bma250 = input_get_drvdata(input);
    struct bma250_acc acc;

    bma250_read_data(bma250->bma250_client, &acc);
    bma250_axis_remap(bma250->bma250_client, &acc);   
    
    return sprintf(buf, "%d %d %d\n", acc.x, acc.y, acc.z);
}

static ssize_t bma250_delay_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma250_data *bma250 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&bma250->delay));

}

static ssize_t bma250_delay_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma250_data *bma250 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&bma250->delay, (unsigned int) data);

    return count;
}


static ssize_t bma250_enable_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma250_data *bma250 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&bma250->enable));

}

static void bma250_do_enable(struct device *dev, int enable)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma250_data *bma250 = i2c_get_clientdata(client);

    if (enable) {
        schedule_delayed_work(&bma250->work,
            msecs_to_jiffies(atomic_read(&bma250->delay)));
    } else {
        cancel_delayed_work_sync(&bma250->work);
    }
}

static void bma250_set_enable(struct device *dev, int enable)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma250_data *bma250 = i2c_get_clientdata(client);
    int pre_enable = atomic_read(&bma250->enable);

    mutex_lock(&bma250->enable_mutex);
    if (enable != pre_enable) {
        bma250_do_enable(dev, enable);
        atomic_set(&bma250->enable, enable);
    }
    mutex_unlock(&bma250->enable_mutex);
}

static ssize_t bma250_enable_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if ((data == 0) || (data == 1))
        bma250_set_enable(dev, data);

    return count;
}

static ssize_t bma250_board_position_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma250_data *bma250 = i2c_get_clientdata(client);

    data = atomic_read(&(bma250->position));

    return sprintf(buf, "%d\n", data);
}

static ssize_t bma250_board_position_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma250_data *bma250 = i2c_get_clientdata(client);

    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&(bma250->position), (int) data);

    return count;
}

static DEVICE_ATTR(value, S_IRUGO,
        bma250_value_show, NULL);
static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma250_delay_show, bma250_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma250_enable_show, bma250_enable_store);
static DEVICE_ATTR(board_position, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma250_board_position_show, bma250_board_position_store);

static struct attribute *bma250_attributes[] = {
    &dev_attr_value.attr,
    &dev_attr_delay.attr,
    &dev_attr_enable.attr,
    &dev_attr_board_position.attr,
    NULL
};

static struct attribute_group bma250_attribute_group = {
    .attrs = bma250_attributes
};

static void bma250_work_func(struct work_struct *work)
{
    struct bma250_data *bma250 = container_of((struct delayed_work *)work,
            struct bma250_data, work);
    static struct bma250_acc acc;    
    int result;
    unsigned long delay = msecs_to_jiffies(atomic_read(&bma250->delay));
    
    result = bma250_read_data(bma250->bma250_client, &acc);
    if (result == 0) {
        bma250_axis_remap(bma250->bma250_client, &acc);
        
        input_report_abs(bma250->input, ABS_X, acc.x);
        input_report_abs(bma250->input, ABS_Y, acc.y);
        input_report_abs(bma250->input, ABS_Z, acc.z);
        input_sync(bma250->input);
    }
    schedule_delayed_work(&bma250->work, delay);
}

static int bma250_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int err = 0;
    struct bma250_data *data;
    struct input_dev *dev;
    int cfg_position;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk(KERN_INFO "i2c_check_functionality error\n");
        goto exit;
    }
    data = kzalloc(sizeof(struct bma250_data), GFP_KERNEL);
    if (!data) {
        err = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    data->bma250_client = client;
    mutex_init(&data->enable_mutex);

    INIT_DELAYED_WORK(&data->work, bma250_work_func);
    atomic_set(&data->delay, MAX_DELAY);
    atomic_set(&data->enable, 0);

#if CFG_GSENSOR_USE_CONFIG > 0
        /*get xml cfg*/
        err = get_config(CFG_GSENSOR_POSITION, (char *)(&cfg_position), sizeof(int));
        if (err != 0) {
            printk(KERN_ERR"get position %d fail\n", cfg_position);
            goto kfree_exit;
        }
#else
        cfg_position = SENSOR_ROTATION;
#endif
    atomic_set(&data->position, cfg_position);
        
    dev = input_allocate_device();
    if (!dev)
        return -ENOMEM;
    dev->name = SENSOR_NAME;
    dev->id.bustype = BUS_I2C;

    input_set_capability(dev, EV_ABS, ABS_MISC);
    input_set_abs_params(dev, ABS_X, ABSMIN, ABSMAX, 0, 0);
    input_set_abs_params(dev, ABS_Y, ABSMIN, ABSMAX, 0, 0);
    input_set_abs_params(dev, ABS_Z, ABSMIN, ABSMAX, 0, 0);
    input_set_drvdata(dev, data);

    err = input_register_device(dev);
    if (err < 0) {
        input_free_device(dev);
        goto kfree_exit;
    }

    data->input = dev;

    err = sysfs_create_group(&data->input->dev.kobj,
            &bma250_attribute_group);
    if (err < 0)
        goto error_sysfs;

#ifdef CONFIG_HAS_EARLYSUSPEND
    data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    data->early_suspend.suspend = bma250_early_suspend;
    data->early_suspend.resume = bma250_early_resume;
    register_early_suspend(&data->early_suspend);
#endif
        
    return 0;

error_sysfs:
    input_unregister_device(data->input);

kfree_exit:
    kfree(data);
exit:
    return err;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma250_early_suspend(struct early_suspend *h)
{
    // sensor hal will disable when early suspend
}


static void bma250_early_resume(struct early_suspend *h)
{
    // sensor hal will enable when early resume
}
#endif

static int bma250_remove(struct i2c_client *client)
{
    struct bma250_data *data = i2c_get_clientdata(client);

    bma250_set_enable(&client->dev, 0);
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&data->early_suspend);
#endif
    sysfs_remove_group(&data->input->dev.kobj, &bma250_attribute_group);
    input_unregister_device(data->input);
    kfree(data);

    return 0;
}

#ifdef CONFIG_PM

static int bma250_suspend(struct device *dev)
{
    bma250_do_enable(dev, 0);
    return 0;
}

static int bma250_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma250_data *data = i2c_get_clientdata(client);
    
    bma250_do_enable(dev, atomic_read(&data->enable));    
    return 0;
}

#else

#define bma250_suspend        NULL
#define bma250_resume        NULL

#endif /* CONFIG_PM */

static SIMPLE_DEV_PM_OPS(bma250_pm_ops, bma250_suspend, bma250_resume);

static const unsigned short  bma250_addresses[] = {
    SENSOR_I2C_ADDR,
    I2C_CLIENT_END,
};

static const struct i2c_device_id bma250_id[] = {
    { SENSOR_NAME, 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, bma250_id);

static struct i2c_driver bma250_driver = {
    .driver = {
        .owner    = THIS_MODULE,
        .name    = SENSOR_NAME,
        .pm    = &bma250_pm_ops,
    },
    .class        = I2C_CLASS_HWMON,
//    .address_list    = bma250_addresses,
    .id_table    = bma250_id,
    .probe        = bma250_probe,
    .remove        = bma250_remove,

};

static struct i2c_board_info bma250_board_info={
    .type = SENSOR_NAME, 
    .addr = SENSOR_I2C_ADDR,
};

static struct i2c_client *bma250_client;

static int __init bma250_init(void)
{
    struct i2c_adapter *i2c_adap;
    unsigned int cfg_i2c_adap_id;

#if CFG_GSENSOR_USE_CONFIG > 0
    int ret;
    
    /*get xml cfg*/
    ret = get_config(CFG_GSENSOR_ADAP_ID, (char *)(&cfg_i2c_adap_id), sizeof(unsigned int));
    if (ret != 0) {
        printk(KERN_ERR"get i2c_adap_id %d fail\n", cfg_i2c_adap_id);
        return ret;
    }
#else
    cfg_i2c_adap_id = 2;
#endif
    
    i2c_adap = i2c_get_adapter(cfg_i2c_adap_id);  
    bma250_client = i2c_new_device(i2c_adap, &bma250_board_info);  
    i2c_put_adapter(i2c_adap);
    
    return i2c_add_driver(&bma250_driver);
}

static void __exit bma250_exit(void)
{
    i2c_unregister_device(bma250_client);
    i2c_del_driver(&bma250_driver);
}

MODULE_AUTHOR("Zhining Song <songzhining@actions-semi.com>");
MODULE_DESCRIPTION("BMA250 dummy 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");

module_init(bma250_init);
module_exit(bma250_exit);

