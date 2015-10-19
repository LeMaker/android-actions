
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

#define CFG_GSENSOR_USE_CONFIG 0
#define GSENSOR_I2C_ADAPTER 2
// device info
#define SENSOR_NAME                   "ltr301"
#define SENSOR_I2C_ADDR            0x1C
#define ABSMIN                              0
#define ABSMAX                             64000
#define MAX_DELAY                       500

// constant define
#define LTR301_PART_ID                      0x80
#define LTR301_MANUFAC_ID               0x05
#define LTR301_MODE_STANDBY          0x00
#define LTR301_MODE_ACTIVE             0x02

// register define
#define ALS_CONTR_REG                      0x80
#define ALS_MEAS_RATE_REG               0x85
#define PART_ID_REG                           0x86
#define MANUFAC_ID_REG                    0x87
#define ALS_DATA_CH1_0_REG            0x88
#define ALS_DATA_CH1_1_REG            0x89
#define ALS_DATA_CH0_0_REG            0x8A
#define ALS_DATA_CH0_1_REG            0x8B
#define ALS_STATUS_REG                    0x8C

// register bits define
#define ALS_MODE_BIT__POS            0
#define ALS_MODE_BIT__LEN            2
#define ALS_MODE_BIT__MSK            0x03
#define ALS_MODE_BIT__REG            ALS_CONTR_REG


#define LTR301_GET_BITSLICE(regvar, bitname)\
    ((regvar & bitname##__MSK) >> bitname##__POS)

#define LTR301_SET_BITSLICE(regvar, bitname, val)\
    ((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))

static int al3010_get_adc_value(struct i2c_client *client);
struct ltr301_lux{
    u16    ch0;
    u16    ch1;
    u16    lux_val;
} ;

#define AL3010_NUM_CACHABLE_REGS 9
static u8 al3010_reg[AL3010_NUM_CACHABLE_REGS] = 
	{0x00,0x01,0x0c,0x0d,0x10,0x1a,0x1b,0x1c,0x1d};


struct ltr301_data {
    struct i2c_client *ltr301_client;
    struct input_dev *input;
    atomic_t delay;
    atomic_t enable;
    struct mutex lock_al3010;
    u8 reg_cache[AL3010_NUM_CACHABLE_REGS];
    struct mutex enable_mutex;
    struct delayed_work work;
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend early_suspend;
#endif
};

unsigned int cfg_i2c_adap_id = GSENSOR_I2C_ADAPTER;


// cfg data : 1-- used
#define CFG_SENSOR_USE_CONFIG  0
#define CFG_SENSOR_ADAP_ID          "i2c_adap_id"


#ifdef CONFIG_HAS_EARLYSUSPEND
static void ltr301_early_suspend(struct early_suspend *h);
static void ltr301_early_resume(struct early_suspend *h);
#endif

static int ltr301_smbus_read_byte(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data)
{
    s32 dummy;
    dummy = i2c_smbus_read_byte_data(client, reg_addr);
    if (dummy < 0)
        return -1;
    *data = dummy & 0x000000ff;

    return 0;
}

static int ltr301_smbus_write_byte(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data)
{
    s32 dummy;
    dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
    if (dummy < 0)
        return -1;
    return 0;
}


static int ltr301_set_mode(struct i2c_client *client, unsigned char mode)
{
    int comres = 0;
    unsigned char data = 0;

    comres = ltr301_smbus_read_byte(client, ALS_MODE_BIT__REG, &data);    
    data  = LTR301_SET_BITSLICE(data, ALS_MODE_BIT, mode);
    comres += ltr301_smbus_write_byte(client, ALS_MODE_BIT__REG, &data);

    return comres;
}

static int ltr301_get_mode(struct i2c_client *client, unsigned char *mode)
{
    int comres = 0;
    unsigned char data = 0;

    comres = ltr301_smbus_read_byte(client, ALS_CONTR_REG, &data);
    *mode  = LTR301_GET_BITSLICE(data, ALS_MODE_BIT);

    return comres;
}
#if 0
static int ltr301_read_data(struct i2c_client *client, struct ltr301_lux *lux)
{
    int comres = 0;
    unsigned char data0 = 0, data1 = 0;
    int ratio, luxdata_int;

    comres += ltr301_smbus_read_byte(client, ALS_DATA_CH0_0_REG, &data0);
    comres += ltr301_smbus_read_byte(client, ALS_DATA_CH0_1_REG, &data1);
    lux->ch0 = (data1 << 8) | data0;
    
    comres += ltr301_smbus_read_byte(client, ALS_DATA_CH1_0_REG, &data0);
    comres += ltr301_smbus_read_byte(client, ALS_DATA_CH1_1_REG, &data1);
    lux->ch1 = (data1 << 8) | data0;

    /* Compute Lux data from ALS data (ch0 and ch1)
     * ============================================
     * For Ratio <= 0.63:
     * 1.3618*CH0 - 1.645*CH1
     * For 0.63 < Ratio <= 1:
     * 0.4676*CH0 - 0.259*CH1
     * Ratio > 1:
     * 0
     * For high gain, calculated lux/150
     */
    if (lux->ch0 == 0) {
        lux->ch0 = 1;   //avoid zero-div exception
    }
    
    ratio = (lux->ch1 * 100) / lux->ch0;

    if (ratio <= 63){
        luxdata_int = ((13618 * lux->ch0) - (16450 * lux->ch1) + 5000) / 10000;
    }
    else if ((ratio > 63) && (ratio <= 100)){
        luxdata_int = ((4676 * lux->ch0) - (2590 * lux->ch1) + 5000) / 10000;
    }
    else {
        luxdata_int = 0;
    }

    lux->lux_val = luxdata_int;

    return comres;
}
#endif

static ssize_t ltr301_register_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int address, value;
    struct i2c_client *client = to_i2c_client(dev);
    struct ltr301_data *ltr301 = i2c_get_clientdata(client);

    sscanf(buf, "[0x%x]=0x%x", &address, &value);
    
    if (ltr301_smbus_write_byte(ltr301->ltr301_client, (unsigned char)address,
                (unsigned char *)&value) < 0)
        return -EINVAL;

    return count;
}

static ssize_t ltr301_register_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct ltr301_data *ltr301 = i2c_get_clientdata(client);    
    size_t count = 0;
    u8 reg[0x1f];
    int i;
    
    for (i = 0 ; i < 0x1f; i++) {
        ltr301_smbus_read_byte(ltr301->ltr301_client, 0x80+i, reg+i);    
        count += sprintf(&buf[count], "0x%x: 0x%x\n", 0x80+i, reg[i]);
    }
    
    return count;
}

static ssize_t ltr301_mode_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct ltr301_data *ltr301 = i2c_get_clientdata(client);

    if (ltr301_get_mode(ltr301->ltr301_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t ltr301_mode_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct ltr301_data *ltr301 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (ltr301_set_mode(ltr301->ltr301_client, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t ltr301_value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct input_dev *input = to_input_dev(dev);
    struct ltr301_data *ltr301 = input_get_drvdata(input);
    static struct ltr301_lux lux;

	lux.lux_val = al3010_get_adc_value(ltr301->ltr301_client);

    return sprintf(buf, "%d\n",lux.lux_val);
}

static ssize_t ltr301_delay_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct ltr301_data *ltr301 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&ltr301->delay));

}

static ssize_t ltr301_delay_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct ltr301_data *ltr301 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&ltr301->delay, (unsigned int) data);

    return count;
}


static ssize_t ltr301_enable_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct ltr301_data *ltr301 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&ltr301->enable));

}

static int al3010_set_power_state(struct i2c_client *client, int state);
static void ltr301_set_enable(struct device *dev, int enable)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct ltr301_data *ltr301 = i2c_get_clientdata(client);
    int pre_enable = atomic_read(&ltr301->enable);

    mutex_lock(&ltr301->enable_mutex);
    if (enable) {
        if (pre_enable == 0) {
          //  ltr301_set_mode(ltr301->ltr301_client, LTR301_MODE_ACTIVE);//cj
          al3010_set_power_state(ltr301->ltr301_client, 1);
            schedule_delayed_work(&ltr301->work,
                msecs_to_jiffies(atomic_read(&ltr301->delay)));
            atomic_set(&ltr301->enable, 1);
        }

    } else {
        if (pre_enable == 1) {
          //  ltr301_set_mode(ltr301->ltr301_client, LTR301_MODE_STANDBY);//cj
          al3010_set_power_state(ltr301->ltr301_client, 0);
            cancel_delayed_work_sync(&ltr301->work);
            atomic_set(&ltr301->enable, 0);
        }
    }
    mutex_unlock(&ltr301->enable_mutex);
}

static ssize_t ltr301_enable_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;


    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
	
    if ((data == 0) || (data == 1))
        ltr301_set_enable(dev, data);

    return count;
}

static DEVICE_ATTR(mode, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        ltr301_mode_show, ltr301_mode_store);
static DEVICE_ATTR(value, S_IRUGO,
        ltr301_value_show, NULL);
static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        ltr301_delay_show, ltr301_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        ltr301_enable_show, ltr301_enable_store);
static DEVICE_ATTR(reg, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        ltr301_register_show, ltr301_register_store);

static struct attribute *ltr301_attributes[] = {
    &dev_attr_mode.attr,
    &dev_attr_value.attr,
    &dev_attr_delay.attr,
    &dev_attr_enable.attr,
    &dev_attr_reg.attr,
    NULL
};

static struct attribute_group ltr301_attribute_group = {
    .attrs = ltr301_attributes
};
#if 1//cj 


//range
#define	AL3010_ALS_COMMAND	0x10
#define	AL3010_RAN_MASK	0x70
#define	AL3010_RAN_SHIFT	(4)

//mode
#define AL3010_MODE_COMMAND	0x00
#define AL3010_MODE_SHIFT	(0)
#define AL3010_MODE_MASK	0x07


//power state
#define AL3010_POW_MASK		0x01
#define AL3010_POW_UP		0x01
#define AL3010_POW_DOWN		0x00
#define AL3010_POW_SHIFT	(0)


#define	AL3010_ADC_LSB	0x0c
#define	AL3010_ADC_MSB	0x0d
int cali = 100;

static int al3010_range[4] = {77806,19452,4863,1216};


#define ADD_TO_IDX(addr,idx)	{														\
									int i;												\
									for(i = 0; i < AL3010_NUM_CACHABLE_REGS; i++)		\
									{													\
										if (addr == al3010_reg[i])						\
										{												\
											idx = i;									\
											break;										\
										}												\
									}													\
								}

static int __al3010_read_reg(struct i2c_client *client,
			       u32 reg, u8 mask, u8 shift)
{
	struct ltr301_data *data = i2c_get_clientdata(client);
	u8 idx = 0xff;

	ADD_TO_IDX(reg,idx)
	return (data->reg_cache[idx] & mask) >> shift;
}

static int __al3010_write_reg(struct i2c_client *client,
				u32 reg, u8 mask, u8 shift, u8 val)
{
	struct ltr301_data *data = i2c_get_clientdata(client);
	int ret = 0;
	u8 tmp;
	u8 idx = 0xff;

	ADD_TO_IDX(reg,idx)
	if (idx >= AL3010_NUM_CACHABLE_REGS)
		return -EINVAL;

	mutex_lock(&data->lock_al3010);

	tmp = data->reg_cache[idx];
	tmp &= ~mask;
	tmp |= val << shift;

	ret = i2c_smbus_write_byte_data(client, reg, tmp);
	if (!ret)
		data->reg_cache[idx] = tmp;
	mutex_unlock(&data->lock_al3010);
	return ret;
}

/* range */
static int al3010_get_range(struct i2c_client *client)
{
	int tmp;
	tmp = __al3010_read_reg(client, AL3010_ALS_COMMAND,
											AL3010_RAN_MASK, AL3010_RAN_SHIFT);;
	return al3010_range[tmp];
}

static int al3010_get_adc_value(struct i2c_client *client)
{
	struct ltr301_data *data = i2c_get_clientdata(client);
	int lsb, msb, range, val;

	mutex_lock(&data->lock_al3010);
	lsb = i2c_smbus_read_byte_data(client, AL3010_ADC_LSB);

	if (lsb < 0) {
		mutex_unlock(&data->lock_al3010);
		return lsb;
	}

	msb = i2c_smbus_read_byte_data(client, AL3010_ADC_MSB);
	mutex_unlock(&data->lock_al3010);

	if (msb < 0)
		return msb;

	range = al3010_get_range(client);
	val = (((msb << 8) | lsb) * range) >> 16;
	val *= cali;
	return (val / 100);
}
#endif
static void ltr301_work_func(struct work_struct *work)
{
    struct ltr301_data *ltr301 = container_of((struct delayed_work *)work,
            struct ltr301_data, work);
    static struct ltr301_lux lux;
    unsigned long delay = msecs_to_jiffies(atomic_read(&ltr301->delay));
    #if 1//cj
	printk("cj+++work func\n");
	lux.lux_val = al3010_get_adc_value(ltr301->ltr301_client);
	printk("cj+++get adc value = %d \n",lux.lux_val);
	#else
    ltr301_read_data(ltr301->ltr301_client, &lux);
	#endif
	input_report_abs(ltr301->input, ABS_MISC, lux.lux_val);
    input_sync(ltr301->input);
	
	
    schedule_delayed_work(&ltr301->work, delay);
}

#if 1//cj

static int al3010_set_range(struct i2c_client *client, int range)
{
	return __al3010_write_reg(client, AL3010_ALS_COMMAND, 
											AL3010_RAN_MASK, AL3010_RAN_SHIFT, range);
}

static int al3010_set_mode(struct i2c_client *client, int mode)
{
	return __al3010_write_reg(client, AL3010_MODE_COMMAND,
		AL3010_MODE_MASK, AL3010_MODE_SHIFT, mode);
}

/* power_state */
static int al3010_set_power_state(struct i2c_client *client, int state)
{
	return __al3010_write_reg(client, AL3010_MODE_COMMAND,
				AL3010_POW_MASK, AL3010_POW_SHIFT, 
				state ? AL3010_POW_UP : AL3010_POW_DOWN);
}

static int al3010_hw_init(struct i2c_client *client)//cj
{
	struct ltr301_data *data = i2c_get_clientdata(client);
	int i;
	printk("cj+++al3010_hw_init begin\n");
	/* read all the registers once to fill the cache.
	 * if one of the reads fails, we consider the init failed */
#if 1
	for (i = 0; i < ARRAY_SIZE(data->reg_cache); i++) {
		int v = i2c_smbus_read_byte_data(client, al3010_reg[i]);
		if (v < 0)
			return -ENODEV;

		data->reg_cache[i] = v;
	}
#endif
		/* set defaults */
	al3010_set_range(client, 0);
	//i2c_smbus_write_byte_data(client,0x10,0x00);
	al3010_set_mode(client, 0);
	//i2c_smbus_write_byte_data(client,0x00,0x1);
	
	al3010_set_power_state(client, 0);//
		//i2c_smbus_write_byte_data(client,0x00,1);
	printk("cj+++al3010_hw_init end\n");
	return 0;
}
#endif
static int ltr301_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int err = 0;
    //unsigned char pid;
    struct ltr301_data *data;
    struct input_dev *dev;
    
    printk(KERN_ERR"%s,%d\n", __func__, __LINE__);
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk(KERN_INFO "i2c_check_functionality error\n");
        goto exit;
    }
    data = kzalloc(sizeof(struct ltr301_data), GFP_KERNEL);
    if (!data) {
        err = -ENOMEM;
        goto exit;
    }
#if 0 //cj
    /* read chip id */
    pid = i2c_smbus_read_byte_data(client, PART_ID_REG);

    if (pid == LTR301_PART_ID) {
        printk(KERN_INFO "LTR-301 Device detected!\n");
    } else{
        printk(KERN_INFO "LTR-301 Device not found!\n"
                "i2c error %d \n", pid);
        err = -ENODEV;
        goto kfree_exit;
    }
	#else
	/*cj add al3010 hw init*/
    //al3010_hw_init(client);
#endif	

    i2c_set_clientdata(client, data);
    data->ltr301_client = client;
    mutex_init(&data->enable_mutex);
   mutex_init(&data->lock_al3010);


    INIT_DELAYED_WORK(&data->work, ltr301_work_func);
    atomic_set(&data->delay, MAX_DELAY);
    atomic_set(&data->enable, 0);

	al3010_hw_init(client);
	

    dev = input_allocate_device();
    if (!dev)
        return -ENOMEM;
    dev->name = SENSOR_NAME;
    dev->id.bustype = BUS_I2C;

    input_set_capability(dev, EV_ABS, ABS_MISC);
    input_set_abs_params(dev, ABS_MISC, ABSMIN, ABSMAX, 0, 0);
    input_set_drvdata(dev, data);

    err = input_register_device(dev);
    if (err < 0) {
        input_free_device(dev);
        goto kfree_exit;
    }

    data->input = dev;

    err = sysfs_create_group(&data->input->dev.kobj,
            &ltr301_attribute_group);
    if (err < 0)
        goto error_sysfs;

#ifdef CONFIG_HAS_EARLYSUSPEND
    data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    data->early_suspend.suspend = ltr301_early_suspend;
    data->early_suspend.resume = ltr301_early_resume;
    register_early_suspend(&data->early_suspend);
#endif

   // ltr301_set_mode(client, LTR301_MODE_ACTIVE);    cj
    return 0;

error_sysfs:
    input_unregister_device(data->input);

kfree_exit:
    kfree(data);
exit:
    return err;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void ltr301_early_suspend(struct early_suspend *h)
{
    struct ltr301_data *data =
        container_of(h, struct ltr301_data, early_suspend);

    mutex_lock(&data->enable_mutex);
    if (atomic_read(&data->enable) == 1) {
        ltr301_set_mode(data->ltr301_client, LTR301_MODE_STANDBY);
        cancel_delayed_work_sync(&data->work);
    }
    mutex_unlock(&data->enable_mutex);
}


static void ltr301_early_resume(struct early_suspend *h)
{
    struct ltr301_data *data =
        container_of(h, struct ltr301_data, early_suspend);

    mutex_lock(&data->enable_mutex);
    if (atomic_read(&data->enable) == 1) {
        ltr301_set_mode(data->ltr301_client, LTR301_MODE_ACTIVE);
        schedule_delayed_work(&data->work,
                msecs_to_jiffies(atomic_read(&data->delay)));
    }
    mutex_unlock(&data->enable_mutex);
}
#endif

static int  ltr301_remove(struct i2c_client *client)
{
    struct ltr301_data *data = i2c_get_clientdata(client);

    ltr301_set_enable(&client->dev, 0);
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&data->early_suspend);
#endif
    sysfs_remove_group(&data->input->dev.kobj, &ltr301_attribute_group);
    input_unregister_device(data->input);
    kfree(data);

    return 0;
}

#ifdef CONFIG_PM

static int ltr301_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct ltr301_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->enable_mutex);
    if (atomic_read(&data->enable) == 1) {
        ltr301_set_mode(data->ltr301_client, LTR301_MODE_STANDBY);
        cancel_delayed_work_sync(&data->work);
    }
    mutex_unlock(&data->enable_mutex);

    return 0;
}

static int ltr301_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct ltr301_data *data = i2c_get_clientdata(client);

    mutex_lock(&data->enable_mutex);
    if (atomic_read(&data->enable) == 1) {
        ltr301_set_mode(data->ltr301_client, LTR301_MODE_ACTIVE);
        schedule_delayed_work(&data->work,
                msecs_to_jiffies(atomic_read(&data->delay)));
    }
    mutex_unlock(&data->enable_mutex);

    return 0;
}

#else

#define ltr301_suspend        NULL
#define ltr301_resume        NULL

#endif /* CONFIG_PM */

#if CFG_GSENSOR_USE_CONFIG
static int gsensor_of_data_get(void)
{
    int ret = -1;
    struct device_node *of_node;
    
    printk(KERN_ERR"%s,%d\n", __func__, __LINE__);

    of_node = of_find_compatible_node(NULL, NULL, "actions,gsensor");
    if (NULL == of_node){
	printk(KERN_ERR"%s,%d, get gyrosensor compatible fail!\n", __func__, __LINE__);
	return -1;
    }

    ret = of_property_read_u32(of_node, "adapter_id", &cfg_i2c_adap_id);
    if (ret == 0) {
        cfg_i2c_adap_id = GSENSOR_I2C_ADAPTER;
        printk(KERN_ERR"%s,%d\n", __func__, __LINE__);
    }
    printk(KERN_ERR"%s,%d, cfg_i2c_adap_id:%d\n", __func__, __LINE__, cfg_i2c_adap_id);
    return ret;
}
#endif

static SIMPLE_DEV_PM_OPS(ltr301_pm_ops, ltr301_suspend, ltr301_resume);

static const struct i2c_device_id ltr301_id[] = {
    { SENSOR_NAME, 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, ltr301_id);

static struct of_device_id ltr301_of_match[] = {
	{ .compatible = SENSOR_NAME },
	{ }
};
static struct i2c_driver ltr301_driver = {
    .driver = {
        .owner    = THIS_MODULE,
        .name    = SENSOR_NAME,
        .pm    = &ltr301_pm_ops,
		.of_match_table	= of_match_ptr(ltr301_of_match),
    },
    .class        = I2C_CLASS_HWMON,
    .id_table    = ltr301_id,
    .probe        = ltr301_probe,
    .remove        = ltr301_remove,
};

static int __init ltr301_init(void)
{
    return i2c_add_driver(&ltr301_driver);
}

static void __exit ltr301_exit(void)
{
    i2c_del_driver(&ltr301_driver);
}

MODULE_AUTHOR("Zhining Song <songzhining@actions-semi.com>");
MODULE_DESCRIPTION("LTR-301 light sensor driver");
MODULE_LICENSE("GPL");

module_init(ltr301_init);
module_exit(ltr301_exit);

