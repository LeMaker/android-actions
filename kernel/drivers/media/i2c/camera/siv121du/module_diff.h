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

#define CAMERA_MODULE_NAME 		"camera_siv121du"
#define CAMERA_MODULE_PID		0xDE

#define MODULE_PLATFORM_ID		SIV121DU_PLATFORM_ID
#define MODULE_I2C_REAL_ADDRESS	(0x66>>1)
#define MODULE_I2C_REG_ADDRESS	(0x6d>>1)
#define I2C_REGS_WIDTH			1
#define I2C_DATA_WIDTH			1

#if 1
#define PID						0x01 /* Product ID Number */
#else
#define PIDH					XXX /* Product ID Number H byte */
#define PIDL					XXX /* Product ID Number L byte */
#endif

#define OUTTO_SENSO_CLOCK 		24000000

#define DEFAULT_VSYNC_ACTIVE_LEVEL		V4L2_MBUS_VSYNC_ACTIVE_LOW
#define DEFAULT_PCLK_SAMPLE_EDGE      V4L2_MBUS_PCLK_SAMPLE_RISING
#define DEFAULT_POWER_LINE_FREQUENCY	V4L2_CID_POWER_LINE_FREQUENCY_50HZ

#define MODULE_DEFAULT_WIDTH	WIDTH_VGA
#define MODULE_DEFAULT_HEIGHT	HEIGHT_VGA
#define MODULE_MAX_WIDTH		WIDTH_VGA
#define MODULE_MAX_HEIGHT		HEIGHT_VGA

#define AHEAD_LINE_NUM			15    //10行 = 50次循环
#define DROP_NUM_CAPTURE		4
#define DROP_NUM_PREVIEW		4

static unsigned int frame_rate_qvga[]  = {30,};
static unsigned int frame_rate_vga[]   = {30,};

static const struct regval_list module_init_regs[] =
{
     {0x00,0x01},  
	{0x03,0x08}, 
	{0x00,0x01},  
	{0x03,0x08}, 
	
	{0x00,0x00},  
	{0x03,0x04},                                           
	{0x10,0x86},//8d}, //85  
	{0x11,0x71},// f1},	  //0x11  0x61 davis20120514
	
	{0x00,0x01},                             
    {0x04,0x00},//0x04}, 
	{0x06,0x04},                                           
	{0x10,0x11},//46},                                           
	{0x11,0x25},//23},                                           
	{0x12,0x21},                                           
	{0x17,0x94},//86},                                           
	{0x18,0x00},                                           
	{0x20,0x00},                                           
	{0x21,0x05},                                           
  //  {0x22, 0x01},
	{0x23,0x69}, //add 
	{0x40,0x0F},                                        
	{0x41,0x90},                                        
	{0x42,0xd2},                                           
	{0x43,0x00}, 

	// AE
	{0x00,0x02},                                          
	{0x10,0x84},                                           
    {0x11,0x0a},//0d}, min frame rate 最低帧率
    {0x12,0x72},//75},//78},//0x80}, //0x64
    {0x14,0x70},//75},//78},//0x80},
    {0x34,0x96},
    {0x40,0x38},//35},//38},//40}, //0x48
  
    {0x44,0x08},
  

	
	// AWB 
	{0x00,0x03},                                          
    {0x10, 0xd0},
	{0x11,0xc1},//c0},//c0  
    {0x13,0x82},//81},//0x82}, //Cr target
	{0x14,0x7f},//80}, //Cb target  7f                                                                 
	{0x15,0xe0},//c0}, // R gain Top   e0                                                             
    {0x16, 0x7c}, // R gain bottom 
	{0x17,0xe0}, // B gain Top   e0                                                             
	{0x18,0x80},//70}, // B gain bottom 0x80     //                                                  
    {0x19, 0x8c}, // Cr top value 0x90
	{0x1a,0x64},	// Cr bottom value 0x64   60        //YCbYCr                                 
	{0x1b,0x94}, // Cb top value 0x98                                                        
	{0x1c,0x6c}, // Cb bottom value 0x6c           // 0x01        
	{0x1d,0x94}, // 0xa0                                 
	{0x1e,0x6c}, // 0x60                                                
	{0x20,0xe8}, // AWB luminous top value                                                   
	{0x21,0x30}, // AWB luminous bottom value 0x20                                           
	{0x22,0xb8},                                           
	{0x23,0x10},   
	{0x25,0x08},//20},    
	{0x26,0x0f},//0f},    
	{0x27,0x60},    // BRTSRT                                            
	{0x28,0x70},    // BRTEND                                           
	{0x29,0xb7},    // BRTRGNBOT                                        
	{0x2a,0xa3},    // BRTBGNTOP 
	                                         
	{0x40,0x01},                                           
	{0x41,0x03},//04},                                           
	{0x42,0x08},                                           
	{0x43,0x10},                                           
	{0x44,0x13},//12}, // 0x12  
	{0x45,0x6a},//35},  //35                                 
	{0x46,0xca},  //  fc   
	
    {0x62,0x80},//0x78},//80},
    {0x63, 0x90}, // R D30 to D20
    {0x64, 0xd0}, // B D30 to D20
	{0x65,0x98},//a0},//90},  // R D20 to D30                                          
	{0x66,0xd0},  // B D20 to D30  

	// IDP 
	{0x00,0x04},                                          
	{0x10,0x7f},//ff},                                           
	{0x11,0x1d}, // changed
    {0x12, 0x3d},
	{0x13,0xfe},//++ 
	{0x14,0x01},//++ 
	
	
		// DPCBNR                                            
	{0x18,0xbf},//fe},  // DPCNRCTRL                                               
	{0x19,0x00},//04},  // DPCTHV                                                  
	{0x1A,0x00},//01},  // DPCTHVSLP                                               
	{0x1B,0x00},//08},//08},  // DPCTHVDIFSRT                                                                                                                                                                                                                                                                                                              
	//{0x1C,0x0f},//08},  // DPCTHVDIFSLP                                            
	//{0x1d,0xFF},  // DPCTHVMAX      
	                                                             
	{0x1E,0x04},  // BNRTHV  0c                                              
	{0x1F,0x08},//04},  // BNRTHVSLPN 10                                           
	{0x20,0x20},  // BNRTHVSLPD                                              
	{0x21,0x00}, /// BNRNEICNT      / 0x08                                   
	{0x22,0x08},  // STRTNOR        // 0x03                                  
	{0x23,0x38},//40},  // STRTDRK        
	{0x24,0x00},

	// Gamma 
    {0x31, 0x03}, //0x08
    {0x32, 0x0b}, //0x10
    {0x33, 0x1e}, //0x1B
    {0x34, 0x46}, //0x37
    {0x35, 0x62}, //0x4D
    {0x36, 0x78}, //0x60
    {0x37, 0x8b}, //0x72
    {0x38, 0x9b}, //0x82
    {0x39, 0xa8}, //0x91
    {0x3a, 0xb6}, //0xA0
    {0x3b, 0xcc}, //0xBA
    {0x3c, 0xdf}, //0xD3
    {0x3d, 0xf0}, //0xEA
    
    // Shading Register Setting 				 
	{0x40,0x55},//04},  //06                                         
	{0x41,0x44},                                           
	{0x42,0x33},//43},                                           
	{0x43,0x00},                                   
	{0x44,0x22},  // left R gain[7:4], right R gain[3:0] 	 22					                                          
	{0x45,0x22},  // top R gain[7:4], bottom R gain[3:0] 		22				                                          
	{0x46,0x00},  // left G gain[7:4], right G gain[3:0] 	                                                    
	{0x47,0x03},  // top G gain[7:4], bottom G gain[3:0] 	   11                                                   
	{0x48,0x01},  // left B gain[7:4], right B gain[3:0] 	                                                   
	{0x49,0x11},  // top B gain[7:4], bottom B gain[3:0] 	           //color matrix default                                          
	{0x4a,0x05},  // X-axis center high[3:2], Y-axis center high[1:0]                                         
	{0x4b,0x40},  // X-axis center low[7:0]				48		                                                        
	{0x4c,0x10},  // Y-axis center low[7:0]		 e8			                                                          
	{0x4d,0x80},  // Shading Center Gain     80                                                                  
	{0x4e,0x00},  // Shading R Offset                                                                   
	{0x4f,0x00},  // Shading Gr Offset                                                                        
	{0x50,0x00},  // Shading B Offset                                

	
	// Interpolation                                           
	{0x60,0x7f},                                     
	{0x61,0x08},  
	 
	 // Color matrix (D65) - Daylight    
    {0x71, 0x39},//34},	 
    {0x72, 0xc8},//CE},	 
    {0x73, 0xff},	 
	{0x74,0x13},         //0x13       0x10                           
	{0x75,0x25},         //0x25       0x21                           
	{0x76,0x08},         //0x08       0x0f                           
	{0x77,0xf8},		 //0xec 	  0xf8							 
	{0x78,0xc0},		 //0xd1 	  0xc0							 
	{0x79,0x48},		 //0x47 	  0x48
	
	// Color matrix (D20) - A                                           
    {0x83, 0x38}, //0x3c 	
    {0x84, 0xd1}, //0xc6 	
    {0x85, 0xf7}, //0xff   
	{0x86,0x12},     //0x12                                         
	{0x87,0x25},     //0x24 	                                  
	{0x88,0x09},     //0x0a 	                                      
    {0x89, 0xed}, //0xed   
    {0x8a, 0xbb}, //0xc2 	
    {0x8b, 0x58}, //0x51
    
	{0x8c,0x10},     //CMA select
	
	//G Edge                                        
	{0x90,0x35},    //Upper gain                                                                 
	{0x91,0x48},    //down gain                                                                  
	{0x92,0x33},//22},    //[7:4] upper coring [3:0] down coring                                       
	{0x9a,0x40},                                           
	{0x9b,0x40},                                           
	{0x9c,0x38},    //edge suppress start   30                                      
	{0x9d,0x30},    //edge suppress slope   
	{0x9f,0x26},
	{0xa0,0x11},
	
	{0xa8,0x12},//0x11},//10},                                     
    {0xa9,0x12},//cr saturation-----------
    {0xaa,0x12},//cb,-----------
		 
    {0xab, 0x00},//0x08},//brightness-----------
		 
    {0xb9, 0x28}, // nightmode 38 at gain 0x48 5fps
    {0xba, 0x41}, // nightmode 80 at gain 0x48 5fps

    {0xbf, 0x20},
    {0xc0, 0x24},	 
    {0xc1, 0x00},	 
   {0xc2, 0x80},	
   {0xc3, 0x00},	 
   {0xc4, 0xe0},	 
    
	{0xde,0x80},  
	                                         
	{0xe5,0x15},                                           
	{0xe6,0x02},                                           
	{0xe7,0x04}, 
	                                                                                             
	//Sensor On //Sensor On                                                             
    //Sensor On  
	{0x00,0x01},                                           
	{0x03,0x01},  
       
     {0x00,0x00},
     ENDMARKER,
};

/* 320*240: QVGA */
static const struct regval_list module_qvga_regs[] = 
{
 	{0x00,	0x01}, 
	{0x06,	0x06},

	{0x00,	0x04}, 
	//{0x12,	0x3d},

	{0xc0,	0x10},
	{0xc1,	0x00},
	{0xc2,	0x40},
	{0xc3,	0x00},
	{0xc4,	0xf0},
     ENDMARKER,
};

/* 640*480: VGA */
static const struct regval_list module_vga_regs[] = 
{
//  NULL
	{0x00,  0x01}, 
    {0x06,	0x04},

    {0x00,  0x04}, 
   // {0x12,	0x3d},

    {0xc0,	0x24},
    {0xc1,	0x00},
    {0xc2,	0x80},
    {0xc3,	0x00},
    {0xc4,  0xe0},
     ENDMARKER,

};

static const struct regval_list module_init_auto_focus[] = {
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


static struct camera_module_win_size *module_win_list[] = {
	&module_win_qvga,
	&module_win_vga,
};

static struct regval_list module_whitebance_auto_regs[]=
{
	{0x00,	0x03}, 
	{0x10,	0xd0}, 
     ENDMARKER,

};

/* Cloudy Colour Temperature : 6500K - 8000K  */
static struct regval_list module_whitebance_cloudy_regs[]=
{
	{0x00, 0x03}, 
	{0x10, 0x00},  // disable AWB
	{0x60, 0xb4}, 
	{0x61, 0x74}, 
     ENDMARKER,

};

/* ClearDay Colour Temperature : 5000K - 6500K  */
static struct regval_list module_whitebance_sunny_regs[]=
{
		{0x00, 0x03}, 
	   {0x10, 0x00},   // disable AWB
	   {0x60, 0xd8}, 
	   {0x61, 0x90},
     ENDMARKER,

};

/* Office Colour Temperature : 3500K - 5000K ,荧光灯 */
static struct regval_list module_whitebance_fluorescent_regs[]=
{
		{0x00, 0x03}, 
		{0x10, 0x00},  // disable AWB
		{0x60, 0x80},
		{0x61, 0xe0},
     ENDMARKER,

};

/* Home Colour Temperature : 2500K - 3500K ，白炽灯 */
static struct regval_list module_whitebance_incandescent_regs[]=
{
	{0x00, 0x03}, 
    {0x10, 0x00},  // disable AWB
    {0x60, 0xb8},
    {0x61, 0xcc},
     ENDMARKER,
};
#if 0

/*正常模式*/
static struct regval_list module_effect_normal_regs[] =
{
	{0x00, 0x04}, 
	{0xd6, 0x00}, 
     ENDMARKER, 
};

/*单色，黑白照片*/
static struct regval_list module_effect_white_black_regs[] =
{
	{0x00, 0x04}, 
	{0xd6, 0x40}, 
     ENDMARKER,

};

/*负片效果*/
static struct regval_list module_effect_negative_regs[] =
{
	{0x00, 0x04}, 
	{0xd6, 0x20}, 
     ENDMARKER,

};
/*复古效果*/
static struct regval_list module_effect_antique_regs[] =
{    
	{0x00, 0x04}, 
	{0xd6, 0x20}, 
     ENDMARKER,

};
#endif

static struct v4l2_ctl_cmd_info v4l2_ctl_array[] =
{
	{	.id   = V4L2_CID_AUTO_WHITE_BALANCE, 
		.min  = 0, 
		.max  = 1, 
		.step = 1, 
		.def  = 1,},
	{	.id   = V4L2_CID_WHITE_BALANCE_TEMPERATURE, 
		.min  = 0, 
		.max  = 3, 
		.step = 1, 
		.def  = 1,},
#if 0

	{	.id   = V4L2_CID_SCENE_EXPOSURE, 
		.min  = 0, 
		.max  = 1, 
		.step = 1, 
		.def  = 0,},//3.4内核没有定义此命令字
	{	.id   = V4L2_CID_PRIVATE_PREV_CAPT, 
		.min  = 0, 
		.max  = 1, 
		.step = 1, 
		.def  = ACTS_ISP_PREVIEW_MODE,},//3.4内核没有定义此命令字
#endif
#if 1
	{	.id   = V4L2_CID_GAIN, 
		.min  = 10,
		.max  = 0xffff,
		.step = 1,
		.def  = 0x50,},
		
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
	{	.id = V4L2_CID_MIRRORFLIP, //3.10内核没有定义此命令字,同时写入vflip和hflip
		.min = NONE, 
		.max = HFLIP|VFLIP, 
		.step = 1, 
		.def = NONE,},

#endif
	

#if 0
	{   .id   = V4L2_CID_EXPOSURE, 
		.min  = 0, 
		.max  = 0,
		.step = 0, 
		.def  = 0,},
	{	.id   = V4L2_CID_EXPOSURE_COMP, 
		.min  = -4, 
		.max  = 4, 
		.step = 1, 
		.def  = 0,},

    {	.id   = V4L2_CID_AF_MODE,
		.min  = NONE_AF, 
		.max  = CONTINUE_AF|SINGLE_AF, 
		.step = 1, 
		.def  = CONTINUE_AF|SINGLE_AF,},
	{	.id   = V4L2_CID_AF_STATUS, 
		.min  = AF_STATUS_DISABLE, 
		.max  = AF_STATUS_FAIL, 
		.step = 1, 
		.def  = AF_STATUS_DISABLE,},
#endif
};

static struct v4l2_ctl_cmd_info_menu v4l2_ctl_array_menu[] =
{
	{	.id   = V4L2_CID_COLORFX, 
		.max  = 3, 
		.mask = 0x0, 
		.def  = 0,},
	{	.id   = V4L2_CID_EXPOSURE_AUTO, 
		.max  = 1, 
		.mask = 0x0, 
		.def  = 1,},
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
		.def = V4L2_CID_POWER_LINE_FREQUENCY_AUTO,
	},
};


#endif /* __MODULE_DIFF_H__ */
