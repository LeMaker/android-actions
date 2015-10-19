
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

// device info
#define SENSOR_NAME                   "mc6420"
#define SENSOR_I2C_ADDR            0x1e
#define ABSMIN                              -32768
#define ABSMAX                             32767
#define MAX_DELAY                       100

// constant define
#define MC6420_PRODUCT_CODE          0x48

#define MC6420_ACQ_OLDDATA           0x00
#define MC6420_ACQ_NEWDATA           0x01

#define MC6420_MEAS_NORMAL            0x00
#define MC6420_MEAS_P_SELFTEST       0x01
#define MC6420_MEAS_N_SELFTEST       0x02

#define MC6420_MODE_STANDBY          0x00
#define MC6420_MODE_CONTINUOUS    0x01
#define MC6420_MODE_SINGLE              0x02

#define MC6420_RANGE_200UT             0x01
#define MC6420_RANGE_400UT             0x02
#define MC6420_RANGE_600UT             0x03
#define MC6420_RANGE_800UT             0x04
#define MC6420_RANGE_1200UT           0x05

#define MC6420_RATE_25HZ                 (1000/25)
#define MC6420_RATE_50HZ                 (1000/50)
#define MC6420_RATE_100HZ               (1000/100)
#define MC6420_RATE_125HZ               (1000/125)

// register define
#define MC6420_CH1_LSB_REG              0x00
#define MC6420_CH1_MSB_REG              0x01
#define MC6420_CH2_LSB_REG              0x02
#define MC6420_CH2_MSB_REG              0x03
#define MC6420_CH3_LSB_REG              0x04
#define MC6420_CH3_MSB_REG              0x05
#define MC6420_STATUS_REG               0x06
#define MC6420_SETUP1_REG                0x0a
#define MC6420_SETUP2_REG                0x0b
#define MC6420_SETUP5_REG                0x0e
#define MC6420_SETUP6_REG                0x0f
#define MC6420_SETUP7_REG                0x10
#define MC6420_ENABLE1_REG              0x13
#define MC6420_ENABLE2_REG              0x14
#define MC6420_PCODE_REG                 0x16
#define MC6420_POR_REG                     0x18
#define MC6420_RATE_REG                   0x19

// register bits define
#define MC6420_ACQ_BIT__POS            0
#define MC6420_ACQ_BIT__LEN            1
#define MC6420_ACQ_BIT__MSK            0x01
#define MC6420_ACQ_BIT__REG            MC6420_STATUS_REG

#define MC6420_MEAS_BIT__POS            0
#define MC6420_MEAS_BIT__LEN            2
#define MC6420_MEAS_BIT__MSK            0x03
#define MC6420_MEAS_BIT__REG            MC6420_SETUP1_REG

#define MC6420_MODE_BIT__POS            0
#define MC6420_MODE_BIT__LEN            2
#define MC6420_MODE_BIT__MSK            0x03
#define MC6420_MODE_BIT__REG            MC6420_SETUP2_REG

#define MC6420_RANGE_BIT__POS            4
#define MC6420_RANGE_BIT__LEN            4
#define MC6420_RANGE_BIT__MSK            0xf0
#define MC6420_RANGE_BIT__REG            MC6420_SETUP6_REG

#define MC6420_HF_BIT__POS            7
#define MC6420_HF_BIT__LEN            1
#define MC6420_HF_BIT__MSK            0x80
#define MC6420_HF_BIT__REG            MC6420_SETUP7_REG

#define MC6420_POR_BIT__POS            0
#define MC6420_POR_BIT__LEN            1
#define MC6420_POR_BIT__MSK            0x01
#define MC6420_POR_BIT__REG            MC6420_POR_REG



#define MC6420_GET_BITSLICE(regvar, bitname)\
    ((regvar & bitname##__MSK) >> bitname##__POS)

#define MC6420_SET_BITSLICE(regvar, bitname, val)\
    ((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))


struct mc6420_ut{
    s16    x;
    s16    y;
    s16    z;
} ;

struct mc6420_data {
    struct i2c_client *mc6420_client;
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
#define CFG_SENSOR_USE_CONFIG  0

/*******************************************
* for xml cfg
*******************************************/
#define CFG_SENSOR_ADAP_ID          "gsensor.i2c_adap_id"
#define CFG_SENSOR_POSITION         "gsensor.position"

extern int get_config(const char *key, char *buff, int len);
/*******************************************
* end for xml cfg
*******************************************/

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mc6420_early_suspend(struct early_suspend *h);
static void mc6420_early_resume(struct early_suspend *h);
#endif

static int mc6420_axis_remap(struct i2c_client *client, struct mc6420_ut *ut);

static int mc6420_smbus_read_byte(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data)
{
    s32 dummy;
    dummy = i2c_smbus_read_byte_data(client, reg_addr);
    if (dummy < 0)
        return -1;
    *data = dummy & 0x000000ff;

    return 0;
}

static int mc6420_smbus_write_byte(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data)
{
    s32 dummy;
    dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
    if (dummy < 0)
        return -1;
    return 0;
}

static int mc6420_smbus_read_byte_block(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data, unsigned char len)
{
    s32 dummy;
    dummy = i2c_smbus_read_i2c_block_data(client, reg_addr, len, data);
    if (dummy < 0)
        return -1;
    return 0;
}

static int mc6420_smbus_write_byte_block(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data, unsigned char len)
{
    s32 dummy;    
    dummy = i2c_smbus_write_i2c_block_data(client, reg_addr, len, data);
        if (dummy < 0)
            return -1;    
    return 0;
}

static int mc6420_get_acq(struct i2c_client *client, unsigned char *acq)
{
    int comres = 0;
    unsigned char data = 0;

    comres = mc6420_smbus_read_byte(client, MC6420_ACQ_BIT__REG, &data);
    *acq  = MC6420_GET_BITSLICE(data, MC6420_ACQ_BIT);

    return comres;
}

static int mc6420_set_measure(struct i2c_client *client, unsigned char mode)
{
    int comres = 0;
    unsigned char data = 0;

    comres = mc6420_smbus_read_byte(client, MC6420_MEAS_BIT__REG, &data);    
    data  = MC6420_SET_BITSLICE(data, MC6420_MEAS_BIT, mode);
    comres += mc6420_smbus_write_byte(client, MC6420_MEAS_BIT__REG, &data);

    return comres;
}

static int mc6420_get_measure(struct i2c_client *client, unsigned char *mode)
{
    int comres = 0;
    unsigned char data = 0;

    comres = mc6420_smbus_read_byte(client, MC6420_MEAS_BIT__REG, &data);
    *mode  = MC6420_GET_BITSLICE(data, MC6420_MEAS_BIT);

    return comres;
}

static int mc6420_set_mode(struct i2c_client *client, unsigned char mode)
{
    int comres = 0;
    unsigned char data = 0;

    comres = mc6420_smbus_read_byte(client, MC6420_MODE_BIT__REG, &data);    
    data  = MC6420_SET_BITSLICE(data, MC6420_MODE_BIT, mode);
    comres += mc6420_smbus_write_byte(client, MC6420_MODE_BIT__REG, &data);

    return comres;
}

static int mc6420_get_mode(struct i2c_client *client, unsigned char *mode)
{
    int comres = 0;
    unsigned char data = 0;

    comres = mc6420_smbus_read_byte(client, MC6420_MODE_BIT__REG, &data);
    *mode  = MC6420_GET_BITSLICE(data, MC6420_MODE_BIT);

    return comres;
}

static int mc6420_set_range(struct i2c_client *client, unsigned char range)
{
    int comres = 0;
    unsigned char data = 0;

    comres = mc6420_smbus_read_byte(client, MC6420_RANGE_BIT__REG, &data);    
    data  = MC6420_SET_BITSLICE(data, MC6420_RANGE_BIT, range);
    comres += mc6420_smbus_write_byte(client, MC6420_RANGE_BIT__REG, &data);

    return comres;
}

static int mc6420_get_range(struct i2c_client *client, unsigned char *range)
{
    int comres = 0;
    unsigned char data = 0;

    comres = mc6420_smbus_read_byte(client, MC6420_RANGE_BIT__REG, &data);
    *range  = MC6420_GET_BITSLICE(data, MC6420_RANGE_BIT);

    return comres;
}

static int mc6420_set_rate(struct i2c_client *client, unsigned char rate)
{
    int comres = 0;
    unsigned char data = 0;

    // set HF=1
    comres += mc6420_smbus_read_byte(client, MC6420_HF_BIT__REG, &data);    
    data  = MC6420_SET_BITSLICE(data, MC6420_HF_BIT, 1);
    comres += mc6420_smbus_write_byte(client, MC6420_HF_BIT__REG, &data);
    
    // set rate
    comres += mc6420_smbus_write_byte(client, MC6420_RATE_REG, &rate);

    return comres;
}

static int mc6420_get_rate(struct i2c_client *client, unsigned char *rate)
{
    int comres = 0;
    unsigned char data = 0;

    comres = mc6420_smbus_read_byte(client, MC6420_RATE_REG, &data);
    *rate  = data;

    return comres;
}

static int mc6420_reset(struct i2c_client *client)
{
    int comres = 0;
    unsigned char data = 0;

    comres = mc6420_smbus_read_byte(client, MC6420_POR_BIT__REG, &data);    
    data  = MC6420_SET_BITSLICE(data, MC6420_POR_BIT, 1);
    comres += mc6420_smbus_write_byte(client, MC6420_POR_BIT__REG, &data);

    return comres;
}

static int mc6420_hw_init(struct i2c_client *client)
{
    // reset
    mc6420_reset(client);
    
    // normal measure
    mc6420_set_measure(client, MC6420_MEAS_NORMAL);
    
    // range: -200uT ~ +200uT
    mc6420_set_range(client, MC6420_RANGE_200UT);
    
    // sample rate: 125Hz
    mc6420_set_rate(client, MC6420_RATE_125HZ);
    
    return 0;
}

static int mc6420_read_data(struct i2c_client *client, struct mc6420_ut *ut)
{
    int comres = 0;
    unsigned char data[6];
    unsigned char acq;

    // check new data bit
    comres += mc6420_get_acq(client, &acq);
    
    if (acq == MC6420_ACQ_NEWDATA) {
        comres += mc6420_smbus_read_byte_block(client, MC6420_CH1_LSB_REG, data, 6);        
        ut->x = ((signed short)data[0])|(((signed short)data[1])<<8);
        ut->y = ((signed short)data[2])|(((signed short)data[3])<<8);
        ut->z = ((signed short)data[4])|(((signed short)data[5])<<8);
    } else {
        printk(KERN_ERR "acq bit: %d\n", acq);
        comres = -1;
    }

    return comres;
}


static ssize_t mc6420_register_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int address, value;
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    sscanf(buf, "[0x%x]=0x%x", &address, &value);
    
    if (mc6420_smbus_write_byte(mc6420->mc6420_client, (unsigned char)address,
                (unsigned char *)&value) < 0)
        return -EINVAL;

    return count;
}

static ssize_t mc6420_register_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);    
    size_t count = 0;
    u8 reg[0x1a];
    int i;
    
    for (i = 0 ; i < 0x1a; i++) {
        mc6420_smbus_read_byte(mc6420->mc6420_client, i, reg+i);    
        count += sprintf(&buf[count], "0x%x: 0x%x\n", i, reg[i]);
    }
    
    return count;
}

static ssize_t mc6420_measure_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    if (mc6420_get_measure(mc6420->mc6420_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t mc6420_measure_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (mc6420_set_measure(mc6420->mc6420_client, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t mc6420_mode_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    if (mc6420_get_mode(mc6420->mc6420_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t mc6420_mode_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (mc6420_set_mode(mc6420->mc6420_client, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t mc6420_range_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    if (mc6420_get_range(mc6420->mc6420_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t mc6420_range_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (mc6420_set_range(mc6420->mc6420_client, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t mc6420_rate_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    if (mc6420_get_rate(mc6420->mc6420_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t mc6420_rate_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (mc6420_set_rate(mc6420->mc6420_client, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t mc6420_reset_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    mc6420_reset(mc6420->mc6420_client);
    return count;
}

static ssize_t mc6420_value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct input_dev *input = to_input_dev(dev);
    struct mc6420_data *mc6420 = input_get_drvdata(input);
    struct mc6420_ut ut;
    int result;
    int retry = 10;

    do {
        result = mc6420_read_data(mc6420->mc6420_client, &ut);
        mc6420_axis_remap(mc6420->mc6420_client, &ut);   
        retry = retry - 1;
    }while ((result != 0) && (retry > 0));
    
    if (retry > 0) { 
        return sprintf(buf, "%d %d %d\n", ut.x, ut.y, ut.z);
    } else {
        return sprintf(buf, "read data failed!\n");
    }
}

static ssize_t mc6420_delay_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&mc6420->delay));

}

static ssize_t mc6420_delay_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&mc6420->delay, (unsigned int) data);

    return count;
}


static ssize_t mc6420_enable_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&mc6420->enable));

}

static void mc6420_set_enable(struct device *dev, int enable)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);
    int pre_enable = atomic_read(&mc6420->enable);

    mutex_lock(&mc6420->enable_mutex);
    if (enable) {
        if (pre_enable == 0) {
            mc6420_set_mode(mc6420->mc6420_client, MC6420_MODE_CONTINUOUS);
            schedule_delayed_work(&mc6420->work,
                msecs_to_jiffies(atomic_read(&mc6420->delay)));
            atomic_set(&mc6420->enable, 1);
        }

    } else {
        if (pre_enable == 1) {
            mc6420_set_mode(mc6420->mc6420_client, MC6420_MODE_STANDBY);
            cancel_delayed_work_sync(&mc6420->work);
            atomic_set(&mc6420->enable, 0);
        }
    }
    mutex_unlock(&mc6420->enable_mutex);
}

static ssize_t mc6420_enable_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if ((data == 0) || (data == 1))
        mc6420_set_enable(dev, data);

    return count;
}

static ssize_t mc6420_board_position_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int data;
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    data = atomic_read(&(mc6420->position));

    return sprintf(buf, "%d\n", data);
}

static ssize_t mc6420_board_position_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);

    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&(mc6420->position), (int) data);

    return count;
}

static DEVICE_ATTR(reg, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc6420_register_show, mc6420_register_store);
static DEVICE_ATTR(measure, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc6420_measure_show, mc6420_measure_store);
static DEVICE_ATTR(mode, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc6420_mode_show, mc6420_mode_store);
static DEVICE_ATTR(range, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc6420_range_show, mc6420_range_store);
static DEVICE_ATTR(rate, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc6420_rate_show, mc6420_rate_store);
static DEVICE_ATTR(reset, S_IWUSR|S_IWGRP|S_IWOTH,
        NULL, mc6420_reset_store);
static DEVICE_ATTR(value, S_IRUGO,
        mc6420_value_show, NULL);
static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc6420_delay_show, mc6420_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc6420_enable_show, mc6420_enable_store);
static DEVICE_ATTR(board_position, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc6420_board_position_show, mc6420_board_position_store);

static struct attribute *mc6420_attributes[] = {
    &dev_attr_reg.attr,
    &dev_attr_measure.attr,
    &dev_attr_mode.attr,
    &dev_attr_range.attr,
    &dev_attr_rate.attr,
    &dev_attr_reset.attr,
    &dev_attr_value.attr,
    &dev_attr_delay.attr,
    &dev_attr_enable.attr,
    &dev_attr_board_position.attr,
    NULL
};

static struct attribute_group mc6420_attribute_group = {
    .attrs = mc6420_attributes
};

static int mc6420_axis_remap(struct i2c_client *client, struct mc6420_ut *ut)
{
    s16 swap;
    struct mc6420_data *mc6420 = i2c_get_clientdata(client);
    int position = atomic_read(&mc6420->position);

    switch (abs(position)) {
        case 1:
            break;
        case 2:
            swap = ut->x;
            ut->x = ut->y;
            ut->y = -swap; 
            break;
        case 3:
            ut->x = -(ut->x);
            ut->y = -(ut->y);
            break;
        case 4:
            swap = ut->x;
            ut->x = -ut->y;
            ut->y = swap;
            break;
    }
    
    if (position < 0) {
        ut->z = -(ut->z);
        ut->x = -(ut->x);
    }
    
    return 0;
}

static void mc6420_work_func(struct work_struct *work)
{
    struct mc6420_data *mc6420 = container_of((struct delayed_work *)work,
            struct mc6420_data, work);
    static struct mc6420_ut ut;    
    int result;
    unsigned long delay = msecs_to_jiffies(atomic_read(&mc6420->delay));
    
    result = mc6420_read_data(mc6420->mc6420_client, &ut);
    if (result == 0) {
        mc6420_axis_remap(mc6420->mc6420_client, &ut);
        
        input_report_abs(mc6420->input, ABS_X, ut.x);
        input_report_abs(mc6420->input, ABS_Y, ut.y);
        input_report_abs(mc6420->input, ABS_Z, ut.z);
        input_sync(mc6420->input);
    }
    schedule_delayed_work(&mc6420->work, delay);
}

static int mc6420_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int err = 0;
    unsigned char pid;
    struct mc6420_data *data;
    struct input_dev *dev;
    int cfg_position;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk(KERN_INFO "i2c_check_functionality error\n");
        goto exit;
    }
    data = kzalloc(sizeof(struct mc6420_data), GFP_KERNEL);
    if (!data) {
        err = -ENOMEM;
        goto exit;
    }

    /* read chip id */
    pid = i2c_smbus_read_byte_data(client, MC6420_PCODE_REG);

    if (pid == MC6420_PRODUCT_CODE) {
        printk(KERN_INFO "MC6420 Device detected!\n");
    } else{
        printk(KERN_INFO "MC6420 Device not found!\n"
                "i2c error %d \n", pid);
        err = -ENODEV;
        goto kfree_exit;
    }
    i2c_set_clientdata(client, data);
    data->mc6420_client = client;
    mutex_init(&data->enable_mutex);

    INIT_DELAYED_WORK(&data->work, mc6420_work_func);
    atomic_set(&data->delay, MAX_DELAY);
    atomic_set(&data->enable, 0);

#if CFG_SENSOR_USE_CONFIG > 0
        /*get xml cfg*/
        err = get_config(CFG_SENSOR_POSITION, (char *)(&cfg_position), sizeof(int));
        if (err != 0) {
            printk(KERN_ERR"get position %d fail\n", cfg_position);
            goto exit_kfree;
        }
#else
        cfg_position = 3;
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
            &mc6420_attribute_group);
    if (err < 0)
        goto error_sysfs;

#ifdef CONFIG_HAS_EARLYSUSPEND
    data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    data->early_suspend.suspend = mc6420_early_suspend;
    data->early_suspend.resume = mc6420_early_resume;
    register_early_suspend(&data->early_suspend);
#endif

    //power on init regs    
    mc6420_hw_init(data->mc6420_client); 
      
    return 0;

error_sysfs:
    input_unregister_device(data->input);

kfree_exit:
    kfree(data);
exit:
    return err;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mc6420_early_suspend(struct early_suspend *h)
{
    struct mc6420_data *data =
        container_of(h, struct mc6420_data, early_suspend);

    mc6420_set_enable(&data->mc6420_client->dev, 0);
}


static void mc6420_early_resume(struct early_suspend *h)
{
    struct mc6420_data *data =
        container_of(h, struct mc6420_data, early_suspend);

    mc6420_set_enable(&data->mc6420_client->dev, 1);
}
#endif

static int __devexit mc6420_remove(struct i2c_client *client)
{
    struct mc6420_data *data = i2c_get_clientdata(client);

    mc6420_set_enable(&client->dev, 0);
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&data->early_suspend);
#endif
    sysfs_remove_group(&data->input->dev.kobj, &mc6420_attribute_group);
    input_unregister_device(data->input);
    kfree(data);

    return 0;
}

#ifdef CONFIG_PM

static int mc6420_suspend(struct device *dev)
{
    mc6420_set_enable(dev, 0);
    
    return 0;
}

static int mc6420_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct mc6420_data *data = i2c_get_clientdata(client);
    
    //power on init regs    
    mc6420_hw_init(data->mc6420_client);     
    mc6420_set_enable(dev, 1);
    
    return 0;
}

#else

#define mc6420_suspend        NULL
#define mc6420_resume        NULL

#endif /* CONFIG_PM */

static SIMPLE_DEV_PM_OPS(mc6420_pm_ops, mc6420_suspend, mc6420_resume);

static const unsigned short  mc6420_addresses[] = {
    SENSOR_I2C_ADDR,
    I2C_CLIENT_END,
};

static const struct i2c_device_id mc6420_id[] = {
    { SENSOR_NAME, 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, mc6420_id);

static struct i2c_driver mc6420_driver = {
    .driver = {
        .owner    = THIS_MODULE,
        .name    = SENSOR_NAME,
        .pm    = &mc6420_pm_ops,
    },
    .class        = I2C_CLASS_HWMON,
//    .address_list    = mc6420_addresses,
    .id_table    = mc6420_id,
    .probe        = mc6420_probe,
    .remove        = __devexit_p(mc6420_remove),

};

static struct i2c_board_info mc6420_board_info={
    .type = SENSOR_NAME, 
    .addr = SENSOR_I2C_ADDR,
};

static struct i2c_client *mc6420_client;

static int __init mc6420_init(void)
{
    struct i2c_adapter *i2c_adap;
    unsigned int cfg_i2c_adap_id;

#if CFG_SENSOR_USE_CONFIG > 0
    int ret;
    
    /*get xml cfg*/
    ret = get_config(CFG_SENSOR_ADAP_ID, (char *)(&cfg_i2c_adap_id), sizeof(unsigned int));
    if (ret != 0) {
        printk(KERN_ERR"get i2c_adap_id %d fail\n", cfg_i2c_adap_id);
        return ret;
    }
#else
    cfg_i2c_adap_id = 2;
#endif
    
    i2c_adap = i2c_get_adapter(cfg_i2c_adap_id);  
    mc6420_client = i2c_new_device(i2c_adap, &mc6420_board_info);  
    i2c_put_adapter(i2c_adap);
    
    return i2c_add_driver(&mc6420_driver);
}

static void __exit mc6420_exit(void)
{
    i2c_unregister_device(mc6420_client);
    i2c_del_driver(&mc6420_driver);
}

MODULE_AUTHOR("Zhining Song <songzhining@actions-semi.com>");
MODULE_DESCRIPTION("MC6420 3-axis Magnetic field sensor driver");
MODULE_LICENSE("GPL");

module_init(mc6420_init);
module_exit(mc6420_exit);

