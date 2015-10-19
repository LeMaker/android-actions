/*
 * module different macro
 *
 * Copyright (C) 2008 Renesas Solutions Corp.
 * Kuninori Morimoto <morimoto.kuninori@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MODULE_DIFF_H__
#define __MODULE_DIFF_H__

#include "./../module_comm/module_comm.h"

#define CAMERA_MODULE_NAME      "gc0312"
#define CAMERA_MODULE_PID       0xb310  
#define VERSION(pid, ver)       ((pid<<8)|(ver&0xFF))

#define MODULE_PLATFORM_ID				GC0312_PLATFORM_ID
#define MODULE_I2C_REAL_ADDRESS		(0x42>>1)
#define MODULE_I2C_REG_ADDRESS		(0x42>>1)
#define I2C_REGS_WIDTH			1
#define I2C_DATA_WIDTH			1
#define IS_FRONT_OR_REAR        1
#define DEFAULT_VSYNC_ACTIVE_LEVEL		V4L2_MBUS_VSYNC_ACTIVE_LOW
#define DEFAULT_PCLK_SAMPLE_EDGE      V4L2_MBUS_PCLK_SAMPLE_RISING
#define DEFAULT_POWER_LINE_FREQUENCY    V4L2_CID_POWER_LINE_FREQUENCY_50HZ 

#if 0
#define PID						XXX /* Product ID Number */
#else
#define PIDH					0xf0 /* Product ID Number H byte */
#define PIDL					0xf1 /* Product ID Number L byte */
#endif

#define OUTTO_SENSO_CLOCK 		24000000


#define MODULE_DEFAULT_WIDTH	WIDTH_VGA
#define MODULE_DEFAULT_HEIGHT	HEIGHT_VGA
#define MODULE_MAX_WIDTH		WIDTH_VGA
#define MODULE_MAX_HEIGHT		HEIGHT_VGA

#define AHEAD_LINE_NUM			15    //10行 = 50次循环
#define DROP_NUM_CAPTURE		4
#define DROP_NUM_PREVIEW		4

static unsigned int frame_rate_qvga[] = {30,};
static unsigned int frame_rate_vga[]  = {30,};

static const struct regval_list module_init_regs[] =
{
    
    {0xfe, 0xf0},
    {0xfe, 0xf0},
    {0xfe, 0x00},
    {0xfc, 0x0e}, 
    {0xfc, 0x0e}, 
    {0xf2, 0x07}, 
    {0xf3, 0x00},// output_disable 
    {0xf7, 0x1b}, 
    {0xf8, 0x04}, 
    {0xf9, 0x0e}, 
    {0xfa, 0x11},
        
        /////////////////////////////////////////////////
        /////////////////  CISCTL reg   /////////////////
        /////////////////////////////////////////////////
    {0x00, 0x2f},
    {0x01, 0x0f},//06
    {0x02, 0x04},
    {0x03, 0x03},
    {0x04, 0x50},
    {0x09, 0x00},
    {0x0a, 0x00},
    {0x0b, 0x00},
    {0x0c, 0x04},
    {0x0d, 0x01},
    {0x0e, 0xe8},
    {0x0f, 0x02},
    {0x10, 0x88},
    {0x16, 0x00},
    {0x17, 0x14},
    {0x18, 0x1a},
    {0x19, 0x14},
    {0x1b, 0x48},
    {0x1e, 0x6b},
    {0x1f, 0x28},
    {0x20, 0x89},
    {0x21, 0x49},
    {0x22, 0xb0},
    {0x23, 0x04},
    {0x24, 0x16},
    {0x34, 0x20},
        
        /////////////////////////////////////////////////
        ////////////////////   BLK   ////////////////////
        /////////////////////////////////////////////////
    {0x26, 0x23},
    {0x28, 0xff},
    {0x29, 0x00},
    {0x33, 0x10}, 
    {0x37, 0x20},
    {0x38, 0x10},
    {0x47, 0x80},
    {0x4e, 0x66},
    {0xa8, 0x02},
    {0xa9, 0x80},
        
        /////////////////////////////////////////////////
        //////////////////  ISP reg   ///////////////////
        /////////////////////////////////////////////////
    {0x40, 0xff},
    {0x41, 0x21},
    {0x42, 0xcf},
//  {0x44, 0x02},
    {0x45, 0xa8}, 
    {0x46, 0x02}, //sync
    {0x4a, 0x11},
    {0x4b, 0x01},
    {0x4c, 0x20},
    {0x4d, 0x05},
    {0x4f, 0x01},
    {0x50, 0x01},
    {0x55, 0x01},
    {0x56, 0xe0},
    {0x57, 0x02},
    {0x58, 0x80},
        
        /////////////////////////////////////////////////
        ///////////////////   GAIN   ////////////////////
        /////////////////////////////////////////////////
    {0x70, 0x70},
    {0x5a, 0x84},
    {0x5b, 0xc9},
    {0x5c, 0xed},
    {0x77, 0x74},
    {0x78, 0x40},
    {0x79, 0x5f},
        
        ///////////////////////////////////////////////// 
        ///////////////////   DNDD  /////////////////////
        ///////////////////////////////////////////////// 
    {0x82, 0x14}, 
    {0x83, 0x0b},
    {0x89, 0xf0},
        
        ///////////////////////////////////////////////// 
        //////////////////   EEINTP  ////////////////////
        ///////////////////////////////////////////////// 
    {0x8f, 0xaa}, 
    {0x90, 0x8c}, 
    {0x91, 0x90},
    {0x92, 0x03}, 
    {0x93, 0x03}, 
    {0x94, 0x05}, 
    {0x95, 0x65}, 
    {0x96, 0xf0}, 
        
        ///////////////////////////////////////////////// 
        /////////////////////  ASDE  ////////////////////
        ///////////////////////////////////////////////// 
    {0xfe, 0x00},
                                           
    {0x9a, 0x20},
    {0x9b, 0x80},
    {0x9c, 0x40},
    {0x9d, 0x80},
                                       
    {0xa1, 0x30},
     {0xa2, 0x32},
    {0xa4, 0x30},
    {0xa5, 0x30},
    {0xaa, 0x10}, 
    {0xac, 0x22},
         
        /////////////////////////////////////////////////
        ///////////////////   GAMMA   ///////////////////
        /////////////////////////////////////////////////
    {0xfe, 0x00},//default
    {0xbf, 0x08},
    {0xc0, 0x16},
    {0xc1, 0x28},
    {0xc2, 0x41},
    {0xc3, 0x5a},
    {0xc4, 0x6c},
    {0xc5, 0x7a},
    {0xc6, 0x96},
    {0xc7, 0xac},
    {0xc8, 0xbc},
    {0xc9, 0xc9},
    {0xca, 0xd3},
    {0xcb, 0xdd},
    {0xcc, 0xe5},
    {0xcd, 0xf1},
    {0xce, 0xfa},
    {0xcf, 0xff},
        
    /* 
    {0xfe, 0x00},//big gamma
    {0xbf, 0x08},
    {0xc0, 0x1d},
    {0xc1, 0x34},
    {0xc2, 0x4b},
    {0xc3, 0x60},
    {0xc4, 0x73},
    {0xc5, 0x85},
    {0xc6, 0x9f},
    {0xc7, 0xb5},
    {0xc8, 0xc7},
    {0xc9, 0xd5},
    {0xca, 0xe0},
    {0xcb, 0xe7},
    {0xcc, 0xec},
    {0xcd, 0xf4},
    {0xce, 0xfa},
    {0xcf, 0xff},
    */  
    
    /*
    {0xfe, 0x00},//small gamma
    {0xbf, 0x08},
    {0xc0, 0x18},
    {0xc1, 0x2c},
    {0xc2, 0x41},
    {0xc3, 0x59},
    {0xc4, 0x6e},
    {0xc5, 0x81},
    {0xc6, 0x9f},
    {0xc7, 0xb5},
    {0xc8, 0xc7},
    {0xc9, 0xd5},
    {0xca, 0xe0},
    {0xcb, 0xe7},
    {0xcc, 0xec},
    {0xcd, 0xf4},
    {0xce, 0xfa},
    {0xcf, 0xff},
    */
    
#if 0
            case GC0310MIPI_RGB_Gamma_m1:                       //smallest gamma curve
                {0xfe, 0x00},
                {0xbf, 0x06},
                {0xc0, 0x12},
                {0xc1, 0x22},
                {0xc2, 0x35},
                {0xc3, 0x4b},
                {0xc4, 0x5f},
                {0xc5, 0x72},
                {0xc6, 0x8d},
                {0xc7, 0xa4},
                {0xc8, 0xb8},
                {0xc9, 0xc8},
                {0xca, 0xd4},
                {0xcb, 0xde},
                {0xcc, 0xe6},
                {0xcd, 0xf1},
                {0xce, 0xf8},
                {0xcf, 0xfd},
                break;
            case GC0310MIPI_RGB_Gamma_m2:
                {0xBF, 0x08},
                {0xc0, 0x0F},
                {0xc1, 0x21},
                {0xc2, 0x32},
                {0xc3, 0x43},
                {0xc4, 0x50},
                {0xc5, 0x5E},
                {0xc6, 0x78},
                {0xc7, 0x90},
                {0xc8, 0xA6},
                {0xc9, 0xB9},
                {0xcA, 0xC9},
                {0xcB, 0xD6},
                {0xcC, 0xE0},
                {0xcD, 0xEE},
                {0xcE, 0xF8},
                {0xcF, 0xFF},
                break;
                
            case GC0310MIPI_RGB_Gamma_m3:           
                {0xBF, 0x0B},
                {0xc0, 0x16},
                {0xc1, 0x29},
                {0xc2, 0x3C},
                {0xc3, 0x4F},
                {0xc4, 0x5F},
                {0xc5, 0x6F},
                {0xc6, 0x8A},
                {0xc7, 0x9F},
                {0xc8, 0xB4},
                {0xc9, 0xC6},
                {0xcA, 0xD3},
                {0xcB, 0xDD},
                {0xcC, 0xE5},
                {0xcD, 0xF1},
                {0xcE, 0xFA},
                {0xcF, 0xFF},
                
                
            case GC0310MIPI_RGB_Gamma_m4:
                {0xBF, 0x0E},
                {0xc0, 0x1C},
                {0xc1, 0x34},
                {0xc2, 0x48},
                {0xc3, 0x5A},
                {0xc4, 0x6B},
                {0xc5, 0x7B},
                {0xc6, 0x95},
                {0xc7, 0xAB},
                {0xc8, 0xBF},
                {0xc9, 0xCE},
                {0xcA, 0xD9},
                {0xcB, 0xE4},
                {0xcC, 0xEC},
                {0xcD, 0xF7},
                {0xcE, 0xFD},
                {0xcF, 0xFF},
                
                
            case GC0310MIPI_RGB_Gamma_m5:
                {0xBF, 0x10},
                {0xc0, 0x20},
                {0xc1, 0x38},
                {0xc2, 0x4E},
                {0xc3, 0x63},
                {0xc4, 0x76},
                {0xc5, 0x87},
                {0xc6, 0xA2},
                {0xc7, 0xB8},
                {0xc8, 0xCA},
                {0xc9, 0xD8},
                {0xcA, 0xE3},
                {0xcB, 0xEB},
                {0xcC, 0xF0},
                {0xcD, 0xF8},
                {0xcE, 0xFD},
                {0xcF, 0xFF},
                
                
            case GC0310MIPI_RGB_Gamma_m6:// largest gamma curve
                {0xBF, 0x14},
                {0xc0, 0x28},
                {0xc1, 0x44},
                {0xc2, 0x5D},
                {0xc3, 0x72},
                {0xc4, 0x86},
                {0xc5, 0x95},
                {0xc6, 0xB1},
                {0xc7, 0xC6},
                {0xc8, 0xD5},
                {0xc9, 0xE1},
                {0xcA, 0xEA},
                {0xcB, 0xF1},
                {0xcC, 0xF5},
                {0xcD, 0xFB},
                {0xcE, 0xFE},
                {0xcF, 0xFF},
                
            case GC0310MIPI_RGB_Gamma_night:        //Gamma for night mode
                {0xBF, 0x0B},
                {0xc0, 0x16},
                {0xc1, 0x29},
                {0xc2, 0x3C},
                {0xc3, 0x4F},
                {0xc4, 0x5F},
                {0xc5, 0x6F},
                {0xc6, 0x8A},
                {0xc7, 0x9F},
                {0xc8, 0xB4},
                {0xc9, 0xC6},
                {0xcA, 0xD3},
                {0xcB, 0xDD},
                {0xcC, 0xE5},
                {0xcD, 0xF1},
                {0xcE, 0xFA},
                {0xcF, 0xFF},
                break;
            default:
                //GC0310MIPI_RGB_Gamma_m1
                {0xfe, 0x00},
                {0xbf, 0x06},
                {0xc0, 0x12},
                {0xc1, 0x22},
                {0xc2, 0x35},
                {0xc3, 0x4b},
                {0xc4, 0x5f},
                {0xc5, 0x72},
                {0xc6, 0x8d},
                {0xc7, 0xa4},
                {0xc8, 0xb8},
                {0xc9, 0xc8},
                {0xca, 0xd4},
                {0xcb, 0xde},
                {0xcc, 0xe6},
                {0xcd, 0xf1},
                {0xce, 0xf8},
                {0xcf, 0xfd},
#endif
        /////////////////////////////////////////////////
        ///////////////////   YCP  //////////////////////
        /////////////////////////////////////////////////
    {0xd0, 0x40},
    {0xd1, 0x34}, 
    {0xd2, 0x34}, 
    {0xd3, 0x40}, 
    {0xd6, 0xf2},
    {0xd7, 0x1b},
    {0xd8, 0x18},
    {0xdd, 0x03}, 
        
        /////////////////////////////////////////////////
        ////////////////////   AEC   ////////////////////
        /////////////////////////////////////////////////
    {0xfe, 0x01},
    {0x05, 0x30}, 
    {0x06, 0x75}, 
    {0x07, 0x40}, 
    {0x08, 0xb0}, 
    {0x0a, 0xc5}, 
    {0x0b, 0x11}, 
    {0x0c, 0x00},
    {0x12, 0x52}, 
    {0x13, 0x38}, 
    {0x18, 0x95}, 
    {0x19, 0x96}, 
    {0x1f, 0x20}, 
    {0x20, 0xc0}, //80
    {0x3e, 0x40}, 
    {0x3f, 0x57}, 
    {0x40, 0x7d}, 
    {0x03, 0x60},
    {0x44, 0x02},
        
        /////////////////////////////////////////////////
        ////////////////////   AWB   ////////////////////
        /////////////////////////////////////////////////
    {0x1c, 0x91}, 
    {0x21, 0x15}, 
    {0x50, 0x80}, 
    {0x56, 0x04}, 
    {0x59, 0x08}, 
    {0x5b, 0x02},
    {0x61, 0x8d}, 
    {0x62, 0xa7}, 
    {0x63, 0xd0}, 
    {0x65, 0x06},
    {0x66, 0x06}, 
    {0x67, 0x84}, 
    {0x69, 0x08},
    {0x6a, 0x25},//50
    {0x6b, 0x01}, 
    {0x6c, 0x00}, 
    {0x6d, 0x02}, 
    {0x6e, 0xf0}, 
    {0x6f, 0x80}, 
    {0x76, 0x80},
    {0x78, 0xaf}, 
    {0x79, 0x75},
    {0x7a, 0x40},
    {0x7b, 0x50},   
    {0x7c, 0x0c},
                                       
    {0xa4, 0xb9}, 
    {0xa5, 0xa0},
    {0x90, 0xc9},
    {0x91, 0xbe},
                                       
    {0xa6, 0xb8},
    {0xa7, 0x95},
    {0x92, 0xe6},
    {0x93, 0xca},
                                       
    {0xa9, 0xbc}, 
    {0xaa, 0x95}, 
    {0x95, 0x23},
    {0x96, 0xe7},
                                       
    {0xab, 0x9d},
    {0xac, 0x80},
    {0x97, 0x43},
    {0x98, 0x24},
                                       
    {0xae, 0xb7},
    {0xaf, 0x9e},
    {0x9a, 0x43},
    {0x9b, 0x24},
                                       
    {0xb0, 0xc8},
    {0xb1, 0x97},
    {0x9c, 0xc4},
    {0x9d, 0x44},
        
    {0xb3, 0xb7},
    {0xb4, 0x7f},
    {0x9f, 0xc7},
    {0xa0, 0xc8},
        
    {0xb5, 0x00},
    {0xb6, 0x00},
    {0xa1, 0x00},
    {0xa2, 0x00},
        
    {0x86, 0x60},
    {0x87, 0x08},
    {0x88, 0x00},
    {0x89, 0x00},
    {0x8b, 0xde},
    {0x8c, 0x80},
    {0x8d, 0x00},
    {0x8e, 0x00},
        
    {0x94, 0x55},
    {0x99, 0xa6},
    {0x9e, 0xaa},
    {0xa3, 0x0a},
    {0x8a, 0x0a},
    {0xa8, 0x55},
    {0xad, 0x55},
    {0xb2, 0x55},
    {0xb7, 0x05},
    {0x8f, 0x05},
    
    {0xb8, 0xcc}, 
    {0xb9, 0x9a}, 
        /////////////////////////////////////////////////
        ////////////////////   CC    ////////////////////
        /////////////////////////////////////////////////
    {0xfe, 0x01},
        
    {0xd0, 0x38},//skin red
    {0xd1, 0x00},
    {0xd2, 0x02},
    {0xd3, 0x04},
    {0xd4, 0x38},
    {0xd5, 0x12},   
    /*                     
    {0xd0, 0x38},//skin white
    {0xd1, 0xfd},
    {0xd2, 0x06},
    {0xd3, 0xf0},
    {0xd4, 0x40},
    {0xd5, 0x08},
    */
        
    /*                       
    {0xd0, 0x38},//guodengxiang
    {0xd1, 0xf8},
    {0xd2, 0x06},
    {0xd3, 0xfd},
    {0xd4, 0x40},
    {0xd5, 0x00},   
    */
        
    {0xd6, 0x30},
    {0xd7, 0x00},
    {0xd8, 0x0a},
    {0xd9, 0x16},
    {0xda, 0x39},
    {0xdb, 0xf8},
    
        /////////////////////////////////////////////////
        ////////////////////   LSC   ////////////////////
        /////////////////////////////////////////////////
    {0xfe, 0x01},
    {0xc1, 0x3c},
    {0xc2, 0x50},
    {0xc3, 0x00},
    {0xc4, 0x40},
    {0xc5, 0x30},
    {0xc6, 0x30},
    {0xc7, 0x10},
    {0xc8, 0x00},
    {0xc9, 0x00},
    {0xdc, 0x20},
    {0xdd, 0x10},
    {0xdf, 0x00},
    {0xde, 0x00},
        
        /////////////////////////////////////////////////
        ///////////////////  Histogram  /////////////////
        /////////////////////////////////////////////////
    {0x01, 0x10},
    {0x0b, 0x31},
    {0x0e, 0x50},
    {0x0f, 0x0f},
    {0x10, 0x6e},
    {0x12, 0xa0},
    {0x15, 0x60},
    {0x16, 0x60},
    {0x17, 0xe0},
        
        /////////////////////////////////////////////////
        //////////////  Measure Window    ///////////////
        /////////////////////////////////////////////////
    {0xcc, 0x0c}, 
    {0xcd, 0x10},
    {0xce, 0xa0},
    {0xcf, 0xe6},
        
        /////////////////////////////////////////////////
        /////////////////   dark sun   //////////////////
        /////////////////////////////////////////////////
    {0x45, 0xf7},
    {0x46, 0xff},
    {0x47, 0x15},
    {0x48, 0x03}, 
    {0x4f, 0x60},
    
        //////////////////banding//////////////////////
    {0xfe, 0x00}, 
    {0x05, 0x02},
    {0x06, 0xd1}, //HB
    {0x07, 0x00},
    {0x08, 0x22}, //VB
        
    {0xfe, 0x01},
    {0x25, 0x00},   //anti-flicker step [11:8]
    {0x26, 0x6a},   //anti-flicker step [7:0]
    
    {0x27, 0x02},   //exp level 0  20fps
    {0x28, 0x12}, 
    {0x29, 0x03},   //exp level 1  12.50fps
    {0x2a, 0x50}, 
    {0x2b, 0x05},   //7.14fps
    {0x2c, 0xcc}, 
    {0x2d, 0x07},   //exp level 3  5.55fps
    {0x2e, 0x74}, 
    {0x3c, 0x20},   
    {0xfe, 0x00},
        
        /////////////////////////////////////////////////
        /////////////////////  DVP   ////////////////////
        /////////////////////////////////////////////////
    {0xfe, 0x03},
    {0x01, 0x00},
    {0x02, 0x00},
    {0x10, 0x00},
    {0x15, 0x00},
    {0xfe, 0x00},
        ///////////////////OUTPUT//////////////////////
    {0xf3, 0xff},// output_enable
    
		ENDMARKER,
};              

/* 320*240: QVGA */
static const struct regval_list module_qvga_regs[] = 
{
    {0xfe, 0x00}, //
    {0x18, 0x7a}, //
    {0x50, 0x01}, //crop enable
    {0x55, 0x00}, //crop window height
    {0x56, 0xf0},
    {0x57, 0x01}, //crop window width
    {0x58, 0x40},
        
    {0xfe, 0x01}, //
    {0xc1, 0x3c}, //row center
    {0xc2, 0x50}, //col center
    {0xcc, 0x06}, //0c  //aec window size 
    {0xcd, 0x06}, //10 
    {0xce, 0x28}, //a0 
    {0xcf, 0x3a}, //e6 
    {0xfe, 0x00}, //
    ENDMARKER,
    
};

/* 640*480: VGA*/
static const struct regval_list module_vga_regs[] = 
{
    {0xfe, 0x00}, 
    {0x18, 0x1a},  
    {0x50, 0x01}, //crop enable
    {0x55, 0x01}, //crop window height
    {0x56, 0xe0},
    {0x57, 0x02}, //crop window width
    {0x58, 0x80},
        
    {0xfe, 0x01},  
    {0xc1, 0x3c},
    {0xc2, 0x50},
    {0xcc, 0x0c}, 
    {0xcd, 0x10},
    {0xce, 0xa0},
    {0xcf, 0xe6},
    {0xfe, 0x00},  
    ENDMARKER,
};
static const struct regval_list module_init_auto_focus[] =
{
	ENDMARKER,
//  NULL
};

/*
 * window size list
 */
/* 320*240 */
static struct camera_module_win_size module_win_qvga = {
	.name             = "QVGA",
	.width            = WIDTH_QVGA,
	.height           = HEIGHT_QVGA,
	.win_regs         = module_qvga_regs,

	.frame_rate_array = frame_rate_qvga,
	.capture_only     = 0,
};

/* 640*480 */
static struct camera_module_win_size module_win_vga = {
	.name             = "VGA",
	.width            = WIDTH_VGA,
	.height           = HEIGHT_VGA,
	.win_regs         = module_vga_regs,
	.frame_rate_array = frame_rate_vga,
	.capture_only     = 0,
};
static struct camera_module_win_size *module_win_list[] = 
{
//    &module_win_qvga,
    &module_win_vga,
};

static struct regval_list module_whitebance_auto_regs[]=
{
    {0x42, 0x8f},
		ENDMARKER,
};

/* Cloudy Colour Temperature : 6500K - 8000K  */
static struct regval_list module_whitebance_cloudy_regs[]=
{
    {0x42, 0x8d},
    {0x77, 0x8c}, //WB_manual_gain 
    {0x78, 0x50},
    {0x79, 0x40},
		ENDMARKER,

};

/* ClearDay Colour Temperature : 5000K - 6500K  */
static struct regval_list module_whitebance_sunny_regs[]=
{
    {0x42, 0x8d},
    {0x77, 0x74}, 
    {0x78, 0x52},
    {0x79, 0x40},   
		ENDMARKER,
};

/* Office Colour Temperature : 3500K - 5000K ,荧光灯 */
static struct regval_list module_whitebance_fluorescent_regs[]=
{
    {0x42, 0x8d},
    {0x77, 0x40},
    {0x78, 0x42},
    {0x79, 0x50},
		ENDMARKER,
};

/* Home Colour Temperature : 2500K - 3500K ，白炽灯 */
static struct regval_list module_whitebance_incandescent_regs[]=
{
    {0x42, 0x8d},
    {0x77, 0x48},
    {0x78, 0x40},
    {0x79, 0x5c},
    ENDMARKER,
};
#if 0
/*正常模式*/
static struct regval_list module_effect_normal_regs[] =
{
    {0x43 , 0x00},
		ENDMARKER,
};

/*单色，黑白照片*/
static struct regval_list module_effect_white_black_regs[] =
{
    {0x43 , 0x02},
    {0xda , 0x00},
    {0xdb , 0x00},
		ENDMARKER,
};

/*负片效果*/
static struct regval_list module_effect_negative_regs[] =
{
    {0x43 , 0x01},
		ENDMARKER,
};
/*复古效果*/
static struct regval_list module_effect_antique_regs[] =
{    
    {0x43 , 0x02},
    {0xda , 0xd0},
    {0xdb , 0x28},
		ENDMARKER,
};
#endif
static struct regval_list module_scene_auto_regs[] =
{
    {0xfe, 0x01},
    {0x3c, 0x20},
    {0xfe, 0x00},
    ENDMARKER,
};
#if 0
static struct regval_list module_scene_night_regs[] =
{
    {0xfe, 0x01},
    {0x3c, 0x30},
    {0xfe, 0x00},
		ENDMARKER,
};
#endif
static struct v4l2_ctl_cmd_info v4l2_ctl_array[] =
{
	{
        .id = V4L2_CID_AUTO_WHITE_BALANCE,
        .min = 0,
        .max = 1,
        .step = 1,
        .def = 1,
    },
    {
        .id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
        .min = 0,
        .max = 3,
        .step = 1,
        .def = 0,
    },

    {
        .id = V4L2_CID_HFLIP,
        .min = 0,
        .max = 1,
        .step = 1,
        .def = 0,
    },
    {
        .id = V4L2_CID_VFLIP,
        .min = 0,
        .max = 1,
        .step = 1,
        .def = 0,
    },
	{	.id   = V4L2_CID_GAIN, 
		.min  = 10,
		.max  = 0xffff,
		.step = 1,
		.def  = 0x50,
	},
		
	{	.id   = V4L2_CID_FLASH_STROBE, 
		.min  = 0, 
		.max  = 1, 
		.step = 1, 
		.def  = 0,
	},
	
	{	.id   = V4L2_CID_FLASH_STROBE_STOP, 
		.min  = 0, 
		.max  = 1, 
		.step = 1, 
		.def  = 0,
	},
	
    {	.id = V4L2_CID_MIRRORFLIP, //3.10内核没有定义此命令字,同时写入vflip和hflip
		.min = NONE, 
		.max = HFLIP|VFLIP, 
		.step = 1, 
		.def = NONE,
	},
};

static struct v4l2_ctl_cmd_info_menu v4l2_ctl_array_menu[] =
{
	 {
        .id = V4L2_CID_COLORFX,
        .max = 3,
        .mask = 0x0,
        .def = 0,
    },
    {
        .id = V4L2_CID_EXPOSURE_AUTO,
        .max = 1,
        .mask = 0x0,
        .def = 1,
    },
	{  
	   .id = V4L2_CID_SCENE_MODE, 
	   .max = V4L2_SCENE_MODE_TEXT, 
	   .mask = 0x0, 
	   .def = 0,
	},

#if 1

	{	.id   = V4L2_CID_FLASH_LED_MODE, 
		.max  = 3,
		.mask = 0x0,
		.def  = 0,},
#endif
	{
	.id = V4L2_CID_POWER_LINE_FREQUENCY, 
	.max = V4L2_CID_POWER_LINE_FREQUENCY_AUTO, 
	.mask = 0x0,
	.def = V4L2_CID_POWER_LINE_FREQUENCY_AUTO,},
};


#endif /* __MODULE_DIFF_H__ */
