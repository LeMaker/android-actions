/*
 * Copyright (C) 2011 MCUBE, Inc.
 *
 * Initial Code:
 */


/*! \file mc3236.c
    \brief This file contains all function implementations for the mc3236 in linux
    
    Details.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <asm/uaccess.h>
#include <linux/unistd.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/of.h>
#include <linux/of_i2c.h>
#include <linux/of_gpio.h>
//#include "gsensor_common.h"

/* ----------------------------------------------------------------------------------------------------*/

#define mc3236_WR_FUNC_PTR char (* bus_write)(unsigned char, unsigned char *, unsigned char)

#define mc3236_BUS_WRITE_FUNC(dev_addr, reg_addr, reg_data, wr_len)\
           bus_write(reg_addr, reg_data, wr_len)

#define mc3236_RD_FUNC_PTR char (* bus_read)( unsigned char, unsigned char *, unsigned char)

#define mc3236_BUS_READ_FUNC(dev_addr, reg_addr, reg_data, r_len)\
           bus_read(reg_addr, reg_data, r_len)

#define GET_REAL_VALUE(rv, bn) \
    ((rv & (0x01 << (bn - 1))) ? (- (rv & ~(0xffff << (bn - 1)))) : (rv & ~(0xffff << (bn - 1))))


/*******mc3236 define this**********/

//#define MCUBE_1_5G_8BIT
#define MCUBE_2G_8BIT

#define SENSOR_NAME "mc3236"

/* mc3236 Data Range  */
#if defined(MCUBE_1_5G_8BIT)
    #define ABSMIN                -128
    #define ABSMAX                127
    #define FUZZ                     0
    #define LSG                  86
#elif defined(MCUBE_2G_10BIT) 
    #define ABSMIN                -512
    #define ABSMAX                511
    #define FUZZ                     0
    #define LSG                  256
#elif defined(MCUBE_8G_14BIT) 
    #define ABSMIN                -8192
    #define ABSMAX                8191
    #define FUZZ                     0
    #define LSG                  1024
#elif  defined(MCUBE_2G_8BIT	)
    #define ABSMIN                -128
    #define ABSMAX                127
    #define FUZZ                     0
    #define LSG                  64
#else   // default: 8bit
    #define ABSMIN                -128
    #define ABSMAX                127    
    #define FUZZ                     0
    #define LSG                  86
#endif

/* mc3236 I2C Address  */
#define mc3236_I2C_ADDR        0x4c  // 0x98 >> 1     //0x6C

/*  mc3236 API error codes  */
#define E_NULL_PTR        (char)-127

/* register definitions     */
#define mc3236_XOUT_REG                        0x00
#define mc3236_YOUT_REG                        0x01
#define mc3236_ZOUT_REG                        0x02
#define mc3236_Tilt_Status_REG                0x03
#define mc3236_Sampling_Rate_Status_REG        0x04
#define mc3236_Sleep_Count_REG                0x05
#define mc3236_Interrupt_Enable_REG            0x06
#define mc3236_Mode_Feature_REG                0x07
#define mc3236_Sample_Rate_REG                0x08
#define mc3236_Tap_Detection_Enable_REG        0x09
#define mc3236_TAP_Dwell_Reject_REG            0x0a
#define mc3236_DROP_Control_Register_REG    0x0b
#define mc3236_SHAKE_Debounce_REG            0x0c
#define mc3236_XOUT_EX_L_REG                0x0d
#define mc3236_XOUT_EX_H_REG                0x0e
#define mc3236_YOUT_EX_L_REG                0x0f
#define mc3236_YOUT_EX_H_REG                0x10
#define mc3236_ZOUT_EX_L_REG                0x11
#define mc3236_ZOUT_EX_H_REG                0x12
#define mc3236_CHIP_ID                        0x18
#define mc3236_RANGE_Control_REG            0x20
#define mc3236_SHAKE_Threshold_REG            0x2B
#define mc3236_UD_Z_TH_REG                    0x2C
#define mc3236_UD_X_TH_REG                    0x2D
#define mc3236_RL_Z_TH_REG                    0x2E
#define mc3236_RL_Y_TH_REG                    0x2F
#define mc3236_FB_Z_TH_REG                    0x30
#define mc3236_DROP_Threshold_REG            0x31
#define mc3236_TAP_Threshold_REG            0x32
#define mc3236_REG_PRODUCT_CODE            		0x3B

/***********************************************
 *** PRODUCT ID
 ***********************************************/
#define mc3236_PCODE_3210     	0x90
#define mc3236_PCODE_3230     	0x19
#define mc3236_PCODE_3250     	0x88
#define mc3236_PCODE_3410     	0xA8
#define mc3236_PCODE_3410N   0xB8
#define mc3236_PCODE_3430     	0x29
#define mc3236_PCODE_3430N   0x39
#define mc3236_PCODE_3510B   0x40
#define mc3236_PCODE_3530B   0x30
#define mc3236_PCODE_3510C   0x10//MC3216,MC3256,MC3413
#define mc3236_PCODE_3530C   0x60//MC3236,MC3433

#define MCUBE_8BIT    	0x01		//mc3236_LOW_END
#define MCUBE_14BIT     	0x02		//mc3236_HIGH_END

static unsigned char  is_new_mc34x0 = 0;
static unsigned char  is_mc3250 = 0;
static unsigned char  is_mc35xx = 0;
static unsigned char  Sensor_Accuracy = 0;
static unsigned char s_bPCODE      = 0x00;


/** mc3236 acceleration data 
    \brief Structure containing acceleration values for x,y and z-axis in signed short

*/

typedef struct  {
        short x, /**< holds x-axis acceleration data sign extended. Range -512 to 511. */
              y, /**< holds y-axis acceleration data sign extended. Range -512 to 511. */
              z; /**< holds z-axis acceleration data sign extended. Range -512 to 511. */
} mc3236acc_t;

/* RANGE */
#define mc3236_RANGE__POS                2
#define mc3236_RANGE__LEN                2
#define mc3236_RANGE__MSK                0x0c    
#define mc3236_RANGE__REG                mc3236_RANGE_Control_REG

/* MODE */
#define mc3236_MODE__POS                0
#define mc3236_MODE__LEN                2
#define mc3236_MODE__MSK                0x03    
#define mc3236_MODE__REG                mc3236_Mode_Feature_REG

#define mc3236_MODE_DEF                 0x43

/* BANDWIDTH */
#define mc3236_BANDWIDTH__POS            4
#define mc3236_BANDWIDTH__LEN            3
#define mc3236_BANDWIDTH__MSK            0x70    
#define mc3236_BANDWIDTH__REG            mc3236_RANGE_Control_REG


#define mc3236_GET_BITSLICE(regvar, bitname)\
            (regvar & bitname##__MSK) >> bitname##__POS


#define mc3236_SET_BITSLICE(regvar, bitname, val)\
          (regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK)  


#define mc3236_RANGE_2G                    0
#define mc3236_RANGE_4G                    1
#define mc3236_RANGE_8G                    2

#define mc3236_WAKE                        1
#define mc3236_SNIFF                    2
#define mc3236_STANDBY                    3


#define mc3236_LOW_PASS_512HZ            0
#define mc3236_LOW_PASS_256HZ            1
#define mc3236_LOW_PASS_128HZ            2
#define mc3236_LOW_PASS_64HZ            3
#define mc3236_LOW_PASS_32HZ            4
#define mc3236_LOW_PASS_16HZ            5
#define mc3236_LOW_PASS_8HZ                6


typedef struct {    
    unsigned char mode;        /**< save current mc3236 operation mode */
    unsigned char chip_id;    /**< save mc3236's chip id which has to be 0x00/0x01 after calling mc3236_init() */
    unsigned char dev_addr;   /**< initializes mc3236's I2C device address 0x4c */
    mc3236_WR_FUNC_PTR;          /**< function pointer to the SPI/I2C write function */
    mc3236_RD_FUNC_PTR;          /**< function pointer to the SPI/I2C read function */
} mc3236_t;

//------------------------------------------------------------------------------------------------------------------------

mc3236_t *p_mc3236;                /**< pointer to mc3236 device structure  */

int mcube_mc3236_init(mc3236_t *mc3236) 
{
	int comres=0;
	unsigned char data;
    static    unsigned short        mc3236_i2c_auto_probe_addr[] = { 0x4C,0x6C,0x6E};
    int              _nProbeAddrCount = (sizeof(mc3236_i2c_auto_probe_addr) / sizeof(mc3236_i2c_auto_probe_addr[0]));
    int              _nCount = 0;
    
     for (_nCount = 0; _nCount < _nProbeAddrCount; _nCount++)
    {   
			p_mc3236 = mc3236;                                                                                /* assign mc3236 ptr */
			p_mc3236->dev_addr = mc3236_i2c_auto_probe_addr[_nCount];                                                            /* preset  I2C_addr */
			comres += p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, mc3236_REG_PRODUCT_CODE, &data, 1);            /* read Chip Id */
		
			p_mc3236->chip_id = data;
			s_bPCODE = data;
		
			if((data == mc3236_PCODE_3230)||(data == mc3236_PCODE_3430)
				||(data == mc3236_PCODE_3430N)||(data == mc3236_PCODE_3530B)
				||((data & 0xF1) == mc3236_PCODE_3530C))
			{
				Sensor_Accuracy = MCUBE_8BIT;	//8bit
				break;
			}
			else if((data == mc3236_PCODE_3210)||(data == mc3236_PCODE_3410)
				||(data == mc3236_PCODE_3250)||(data == mc3236_PCODE_3410N)
				||(data == mc3236_PCODE_3510B)||((data & 0xF1) == mc3236_PCODE_3510C))
			{
				Sensor_Accuracy = MCUBE_14BIT;		//14bit
				break;
			}
			else
			{
				Sensor_Accuracy = 0;
			}
		}
	if (data == mc3236_PCODE_3250)
       		 is_mc3250 = 1;

	if ((data == mc3236_PCODE_3430N)||(data == mc3236_PCODE_3410N))
		is_new_mc34x0 = 1;

	if((mc3236_PCODE_3510B == data) || (mc3236_PCODE_3510C == (data & 0xF1))
		||(data == mc3236_PCODE_3530B)||((data & 0xF1) == mc3236_PCODE_3530C))
		is_mc35xx = 1;


	return comres;
}

int mc3236_set_image (void) 
{
	int comres;
	unsigned char data;
	//unsigned char data = 0;
	if (p_mc3236==0)
		return E_NULL_PTR;

	if(MCUBE_14BIT == Sensor_Accuracy)
	{
		data = 0x43;
		comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Mode_Feature_REG, &data, 1 );
		data = 0x00;
    		comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Sleep_Count_REG, &data, 1 );

		data = 0x00;
		if (is_mc35xx)
		{	
			data = 0x0A;
		}	
		
	  	comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Sample_Rate_REG, &data, 1 );    

		data = 0x3F;
		if ((mc3236_PCODE_3510B == s_bPCODE) || (mc3236_PCODE_3510C == (s_bPCODE & 0xF1)))
			data = 0x25;
		else if ((mc3236_PCODE_3530B == s_bPCODE) || (mc3236_PCODE_3530C == (s_bPCODE & 0xF1)))
			data = 0x02;
		
	  	comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_RANGE_Control_REG, &data, 1 );

		data = 0x00;
    		comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Tap_Detection_Enable_REG, &data, 1 );
		data = 0x00;
    		comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Interrupt_Enable_REG, &data, 1 );

	}
	else if(MCUBE_8BIT == Sensor_Accuracy)
	{		
		data = 0x43;
		comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Mode_Feature_REG, &data, 1 );
		data = 0x00;
    		comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Sleep_Count_REG, &data, 1 );

		data = 0x00;
		if (is_mc35xx)
		{	
			data = 0x0A;
		}
		
	  	comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Sample_Rate_REG, &data, 1 );    

		data = 0x32;
		if ((mc3236_PCODE_3510B == s_bPCODE) || (mc3236_PCODE_3510C == (s_bPCODE & 0xF1)))
			data = 0x25;
		else if ((mc3236_PCODE_3530B == s_bPCODE) || (mc3236_PCODE_3530C == (s_bPCODE & 0xF1)))
			data = 0x02;
		
	  	comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_RANGE_Control_REG, &data, 1 );
		data = 0x00;
    		comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Tap_Detection_Enable_REG, &data, 1 );
		data = 0x00;
    		comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Interrupt_Enable_REG, &data, 1 );

	}

	data = 0x41;
	comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Mode_Feature_REG, &data, 1 );


    return comres;
}


int mc3236_get_offset(unsigned char *offset, int len) 
{    
    int comres;
    
    if (p_mc3236==0)
        return E_NULL_PTR;

    comres = p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, 0x21, offset, len);
    
    return comres;
}    


int mc3236_set_offset(unsigned char *offset, int len) 
{
    int comres;
    unsigned char data;
    
    if (p_mc3236==0)
        return E_NULL_PTR;

    data = 0x43;
    comres = p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Mode_Feature_REG, &data, 1); 
    
    comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, 0x21, offset, len);
    
    data = 0x41;
    comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_Mode_Feature_REG, &data, 1);
    
    return comres;
}


int mc3236_set_range(char range) 
{            
   int comres = 0;
   unsigned char data;

   if (p_mc3236==0)
        return E_NULL_PTR;

   if (range<3) {    
       comres = p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, mc3236_RANGE__REG, &data, 1);
       data = mc3236_SET_BITSLICE(data, mc3236_RANGE, range);
       comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_RANGE__REG, &data, 1);

   }
   return comres;

}


int mc3236_get_range(unsigned char *range) 
{
	int comres = 0;
	unsigned char data;

	if (p_mc3236==0)
	    return E_NULL_PTR;
	comres = p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, mc3236_RANGE__REG, &data, 1);
	data = mc3236_GET_BITSLICE(data, mc3236_RANGE);

	*range = data;

	return comres;
}



int mc3236_set_mode(unsigned char mode) {
    
    int comres=0;
    unsigned char data;

    if (p_mc3236==0)
        return E_NULL_PTR;

    if (mode<4) {
        data  = mc3236_MODE_DEF;
        data  = mc3236_SET_BITSLICE(data, mc3236_MODE, mode);          
        comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_MODE__REG, &data, 1 );

          p_mc3236->mode = mode;
    } 
    return comres;
}


int mc3236_get_mode(unsigned char *mode) 
{
    if (p_mc3236==0)
        return E_NULL_PTR;    
        *mode =  p_mc3236->mode;
      return 0;
}

int mc3236_set_bandwidth(char bw) 
{
	int comres = 0;
	unsigned char data;

	if (p_mc3236==0)
	    return E_NULL_PTR;

	if (bw<7) 
	{
		comres = p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, mc3236_BANDWIDTH__REG, &data, 1 );
		data = mc3236_SET_BITSLICE(data, mc3236_BANDWIDTH, bw);
		comres += p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, mc3236_BANDWIDTH__REG, &data, 1 );
	}

	return comres;
}

int mc3236_get_bandwidth(unsigned char *bw) 
{
	int comres = 1;
	if (p_mc3236==0)
		return E_NULL_PTR;

	comres = p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, mc3236_BANDWIDTH__REG, bw, 1 );        

	*bw = mc3236_GET_BITSLICE(*bw, mc3236_BANDWIDTH);

	return comres;
}


int mc3236_read_accel_x(short *a_x) 
{
	int comres;
	unsigned char data[2];


	if (p_mc3236==0)
		return E_NULL_PTR;

	comres = p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, mc3236_XOUT_EX_L_REG, &data[0],2);

	*a_x = ((short)data[0])|(((short)data[1])<<8);

	return comres;
}


int mc3236_read_accel_y(short *a_y) 
{
	int comres;
	unsigned char data[2];    


	if (p_mc3236==0)
		return E_NULL_PTR;

	comres = p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, mc3236_YOUT_EX_L_REG, &data[0],2);

	*a_y = ((short)data[0])|(((short)data[1])<<8);

	return comres;
}


int mc3236_read_accel_z(short *a_z)
{
	int comres;
	unsigned char data[2];    

	if (p_mc3236==0)
		return E_NULL_PTR;

	comres = p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, mc3236_ZOUT_EX_L_REG, &data[0],2);

	*a_z = ((short)data[0])|(((short)data[1])<<8);

	return comres;
}


int mc3236_read_accel_xyz(mc3236acc_t * acc)
{
    int comres = 0;
    unsigned char data[6];

	if (p_mc3236==0)
		return E_NULL_PTR;
    
	if(Sensor_Accuracy == MCUBE_14BIT)
	{
		comres = p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, mc3236_XOUT_EX_L_REG, &data[0],6);

		acc->x = ((signed short)data[0])|(((signed short)data[1])<<8);
		acc->y = ((signed short)data[2])|(((signed short)data[3])<<8);
		acc->z = ((signed short)data[4])|(((signed short)data[5])<<8);
	}
	 else if(Sensor_Accuracy == MCUBE_8BIT)
	 {
	 	comres = p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, mc3236_XOUT_REG, &data[0],3);
 		acc->x = (signed char)data[0];
                acc->y = (signed char)data[1];
                acc->z = (signed char)data[2];
	 }

	if (is_mc3250)
	{
		s16    temp = 0;

		temp = acc->x;
		acc->x = acc->y;
		acc->y = -temp;
	}
    
    return comres;
    
}



int mc3236_get_interrupt_status(unsigned char * ist) 
{

    int comres=0;    
    if (p_mc3236==0)
        return E_NULL_PTR;
    comres = p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, mc3236_Tilt_Status_REG, ist, 1);
    return comres;
}



int mc3236_read_reg(unsigned char addr, unsigned char *data, unsigned char len)
{

    int comres;
    if (p_mc3236==0)
        return E_NULL_PTR;

    comres = p_mc3236->mc3236_BUS_READ_FUNC(p_mc3236->dev_addr, addr, data, len);
    return comres;

}


int mc3236_write_reg(unsigned char addr, unsigned char *data, unsigned char len) 
{

    int comres;

    if (p_mc3236==0)
        return E_NULL_PTR;

    comres = p_mc3236->mc3236_BUS_WRITE_FUNC(p_mc3236->dev_addr, addr, data, len);

    return comres;

}

//------------------------------------------------------------------------------------------------------------------------

#define mc3236_IOC_MAGIC 'M'

#define mc3236_SET_RANGE                _IOWR(mc3236_IOC_MAGIC,4, unsigned char)
#define mc3236_GET_RANGE                _IOWR(mc3236_IOC_MAGIC,5, unsigned char)
#define mc3236_SET_MODE                    _IOWR(mc3236_IOC_MAGIC,6, unsigned char)
#define mc3236_GET_MODE                    _IOWR(mc3236_IOC_MAGIC,7, unsigned char)
#define mc3236_SET_BANDWIDTH            _IOWR(mc3236_IOC_MAGIC,8, unsigned char)
#define mc3236_GET_BANDWIDTH            _IOWR(mc3236_IOC_MAGIC,9, unsigned char)
#define mc3236_READ_ACCEL_X                _IOWR(mc3236_IOC_MAGIC,10,short)
#define mc3236_READ_ACCEL_Y                _IOWR(mc3236_IOC_MAGIC,11,short)
#define mc3236_READ_ACCEL_Z                _IOWR(mc3236_IOC_MAGIC,12,short)
#define mc3236_GET_INTERRUPT_STATUS        _IOWR(mc3236_IOC_MAGIC,13,unsigned char)
#define mc3236_READ_ACCEL_XYZ            _IOWR(mc3236_IOC_MAGIC,14,short)

#define mc3236_IOC_MAXNR                50

#define mc3236_DEBUG                 1

// configuration
#define mc3236_POLL_INTERVAL        (100)
#define mc3236_DEF_VOLTAGE          (3300000)

// cfg data : 1-- used
#define CFG_GSENSOR_USE_CONFIG  1

// calibration file path
#define CFG_GSENSOR_CALIBFILE   "/data/data/com.actions.sensor.calib/files/gsensor_calib.txt"

static struct input_dev *mc3236_idev;
static struct i2c_client *mc3236_client = NULL;  
static struct delayed_work mc3236_work;

//static char mc3236_regulator_name[16];
//static struct regulator *mc3236_regulator = NULL;
//static int mc3236_voltage = mc3236_DEF_VOLTAGE;

static atomic_t mc3236_delay = {0};
static atomic_t mc3236_enable = {0};
static atomic_t mc3236_position = {0};
static atomic_t mc3236_fuzz = {0};

static int mc3236_calib_inited = 0;
static unsigned char mc3236_calib_default[9];
static unsigned char mc3236_offset_saved[9];

struct mc3236_data{
    mc3236_t mc3236;
};

static char mc3236_i2c_write(unsigned char reg_addr, unsigned char *data, unsigned char len);
static char mc3236_i2c_read(unsigned char reg_addr, unsigned char *data, unsigned char len);
static void mc3236_i2c_delay(unsigned int msec);
static int mc3236_axis_remap(mc3236acc_t *acc);

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend mc3236_es_handler;
static void mc3236_early_suspend(struct early_suspend *handler);
static void mc3236_early_resume(struct early_suspend *handler);
#endif

static ssize_t mc3236_delay_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc3236_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc3236_enable_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc3236_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc3236_fuzz_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc3236_fuzz_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc3236_regs_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc3236_regs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc3236_value_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc3236_board_position_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc3236_board_position_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc3236_calibration_run_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc3236_calibration_reset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc3236_calibration_value_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc3236_calibration_value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
        mc3236_delay_show, mc3236_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
        mc3236_enable_show, mc3236_enable_store);
static DEVICE_ATTR(fuzz, S_IRUGO|S_IWUSR|S_IWGRP,
        mc3236_fuzz_show, mc3236_fuzz_store);
static DEVICE_ATTR(regs, S_IRUGO|S_IWUSR|S_IWGRP,
        mc3236_regs_show, mc3236_regs_store);
static DEVICE_ATTR(value, S_IRUGO, mc3236_value_show, NULL);
static DEVICE_ATTR(board_position, S_IRUGO|S_IWUSR|S_IWGRP,
        mc3236_board_position_show, mc3236_board_position_store);
static DEVICE_ATTR(calibration_run, S_IWUSR|S_IWGRP,
        NULL, mc3236_calibration_run_store);
static DEVICE_ATTR(calibration_reset, S_IWUSR|S_IWGRP,
        NULL, mc3236_calibration_reset_store);
static DEVICE_ATTR(calibration_value, S_IRUGO|S_IWUSR|S_IWGRP,
        mc3236_calibration_value_show,
        mc3236_calibration_value_store);

static struct attribute* mc3236_attrs[] =
{
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	&dev_attr_fuzz.attr,
	&dev_attr_regs.attr,
	&dev_attr_value.attr,
	&dev_attr_board_position.attr,
	&dev_attr_calibration_run.attr,
	&dev_attr_calibration_reset.attr,
	&dev_attr_calibration_value.attr,
	NULL
};

static const struct attribute_group mc3236_group =
{
  .attrs = mc3236_attrs,
};

static ssize_t mc3236_delay_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", atomic_read(&mc3236_delay));
}

static ssize_t mc3236_delay_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long data;
    int error;
 
#ifdef mc3236_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    
    atomic_set(&mc3236_delay, (unsigned int) data);
    return count;
}


static ssize_t mc3236_enable_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", atomic_read(&mc3236_enable));
}

static void mc3236_do_enable(struct device *dev, int enable)
{
    if (enable) {
        mc3236_set_mode(mc3236_WAKE); 
        schedule_delayed_work(&mc3236_work,
            msecs_to_jiffies(atomic_read(&mc3236_delay)));
    } else {
        mc3236_set_mode(mc3236_STANDBY);  
        cancel_delayed_work_sync(&mc3236_work);
    }
}

static void mc3236_set_enable(struct device *dev, int enable)
{
    int pre_enable = atomic_read(&mc3236_enable);

    if (enable != pre_enable) {
        mc3236_do_enable(dev, enable);
        atomic_set(&mc3236_enable, enable);        
    }  
}

static ssize_t mc3236_enable_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{ 
    unsigned long data;
    int error;
    
#ifdef mc3236_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif   

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    
    if ((data == 0) || (data == 1)) {
        mc3236_set_enable(dev, data);
    }

    return count;
}

static ssize_t mc3236_fuzz_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int data;

    data = atomic_read(&mc3236_fuzz);

    return sprintf(buf, "%d\n", data);
}

static ssize_t mc3236_fuzz_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;

    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&mc3236_fuzz, (int) data);
    
    if(mc3236_idev != NULL) {
        mc3236_idev->absinfo[ABS_X].fuzz = data;
        mc3236_idev->absinfo[ABS_Y].fuzz = data;
        mc3236_idev->absinfo[ABS_Z].fuzz = data;
    }
    
    return count;
}

static ssize_t mc3236_regs_show(struct device *dev, 
        struct device_attribute *attr, char *buf)
{
  int idx, len=0, result;
  unsigned char regs[0x13];

  result = mc3236_read_reg(0x0, regs, 0x13);
  if(result != 0) 
  {
    printk(KERN_ERR "read reg error!\n");
    return -1;
  }

  for(idx=0; idx<0x13; idx++)
  {
    len += sprintf(buf+len, "[0x%x]=0x%x\n", idx, regs[idx]);
  }
  
  result = mc3236_read_reg(0x20, regs, 0x0a);
  if(result != 0) 
  {
    printk(KERN_ERR "read reg error!\n");
    return -1;
  }

  for(idx=0; idx<0x0a; idx++)
  {
    len += sprintf(buf+len, "[0x%x]=0x%x\n", 0x20+idx, regs[idx]);
  }
  return len;
}

static ssize_t mc3236_regs_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int addr;
    unsigned int data;
    int error;
    
    sscanf(buf, "[0x%x]=0x%x", &addr, &data); 
    
    error = mc3236_write_reg((unsigned char)addr, (unsigned char *)&data, 1);
    if(error) 
    {
        printk(KERN_INFO "write reg error!\n");
        return error;
    }
    
    return count;
}

static ssize_t mc3236_value_show(struct device *dev, 
        struct device_attribute *attr, char *buf)
{
  int result;
  mc3236acc_t acc;

  result = mc3236_read_accel_xyz(&acc);
  if(result != 0) 
  {
    printk(KERN_ERR "read accel xyz error!\n");
    return -1;
  }
  
  mc3236_axis_remap(&acc);
  
  return sprintf(buf, "%d,%d,%d\n", acc.x, acc.y, acc.z);
}

static ssize_t mc3236_board_position_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int data;

    data = atomic_read(&mc3236_position);

    return sprintf(buf, "%d\n", data);
}

static ssize_t mc3236_board_position_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;

    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&mc3236_position, (int) data);

    return count;
}

static int mc3236_calibration_offset(mc3236acc_t *acc)
{    
    int result;
    unsigned char buf[9];
    short tmp, x_gain, y_gain, z_gain ;
    int x_off, y_off, z_off;

	u8  bMsbFilter       = 0x3F;
	s16 wSignBitMask     = 0x2000;
	s16 wSignPaddingBits = 0xC000;
	s32 dwRangePosLimit  = 0x1FFF;
	s32 dwRangeNegLimit  = -0x2000;

	if (is_mc35xx)
	{
	    bMsbFilter       = 0x7F;
	    wSignBitMask     = 0x4000;
	    wSignPaddingBits = 0x8000;
	    dwRangePosLimit  = 0x3FFF;
	    dwRangeNegLimit  = -0x4000;
	}

	//read register 0x21~0x29
	result = mc3236_read_reg(0x21, buf, 9);
    
	 // get x,y,z offset
	tmp = ((buf[1] & bMsbFilter) << 8) + buf[0];
	if (tmp & wSignBitMask)
		tmp |= wSignPaddingBits;
	x_off = tmp;
					
	tmp = ((buf[3] & bMsbFilter) << 8) + buf[2];
	if (tmp & wSignBitMask)
		tmp |= wSignPaddingBits;
	y_off = tmp;
					
	tmp = ((buf[5] & bMsbFilter) << 8) + buf[4];
	if (tmp & wSignBitMask)
		tmp |= wSignPaddingBits;
	z_off = tmp;
                    
    // get x,y,z gain
    x_gain = ((buf[1] >> 7) << 8) + buf[6];
    y_gain = ((buf[3] >> 7) << 8) + buf[7];
    z_gain = ((buf[5] >> 7) << 8) + buf[8];
                                
    // prepare new offset
    x_off = x_off + 16 * acc->x * 256 * 128 / 3 / LSG / (40 + x_gain);
    y_off = y_off + 16 * acc->y * 256 * 128 / 3 / LSG / (40 + y_gain);
    z_off = z_off + 16 * acc->z * 256 * 128 / 3 / LSG / (40 + z_gain);
    
    // write offset to register 0x21~0x26
    buf[0] = 0x43;
    result += mc3236_write_reg(0x07, buf, 1);
    
    buf[0] = x_off & 0xff;
    buf[1] = ((x_off >> 8) & bMsbFilter) | (x_gain & 0x0100 ? 0x80 : 0);
    buf[2] = y_off & 0xff;
    buf[3] = ((y_off >> 8) & bMsbFilter) | (y_gain & 0x0100 ? 0x80 : 0);
    buf[4] = z_off & 0xff;
    buf[5] = ((z_off >> 8) & bMsbFilter) | (z_gain & 0x0100 ? 0x80 : 0);    
    result += mc3236_write_reg(0x21, buf, 6);
    
    buf[0] = 0x41;
    result += mc3236_write_reg(0x07, buf, 1);
    
    return result;
}

static ssize_t mc3236_calibration_run_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int ret;
    int cfg_calibration[9];
    unsigned char offset[9];
    int idx;
    mc3236acc_t acc;
    
    ret = mc3236_read_accel_xyz(&acc);
    if(ret != 0) 
    {
        printk(KERN_ERR "read accel xyz error!\n");
        return -1;
    }

    // get diff
    acc.x = 0 - acc.x;
    acc.y = 0 - acc.y;
    if (atomic_read(&mc3236_position) > 0) {
        acc.z = LSG - acc.z;
    } else {
        acc.z = (-LSG) - acc.z;
    }
    
    mc3236_calibration_offset(&acc);

    if (mc3236_get_offset(offset, sizeof(offset)) < 0)
        return sprintf((char*)buf, "Read error\n");
    
    for (idx = 0; idx < sizeof(offset); idx++)
    {
        cfg_calibration[idx] = offset[idx];
    }
    
    printk(KERN_INFO "run fast calibration finished\n");
    return count;
}

static ssize_t mc3236_calibration_reset_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    //int ret;
    int cfg_calibration[9];
    int idx;
    
    mc3236_set_offset(mc3236_calib_default, sizeof(mc3236_calib_default));    
    
    for (idx = 0; idx < sizeof(mc3236_calib_default); idx++)
    {
        cfg_calibration[idx] = mc3236_calib_default[idx];
    }

    printk(KERN_INFO "reset fast calibration finished\n");
    return count;
}

static ssize_t mc3236_calibration_value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char offset[9];

    if (mc3236_get_offset(offset, sizeof(offset)) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d %d %d %d %d %d %d %d %d\n", offset[0], offset[1], offset[2], 
                                offset[3], offset[4], offset[5], offset[6], offset[7], offset[8]);
}

static ssize_t mc3236_calibration_value_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int data[9];
    unsigned char offset[9];
    int idx;

    sscanf(buf, "%d %d %d %d %d %d %d %d %d", &data[0], &data[1], &data[2],
                    &data[3], &data[4], &data[5], &data[6], &data[7], &data[8]);
                    
    for (idx = 0; idx < sizeof(offset); idx++)
    {
        offset[idx] = (unsigned char) data[idx];
    }
    
    if (mc3236_set_offset(offset, sizeof(offset)) < 0)
        return -EINVAL;

    printk(KERN_INFO "set fast calibration finished\n");
    return count;
}

/*    i2c delay routine for eeprom    */
static inline void mc3236_i2c_delay(unsigned int msec)
{
    mdelay(msec);
}


/*    i2c write routine for mc3236    */
static inline char mc3236_i2c_write(unsigned char reg_addr, unsigned char *data, unsigned char len)
{
    s32 dummy;
    if( mc3236_client == NULL )    /*    No global client pointer?    */
        return -1;

        dummy = i2c_smbus_write_i2c_block_data(mc3236_client, reg_addr, len, data);
        if(dummy < 0)
            return -1;        
            
    return 0;
}

/*    i2c read routine for mc3236    */
static inline char mc3236_i2c_read(unsigned char reg_addr, unsigned char *data, unsigned char len) 
{
    s32 dummy;
    if( mc3236_client == NULL )    /*    No global client pointer?    */
        return -1;

        dummy = i2c_smbus_read_i2c_block_data(mc3236_client, reg_addr, len, data);
        if(dummy < 0)
            return -1;        
    
    return 0;
}
#if 0
static int mc3236_power_on(void)
{
    if(mc3236_regulator_name[0] != '\0') {        
        // request regulator
        mc3236_regulator = regulator_get(NULL, mc3236_regulator_name);
        if (IS_ERR(mc3236_regulator)) {
            printk(KERN_ERR "mc3236 get regulator failed\n");
            return -1;
        }
        
        if (regulator_set_voltage(mc3236_regulator, mc3236_voltage, mc3236_voltage)) {
            printk(KERN_ERR "mc3236 set regulator voltage failed\n");
            regulator_put(mc3236_regulator);
            return -1;
        }
        
        regulator_enable(mc3236_regulator);
        msleep(20);
        
    #ifdef mc3236_DEBUG
        printk(KERN_INFO "%s\n",__FUNCTION__);
    #endif
    }

    return 0;
}

static int mc3236_power_off(void)
{
    if(mc3236_regulator != NULL) {
        // save current voltage
        mc3236_voltage = regulator_get_voltage(mc3236_regulator);
        
        regulator_disable(mc3236_regulator);    
        
        // release regulator
        regulator_put(mc3236_regulator);
        
    #ifdef mc3236_DEBUG
        printk(KERN_INFO "%s\n",__FUNCTION__);
    #endif
    }
    
    return 0;
}
#endif
static int mc3236_read_file(char *path, char *buf, int size)
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

static int mc3236_load_user_calibration(void)
{
    char buffer[64];
    int ret = 0;
    int data[9];
    unsigned char offset[9];
    int idx;
    
    // only calibrate once
    if (mc3236_calib_inited) {
        goto usr_calib_end;
    } else {
        mc3236_calib_inited = 1;
    }

    ret = mc3236_read_file(CFG_GSENSOR_CALIBFILE, buffer, sizeof(buffer));
    if (ret <= 0) {
        printk(KERN_ERR "gsensor calibration file not exist!\n");
        goto usr_calib_end;
    }
    
    sscanf(buffer, "%d %d %d %d %d %d %d %d %d", &data[0], &data[1], &data[2],
                                &data[3], &data[4], &data[5], &data[6], &data[7], &data[8]);
                                
    for (idx = 0; idx < sizeof(offset); idx ++) {
        offset[idx] = (unsigned char) data[idx];
    }
    
    if (mc3236_set_offset(offset, sizeof(offset)) < 0) {
        printk(KERN_ERR"set offset fail\n");
        goto usr_calib_end;
    }
    
    printk(KERN_INFO "load user calibration finished\n");
    
usr_calib_end:
    return ret;
}

static int mc3236_axis_remap(mc3236acc_t *acc)
{
    s16 swap;
    int position = atomic_read(&mc3236_position);

    switch (abs(position)) {
        case 1:
            break;
        case 2:
            swap = acc->x;
            acc->x = acc->y;
            acc->y = -swap; 
            break;
        case 3:
            acc->x = -(acc->x);
            acc->y = -(acc->y);
            break;
        case 4:
            swap = acc->x;
            acc->x = -acc->y;
            acc->y = swap;
            break;
    }
    
    if (position < 0) {
        acc->z = -(acc->z);
        acc->x = -(acc->x);
    }
    
    return 0;
}

static void mc3236_work_func(struct work_struct *work)
{
    mc3236acc_t acc;
    unsigned long delay = msecs_to_jiffies(atomic_read(&mc3236_delay));

    mc3236_load_user_calibration();
    
    mc3236_read_accel_xyz(&acc);
    mc3236_axis_remap(&acc);
    input_report_abs(mc3236_idev, ABS_X, acc.x);
    input_report_abs(mc3236_idev, ABS_Y, acc.y);
    input_report_abs(mc3236_idev, ABS_Z, acc.z);
    input_sync(mc3236_idev);
    
    schedule_delayed_work(&mc3236_work, delay);
}

static int mc3236_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;
#ifdef mc3236_DEBUG
    printk(KERN_INFO "%s\n", __FUNCTION__);
#endif
    if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
        return -ENODEV;

    strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);

    return 0;
}

static int mc3236_probe(struct i2c_client *client,
             const struct i2c_device_id *id)
{
        struct mc3236_data *data;
        int cfg_position;
        int err = 0;
//        int tempvalue;
        int cfg_calibration[3];
        mc3236acc_t acc;
//        unsigned char *regulator;
        const char * buf;
        long int temp;
    
#ifdef mc3236_DEBUG
        printk(KERN_INFO "%s\n",__FUNCTION__);
#endif
    
        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
        {
            printk(KERN_INFO "i2c_check_functionality error\n");
            goto exit;
        }
    
        /* OK. For now, we presume we have a valid client. We now create the
           client structure, even though we cannot fill it completely yet. */
        if (!(data = kmalloc(sizeof(struct mc3236_data), GFP_KERNEL)))
        {
            err = -ENOMEM;
            printk(KERN_INFO "kmalloc error\n");
            goto exit;
        }
        memset(data, 0, sizeof(struct mc3236_data));
    
        i2c_set_clientdata(client, data);

        mc3236_client = client;
		
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

        atomic_set(&mc3236_position, cfg_position);
        atomic_set(&mc3236_delay, mc3236_POLL_INTERVAL);
        atomic_set(&mc3236_enable, 0); 
        atomic_set(&mc3236_fuzz, FUZZ);       
        INIT_DELAYED_WORK(&mc3236_work, mc3236_work_func);
    
        /*input poll device register */
        mc3236_idev = input_allocate_device();
        if (!mc3236_idev) {
            printk(KERN_ERR"alloc poll device failed!\n");
            goto exit_kfree;
        }

        mc3236_idev->name = SENSOR_NAME;
        mc3236_idev->id.bustype = BUS_I2C;
//        mc3236_idev->dev.parent = &client->dev;
        
        input_set_capability(mc3236_idev, EV_ABS, ABS_MISC);
        input_set_abs_params(mc3236_idev, ABS_X, ABSMIN, ABSMAX, FUZZ, 0);
        input_set_abs_params(mc3236_idev, ABS_Y, ABSMIN, ABSMAX, FUZZ, 0);
        input_set_abs_params(mc3236_idev, ABS_Z, ABSMIN, ABSMAX, FUZZ, 0);

        err = input_register_device(mc3236_idev);
        if (err) {
            printk(KERN_ERR "mc3236 input register failed\n");
            goto error_register;
        }
  
        err = sysfs_create_group(&mc3236_idev->dev.kobj, &mc3236_group);
        if (err) {
            printk(KERN_ERR "mc3236 create sysfs group failed\n");
            goto error_register;
        }
  
#ifdef CONFIG_HAS_EARLYSUSPEND
        mc3236_es_handler.suspend = mc3236_early_suspend;
        mc3236_es_handler.resume = mc3236_early_resume;
        register_early_suspend(&mc3236_es_handler);
#endif

        printk(KERN_INFO "mc3236 device create ok: %s\n", SENSOR_NAME);
        
        data->mc3236.bus_write = mc3236_i2c_write;
        data->mc3236.bus_read = mc3236_i2c_read;
        mcube_mc3236_init(&(data->mc3236));
        
        mc3236_set_image();    
        
        // save default offset & gain
        mc3236_get_offset(mc3236_calib_default, sizeof(mc3236_calib_default));

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
    
        acc.x = (unsigned char) cfg_calibration[0];
        acc.y = (unsigned char) cfg_calibration[1];
        acc.z = (unsigned char) cfg_calibration[2];

        // calibration from xml config
        mc3236_calibration_offset(&acc);

        return 0;
        
error_register:
        input_unregister_device(mc3236_idev);
exit_kfree:
        kfree(data);
 exit:
        return err;
}



static int mc3236_remove(struct i2c_client *client)
{
    struct mc3236_data *data;
#ifdef mc3236_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif    
    mc3236_do_enable(&client->dev, 0);
    //mc3236_power_off();

    sysfs_remove_group(&client->dev.kobj, &mc3236_group);

    data = i2c_get_clientdata(client);
    mc3236_client = NULL;
    
    if (mc3236_idev) 
    {
        printk(KERN_INFO "remove input device\n");
        input_unregister_device(mc3236_idev);
        input_free_device(mc3236_idev);
        mc3236_idev = NULL;
    }
    kfree(data);
    return 0;
}

#ifdef CONFIG_PM
static int mc3236_suspend(struct device *dev)
{    
#ifdef mc3236_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif

    // save offset 
    mc3236_get_offset(mc3236_offset_saved, sizeof(mc3236_offset_saved));      
    
    mc3236_do_enable(dev, 0);      
    //mc3236_power_off();
    
    return 0;
}

static int mc3236_resume(struct device *dev)
{
#ifdef mc3236_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif    

    //power on init regs    
    //mc3236_power_on();
    mc3236_set_image();     
    mc3236_do_enable(dev, atomic_read(&mc3236_enable));
    
    // restore offset 
    mc3236_set_offset(mc3236_offset_saved, sizeof(mc3236_offset_saved));        
    
    return 0;
}
#else

#define mc3236_suspend NULL
#define mc3236_resume NULL

#endif /* CONFIG_PM */

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mc3236_early_suspend(struct early_suspend *handler)
{
#ifdef mc3236_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif    
    // sensor hal will disable when early suspend
}

static void mc3236_early_resume(struct early_suspend *handler)
{ 
#ifdef mc3236_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif    
    // sensor hal will enable when early resume
}
#endif

static SIMPLE_DEV_PM_OPS(mc3236_pm_ops, mc3236_suspend, mc3236_resume);

static const struct i2c_device_id mc3236_id[] = {
    { SENSOR_NAME, 0 },
    { }
};

MODULE_DEVICE_TABLE(i2c, mc3236_id);

static struct of_device_id mc323x_of_match[] = {
	{ .compatible = "mc323x" },
	{ }
};

static struct i2c_driver mc3236_driver = {    
    .driver = {
        .owner    = THIS_MODULE,    
        .name    = SENSOR_NAME,
        .pm    = &mc3236_pm_ops,
        .of_match_table	= of_match_ptr(mc323x_of_match),
    },
    .class        = I2C_CLASS_HWMON,
    .id_table    = mc3236_id,
    .probe        = mc3236_probe,
    .remove        = mc3236_remove,
    .detect        = mc3236_detect,
};
#if 0
static struct i2c_board_info mc3236_board_info={
    .type = SENSOR_NAME, 
    .addr = mc3236_I2C_ADDR,
};
#endif

static int __init mc3236_init(void)
{
    printk(KERN_ERR "mc3236 init\n");    
    return i2c_add_driver(&mc3236_driver);
}

static void __exit mc3236_exit(void)
{
    i2c_del_driver(&mc3236_driver);

    printk(KERN_ERR "mc3236 exit,%d\n", __LINE__);
}

MODULE_DESCRIPTION("mc3236 driver");
MODULE_LICENSE("GPL");

module_init(mc3236_init);
module_exit(mc3236_exit);
