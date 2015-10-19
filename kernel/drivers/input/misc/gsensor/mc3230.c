/*
 * Copyright (C) 2011 MCUBE, Inc.
 *
 * Initial Code:
 *    Tan Liang
 */


/*! \file mc32x0_driver.c
    \brief This file contains all function implementations for the mc32x0 in linux
    
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
#include <linux/of.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

/* ----------------------------------------------------------------------------------------------------*/

#define MC32X0_WR_FUNC_PTR char (* bus_write)(unsigned char, unsigned char *, unsigned char)

#define MC32X0_BUS_WRITE_FUNC(dev_addr, reg_addr, reg_data, wr_len)\
           bus_write(reg_addr, reg_data, wr_len)

#define MC32X0_RD_FUNC_PTR char (* bus_read)( unsigned char, unsigned char *, unsigned char)

#define MC32X0_BUS_READ_FUNC(dev_addr, reg_addr, reg_data, r_len)\
           bus_read(reg_addr, reg_data, r_len)

#define GET_REAL_VALUE(rv, bn) \
    ((rv & (0x01 << (bn - 1))) ? (- (rv & ~(0xffff << (bn - 1)))) : (rv & ~(0xffff << (bn - 1))))



//#define MC32X0_HIGH_END
/*******MC3210/20 define this**********/

//#define MCUBE_2G_10BIT_TAP
//#define MCUBE_2G_10BIT
//#define MCUBE_8G_14BIT_TAP
//#define MCUBE_8G_14BIT
#define MC32X0_LOW_END
/*******MC3230 define this**********/

#define MCUBE_1_5G_8BIT
//#define MCUBE_1_5G_8BIT_TAP

#define SENSOR_NAME "mc3230"



/** MC32X0 Data Range
*/
#if defined(MCUBE_1_5G_8BIT) || defined(MCUBE_1_5G_8BIT_TAP)
    #define ABSMIN                -128
    #define ABSMAX                127
    #define FUZZ                     0
    #define LSG                  86
#elif defined(MCUBE_2G_10BIT) || defined(MCUBE_2G_10BIT_TAP)
    #define ABSMIN                -512
    #define ABSMAX                511
    #define FUZZ                     0
    #define LSG                  256
#elif defined(MCUBE_8G_14BIT) || defined(MCUBE_8G_14BIT_TAP)
    #define ABSMIN                -8192
    #define ABSMAX                8191
    #define FUZZ                     0
    #define LSG                  1024
#else   // default: 8bit
    #define ABSMIN                -128
    #define ABSMAX                127    
    #define FUZZ                     0
    #define LSG                  86
#endif

/** MC32X0 I2C Address
*/

#define MC32X0_I2C_ADDR        0x4c // 0x98 >> 1



/*
    MC32X0 API error codes
*/

#define E_NULL_PTR        (char)-127

/* 
 *    
 *    register definitions     
 *
 */

#define MC32X0_XOUT_REG                        0x00
#define MC32X0_YOUT_REG                        0x01
#define MC32X0_ZOUT_REG                        0x02
#define MC32X0_Tilt_Status_REG                0x03
#define MC32X0_Sampling_Rate_Status_REG        0x04
#define MC32X0_Sleep_Count_REG                0x05
#define MC32X0_Interrupt_Enable_REG            0x06
#define MC32X0_Mode_Feature_REG                0x07
#define MC32X0_Sample_Rate_REG                0x08
#define MC32X0_Tap_Detection_Enable_REG        0x09
#define MC32X0_TAP_Dwell_Reject_REG            0x0a
#define MC32X0_DROP_Control_Register_REG    0x0b
#define MC32X0_SHAKE_Debounce_REG            0x0c
#define MC32X0_XOUT_EX_L_REG                0x0d
#define MC32X0_XOUT_EX_H_REG                0x0e
#define MC32X0_YOUT_EX_L_REG                0x0f
#define MC32X0_YOUT_EX_H_REG                0x10
#define MC32X0_ZOUT_EX_L_REG                0x11
#define MC32X0_ZOUT_EX_H_REG                0x12
#define MC32X0_CHIP_ID                        0x18
#define MC32X0_RANGE_Control_REG            0x20
#define MC32X0_SHAKE_Threshold_REG            0x2B
#define MC32X0_UD_Z_TH_REG                    0x2C
#define MC32X0_UD_X_TH_REG                    0x2D
#define MC32X0_RL_Z_TH_REG                    0x2E
#define MC32X0_RL_Y_TH_REG                    0x2F
#define MC32X0_FB_Z_TH_REG                    0x30
#define MC32X0_DROP_Threshold_REG            0x31
#define MC32X0_TAP_Threshold_REG            0x32




/** MC32X0 acceleration data 
    \brief Structure containing acceleration values for x,y and z-axis in signed short

*/

typedef struct  {
        short x, /**< holds x-axis acceleration data sign extended. Range -512 to 511. */
              y, /**< holds y-axis acceleration data sign extended. Range -512 to 511. */
              z; /**< holds z-axis acceleration data sign extended. Range -512 to 511. */
} mc32x0acc_t;

/* RANGE */

#define MC32X0_RANGE__POS                2
#define MC32X0_RANGE__LEN                2
#define MC32X0_RANGE__MSK                0x0c    
#define MC32X0_RANGE__REG                MC32X0_RANGE_Control_REG

/* MODE */

#define MC32X0_MODE__POS                0
#define MC32X0_MODE__LEN                2
#define MC32X0_MODE__MSK                0x03    
#define MC32X0_MODE__REG                MC32X0_Mode_Feature_REG

#define MC32X0_MODE_DEF                 0x43


/* BANDWIDTH */

#define MC32X0_BANDWIDTH__POS            4
#define MC32X0_BANDWIDTH__LEN            3
#define MC32X0_BANDWIDTH__MSK            0x70    
#define MC32X0_BANDWIDTH__REG            MC32X0_RANGE_Control_REG


#define MC32X0_GET_BITSLICE(regvar, bitname)\
            (regvar & bitname##__MSK) >> bitname##__POS


#define MC32X0_SET_BITSLICE(regvar, bitname, val)\
          (regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK)  






#define MC32X0_RANGE_2G                    0
#define MC32X0_RANGE_4G                    1
#define MC32X0_RANGE_8G                    2


#define MC32X0_WAKE                        1
#define MC32X0_SNIFF                    2
#define MC32X0_STANDBY                    3


#define MC32X0_LOW_PASS_512HZ            0
#define MC32X0_LOW_PASS_256HZ            1
#define MC32X0_LOW_PASS_128HZ            2
#define MC32X0_LOW_PASS_64HZ            3
#define MC32X0_LOW_PASS_32HZ            4
#define MC32X0_LOW_PASS_16HZ            5
#define MC32X0_LOW_PASS_8HZ                6





typedef struct {    
    unsigned char mode;        /**< save current MC32X0 operation mode */
    unsigned char chip_id;    /**< save MC32X0's chip id which has to be 0x00/0x01 after calling mc32x0_init() */
    unsigned char dev_addr;   /**< initializes MC32X0's I2C device address 0x4c */
    MC32X0_WR_FUNC_PTR;          /**< function pointer to the SPI/I2C write function */
    MC32X0_RD_FUNC_PTR;          /**< function pointer to the SPI/I2C read function */
} mc32x0_t;

//------------------------------------------------------------------------------------------------------------------------

mc32x0_t *p_mc32x0;                /**< pointer to MC32X0 device structure  */


/** API Initialization routine
 \param *mc32x0 pointer to MC32X0 structured type
 \return result of communication routines 
 */

int mcube_mc32x0_init(mc32x0_t *mc32x0) 
{
    int comres=0;
    unsigned char data;

    p_mc32x0 = mc32x0;                                                                                /* assign mc32x0 ptr */
    p_mc32x0->dev_addr = MC32X0_I2C_ADDR;                                                            /* preset  I2C_addr */
    comres += p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_CHIP_ID, &data, 1);            /* read Chip Id */
    
    p_mc32x0->chip_id = data;                        
    return comres;
}

int mc32x0_set_image (void) 
{
    int comres;
    unsigned char data;
    if (p_mc32x0==0)
        return E_NULL_PTR;
    
#ifdef MCUBE_2G_10BIT_TAP        
    data = MC32X0_MODE_DEF;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Mode_Feature_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sleep_Count_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sample_Rate_REG, &data, 1 );    
    
    data = 0x80;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Tap_Detection_Enable_REG, &data, 1 );
    
    data = 0x05;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_TAP_Dwell_Reject_REG, &data, 1 );
    
    data = 0x33;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_RANGE_Control_REG, &data, 1 );
    
    data = 0x07;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_TAP_Threshold_REG, &data, 1 );
    
    data = 0x04;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Interrupt_Enable_REG, &data, 1 );
        
#endif

#ifdef MCUBE_2G_10BIT
    data = MC32X0_MODE_DEF;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Mode_Feature_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sleep_Count_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sample_Rate_REG, &data, 1 );    

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Tap_Detection_Enable_REG, &data, 1 );

    data = 0x33;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_RANGE_Control_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Interrupt_Enable_REG, &data, 1 );

#endif

#ifdef MCUBE_8G_14BIT_TAP        
    data = MC32X0_MODE_DEF;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Mode_Feature_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sleep_Count_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sample_Rate_REG, &data, 1 );    
    
    data = 0x80;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Tap_Detection_Enable_REG, &data, 1 );
    
    data = 0x05;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_TAP_Dwell_Reject_REG, &data, 1 );
    
    data = 0x3F;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_RANGE_Control_REG, &data, 1 );
    
    data = 0x07;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_TAP_Threshold_REG, &data, 1 );
    
    data = 0x04;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Interrupt_Enable_REG, &data, 1 );
        
#endif

#ifdef MCUBE_8G_14BIT
    data = MC32X0_MODE_DEF;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Mode_Feature_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sleep_Count_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sample_Rate_REG, &data, 1 );    

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Tap_Detection_Enable_REG, &data, 1 );

    data = 0x3F;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_RANGE_Control_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Interrupt_Enable_REG, &data, 1 );

#endif



#ifdef MCUBE_1_5G_8BIT
    data = MC32X0_MODE_DEF;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Mode_Feature_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sleep_Count_REG, &data, 1 );    

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sample_Rate_REG, &data, 1 );

    data = 0x02;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_RANGE_Control_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Tap_Detection_Enable_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Interrupt_Enable_REG, &data, 1 );

#endif


#ifdef MCUBE_1_5G_8BIT_TAP
    data = MC32X0_MODE_DEF;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Mode_Feature_REG, &data, 1 );

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sleep_Count_REG, &data, 1 );    

    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sample_Rate_REG, &data, 1 );

    data = 0x80;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Tap_Detection_Enable_REG, &data, 1 );    

    data = 0x02;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_RANGE_Control_REG, &data, 1 );

    data = 0x03;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_TAP_Dwell_Reject_REG, &data, 1 );

    data = 0x07;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_TAP_Threshold_REG, &data, 1 );

    data = 0x04;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Interrupt_Enable_REG, &data, 1 );

#endif

    
#ifdef  MCUBE_1_5G_6BIT
    
    data = MC32X0_MODE_DEF;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Mode_Feature_REG, &data, 1 );
    
    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sleep_Count_REG, &data, 1 );    
    
    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Sample_Rate_REG, &data, 1 );
    
    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_RANGE_Control_REG, &data, 1 );
    
    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Tap_Detection_Enable_REG, &data, 1 );
    
    data = 0x00;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Interrupt_Enable_REG, &data, 1 );
    
        
#endif



    return comres;
}


int mc32x0_get_offset(unsigned char *offset, int len) 
{    
    int comres;
    
    if (p_mc32x0==0)
        return E_NULL_PTR;

    comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, 0x21, offset, len);
    
    return comres;
}    


int mc32x0_set_offset(unsigned char *offset, int len) 
{
    int comres;
    unsigned char data;
    
    if (p_mc32x0==0)
        return E_NULL_PTR;

    data = 0x43;
    comres = p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Mode_Feature_REG, &data, 1); 
    
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, 0x21, offset, len);
    
    data = 0x41;
    comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_Mode_Feature_REG, &data, 1);
    
    return comres;
}




/**    set mc32x0s range 
 \param range 
 
 \see MC32X0_RANGE_2G        
 \see MC32X0_RANGE_4G            
 \see MC32X0_RANGE_8G            
*/
int mc32x0_set_range(char range) 
{            
   int comres = 0;
   unsigned char data;

   if (p_mc32x0==0)
        return E_NULL_PTR;

   if (range<3) {    
       comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_RANGE__REG, &data, 1);
       data = MC32X0_SET_BITSLICE(data, MC32X0_RANGE, range);
       comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_RANGE__REG, &data, 1);

   }
   return comres;

}


/* readout select range from MC32X0 
   \param *range pointer to range setting
   \return result of bus communication function
   \see MC32X0_RANGE_2G, MC32X0_RANGE_4G, MC32X0_RANGE_8G        
   \see mc32x0_set_range()
*/
int mc32x0_get_range(unsigned char *range) 
{

    int comres = 0;
    unsigned char data;

    if (p_mc32x0==0)
        return E_NULL_PTR;
    comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_RANGE__REG, &data, 1);
    data = MC32X0_GET_BITSLICE(data, MC32X0_RANGE);

    *range = data;

    
    return comres;

}



int mc32x0_set_mode(unsigned char mode) {
    
    int comres=0;
    unsigned char data;

    if (p_mc32x0==0)
        return E_NULL_PTR;

    if (mode<4) {
        data  = MC32X0_MODE_DEF;
        data  = MC32X0_SET_BITSLICE(data, MC32X0_MODE, mode);          
        comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_MODE__REG, &data, 1 );

          p_mc32x0->mode = mode;
    } 
    return comres;
    
}



int mc32x0_get_mode(unsigned char *mode) 
{
    if (p_mc32x0==0)
        return E_NULL_PTR;    
        *mode =  p_mc32x0->mode;
      return 0;
}


int mc32x0_set_bandwidth(char bw) 
{
    int comres = 0;
    unsigned char data;


    if (p_mc32x0==0)
        return E_NULL_PTR;

    if (bw<7) {

        comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_BANDWIDTH__REG, &data, 1 );
      data = MC32X0_SET_BITSLICE(data, MC32X0_BANDWIDTH, bw);
      comres += p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, MC32X0_BANDWIDTH__REG, &data, 1 );

    }

    return comres;


}

int mc32x0_get_bandwidth(unsigned char *bw) {
    int comres = 1;
    if (p_mc32x0==0)
        return E_NULL_PTR;

    comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_BANDWIDTH__REG, bw, 1 );        

    *bw = MC32X0_GET_BITSLICE(*bw, MC32X0_BANDWIDTH);
    
    return comres;

}


int mc32x0_read_accel_x(short *a_x) 
{
    int comres;
    unsigned char data[2];
    
    
    if (p_mc32x0==0)
        return E_NULL_PTR;

    comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_XOUT_EX_L_REG, &data[0],2);
    
    *a_x = ((short)data[0])|(((short)data[1])<<8);

    return comres;
    
}



int mc32x0_read_accel_y(short *a_y) 
{
    int comres;
    unsigned char data[2];    


    if (p_mc32x0==0)
        return E_NULL_PTR;

    comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_YOUT_EX_L_REG, &data[0],2);
    
    *a_y = ((short)data[0])|(((short)data[1])<<8);

    return comres;
}


int mc32x0_read_accel_z(short *a_z)
{
    int comres;
    unsigned char data[2];    

    if (p_mc32x0==0)
        return E_NULL_PTR;

    comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_ZOUT_EX_L_REG, &data[0],2);
    
    *a_z = ((short)data[0])|(((short)data[1])<<8);

    return comres;
}


int mc32x0_read_accel_xyz(mc32x0acc_t * acc)
{
    int comres;
    unsigned char data[6];


    if (p_mc32x0==0)
        return E_NULL_PTR;
    
#ifdef MC32X0_HIGH_END
    comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_XOUT_EX_L_REG, &data[0],6);
    
    acc->x = ((signed short)data[0])|(((signed short)data[1])<<8);
    acc->y = ((signed short)data[2])|(((signed short)data[3])<<8);
    acc->z = ((signed short)data[4])|(((signed short)data[5])<<8);
#endif

#ifdef MC32X0_LOW_END
        comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_XOUT_REG, &data[0],3);
        
#ifndef MCUBE_1_5G_6BIT        
                acc->x = (signed char)data[0];
                acc->y = (signed char)data[1];
                acc->z = (signed char)data[2];
#else 
                acc->x = (signed short)GET_REAL_VALUE(data[0],6);
                acc->y = (signed short)GET_REAL_VALUE(data[1],6);
                acc->z = (signed short)GET_REAL_VALUE(data[2],6);
#endif
        
#endif



    
    return comres;
    
}



int mc32x0_get_interrupt_status(unsigned char * ist) 
{

    int comres=0;    
    if (p_mc32x0==0)
        return E_NULL_PTR;
    comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, MC32X0_Tilt_Status_REG, ist, 1);
    return comres;
}



int mc32x0_read_reg(unsigned char addr, unsigned char *data, unsigned char len)
{

    int comres;
    if (p_mc32x0==0)
        return E_NULL_PTR;

    comres = p_mc32x0->MC32X0_BUS_READ_FUNC(p_mc32x0->dev_addr, addr, data, len);
    return comres;

}


int mc32x0_write_reg(unsigned char addr, unsigned char *data, unsigned char len) 
{

    int comres;

    if (p_mc32x0==0)
        return E_NULL_PTR;

    comres = p_mc32x0->MC32X0_BUS_WRITE_FUNC(p_mc32x0->dev_addr, addr, data, len);

    return comres;

}

//------------------------------------------------------------------------------------------------------------------------

#define MC32X0_IOC_MAGIC 'M'

#define MC32X0_SET_RANGE                _IOWR(MC32X0_IOC_MAGIC,4, unsigned char)
#define MC32X0_GET_RANGE                _IOWR(MC32X0_IOC_MAGIC,5, unsigned char)
#define MC32X0_SET_MODE                    _IOWR(MC32X0_IOC_MAGIC,6, unsigned char)
#define MC32X0_GET_MODE                    _IOWR(MC32X0_IOC_MAGIC,7, unsigned char)
#define MC32X0_SET_BANDWIDTH            _IOWR(MC32X0_IOC_MAGIC,8, unsigned char)
#define MC32X0_GET_BANDWIDTH            _IOWR(MC32X0_IOC_MAGIC,9, unsigned char)
#define MC32X0_READ_ACCEL_X                _IOWR(MC32X0_IOC_MAGIC,10,short)
#define MC32X0_READ_ACCEL_Y                _IOWR(MC32X0_IOC_MAGIC,11,short)
#define MC32X0_READ_ACCEL_Z                _IOWR(MC32X0_IOC_MAGIC,12,short)
#define MC32X0_GET_INTERRUPT_STATUS        _IOWR(MC32X0_IOC_MAGIC,13,unsigned char)
#define MC32X0_READ_ACCEL_XYZ            _IOWR(MC32X0_IOC_MAGIC,14,short)

#define MC32X0_IOC_MAXNR                50

#define MC32X0_DEBUG                 1

// configuration
#define MC32X0_POLL_INTERVAL        (100)
#define MC32X0_DEF_VOLTAGE          (3300000)

// cfg data : 1-- used
#define CFG_GSENSOR_USE_CONFIG  0

// calibration file path
#define CFG_GSENSOR_CALIBFILE   "/data/data/com.actions.sensor.calib/files/gsensor_calib.txt"

/*******************************************
* for xml cfg
*******************************************/
#define CFG_GSENSOR_ADAP_ID          "gsensor.i2c_adap_id"
#define CFG_GSENSOR_POSITION         "gsensor.position"
#define CFG_GSENSOR_CALIBRATION      "gsensor.calibration"
#define CFG_GSENSOR_REGULATOR        "gsensor.regulator"

extern int get_config(const char *key, char *buff, int len);

static struct input_dev *mc32x0_idev;
static struct i2c_client *mc32x0_client = NULL;  
static struct delayed_work mc32x0_work;

static char mc32x0_regulator_name[16];
static struct regulator *mc32x0_regulator = NULL;
static int mc32x0_voltage = MC32X0_DEF_VOLTAGE;

static atomic_t mc32x0_delay = {0};
static atomic_t mc32x0_enable = {0};
static atomic_t mc32x0_position = {0};
static atomic_t mc32x0_fuzz = {0};

static int mc32x0_calib_inited = 0;
static unsigned char mc32x0_calib_default[9];
static unsigned char mc32x0_offset_saved[9];

struct mc32x0_data{
    mc32x0_t mc32x0;
};

static char mc32x0_i2c_write(unsigned char reg_addr, unsigned char *data, unsigned char len);
static char mc32x0_i2c_read(unsigned char reg_addr, unsigned char *data, unsigned char len);
static void mc32x0_i2c_delay(unsigned int msec);
static int mc32x0_axis_remap(mc32x0acc_t *acc);

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend mc32x0_es_handler;
static void mc32x0_early_suspend(struct early_suspend *handler);
static void mc32x0_early_resume(struct early_suspend *handler);
#endif

static ssize_t mc32x0_delay_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc32x0_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc32x0_enable_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc32x0_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc32x0_fuzz_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc32x0_fuzz_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc32x0_regs_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc32x0_regs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc32x0_value_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc32x0_board_position_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc32x0_board_position_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc32x0_calibration_run_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc32x0_calibration_reset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t mc32x0_calibration_value_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t mc32x0_calibration_value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc32x0_delay_show, mc32x0_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc32x0_enable_show, mc32x0_enable_store);
static DEVICE_ATTR(fuzz, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc32x0_fuzz_show, mc32x0_fuzz_store);
static DEVICE_ATTR(regs, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc32x0_regs_show, mc32x0_regs_store);
static DEVICE_ATTR(value, S_IRUGO, mc32x0_value_show, NULL);
static DEVICE_ATTR(board_position, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc32x0_board_position_show, mc32x0_board_position_store);
static DEVICE_ATTR(calibration_run, S_IWUSR|S_IWGRP|S_IWOTH,
        NULL, mc32x0_calibration_run_store);
static DEVICE_ATTR(calibration_reset, S_IWUSR|S_IWGRP|S_IWOTH,
        NULL, mc32x0_calibration_reset_store);
static DEVICE_ATTR(calibration_value, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
        mc32x0_calibration_value_show,
        mc32x0_calibration_value_store);

static struct attribute* mc32x0_attrs[] =
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

static const struct attribute_group mc32x0_group =
{
  .attrs = mc32x0_attrs,
};

static ssize_t mc32x0_delay_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", atomic_read(&mc32x0_delay));
}

static ssize_t mc32x0_delay_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long data;
    int error;
 
#ifdef MC32X0_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    
    atomic_set(&mc32x0_delay, (unsigned int) data);
    return count;
}


static ssize_t mc32x0_enable_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", atomic_read(&mc32x0_enable));
}

static void mc32x0_do_enable(struct device *dev, int enable)
{
    if (enable) {
        mc32x0_set_mode(MC32X0_WAKE); 
        schedule_delayed_work(&mc32x0_work,
            msecs_to_jiffies(atomic_read(&mc32x0_delay)));
    } else {
        mc32x0_set_mode(MC32X0_STANDBY);  
        cancel_delayed_work_sync(&mc32x0_work);
    }
}

static void mc32x0_set_enable(struct device *dev, int enable)
{
    int pre_enable = atomic_read(&mc32x0_enable);

    if (enable != pre_enable) {
        mc32x0_do_enable(dev, enable);
        atomic_set(&mc32x0_enable, enable);        
    }  
}

static ssize_t mc32x0_enable_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{ 
    unsigned long data;
    int error;
    
#ifdef MC32X0_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif   

    error = strict_strtoul(buf, 10, &data);
    if (error)
        return error;
    
    if ((data == 0) || (data == 1)) {
        mc32x0_set_enable(dev, data);
    }

    return count;
}

static ssize_t mc32x0_fuzz_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int data;

    data = atomic_read(&mc32x0_fuzz);

    return sprintf(buf, "%d\n", data);
}

static ssize_t mc32x0_fuzz_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;

    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&mc32x0_fuzz, (int) data);
    
    if(mc32x0_idev != NULL) {
        mc32x0_idev->absinfo[ABS_X].fuzz = data;
        mc32x0_idev->absinfo[ABS_Y].fuzz = data;
        mc32x0_idev->absinfo[ABS_Z].fuzz = data;
    }
    
    return count;
}

static ssize_t mc32x0_regs_show(struct device *dev, 
        struct device_attribute *attr, char *buf)
{
  int idx, len=0, result;
  unsigned char regs[0x13];

  result = mc32x0_read_reg(0x0, regs, 0x13);
  if(result != 0) 
  {
    printk(KERN_ERR "read reg error!\n");
    return -1;
  }

  for(idx=0; idx<0x13; idx++)
  {
    len += sprintf(buf+len, "[0x%x]=0x%x\n", idx, regs[idx]);
  }
  
  result = mc32x0_read_reg(0x20, regs, 0x0a);
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

static ssize_t mc32x0_regs_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int addr;
    unsigned int data;
    int error;
    
    sscanf(buf, "[0x%x]=0x%x", &addr, &data); 
    
    error = mc32x0_write_reg((unsigned char)addr, (unsigned char *)&data, 1);
    if(error) 
    {
        printk(KERN_INFO "write reg error!\n");
        return error;
    }
    
    return count;
}

static ssize_t mc32x0_value_show(struct device *dev, 
        struct device_attribute *attr, char *buf)
{
  int result;
  mc32x0acc_t acc;

  result = mc32x0_read_accel_xyz(&acc);
  if(result != 0) 
  {
    printk(KERN_ERR "read accel xyz error!\n");
    return -1;
  }
  
  mc32x0_axis_remap(&acc);
  
  return sprintf(buf, "%d,%d,%d\n", acc.x, acc.y, acc.z);
}

static ssize_t mc32x0_board_position_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    int data;

    data = atomic_read(&mc32x0_position);

    return sprintf(buf, "%d\n", data);
}

static ssize_t mc32x0_board_position_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    unsigned long data;
    int error;

    error = strict_strtol(buf, 10, &data);
    if (error)
        return error;

    atomic_set(&mc32x0_position, (int) data);

    return count;
}

static int mc32x0_calibration_offset(mc32x0acc_t *acc)
{    
    int result;
    unsigned char buf[9];
    short tmp, x_gain, y_gain, z_gain ;
    int x_off, y_off, z_off;

    //read register 0x21~0x29
    result = mc32x0_read_reg(0x21, buf, 9);
    
    // get x,y,z offset
    tmp = ((buf[1] & 0x3f) << 8) + buf[0];
    if (tmp & 0x2000)
        tmp |= 0xc000;
    x_off = tmp;
                    
    tmp = ((buf[3] & 0x3f) << 8) + buf[2];
    if (tmp & 0x2000)
        tmp |= 0xc000;
    y_off = tmp;
                    
    tmp = ((buf[5] & 0x3f) << 8) + buf[4];
    if (tmp & 0x2000)
        tmp |= 0xc000;
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
    result += mc32x0_write_reg(0x07, buf, 1);
    
    buf[0] = x_off & 0xff;
    buf[1] = ((x_off >> 8) & 0x3f) | (x_gain & 0x0100 ? 0x80 : 0);
    buf[2] = y_off & 0xff;
    buf[3] = ((y_off >> 8) & 0x3f) | (y_gain & 0x0100 ? 0x80 : 0);
    buf[4] = z_off & 0xff;
    buf[5] = ((z_off >> 8) & 0x3f) | (z_gain & 0x0100 ? 0x80 : 0);    
    result += mc32x0_write_reg(0x21, buf, 6);
    
    buf[0] = 0x41;
    result += mc32x0_write_reg(0x07, buf, 1);
    
    return result;
}

static ssize_t mc32x0_calibration_run_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int ret;
    int cfg_calibration[9];
    unsigned char offset[9];
    int idx;
    mc32x0acc_t acc;
    
    ret = mc32x0_read_accel_xyz(&acc);
    if(ret != 0) 
    {
        printk(KERN_ERR "read accel xyz error!\n");
        return -1;
    }

    // get diff
    acc.x = 0 - acc.x;
    acc.y = 0 - acc.y;
    if (atomic_read(&mc32x0_position) > 0) {
        acc.z = LSG - acc.z;
    } else {
        acc.z = (-LSG) - acc.z;
    }
    
    mc32x0_calibration_offset(&acc);

    if (mc32x0_get_offset(offset, sizeof(offset)) < 0)
        return sprintf((char*)buf, "Read error\n");
    
    for (idx = 0; idx < sizeof(offset); idx++)
    {
        cfg_calibration[idx] = offset[idx];
    }
    
    printk(KERN_INFO "run fast calibration finished\n");
    return count;
}

static ssize_t mc32x0_calibration_reset_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
    int ret;
    int cfg_calibration[9];
    int idx;
    
    mc32x0_set_offset(mc32x0_calib_default, sizeof(mc32x0_calib_default));    
    
    for (idx = 0; idx < sizeof(mc32x0_calib_default); idx++)
    {
        cfg_calibration[idx] = mc32x0_calib_default[idx];
    }

    printk(KERN_INFO "reset fast calibration finished\n");
    return count;
}

static ssize_t mc32x0_calibration_value_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    unsigned char offset[9];

    if (mc32x0_get_offset(offset, sizeof(offset)) < 0)
        return sprintf(buf, "Read error\n");

    return sprintf(buf, "%d %d %d %d %d %d %d %d %d\n", offset[0], offset[1], offset[2], 
                                offset[3], offset[4], offset[5], offset[6], offset[7], offset[8]);
}

static ssize_t mc32x0_calibration_value_store(struct device *dev,
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
    
    if (mc32x0_set_offset(offset, sizeof(offset)) < 0)
        return -EINVAL;

    printk(KERN_INFO "set fast calibration finished\n");
    return count;
}

/*    i2c delay routine for eeprom    */
static inline void mc32x0_i2c_delay(unsigned int msec)
{
    mdelay(msec);
}
#if 1
/*    i2c write routine for mc32x0    */
static inline char mc32x0_i2c_write(unsigned char reg_addr, unsigned char *data, unsigned char len)
{
    s32 dummy;
    if( mc32x0_client == NULL )    /*    No global client pointer?    */
        return -1;

        dummy = i2c_smbus_write_i2c_block_data(mc32x0_client, reg_addr, len, data);
        if(dummy < 0)
            return -1;        
            
    return 0;
}

/*    i2c read routine for mc32x0    */
static inline char mc32x0_i2c_read(unsigned char reg_addr, unsigned char *data, unsigned char len) 
{
    s32 dummy;
    if( mc32x0_client == NULL )    /*    No global client pointer?    */
        return -1;

        dummy = i2c_smbus_read_i2c_block_data(mc32x0_client, reg_addr, len, data);
        if(dummy < 0)
            return -1;        
    
    return 0;
}
#else
/*    i2c write routine for mc32x0    */
static inline char mc32x0_i2c_write(unsigned char reg_addr, unsigned char *data, unsigned char len)
{
    s32 dummy;
    unsigned char buffer[2];
    if( mc32x0_client == NULL )    /*    No global client pointer?    */
        return -1;

    while(len--)
    {
        buffer[0] = reg_addr;
        buffer[1] = *data;
        dummy = i2c_master_send(mc32x0_client, (char*)buffer, 2);

        reg_addr++;
        data++;
        if(dummy < 0)
            return -1;
    }
    return 0;
}

/*    i2c read routine for mc32x0    */
static inline char mc32x0_i2c_read(unsigned char reg_addr, unsigned char *data, unsigned char len) 
{
    s32 dummy;
    if( mc32x0_client == NULL )    /*    No global client pointer?    */
        return -1;

       
        dummy = i2c_master_send(mc32x0_client, (char*)&reg_addr, 1);
        if(dummy < 0)
            return -1;
        dummy = i2c_master_recv(mc32x0_client, (char*)data, len);
        if(dummy < 0)
            return -1;
        
    
    return 0;
}
#endif

static int mc32x0_power_on(void)
{
    if(mc32x0_regulator_name[0] != '\0') {        
        // request regulator
        mc32x0_regulator = regulator_get(NULL, mc32x0_regulator_name);
        if (IS_ERR(mc32x0_regulator)) {
            printk(KERN_ERR "mc32x0 get regulator failed\n");
            return -1;
        }
        
        if (regulator_set_voltage(mc32x0_regulator, mc32x0_voltage, mc32x0_voltage)) {
            printk(KERN_ERR "mc32x0 set regulator voltage failed\n");
            regulator_put(mc32x0_regulator);
            return -1;
        }
        
        regulator_enable(mc32x0_regulator);
        msleep(20);
        
    #ifdef MC32X0_DEBUG
        printk(KERN_INFO "%s\n",__FUNCTION__);
    #endif
    }

    return 0;
}

static int mc32x0_power_off(void)
{
    if(mc32x0_regulator != NULL) {
        // save current voltage
        mc32x0_voltage = regulator_get_voltage(mc32x0_regulator);
        
        regulator_disable(mc32x0_regulator);    
        
        // release regulator
        regulator_put(mc32x0_regulator);
        
    #ifdef MC32X0_DEBUG
        printk(KERN_INFO "%s\n",__FUNCTION__);
    #endif
    }
    
    return 0;
}

static int mc32x0_read_file(char *path, char *buf, int size)
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

static int mc32x0_load_user_calibration(void)
{
    char buffer[64];
    int ret = 0;
    int data[9];
    unsigned char offset[9];
    int idx;
    
    // only calibrate once
    if (mc32x0_calib_inited) {
        goto usr_calib_end;
    } else {
        mc32x0_calib_inited = 1;
    }

    ret = mc32x0_read_file(CFG_GSENSOR_CALIBFILE, buffer, sizeof(buffer));
    if (ret <= 0) {
        printk(KERN_ERR "gsensor calibration file not exist!\n");
        goto usr_calib_end;
    }
    
    sscanf(buffer, "%d %d %d %d %d %d %d %d %d", &data[0], &data[1], &data[2],
                                &data[3], &data[4], &data[5], &data[6], &data[7], &data[8]);
                                
    for (idx = 0; idx < sizeof(offset); idx ++) {
        offset[idx] = (unsigned char) data[idx];
    }
    
    if (mc32x0_set_offset(offset, sizeof(offset)) < 0) {
        printk(KERN_ERR"set offset fail\n");
        goto usr_calib_end;
    }
    
    printk(KERN_INFO "load user calibration finished\n");
    
usr_calib_end:
    return ret;
}

static int mc32x0_axis_remap(mc32x0acc_t *acc)
{
    s16 swap;
    int position = atomic_read(&mc32x0_position);

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

static void mc32x0_work_func(struct work_struct *work)
{
    mc32x0acc_t acc;
    unsigned long delay = msecs_to_jiffies(atomic_read(&mc32x0_delay));

    mc32x0_load_user_calibration();
    
    mc32x0_read_accel_xyz(&acc);
    mc32x0_axis_remap(&acc);
    input_report_abs(mc32x0_idev, ABS_X, acc.x);
    input_report_abs(mc32x0_idev, ABS_Y, acc.y);
    input_report_abs(mc32x0_idev, ABS_Z, acc.z);
    input_sync(mc32x0_idev);
    
    schedule_delayed_work(&mc32x0_work, delay);
}

static int mc32x0_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;
#ifdef MC32X0_DEBUG
    printk(KERN_INFO "%s\n", __FUNCTION__);
#endif
    if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
        return -ENODEV;

    strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);

    return 0;
}

static int mc32x0_probe(struct i2c_client *client,
             const struct i2c_device_id *id)
{
        struct mc32x0_data *data;
        int cfg_position;
        int err = 0;
        int tempvalue;
        int cfg_calibration[3];
        mc32x0acc_t acc;
    	char * buf;
			int temp;
#ifdef MC32X0_DEBUG
        printk(KERN_INFO "%s\n",__FUNCTION__);
#endif
    
        if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
        {
            printk(KERN_INFO "i2c_check_functionality error\n");
            goto exit;
        }
    
        /* OK. For now, we presume we have a valid client. We now create the
           client structure, even though we cannot fill it completely yet. */
        if (!(data = kmalloc(sizeof(struct mc32x0_data), GFP_KERNEL)))
        {
            err = -ENOMEM;
            printk(KERN_INFO "kmalloc error\n");
            goto exit;
        }
        memset(data, 0, sizeof(struct mc32x0_data));
    
        i2c_set_clientdata(client, data);
        
        /* read chip id */
        tempvalue = i2c_smbus_read_byte_data(client, MC32X0_CHIP_ID);

        if((tempvalue&0x00FF) == 0x0001)
        {
            printk(KERN_INFO "mCube Device detected!\nMC32X0 registered I2C driver!\n");
            mc32x0_client = client;
        }
        else
        {
            printk(KERN_INFO "mCube Device not found, i2c error %d \n", tempvalue);
            
            data = i2c_get_clientdata(client);
    
            mc32x0_client = NULL;
            err = -1;
            goto exit_kfree;
        }
#if CFG_GSENSOR_USE_CONFIG > 0
        err = of_property_read_string(client->dev.of_node, "position", &buf);
        if (err != 0) {
            printk(KERN_ERR"get position fail\n");
        //    goto exit_kfree;
        }
      err = strict_strtol(buf, 10, &temp);	
			cfg_position = (int)temp;
#else
        cfg_position = 4;
#endif
	printk(KERN_ERR "position is %d\n", cfg_position);
	
        atomic_set(&mc32x0_position, cfg_position);
        atomic_set(&mc32x0_delay, MC32X0_POLL_INTERVAL);
        atomic_set(&mc32x0_enable, 0); 
        atomic_set(&mc32x0_fuzz, FUZZ);       
        INIT_DELAYED_WORK(&mc32x0_work, mc32x0_work_func);
    
        /*input poll device register */
        mc32x0_idev = input_allocate_device();
        if (!mc32x0_idev) {
            printk(KERN_ERR"alloc poll device failed!\n");
            goto exit_kfree;
        }

        mc32x0_idev->name = SENSOR_NAME;
        mc32x0_idev->id.bustype = BUS_I2C;
//        mc32x0_idev->dev.parent = &client->dev;
        
        input_set_capability(mc32x0_idev, EV_ABS, ABS_MISC);
        input_set_abs_params(mc32x0_idev, ABS_X, ABSMIN, ABSMAX, FUZZ, 0);
        input_set_abs_params(mc32x0_idev, ABS_Y, ABSMIN, ABSMAX, FUZZ, 0);
        input_set_abs_params(mc32x0_idev, ABS_Z, ABSMIN, ABSMAX, FUZZ, 0);

        err = input_register_device(mc32x0_idev);
        if (err) {
            printk(KERN_ERR "mc32x0 input register failed\n");
            goto error_register;
        }
  
        err = sysfs_create_group(&mc32x0_idev->dev.kobj, &mc32x0_group);
        if (err) {
            printk(KERN_ERR "mc32x0 create sysfs group failed\n");
            goto error_register;
        }
  
#ifdef CONFIG_HAS_EARLYSUSPEND
        mc32x0_es_handler.suspend = mc32x0_early_suspend;
        mc32x0_es_handler.resume = mc32x0_early_resume;
        register_early_suspend(&mc32x0_es_handler);
#endif

        printk(KERN_INFO "mc32x0 device create ok: %s\n", SENSOR_NAME);
        
        data->mc32x0.bus_write = mc32x0_i2c_write;
        data->mc32x0.bus_read = mc32x0_i2c_read;
        mcube_mc32x0_init(&(data->mc32x0));
        
        mc32x0_set_image();    
        
        // save default offset & gain
        mc32x0_get_offset(mc32x0_calib_default, sizeof(mc32x0_calib_default));

#if CFG_GSENSOR_USE_CONFIG > 0
        /*get xml cfg*/
        err = of_property_read_u32_array(client->dev.of_node, "calibration_table", cfg_calibration, 3);
        if (err != 0) {
            printk(KERN_ERR"get calibration fail\n");
           // goto error_register;
        }
#else
        memset(cfg_calibration, 0, sizeof(cfg_calibration));
#endif    
    
        acc.x = (unsigned char) cfg_calibration[0];
        acc.y = (unsigned char) cfg_calibration[1];
        acc.z = (unsigned char) cfg_calibration[2];

        // calibration from xml config
        mc32x0_calibration_offset(&acc);

        return 0;
        
error_register:
        input_unregister_device(mc32x0_idev);
exit_kfree:
        kfree(data);
 exit:
        return err;
}



static int mc32x0_remove(struct i2c_client *client)
{
    struct mc32x0_data *data;
#ifdef MC32X0_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif    
    mc32x0_set_mode(MC32X0_STANDBY);
    mc32x0_power_off();

    sysfs_remove_group(&client->dev.kobj, &mc32x0_group);

    data = i2c_get_clientdata(client);
    mc32x0_client = NULL;
    
    if (mc32x0_idev) 
    {
        printk(KERN_INFO "remove input device\n");
        input_unregister_device(mc32x0_idev);
        input_free_device(mc32x0_idev);
        mc32x0_idev = NULL;
    }
    kfree(data);
    return 0;
}

#ifdef CONFIG_PM
static int mc32x0_suspend(struct device *dev)
{    
#ifdef MC32X0_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif

    // save offset 
    mc32x0_get_offset(mc32x0_offset_saved, sizeof(mc32x0_offset_saved));      
    
    mc32x0_do_enable(dev, 0);      
    //mc32x0_power_off();
    
    return 0;
}

static int mc32x0_resume(struct device *dev)
{
#ifdef MC32X0_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif    

    mc32x0_set_image();     
    mc32x0_do_enable(dev, atomic_read(&mc32x0_enable));
    
    // restore offset 
    mc32x0_set_offset(mc32x0_offset_saved, sizeof(mc32x0_offset_saved));        
    
    return 0;
}
#else

#define mc32x0_suspend NULL
#define mc32x0_resume NULL

#endif /* CONFIG_PM */

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mc32x0_early_suspend(struct early_suspend *handler)
{
#ifdef MC32X0_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif    
    // sensor hal will disable when early suspend
}

static void mc32x0_early_resume(struct early_suspend *handler)
{ 
#ifdef MC32X0_DEBUG
    printk(KERN_INFO "%s\n",__FUNCTION__);
#endif    
    // sensor hal will enable when early resume
}
#endif

static SIMPLE_DEV_PM_OPS(mc32x0_pm_ops, mc32x0_suspend, mc32x0_resume);

//static unsigned short normal_i2c[] = { MC32X0_I2C_ADDR, I2C_CLIENT_END};

static const struct i2c_device_id mc32x0_id[] = {
    { SENSOR_NAME, 0 },
    { }
};

static struct of_device_id mc323x_of_match[] = {
	{ .compatible = "mc323x" },
	{ }
};

MODULE_DEVICE_TABLE(i2c, mc32x0_id);

static struct i2c_driver mc32x0_driver = {    
    .driver = {
        .owner    = THIS_MODULE,    
        .name    = SENSOR_NAME,
        .pm    = &mc32x0_pm_ops,
        .of_match_table	= of_match_ptr(mc323x_of_match),
    },
    .class        = I2C_CLASS_HWMON,
    .id_table    = mc32x0_id,
//    .address_list    = normal_i2c,
    .probe        = mc32x0_probe,
    .remove        = mc32x0_remove,
    .detect        = mc32x0_detect,
};

static struct i2c_board_info mc32x0_board_info={
    .type = SENSOR_NAME, 
    .addr = MC32X0_I2C_ADDR,
};

static struct i2c_client *mc32x0_init_client;

static int __init MC32X0_init(void)
{
    printk(KERN_ERR "MC32X0 init\n");    
    return i2c_add_driver(&mc32x0_driver);
}

static void __exit MC32X0_exit(void)
{
    i2c_del_driver(&mc32x0_driver);
    printk(KERN_ERR "MC32X0 exit\n");
}

MODULE_DESCRIPTION("MC32X0 driver");
MODULE_LICENSE("GPL");

module_init(MC32X0_init);
module_exit(MC32X0_exit);
