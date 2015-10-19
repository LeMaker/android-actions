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


#define CAMERA_MODULE_NAME 		"gc0308"
#define CAMERA_MODULE_PID		0x9B
#define VERSION(pid, ver) 		((pid<<8)|(ver&0xFF))

#define MODULE_I2C_REAL_ADDRESS  (0x42>>1)
#define MODULE_I2C_REG_ADDRESS		(0x21)

#define I2C_REGS_WIDTH			1
#define I2C_DATA_WIDTH			1
#define IS_FRONT_OR_REAR        0

#define DEFAULT_VSYNC_ACTIVE_LEVEL		V4L2_MBUS_VSYNC_ACTIVE_HIGH
#define DEFAULT_PCLK_SAMPLE_EDGE      V4L2_MBUS_PCLK_SAMPLE_RISING

#define DEFAULT_POWER_LINE_FREQUENCY	V4L2_CID_POWER_LINE_FREQUENCY_50HZ



#if 1
#define PID						0x00 /* Product ID Number */
#else
#define PIDH					XXX /* Product ID Number H byte */
#define PIDL					XXX /* Product ID Number L byte */
#endif

#define OUTTO_SENSO_CLOCK 		24000000


#define MODULE_DEFAULT_WIDTH	WIDTH_VGA
#define MODULE_DEFAULT_HEIGHT	HEIGHT_VGA
#define MODULE_MAX_WIDTH		WIDTH_VGA
#define MODULE_MAX_HEIGHT		HEIGHT_VGA

#define AHEAD_LINE_NUM			15    //10行 = 50次循环
#define DROP_NUM_CAPTURE		1
#define DROP_NUM_PREVIEW		2
//static unsigned int frame_rate_qqvga[]  = {30,};
static unsigned int frame_rate_qvga[]  = {30,};
static unsigned int frame_rate_vga[]   = {15,};


static const struct regval_list module_init_regs[] =
{
	/* for the setting , 24M Mlck input and 24M Plck output */
    // VGA_YUV 25 fps
    // 24 MHz input clock, 24Mhz PCLK

    {0xfe, 0x80},       // softreset
    {0xfe, 0x00},
    {0x22, 0x55},       // auto DNDD,SA,ABS
    {0x03, 0x01},
    {0x04, 0x2c},       //exp 300
    {0x5a, 0x56},       //awb_r_gain
    {0x5b, 0x40},       //awb_g_gain
    {0x5c, 0x4a},       //awb_b_gain
    {0x22, 0x57},       //add auto AWB
    {0x0f, 0x00},

    {0x01, 0x6a},       //horizontal blanking 106
    {0x02, 0x70},       //vertical blanking 137
    {0x0f, 0x00},       // vb and hb high 4 bits
    {0xe2, 0x00},       //anti-flicker step [11:8]
    {0xe3, 0x96},       // anti-flicker step [7:0]
    {0xe4, 0x03},       //exposure level 1~4
    {0xe5, 0x84},       //25FPS
    {0xe6, 0x04},
    {0xe7, 0xb0},
    {0xe8, 0x07},
    {0xe9, 0x08},
    {0xea, 0x07},
    {0xeb, 0x08},
    //frame rate ctrl
    {0x05, 0x00},       //start row
    {0x06, 0x00},
    {0x07, 0x00},       //start column
    {0x08, 0x02},
    {0x09, 0x01},       // height
    {0x0a, 0xea},
    {0x0b, 0x02},       // width
    {0x0c, 0x88},
    {0x0d, 0x02},       //st
    {0x0e, 0x02},       //et
    {0x10, 0x26},
    {0x11, 0x0d},
    {0x12, 0x2a},       //sample-hold delay time

    {0x13, 0x00},
    {0x14, 0x10},       // h_v
    {0x15, 0x0a},
    {0x16, 0x05},
    {0x17, 0x01},
    {0x18, 0x44},
    {0x19, 0x44},
    {0x1a, 0x2a},
    {0x1b, 0x00},
    {0x1c, 0x49},
    {0x1d, 0x9a},
    {0x1e, 0x61},
    {0x1f, 0x2b/*0x16*/}, // pad drive level
    //isp
    {0x20, 0xff},       //block enable
    {0x21, 0xf8},       //fa
    {0x22, 0x57},       // auto enable
    {0x24, 0xa0},       //UYVY
    {0x25, 0x0f},       // data, pclk, hsync, vsync enable
    {0x26, 0x03},       //hsync and vsync high valid
    {0x2f, 0x01},       //update gain mode
    //blk
    {0x30, 0xf7},
    {0x31, 0x50},
    {0x32, 0x00},
    {0x39, 0x04},
    {0x3a, 0x20},
    {0x3b, 0x20},
    {0x3c, 0x00},       //manual gain
    {0x3d, 0x00},
    {0x3e, 0x00},
    {0x3f, 0x00},
    //pregain
    {0x50, 0x14},       //global gain
    {0x53, 0x80},       //channel gain
    {0x54, 0x87},
    {0x55, 0x87},
    {0x56, 0x80},
    //lsc
    {0x8b, 0x20},
    {0x8c, 0x20},
    {0x8d, 0x20},
    {0x8e, 0x14},
    {0x8f, 0x10},
    {0x90, 0x14},
    {0x91, 0x3c},
    {0x92, 0x50},
    {0x5d, 0x12},
    {0x5e, 0x1a},
    {0x5f, 0x24},
    //dndd
    {0x60, 0x07},
    {0x61, 0x15},
    {0x62, 0x08},
    {0x64, 0x03},
    {0x66, 0xe8},
    {0x67, 0x86},
    {0x68, 0xa2},
    // asde
    {0x69, 0x18},
    {0x6a, 0x0f},
    {0x6b, 0x00},
    {0x6c, 0x5f},
    {0x6d, 0x8f},
    {0x6e, 0x55},
    {0x6f, 0x38},
    {0x70, 0x15},
    {0x71, 0x33},
    //intpee
    {0x72, 0xdc},
    {0x73, 0x80},
    {0x74, 0x02},
    {0x75, 0x3f},
    {0x76, 0x02},
    {0x77, 0x54},
    {0x78, 0x88},
    {0x79, 0x81},
    {0x7a, 0x81},
    {0x7b, 0x22},
    {0x7c, 0xff},
    //cc
    {0x93, 0x4c},
    {0x94, 0x02},
    {0x95, 0x07},
    {0x96, 0xe0},
    {0x97, 0x46},
    {0x98, 0xf3},
    //ycp
    {0xb1, 0x40},       //3
    {0xb2, 0x40},
    {0xb3, 0x40},
    {0xb5, 0x00},
    {0xb6, 0xe0},
    {0xbd, 0x3C},
    {0xbe, 0x36},
    //aec
    {0xd0, 0xCb},
    {0xd1, 0x10},
    {0xd2, 0x90},       // AEC enable
    {0xd3, 0x58},
    {0xd5, 0xF2},
    {0xd6, 0x10},
    {0xdb, 0x92},
    {0xdc, 0xA5},
    {0xdf, 0x23},
    {0xd9, 0x00},
    {0xda, 0x00},
    {0xe0, 0x09},
    {0xed, 0x04},
    {0xee, 0xa0},
    {0xef, 0x40},
    //abb
    {0x80, 0x03},       //ABB enable
    //gamma
    {0x9F, 0x0e},
    {0xA0, 0x1c},
    {0xA1, 0x34},
    {0xA2, 0x48},
    {0xA3, 0x5a},
    {0xA4, 0x6b},
    {0xA5, 0x7b},
    {0xA6, 0x95},
    {0xA7, 0xab},
    {0xA8, 0xbf},
    {0xA9, 0xce},
    {0xAA, 0xd9},
    {0xAB, 0xe4},
    {0xAC, 0xec},
    {0xAD, 0xF7},
    {0xAE, 0xFd},
    {0xAF, 0xFF},
    //ycp-gamma
    {0xc0, 0x00},
    {0xc1, 0x14},
    {0xc2, 0x21},
    {0xc3, 0x36},
    {0xc4, 0x49},
    {0xc5, 0x5B},
    {0xc6, 0x6B},
    {0xc7, 0x7B},
    {0xc8, 0x98},
    {0xc9, 0xB4},
    {0xca, 0xCE},
    {0xcb, 0xE8},
    {0xcc, 0xFF},
    //abs
    {0xf0, 0x02},
    {0xf1, 0x01},
    {0xf2, 0x02},
    {0xf3, 0x30},
    //measure window
    {0xf7, 0x12},
    {0xf8, 0x0a},
    {0xf9, 0x9f},
    {0xfa, 0x78},
    //awb
    {0xfe, 0x01},       //select page1
    {0x00, 0xf5},
    {0x02, 0x20},
    {0x04, 0x10},
    {0x05, 0x08},
    {0x06, 0x20},
    {0x08, 0x0a},
    {0x0a, 0xa0},
    {0x0b, 0x64},
    {0x0c, 0x08},
    {0x0e, 0x44},
    {0x0f, 0x32},
    {0x10, 0x41},
    {0x11, 0x37},
    {0x12, 0x22},       //awb gain adjust speed
    {0x13, 0x19},
    {0x14, 0x44},       //awb_set1
    {0x15, 0x44},
    {0x16, 0xc2},
    {0x17, 0xA8},
    {0x18, 0x18},
    {0x19, 0x50},
    {0x1a, 0xd8},
    {0x1b, 0xf5},
    {0x1c, 0x60},       //r gain limit
    {0x70, 0x40},       //awb_set2
    {0x71, 0x58},
    {0x72, 0x30},
    {0x73, 0x48},
    {0x74, 0x20},
    {0x75, 0x60},
    {0x77, 0x20},
    {0x78, 0x32},
    //hsp
    {0x30, 0x03},
    {0x31, 0x40},
    {0x32, 0x10},
    {0x33, 0xe0},
    {0x34, 0xe0},
    {0x35, 0x00},
    {0x36, 0x80},
    {0x37, 0x00},
    {0x38, 0x04},
    {0x39, 0x09},
    {0x3a, 0x12},
    {0x3b, 0x1C},
    {0x3c, 0x28},
    {0x3d, 0x31},
    {0x3e, 0x44},
    {0x3f, 0x57},
    {0x40, 0x6C},
    {0x41, 0x81},
    {0x42, 0x94},
    {0x43, 0xA7},
    {0x44, 0xB8},
    {0x45, 0xD6},
    {0x46, 0xEE},
    {0x47, 0x0d},

    //-----------Update the registers end---------//

    {0xfe, 0x00},       // back to page0
    ENDMARKER,
};

#if 0
/* 160*120: QVGA */
static const struct regval_list module_qqvga_regs[] = 
{
	// YUV 25 fps
	{0xfe, 0x01},
	{0x54, 0x44},
	{0x55, 0x03},
	{0x56, 0x00},
	{0x57, 0x00},
	{0x58, 0x00},
	{0x59, 0x00},

	{0xfe, 0x00},
	{0x01, 0x6a},
	{0x02, 0x70},
	{0x0f, 0x00},
	{0x05, 0x00},
	{0x06, 0x00},
	{0x07, 0x00},
	{0x08, 0x00},
	{0x09, 0x01},
	{0x0a, 0xe8},
	{0x0b, 0x02},
	{0x0c, 0x88},

	{0x91, 0x3c},		//LSC row center
	{0x92, 0x50},		//LSC colum center
	{0xf7, 0x01},		//big_win_x0,X4
	{0xf8, 0x01},		//big_win_y0
	{0xf9, 0x9c},		//big_win_x1
	{0xfa, 0x76},		//big_win_y1

	{0xfe, 0x01},
	{0x0a, 0xa0},
	{0x0e, 0x44},
	{0x0f, 0x32},
	{0xfe, 0x00},	
	ENDMARKER,
};
#endif

/* 320*240: QVGA */
static const struct regval_list module_qvga_regs[] = 
{
	
	//YUV 25fps
	   {0xfe, 0x01},
	   {0x54, 0x22},
	   {0x55, 0x03},
	   {0x56, 0x00},
	   {0x57, 0x00},
	   {0x58, 0x00},
	   {0x59, 0x00},
	
	   {0xfe, 0x00},
	   {0x01, 0x6a},
	   {0x02, 0x70},
	   {0x0f, 0x00},
	   {0x05, 0x00},
	   {0x06, 0x00},
	   {0x07, 0x00},
	   {0x08, 0x00},
	   {0x09, 0x01},
	   {0x0a, 0xe8},
	   {0x0b, 0x02},
	   {0x0c, 0x88},
	
	   {0x91, 0x3c},	   //LSC row center
	   {0x92, 0x50},	   //LSC colum center
	   {0xf7, 0x01},	   //big_win_x0,X4
	   {0xf8, 0x01},	   //big_win_y0
	   {0xf9, 0x9c},	   //big_win_x1
	   {0xfa, 0x76},	   //big_win_y1
	
	   {0xfe, 0x01},
	   {0x0a, 0xa0},
	   {0x0e, 0x44},
	   {0x0f, 0x32},
	   {0xfe, 0x00},
	   ENDMARKER,
};

/* 640*480: VGA */
static const struct regval_list module_vga_regs[] = 
{

	 //YUV 25fps
    {0xfe, 0x01},
    {0x54, 0x11},
    {0x55, 0x03},
    {0x56, 0x00},
    {0x57, 0x00},
    {0x58, 0x00},
    {0x59, 0x00},

    {0xfe, 0x00},
    {0x01, 0x6a},
    {0x02, 0x70},
    {0x0f, 0x00},
    {0x05, 0x00},
    {0x06, 0x00},
    {0x07, 0x00},
    {0x08, 0x00},
    {0x09, 0x01},
    {0x0a, 0xe8},
    {0x0b, 0x02},
    {0x0c, 0x88},

    {0x91, 0x3c},       //LSC row center
    {0x92, 0x50},       //LSC colum center
    {0xf7, 0x04},       //big_win_x0,X4
    {0xf8, 0x02},       //big_win_y0
    {0xf9, 0x9f},       //big_win_x1
    {0xfa, 0x78},       //big_win_y1

    {0xfe, 0x01},
    {0x0a, 0xa0},
    {0x0e, 0x44},
    {0x0f, 0x32},
    {0xfe, 0x00},
    ENDMARKER,
};

/* 800*600: SVGA */
static const struct regval_list module_svga_regs[] = 
{
//  NULL
};

/* 1280*720: 720P*/
static const struct regval_list module_720p_regs[] = 
{
//  NULL
};

/* 1600*1200: UXGA */
static const struct regval_list module_uxga_regs[] = 
{
//  NULL
};

/* 1920*1080: 1080P*/
static const struct regval_list module_1080p_regs[] = 
{
//  NULL
};

/* 2592X1944 QSXGA */
static const struct regval_list module_qsxga_regs[] = 
{
//  NULL
};

static const struct regval_list module_init_auto_focus[] =
{
//  NULL
};

#if 0
/*
 * window size list
 */
 /* 320*240 */
static struct camera_module_win_size module_win_qqvga = {
	.name             = "QQVGA",
	.width            = WIDTH_QQVGA,
	.height           = HEIGHT_QQVGA,
	.win_regs         = module_qqvga_regs,

	.frame_rate_array = frame_rate_qqvga,
	 .capture_only     = 0,
};
#endif
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
#if 0
/* 800*600 */
static struct camera_module_win_size module_win_svga = {
	.name             = "VGA",
	.width            = WIDTH_SVGA,
	.height           = HEIGHT_SVGA,
	.win_regs         = module_svga_regs,
	.win_regs_size    = ARRAY_SIZE(module_svga_regs),

	.frame_rate_array = frame_rate_svga,
};

/* 1280*720 */
static struct camera_module_win_size module_win_720p = {
	.name             = "720P",
	.width            = WIDTH_720P,
	.height           = HEIGHT_720P,
	.win_regs         = module_720p_regs,
	.win_regs_size    = ARRAY_SIZE(module_720p_regs),
	.frame_rate_array = frame_rate_720p,
};
/* 1600*1200 */
static struct camera_module_win_size module_win_uxga = {
	.name             = "UXGA",
	.width            = WIDTH_UXGA,
	.height           = HEIGHT_UXGA,
	.win_regs         = module_uxga_regs,
	.win_regs_size    = ARRAY_SIZE(module_uxga_regs),
	.frame_rate_array = frame_rate_uxga,
};

/* 1920*1080 */
static struct camera_module_win_size module_win_1080p = {
	.name             = "1080P",
	.width            = WIDTH_1080P,
	.height           = HEIGHT_1080P,
	.win_regs         = module_1080p_regs,
	.win_regs_size    = ARRAY_SIZE(module_1080p_regs),

	.frame_rate_array = frame_rate_1080p,
};

/* 2592*1944 */
static struct camera_module_win_size module_win_qsxga = {
	.name             = "QSXGA",
	.width            = WIDTH_QSXGA,
	.height           = HEIGHT_QSXGA,
	.win_regs         = module_qsxga_regs,
	.win_regs_size    = ARRAY_SIZE(module_qsxga_regs),
	.frame_rate_array = frame_rate_qsxga,
};
#endif
static struct camera_module_win_size *module_win_list[] = {
	&module_win_vga,
	//&module_win_qqvga,
	&module_win_qvga, 
};

static struct regval_list module_whitebance_auto_regs[]=
{
	{0x22, 0x57}, 
	{0x5a, 0x56},
	{0x5b, 0x40},
	{0x5c, 0x4a},
	ENDMARKER,
};

/* Cloudy Colour Temperature : 6500K - 8000K  */
static struct regval_list module_whitebance_cloudy_regs[]=
{
	{0x22, 0x55},
	{0x5a, 0x78},
	{0x5b, 0x44},
	{0x5c, 0x40},
	ENDMARKER,
};

/* ClearDay Colour Temperature : 5000K - 6500K  */
static struct regval_list module_whitebance_sunny_regs[]=
{
	{0x22, 0x55},
    {0x5a, 0x68},
    {0x5b, 0x44},
    {0x5c, 0x40},
    ENDMARKER,
};

/* Office Colour Temperature : 3500K - 5000K ,荧光灯 */
static struct regval_list module_whitebance_fluorescent_regs[]=
{
	{0x22, 0x55},
    {0x5a, 0x48},
    {0x5b, 0x40},
    {0x5c, 0x58},
    ENDMARKER,
};

/* Home Colour Temperature : 2500K - 3500K ，白炽灯 */
static struct regval_list module_whitebance_incandescent_regs[]=
{
	{0x22, 0x55},
    {0x5a, 0x48},
    {0x5b, 0x40},
    {0x5c, 0x68},
	ENDMARKER,
};


static struct regval_list module_scene_auto_regs[] =
{
	{0xec, 0x20},
	ENDMARKER,
};



static struct regval_list module_scene_night_regs[] =
{
	{0xec, 0x30},
	ENDMARKER,

};


/*
 * The exposure target setttings
 */
static struct regval_list module_exp_comp_neg4_regs[] = {
	
	{0xb5,0xd0},	
	{0xd3,0x38}, 		
	ENDMARKER,
};

static struct regval_list module_exp_comp_neg3_regs[] = {
	
	{0xb5,0xe0}, 
	{0xd3,0x40},	
	ENDMARKER,
};

static struct regval_list module_exp_comp_neg2_regs[] = {
	
	{0xb5,0xf0},   
	{0xd3,0x48},	
	ENDMARKER,
};

static struct regval_list module_exp_comp_neg1_regs[] = {
	
	{0xb5,0x00},  
	{0xd3,0x50},	
	ENDMARKER,
};

static struct regval_list module_exp_comp_zero_regs[] = {
	
	{0xb5,0x00},  
	{0xd3,0x58},	
	ENDMARKER,
};

static struct regval_list module_exp_comp_pos1_regs[] = {
	
	{0xb5,0x30},  
	{0xd3,0x60},	
	ENDMARKER,
};

static struct regval_list module_exp_comp_pos2_regs[] = {
	
	{0xb5,0x40},
	{0xd3,0x68},	
	ENDMARKER,
};

static struct regval_list module_exp_comp_pos3_regs[] = {
   
   {0xb5,0x50},  
   {0xd3,0x70},	    

	ENDMARKER,
};

static struct regval_list module_exp_comp_pos4_regs[] = {
	
	{0xb5,0x60},  
	{0xd3,0x78},	
	ENDMARKER,
};

static struct v4l2_ctl_cmd_info v4l2_ctl_array[] =
{
	{   .id = V4L2_CID_EXPOSURE, 
		.min = 0, 
		.max = 975,
		.step = 1, 
		.def = 500,
	},
	{	.id = V4L2_CID_EXPOSURE_COMP, 
		.min = -4, 
		.max = 4, 
		.step = 1, 
		.def = 0,
	},	
	{	.id = V4L2_CID_GAIN, 
		.min = 10, 
		.max = 2048, 
		.step = 1, 
		.def = 30,
	},
	{	.id   = V4L2_CID_FLASH_STROBE, 
		.min  = 0, 
		.max  = 1, 
		.step = 1, 
		.def  = 0,},
	{	.id   = V4L2_CID_FLASH_STROBE_STOP, 
		.min  = 0, 
		.max  = 1, 
		.step = 1, 
		.def  = 0,},
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
       {	.id = V4L2_CID_AF_MODE,
		.min = NONE_AF, 
		.max = CONTINUE_AF|SINGLE_AF, 
		.step = 1, 
		.def = NONE_AF,},
    {	.id = V4L2_CID_AF_STATUS, 
		.min = AF_STATUS_DISABLE, 
		.max = AF_STATUS_FAIL, 
		.step = 1, 
		.def = AF_STATUS_DISABLE,},
	{	.id = V4L2_CID_MIRRORFLIP, //3.10内核没有定义此命令字,同时写入vflip和hflip
		.min = NONE, 
		.max = HFLIP|VFLIP, 
		.step = 1, 
		.def = NONE,},
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
	{	
		.id   = V4L2_CID_FLASH_LED_MODE, 
		.max  = 3,
		.mask = 0x0,
		.def  = 0,
	},
	{
	.id = V4L2_CID_POWER_LINE_FREQUENCY, 
	.max = V4L2_CID_POWER_LINE_FREQUENCY_AUTO, 
	.mask = 0x0,
	.def = V4L2_CID_POWER_LINE_FREQUENCY_AUTO,},
};


#endif /* __MODULE_DIFF_H__ */
