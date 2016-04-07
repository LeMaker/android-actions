/* file bma220.c
   brief This file contains all function implementations for the BMA220 in linux
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
#include <linux/of.h>
#include <linux/of_gpio.h>
#include "../gsensor_common.h"


#define SENSOR_NAME              "bma220"
#define SENSOR_I2C_ADDR         0x0a
#define ABSMIN                          -32
#define ABSMAX                          31
#define FUZZ                    1

#define BMA220_MAX_DELAY        200
#define BMA220_CHIP_ID              0xdd
#define BMA220_RANGE_SET        0
#define BMA220_BW_SET              4

#define BMA220_CHIP_ID_REG                      0x00
#define BMA220_VERSION_REG                      0x01
#define BMA220_X_AXIS_DATA_REG              0x02
#define BMA220_Y_AXIS_DATA_REG              0x03
#define BMA220_Z_AXIS_DATA_REG              0x04
#define BMA220_SLEEP_CONFIG_REG               0x0F
#define BMA220_BANDWIDTH_CONFIG_REG   0x10
#define BMA220_RANGE_SELFTEST_REG        0x11
#define BMA220_HPASS_CONFIG_REG            0x12
#define BMA220_OFFSET_TARGET_REG          0x13
#define BMA220_OFFSET_X_RESULT_REG        0x14
#define BMA220_OFFSET_Y_RESULT_REG        0x15
#define BMA220_OFFSET_Z_RESULT_REG        0x16
#define BMA220_SUSPEND_MODE_REG           0x18
#define BMA220_SOFTRESET_REG                  0x19

#define BMA220_RANGE__POS                0
#define BMA220_RANGE__MSK                0x03
#define BMA220_RANGE__LEN                2
#define BMA220_RANGE__REG                BMA220_RANGE_SELFTEST_REG

#define BMA220_SUSPEND__POS                0
#define BMA220_SUSPEND__MSK                0xFF
#define BMA220_SUSPEND__LEN                 8
#define BMA220_SUSPEND__REG                BMA220_SUSPEND_MODE_REG

#define BMA220_SERIAL_HIGH_BW__POS        7
#define BMA220_SERIAL_HIGH_BW__MSK        0x80
#define BMA220_SERIAL_HIGH_BW__LEN        1
#define BMA220_SERIAL_HIGH_BW__REG        BMA220_BANDWIDTH_CONFIG_REG

#define BMA220_SC_FILT_CONFIG__POS        0
#define BMA220_SC_FILT_CONFIG__MSK        0x0F
#define BMA220_SC_FILT_CONFIG__LEN        4
#define BMA220_SC_FILT_CONFIG__REG        BMA220_BANDWIDTH_CONFIG_REG

#define BMA220_SBIST__POS                2
#define BMA220_SBIST__MSK                0x0C
#define BMA220_SBIST__LEN                2
#define BMA220_SBIST__REG                BMA220_RANGE_SELFTEST_REG

#define BMA220_SBIST_SIGN__POS            4
#define BMA220_SBIST_SIGN__MSK            0x10
#define BMA220_SBIST_SIGN__LEN            1
#define BMA220_SBIST_SIGN__REG            BMA220_RANGE_SELFTEST_REG

#define BMA220_OFFSET_RESET__POS        3
#define BMA220_OFFSET_RESET__MSK        0x08
#define BMA220_OFFSET_RESET__LEN               1
#define BMA220_OFFSET_RESET__REG        BMA220_HPASS_CONFIG_REG

#define BMA220_CAL_RDY__POS                4
#define BMA220_CAL_RDY__MSK                0x10
#define BMA220_CAL_RDY__LEN                1
#define BMA220_CAL_RDY__REG                BMA220_HPASS_CONFIG_REG

#define BMA220_CAL_TRIGGER__POS            5
#define BMA220_CAL_TRIGGER__MSK            0xE0
#define BMA220_CAL_TRIGGER__LEN            3
#define BMA220_CAL_TRIGGER__REG            BMA220_HPASS_CONFIG_REG

#define BMA220_CUT_OFF__POS                0
#define BMA220_CUT_OFF__MSK                0x01
#define BMA220_CUT_OFF__LEN                1
#define BMA220_CUT_OFF__REG                BMA220_OFFSET_TARGET_REG

#define BMA220_OFFSET_TARGET_Z__POS        1
#define BMA220_OFFSET_TARGET_Z__MSK        0x06
#define BMA220_OFFSET_TARGET_Z__LEN        2
#define BMA220_OFFSET_TARGET_Z__REG        BMA220_OFFSET_TARGET_REG

#define BMA220_OFFSET_TARGET_Y__POS        3
#define BMA220_OFFSET_TARGET_Y__MSK        0x18
#define BMA220_OFFSET_TARGET_Y__LEN        2
#define BMA220_OFFSET_TARGET_Y__REG        BMA220_OFFSET_TARGET_REG

#define BMA220_OFFSET_TARGET_X__POS        5
#define BMA220_OFFSET_TARGET_X__MSK        0x60
#define BMA220_OFFSET_TARGET_X__LEN        2
#define BMA220_OFFSET_TARGET_X__REG        BMA220_OFFSET_TARGET_REG

#define BMA220_CAL_MANUAL__POS            7
#define BMA220_CAL_MANUAL__MSK            0x80
#define BMA220_CAL_MANUAL__LEN            1
#define BMA220_CAL_MANUAL__REG            BMA220_OFFSET_TARGET_REG

#define BMA220_OFFSET_X__POS            2
#define BMA220_OFFSET_X__MSK            0xFC
#define BMA220_OFFSET_X__LEN            6
#define BMA220_OFFSET_X__REG            BMA220_OFFSET_X_RESULT_REG

#define BMA220_OFFSET_Y__POS            2
#define BMA220_OFFSET_Y__MSK            0xFC
#define BMA220_OFFSET_Y__LEN            6
#define BMA220_OFFSET_Y__REG            BMA220_OFFSET_Y_RESULT_REG

#define BMA220_OFFSET_Z__POS            2
#define BMA220_OFFSET_Z__MSK            0xFC
#define BMA220_OFFSET_Z__LEN            6
#define BMA220_OFFSET_Z__REG            BMA220_OFFSET_Z_RESULT_REG

#define BMA220_SLEEP_EN__POS            6
#define BMA220_SLEEP_EN__MSK            0x40
#define BMA220_SLEEP_EN__LEN              1
#define BMA220_SLEEP_EN__REG            BMA220_SLEEP_CONFIG_REG

#define BMA220_ACC_X__REG               BMA220_X_AXIS_DATA_REG
#define BMA220_ACC_Y__REG               BMA220_Y_AXIS_DATA_REG
#define BMA220_ACC_Z__REG               BMA220_Z_AXIS_DATA_REG
#define BMA220_DATA_SHIFT_RIGHT         0x02


#define BMA220_RANGE_2G                 0
#define BMA220_RANGE_4G                 1
#define BMA220_RANGE_8G                 2
#define BMA220_RANGE_16G                3

#define BMA220_BW_7_81HZ        0x08
#define BMA220_BW_15_63HZ       0x09
#define BMA220_BW_31_25HZ       0x0A
#define BMA220_BW_62_50HZ       0x0B
#define BMA220_BW_125HZ         0x0C
#define BMA220_BW_250HZ         0x0D
#define BMA220_BW_500HZ         0x0E
#define BMA220_BW_1000HZ        0x0F

#define BMA220_MODE_NORMAL      0
#define BMA220_MODE_LOWPOWER    1
#define BMA220_MODE_SUSPEND     2


#define BMA220_GET_BITSLICE(regvar, bitname)\
    ((regvar & bitname##__MSK) >> bitname##__POS)


#define BMA220_SET_BITSLICE(regvar, bitname, val)\
    ((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))


struct bma220acc{
    s16    x,
        y,
        z;
} ;

struct bma220_data {
    struct i2c_client *bma220_client;
    struct mutex enable_mutex;
    atomic_t selftest_result;
    
    atomic_t delay;
    atomic_t enable;
    struct input_dev *input;
    struct delayed_work work;
    unsigned char mode;
    
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
static void bma220_early_suspend(struct early_suspend *h);
static void bma220_late_resume(struct early_suspend *h);
#endif

static int bma220_smbus_read_byte(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data)
{
    s32 dummy;
    unsigned char addr;
    addr = reg_addr<<1;        /*bma220 i2c addr left shift*/
    dummy = i2c_smbus_read_byte_data(client, addr);
    if (dummy < 0)
        return -1;
    *data = dummy & 0x000000ff;

    return 0;
}

static int bma220_smbus_write_byte(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data)
{
    s32 dummy;
    unsigned char addr;
    addr = reg_addr<<1;        /*bma220 i2c addr left shift*/
    dummy = i2c_smbus_write_byte_data(client, addr, *data);
    if (dummy < 0)
        return -1;
    return 0;
}

static int bma220_smbus_read_byte_block(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data, unsigned char len)
{
    s32 dummy;
    unsigned char addr;
    addr = reg_addr<<1;        /*bma220 i2c addr left shift*/
    dummy = i2c_smbus_read_i2c_block_data(client, addr, len, data);
    if (dummy < 0)
        return -1;
    return 0;
}

static int bma220_smbus_write_byte_block(struct i2c_client *client,
        unsigned char reg_addr, unsigned char *data, unsigned char len)
{
    s32 dummy;
    s32 idx;
    unsigned char addr;

    //bma220 only support byte data write
    for (idx = 0; idx < len; idx ++)
    {
        addr = (reg_addr+idx)<<1;        /*bma220 i2c addr left shift*/
        dummy = i2c_smbus_write_byte_data(client, addr, data[idx]);
        if (dummy < 0)
            return -1;
    }
    
    return 0;
}

/** Perform soft reset of BMA220 via bus command
    \param none
    \return result of communication routines 
*/
int bma220_soft_reset(struct i2c_client *client) 
{
    int comres;
    unsigned char data=0;
    struct bma220_data *bma220 = i2c_get_clientdata(client);
    
    /* read softreset twice for setting and releasing the reset */
    comres = bma220_smbus_read_byte(client, BMA220_SOFTRESET_REG, &data);    
    comres = bma220_smbus_read_byte(client, BMA220_SOFTRESET_REG, &data);  
     
    /* Required to reset mode to normal */
    bma220->mode = BMA220_MODE_NORMAL;
    
    return comres;
}

/** Set/reset suspend of BMA220
    \param none
    \return result of communication routines 
    \note calling this function will toggle between normal mode and suspend mode
*/
int bma220_set_suspend(struct i2c_client *client) 
{
    int comres;
    unsigned char data=0;
    
    /* read suspend to toggle between suspend and normal operation mode */
    comres = bma220_smbus_read_byte(client, BMA220_SUSPEND__REG, &data);    
    return comres;
}


/** Set BMA220 to sleep mode via bus command
    \param sleep    0=disable sleep mode, 1=enable sleep mode
    \return result of communication routines
*/
int bma220_set_sleep_en(struct i2c_client *client, unsigned char sleep) 
{
    int comres;
    unsigned char data=0;
    
    comres = bma220_smbus_read_byte(client, BMA220_SLEEP_EN__REG, &data);
    data = BMA220_SET_BITSLICE(data, BMA220_SLEEP_EN, sleep);
    comres |= bma220_smbus_write_byte(client, BMA220_SLEEP_EN__REG, &data);
    return comres;
}

/** Set Mode
    \param mode 0=Normal Mode, 1=Sleep Mode, 2=Suspend Mode
    \return result of communication routines
*/
static int bma220_set_mode(struct bma220_data *bma220, unsigned char mode)
{
    int comres=0;
        
    if (mode == bma220->mode)
        return 0;
        
    switch(bma220->mode) {
        case 0: if (mode==1)
                    comres = bma220_set_sleep_en(bma220->bma220_client,1);
                else if (mode==2)
                    comres = bma220_set_suspend(bma220->bma220_client);
                break;
        case 1: comres = bma220_set_sleep_en(bma220->bma220_client,0);
                if (mode==2)
                    comres |= bma220_set_suspend(bma220->bma220_client);
                break;
        case 2: comres = bma220_set_suspend(bma220->bma220_client);
                if (mode==1)
                    comres |= bma220_set_sleep_en(bma220->bma220_client,1);
                break;
        default: comres = -1;
                break;
    }
    bma220->mode = mode;            
    return comres;
}

/** Set range
    \param range 0=2g, 1=4g, 2=8g, 3=16g
    \return result of communication routines 
*/
static int bma220_set_range(struct i2c_client *client, unsigned char range)
{
    unsigned char data;
    int comres;

    
    comres = bma220_smbus_read_byte(client, BMA220_RANGE__REG, &data);
    data = BMA220_SET_BITSLICE(data, BMA220_RANGE, range);
    comres |= bma220_smbus_write_byte(client, BMA220_RANGE__REG, &data);
    return comres;
}

/** Get range
    \param *range 0=2g, 1=4g, 2=8g, 3=16g.
    \return result of communication routines 
*/
static int bma220_get_range(struct i2c_client *client, unsigned char *range)
{
    unsigned char data;
    int comres = 0;

    comres = bma220_smbus_read_byte(client, BMA220_RANGE__REG, &data);
    *range = BMA220_GET_BITSLICE(data, BMA220_RANGE);
    return comres;
}

/** Set SC_FILT_CONFIG
    \param sc_filt 0=1kHz, 1=600hz, 2=250Hz, 3=150Hz, 4=75Hz, 5=50Hz.
    \return result of communication routines 
*/
static int bma220_set_sc_filt_config(struct i2c_client *client, char sc_filt) 
{
    int comres = 0;
    unsigned char data;

    if (sc_filt<16)
    {
        comres = bma220_smbus_read_byte(client, BMA220_SC_FILT_CONFIG__REG, &data);
      data = BMA220_SET_BITSLICE(data, BMA220_SC_FILT_CONFIG, sc_filt);
      comres += bma220_smbus_write_byte(client, BMA220_SC_FILT_CONFIG__REG, &data);
    }
    return comres;
}

/** Get SC_FILT_CONFIG
    \param *sc_filt 0=1kHz, 1=600hz, 2=250Hz, 3=150Hz, 4=75Hz, 5=50Hz.
    \return result of communication routines 
*/
static int bma220_get_sc_filt_config(struct i2c_client *client, unsigned char *sc_filt)
{
    unsigned char data;
    int comres = 0;

    comres = bma220_smbus_read_byte(client, BMA220_SC_FILT_CONFIG__REG, &data);        
    *sc_filt = BMA220_GET_BITSLICE(data, BMA220_SC_FILT_CONFIG);
    return comres;
}

/* Bandwidth configuration

*/

/** Set Bandwidth (SC_FILT_CONFIG)
    \param bw 0=1kHz, 1=600hz, 2=250Hz, 3=150Hz, 4=75Hz, 5=50Hz.
    \return result of communication routines 
*/
static int bma220_set_bandwidth(struct i2c_client *client, unsigned char bw)
{
    return bma220_set_sc_filt_config(client,bw);
}

/** Get Bandwidth (SC_FILT_CONFIG)
    \param *bw 0=1kHz, 1=600hz, 2=250Hz, 3=150Hz, 4=75Hz, 5=50Hz.
    \return result of communication routines 
*/
static int bma220_get_bandwidth(struct i2c_client *client, unsigned char *bw)
{
    return bma220_get_sc_filt_config(client,bw);
}

/** Get Acceleration Data for all 3 axis XYZ
    \param *accel pointer to struc of three signed char (-32..31)
    \return result of communication routines
*/
static int bma220_read_accel_xyz(struct i2c_client *client, struct bma220acc *accel)
{
    int comres;
    unsigned char data[3];
    
    comres = bma220_smbus_read_byte_block(client, BMA220_ACC_X__REG, data, 3);
    accel->x = ((signed char)data[0]);    
    accel->x = accel->x >> BMA220_DATA_SHIFT_RIGHT;
    accel->y = ((signed char)data[1]);
    accel->y = accel->y >> BMA220_DATA_SHIFT_RIGHT;
    accel->z = ((signed char)data[2]);
    accel->z = accel->z >> BMA220_DATA_SHIFT_RIGHT;
    return comres;
}

/** Get Acceleration Data for X
    \param *accel_x pointer to signed char (-32..31)
    \return result of communication routines
*/
static int bma220_read_accel_x(struct i2c_client *client, signed char  *accel_x) 
{
    int comres;
    unsigned char data;
    
    comres = bma220_smbus_read_byte(client, BMA220_ACC_X__REG, &data);
    *accel_x = ((signed char) data>>BMA220_DATA_SHIFT_RIGHT);
    return comres;
}

/** Get Acceleration Data for Y
    \param *accel_y pointer to signed char (-32..31)
    \return result of communication routines
*/
static int bma220_read_accel_y(struct i2c_client *client, signed char  *accel_y) 
{
    int comres;
    unsigned char data;
    
    comres =  bma220_smbus_read_byte(client, BMA220_ACC_Y__REG, &data);
    *accel_y = ((signed char) data>>BMA220_DATA_SHIFT_RIGHT);
    return comres;
}

/** Get Acceleration Data for Z
    \param *accel_z pointer to signed char (-32..31)
    \return result of communication routines
*/
static int bma220_read_accel_z(struct i2c_client *client, signed char  *accel_z) 
{
    int comres;
    unsigned char data;
    
    comres =  bma220_smbus_read_byte(client, BMA220_ACC_Z__REG, &data);
    *accel_z = ((signed char) data>>BMA220_DATA_SHIFT_RIGHT);
    return comres;
}

/** Set offset_target_x
    \param offset_target_x 0=0g, 1=1g,2=-1g.
    \return result of communication routines
*/
static int bma220_set_offset_target_x(struct i2c_client *client, unsigned char offset_target_x)
{
    unsigned char data;
    int comres;
    
    comres = bma220_smbus_read_byte(client, BMA220_OFFSET_TARGET_X__REG, &data);
    data = BMA220_SET_BITSLICE(data, BMA220_OFFSET_TARGET_X, offset_target_x);
    comres |= bma220_smbus_write_byte(client, BMA220_OFFSET_TARGET_X__REG, &data);
    return comres;
}

/** Get offset_target_x
    \param *offset_target_x 0=0g, 1=1g,2=-1g.
    \return result of communication routines
*/
static int bma220_get_offset_target_x(struct i2c_client *client, unsigned char *offset_target_x) 
{
    unsigned char data;
    int comres;
    
    comres = bma220_smbus_read_byte(client, BMA220_OFFSET_TARGET_X__REG, &data);
    *offset_target_x = BMA220_GET_BITSLICE(data, BMA220_OFFSET_TARGET_X);
    return comres;
}

/** Set offset_target_y
    \param offset_target_y 0=0g, 1=1g,2=-1g.
    \return result of communication routines
*/
static int bma220_set_offset_target_y(struct i2c_client *client, unsigned char offset_target_y)
{
    unsigned char data;
    int comres;
    
    comres = bma220_smbus_read_byte(client, BMA220_OFFSET_TARGET_Y__REG, &data);
    data = BMA220_SET_BITSLICE(data, BMA220_OFFSET_TARGET_Y, offset_target_y);
    comres |= bma220_smbus_write_byte(client, BMA220_OFFSET_TARGET_Y__REG, &data);
    return comres;
}

/** Get offset_target_y
    \param *offset_target_y 0=0g, 1=1g,2=-1g.
    \return result of communication routines
*/
static int bma220_get_offset_target_y(struct i2c_client *client, unsigned char *offset_target_y) 
{
    unsigned char data;
    int comres;
    
    comres = bma220_smbus_read_byte(client, BMA220_OFFSET_TARGET_Y__REG, &data);
    *offset_target_y = BMA220_GET_BITSLICE(data, BMA220_OFFSET_TARGET_Y);
    return comres;
}

/** Set offset_target_z
    \param offset_target_z 0=0g, 1=1g,2=-1g.
    \return result of communication routines
*/
static int bma220_set_offset_target_z(struct i2c_client *client, unsigned char offset_target_z)
{
    unsigned char data;
    int comres;
    
    comres = bma220_smbus_read_byte(client, BMA220_OFFSET_TARGET_Z__REG, &data);
    data = BMA220_SET_BITSLICE(data, BMA220_OFFSET_TARGET_Z, offset_target_z);
    comres |= bma220_smbus_write_byte(client, BMA220_OFFSET_TARGET_Z__REG, &data);
    return comres;
}

/** Get offset_target_z
    \param *offset_target_z 0=0g, 1=1g,2=-1g.
    \return result of communication routines
*/
static int bma220_get_offset_target_z(struct i2c_client *client, unsigned char *offset_target_z)
{
    unsigned char data;
    int comres;
    
    comres = bma220_smbus_read_byte(client, BMA220_OFFSET_TARGET_Z__REG, &data);
    *offset_target_z = BMA220_GET_BITSLICE(data, BMA220_OFFSET_TARGET_Z);
    return comres;
}

static int bma220_get_cal_ready(struct i2c_client *client, unsigned char *calrdy)
{
    int comres = 0;
    unsigned char data;

    comres = bma220_smbus_read_byte(client, BMA220_CAL_RDY__REG, &data);
    data = BMA220_GET_BITSLICE(data, BMA220_CAL_RDY);
    *calrdy = data;
    
    return comres;
}

static int bma220_reset_cal_offset(struct i2c_client *client)
{
    int comres = 0;
    unsigned char data;

    comres = bma220_smbus_read_byte(client, BMA220_OFFSET_RESET__REG, &data);
    data = BMA220_SET_BITSLICE(data, BMA220_OFFSET_RESET, 1);
    comres = bma220_smbus_write_byte(client, BMA220_OFFSET_RESET__REG,  &data);

    return comres;
}

static int bma220_set_cal_trigger(struct i2c_client *client, unsigned char
        caltrigger)
{
    int comres = 0;
    unsigned char data;

    comres = bma220_smbus_read_byte(client, BMA220_CAL_TRIGGER__REG, &data);
    data = BMA220_SET_BITSLICE(data, BMA220_CAL_TRIGGER, caltrigger);
    comres = bma220_smbus_write_byte(client, BMA220_CAL_TRIGGER__REG, &data);

    return comres;
}

/** Set sbist(off,x,y,z) Enable/Disable selftest 
    \param sbist 0=off, 1=x axis, 2=y axis, 3=z axis.
    \return result of communication routines 
*/
static int bma220_set_selftest_st(struct i2c_client *client, unsigned char selftest)
{
    int comres = 0;
    unsigned char data;

    comres = bma220_smbus_read_byte(client, BMA220_SBIST__REG, &data);
    data = BMA220_SET_BITSLICE(data, BMA220_SBIST, selftest);
    comres = bma220_smbus_write_byte(client, BMA220_SBIST__REG, &data);

    return comres;
}

/** Set sbist_sign 
    \param sbist_sign 0=positive stimuli, 1=negative stimuli.
    \return result of communication routines 
*/
static int bma220_set_selftest_stn(struct i2c_client *client, unsigned char stn)
{
    int comres = 0;
    unsigned char data;

    comres = bma220_smbus_read_byte(client, BMA220_SBIST_SIGN__REG, &data);
    data = BMA220_SET_BITSLICE(data, BMA220_SBIST_SIGN, stn);
    comres = bma220_smbus_write_byte(client, BMA220_SBIST_SIGN__REG, &data);

    return comres;
}

/** Set cal_manual
    \param cal_manual 0=write access to offset register disabled, 1=write access to offset register enabled
    \return result of communication routines
*/
static int bma220_set_cal_manual(struct i2c_client *client, unsigned char cal_manual)
{
    unsigned char data;
    int comres;
    
    comres = bma220_smbus_read_byte(client, BMA220_CAL_MANUAL__REG, &data);
    data = BMA220_SET_BITSLICE(data, BMA220_CAL_MANUAL, cal_manual);
    comres |= bma220_smbus_write_byte(client, BMA220_CAL_MANUAL__REG, &data);
    return comres;
}

/** Set offset word for all 3 axis XYZ
    \param offset struct of three signed char (-32..31)
    \return result of communication routines
*/
static int bma220_set_offset_filt_xyz(struct i2c_client *client, unsigned char *offset, int len)
{
    int comres = 0;
    unsigned char data[8];
    int idx;

    // Enable write access to offset register by setting cal_manual
    bma220_set_cal_manual(client, 1);
    
    for (idx=0; idx < len; idx++)
    {
        data[idx] =(offset[idx]<<BMA220_DATA_SHIFT_RIGHT);
    }    
    comres = bma220_smbus_write_byte_block(client, BMA220_OFFSET_X__REG, data, len);
        
    // Enable write access to offset register by setting cal_manual
    bma220_set_cal_manual(client, 0);
    
    return comres;
}

/** Get offset word for all 3 axis XYZ 
    \param *offset pointer to struct of three signed char (-32..31)
    \return result of communication routines
*/
static int bma220_get_offset_filt_xyz(struct i2c_client *client, unsigned char *offset, int len)
{
    int comres = 0 ;
    unsigned char data[8];
    int idx;

    comres = bma220_smbus_read_byte_block(client, BMA220_OFFSET_X__REG, data, len);
    
    for (idx=0; idx < len; idx++)
    {
        offset[idx] = (data[idx]>>BMA220_DATA_SHIFT_RIGHT);
    }
    return comres;
}

static int bma220_read_file(char *path, char *buf, int size)
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

static int bma220_load_user_calibration(struct i2c_client *client)
{
    char buffer[16];
    int ret = 0;
    int data[3];
    unsigned char offset[3];
    struct bma220_data *bma220 = i2c_get_clientdata(client);    
    int calibrated = atomic_read(&bma220->calibrated);
    
    // only calibrate once
    if (calibrated) {
        goto usr_calib_end;
    } else {
        atomic_set(&bma220->calibrated, 1);
    }

    ret = bma220_read_file(CFG_GSENSOR_CALIBFILE, buffer, sizeof(buffer));
    if (ret <= 0) {
        printk(KERN_ERR "gsensor calibration file not exist!\n");
        goto usr_calib_end;
    }
    
    sscanf(buffer, "%d %d %d", &data[0], &data[1], &data[2]);
    offset[0] = (unsigned char) data[0];
    offset[1] = (unsigned char) data[1];
    offset[2] = (unsigned char) data[2];    
    
    printk(KERN_INFO "user cfg_calibration: %d %d %d\n", offset[0], offset[1], offset[2]);
    
    if (bma220_set_offset_filt_xyz(bma220->bma220_client, offset, sizeof(offset)) < 0) {
        printk(KERN_ERR"set offset fail\n");
        goto usr_calib_end;
    }
    
    printk(KERN_INFO "load user calibration finished\n");
    
usr_calib_end:
    return ret;
}

static int bma220_axis_remap(struct i2c_client *client,
        struct bma220acc *acc)
{
    struct bma220_data *bma220 = i2c_get_clientdata(client);
    s16 swap;
    int bma_position = atomic_read(&bma220->position);

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

static void bma220_work_func(struct work_struct *work)
{
    struct bma220_data *bma220 = container_of((struct delayed_work *)work,
            struct bma220_data, work);
    static struct bma220acc acc;
    unsigned long delay = msecs_to_jiffies(atomic_read(&bma220->delay));

    //bma220_load_user_calibration(bma220->bma220_client);
    
    bma220_read_accel_xyz(bma220->bma220_client, &acc);
    bma220_axis_remap(bma220->bma220_client, &acc);
    input_report_abs(bma220->input, ABS_X, acc.x);
    input_report_abs(bma220->input, ABS_Y, acc.y);
    input_report_abs(bma220->input, ABS_Z, acc.z);
    input_sync(bma220->input);
    
    schedule_delayed_work(&bma220->work, delay);
}

static ssize_t bma220_register_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int address, value;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    sscanf(buf, "[0x%x]=0x%x", &address, &value);

    if (bma220_smbus_write_byte(bma220->bma220_client, (unsigned char)address,
                (unsigned char *)&value) < 0)
        return -EINVAL;

    return count;
}

static ssize_t bma220_register_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    size_t count = 0;
    u8 reg[0x20];
    int i;

    for (i = 0 ; i <= 0x17; i++) {
        bma220_smbus_read_byte(bma220->bma220_client, i, reg+i);

        count += sprintf(&buf[count], "[0x%x]=0x%x\n", i, reg[i]);
    }
    return count;
}

static ssize_t bma220_range_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    if (bma220_get_range(bma220->bma220_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t bma220_range_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (bma220_set_range(bma220->bma220_client, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t bma220_bandwidth_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    if (bma220_get_bandwidth(bma220->bma220_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);

}

static ssize_t bma220_bandwidth_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (bma220_set_bandwidth(bma220->bma220_client,
                (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t bma220_mode_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", bma220->mode);
}

static ssize_t bma220_mode_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (bma220_set_mode(bma220, (unsigned char) data) < 0)
        return -EINVAL;

    return count;
}

static ssize_t bma220_value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct input_dev *input = to_input_dev(dev);
    struct bma220_data *bma220 = input_get_drvdata(input);
    struct bma220acc acc;
    
    bma220_read_accel_xyz(bma220->bma220_client, &acc);
    bma220_axis_remap(bma220->bma220_client, &acc);

    return sprintf(buf, "%d %d %d\n", acc.x, acc.y, acc.z);
}

static ssize_t bma220_delay_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&bma220->delay));
}

static ssize_t bma220_delay_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);
    unsigned char bandwidth;

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if (data > BMA220_MAX_DELAY)
        data = BMA220_MAX_DELAY;
    atomic_set(&bma220->delay, (unsigned int) data);

    // change band width
    data = 1000 / data;
    if (data > 500) {
        bandwidth = 0;
    } else if (data > 250) {
        bandwidth = 1;
    } else if (data > 125) {
        bandwidth = 2;
    } else if (data > 64) {
        bandwidth = 3;
    } else if (data > 32) {
        bandwidth = 4;
    } else {
        bandwidth = 5;
    }
    bma220_set_bandwidth(bma220->bma220_client, bandwidth);
    
    return count;
}

static ssize_t bma220_enable_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&bma220->enable));

}

static void bma220_do_enable(struct device *dev, int enable)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);
    int used = enable;

    if (used) {
        bma220_set_mode(bma220, BMA220_MODE_NORMAL);
    } else {
        bma220_set_mode(bma220, BMA220_MODE_SUSPEND);
    }
    
    if (enable) {
        schedule_delayed_work(&bma220->work,
            msecs_to_jiffies(atomic_read(&bma220->delay)));
    } else {
        cancel_delayed_work_sync(&bma220->work);
    }
}

static void bma220_set_enable(struct device *dev, int enable)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);
    int pre_enable = atomic_read(&bma220->enable);

    mutex_lock(&bma220->enable_mutex);
    if (enable != pre_enable) {
        bma220_do_enable(dev, enable);
        atomic_set(&bma220->enable, enable);
    }
    mutex_unlock(&bma220->enable_mutex);
}

static ssize_t bma220_enable_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    if ((data == 0) || (data == 1)) {
        bma220_set_enable(dev, data);
    }

    return count;
}

static ssize_t bma220_board_position_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    data = atomic_read(&(bma220->position));

    return sprintf(buf, "%d\n", data);
}

static ssize_t bma220_board_position_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&(bma220->position), (int) data);

    return count;
}

static ssize_t bma220_fast_calibration_x_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    if (bma220_get_offset_target_x(bma220->bma220_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);

}

static ssize_t bma220_fast_calibration_x_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    signed char tmp;
    unsigned char timeout = 0;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    if (bma220_set_offset_target_x(bma220->bma220_client, (unsigned
                    char)data) < 0)
        return -EINVAL;

    if (bma220_set_cal_trigger(bma220->bma220_client, 1) < 0)
        return -EINVAL;

    do {
        mdelay(2);
        bma220_get_cal_ready(bma220->bma220_client, &tmp);

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

static ssize_t bma220_fast_calibration_y_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    if (bma220_get_offset_target_y(bma220->bma220_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);

}

static ssize_t bma220_fast_calibration_y_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    signed char tmp;
    unsigned char timeout = 0;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    if (bma220_set_offset_target_y(bma220->bma220_client, (unsigned
                    char)data) < 0)
        return -EINVAL;

    if (bma220_set_cal_trigger(bma220->bma220_client, 2) < 0)
        return -EINVAL;

    do {
        mdelay(2);
        bma220_get_cal_ready(bma220->bma220_client, &tmp);

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

static ssize_t bma220_fast_calibration_z_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    if (bma220_get_offset_target_z(bma220->bma220_client, &data) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d\n", data);
}

static ssize_t bma220_fast_calibration_z_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    signed char tmp;
    unsigned char timeout = 0;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    if (bma220_set_offset_target_z(bma220->bma220_client, (unsigned
                    char)data) < 0)
        return -EINVAL;

    if (bma220_set_cal_trigger(bma220->bma220_client, 3) < 0)
        return -EINVAL;

    do {
        mdelay(2);
        bma220_get_cal_ready(bma220->bma220_client, &tmp);

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

static ssize_t bma220_selftest_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    return sprintf(buf, "%d\n", atomic_read(&bma220->selftest_result));
}

static ssize_t bma220_selftest_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    unsigned char value1 = 0;
    unsigned char value2 = 0;
    char diff = 0;
    unsigned long result = 0;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;

    if (data != 1)
        return -EINVAL;
    /* set to 2 G range */
    if (bma220_set_range(bma220->bma220_client, 0) < 0)
        return -EINVAL;

    bma220_set_selftest_st(bma220->bma220_client, 0); /* off */
    bma220_set_selftest_stn(bma220->bma220_client, 0); /* positive direction*/
    mdelay(10);
    
    bma220_set_selftest_st(bma220->bma220_client, 1); /* 1 for x-axis*/
    bma220_set_selftest_stn(bma220->bma220_client, 0); /* positive direction*/
    mdelay(10);
    bma220_read_accel_x(bma220->bma220_client, &value1);
    bma220_set_selftest_stn(bma220->bma220_client, 1); /* negative direction*/
    mdelay(10);
    bma220_read_accel_x(bma220->bma220_client, &value2);
    diff = value1-value2;

    printk(KERN_INFO "diff x is %d,value1 is %d, value2 is %d\n", diff,
            value1, value2);

    if (abs(diff) < 204)
        result |= 1;

    bma220_set_selftest_st(bma220->bma220_client, 2); /* 2 for y-axis*/
    bma220_set_selftest_stn(bma220->bma220_client, 0); /* positive direction*/
    mdelay(10);
    bma220_read_accel_y(bma220->bma220_client, &value1);
    bma220_set_selftest_stn(bma220->bma220_client, 1); /* negative direction*/
    mdelay(10);
    bma220_read_accel_y(bma220->bma220_client, &value2);
    diff = value1-value2;
    printk(KERN_INFO "diff y is %d,value1 is %d, value2 is %d\n", diff,
            value1, value2);
    if (abs(diff) < 204)
        result |= 2;

    bma220_set_selftest_st(bma220->bma220_client, 3); /* 3 for z-axis*/
    bma220_set_selftest_stn(bma220->bma220_client, 0); /* positive direction*/
    mdelay(10);
    bma220_read_accel_z(bma220->bma220_client, &value1);
    bma220_set_selftest_stn(bma220->bma220_client, 1); /* negative direction*/
    mdelay(10);
    bma220_read_accel_z(bma220->bma220_client, &value2);
    diff = value1-value2;

    printk(KERN_INFO "diff z is %d,value1 is %d, value2 is %d\n", diff,
            value1, value2);
    if (abs(diff) < 102)
        result |= 4;

    atomic_set(&bma220->selftest_result, (unsigned int)result);

    printk(KERN_INFO "self test finished\n");
    return count;
}

static ssize_t bma220_calibration_run_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int cfg_calibration[3];
    unsigned char offset[3];
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);
    int bma_position = atomic_read(&bma220->position);
    
    bma220_fast_calibration_x_store(dev, attr, "0", 2);
    bma220_fast_calibration_y_store(dev, attr, "0", 2);
    if (bma_position > 0) {
        bma220_fast_calibration_z_store(dev, attr, "1", 2); // z: +g
    } else {
        bma220_fast_calibration_z_store(dev, attr, "2", 2); // z: -g
    }

    if (bma220_get_offset_filt_xyz(bma220->bma220_client, offset, sizeof(offset)) < 0)
        return sprintf((char*)buf, "Read error\n");
    
    cfg_calibration[0] = offset[0];
    cfg_calibration[1] = offset[1];
    cfg_calibration[2] = offset[2];
    
    printk(KERN_INFO "run fast calibration finished\n");
    return count;
}

static ssize_t bma220_calibration_reset_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int cfg_calibration[3];
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);
    
    bma220_reset_cal_offset(bma220->bma220_client);
    memset(cfg_calibration, 0, sizeof(cfg_calibration));

    printk(KERN_INFO "reset fast calibration finished\n");
    return count;
}

static ssize_t bma220_calibration_value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char offset[3];
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    if (bma220_get_offset_filt_xyz(bma220->bma220_client, offset, sizeof(offset)) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d %d %d\n", offset[0], offset[1], offset[2]);
}

static ssize_t bma220_calibration_value_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int data[3];
    unsigned char offset[3];
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    sscanf(buf, "%d %d %d", &data[0], &data[1], &data[2]);
    offset[0] = (unsigned char) data[0];
    offset[1] = (unsigned char) data[1];
    offset[2] = (unsigned char) data[2];
    
    if (bma220_set_offset_filt_xyz(bma220->bma220_client, offset, sizeof(offset)) < 0)
        return -EINVAL;

    printk(KERN_INFO "set fast calibration finished\n");
    return count;
}

static ssize_t bma220_fuzz_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int data;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    data = atomic_read(&(bma220->fuzz));

    return sprintf(buf, "%d\n", data);
}

static ssize_t bma220_fuzz_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *bma220 = i2c_get_clientdata(client);

    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&(bma220->fuzz), (int) data);
    
    if(bma220->input != NULL) {
        bma220->input->absinfo[ABS_X].fuzz = data;
        bma220->input->absinfo[ABS_Y].fuzz = data;
        bma220->input->absinfo[ABS_Z].fuzz = data;
    }
    
    return count;
}

static DEVICE_ATTR(range, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_range_show, bma220_range_store);
static DEVICE_ATTR(bandwidth, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_bandwidth_show, bma220_bandwidth_store);
static DEVICE_ATTR(mode, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_mode_show, bma220_mode_store);
static DEVICE_ATTR(value, S_IRUGO,
        bma220_value_show, NULL);
static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_delay_show, bma220_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_enable_show, bma220_enable_store);
static DEVICE_ATTR(board_position, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_board_position_show, bma220_board_position_store);
static DEVICE_ATTR(reg, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_register_show, bma220_register_store);
static DEVICE_ATTR(fast_calibration_x, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_fast_calibration_x_show,
        bma220_fast_calibration_x_store);
static DEVICE_ATTR(fast_calibration_y, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_fast_calibration_y_show,
        bma220_fast_calibration_y_store);
static DEVICE_ATTR(fast_calibration_z, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_fast_calibration_z_show,
        bma220_fast_calibration_z_store);
static DEVICE_ATTR(selftest, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_selftest_show, bma220_selftest_store);
static DEVICE_ATTR(calibration_run, S_IWUSR|S_IWGRP|S_IWOTH,
        NULL, bma220_calibration_run_store);
static DEVICE_ATTR(calibration_reset, S_IWUSR|S_IWGRP|S_IWOTH,
        NULL, bma220_calibration_reset_store);
static DEVICE_ATTR(calibration_value, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_calibration_value_show, bma220_calibration_value_store);
static DEVICE_ATTR(fuzz, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        bma220_fuzz_show, bma220_fuzz_store);

static struct attribute *bma220_attributes[] = {
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

static struct attribute_group bma220_attribute_group = {
    .attrs = bma220_attributes
};

static int bma220_register_input(struct bma220_data *data)
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
    return 0;
}

static int bma220_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
{
    int err = 0;
    unsigned char tempvalue;
    struct bma220_data *data;
    int cfg_calibration[3],cfg_position;
    unsigned char offset[3];

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        printk(KERN_INFO "i2c_check_functionality error\n");
        goto exit;
    }
    
    data = kzalloc(sizeof(struct bma220_data), GFP_KERNEL);
    if (!data) {
        err = -ENOMEM;
        goto exit;
    }

    /* read chip id */
    tempvalue = i2c_smbus_read_byte_data(client, BMA220_CHIP_ID_REG);
    if (tempvalue == BMA220_CHIP_ID) {
        printk(KERN_INFO "BMA220 detected!\n");
    } else{
        printk(KERN_INFO "BMA220 not found! I2c error %d \n", tempvalue);
        err = -ENODEV;
        goto kfree_exit;
    }    
    i2c_set_clientdata(client, data);
    data->bma220_client = client;
    
    memset(cfg_calibration, 0, sizeof(cfg_calibration));
	if (gsensor_read_calibration(&cfg_calibration)!=0){		//if get the file calib failed,then read dts default.
		if (gsensor_dt_calib(SENSOR_NAME,&cfg_calibration,client)!=0)
		{
			printk(KERN_ERR "get calibration error\n");
			memset(cfg_calibration, 0, sizeof(cfg_calibration));
		}
	}
    offset[0] = (unsigned char) cfg_calibration[0];
    offset[1] = (unsigned char) cfg_calibration[1];
    offset[2] = (unsigned char) cfg_calibration[2];
    printk(KERN_INFO "cfg_calibration: %d %d %d\n", offset[0], offset[1], offset[2]);

    if (bma220_set_offset_filt_xyz(client, offset, sizeof(offset)) < 0) {
        printk(KERN_ERR"set offset fail\n");
        goto kfree_exit;
    }    

    cfg_position=gsensor_dt_position(SENSOR_NAME,client);
    atomic_set(&data->position, cfg_position);
    
    /* register input device */
    err = bma220_register_input(data);
    if (err < 0) {
        goto error_sysfs;
    }

    err = sysfs_create_group(&data->input->dev.kobj, &bma220_attribute_group);
    if (err < 0) {
        goto error_sysfs;
    }

#ifdef CONFIG_HAS_EARLYSUSPEND
    data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    data->early_suspend.suspend = bma220_early_suspend;
    data->early_suspend.resume = bma220_late_resume;
    register_early_suspend(&data->early_suspend);
#endif

    INIT_DELAYED_WORK(&data->work, bma220_work_func);
    
    mutex_init(&data->enable_mutex);
    atomic_set(&data->delay, BMA220_MAX_DELAY); 
    atomic_set(&data->enable, 0);
    atomic_set(&data->calibrated, 0);
    atomic_set(&data->fuzz, FUZZ);
    
    bma220_set_bandwidth(client, BMA220_BW_SET);
    bma220_set_range(client, BMA220_RANGE_SET);

    return 0;

error_sysfs:
    input_unregister_device(data->input);
kfree_exit:
    kfree(data);
exit:
    return err;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma220_early_suspend(struct early_suspend *h)
{
    // sensor hal will disable when early suspend
}


static void bma220_late_resume(struct early_suspend *h)
{
    // sensor hal will enable when early resume
}
#endif

static int bma220_remove(struct i2c_client *client)
{
    struct bma220_data *data = i2c_get_clientdata(client);

    bma220_set_enable(&client->dev, 0);
#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&data->early_suspend);
#endif
    sysfs_remove_group(&data->input->dev.kobj, &bma220_attribute_group);
    input_unregister_device(data->input);
    kfree(data);

    return 0;
}
#ifdef CONFIG_PM

static int bma220_suspend(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *data = i2c_get_clientdata(client);
    
    // save offset
    bma220_get_offset_filt_xyz(data->bma220_client, 
                data->offset_saved, sizeof(data->offset_saved));
                
    bma220_do_enable(dev, 0);  
    
    return 0;
}

static int bma220_resume(struct device *dev)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct bma220_data *data = i2c_get_clientdata(client);

    bma220_soft_reset(client);

    bma220_set_bandwidth(client, BMA220_BW_SET);

    bma220_set_range(client, BMA220_RANGE_SET);


     /*restore offset,must before bma220_do_enable,
     because i2c can't write to bma220 when suspend.
     */
    bma220_set_offset_filt_xyz(data->bma220_client, 
                data->offset_saved, sizeof(data->offset_saved));


    bma220_do_enable(dev, atomic_read(&data->enable));
    
    return 0;
}

#else

#define bma220_suspend        NULL
#define bma220_resume        NULL

#endif /* CONFIG_PM */

static SIMPLE_DEV_PM_OPS(bma220_pm_ops, bma220_suspend, bma220_resume);

static const unsigned short  bma220_addresses[] = {
    SENSOR_I2C_ADDR,
    I2C_CLIENT_END,
};

static const struct i2c_device_id bma220_id[] = {
    { SENSOR_NAME, 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, bma220_id);

static int bma220_detect(struct i2c_client *client, struct i2c_board_info *info)
{
  struct i2c_adapter *adapter = client->adapter;

  if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
    return -ENODEV;
  strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);

  return 0;
}

static struct i2c_driver bma220_driver = {
    .driver = {
        .owner    = THIS_MODULE,
        .name    = SENSOR_NAME,
        .pm    = &bma220_pm_ops,
    },
    .class        = I2C_CLASS_HWMON,
    .address_list    = bma220_addresses,
    .detect = bma220_detect,
    .id_table    = bma220_id,
    .probe        = bma220_probe,
    .remove        = bma220_remove,

};

static struct i2c_board_info bma220_board_info={
    .type = SENSOR_NAME, 
    .addr = SENSOR_I2C_ADDR,
};


static int __init BMA220_init(void)
{
	return i2c_add_driver(&bma220_driver);
}

static void __exit BMA220_exit(void)
{
    i2c_del_driver(&bma220_driver);
}

MODULE_AUTHOR("Albert Zhang <xu.zhang@bosch-sensortec.com>");
MODULE_DESCRIPTION("BMA220 accelerometer sensor driver");
MODULE_LICENSE("GPL");

module_init(BMA220_init);
module_exit(BMA220_exit);

