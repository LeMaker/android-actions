/* file bma222.c
   brief This file contains all function implementations for the bma222 in linux
 */
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
//#include "gsensor_common.h"

#define SENSOR_NAME     "bma222"
#define ABSMIN                -512
#define ABSMAX                512
#define FUZZ                     0

#define SENSOR_NAME2    "bma222t"
#define ABSMIN2                (24*2-128)
#define ABSMAX2               (24*2+127)
#define FUZZ2                    1

#define bma222_MAX_DELAY        200
#define bma222_CHIP_ID            2
#define bma222E_CHIP_ID            248
#define bma222_RANGE_SET        0
#define bma222_BW_SET            4

#define bma222_CHIP_ID_REG                      0x00
#define bma222_VERSION_REG                      0x01
#define bma222_X_AXIS_LSB_REG                   0x02
#define bma222_X_AXIS_MSB_REG                   0x03
#define bma222_Y_AXIS_LSB_REG                   0x04
#define bma222_Y_AXIS_MSB_REG                   0x05
#define bma222_Z_AXIS_LSB_REG                   0x06
#define bma222_Z_AXIS_MSB_REG                   0x07
#define bma222_TEMP_RD_REG                      0x08
#define bma222_RANGE_SEL_REG                    0x0F
#define bma222_BW_SEL_REG                       0x10
#define bma222_MODE_CTRL_REG                    0x11
#define bma222_SELF_TEST_REG                    0x32
#define bma222_OFFSET_CTRL_REG                  0x36
#define bma222_OFFSET_PARAMS_REG                0x37
#define bma222_OFFSET_FILT_X_REG                0x38
#define bma222_OFFSET_FILT_Y_REG                0x39
#define bma222_OFFSET_FILT_Z_REG                0x3A

#define bma222_ACC_X_LSB__POS           0
#define bma222_ACC_X_LSB__LEN           0
#define bma222_ACC_X_LSB__MSK           0x00
#define bma222_ACC_X_LSB__REG           bma222_X_AXIS_LSB_REG

#define bma222_ACC_X_MSB__POS           0
#define bma222_ACC_X_MSB__LEN           8
#define bma222_ACC_X_MSB__MSK           0xFF
#define bma222_ACC_X_MSB__REG           bma222_X_AXIS_MSB_REG

#define bma222_ACC_Y_LSB__POS           0
#define bma222_ACC_Y_LSB__LEN           0
#define bma222_ACC_Y_LSB__MSK           0x00
#define bma222_ACC_Y_LSB__REG           bma222_Y_AXIS_LSB_REG

#define bma222_ACC_Y_MSB__POS           0
#define bma222_ACC_Y_MSB__LEN           8
#define bma222_ACC_Y_MSB__MSK           0xFF
#define bma222_ACC_Y_MSB__REG           bma222_Y_AXIS_MSB_REG

#define bma222_ACC_Z_LSB__POS           0
#define bma222_ACC_Z_LSB__LEN           0
#define bma222_ACC_Z_LSB__MSK           0x00
#define bma222_ACC_Z_LSB__REG           bma222_Z_AXIS_LSB_REG

#define bma222_ACC_Z_MSB__POS           0
#define bma222_ACC_Z_MSB__LEN           8
#define bma222_ACC_Z_MSB__MSK           0xFF
#define bma222_ACC_Z_MSB__REG           bma222_Z_AXIS_MSB_REG

#define bma222_RANGE_SEL__POS             0
#define bma222_RANGE_SEL__LEN             4
#define bma222_RANGE_SEL__MSK             0x0F
#define bma222_RANGE_SEL__REG             bma222_RANGE_SEL_REG

#define bma222_BANDWIDTH__POS             0
#define bma222_BANDWIDTH__LEN             5
#define bma222_BANDWIDTH__MSK             0x1F
#define bma222_BANDWIDTH__REG             bma222_BW_SEL_REG

#define bma222_EN_LOW_POWER__POS          6
#define bma222_EN_LOW_POWER__LEN          1
#define bma222_EN_LOW_POWER__MSK          0x40
#define bma222_EN_LOW_POWER__REG          bma222_MODE_CTRL_REG

#define bma222_EN_SUSPEND__POS          5
#define bma222_EN_SUSPEND__LEN            1
#define bma222_EN_SUSPEND__MSK            0x20
#define bma222_EN_SUSPEND__REG            bma222_MODE_CTRL_REG

#define bma222_EN_SELF_TEST__POS                0
#define bma222_EN_SELF_TEST__LEN                2
#define bma222_EN_SELF_TEST__MSK                0x03
#define bma222_EN_SELF_TEST__REG                bma222_SELF_TEST_REG

#define bma222_NEG_SELF_TEST__POS               2
#define bma222_NEG_SELF_TEST__LEN               1
#define bma222_NEG_SELF_TEST__MSK               0x04
#define bma222_NEG_SELF_TEST__REG               bma222_SELF_TEST_REG

#define bma222_RESET_FAST_COMP__POS                7
#define bma222_RESET_FAST_COMP__LEN                1
#define bma222_RESET_FAST_COMP__MSK                0x80
#define bma222_RESET_FAST_COMP__REG                bma222_OFFSET_CTRL_REG

#define bma222_EN_FAST_COMP__POS                5
#define bma222_EN_FAST_COMP__LEN                2
#define bma222_EN_FAST_COMP__MSK                0x60
#define bma222_EN_FAST_COMP__REG                bma222_OFFSET_CTRL_REG

#define bma222_FAST_COMP_RDY_S__POS             4
#define bma222_FAST_COMP_RDY_S__LEN             1
#define bma222_FAST_COMP_RDY_S__MSK             0x10
#define bma222_FAST_COMP_RDY_S__REG             bma222_OFFSET_CTRL_REG

#define bma222_COMP_TARGET_OFFSET_X__POS        1
#define bma222_COMP_TARGET_OFFSET_X__LEN        2
#define bma222_COMP_TARGET_OFFSET_X__MSK        0x06
#define bma222_COMP_TARGET_OFFSET_X__REG        bma222_OFFSET_PARAMS_REG

#define bma222_COMP_TARGET_OFFSET_Y__POS        3
#define bma222_COMP_TARGET_OFFSET_Y__LEN        2
#define bma222_COMP_TARGET_OFFSET_Y__MSK        0x18
#define bma222_COMP_TARGET_OFFSET_Y__REG        bma222_OFFSET_PARAMS_REG

#define bma222_COMP_TARGET_OFFSET_Z__POS        5
#define bma222_COMP_TARGET_OFFSET_Z__LEN        2
#define bma222_COMP_TARGET_OFFSET_Z__MSK        0x60
#define bma222_COMP_TARGET_OFFSET_Z__REG        bma222_OFFSET_PARAMS_REG

#define bma222_RANGE_2G                 0
#define bma222_RANGE_4G                 1
#define bma222_RANGE_8G                 2
#define bma222_RANGE_16G                3

#define bma222_BW_7_81HZ        0x08
#define bma222_BW_15_63HZ       0x09
#define bma222_BW_31_25HZ       0x0A
#define bma222_BW_62_50HZ       0x0B
#define bma222_BW_125HZ         0x0C
#define bma222_BW_250HZ         0x0D
#define bma222_BW_500HZ         0x0E
#define bma222_BW_1000HZ        0x0F

#define bma222_MODE_NORMAL      0
#define bma222_MODE_LOWPOWER    1
#define bma222_MODE_SUSPEND     2


#define bma222_GET_BITSLICE(regvar, bitname)\
    ((regvar & bitname##__MSK) >> bitname##__POS)


#define bma222_SET_BITSLICE(regvar, bitname, val)\
    ((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))


struct bma222acc{
    s16    x,
        y,
        z,
        temp;
} ;

struct bma222_data {
    struct i2c_client *bma222_client;
    struct mutex enable_mutex;
    atomic_t selftest_result;
    
    atomic_t delay;
    atomic_t enable;
    struct input_dev *input;
    struct delayed_work work;
    
    atomic_t delay2;
    atomic_t enable2;
    struct input_dev *input2;
    struct delayed_work work2;
    
#ifdef CONFIG_HAS_EARLYSUSPEND
    struct early_suspend early_suspend;
#endif
    atomic_t fuzz;
    atomic_t position;
    atomic_t calibrated;
    unsigned char offset_saved[3];
};

// cfg data : 1-- used
#define CFG_GSENSOR_USE_CONFIG  1

// calibration file path
#define CFG_GSENSOR_CALIBFILE   "/data/data/com.actions.sensor.calib/files/gsensor_calib.txt"


#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma222_early_suspend(struct early_suspend *h);
static void bma222_late_resume(struct early_suspend *h);
#endif

static int bma222_smbus_read_byte(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data)
{
    s32 dummy;
    dummy = i2c_smbus_read_byte_data(client, reg_addr);
    if (dummy < 0)
        return -1;
    *data = dummy & 0x000000ff;

    return 0;
}

static int bma222_smbus_write_byte(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data)
{
    s32 dummy;
    dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
    if (dummy < 0)
        return -1;
    return 0;
}

static int bma222_smbus_read_byte_block(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data, unsigned char len)
{
    s32 dummy;
    dummy = i2c_smbus_read_i2c_block_data(client, reg_addr, len, data);
    if (dummy < 0)
        return -1;
    return 0;
}

static int bma222_smbus_write_byte_block(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data, unsigned char len)
{
    s32 dummy;
    s32 idx;
    
    //bma222 only support byte data write
    for (idx = 0; idx < len; idx ++)
    {
        dummy = i2c_smbus_write_byte_data(client, reg_addr+idx, data[idx]);
        if (dummy < 0)
            return -1;
    }
    
    return 0;
}
static int bma222_set_mode(struct i2c_client *client, unsigned char Mode)
{
    int comres = 0;
    unsigned char data1 = 0;

    if (Mode < 3) {
        comres = bma222_smbus_read_byte(client,
                bma222_EN_LOW_POWER__REG, &data1);
        switch (Mode) {
        case bma222_MODE_NORMAL:
            data1  = bma222_SET_BITSLICE(data1,
                    bma222_EN_LOW_POWER, 0);
            data1  = bma222_SET_BITSLICE(data1,
                    bma222_EN_SUSPEND, 0);
            break;
        case bma222_MODE_LOWPOWER:
            data1  = bma222_SET_BITSLICE(data1,
                    bma222_EN_LOW_POWER, 1);
            data1  = bma222_SET_BITSLICE(data1,
                    bma222_EN_SUSPEND, 0);
            break;
        case bma222_MODE_SUSPEND:
            data1  = bma222_SET_BITSLICE(data1,
                    bma222_EN_LOW_POWER, 0);
            data1  = bma222_SET_BITSLICE(data1,
                    bma222_EN_SUSPEND, 1);
            break;
        default:
            break;
        }

        comres += bma222_smbus_write_byte(client,
                bma222_EN_LOW_POWER__REG, &data1);
    } else{
        comres = -1;
    }

    return comres;
}

static int bma222_get_mode(struct i2c_client *client, unsigned char *Mode)
{
    int comres = 0;

    comres = bma222_smbus_read_byte(client,
            bma222_EN_LOW_POWER__REG, Mode);
    *Mode  = (*Mode) >> 6;

    return comres;
}

static int bma222_set_range(struct i2c_client *client, unsigned char Range)
{
    int comres = 0;
    unsigned char data1 = 0;

    if (Range < 4) {
        comres = bma222_smbus_read_byte(client,
                bma222_RANGE_SEL_REG, &data1);
        switch (Range) {
        case 0:
            data1  = bma222_SET_BITSLICE(data1,
                    bma222_RANGE_SEL, 3);
            break;
        case 1:
            data1  = bma222_SET_BITSLICE(data1,
                    bma222_RANGE_SEL, 5);
            break;
        case 2:
            data1  = bma222_SET_BITSLICE(data1,
                    bma222_RANGE_SEL, 8);
            break;
        case 3:
            data1  = bma222_SET_BITSLICE(data1,
                    bma222_RANGE_SEL, 12);
            break;
        default:
            break;
        }
        comres += bma222_smbus_write_byte(client,
                bma222_RANGE_SEL_REG, &data1);
    } else{
        comres = -1;
    }

    return comres;
}

static int bma222_get_range(struct i2c_client *client, unsigned char *Range)
{
    int comres = 0;
    unsigned char data;

    comres = bma222_smbus_read_byte(client, bma222_RANGE_SEL__REG,
            &data);
    data = bma222_GET_BITSLICE(data, bma222_RANGE_SEL);
    *Range = data;

    return comres;
}


static int bma222_set_bandwidth(struct i2c_client *client, unsigned char BW)
{
    int comres = 0;
    unsigned char data = 0;
    int Bandwidth = 0;

    if (BW < 8) {
        switch (BW) {
        case 0:
            Bandwidth = bma222_BW_7_81HZ;
            break;
        case 1:
            Bandwidth = bma222_BW_15_63HZ;
            break;
        case 2:
            Bandwidth = bma222_BW_31_25HZ;
            break;
        case 3:
            Bandwidth = bma222_BW_62_50HZ;
            break;
        case 4:
            Bandwidth = bma222_BW_125HZ;
            break;
        case 5:
            Bandwidth = bma222_BW_250HZ;
            break;
        case 6:
            Bandwidth = bma222_BW_500HZ;
            break;
        case 7:
            Bandwidth = bma222_BW_1000HZ;
            break;
        default:
            break;
        }
        comres = bma222_smbus_read_byte(client,
                bma222_BANDWIDTH__REG, &data);
        data = bma222_SET_BITSLICE(data, bma222_BANDWIDTH,
                Bandwidth);
        comres += bma222_smbus_write_byte(client,
                bma222_BANDWIDTH__REG, &data);
    } else{
        comres = -1;
    }

    return comres;
}

static int bma222_get_bandwidth(struct i2c_client *client, unsigned char *BW)
{
    int comres = 0;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client, bma222_BANDWIDTH__REG,
            &data);
    data = bma222_GET_BITSLICE(data, bma222_BANDWIDTH);
    if (data <= 8) {
        *BW = 0;
    } else{
        if (data >= 0x0F)
            *BW = 7;
        else
            *BW = data - 8;
    }

    return comres;
}

static int bma222_set_offset_target_x(struct i2c_client *client, unsigned char
        offsettarget)
{
    int comres = 0;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client,
            bma222_COMP_TARGET_OFFSET_X__REG, &data);
    data = bma222_SET_BITSLICE(data, bma222_COMP_TARGET_OFFSET_X,
            offsettarget);
    comres = bma222_smbus_write_byte(client,
            bma222_COMP_TARGET_OFFSET_X__REG, &data);

    return comres;
}

static int bma222_get_offset_target_x(struct i2c_client *client, unsigned char
        *offsettarget)
{
    int comres = 0 ;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client, bma222_OFFSET_PARAMS_REG,
            &data);
    data = bma222_GET_BITSLICE(data, bma222_COMP_TARGET_OFFSET_X);
    *offsettarget = data;

    return comres;
}

static int bma222_set_offset_target_y(struct i2c_client *client, unsigned char
        offsettarget)
{
    int comres = 0;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client,
            bma222_COMP_TARGET_OFFSET_Y__REG, &data);
    data = bma222_SET_BITSLICE(data, bma222_COMP_TARGET_OFFSET_Y,
            offsettarget);
    comres = bma222_smbus_write_byte(client,
            bma222_COMP_TARGET_OFFSET_Y__REG, &data);

    return comres;
}

static int bma222_get_offset_target_y(struct i2c_client *client, unsigned char
        *offsettarget)
{
    int comres = 0;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client, bma222_OFFSET_PARAMS_REG,
            &data);
    data = bma222_GET_BITSLICE(data, bma222_COMP_TARGET_OFFSET_Y);
    *offsettarget = data;

    return comres;
}

static int bma222_set_offset_target_z(struct i2c_client *client, unsigned char
        offsettarget)
{
    int comres = 0;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client,
            bma222_COMP_TARGET_OFFSET_Z__REG, &data);
    data = bma222_SET_BITSLICE(data, bma222_COMP_TARGET_OFFSET_Z,
            offsettarget);
    comres = bma222_smbus_write_byte(client,
            bma222_COMP_TARGET_OFFSET_Z__REG, &data);

    return comres;
}

static int bma222_get_offset_target_z(struct i2c_client *client, unsigned char
        *offsettarget)
{
    int comres = 0;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client, bma222_OFFSET_PARAMS_REG,
            &data);
    data = bma222_GET_BITSLICE(data, bma222_COMP_TARGET_OFFSET_Z);
    *offsettarget = data;

    return comres;
}

static int bma222_get_cal_ready(struct i2c_client *client, unsigned char
        *calrdy)
{
    int comres = 0;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client, bma222_OFFSET_CTRL_REG, &data);
    data = bma222_GET_BITSLICE(data, bma222_FAST_COMP_RDY_S);
    *calrdy = data;

    return comres;
}

static int bma222_reset_cal_offset(struct i2c_client *client)
{
    int comres = 0;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client, bma222_RESET_FAST_COMP__REG,
            &data);
    data = bma222_SET_BITSLICE(data, bma222_RESET_FAST_COMP, 1);
    comres = bma222_smbus_write_byte(client, bma222_RESET_FAST_COMP__REG,
            &data);

    return comres;
}

static int bma222_set_cal_trigger(struct i2c_client *client, unsigned char
        caltrigger)
{
    int comres = 0;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client, bma222_EN_FAST_COMP__REG,
            &data);
    data = bma222_SET_BITSLICE(data, bma222_EN_FAST_COMP, caltrigger);
    comres = bma222_smbus_write_byte(client, bma222_EN_FAST_COMP__REG,
            &data);

    return comres;
}

static int bma222_set_selftest_st(struct i2c_client *client, unsigned char
        selftest)
{
    int comres = 0;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client, bma222_EN_SELF_TEST__REG,
            &data);
    data = bma222_SET_BITSLICE(data, bma222_EN_SELF_TEST, selftest);
    comres = bma222_smbus_write_byte(client, bma222_EN_SELF_TEST__REG,
            &data);

    return comres;
}

static int bma222_set_selftest_stn(struct i2c_client *client, unsigned char stn)
{
    int comres = 0;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client, bma222_NEG_SELF_TEST__REG,
            &data);
    data = bma222_SET_BITSLICE(data, bma222_NEG_SELF_TEST, stn);
    comres = bma222_smbus_write_byte(client, bma222_NEG_SELF_TEST__REG,
            &data);

    return comres;
}
static int bma222_read_accel_x(struct i2c_client *client, short *a_x)
{
    int comres;
    unsigned char data[2] = {0};

    comres = bma222_smbus_read_byte_block(client, bma222_ACC_X_LSB__REG,
            data, 2);
    *a_x = bma222_GET_BITSLICE(data[0], bma222_ACC_X_LSB) |
        (bma222_GET_BITSLICE(data[1],
                     bma222_ACC_X_MSB)<<bma222_ACC_X_LSB__LEN);
    *a_x = *a_x <<
        (sizeof(short)*8-(bma222_ACC_X_LSB__LEN+bma222_ACC_X_MSB__LEN));
    *a_x = *a_x >>
        (sizeof(short)*8-(bma222_ACC_X_LSB__LEN+bma222_ACC_X_MSB__LEN));

    return comres;
}
static int bma222_read_accel_y(struct i2c_client *client, short *a_y)
{
    int comres;
    unsigned char data[2];

    comres = bma222_smbus_read_byte_block(client, bma222_ACC_Y_LSB__REG,
            data, 2);
    *a_y = bma222_GET_BITSLICE(data[0], bma222_ACC_Y_LSB) |
        (bma222_GET_BITSLICE(data[1],
                     bma222_ACC_Y_MSB)<<bma222_ACC_Y_LSB__LEN);
    *a_y = *a_y <<
        (sizeof(short)*8-(bma222_ACC_Y_LSB__LEN+bma222_ACC_Y_MSB__LEN));
    *a_y = *a_y >>
        (sizeof(short)*8-(bma222_ACC_Y_LSB__LEN+bma222_ACC_Y_MSB__LEN));

    return comres;
}

static int bma222_read_accel_z(struct i2c_client *client, short *a_z)
{
    int comres;
    unsigned char data[2];

    comres = bma222_smbus_read_byte_block(client, bma222_ACC_Z_LSB__REG,
            data, 2);
    *a_z = bma222_GET_BITSLICE(data[0], bma222_ACC_Z_LSB) |
        bma222_GET_BITSLICE(data[1],
                bma222_ACC_Z_MSB)<<bma222_ACC_Z_LSB__LEN;
    *a_z = *a_z <<
        (sizeof(short)*8-(bma222_ACC_Z_LSB__LEN+bma222_ACC_Z_MSB__LEN));
    *a_z = *a_z >>
        (sizeof(short)*8-(bma222_ACC_Z_LSB__LEN+bma222_ACC_Z_MSB__LEN));

    return comres;
}

static int bma222_read_accel_xyz(struct i2c_client *client,
        struct bma222acc *acc)
{
    int comres;
    unsigned char data[6];

    comres = bma222_smbus_read_byte_block(client,
            bma222_ACC_X_LSB__REG, data, 6);

    acc->x = bma222_GET_BITSLICE(data[0], bma222_ACC_X_LSB)
        |(bma222_GET_BITSLICE(data[1],
                bma222_ACC_X_MSB)<<bma222_ACC_X_LSB__LEN);
    acc->x = acc->x << (sizeof(short)*8-(bma222_ACC_X_LSB__LEN
                + bma222_ACC_X_MSB__LEN));
    acc->x = acc->x >> (sizeof(short)*8-(bma222_ACC_X_LSB__LEN
                + bma222_ACC_X_MSB__LEN));
    acc->y = bma222_GET_BITSLICE(data[2], bma222_ACC_Y_LSB)
        | (bma222_GET_BITSLICE(data[3],
                bma222_ACC_Y_MSB)<<bma222_ACC_Y_LSB__LEN);
    acc->y = acc->y << (sizeof(short)*8-(bma222_ACC_Y_LSB__LEN
                + bma222_ACC_Y_MSB__LEN));
    acc->y = acc->y >> (sizeof(short)*8-(bma222_ACC_Y_LSB__LEN
                + bma222_ACC_Y_MSB__LEN));

    acc->z = bma222_GET_BITSLICE(data[4], bma222_ACC_Z_LSB)
        | (bma222_GET_BITSLICE(data[5],
                bma222_ACC_Z_MSB)<<bma222_ACC_Z_LSB__LEN);
    acc->z = acc->z << (sizeof(short)*8-(bma222_ACC_Z_LSB__LEN
                + bma222_ACC_Z_MSB__LEN));
    acc->z = acc->z >> (sizeof(short)*8-(bma222_ACC_Z_LSB__LEN
                + bma222_ACC_Z_MSB__LEN));

    return comres;
}

static int bma222_read_temperature(struct i2c_client *client,
        struct bma222acc *acc)
{
    int comres;
    unsigned char data = 0;

    comres = bma222_smbus_read_byte(client, bma222_TEMP_RD_REG, &data);
    acc->temp = 24 * 2 + (signed char)data;

    return comres;
}

static int bma222_set_offset_filt_xyz(struct i2c_client *client, unsigned char
        *offset, int len)
{
    int comres = 0;
	
    comres = bma222_smbus_write_byte_block(client, bma222_OFFSET_FILT_X_REG, offset, len);
        
    return comres;
}

static int bma222_get_offset_filt_xyz(struct i2c_client *client, unsigned char
        *offset, int len)
{
    int comres = 0 ;

    comres = bma222_smbus_read_byte_block(client, bma222_OFFSET_FILT_X_REG,    offset, len);

	return comres;
}

static int bma222_read_file(char *path, char *buf, int size)
{
    struct file *filp;
    loff_t len, offset;
    int ret=0;
    mm_segment_t fs;

    filp = filp_open(path, O_RDWR, 0777);
    if (IS_ERR(filp)) {
        ret = PTR_ERR(filp);
        goto out;
    }

    len = vfs_llseek(filp, 0, SEEK_END);
    if (len > size) {
        len = size;
    }
    
    offset = vfs_llseek(filp, 0, SEEK_SET);

    fs=get_fs();
    set_fs(KERNEL_DS);

    ret = vfs_read(filp, (char __user *)buf, (size_t)len, &(filp->f_pos));

    set_fs(fs);

    filp_close(filp, NULL);    
out:
    return ret;
}

static int bma222_load_user_calibration(struct i2c_client *client)
{
    char buffer[16];
    int ret = 0;
    int data[3];
    unsigned char offset[3];
    struct bma222_data *bma222 = i2c_get_clientdata(client);    
    int calibrated = atomic_read(&bma222->calibrated);
    
    // only calibrate once
    if (calibrated) {
        goto usr_calib_end;
    } else {
        atomic_set(&bma222->calibrated, 1);
    }

    ret = bma222_read_file(CFG_GSENSOR_CALIBFILE, buffer, sizeof(buffer));
    if (ret <= 0) {
        printk(KERN_ERR "gsensor calibration file not exist!\n");
        goto usr_calib_end;
    }
    
    sscanf(buffer, "%d %d %d", &data[0], &data[1], &data[2]);
    offset[0] = (unsigned char) data[0];
    offset[1] = (unsigned char) data[1];
    offset[2] = (unsigned char) data[2];    
    
    printk(KERN_INFO "user cfg_calibration: %d %d %d\n", offset[0], offset[1], offset[2]);
    
    if (bma222_set_offset_filt_xyz(bma222->bma222_client, offset, sizeof(offset)) < 0) {
        printk(KERN_ERR"set offset fail\n");
        goto usr_calib_end;
    }
    
    printk(KERN_INFO "load user calibration finished\n");
    
usr_calib_end:
    return ret;
}

static int bma222_axis_remap(struct i2c_client *client,
        struct bma222acc *acc)
{
    struct bma222_data *bma222 = i2c_get_clientdata(client);
    s16 swap;
    int bma_position = atomic_read(&bma222->position);

    switch (abs(bma_position)) {
        case 1:
            acc->x = -(acc->x);
            acc->y = -(acc->y);
            break;
        case 2:
            swap = acc->x;
            acc->x = -acc->y;
            acc->y = swap;
            break;
        case 3:
            break;
        case 4:
            swap = acc->x;
            acc->x = acc->y;
            acc->y = -swap; 
            break;
    }
    
    if (bma_position < 0) {
        acc->z = -(acc->z);
        acc->x = -(acc->x);
    }
    
    return 0;
}

static void bma222_work_func(struct work_struct *work)
{
    struct bma222_data *bma222 = container_of((struct delayed_work *)work,
            struct bma222_data, work);
    static struct bma222acc acc;
    unsigned long delay = msecs_to_jiffies(atomic_read(&bma222->delay));

    bma222_load_user_calibration(bma222->bma222_client);
    
    bma222_read_accel_xyz(bma222->bma222_client, &acc);
    bma222_axis_remap(bma222->bma222_client, &acc);
    input_report_abs(bma222->input, ABS_X, acc.x);
    input_report_abs(bma222->input, ABS_Y, acc.y);
    input_report_abs(bma222->input, ABS_Z, acc.z);
    input_sync(bma222->input);
    
    schedule_delayed_work(&bma222->work, delay);
}

static void bma222_work_func2(struct work_struct *work)
{
    struct bma222_data *bma222 = container_of((struct delayed_work *)work,
            struct bma222_data, work2);
    static struct bma222acc acc;
    unsigned long delay = msecs_to_jiffies(atomic_read(&bma222->delay2));
    
    bma222_read_temperature(bma222->bma222_client, &acc);
    input_report_abs(bma222->input2, ABS_MISC, acc.temp);
    input_sync(bma222->input2);
    
    schedule_delayed_work(&bma222->work2, delay);
}

static ssize_t bma222_register_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int address, value;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    sscanf(buf, "0x%x=0x%x", &address, &value);

    if (bma222_smbus_write_byte(bma222->bma222_client, (unsigned char)address,
                (unsigned char *)&value) < 0)
        return -EINVAL;

    return count;
}

static ssize_t bma222_register_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{

    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    size_t count = 0;
    u8 reg[0x3d];
    int i;

    for (i = 0 ; i <= 0x3d; i++) {
        bma222_smbus_read_byte(bma222->bma222_client, i, reg+i);

        count += sprintf(&buf[count], "0x%x: 0x%x\n", i, reg[i]);
    }
    return count;


}

static ssize_t bma222_range_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    if (bma222_get_range(bma222->bma222_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t bma222_range_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (bma222_set_range(bma222->bma222_client, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t bma222_bandwidth_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    if (bma222_get_bandwidth(bma222->bma222_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);

}

static ssize_t bma222_bandwidth_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (bma222_set_bandwidth(bma222->bma222_client,
                (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t bma222_mode_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    if (bma222_get_mode(bma222->bma222_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t bma222_mode_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (bma222_set_mode(bma222->bma222_client, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}


static ssize_t bma222_value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct input_dev *input = to_input_dev(dev);
    struct bma222_data *bma222 = input_get_drvdata(input);
    struct bma222acc acc;
    
    bma222_read_accel_xyz(bma222->bma222_client, &acc);
    bma222_axis_remap(bma222->bma222_client, &acc);

    return sprintf(buf, "%d %d %d\n", acc.x, acc.y, acc.z);
}

static ssize_t bma222_delay_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&bma222->delay));

}

static ssize_t bma222_delay_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);
    unsigned char bandwidth;

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (data > bma222_MAX_DELAY)
        data = bma222_MAX_DELAY;
    atomic_set(&bma222->delay, (unsigned int) data);

    // change band width
    data = 1000 / data;
    if (data > 500) {
        bandwidth = 7;
    } else if (data > 250) {
        bandwidth = 6;
    } else if (data > 125) {
        bandwidth = 5;
    } else if (data > 62) {
        bandwidth = 4;
    } else if (data > 31) {
        bandwidth = 3;
    } else {
        bandwidth = 2;
    }
    bma222_set_bandwidth(bma222->bma222_client, bandwidth);
    
    return count;
}

static ssize_t bma222_enable_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&bma222->enable));

}

static void bma222_do_enable(struct device *dev, int enable)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);
    int used = enable || atomic_read(&bma222->enable2);
	
	if(enable == 0)//add by sujiewei(2014-12-11),save offset before turn off gsensor 
	{
		bma222_get_offset_filt_xyz(bma222->bma222_client, 
				bma222->offset_saved, sizeof(bma222->offset_saved));
	}

    if (used) {
        bma222_set_mode(bma222->bma222_client,
                bma222_MODE_NORMAL);
    } else {
        bma222_set_mode(bma222->bma222_client,
                bma222_MODE_SUSPEND);
    }

    if (enable) {
        schedule_delayed_work(&bma222->work,
            msecs_to_jiffies(atomic_read(&bma222->delay)));
    } else {
        cancel_delayed_work_sync(&bma222->work);
    }
	
	if(enable == 1)//add by sujiewei(2014-12-11),get offset after turn on gsensor
	{
		mdelay(2);	//delay for set offset accurately
		bma222_set_offset_filt_xyz(bma222->bma222_client, 
                bma222->offset_saved, sizeof(bma222->offset_saved));
	}
}

static void bma222_set_enable(struct device *dev, int enable)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);
    int pre_enable = atomic_read(&bma222->enable);

    mutex_lock(&bma222->enable_mutex);
    if (enable != pre_enable) {
        bma222_do_enable(dev, enable);
        atomic_set(&bma222->enable, enable);
    }
    mutex_unlock(&bma222->enable_mutex);
}

static ssize_t bma222_enable_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if ((data == 0) || (data == 1)) {
        bma222_set_enable(dev, data);
    }

    return count;
}

static ssize_t bma222_board_position_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    data = atomic_read(&(bma222->position));

    return sprintf(buf, "%d\n", data);
}

static ssize_t bma222_board_position_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&(bma222->position), (int) data);

    return count;
}

static ssize_t bma222_fast_calibration_x_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{


    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    if (bma222_get_offset_target_x(bma222->bma222_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);

}

static ssize_t bma222_fast_calibration_x_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    signed char tmp;
    unsigned char timeout = 0;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    if (bma222_set_offset_target_x(bma222->bma222_client, (unsigned
                    char)data) < 0)
        return -EINVAL;

    if (bma222_set_cal_trigger(bma222->bma222_client, 1) < 0)
        return -EINVAL;

    do {
        mdelay(2);
        bma222_get_cal_ready(bma222->bma222_client, &tmp);

    /*    printk(KERN_INFO "wait 2ms cal ready flag is %d\n",tmp);*/
        timeout++;
        if (timeout == 500) {
            printk(KERN_INFO "get fast calibration ready error\n");
            return -EINVAL;
        };

    } while (tmp == 0);

    printk(KERN_INFO "x axis fast calibration finished\n");
    return count;
}

static ssize_t bma222_fast_calibration_y_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{


    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    if (bma222_get_offset_target_y(bma222->bma222_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);

}

static ssize_t bma222_fast_calibration_y_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    signed char tmp;
    unsigned char timeout = 0;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    if (bma222_set_offset_target_y(bma222->bma222_client, (unsigned
                    char)data) < 0)
        return -EINVAL;

    if (bma222_set_cal_trigger(bma222->bma222_client, 2) < 0)
        return -EINVAL;

    do {
        mdelay(2);
        bma222_get_cal_ready(bma222->bma222_client, &tmp);

    /*    printk(KERN_INFO "wait 2ms cal ready flag is %d\n",tmp);*/
        timeout++;
        if (timeout == 500) {
            printk(KERN_INFO "get fast calibration ready error\n");
            return -EINVAL;
        };

    } while (tmp == 0);

    printk(KERN_INFO "y axis fast calibration finished\n");
    return count;
}

static ssize_t bma222_fast_calibration_z_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    if (bma222_get_offset_target_z(bma222->bma222_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t bma222_fast_calibration_z_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    signed char tmp;
    unsigned char timeout = 0;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    if (bma222_set_offset_target_z(bma222->bma222_client, (unsigned
                    char)data) < 0)
        return -EINVAL;

    if (bma222_set_cal_trigger(bma222->bma222_client, 3) < 0)
        return -EINVAL;

    do {
        mdelay(2);
        bma222_get_cal_ready(bma222->bma222_client, &tmp);

    /*    printk(KERN_INFO "wait 2ms cal ready flag is %d\n",tmp);*/
        timeout++;
        if (timeout == 500) {
            printk(KERN_INFO "get fast calibration ready error\n");
            return -EINVAL;
        };

    } while (tmp == 0);

    printk(KERN_INFO "z axis fast calibration finished\n");
    return count;
}

static ssize_t bma222_selftest_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&bma222->selftest_result));
}

static ssize_t bma222_selftest_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    unsigned char clear_value = 0;
    int error;
    short value1 = 0;
    short value2 = 0;
    short diff = 0;
    unsigned long result = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    if (data != 1)
        return -EINVAL;
    /* set to 2 G range */
    if (bma222_set_range(bma222->bma222_client, 0) < 0)
        return -EINVAL;

    bma222_smbus_write_byte(bma222->bma222_client, 0x32, &clear_value);

    bma222_set_selftest_st(bma222->bma222_client, 1); /* 1 for x-axis*/
    bma222_set_selftest_stn(bma222->bma222_client, 0); /* positive
                                  direction*/
    mdelay(10);
    bma222_read_accel_x(bma222->bma222_client, &value1);
    bma222_set_selftest_stn(bma222->bma222_client, 1); /* negative
                                  direction*/
    mdelay(10);
    bma222_read_accel_x(bma222->bma222_client, &value2);
    diff = value1-value2;

    printk(KERN_INFO "diff x is %d,value1 is %d, value2 is %d\n", diff,
            value1, value2);

    if (abs(diff) < 204)
        result |= 1;

    bma222_set_selftest_st(bma222->bma222_client, 2); /* 2 for y-axis*/
    bma222_set_selftest_stn(bma222->bma222_client, 0); /* positive
                                  direction*/
    mdelay(10);
    bma222_read_accel_y(bma222->bma222_client, &value1);
    bma222_set_selftest_stn(bma222->bma222_client, 1); /* negative
                                  direction*/
    mdelay(10);
    bma222_read_accel_y(bma222->bma222_client, &value2);
    diff = value1-value2;
    printk(KERN_INFO "diff y is %d,value1 is %d, value2 is %d\n", diff,
            value1, value2);
    if (abs(diff) < 204)
        result |= 2;

    bma222_set_selftest_st(bma222->bma222_client, 3); /* 3 for z-axis*/
    bma222_set_selftest_stn(bma222->bma222_client, 0); /* positive
                                  direction*/
    mdelay(10);
    bma222_read_accel_z(bma222->bma222_client, &value1);
    bma222_set_selftest_stn(bma222->bma222_client, 1); /* negative
                                  direction*/
    mdelay(10);
    bma222_read_accel_z(bma222->bma222_client, &value2);
    diff = value1-value2;

    printk(KERN_INFO "diff z is %d,value1 is %d, value2 is %d\n", diff,
            value1, value2);
    if (abs(diff) < 102)
        result |= 4;

    atomic_set(&bma222->selftest_result, (unsigned int)result);

    printk(KERN_INFO "self test finished\n");
    return count;
}

static ssize_t bma222_calibration_run_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int cfg_calibration[3];
    unsigned char offset[3];
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);
    int bma_position = atomic_read(&bma222->position);
    
    bma222_fast_calibration_x_store(dev, attr, "0", 2);
    bma222_fast_calibration_y_store(dev, attr, "0", 2);
    if (bma_position > 0) {
        bma222_fast_calibration_z_store(dev, attr, "1", 2); // z: +g
    } else {
        bma222_fast_calibration_z_store(dev, attr, "2", 2); // z: -g
    }

    if (bma222_get_offset_filt_xyz(bma222->bma222_client, offset, sizeof(offset)) < 0)
        return sprintf((char*)buf, "Read error\n");
    
    cfg_calibration[0] = offset[0];
    cfg_calibration[1] = offset[1];
    cfg_calibration[2] = offset[2];
    
    printk(KERN_INFO "run fast calibration finished\n");
    return count;
}

static ssize_t bma222_calibration_reset_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int cfg_calibration[3];
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);
    
    bma222_reset_cal_offset(bma222->bma222_client);
    memset(cfg_calibration, 0, sizeof(cfg_calibration));

    printk(KERN_INFO "reset fast calibration finished\n");
    return count;
}

static ssize_t bma222_calibration_value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char offset[3];
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    if (bma222_get_offset_filt_xyz(bma222->bma222_client, offset, sizeof(offset)) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d %d %d\n", offset[0], offset[1], offset[2]);
}

static ssize_t bma222_calibration_value_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int data[3];
    unsigned char offset[3];
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    sscanf(buf, "%d %d %d", &data[0], &data[1], &data[2]);
    offset[0] = (unsigned char) data[0];
    offset[1] = (unsigned char) data[1];
    offset[2] = (unsigned char) data[2];
    
    if (bma222_set_offset_filt_xyz(bma222->bma222_client, offset, sizeof(offset)) < 0)
        return -EINVAL;

    printk(KERN_INFO "set fast calibration finished\n");
    return count;
}

static ssize_t bma222_fuzz_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int data = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    data = atomic_read(&(bma222->fuzz));

    return sprintf(buf, "%d\n", data);
}

static ssize_t bma222_fuzz_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data = 0;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&(bma222->fuzz), (int) data);
    
    if(bma222->input != NULL) {
        bma222->input->absinfo[ABS_X].fuzz = data;
        bma222->input->absinfo[ABS_Y].fuzz = data;
        bma222->input->absinfo[ABS_Z].fuzz = data;
    }
    
    return count;
}

static DEVICE_ATTR(range, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_range_show, bma222_range_store);
static DEVICE_ATTR(bandwidth, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_bandwidth_show, bma222_bandwidth_store);
static DEVICE_ATTR(mode, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_mode_show, bma222_mode_store);
static DEVICE_ATTR(value, S_IRUGO,
        bma222_value_show, NULL);
static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_delay_show, bma222_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_enable_show, bma222_enable_store);
static DEVICE_ATTR(board_position, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_board_position_show, bma222_board_position_store);
static DEVICE_ATTR(reg, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_register_show, bma222_register_store);
static DEVICE_ATTR(fast_calibration_x, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_fast_calibration_x_show,
        bma222_fast_calibration_x_store);
static DEVICE_ATTR(fast_calibration_y, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_fast_calibration_y_show,
        bma222_fast_calibration_y_store);
static DEVICE_ATTR(fast_calibration_z, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_fast_calibration_z_show,
        bma222_fast_calibration_z_store);
static DEVICE_ATTR(selftest, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_selftest_show, bma222_selftest_store);
static DEVICE_ATTR(calibration_run, S_IWUSR|S_IWGRP,
        NULL, bma222_calibration_run_store);
static DEVICE_ATTR(calibration_reset, S_IWUSR|S_IWGRP,
        NULL, bma222_calibration_reset_store);
static DEVICE_ATTR(calibration_value, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_calibration_value_show, bma222_calibration_value_store);
static DEVICE_ATTR(fuzz, S_IRUGO|S_IWUSR|S_IWGRP,
        bma222_fuzz_show, bma222_fuzz_store);

static struct attribute *bma222_attributes[] = {
    &dev_attr_range.attr,
    &dev_attr_bandwidth.attr,
    &dev_attr_mode.attr,
    &dev_attr_value.attr,
    &dev_attr_delay.attr,
    &dev_attr_enable.attr,
    &dev_attr_board_position.attr,
    &dev_attr_reg.attr,
    &dev_attr_fast_calibration_x.attr,
    &dev_attr_fast_calibration_y.attr,
    &dev_attr_fast_calibration_z.attr,
    &dev_attr_selftest.attr,
    &dev_attr_calibration_run.attr,
    &dev_attr_calibration_reset.attr,
    &dev_attr_calibration_value.attr,
    &dev_attr_fuzz.attr,
    NULL
};

static struct attribute_group bma222_attribute_group = {
    .attrs = bma222_attributes
};

static ssize_t bma222_value2_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct input_dev *input = to_input_dev(dev);
    struct bma222_data *bma222 = input_get_drvdata(input);
    struct bma222acc acc;
    
    bma222_read_temperature(bma222->bma222_client, &acc);

    return sprintf(buf, "%d\n", acc.temp);
}

static ssize_t bma222_delay2_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&bma222->delay2));

}

static ssize_t bma222_delay2_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
        
    if (data > bma222_MAX_DELAY)
        data = bma222_MAX_DELAY;
        
    atomic_set(&bma222->delay2, (unsigned int) data);

    return count;
}

static ssize_t bma222_enable2_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&bma222->enable2));

}

static void bma222_do_enable2(struct device *dev, int enable2)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);
    int used = enable2 || atomic_read(&bma222->enable);

    if (used) {
        bma222_set_mode(bma222->bma222_client,
                bma222_MODE_NORMAL);
    } else {
        bma222_set_mode(bma222->bma222_client,
                bma222_MODE_SUSPEND);
    }
    
    if (enable2) {
        schedule_delayed_work(&bma222->work2,
            msecs_to_jiffies(atomic_read(&bma222->delay2)));
    } else {
        cancel_delayed_work_sync(&bma222->work2);
    }
}

static void bma222_set_enable2(struct device *dev, int enable2)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *bma222 = i2c_get_clientdata(client);
    int pre_enable = atomic_read(&bma222->enable2);

    mutex_lock(&bma222->enable_mutex);
    if (enable2 != pre_enable) {
        bma222_do_enable2(dev, enable2);
        atomic_set(&bma222->enable2, enable2);
    }
    mutex_unlock(&bma222->enable_mutex);
}

static ssize_t bma222_enable2_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if ((data == 0) || (data == 1)) {
        bma222_set_enable2(dev, data);
    }

    return count;
}

static struct device_attribute dev_attr_value2 = __ATTR(value, S_IRUGO,
        bma222_value2_show, NULL);
static struct device_attribute dev_attr_delay2 = __ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma222_delay2_show, bma222_delay2_store);
static struct device_attribute dev_attr_enable2 = __ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma222_enable2_show, bma222_enable2_store);

static struct attribute *bma222_attributes2[] = {
    &dev_attr_value2.attr,
    &dev_attr_delay2.attr,
    &dev_attr_enable2.attr,
    &dev_attr_reg.attr,
    NULL
};

static struct attribute_group bma222_attribute_group2 = {
    .attrs = bma222_attributes2
};

static int bma222_register_input(struct bma222_data *data)
{
    int err = 0;
    struct input_dev *dev;
    
    /* register gsensor input device */
    dev = input_allocate_device();
    if (!dev) {
        return -ENOMEM;
    }

    dev->name = SENSOR_NAME;
    dev->id.bustype = BUS_I2C;    
    input_set_capability(dev, EV_ABS, ABS_MISC);
    input_set_abs_params(dev, ABS_X, ABSMIN, ABSMAX, FUZZ, 0);
    input_set_abs_params(dev, ABS_Y, ABSMIN, ABSMAX, FUZZ, 0);
    input_set_abs_params(dev, ABS_Z, ABSMIN, ABSMAX, FUZZ, 0);
    input_set_drvdata(dev, data);

    err = input_register_device(dev);
    if (err < 0) {
        input_free_device(dev);
        return err;
    }
    
    data->input = dev;
    
    /* register temperature sensor input device */
    dev = input_allocate_device();
    if (!dev) {
        return -ENOMEM;
    }

    dev->name = SENSOR_NAME2;
    dev->id.bustype = BUS_I2C;
    input_set_capability(dev, EV_ABS, ABS_MISC);
    input_set_abs_params(dev, ABS_MISC, ABSMIN2, ABSMAX2, FUZZ2, 0);    
    input_set_drvdata(dev, data);

    err = input_register_device(dev);
    if (err < 0) {
        input_free_device(dev);
        return err;
    }

    data->input2 = dev;
    
    return 0;
}

static int bma222_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int err = 0;
    unsigned char tempvalue;
    struct bma222_data *data;
    int cfg_position;
    int cfg_calibration[3];
    unsigned char offset[3];
    char const * buf;
    long int temp;

    printk("%s,%d, addr:0x%x\n", __func__, __LINE__, client->addr);
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk( "i2c_check_functionality error\n");
        err = -ENODEV;
        goto exit;
    }
    
    data = kzalloc(sizeof(struct bma222_data), GFP_KERNEL);
    if (!data) {
        err = -ENOMEM;
        goto exit;
    }

    /* read chip id */
    tempvalue = i2c_smbus_read_byte_data(client, bma222_CHIP_ID_REG);
    if ((tempvalue == bma222_CHIP_ID) || (tempvalue == bma222E_CHIP_ID)) {
        printk(KERN_INFO "bma222 detected!\n");
    } else{
        printk(KERN_INFO "bma222 not found! I2c error %d \n", tempvalue);
        err = -ENODEV;
        goto kfree_exit;
    }    
    i2c_set_clientdata(client, data);
    data->bma222_client = client;
    
#if CFG_GSENSOR_USE_CONFIG > 0
        //get dts configures
        err = of_property_read_u32_array(client->dev.of_node, "calibration_table", cfg_calibration, 3);
        if (err != 0) {
            printk(KERN_ERR"get calibration fail\n");
            memset(cfg_calibration, 0, sizeof(cfg_calibration));
        }
#else
    memset(cfg_calibration, 0, sizeof(cfg_calibration));
#endif    

    offset[0] = (unsigned char) cfg_calibration[0];
    offset[1] = (unsigned char) cfg_calibration[1];
    offset[2] = (unsigned char) cfg_calibration[2];
    printk(KERN_INFO "cfg_calibration: %d %d %d\n", offset[0], offset[1], offset[2]);

    if (bma222_set_offset_filt_xyz(client, offset, sizeof(offset)) < 0) {
        printk(KERN_ERR"set offset fail\n");
        goto kfree_exit;
    }    
//K70 Gsensor IC bma 223和8452方向不一样
#if CFG_GSENSOR_USE_CONFIG > 0
    err = of_property_read_string(client->dev.of_node, "position", &buf);
    if (err != 0) {
      printk(KERN_ERR"get position fail\n");
    }
    err = strict_strtol(buf, 10, &temp);  
    cfg_position = (int)temp;

#else
    cfg_position = -3;
#endif

    atomic_set(&data->position, cfg_position);
    
    /* register input device */
    err = bma222_register_input(data);
    if (err < 0) {
        goto error_sysfs;
    }

    err = sysfs_create_group(&data->input->dev.kobj, &bma222_attribute_group);
    if (err < 0) {
        goto error_sysfs;
    }

    err = sysfs_create_group(&data->input2->dev.kobj, &bma222_attribute_group2);
    if (err < 0) {
        goto error_sysfs;
    }
    
#ifdef CONFIG_HAS_EARLYSUSPEND
    data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    data->early_suspend.suspend = bma222_early_suspend;
    data->early_suspend.resume = bma222_late_resume;
    register_early_suspend(&data->early_suspend);
#endif

    INIT_DELAYED_WORK(&data->work, bma222_work_func);
    INIT_DELAYED_WORK(&data->work2, bma222_work_func2);
    
    mutex_init(&data->enable_mutex);
    atomic_set(&data->delay, bma222_MAX_DELAY);
    atomic_set(&data->delay2, bma222_MAX_DELAY);    
    atomic_set(&data->enable, 0);
    atomic_set(&data->enable2, 0);
    atomic_set(&data->calibrated, 0);
    atomic_set(&data->fuzz, FUZZ);
    
    bma222_set_bandwidth(client, bma222_BW_SET);
    bma222_set_range(client, bma222_RANGE_SET);

    return 0;

error_sysfs:
    input_unregister_device(data->input);
    input_unregister_device(data->input2);
kfree_exit:
    kfree(data);
exit:
    return err;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma222_early_suspend(struct early_suspend *h)
{
    // sensor hal will disable when early suspend
}


static void bma222_late_resume(struct early_suspend *h)
{
    // sensor hal will enable when early resume
}
#endif

static int  bma222_remove(struct i2c_client *client)
{
    struct bma222_data *data = i2c_get_clientdata(client);

    bma222_set_enable(&client->dev, 0);
    bma222_set_enable2(&client->dev, 0);
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&data->early_suspend);
#endif
    sysfs_remove_group(&data->input->dev.kobj, &bma222_attribute_group);
    sysfs_remove_group(&data->input->dev.kobj, &bma222_attribute_group2);
    input_unregister_device(data->input);
    input_unregister_device(data->input2);
    kfree(data);

    return 0;
}

static int bma222_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;

    printk( "%s,%d\n", __func__, __LINE__);

    if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
        return -ENODEV;

    strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);

    return 0;
}

#ifdef CONFIG_PM

static int bma222_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *data = i2c_get_clientdata(client);
    
    // save offset
    bma222_get_offset_filt_xyz(data->bma222_client, 
                data->offset_saved, sizeof(data->offset_saved));
                
    bma222_do_enable(dev, 0);  
    bma222_do_enable2(dev, 0);  
    
    return 0;
}

static int bma222_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma222_data *data = i2c_get_clientdata(client);

    bma222_do_enable(dev, atomic_read(&data->enable));
    bma222_do_enable2(dev, atomic_read(&data->enable2));
    
    // restore offset 
    bma222_set_offset_filt_xyz(data->bma222_client, 
                data->offset_saved, sizeof(data->offset_saved));
    
    return 0;
}

#else

#define bma222_suspend        NULL
#define bma222_resume        NULL

#endif /* CONFIG_PM */

static SIMPLE_DEV_PM_OPS(bma222_pm_ops, bma222_suspend, bma222_resume);

static const unsigned short  bma222_addresses[] = {
    0x18,
    I2C_CLIENT_END,
};

static const struct i2c_device_id bma222_id[] = {
    { SENSOR_NAME, 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, bma222_id);

static struct of_device_id bma222_of_match[] = {
	{ .compatible = "bma222" },
	{ }
};

static struct i2c_driver bma222_driver = {
    .driver = {
        .owner    = THIS_MODULE,
        .name    = SENSOR_NAME,
        .pm    = &bma222_pm_ops,
        .of_match_table	= of_match_ptr(bma222_of_match),
    },
    .class        = I2C_CLASS_HWMON,
//    .address_list    = bma222_addresses,
    .id_table    = bma222_id,
    .probe        = bma222_probe,
    .remove        = bma222_remove,
    .detect        = bma222_detect,
};

#if 0
static struct i2c_board_info bma222_board_info={
    .type = SENSOR_NAME, 
    .addr = 0x18,
};
#endif

static int __init bma222_init(void)
{
    return i2c_add_driver(&bma222_driver);
}

static void __exit bma222_exit(void)
{
    i2c_del_driver(&bma222_driver);
}

MODULE_AUTHOR("Albert Zhang <xu.zhang@bosch-sensortec.com>");
MODULE_DESCRIPTION("bma222 accelerometer sensor driver");
MODULE_LICENSE("GPL");

module_init(bma222_init);
module_exit(bma222_exit);

