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


#define CAMERA_MODULE_NAME 		"soc5140"
#define CAMERA_MODULE_PID		0x2880
#define VERSION(pid, ver) 		((pid<<8)|(ver&0xFF))

#define MODULE_I2C_REAL_ADDRESS  (0x78 >> 1)
#define MODULE_I2C_REG_ADDRESS		(0x78 >> 1)

#define I2C_REGS_WIDTH			2
#define I2C_DATA_WIDTH			2

#define REGLIST_8BIT_START_FLAG  {0xFdFF,0x0000}
#define REGLIST_8BIT_STOP_FLAG	 {0xFdFF,0xFFFF}


#define DELAY_nMS_ARRAY(n)	 	 {0xFF,n}


#define DEFAULT_VSYNC_ACTIVE_LEVEL		V4L2_MBUS_VSYNC_ACTIVE_HIGH
#define DEFAULT_POWER_LINE_FREQUENCY	V4L2_CID_POWER_LINE_FREQUENCY_50HZ
#define DEFAULT_PCLK_SAMPLE_EDGE      V4L2_MBUS_PCLK_SAMPLE_RISING
//V4L2_MBUS_PCLK_SAMPLE_RISING




#define PID						0x0000 /* Product ID Number */


#define OUTTO_SENSO_CLOCK 		24000000


#define MODULE_DEFAULT_WIDTH	WIDTH_VGA
#define MODULE_DEFAULT_HEIGHT	HEIGHT_VGA                                                                           
#define MODULE_MAX_WIDTH		WIDTH_QSXGA
#define MODULE_MAX_HEIGHT		HEIGHT_QSXGA

#define AHEAD_LINE_NUM			15    //10行 = 50次循环
#define DROP_NUM_CAPTURE		3
#define DROP_NUM_PREVIEW		3
static unsigned int frame_rate_vga[]   = {30,};
static unsigned int frame_rate_720p[]  = {30,};
static unsigned int frame_rate_1080p[] = {20,};


static unsigned int frame_rate_svga[] = {30,};

static unsigned int frame_rate_qsxga[] = {15,};



static const struct regval_list module_init_regs[] =
{ 
	
	{0x001A, 0x0019}, //SOFT RESET
     {0x001A, 0x0018},
	//MCLK=24MHz, preview: 640 x 480@30fps, capture YUV 2592 X 1944@5fps
	//Load sections in this order even if making changes
	//LOAD = Step2-PLL_Timing		//PLL, Timing & Flicker 
	// [PLL_settings]
	{0x0010, 0x0339},	//PLL Dividers = 832
	{0x0012, 0x0080},	//PLL P Dividers = 112
	{0x0014, 0x2025},	//PLL Control: TEST_BYPASS off = 8229
	{0x001E, 0x0663},	//Pad Slew Pad Config = 1911
	{0x0022, 0x0048},	//VDD_DIS counter delay
	{0x002A, 0x7F7F},	//PLL P Dividers 4-5-6 = 32636
	{0x002C, 0x0000},	//PLL P Dividers 7 = 0
	{0x002E, 0x0000},	//Sensor Clock Divider = 0
	{0x0018, 0x4008},	//Standby Control and Status: Out of standby
	DELAY_nMS_ARRAY(10),
	{0x3CAA, 0x0F0F},	//Disable Double Samplings

	{0x098E, 0x1000},
	{0xC86C, 0x0518},	//Output Width (A) = 1304
	{0xC86E, 0x03D4},	//Output Height (A) = 980
	{0xC83A, 0x000C},	//Row Start (A) = 12
	{0xC83C, 0x0018},	//Column Start (A) = 24
	
	DELAY_nMS_ARRAY(10),
	
	{0xC83E, 0x07B1},	//Row End (A) = 1969
	{0xC840, 0x0A45},	//Column End (A) = 2629
	{0xC842, 0x0001},	//Row Speed (A) = 1
	{0xC844, 0x0103},	//Core Skip X (A) = 259
	{0xC846, 0x0103},	//Core Skip Y (A) = 259
	{0xC848, 0x0103},	//Pipe Skip X (A) = 259
	{0xC84A, 0x0103},	//Pipe Skip Y (A) = 259
	{0xC84C, 0x00F6},	//Power Mode (A) = 246
	{0xC84E, 0x0001},	//Bin Mode (A) = 1

	REGLIST_8BIT_START_FLAG,
	{0xC850, 0x00},		//Orientation (A) = 0    modify
	{0xC851, 0x00},		//Pixel Order (A) = 0
	REGLIST_8BIT_STOP_FLAG,

	{0xC852, 0x019C},	//Fine Correction (A) = 412
	{0xC854, 0x0732},	//Fine IT Min (A) = 1842
	{0xC856, 0x048E},	//Fine IT Max Margin (A) = 1166
	{0xC858, 0x0002},	//Coarse IT Min (A) = 2
	{0xC85A, 0x0001},	//Coarse IT Max Margin (A) = 1
	{0xC85C, 0x0423},	//Min Frame Lines (A) = 1059
	{0xC85E, 0xFFFF},	//Max Frame Lines (A) = 65535
	{0xC860, 0x0423},	//Base Frame Lines (A) = 1059
	{0xC862, 0x0DBB},	//Min Line Length (A) = 3719
	{0xC864, 0xFFFE},	//Max Line Length (A) = 65534
	{0xC866, 0x7F7E},	//P456 Divider (A) = 32636
	{0xC868, 0x0423},	//Frame Lines (A) = 1059
	{0xC86A, 0x0DBB},	//Line Length (A) = 3719
	{0xC870, 0x0014},	//RX FIFO Watermark (A) = 20
	{0xC8AA, 0x0320}, 	// CAM_OUTPUT_0_IMAGE_WIDTH
	{0xC8AC, 0x0258}, 	// CAM_OUTPUT_0_IMAGE_HEIGHT
	{0xC8AE, 0x0001},	//Output_0 Image Format = 1
	{0xC8B0, 0x0000},	//Output_0 Format Order = 0
	{0xC8B8, 0x0004},	//Output_0 JPEG control = 4
	{0xC8A4, 0x0A28},	//Output Width (B) = 2600
	{0xC8A6, 0x07A0},	//Output Height (B) = 1952
	{0xC872, 0x0010},	//Row Start (B) = 16
	{0xC874, 0x001C},	//Column Start (B) = 28
	{0xC876, 0x07AF},	//Row End (B) = 1967
	{0xC878, 0x0A43},	//Column End (B) = 2627
	{0xC87A, 0x0001},	//Row Speed (B) = 1
	{0xC87C, 0x0101},	//Core Skip X (B) = 257
	{0xC87E, 0x0101},	//Core Skip Y (B) = 257
	{0xC880, 0x0101},	//Pipe Skip X (B) = 257
	{0xC882, 0x0101},	//Pipe Skip Y (B) = 257
	{0xC884, 0x00F2},	//Power Mode (B) = 242
	{0xC886, 0x0000},	//Bin Mode (B) = 0

	REGLIST_8BIT_START_FLAG,
	{0xC888, 0x00},	    //Orientation (B) = 0
	{0xC889, 0x00},	    //Pixel Order (B) = 0
	REGLIST_8BIT_STOP_FLAG,
	
	{0xC88A, 0x009C},	//Fine Correction (B) = 156
	{0xC88C, 0x034A},	//Fine IT Min (B) = 842
	{0xC88E, 0x02A6},	//Fine IT Max Margin (B) = 678
	{0xC890, 0x0002},	//Coarse IT Min (B) = 2
	{0xC892, 0x0001},	//Coarse IT Max Margin (B) = 1
	{0xC894, 0x07EF},	//Min Frame Lines (B) = 2031
	{0xC896, 0xFFFF},	//Max Frame Lines (B) = 65535
	{0xC898, 0x07EF},	//Base Frame Lines (B) = 2031
	{0xC89A, 0x1B31},	//Min Line Length (B) = 7752
	{0xC89C, 0xFFFE},	//Max Line Length (B) = 65534
	{0xC89E, 0x7F7F},	//P456 Divider (B) = 32636
	{0xC8A0, 0x07EF},	//Frame Lines (B) = 2031
	{0xC8A2, 0x1B31},	//Line Length (B) = 7752
	{0xC8A8, 0x0014},	//RX FIFO Watermark (B) = 20
	{0xC8C0, 0x0A20},	//Output_1 Image Width = 2592
	{0xC8C2, 0x0798},	//Output_1 Image Height = 1944
	{0xC8C4, 0x0001},	//Output_1 Image Format = 1
	{0xC8C6, 0x0000},	//Output_1 Format Order = 0
	{0xC8CE, 0x0004},	//Output_1 JPEG control = 4
	#if 0
	{0xA010, 0x00DF},	//fd_min_expected50hz_flicker_period = 308
	{0xA012, 0x00FD},	//fd_max_expected50hz_flicker_period = 328
	{0xA014, 0x00B6},	//fd_min_expected60hz_flicker_period = 255
	{0xA016, 0x00D4},	//fd_max_expected60hz_flicker_period = 275
	{0xA018, 0x0104},	//fd_expected50hz_flicker_period (A) = 318
	{0xA01A, 0x007B},	//fd_expected50hz_flicker_period (B) = 152
	{0xA01C, 0x00D7},	//fd_expected60hz_flicker_period (A) = 265
	{0xA01E, 0x0066},	//fd_expected60hz_flicker_period (B) = 127
	#endif
	
	{0xA010,0x0134},	//fd_min_expected50hz_flicker_period = 303	 
	{0xA012,0x0148},	//fd_max_expected50hz_flicker_period = 323	 
	{0xA014,0x00FF},	//fd_min_expected60hz_flicker_period = 251	 
	{0xA016,0x0113},	//fd_max_expected60hz_flicker_period = 271	 
	{0xA018,0x013E},	//fd_expected50hz_flicker_period (A) = 313	 
	{0xA01A,0x0066},	//fd_expected50hz_flicker_period (B) = 77	 
	{0xA01C,0x0109},	//fd_expected60hz_flicker_period (A) = 261	 
	{0xA01E,0x0055},	//fd_expected60hz_flicker_period (B) = 64	 
	
	REGLIST_8BIT_START_FLAG,
	{0xDC0A, 0x06},  	//Scaler Allow Zoom Ratio = 6
	
	REGLIST_8BIT_STOP_FLAG,
	
	{0xDC1C, 0x2710},	//System Zoom Ratio = 10000
    //{0xE004, 0x1E00},	//I2C Master Clock Divider = 7680
	//LOAD = Step3-Recommended		//Patch & Char settings
	//k28a_rev3_FW_patch

	{0x0982, 0x0000}, 	// ACCESS_CTL_STAT

	{0x098A, 0x0000}, 	// PHYSICAL_ADDRESS_ACCESS
	{0x886C, 0xC0F1},
	{0x886E, 0xC5E1},
	{0x8870, 0x246A},
	{0x8872, 0x1280},
	{0x8874, 0xC4E1},
	{0x8876, 0xD20F},
	{0x8878, 0x2069},
	{0x887A, 0x0000},
	{0x887C, 0x6A62},
	{0x887E, 0x1303},
	{0x8880, 0x0084},
	{0x8882, 0x1734},
	{0x8884, 0x7005},
	{0x8886, 0xD801},
	{0x8888, 0x8A41},
	{0x888A, 0xD900},
	{0x888C, 0x0D5A},
	{0x888E, 0x0664},
	{0x8890, 0x8B61},
	{0x8892, 0xE80B},
	{0x8894, 0x000D},
	{0x8896, 0x0020},
	{0x8898, 0xD508},
	{0x889A, 0x1504},
	{0x889C, 0x1400},
	{0x889E, 0x7840},
	{0x88A0, 0xD007},
	{0x88A2, 0x0DFB},
	{0x88A4, 0x9004},
	{0x88A6, 0xC4C1},
	{0x88A8, 0x2029},
	{0x88AA, 0x0300},
	{0x88AC, 0x0219},
	{0x88AE, 0x06C4},
	{0x88B0, 0xFF80},
	{0x88B2, 0x08C4},
	{0x88B4, 0xFF80},
	{0x88B6, 0x086C},
	{0x88B8, 0xFF80},
	{0x88BA, 0x08C0},
	{0x88BC, 0xFF80},
	{0x88BE, 0x08C4},
	{0x88C0, 0xFF80},
	{0x88C2, 0x097C},
	{0x88C4, 0x0001},
	{0x88C6, 0x0005},
	{0x88C8, 0x0000},
	{0x88CA, 0x0000},
	{0x88CC, 0xC0F1},
	{0x88CE, 0x0976},
	{0x88D0, 0x06C4},
	{0x88D2, 0xD639},
	{0x88D4, 0x7708},
	{0x88D6, 0x8E01},
	{0x88D8, 0x1604},
	{0x88DA, 0x1091},
	{0x88DC, 0x2046},
	{0x88DE, 0x00C1},
	{0x88E0, 0x202F},
	{0x88E2, 0x2047},
	{0x88E4, 0xAE21},
	{0x88E6, 0x0F8F},
	{0x88E8, 0x1440},
	{0x88EA, 0x8EAA},
	{0x88EC, 0x8E0B},
	{0x88EE, 0x224A},
	{0x88F0, 0x2040},
	{0x88F2, 0x8E2D},
	{0x88F4, 0xBD08},
	{0x88F6, 0x7D05},
	{0x88F8, 0x8E0C},
	{0x88FA, 0xB808},
	{0x88FC, 0x7825},
	{0x88FE, 0x7510},
	{0x8900, 0x22C2},
	{0x8902, 0x248C},
	{0x8904, 0x081D},
	{0x8906, 0x0363},
	{0x8908, 0xD9FF},
	{0x890A, 0x2502},
	{0x890C, 0x1002},
	{0x890E, 0x2A05},
	{0x8910, 0x03FE},
	{0x8912, 0x0A16},
	{0x8914, 0x06E4},
	{0x8916, 0x702F},
	{0x8918, 0x7810},
	{0x891A, 0x7D02},
	{0x891C, 0x7DB0},
	{0x891E, 0xF00B},
	{0x8920, 0x78A2},
	{0x8922, 0x2805},
	{0x8924, 0x03FE},
	{0x8926, 0x0A02},
	{0x8928, 0x06E4},
	{0x892A, 0x702F},
	{0x892C, 0x7810},
	{0x892E, 0x651D},
	{0x8930, 0x7DB0},
	{0x8932, 0x7DAF},
	{0x8934, 0x8E08},
	{0x8936, 0xBD06},
	{0x8938, 0xD120},
	{0x893A, 0xB8C3},
	{0x893C, 0x78A5},
	{0x893E, 0xB88F},
	{0x8940, 0x1908},
	{0x8942, 0x0024},
	{0x8944, 0x2841},
	{0x8946, 0x0201},
	{0x8948, 0x1E26},
	{0x894A, 0x1042},
	{0x894C, 0x0F15},
	{0x894E, 0x1463},
	{0x8950, 0x1E27},
	{0x8952, 0x1002},
	{0x8954, 0x224C},
	{0x8956, 0xA000},
	{0x8958, 0x224A},
	{0x895A, 0x2040},
	{0x895C, 0x22C2},
	{0x895E, 0x2482},
	{0x8960, 0x204F},
	{0x8962, 0x2040},
	{0x8964, 0x224C},
	{0x8966, 0xA000},
	{0x8968, 0xB8A2},
	{0x896A, 0xF204},
	{0x896C, 0x2045},
	{0x896E, 0x2180},
	{0x8970, 0xAE01},
	{0x8972, 0x0D9E},
	{0x8974, 0xFFE3},
	{0x8976, 0x70E9},
	{0x8978, 0x0125},
	{0x897A, 0x06C4},
	{0x897C, 0xC0F1},
	{0x897E, 0xD010},
	{0x8980, 0xD110},
	{0x8982, 0xD20D},
	{0x8984, 0xA020},
	{0x8986, 0x8A00},
	{0x8988, 0x0809},
	{0x898A, 0x01DE},
	{0x898C, 0xB8A7},
	{0x898E, 0xAA00},
	{0x8990, 0xDBFF},
	{0x8992, 0x2B41},
	{0x8994, 0x0200},
	{0x8996, 0xAA0C},
	{0x8998, 0x1228},
	{0x899A, 0x0080},
	{0x899C, 0xAA6D},
	{0x899E, 0x0815},
	{0x89A0, 0x01DE},
	{0x89A2, 0xB8A7},
	{0x89A4, 0x1A28},
	{0x89A6, 0x0002},
	{0x89A8, 0x8123},
	{0x89AA, 0x7960},
	{0x89AC, 0x1228},
	{0x89AE, 0x0080},
	{0x89B0, 0xC0D1},
	{0x89B2, 0x7EE0},
	{0x89B4, 0xFF80},
	{0x89B6, 0x0158},
	{0x89B8, 0xFF00},
	{0x89BA, 0x0618},
	{0x89BC, 0x8000},
	{0x89BE, 0x0008},
	{0x89C0, 0xFF80},
	{0x89C2, 0x0A08},
	{0x89C4, 0xE280},
	{0x89C6, 0x24CA},
	{0x89C8, 0x7082},
	{0x89CA, 0x78E0},
	{0x89CC, 0x20E8},
	{0x89CE, 0x01A2},
	{0x89D0, 0x1002},
	{0x89D2, 0x0D02},
	{0x89D4, 0x1902},
	{0x89D6, 0x0094},
	{0x89D8, 0x7FE0},
	{0x89DA, 0x7028},
	{0x89DC, 0x7308},
	{0x89DE, 0x1000},
	{0x89E0, 0x0900},
	{0x89E2, 0x7904},
	{0x89E4, 0x7947},
	{0x89E6, 0x1B00},
	{0x89E8, 0x0064},
	{0x89EA, 0x7EE0},
	{0x89EC, 0xE280},
	{0x89EE, 0x24CA},
	{0x89F0, 0x7082},
	{0x89F2, 0x78E0},
	{0x89F4, 0x20E8},
	{0x89F6, 0x01A2},
	{0x89F8, 0x1102},
	{0x89FA, 0x0502},
	{0x89FC, 0x1802},
	{0x89FE, 0x00B4},
	{0x8A00, 0x7FE0},
	{0x8A02, 0x7028},
	{0x8A04, 0x0000},
	{0x8A06, 0x0000},
	{0x8A08, 0xFF80},
	{0x8A0A, 0x097C},
	{0x8A0C, 0xFF80},
	{0x8A0E, 0x08CC},
	{0x8A10, 0x0000},
	{0x8A12, 0x08DC},
	{0x8A14, 0x0000},
	{0x8A16, 0x0998},
	{0x098E, 0x0016}, 	// LOGICAL_ADDRESS_ACCESS [MON_ADDRESS_LO]
	{0x8016, 0x086C}, 	// MON_ADDRESS_LO 
	{0x8002, 0x0001}, 	// MON_CMD
	DELAY_nMS_ARRAY(10),
	{0x098E, 0xC40C}, 	// LOGICAL_ADDRESS_ACCESS
	{0xC40C, 0x00FF}, 	// AFM_POS_MAX
	{0xC40A, 0x0000}, 	// AFM_POS_MIN
	{0x30D4, 0x9080}, 	// COLUMN_CORRECTION
	{0x316E, 0xCAFF}, 	// DAC_ECL
	{0x305E, 0x10A0}, 	// GLOBAL_GAIN
	{0x3E00, 0x0010}, 	// SAMP_CONTROL
	{0x3E02, 0xED02}, 	// SAMP_ADDR_EN
	{0x3E04, 0xC88C}, 	// SAMP_RD1_SIG
	{0x3E06, 0xC88C}, 	// SAMP_RD1_SIG_BOOST
	{0x3E08, 0x700A}, 	// SAMP_RD1_RST
	{0x3E0A, 0x701E}, 	// SAMP_RD1_RST_BOOST
	{0x3E0C, 0x00FF}, 	// SAMP_RST1_EN
	{0x3E0E, 0x00FF}, 	// SAMP_RST1_BOOST
	{0x3E10, 0x00FF}, 	// SAMP_RST1_CLOOP_SH
	{0x3E12, 0x0000}, 	// SAMP_RST_BOOST_SEQ
	{0x3E14, 0xC78C}, 	// SAMP_SAMP1_SIG
	{0x3E16, 0x6E06}, 	// SAMP_SAMP1_RST
	{0x3E18, 0xA58C}, 	// SAMP_TX_EN
	{0x3E1A, 0xA58E}, 	// SAMP_TX_BOOST
	{0x3E1C, 0xA58E}, 	// SAMP_TX_CLOOP_SH
	{0x3E1E, 0xC0D0}, 	// SAMP_TX_BOOST_SEQ
	{0x3E20, 0xEB00}, 	// SAMP_VLN_EN
	{0x3E22, 0x00FF}, 	// SAMP_VLN_HOLD
	{0x3E24, 0xEB02}, 	// SAMP_VCL_EN
	{0x3E26, 0xEA02}, 	// SAMP_COLCLAMP
	{0x3E28, 0xEB0A}, 	// SAMP_SH_VCL
	{0x3E2A, 0xEC01}, 	// SAMP_SH_VREF
	{0x3E2C, 0xEB01}, 	// SAMP_SH_VBST
	{0x3E2E, 0x00FF}, 	// SAMP_SPARE
	{0x3E30, 0x00F3}, 	// SAMP_READOUT
	{0x3E32, 0x3DFA}, 	// SAMP_RESET_DONE
	{0x3E34, 0x00FF}, 	// SAMP_VLN_CLAMP
	{0x3E36, 0x00F3}, 	// SAMP_ASC_INT
	{0x3E38, 0x0000}, 	// SAMP_RS_CLOOP_SH_R
	{0x3E3A, 0xF802}, 	// SAMP_RS_CLOOP_SH
	{0x3E3C, 0x0FFF}, 	// SAMP_RS_BOOST_SEQ
	{0x3E3E, 0xEA10}, 	// SAMP_TXLO_GND
	{0x3E40, 0xEB05}, 	// SAMP_VLN_PER_COL
	{0x3E42, 0xE5C8}, 	// SAMP_RD2_SIG
	{0x3E44, 0xE5C8}, 	// SAMP_RD2_SIG_BOOST
	{0x3E46, 0x8C70}, 	// SAMP_RD2_RST
	{0x3E48, 0x8C71}, 	// SAMP_RD2_RST_BOOST
	{0x3E4A, 0x00FF}, 	// SAMP_RST2_EN
	{0x3E4C, 0x00FF}, 	// SAMP_RST2_BOOST
	{0x3E4E, 0x00FF}, 	// SAMP_RST2_CLOOP_SH
	{0x3E50, 0xE38D}, 	// SAMP_SAMP2_SIG
	{0x3E52, 0x8B0A}, 	// SAMP_SAMP2_RST
	{0x3E58, 0xEB0A}, 	// SAMP_PIX_CLAMP_EN
	{0x3E5C, 0x0A00}, 	// SAMP_PIX_PULLUP_EN
	{0x3E5E, 0x00FF}, 	// SAMP_PIX_PULLDOWN_EN_R
	{0x3E60, 0x00FF}, 	// SAMP_PIX_PULLDOWN_EN_S
	{0x3E90, 0x3C01}, 	// RST_ADDR_EN
	{0x3E92, 0x00FF}, 	// RST_RST_EN
	{0x3E94, 0x00FF}, 	// RST_RST_BOOST
	{0x3E96, 0x3C00}, 	// RST_TX_EN
	{0x3E98, 0x3C00}, 	// RST_TX_BOOST
	{0x3E9A, 0x3C00}, 	// RST_TX_CLOOP_SH
	{0x3E9C, 0xC0E0}, 	// RST_TX_BOOST_SEQ
	{0x3E9E, 0x00FF}, 	// RST_RST_CLOOP_SH
	{0x3EA0, 0x0000}, 	// RST_RST_BOOST_SEQ
	{0x3EA6, 0x3C00}, 	// RST_PIX_PULLUP_EN
	{0x3ED8, 0x3057}, 	// DAC_LD_12_13
	{0x316C, 0xB44F}, 	// DAC_TXLO
	{0x316E, 0xCAFF}, 	// DAC_ECL
	{0x3ED2, 0xEA0A}, 	// DAC_LD_6_7
	{0x3ED4, 0x00A3}, 	// DAC_LD_8_9
	{0x3EDC, 0x6020}, 	// DAC_LD_16_17
	{0x3EE6, 0xA541}, 	// DAC_LD_26_27
	{0x31E0, 0x0000}, 	// PIX_DEF_ID
	{0x3ED0, 0x2409}, 	// DAC_LD_4_5
	{0x3EDE, 0x0A49}, 	// DAC_LD_18_19
	{0x3EE0, 0x4910}, 	// DAC_LD_20_21
	{0x3EE2, 0x09D2}, 	// DAC_LD_22_23
	{0x30B6, 0x0006}, 	// AUTOLR_CONTROL
	{0x337C, 0x0006}, 	// YUV_YCBCR_CONTROL
	{0x3210, 0x49B0}, 	// COLOR_PIPELINE_CONTROL 
	{0x337C, 0x0006},  	// YUV_YCBCR_CONTROL

	{0x3640, 0x00D0},	// P_G1_P0Q0	
	{0x3642, 0x5A2C},	// P_G1_P0Q1	
	{0x3644, 0x2FF1},	// P_G1_P0Q2	
	{0x3646, 0xD9A9},	// P_G1_P0Q3	
	{0x3648, 0xB710},	// P_G1_P0Q4	
	{0x364A, 0x0350},	// P_R_P0Q0 	
	{0x364C, 0x800D},	// P_R_P0Q1 	
	{0x364E, 0x35F0},	// P_R_P0Q2 	
	{0x3650, 0x176F},	// P_R_P0Q3 	
	{0x3652, 0x8F0F},	// P_R_P0Q4 	
	{0x3654, 0x0370},	// P_B_P0Q0 	
	{0x3656, 0x08ED},	// P_B_P0Q1 	
	{0x3658, 0x2190},	// P_B_P0Q2 	
	{0x365A, 0xF3AD},	// P_B_P0Q3 	
	{0x365C, 0x832F},	// P_B_P0Q4 	
	{0x365E, 0x0ED0},	// P_G2_P0Q0	
	{0x3660, 0xBB2E},	// P_G2_P0Q1	
	{0x3662, 0x4911},	// P_G2_P0Q2	
	{0x3664, 0x382F},	// P_G2_P0Q3	
	{0x3666, 0x85B1},	// P_G2_P0Q4	
	{0x3680, 0x978B},	// P_G1_P1Q0	
	{0x3682, 0x858E},	// P_G1_P1Q1	
	{0x3684, 0x41CD},	// P_G1_P1Q2	
	{0x3686, 0x322D},	// P_G1_P1Q3	
	{0x3688, 0xE1CE},	// P_G1_P1Q4	
	{0x368A, 0x19AE},	// P_R_P1Q0 	
	{0x368C, 0x20EE},	// P_R_P1Q1 	
	{0x368E, 0x862F},	// P_R_P1Q2 	
	{0x3690, 0xD50E},	// P_R_P1Q3 	
	{0x3692, 0x26CA},	// P_R_P1Q4 	
	{0x3694, 0xBDAD},	// P_B_P1Q0 	
	{0x3696, 0x508B},	// P_B_P1Q1 	
	{0x3698, 0x272D},	// P_B_P1Q2 	
	{0x369A, 0xD48E},	// P_B_P1Q3 	
	{0x369C, 0x8A8F},	// P_B_P1Q4 	
	{0x369E, 0xDB2C},	// P_G2_P1Q0	
	{0x36A0, 0x9E4E},	// P_G2_P1Q1	
	{0x36A2, 0xC18D},	// P_G2_P1Q2	
	{0x36A4, 0x0FCF},	// P_G2_P1Q3	
	{0x36A6, 0x2E8E},	// P_G2_P1Q4	
	{0x36C0, 0x57B1},	// P_G1_P2Q0	
	{0x36C2, 0x0CCF},	// P_G1_P2Q1	
	{0x36C4, 0xCD91},	// P_G1_P2Q2	
	{0x36C6, 0xB94F},	// P_G1_P2Q3	
	{0x36C8, 0x5091},	// P_G1_P2Q4	
	{0x36CA, 0x27F1},	// P_R_P2Q0 	
	{0x36CC, 0x8968},	// P_R_P2Q1 	
	{0x36CE, 0xE630},	// P_R_P2Q2 	
	{0x36D0, 0x9EB0},	// P_R_P2Q3 	
	{0x36D2, 0x0DF2},	// P_R_P2Q4 	
	{0x36D4, 0x1531},	// P_B_P2Q0 	
	{0x36D6, 0x3D8F},	// P_B_P2Q1 	
	{0x36D8, 0xB351},	// P_B_P2Q2 	
	{0x36DA, 0x9AB0},	// P_B_P2Q3 	
	{0x36DC, 0x6C52},	// P_B_P2Q4 	
	{0x36DE, 0x6E31},	// P_G2_P2Q0	
	{0x36E0, 0x418C},	// P_G2_P2Q1	
	{0x36E2, 0xC292},	// P_G2_P2Q2	
	{0x36E4, 0xE650},	// P_G2_P2Q3	
	{0x36E6, 0x1193},	// P_G2_P2Q4	
	{0x3700, 0x6F4F},	// P_G1_P3Q0	
	{0x3702, 0x248B},	// P_G1_P3Q1	
	{0x3704, 0x946B},	// P_G1_P3Q2	
	{0x3706, 0xEE4E},	// P_G1_P3Q3	
	{0x3708, 0x5330},	// P_G1_P3Q4	
	{0x370A, 0x07D0},	// P_R_P3Q0 	
	{0x370C, 0xD12B},	// P_R_P3Q1 	
	{0x370E, 0xAB10},	// P_R_P3Q2 	
	{0x3710, 0x810F},	// P_R_P3Q3 	
	{0x3712, 0x5052},	// P_R_P3Q4 	
	{0x3714, 0xA16F},	// P_B_P3Q0 	
	{0x3716, 0x9F4E},	// P_B_P3Q1 	
	{0x3718, 0x1872},	// P_B_P3Q2 	
	{0x371A, 0x0250},	// P_B_P3Q3 	
	{0x371C, 0xADB2},	// P_B_P3Q4 	
	{0x371E, 0x47CD},	// P_G2_P3Q0	
	{0x3720, 0x156E},	// P_G2_P3Q1	
	{0x3722, 0x5D91},	// P_G2_P3Q2	
	{0x3724, 0xC9D1},	// P_G2_P3Q3	
	{0x3726, 0xC530},	// P_G2_P3Q4	
	{0x3740, 0x86B1},	// P_G1_P4Q0	
	{0x3742, 0x8ED0},	// P_G1_P4Q1	
	{0x3744, 0xB12E},	// P_G1_P4Q2	
	{0x3746, 0x5FF2},	// P_G1_P4Q3	
	{0x3748, 0x01B1},	// P_G1_P4Q4	
	{0x374A, 0x9CB1},	// P_R_P4Q0 	
	{0x374C, 0x81F0},	// P_R_P4Q1 	
	{0x374E, 0x1733},	// P_R_P4Q2 	
	{0x3750, 0x2BF2},	// P_R_P4Q3 	
	{0x3752, 0xF133},	// P_R_P4Q4 	
	{0x3754, 0xC04F},	// P_B_P4Q0 	
	{0x3756, 0xB070},	// P_B_P4Q1 	
	{0x3758, 0x1AD0},	// P_B_P4Q2 	
	{0x375A, 0x4F92},	// P_B_P4Q3 	
	{0x375C, 0xF971},	// P_B_P4Q4 	
	{0x375E, 0xAC11},	// P_G2_P4Q0	
	{0x3760, 0xC28F},	// P_G2_P4Q1	
	{0x3762, 0x34D2},	// P_G2_P4Q2	
	{0x3764, 0x0AD3},	// P_G2_P4Q3	
	{0x3766, 0xBF33},	// P_G2_P4Q4	
	{0x3782, 0x0300},	// CENTER_ROW	
	{0x3784, 0x04F4},	// CENTER_COLUMN

	{0x3210, 0x49B8},  	// COLOR_PIPELINE_CONTROL
	{0x098E, 0xAC01}, 	// LOGICAL_ADDRESS_ACCESS [AWB_MODE]

	REGLIST_8BIT_START_FLAG,
	{0xAC01, 0xAB}, 	// AWB_MODE
	REGLIST_8BIT_STOP_FLAG,
	//
	//[AWB and CCMs 06/26/14 14:12:30]
	{0x098E, 0x2C46},	// LOGICAL_ADDRESS_ACCESS [AWB_LEFT_CCM_0]
	{0xAC46, 0x0221},	// AWB_LEFT_CCM_0
	{0xAC48, 0xFEAE},	// AWB_LEFT_CCM_1
	{0xAC4A, 0x0032},	// AWB_LEFT_CCM_2
	{0xAC4C, 0xFFA1},	// AWB_LEFT_CCM_3
	{0xAC4E, 0x0187},	// AWB_LEFT_CCM_4
	{0xAC50, 0xFFD8},	// AWB_LEFT_CCM_5
	{0xAC52, 0xFFB1},	// AWB_LEFT_CCM_6
	{0xAC54, 0xFEC5},	// AWB_LEFT_CCM_7
	{0xAC56, 0x028A},	// AWB_LEFT_CCM_8                                         
	{0xAC58, 0x0118},	// AWB_LEFT_CCM_R2BRATIO                                         
	{0xAC5C, 0x02F7},	// AWB_RIGHT_CCM_0                                         
	{0xAC5E, 0xFE0F},	// AWB_RIGHT_CCM_1                                         
	{0xAC60, 0xFFF5},	// AWB_RIGHT_CCM_2                                         
	{0xAC62, 0xFFCB},	// AWB_RIGHT_CCM_3                                         
	{0xAC64, 0x0139},	// AWB_RIGHT_CCM_4                                         
	{0xAC66, 0xFFF9},	// AWB_RIGHT_CCM_5                                         
	{0xAC68, 0x0047},	// AWB_RIGHT_CCM_6                                         
	{0xAC6A, 0xFE34},	// AWB_RIGHT_CCM_7                                         
	{0xAC6C, 0x033A},	// AWB_RIGHT_CCM_8                                         
	{0xAC6E, 0x0070},	// AWB_RIGHT_CCM_R2BRATIO  
	REGLIST_8BIT_START_FLAG,
	{0xB83E, 0x00},	// STAT_AWB_WINDOW_POS_X                                         
	{0xB83F, 0x00},	// STAT_AWB_WINDOW_POS_Y                                         
	{0xB840, 0xFF},	// STAT_AWB_WINDOW_SIZE_X                                         
	{0xB841, 0xEF},	// STAT_AWB_WINDOW_SIZE_Y 
	REGLIST_8BIT_STOP_FLAG,
	{0x098E, 0x3842}, 	// LOGICAL_ADDRESS_ACCESS [STAT_AWB_GRAY_CHECKER_OFFSET_X]
	{0xB842, 0x0037}, 	// STAT_AWB_GRAY_CHECKER_OFFSET_X
	{0xB844, 0x0044}, 	// STAT_AWB_GRAY_CHECKER_OFFSET_Y
	{0x3240, 0x0024}, 	// AWB_XY_SCALE
	{0x3240, 0x0024}, 	// AWB_XY_SCALE
	{0x3242, 0x0000}, 	// AWB_WEIGHT_R0
	{0x3244, 0x0000}, 	// AWB_WEIGHT_R1
	{0x3246, 0x0000}, 	// AWB_WEIGHT_R2
	{0x3248, 0x7F00}, 	// AWB_WEIGHT_R3
	{0x324A, 0xA500}, 	// AWB_WEIGHT_R4
	{0x324C, 0x1540}, 	// AWB_WEIGHT_R5
	{0x324E, 0x01AC}, 	// AWB_WEIGHT_R6
	{0x3250, 0x003E}, 	// AWB_WEIGHT_R7
	{0x098E, 0xAC3C}, 	// LOGICAL_ADDRESS_ACCESS [AWB_MIN_ACCEPTED_PRE_AWB_R2G_RATIO]

	REGLIST_8BIT_START_FLAG,
	{0xAC3C, 0x2E}, 	// AWB_MIN_ACCEPTED_PRE_AWB_R2G_RATIO
	{0xAC3D, 0x84}, 	// AWB_MAX_ACCEPTED_PRE_AWB_R2G_RATIO
	{0xAC3E, 0x11}, 	// AWB_MIN_ACCEPTED_PRE_AWB_B2G_RATIO
	{0xAC3F, 0x63}, 	// AWB_MAX_ACCEPTED_PRE_AWB_B2G_RATIO
	{0xACB0, 0x2B}, 	// AWB_RG_MIN
	{0xACB1, 0x84}, 	// AWB_RG_MAX
	{0xACB4, 0x11}, 	// AWB_BG_MIN
	{0xACB5, 0x63}, 	// AWB_BG_MAX
	REGLIST_8BIT_STOP_FLAG,
	
	{0x098E, 0xD80F}, 	// LOGICAL_ADDRESS_ACCESS [JPEG_QSCALE_0]

	REGLIST_8BIT_START_FLAG,
	{0xD80F, 0x04}, 	// JPEG_QSCALE_0
	{0xD810, 0x08}, 	// JPEG_QSCALE_1
	{0xC8D2, 0x04}, 	// CAM_OUTPUT_1_JPEG_QSCALE_0
	{0xC8D3, 0x08}, 	// CAM_OUTPUT_1_JPEG_QSCALE_1
	{0xC8BC, 0x04}, 	// CAM_OUTPUT_0_JPEG_QSCALE_0
	{0xC8BD, 0x08}, 	// CAM_OUTPUT_0_JPEG_QSCALE_1
	REGLIST_8BIT_STOP_FLAG,
	
	{0x301A, 0x10F4}, 	// RESET_REGISTER
	{0x301E, 0x0000}, 	// DATA_PEDESTAL
	{0x301A, 0x10FC}, 	// RESET_REGISTER
	{0x098E, 0xDC33}, 	// LOGICAL_ADDRESS_ACCESS [SYS_FIRST_BLACK_LEVEL]

	REGLIST_8BIT_START_FLAG,
	{0xDC33, 0x00}, 	// SYS_FIRST_BLACK_LEVEL
	{0xDC35, 0x04}, 	// SYS_UV_COLOR_BOOST
	REGLIST_8BIT_STOP_FLAG,
	
	{0x326E, 0x0006}, 	// LOW_PASS_YUV_FILTER
	REGLIST_8BIT_START_FLAG,
	{0xDC37, 0x62}, 	// SYS_BRIGHT_COLORKILL
	REGLIST_8BIT_STOP_FLAG,
	{0x35A4, 0x0596}, 	// BRIGHT_COLOR_KILL_CONTROLS
	{0x35A2, 0x0094}, 	// DARK_COLOR_KILL_CONTROLS
	REGLIST_8BIT_START_FLAG,
	{0xDC36, 0x23}, 	// SYS_DARK_COLOR_KILL
	REGLIST_8BIT_STOP_FLAG,
	{0x098E, 0xBC18}, 	// LOGICAL_ADDRESS_ACCESS [LL_GAMMA_CONTRAST_CURVE_0]
	REGLIST_8BIT_START_FLAG,
	{0xBC18, 0x00}, 	// LL_GAMMA_CONTRAST_CURVE_0
	{0xBC19, 0x11}, 	// LL_GAMMA_CONTRAST_CURVE_1
	{0xBC1A, 0x23}, 	// LL_GAMMA_CONTRAST_CURVE_2
	{0xBC1B, 0x3F}, 	// LL_GAMMA_CONTRAST_CURVE_3
	{0xBC1C, 0x67}, 	// LL_GAMMA_CONTRAST_CURVE_4
	{0xBC1D, 0x85}, 	// LL_GAMMA_CONTRAST_CURVE_5
	{0xBC1E, 0x9B}, 	// LL_GAMMA_CONTRAST_CURVE_6
	{0xBC1F, 0xAD}, 	// LL_GAMMA_CONTRAST_CURVE_7
	{0xBC20, 0xBB}, 	// LL_GAMMA_CONTRAST_CURVE_8
	{0xBC21, 0xC7}, 	// LL_GAMMA_CONTRAST_CURVE_9
	{0xBC22, 0xD1}, 	// LL_GAMMA_CONTRAST_CURVE_10
	{0xBC23, 0xDA}, 	// LL_GAMMA_CONTRAST_CURVE_11
	{0xBC24, 0xE1}, 	// LL_GAMMA_CONTRAST_CURVE_12
	{0xBC25, 0xE8}, 	// LL_GAMMA_CONTRAST_CURVE_13
	{0xBC26, 0xEE}, 	// LL_GAMMA_CONTRAST_CURVE_14
	{0xBC27, 0xF3}, 	// LL_GAMMA_CONTRAST_CURVE_15
	{0xBC28, 0xF7}, 	// LL_GAMMA_CONTRAST_CURVE_16
	{0xBC29, 0xFB}, 	// LL_GAMMA_CONTRAST_CURVE_17
	{0xBC2A, 0xFF}, 	// LL_GAMMA_CONTRAST_CURVE_18
	{0xBC2B, 0x00}, 	// LL_GAMMA_NEUTRAL_CURVE_0
	{0xBC2C, 0x11}, 	// LL_GAMMA_NEUTRAL_CURVE_1
	{0xBC2D, 0x23}, 	// LL_GAMMA_NEUTRAL_CURVE_2
	{0xBC2E, 0x3F}, 	// LL_GAMMA_NEUTRAL_CURVE_3
	{0xBC2F, 0x67}, 	// LL_GAMMA_NEUTRAL_CURVE_4
	{0xBC30, 0x85}, 	// LL_GAMMA_NEUTRAL_CURVE_5
	{0xBC31, 0x9B}, 	// LL_GAMMA_NEUTRAL_CURVE_6
	{0xBC32, 0xAD}, 	// LL_GAMMA_NEUTRAL_CURVE_7
	{0xBC33, 0xBB}, 	// LL_GAMMA_NEUTRAL_CURVE_8
	{0xBC34, 0xC7}, 	// LL_GAMMA_NEUTRAL_CURVE_9
	{0xBC35, 0xD1}, 	// LL_GAMMA_NEUTRAL_CURVE_10
	{0xBC36, 0xDA}, 	// LL_GAMMA_NEUTRAL_CURVE_11
	{0xBC37, 0xE1}, 	// LL_GAMMA_NEUTRAL_CURVE_12
	{0xBC38, 0xE8}, 	// LL_GAMMA_NEUTRAL_CURVE_13
	{0xBC39, 0xEE}, 	// LL_GAMMA_NEUTRAL_CURVE_14
	{0xBC3A, 0xF3}, 	// LL_GAMMA_NEUTRAL_CURVE_15
	{0xBC3B, 0xF7}, 	// LL_GAMMA_NEUTRAL_CURVE_16
	{0xBC3C, 0xFB}, 	// LL_GAMMA_NEUTRAL_CURVE_17
	{0xBC3D, 0xFF}, 	// LL_GAMMA_NEUTRAL_CURVE_18
	{0xBC3E, 0x00}, 	// LL_GAMMA_NR_CURVE_0
	{0xBC3F, 0x18}, 	// LL_GAMMA_NR_CURVE_1
	{0xBC40, 0x25}, 	// LL_GAMMA_NR_CURVE_2
	{0xBC41, 0x3A}, 	// LL_GAMMA_NR_CURVE_3
	{0xBC42, 0x59}, 	// LL_GAMMA_NR_CURVE_4
	{0xBC43, 0x70}, 	// LL_GAMMA_NR_CURVE_5
	{0xBC44, 0x81}, 	// LL_GAMMA_NR_CURVE_6
	{0xBC45, 0x90}, 	// LL_GAMMA_NR_CURVE_7
	{0xBC46, 0x9E}, 	// LL_GAMMA_NR_CURVE_8
	{0xBC47, 0xAB}, 	// LL_GAMMA_NR_CURVE_9
	{0xBC48, 0xB6}, 	// LL_GAMMA_NR_CURVE_10
	{0xBC49, 0xC1}, 	// LL_GAMMA_NR_CURVE_11
	{0xBC4A, 0xCB}, 	// LL_GAMMA_NR_CURVE_12
	{0xBC4B, 0xD5}, 	// LL_GAMMA_NR_CURVE_13
	{0xBC4C, 0xDE}, 	// LL_GAMMA_NR_CURVE_14
	{0xBC4D, 0xE7}, 	// LL_GAMMA_NR_CURVE_15
	{0xBC4E, 0xEF}, 	// LL_GAMMA_NR_CURVE_16
	{0xBC4F, 0xF7}, 	// LL_GAMMA_NR_CURVE_17
	{0xBC50, 0xFF}, 	// LL_GAMMA_NR_CURVE_18
	REGLIST_8BIT_STOP_FLAG,
	{0x098E, 0xB801}, 	// LOGICAL_ADDRESS_ACCESS [STAT_MODE]
	REGLIST_8BIT_START_FLAG,
	{0xB801, 0xE0}, 	// STAT_MODE
	{0xB862, 0x04}, 	// STAT_BMTRACKING_SPEED
	REGLIST_8BIT_STOP_FLAG,
	{0x098E, 0xB829}, 	// LOGICAL_ADDRESS_ACCESS [STAT_LL_BRIGHTNESS_METRIC_DIVISOR]
	REGLIST_8BIT_START_FLAG,
	{0xB829, 0x02}, 	// STAT_LL_BRIGHTNESS_METRIC_DIVISOR
	{0xB863, 0x02}, 	// STAT_BM_MUL
	{0xB827, 0x0F}, 	// STAT_AE_EV_SHIFT
	//{0xA409, 0x37}, 	// AE_RULE_BASE_TARGET
	{0xA409, 0x27},	 // AE_RULE_BASE_TARGET   áá?è
	REGLIST_8BIT_STOP_FLAG,
	{0x337E, 0x2000},
	{0x098E, 0x3C52}, 	// LOGICAL_ADDRESS_ACCESS [LL_START_BRIGHTNESS_METRIC]
	{0xBC52, 0x00C8}, 	// LL_START_BRIGHTNESS_METRIC
	{0xBC54, 0x0A28}, 	// LL_END_BRIGHTNESS_METRIC
	{0xBC58, 0x00C8}, 	// LL_START_GAIN_METRIC
	{0xBC5A, 0x12C0}, 	// LL_END_GAIN_METRIC
	{0xBC5E, 0x00FA}, 	// LL_START_APERTURE_GAIN_BM
	{0xBC60, 0x0258}, 	// LL_END_APERTURE_GAIN_BM
	{0xBC66, 0x00FA}, 	// LL_START_APERTURE_GM
	{0xBC68, 0x0258}, 	// LL_END_APERTURE_GM
	{0xBC86, 0x00C8}, 	// LL_START_FFNR_GM
	{0xBC88, 0x0640}, 	// LL_END_FFNR_GM
	{0xBCBC, 0x0040}, 	// LL_SFFB_START_GAIN
	{0xBCBE, 0x01FC}, 	// LL_SFFB_END_GAIN
	{0xBCCC, 0x00C8}, 	// LL_SFFB_START_MAX_GM
	{0xBCCE, 0x0640}, 	// LL_SFFB_END_MAX_GM
	{0xBC90, 0x00C8}, 	// LL_START_GRB_GM
	{0xBC92, 0x0640}, 	// LL_END_GRB_GM
	{0xBC0E, 0x0001}, 	// LL_GAMMA_CURVE_ADJ_START_POS
	{0xBC10, 0x0002}, 	// LL_GAMMA_CURVE_ADJ_MID_POS
	{0xBC12, 0x02BC}, 	// LL_GAMMA_CURVE_ADJ_END_POS
	{0xBCAA, 0x044C}, 	// LL_CDC_THR_ADJ_START_POS
	{0xBCAC, 0x00AF}, 	// LL_CDC_THR_ADJ_MID_POS
	{0xBCAE, 0x0009}, 	// LL_CDC_THR_ADJ_END_POS
	{0xBCD8, 0x00C8}, 	// LL_PCR_START_BM
	{0xBCDA, 0x0A28}, 	// LL_PCR_END_BM
	{0x3380, 0x0504}, 	// KERNEL_CONFIG
	{0x098E, 0xBC94}, 	// LOGICAL_ADDRESS_ACCESS [LL_GB_START_THRESHOLD_0]
	REGLIST_8BIT_START_FLAG,
	{0xBC94, 0x0C}, 	// LL_GB_START_THRESHOLD_0
	{0xBC95, 0x08}, 	// LL_GB_START_THRESHOLD_1
	{0xBC9C, 0x3C}, 	// LL_GB_END_THRESHOLD_0
	{0xBC9D, 0x28}, 	// LL_GB_END_THRESHOLD_1
	REGLIST_8BIT_STOP_FLAG,
	{0x33B0, 0x2A16}, 	// FFNR_ALPHA_BETA
	{0x098E, 0xBC8A}, 	// LOGICAL_ADDRESS_ACCESS [LL_START_FF_MIX_THRESH_Y]
	REGLIST_8BIT_START_FLAG,
	{0xBC8A, 0x02}, 	// LL_START_FF_MIX_THRESH_Y
	{0xBC8B, 0x0F}, 	// LL_END_FF_MIX_THRESH_Y
	{0xBC8C, 0xFF}, 	// LL_START_FF_MIX_THRESH_YGAIN
	{0xBC8D, 0xFF}, 	// LL_END_FF_MIX_THRESH_YGAIN
	{0xBC8E, 0xFF}, 	// LL_START_FF_MIX_THRESH_GAIN
	{0xBC8F, 0x00}, 	// LL_END_FF_MIX_THRESH_GAIN
	REGLIST_8BIT_STOP_FLAG,
	{0x098E, 0xBCB2}, 	// LOGICAL_ADDRESS_ACCESS [LL_CDC_DARK_CLUS_SLOPE]
	
	REGLIST_8BIT_START_FLAG,
	{0xBCB2, 0x20}, 	// LL_CDC_DARK_CLUS_SLOPE
	{0xBCB3, 0x3A}, 	// LL_CDC_DARK_CLUS_SATUR
	{0xBCB4, 0x39}, 	// LL_CDC_BRIGHT_CLUS_LO_LIGHT_SLOPE
	{0xBCB7, 0x39}, 	// LL_CDC_BRIGHT_CLUS_LO_LIGHT_SATUR
	{0xBCB5, 0x20}, 	// LL_CDC_BRIGHT_CLUS_MID_LIGHT_SLOPE
	{0xBCB8, 0x3A}, 	// LL_CDC_BRIGHT_CLUS_MID_LIGHT_SATUR
	{0xBCB6, 0x80}, 	// LL_CDC_BRIGHT_CLUS_HI_LIGHT_SLOPE
	{0xBCB9, 0x24}, 	// LL_CDC_BRIGHT_CLUS_HI_LIGHT_SATUR
	REGLIST_8BIT_STOP_FLAG,
	{0xBCAA, 0x03E8}, 	// LL_CDC_THR_ADJ_START_POS
	{0xBCAC, 0x012C}, 	// LL_CDC_THR_ADJ_MID_POS
	{0xBCAE, 0x0009}, 	// LL_CDC_THR_ADJ_END_POS
	{0x33BA, 0x0084}, 	// APEDGE_CONTROL
	{0x33BE, 0x0000}, 	// UA_KNEE_L
	{0x33C2, 0x8800}, 	// UA_WEIGHTS
	{0x098E, 0x3C5E}, 	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_GAIN_BM]
	{0xBC5E, 0x0154}, 	// LL_START_APERTURE_GAIN_BM
	{0xBC60, 0x0640}, 	// LL_END_APERTURE_GAIN_BM

	REGLIST_8BIT_START_FLAG,
	{0xBC62, 0x0E}, 	// LL_START_APERTURE_KPGAIN
	{0xBC63, 0x14}, 	// LL_END_APERTURE_KPGAIN
	{0xBC64, 0x0E}, 	// LL_START_APERTURE_KNGAIN
	{0xBC65, 0x14}, 	// LL_END_APERTURE_KNGAIN
	{0xBCE2, 0x0A}, 	// LL_START_POS_KNEE
	{0xBCE3, 0x2B}, 	// LL_END_POS_KNEE
	{0xBCE4, 0x0A}, 	// LL_START_NEG_KNEE
	{0xBCE5, 0x2B}, 	// LL_END_NEG_KNEE
	REGLIST_8BIT_STOP_FLAG,
	
	{0x098E, 0xBCC0}, 	// LOGICAL_ADDRESS_ACCESS [LL_SFFB_RAMP_START]
	REGLIST_8BIT_START_FLAG,
	{0xBCC0, 0x1F}, 	// LL_SFFB_RAMP_START
	{0xBCC1, 0x03}, 	// LL_SFFB_RAMP_STOP
	{0xBCC2, 0x2C}, 	// LL_SFFB_SLOPE_START
	{0xBCC3, 0x10}, 	// LL_SFFB_SLOPE_STOP
	{0xBCC4, 0x07}, 	// LL_SFFB_THSTART
	{0xBCC5, 0x0B}, 	// LL_SFFB_THSTOP
	REGLIST_8BIT_STOP_FLAG,
	{0xBCBA, 0x0009}, 	// LL_SFFB_CONFIG
	{0x098E, 0x3C14}, 	// LOGICAL_ADDRESS_ACCESS [LL_GAMMA_FADE_TO_BLACK_START_POS]
	{0xBC14, 0xFFFE}, 	// LL_GAMMA_FADE_TO_BLACK_START_POS
	{0xBC16, 0xFFFF}, 	// LL_GAMMA_FADE_TO_BLACK_END_POS
	{0x098E, 0x3C66}, 	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_GM]
	{0xBC66, 0x0154}, 	// LL_START_APERTURE_GM
	{0xBC68, 0x07D0}, 	// LL_END_APERTURE_GM

	REGLIST_8BIT_START_FLAG,
	{0xBC6A, 0x04}, 	// LL_START_APERTURE_INTEGER_GAIN
	{0xBC6B, 0x00}, 	// LL_END_APERTURE_INTEGER_GAIN
	{0xBC6C, 0x00}, 	// LL_START_APERTURE_EXP_GAIN
	{0xBC6D, 0x00}, 	// LL_END_APERTURE_EXP_GAIN
	REGLIST_8BIT_STOP_FLAG,
	
	{0x098E, 0x281C}, 	// LOGICAL_ADDRESS_ACCESS [AE_TRACK_MIN_AGAIN]
	{0xA81C, 0x0080}, 	// AE_TRACK_MIN_AGAIN
	//{0xA820, 0x01FC}, 	// AE_TRACK_MAX_AGAIN
	{0xA820, 0x017E}, 	// AE_TRACK_MAX_AGAIN   DT???a???é???óDT??×?′ó???ê
	{0xA822, 0x0080}, 	// AE_TRACK_MIN_DGAIN
	{0xA824, 0x0100}, 	// AE_TRACK_MAX_DGAIN
	{0x098E, 0xBC56}, 	// LOGICAL_ADDRESS_ACCESS [LL_START_CCM_SATURATION]
	REGLIST_8BIT_START_FLAG,
	{0xBC56, 0x64}, 	// LL_START_CCM_SATURATION
	{0xBC57, 0x1E}, 	// LL_END_CCM_SATURATION
	REGLIST_8BIT_STOP_FLAG,
	{0x098E, 0xBCDE}, 	// LOGICAL_ADDRESS_ACCESS [LL_START_SYS_THRESHOLD]
	REGLIST_8BIT_START_FLAG,
	{0xBCDE, 0x03}, 	// LL_START_SYS_THRESHOLD
	{0xBCDF, 0x50}, 	// LL_STOP_SYS_THRESHOLD
	{0xBCE0, 0x08}, 	// LL_START_SYS_GAIN
	{0xBCE1, 0x03}, 	// LL_STOP_SYS_GAIN
	REGLIST_8BIT_STOP_FLAG,
	{0x098E, 0x3CD0}, 	// LOGICAL_ADDRESS_ACCESS [LL_SFFB_SOBEL_FLAT_START]
	{0xBCD0, 0x000A}, 	// LL_SFFB_SOBEL_FLAT_START
	{0xBCD2, 0x00FE}, 	// LL_SFFB_SOBEL_FLAT_STOP
	{0xBCD4, 0x001E}, 	// LL_SFFB_SOBEL_SHARP_START
	{0xBCD6, 0x00FF}, 	// LL_SFFB_SOBEL_SHARP_STOP
	REGLIST_8BIT_START_FLAG,
	{0xBCC6, 0x00}, 	// LL_SFFB_SHARPENING_START
	{0xBCC7, 0x00}, 	// LL_SFFB_SHARPENING_STOP
	{0xBCC8, 0x20}, 	// LL_SFFB_FLATNESS_START
	{0xBCC9, 0x40}, 	// LL_SFFB_FLATNESS_STOP
	{0xBCCA, 0x04}, 	// LL_SFFB_TRANSITION_START
	{0xBCCB, 0x00}, 	// LL_SFFB_TRANSITION_STOP
	REGLIST_8BIT_STOP_FLAG,
	{0x098E, 0xBCE6}, 	// LOGICAL_ADDRESS_ACCESS [LL_SFFB_ZERO_ENABLE]
	REGLIST_8BIT_START_FLAG,
	{0xBCE6, 0x03}, 	// LL_SFFB_ZERO_ENABLE
	{0xBCE6, 0x03}, 	// LL_SFFB_ZERO_ENABLE
	REGLIST_8BIT_STOP_FLAG,
	{0x098E, 0xA410}, 	// LOGICAL_ADDRESS_ACCESS [AE_RULE_TARGET_AE_6]
	REGLIST_8BIT_START_FLAG,
	{0xA410, 0x04}, 	// AE_RULE_TARGET_AE_6
	{0xA411, 0x06}, 	// AE_RULE_TARGET_AE_7
	
	REGLIST_8BIT_STOP_FLAG,
	{0x098E, 0xC8BC}, 	// LOGICAL_ADDRESS_ACCESS [CAM_OUTPUT_0_JPEG_QSCALE_0]
	REGLIST_8BIT_START_FLAG,
	{0xC8BC, 0x04}, 	// CAM_OUTPUT_0_JPEG_QSCALE_0
	{0xC8BD, 0x0A}, 	// CAM_OUTPUT_0_JPEG_QSCALE_1
	{0xC8D2, 0x04}, 	// CAM_OUTPUT_1_JPEG_QSCALE_0
	{0xC8D3, 0x0A}, 	// CAM_OUTPUT_1_JPEG_QSCALE_1
	{0xDC3A, 0x23}, 	// SYS_SEPIA_CR
	{0xDC3B, 0xB2}, 	// SYS_SEPIA_CB
	REGLIST_8BIT_STOP_FLAG,
	
	{0x098E, 0x8404},

	REGLIST_8BIT_START_FLAG,
	{0x8404, 0x06}, 	// SEQ_CMD
	REGLIST_8BIT_STOP_FLAG,
	DELAY_nMS_ARRAY(10),

	{0x0018, 0x2008}, 	// STANDBY_CONTROL_AND_STATUS
	DELAY_nMS_ARRAY(10),

	{0x098E, 0x1000}, 
	{0xC872, 0x0010}, 
	{0xC874, 0x001C}, 
	{0xC876, 0x07AF}, 
	{0xC878, 0x0A43}, 
	REGLIST_8BIT_START_FLAG,
	{0xDC0A, 0x06	}, 
	REGLIST_8BIT_STOP_FLAG,
	{0xDC1C, 0x2710 }, 
	{0xC8A4, 0x0A28 }, 
	{0xC8A6, 0x07A0 },
	{0xC8C0, 0x0A20 },	// CAM_OUTPUT_1_IMAGE_WIDTH
	{0xC8C2, 0x0798},	// CAM_OUTPUT_1_IMAGE_HEIGHT

	{0x098E, 0x843C}, 
	REGLIST_8BIT_START_FLAG,
	{0x843C, 0xFF}, 
	{0x8404, 0x02},
	{0xac01, 0xab}, 
	{0xac97, 0x80}, 
	{0xac98, 0x80}, 
	{0xac99, 0x80}, 
	{0xAC9a, 0x80}, 
	{0xAC9b, 0x80},
	{0xAC9c, 0x80},  
	REGLIST_8BIT_STOP_FLAG,

	{0xFFFF,0xFFFF},
};




/* 640*480: VGA */
static const struct regval_list module_vga_regs[] = 
{

    {0x098E, 0x48AA}, 	// LOGICAL_ADDRESS_ACCESS [CAM_OUTPUT_0_IMAGE_WIDTH]
	{0xC8AA, 0x0280}, 	// CAM_OUTPUT_0_IMAGE_WIDTH
	{0xC8AC, 0x01E0}, 	// CAM_OUTPUT_0_IMAGE_HEIGH
	{0x098E, 0x843C}, 	// LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_5_MAX_FRAME_CNT]
	REGLIST_8BIT_START_FLAG,
	{0x843C, 0x01},	// SEQ_STATE_CFG_5_MAX_FRAME_CNT
	{0x8404, 0x01},	// SEQ_CMD
	REGLIST_8BIT_STOP_FLAG,
	DELAY_nMS_ARRAY(10),
	{0x0016, 0x0447}, 	// CLOCKS_CONTROL
    {0xFFFF,0xFFFF},
};

/* 800*600: SVGA */
static const struct regval_list module_svga_regs[] = 
{

	//DELAY_nMS_ARRAY(20),
    {0x098E, 0x48AA}, 	// LOGICAL_ADDRESS_ACCESS [CAM_OUTPUT_0_IMAGE_WIDTH]
	{0xC8AA, 0x0320}, 	// CAM_OUTPUT_0_IMAGE_WIDTH   320
	{0xC8AC, 0x0258}, 	// CAM_OUTPUT_0_IMAGE_HEIGH   258
	{0x098E, 0x843C}, 	// LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_5_MAX_FRAME_CNT]
	REGLIST_8BIT_START_FLAG,
	{0x843C, 0x01},	// SEQ_STATE_CFG_5_MAX_FRAME_CNT
	{0x8404, 0x01},	// SEQ_CMD
	REGLIST_8BIT_STOP_FLAG,
	DELAY_nMS_ARRAY(10),
	{0x0016, 0x0447}, 	// CLOCKS_CONTROL
   {0xFFFF,0xFFFF},
};


/* 1280*720: 720P*/
static const struct regval_list module_720p_regs[] = 
{
	
	{0x098E, 0x843C}, // LOGICAL_ADDRESS_ACCESS [CAM_CORE_A_Y_ADDR_START]
	REGLIST_8BIT_START_FLAG,
	{0x843C, 0x01}, // SEQ_STATE_CFG_5_MAX_FRAME_CNT
	{0x8404, 0x01}, // SEQ_CMD
	REGLIST_8BIT_STOP_FLAG, 
	{0x0016, 0x0447},  // CLOCKS_CONTROL
	{0xC83A, 0x0106},  // CAM_CORE_A_Y_ADDR_START
	{0xC83C, 0x0018},  // CAM_CORE_A_X_ADDR_START
	{0xC83E, 0x06B7},  // CAM_CORE_A_Y_ADDR_END
	{0xC840, 0x0A45},  // CAM_CORE_A_X_ADDR_END
	{0xC86C, 0x0518},  // CAM_CORE_A_OUTPUT_SIZE_WIDTH
	{0xC86E, 0x02D8},  // CAM_CORE_A_OUTPUT_SIZE_HEIGHT
	{0xC870, 0x0014},  // CAM_CORE_A_RX_FIFO_TRIGGER_MARK
	{0xC858, 0x0003}, // CAM_CORE_A_COARSE_ITMIN
	{0xC8B8, 0x0004},  // CAM_OUTPUT_0_JPEG_CONTROL
	{0xC8AA, 0x0500},  // CAM_OUTPUT_0_IMAGE_WIDTH
	{0xC8AC, 0x02D0},  // CAM_OUTPUT_0_IMAGE_HEIGHT
	{0xC8AE, 0x0001},  // CAM_OUTPUT_0_OUTPUT_FORMAT
	REGLIST_8BIT_START_FLAG,
	{0x8404, 0x06},  // SEQ_CMD
	REGLIST_8BIT_STOP_FLAG, 
	DELAY_nMS_ARRAY(100),
	 {0xFFFF,0xFFFF},
};




/* 1920*1080: 1080P*/
static const struct regval_list module_1080p_regs[] = 
{
	//DELAY_nMS_ARRAY(10),
	//{0x001E, 0x0663},
	{0x098E, 0x1000}, 
	{0xC872, 0x0110}, 
	{0xC874, 0x0020}, 
	{0xC876, 0x06C9	}, 
	{0xC878, 0x0A47	},
	REGLIST_8BIT_START_FLAG,
	{0xDC0A, 0x06	}, 
	REGLIST_8BIT_STOP_FLAG,
	{0xDC1C, 0x2710	}, 
	{0xC8A4, 0x0A28	}, 
	{0xC8A6, 0x05BA	}, 
	{0xC8C0, 0x0780	}, 
	{0xC8C2, 0x0438},

	{0x098E, 0x843C}, 
	REGLIST_8BIT_START_FLAG,
	{0x843C, 0xff}, 
	{0x8404, 0x02}, 
	REGLIST_8BIT_STOP_FLAG,
	DELAY_nMS_ARRAY(10),
	 {0xFFFF,0xFFFF},
};


/* 2592X1944 QSXGA */
static const struct regval_list module_qsxga_regs[] = 
{
	#if 1
	{0x098E, 0x1000}, 
	{0xC872, 0x0010}, 
	{0xC874, 0x001C}, 
	{0xC876, 0x07AF}, 
	{0xC878, 0x0A43}, 
	REGLIST_8BIT_START_FLAG,
	{0xDC0A, 0x06	}, 
	REGLIST_8BIT_STOP_FLAG,
	{0xDC1C, 0x2710 }, 
	{0xC8A4, 0x0A28 }, 
	{0xC8A6, 0x07A0 },
	{0xC8C0, 0x0A20 },	// CAM_OUTPUT_1_IMAGE_WIDTH
	{0xC8C2, 0x0798},	// CAM_OUTPUT_1_IMAGE_HEIGHT

	{0x098E, 0x843C}, 
	REGLIST_8BIT_START_FLAG,
	{0x843C, 0xFF}, 
	{0x8404, 0x02}, 
	REGLIST_8BIT_STOP_FLAG,
	DELAY_nMS_ARRAY(10),
	{0xFFFF,0xFFFF},
	#endif
};



static const struct regval_list module_init_auto_focus[] =
{
	{0xFFFF,0xFFFF},
//  NULL
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

/* 1280*720 */
static struct camera_module_win_size module_win_720p = {
	.name             = "720P",
	.width            = WIDTH_720P,
	.height           = HEIGHT_720P,
	.win_regs         = module_720p_regs,
	
	.frame_rate_array = frame_rate_720p,
	.capture_only     = 0,
};

/* 800*600 */
static struct camera_module_win_size module_win_svga = {
	.name             = "SVGA",
	.width            = WIDTH_SVGA,
	.height           = HEIGHT_SVGA,
	.win_regs         = module_svga_regs,
	
	.frame_rate_array = frame_rate_svga,
	.capture_only     = 0,
};


/* 1920*1080 */
static struct camera_module_win_size module_win_1080p = {
	.name             = "1080P",
	.width            = WIDTH_1080P,
	.height           = HEIGHT_1080P,
	.win_regs         = module_1080p_regs,
	
	.frame_rate_array = frame_rate_1080p,
	.capture_only     = 0,
};

/* 2592*1944 */
static struct camera_module_win_size module_win_qsxga = {
	.name             = "QSXGA",
	.width            = WIDTH_QSXGA,
	.height           = HEIGHT_QSXGA,
	.win_regs         = module_qsxga_regs,
	
	.frame_rate_array = frame_rate_qsxga,
	.capture_only     = 1,
};

static struct camera_module_win_size *module_win_list[] = {
	&module_win_vga,
	&module_win_svga,
//	&module_win_uxga,
	&module_win_720p,
	&module_win_1080p,
	&module_win_qsxga,
};



static struct regval_list module_whitebance_auto_regs[]=
{
	REGLIST_8BIT_START_FLAG,
	{0xac01, 0xab}, 
	{0xac97, 0x80}, 
	{0xac98, 0x80}, 
	{0xac99, 0x80}, 
	{0xAC9a, 0x80}, 
	{0xAC9b, 0x80},
	{0xAC9c, 0x80},  
	REGLIST_8BIT_STOP_FLAG,
	{0xFFFF,0xFFFF},
};

/* Cloudy Colour Temperature : 6500K - 8000K  */
static struct regval_list module_whitebance_cloudy_regs[]=
{
	REGLIST_8BIT_START_FLAG,
	{0xac01, 0xeb}, 
	{0xac97, 0xf0}, 
	{0xac98, 0x80}, 
	{0xac99, 0x80}, 
	{0xAC9a, 0xf0}, 
	{0xAC9b, 0x80},
	{0xAC9c, 0x80},
	REGLIST_8BIT_STOP_FLAG,      
	{0xFFFF,0xFFFF},
};

/* ClearDay Colour Temperature : 5000K - 6500K  */
static struct regval_list module_whitebance_sunny_regs[]=
{
	REGLIST_8BIT_START_FLAG,
	{0xac01, 0xeb},
	{0xac97, 0xc0}, 
	{0xac98, 0x80}, 
	{0xac99, 0x80}, 
	{0xAC9a, 0xc0}, 
	{0xAC9b, 0x80},
	{0xAC9c, 0x80},     
	REGLIST_8BIT_STOP_FLAG,    
    {0xFFFF,0xFFFF},
};

/* Office Colour Temperature : 3500K - 5000K ,荧光灯 */
static struct regval_list module_whitebance_fluorescent_regs[]=
{
	REGLIST_8BIT_START_FLAG,
	{0xac01, 0xeb}, 
	{0xac97, 0xa0}, 
	{0xac98, 0x90}, 
	{0xac99, 0x80}, 
	{0xAC9a, 0xa0}, 
	{0xAC9b, 0x90},
	{0xAC9c, 0x80},        
	REGLIST_8BIT_STOP_FLAG, 
    {0xFFFF,0xFFFF},
};

/* Home Colour Temperature : 2500K - 3500K ，白炽灯 */
static struct regval_list module_whitebance_incandescent_regs[]=
{
	REGLIST_8BIT_START_FLAG,
	{0xac01, 0xeb}, 
	{0xac97, 0x80}, 
	{0xac98, 0x80}, 
	{0xac99, 0xb0}, 
	{0xAC9a, 0x80}, 
	{0xAC9b, 0x80},
	{0xAC9c, 0xb0},  
	REGLIST_8BIT_STOP_FLAG,   
	{0xFFFF,0xFFFF},
};


static struct regval_list module_scene_auto_regs[] =
{
	{0x337E, 0x0000},
	{0x098E, 0xA401},		// [AE_BASETARGET]   	
	REGLIST_8BIT_START_FLAG,	   	
    {0xA401, 0x00},                      
	//{0xA805, 0x04},		// [SEQ_CMD]       
	{0xA409, 0x27},     //                 
    //{0x8404, 0x06},      //delete for decrease capture time
    REGLIST_8BIT_STOP_FLAG,   
	{0xFFFF,0xFFFF},
};

/*
 * The exposure target setttings
 */
static struct regval_list module_exp_comp_neg4_regs[] = {
	REGLIST_8BIT_START_FLAG,
	{0xA409,0x18},
	REGLIST_8BIT_STOP_FLAG,  
	{0xFFFF,0xFFFF},
};

static struct regval_list module_exp_comp_neg3_regs[] = {
	REGLIST_8BIT_START_FLAG,
	{0xA409,0x20},
	REGLIST_8BIT_STOP_FLAG,  
	{0xFFFF,0xFFFF},
};

static struct regval_list module_exp_comp_neg2_regs[] = {
	
	REGLIST_8BIT_START_FLAG,
	{0xA409, 0x28},
	REGLIST_8BIT_STOP_FLAG,  
	{0xFFFF,0xFFFF},
};

static struct regval_list module_exp_comp_neg1_regs[] = {
	REGLIST_8BIT_START_FLAG,
	{0xA409,0x30},
	REGLIST_8BIT_STOP_FLAG,  
	{0xFFFF,0xFFFF},
};

static struct regval_list module_exp_comp_zero_regs[] = {
	REGLIST_8BIT_START_FLAG,
	{0xA409, 0x38},
	REGLIST_8BIT_STOP_FLAG,   
	{0xFFFF,0xFFFF},
};

static struct regval_list module_exp_comp_pos1_regs[] = {
	REGLIST_8BIT_START_FLAG,
	{0xA409,0x40},
	REGLIST_8BIT_STOP_FLAG,  
	{0xFFFF,0xFFFF},
};

static struct regval_list module_exp_comp_pos2_regs[] = {
	REGLIST_8BIT_START_FLAG,
	{0xA409,0x48},
	REGLIST_8BIT_STOP_FLAG,  
	{0xFFFF,0xFFFF},
};

static struct regval_list module_exp_comp_pos3_regs[] = {
    
	REGLIST_8BIT_START_FLAG,
	{0xA409,0x50},
	REGLIST_8BIT_STOP_FLAG,  
	{0xFFFF,0xFFFF},
};

static struct regval_list module_exp_comp_pos4_regs[] = {
	REGLIST_8BIT_START_FLAG,
	{0xA409, 0x58},
	REGLIST_8BIT_STOP_FLAG,  
	{0xFFFF,0xFFFF},
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
	{	.id   = V4L2_CID_FLASH_LED_MODE, 
		.max  = 3,
		.mask = 0x0,
		.def  = 0,},
	 {
	.id = V4L2_CID_POWER_LINE_FREQUENCY, 
	.max = V4L2_CID_POWER_LINE_FREQUENCY_AUTO, 
	.mask = 0x0,
	.def = V4L2_CID_POWER_LINE_FREQUENCY_AUTO,},
};


#endif /* __MODULE_DIFF_H__ */
