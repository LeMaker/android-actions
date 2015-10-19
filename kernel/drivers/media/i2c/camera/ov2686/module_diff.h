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


#define CAMERA_MODULE_NAME 		"ov2686"
#define CAMERA_MODULE_PID		0x2685
#define VERSION(pid, ver) 		((pid<<8)|(ver&0xFF))

#define MODULE_I2C_REAL_ADDRESS  (0x78 >> 1)
#define MODULE_I2C_REG_ADDRESS		(0x78 >> 1)

#define I2C_REGS_WIDTH			2
#define I2C_DATA_WIDTH			1
//#define IS_FRONT_OR_REAR        0


#define DEFAULT_VSYNC_ACTIVE_LEVEL		V4L2_MBUS_VSYNC_ACTIVE_LOW
#define DEFAULT_POWER_LINE_FREQUENCY	V4L2_CID_POWER_LINE_FREQUENCY_50HZ
#define DEFAULT_PCLK_SAMPLE_EDGE      V4L2_MBUS_PCLK_SAMPLE_RISING
//V4L2_MBUS_PCLK_SAMPLE_FALLING



#if 0
#define PID						0x00 /* Product ID Number */
#else
#define PIDH					0x300a /* Product ID Number H byte */
#define PIDL					0x300b /* Product ID Number L byte */
#endif

#define OUTTO_SENSO_CLOCK 		24000000


#define MODULE_DEFAULT_WIDTH	WIDTH_VGA
#define MODULE_DEFAULT_HEIGHT	HEIGHT_VGA
#define MODULE_MAX_WIDTH		WIDTH_QSXGA
#define MODULE_MAX_HEIGHT		HEIGHT_QSXGA


#define AHEAD_LINE_NUM			15    //10行 = 50次循环
#define DROP_NUM_CAPTURE		2
#define DROP_NUM_PREVIEW		4
static unsigned int frame_rate_vga[]   = {30,};
static unsigned int frame_rate_720p[]  = {30,};
static unsigned int frame_rate_uxga[] = {15,};



static const struct regval_list module_init_regs[] =
{ 
	//																											 
	//OV2686 setting version History																			 
	//;;1. 05/17/2014 V00																						 
	// based on OV2686_AA_00_02_00.ovt																			 
	// 1. Initial release																						 
	//;;1. 6/11/2014 V02a																						 
	// based on OV2686_AA_00_02_00.ovt																			 
	// 1. update CMX,AWB,LENC,DPC settings																		 
	{0x0103,0x01},//software reset																				 
	{0x3000,0x03},//Y[9:8] output																				 
	{0x3001,0xff},//Y[7:0] output																				 
	{0x3002,0x1a},//Vsync output, FSIN input																	 
	{0x3011,0x00},//Pad 																						 
	{0x301d,0xf0},//sclk_fc, sclk_grp, sclk_bist, daclk_sel0													 
	{0x3020,0x00},//output raw																					 
	{0x3021,0x23},//software standby enter at l_blk, frex_ef_sel, cen_blobal_o									 
	{0x3082,0x2c},//PLL 																						 
	{0x3083,0x00},																								 
	{0x3084,0x07},																								 
	{0x3085,0x03},																								 
	{0x3086,0x01},																								 
	{0x3087,0x00},																								 
	{0x3106,0x01},//PLL 																						 
	{0x3501,0x26},//exposure M																					 
	{0x3502,0x40},//exposure L																					 
	{0x3503,0x03},//gain delay 1 frame, vts auto, agc off, aec off												 
	{0x350b,0x36},//gain L																						 
	{0x3600,0xb4},//Analog Control																				 
	{0x3603,0x35},																								 
	{0x3604,0x24},																								 
	{0x3605,0x00},																								 
	{0x3620,0x25},																								 
	{0x3621,0x37},																								 
	{0x3622,0x23},																								 
	{0x3628,0x10},//Analog Control																				 
	{0x3701,0x64},//Sensor Conrol																				 
	{0x3705,0x3c},																								 
	{0x370a,0x23},																								 
	{0x370c,0x50},																								 
	{0x370d,0xc0},																								 
	{0x3717,0x58},																								 
	{0x3718,0x80},																								 
	{0x3720,0x00},																								 
	{0x3721,0x00},																								 
	{0x3722,0x00},																								 
	{0x3723,0x00},																								 
	{0x3738,0x00},//Sensor Control																				 
	{0x3781,0x80},//PSRAM control																				 
	{0x3789,0x60},//PSRAM control																				 
	{0x3800,0x00},//Timing, x start H																			 
	{0x3801,0x00},//x start L																					 
	{0x3802,0x00},//y start H																					 
	{0x3803,0x00},//y start L																					 
	{0x3804,0x06},//x end H 																					 
	{0x3805,0x4f},//x end L 																					 
	{0x3806,0x04},//y end H 																					 
	{0x3807,0xbf},//y end L 																					 
	{0x3808,0x03},//x output size H 																			 
	{0x3809,0x20},//x output size L 																			 
	{0x380a,0x02},//y output size H 																			 
	{0x380b,0x58},//y output size L 																			 
	{0x380c,0x06},//HTS H																						 
	{0x380d,0xac},//HTS L																						 
	{0x380e,0x02},//VTS H																						 
	{0x380f,0x84},//VTS L																						 
	{0x3810,0x00},//isp x win H 																				 
	{0x3811,0x04},//isp x win L 																				 
	{0x3812,0x00},//isp y win H 																				 
	{0x3813,0x04},//isp y win L 																				 
	{0x3814,0x31},//x inc																						 
	{0x3815,0x31},//y inc																						 
	{0x3819,0x04},//vsync_end_row L 																			 
	{0x3820,0xc2},//vsub48_blc, vflip_blc, vbinf																 
	{0x3821,0x01},//hbin																						 
	{0x3a06,0x00},//B50 H																						 
	{0x3a07,0xc2},//B50 L																						 
	{0x3a08,0x00},//B60 H																						 
	{0x3a09,0xA1},//B60 L																						 
	{0x3a0a,0x07},//max exp 50 H																				 
	{0x3a0b,0x94},//max exp 50 L																				 
	{0x3a0c,0x07},//max exp 60 H																				 
	{0x3a0d,0x94},//max exp 60 L																				 
	{0x3a0e,0x02},//VTS band 50 H																				 
	{0x3a0f,0x46},//VTS band 50 L																				 
	{0x3a10,0x02},//VTS band 60 H																				 
	{0x3a11,0x84},//VTS band 60 L																				 
	{0x4000,0x81},//avg_weight, mf_en																			 
	{0x4001,0x40},//format_trig_beh 																			 
	{0x4008,0x00},//bl_start																					 
	{0x4009,0x03},//bl_end																						 
	{0x4300,0x31},//YUV422	
	{0x4301,0x41},
	{0x430e,0x20},//no swap, no bypass																			 
	{0x4602,0x02},//Frame reset enable																			 
	{0x5000,0xff},//lenc_en, awb_gain_en, lcd_en, avg_en, dgc_en, bc_en, wc_en, blc-en							 
	{0x5001,0x05},//avg_sel after LCD																			 
	{0x5002,0x32},//dpc_href_s, sof_sel, bias_plus																 
	{0x5003,0x04},//bias_man																					 
	{0x5004,0xff},//uv_dns_en, rng_dns_en, gamma_en, cmxen, cip_en, raw_dns_en, stretch_en, awb_en				 
	{0x5005,0x12},//sde_en, rgb2yuv_en																			 
	{0x5180,0xf4},//AWB 																						 
	{0x5181,0x11},																								 
	{0x5182,0x41},																								 
	{0x5183,0x42},																								 
	{0x5184,0x6e},																								 
	{0x5185,0x56},																								 
	{0x5186,0xb4},																								 
	{0x5187,0xb2},																								 
	{0x5188,0x08},																								 
	{0x5189,0x0e},																								 
	{0x518a,0x0e},																								 
	{0x518b,0x46},																								 
	{0x518c,0x38},																								 
	{0x518d,0xf8},																								 
	{0x518e,0x04},																								 
	{0x518f,0x7f},																								 
	{0x5190,0x40},																								 
	{0x5191,0x5f},																								 
	{0x5192,0x40},																								 
	{0x5193,0xff},																								 
	{0x5194,0x40},																								 
	{0x5195,0x07},																								 
	{0x5196,0x04},																								 
	{0x5197,0x04},																								 
	{0x5198,0x00},																								 
	{0x5199,0x05},																								 
	{0x519a,0xd2},																								 
	{0x519b,0x04},//AWB 																						 
	{0x5200,0x09},//stretch minimum value is 3096, auto 														 
	{0x5201,0x00},//stretch min low level																		 
	{0x5202,0x06},//stretch max low level																		 
	{0x5203,0x20},//stretch min high level																		 
	{0x5204,0x41},//stretch step2, stretch step1																 
	{0x5205,0x16},//stretch current low level																	 
	{0x5206,0x00},//stretch current high level L																 
	{0x5207,0x05},//stretch current high level H																 
	{0x520b,0x30},//stretch_thres1 L																			 
	{0x520c,0x75},//stretch_thres1 M																			 
	{0x520d,0x00},//stretch_thres1 H																			 
	{0x520e,0x30},//stretch_thres2 L																			 
	{0x520f,0x75},//stretch_thres2 M																			 
	{0x5210,0x00},//stretch_thres2 H																			 
	{0x5280,0x14},//raw_DNS, m_nNoiseYslop = 5																	 
	{0x5281,0x02},//m_nNoiseList[0] 																			 
	{0x5282,0x02},//m_nNoiseList[1] 																			 
	{0x5283,0x04},//m_nNoiseList[2] 																			 
	{0x5284,0x06},//m_nNoiseList[3] 																			 
	{0x5285,0x08},//m_nNoiseList[4] 																			 
	{0x5286,0x0c},//m_nNoiseList[5] 																			 
	{0x5287,0x10},//m_nMaxEdgeThre																				 
	{0x5300,0xc5},//CIP, m_bColorEdgeEnable, m_bAntiAliasing, m_nDetailSlop=0, m_nNoiseYSlop=5					 
	{0x5301,0xa0},//m_nSharpenSlope=1, m_nGbGrShift=0															 
	{0x5302,0x06},//m_nNoiseList[0] 																			 
	{0x5303,0x0a},//m_nNoiseList[1] 																			 
	{0x5304,0x30},//m_nNoiseList[2] 																			 
	{0x5305,0x60},//m_nNoiseList[3] 																			 
	{0x5306,0x90},//m_nNoiseList[4] 																			 
	{0x5307,0xc0},//m_nNoiseList[5] 																			 
	{0x5308,0x82},//m_nMaxSharpenGain=8, m_nMinSharpenGain=2													 
	{0x5309,0x00},//m_nMinSharpen																				 
	{0x530a,0x26},//m_nMaxSharpen																				 
	{0x530b,0x02},//m_nMinDetail																				 
	{0x530c,0x02},//m_nMaxDetail																				 
	{0x530d,0x00},//m_nDetailRatioList[0]																		 
	{0x530e,0x0c},//m_nDetailRatioList[1]																		 
	{0x530f,0x14},//m_nDetailRatioList[2]																		 
	{0x5310,0x1a},//m_nSharpenNegEdgeRatio																		 
	{0x5311,0x20},//m_nClrEdgeShpT1 																			 
	{0x5312,0x80},//m_nClrEdgeShpT2 																			 
	{0x5313,0x4b},//m_nClrEdgeShpSlope																			 
	{0x5380,0x01},//nCCM_D[0][0] H																				 
	{0x5381,0x0c},//nCCM_D[0][0] L																				 
	{0x5382,0x00},//nCCM_D[0][1] H																				 
	{0x5383,0x16},//nCCM_D[0][1] L																				 
	{0x5384,0x00},//nCCM_D[1][0] H																				 
	{0x5385,0xb3},//nCCM_D[1][0] L																				 
	{0x5386,0x00},//nCCM_D[1][1] H																				 
	{0x5387,0x7e},//nCCM_D[1][1] L																				 
	{0x5388,0x00},//nCCM_D[2][0] H																				 
	{0x5389,0x07},//nCCM_D[2][0] L																				 
	{0x538a,0x01},//nCCM_D[2][1] H																				 
	{0x538b,0x35},//nCCM_D[2][1] L																				 
	{0x538c,0x00},//sign bit of nCCM_D[2][1], [2][0], [1][1], [1][0], [0][1], [0][0]							 
	{0x5400,0x0d},//Gamma, m_pCurveYlist[0] 																	 
	{0x5401,0x18},//m_pCurveYlist[1]																			 
	{0x5402,0x31},//m_pCurveYlist[2]																			 
	{0x5403,0x5a},//m_pCurveYlist[3]																			 
	{0x5404,0x65},//m_pCurveYlist[4]																			 
	{0x5405,0x6f},//m_pCurveYlist[5]																			 
	{0x5406,0x77},//m_pCurveYlist[6]																			 
	{0x5407,0x80},//m_pCurveYlist[7]																			 
	{0x5408,0x87},//m_pCurveYlist[8]																			 
	{0x5409,0x8f},//m_pCurveYlist[9]																			 
	{0x540a,0xa2},//m_pCurveYlist[10]																			 
	{0x540b,0xb2},//m_pCurveYlist[11]																			 
	{0x540c,0xcc},//m_pCurveYlist[12]																			 
	{0x540d,0xe4},//m_pCurveYlist[13]																			 
	{0x540e,0xf0},//m_pCurveYlist[14]																			 
	{0x540f,0xa0},//m_nMaxShadowHGain																			 
	{0x5410,0x6e},//m_nMidTondHGain 																			 
	{0x5411,0x06},//m_nHighLightGain																			 
	{0x5480,0x19},//RGB_DNS, m_nShadowExtraNoise = 12, m_bSmoothYEnable 										 
	{0x5481,0x00},//m_nNoiseYList[1], m_nNoiseYList[0]															 
	{0x5482,0x09},//m_nNoiseYList[3], m_nNoiseYList[2]															 
	{0x5483,0x12},//m_nNoiseYList[5], m_nNoiseYList[4]															 
	{0x5484,0x04},//m_nNoiseUVList[0]																			 
	{0x5485,0x06},//m_nNoiseUVList[1]																			 
	{0x5486,0x08},//m_nNoiseUVList[2]																			 
	{0x5487,0x0c},//m_nNoiseUVList[3]																			 
	{0x5488,0x10},//m_nNoiseUVList[4]																			 
	{0x5489,0x18},//m_nNoiseUVList[5]																			 
	{0x5500,0x02},//UV_DNS, m_nNoiseList[0] 																	 
	{0x5501,0x03},//m_nNoiseList[1] 																			 
	{0x5502,0x04},//m_nNoiseList[2] 																			 
	{0x5503,0x05},//m_nNoiseList[3] 																			 
	{0x5504,0x06},//m_nNoiseList[4] 																			 
	{0x5505,0x08},//m_nNoiseList[5] 																			 
	{0x5506,0x00},//m_nShadowExtraNoise = 0 																	 
	{0x5600,0x06},//SDE, saturation_en, contrast_en 															 
	{0x5603,0x40},//sat u																						 
	{0x5604,0x28},//sat v																						 
	{0x5609,0x20},//uvadjust_th1																				 
	{0x560a,0x60},//uvadjust_th2																				 
	{0x560b,0x00},//uvadjust_auto																				 
	{0x5780,0x3e},//DPC 																						 
	{0x5781,0x0f},																								 
	{0x5782,0x04},																								 
	{0x5783,0x02},																								 
	{0x5784,0x01},																								 
	{0x5785,0x01},																								 
	{0x5786,0x00},																								 
	{0x5787,0x04},																								 
	{0x5788,0x02},																								 
	{0x5789,0x00},																								 
	{0x578a,0x01},																								 
	{0x578b,0x02},																								 
	{0x578c,0x03},																								 
	{0x578d,0x03},																								 
	{0x578e,0x08},																								 
	{0x578f,0x0c},																								 
	{0x5790,0x08},																								 
	{0x5791,0x04},																								 
	{0x5792,0x00},																								 
	{0x5793,0x00},																								 
	{0x5794,0x03},//DPC 																						 
	{0x5800,0x03},//Lenc, red x H																				 
	{0x5801,0x14},//red x L 																					 
	{0x5802,0x02},//red y H 																					 
	{0x5803,0x64},//red y L 																					 
	{0x5804,0x1e},//red_a1																						 
	{0x5805,0x05},//red_a2																						 
	{0x5806,0x2a},//red_b1																						 
	{0x5807,0x05},//red_b2																						 
	{0x5808,0x03},//green x H																					 
	{0x5809,0x17},//green x L																					 
	{0x580a,0x02},//green y H																					 
	{0x580b,0x63},//green y L																					 
	{0x580c,0x1a},//green a1																					 
	{0x580d,0x05},//green a2																					 
	{0x580e,0x1f},//green b1																					 
	{0x580f,0x05},//green b2																					 
	{0x5810,0x03},//blue x H																					 
	{0x5811,0x0c},//blue x L																					 
	{0x5812,0x02},//blue y H																					 
	{0x5813,0x5e},//blue Y L																					 
	{0x5814,0x18},//blue a1 																					 
	{0x5815,0x05},//blue a2 																					 
	{0x5816,0x19},//blue b1 																					 
	{0x5817,0x05},//blue b2 																					 
	{0x5818,0x0d},//rst_seed, rnd_en, gcoef_en																	 
	{0x5819,0x40},//lenc_coef_th																				 
	{0x581a,0x04},//lenc_gain_thre1 																			 
	{0x581b,0x0c},//lenc_gain__thre2																			 
	{0x3106,0x21},//byp_arb 																					 
	{0x3784,0x08},																								 
	//AEC GC																								 
	{0x3a03,0x4c},//AEC H																						 
	{0x3a04,0x40},//AEC L																						 
	{0x3503,0x00},//AEC auto, AGC auto																			 
	{0x3a02,0x90},//50Hz, speed ratio = 0x10	
	{0x382D,0x03},
	{0x0100,0x01},//stream on																					 
   										  
  	
    ENDMARKER,
};




/* 800*600: SVGA */
static const struct regval_list module_svga_regs[] = 
{
	
	#if 1
	{0x3503,0x00},// AGC on, AEC on 				  
	{0x3086,0x01},// PLL							  
	{0x370a,0x23},// Sensor Conrol					  
	{0x3801,0x00},// x start L						  
	{0x3803,0x00},// y start L						  
	{0x3804,0x06},// x end H						  
	{0x3805,0x4f},// x end L						  
	{0x3806,0x04},// y end H						  
	{0x3807,0xbf},// y end L						  
	{0x3808,0x03},// x output size H				  
	{0x3809,0x20},// x output size L				  
	{0x380a,0x02},// y output size H				  
	{0x380b,0x58},// y output size L				  
	{0x380c,0x06},// HTS H							  
	{0x380d,0xac},// HTS L							  
	{0x380e,0x02},// VTS H							  
	{0x380f,0x84},// VTS L							  
	{0x3811,0x04},// isp x win L					  
	{0x3813,0x04},// isp y win L					  
	{0x3814,0x31},// x inc							  
	{0x3815,0x31},// y inc							  
	{0x3820,0xc2},// vsub48_blc, vflip_blc, vbinf	  
	{0x3821,0x01},// hbin							  
	{0x3a07,0xc2},// B50 L							  
	{0x3a09,0xa1},// B60 L							  
	{0x3a0a,0x07},// max exp 50 H					  
	{0x3a0b,0x94},// max exp 50 L					  
	{0x3a0c,0x07},// max exp 60 H					  
	{0x3a0d,0x94},// max exp 60 L					  
	{0x3a0e,0x02},// VTS band 50 H					  
	{0x3a0f,0x46},// VTS band 50 L					  
	{0x3a10,0x02},// VTS band 60 H					  
	{0x3a11,0x84},// VTS band 60 L					  
	{0x4008,0x00},// bl_start						  
	{0x4009,0x03},// bl_end 	
	#endif
	#if 0
	{0x3503,0x00},// AGC on, AEC on 			   
	{0x3086,0x03},// PLL						   
	{0x370a,0x23},// Sensor Conrol				   
	{0x3801,0x00},// x start L					   
	{0x3803,0x00},// y start L					   
	{0x3804,0x06},// x end H					   
	{0x3805,0x4f},// x end L					   
	{0x3806,0x04},// y end H					   
	{0x3807,0xbf},// y end L					   
	{0x3808,0x03},// x output size H			   
	{0x3809,0x20},// x output size L			   
	{0x380a,0x02},// y output size H			   
	{0x380b,0x58},// y output size L			   
	{0x380c,0x06},// HTS H						   
	{0x380d,0xac},// HTS L						   
	{0x380e,0x02},// VTS H						   
	{0x380f,0x84},// VTS L						   
	{0x3811,0x04},// isp x win L				   
	{0x3813,0x04},// isp y win L				   
	{0x3814,0x31},// x inc						   
	{0x3815,0x31},// y inc						   
	{0x3820,0xc2},// vsub48_blc, vflip_blc, vbinf  
	{0x3821,0x01},// hbin						   
	{0x3a07,0xc2},// B50 L						   
	{0x3a09,0xa1},// B60 L						   
	{0x3a0a,0x07},// max exp 50 H				   
	{0x3a0b,0x94},// max exp 50 L				   
	{0x3a0c,0x07},// max exp 60 H				   
	{0x3a0d,0x94},// max exp 60 L				   
	{0x3a0e,0x02},// VTS band 50 H				   
	{0x3a0f,0x46},// VTS band 50 L				   
	{0x3a10,0x02},// VTS band 60 H				   
	{0x3a11,0x84},// VTS band 60 L				   
	{0x4008,0x00},// bl_start					   
	{0x4009,0x03},// bl_end 
	#endif
    ENDMARKER,
};



/* 1280*720: 720P*/
static const struct regval_list module_720p_regs[] = 
{
	
	{0x3503,0x00},// AGC on, AEC on 		
	{0x3086,0x01},// PLL					
	{0x370a,0x21},// Sensor Conrol			
	{0x3801,0xa0},// x start L				
	{0x3803,0xf2},// y start L				
	{0x3804,0x05},// x end H				
	{0x3805,0xaf},// x end L				
	{0x3806,0x03},// y end H				
	{0x3807,0xcd},// y end L				
	{0x3808,0x05},// x output size H		
	{0x3809,0x00},// x output size L		
	{0x380a,0x02},// y output size H		
	{0x380b,0xd0},// y output size L		
	{0x380c,0x05},// HTS H					
	{0x380d,0xa6},// HTS L					
	{0x380e,0x02},// VTS H					
	{0x380f,0xf8},// VTS L					
	{0x3811,0x08},// isp x win L			
	{0x3813,0x06},// isp y win L			
	{0x3814,0x11},// x inc					
	{0x3815,0x11},// y inc					
	{0x3820,0xc0},// vsub48_blc, vflip_blc	
	{0x3821,0x00},// hbin off				
	{0x3a07,0xe4},// B50 L					
	{0x3a09,0xbd},// B60 L					
	{0x3a0a,0x0e},// max exp 50 H			
	{0x3a0b,0x40},// max exp 50 L			
	{0x3a0c,0x17},// max exp 60 H			
	{0x3a0d,0xc0},// max exp 60 L			
	{0x3a0e,0x02},// VTS band 50 H			
	{0x3a0f,0xac},// VTS band 50 L			
	{0x3a10,0x02},// VTS band 60 H			
	{0x3a11,0xf8},// VTS band 60 L			
	{0x4008,0x02},// bl_start				
	{0x4009,0x09},// bl_end 				
							  
	 ENDMARKER,
};



/* 1600X1200 UXGA */
static const struct regval_list module_uxga_regs[] = 
{
	{0x3503,0x03},// AEC off, AGC off		
	{0x3086,0x01},// PLL					
	{0x370a,0x21},// Sensor Conrol			
	{0x3801,0x00},// x start L				
	{0x3803,0x00},// y start L				
	{0x3804,0x06},// x end H				
	{0x3805,0x4f},// x end L				
	{0x3806,0x04},// y end H				
	{0x3807,0xbf},// y end L				
	{0x3808,0x06},// x output size H		
	{0x3809,0x40},// x output size L		
	{0x380a,0x04},// y output size H		
	{0x380b,0xb0},// y output size L		
	{0x380c,0x06},// HTS H					
	{0x380d,0xa4},// HTS L					
	{0x380e,0x05},// VTS H					
	{0x380f,0x0e},// VTS L					
	{0x3811,0x08},// isp x win L			
	{0x3813,0x08},// isp y win L			
	{0x3814,0x11},// x inc					
	{0x3815,0x11},// y inc					
	{0x3820,0xc0},// vsub48_blc, vflip_blc	
	{0x3821,0x00},// hbin off				
	{0x3a07,0xc2},// B50 L					
	{0x3a09,0xa1},// B60 L					
	{0x3a0a,0x0f},// max exp 50 H			
	{0x3a0b,0x28},// max exp 50 L			
	{0x3a0c,0x0f},// max exp 60 H			
	{0x3a0d,0x28},// max exp 60 L			
	{0x3a0e,0x04},// VTS band 50 H			
	{0x3a0f,0x8c},// VTS band 50 L			
	{0x3a10,0x05},// VTS band 60 H			
	{0x3a11,0x08},// VTS band 60 L			
	{0x4008,0x02},// bl_start				
	{0x4009,0x09},// bl_end 				
							   
     ENDMARKER,
};




static const struct regval_list module_init_auto_focus[] =
{
//  NULL
ENDMARKER,

};

/* 640*480 */
static struct camera_module_win_size module_win_svga = {
	.name             = "SVGA",
	.width            = WIDTH_SVGA,
	.height           = HEIGHT_SVGA,
	.win_regs         = module_svga_regs,
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

/* 1600*1200 */
static struct camera_module_win_size module_win_uxga = {
	.name             = "UXGA",
	.width            = WIDTH_UXGA,
	.height           = HEIGHT_UXGA,
	.win_regs         = module_uxga_regs,
	
	.frame_rate_array = frame_rate_uxga,
	.capture_only     = 1,
};



static struct camera_module_win_size *module_win_list[] = {
	&module_win_svga,
	&module_win_720p,
	&module_win_uxga,
};



static struct regval_list module_whitebance_auto_regs[]=
{
	
	{0x3208,0x00},// start group 1	  
	{0x5180,0xf4},					  
	{0x3208,0x10},// end group 1	  
	{0x3208,0xa0},// launch group 1   
	
	ENDMARKER,
};

/* Cloudy Colour Temperature : 6500K - 8000K  */
static struct regval_list module_whitebance_cloudy_regs[]=
{
	
	
	
	{0x3208,0x00},// start group 1	   
	{0x5180,0xf6},//				   
	{0x5195,0x07},//R Gain			   
	{0x5196,0xdc},//				   
	{0x5197,0x04},//G Gain			   
	{0x5198,0x00},//				   
	{0x5199,0x05},//B Gain			   
	{0x519a,0xd3},//				   
	{0x3208,0x10},// end group 1	   
	{0x3208,0xa0},// launch group 1    
	
	ENDMARKER,
};

/* ClearDay Colour Temperature : 5000K - 6500K  */
static struct regval_list module_whitebance_sunny_regs[]=
{
	
	{0x3208,0x00},// start group 1 
	{0x5180,0xf6},//			   
	{0x5195,0x07},//R Gain		   
	{0x5196,0x9c},//			   
	{0x5197,0x04},//G Gain		   
	{0x5198,0x00},//			   
	{0x5199,0x05},//B Gain		   
	{0x519a,0xf3},//			   
	{0x3208,0x10},// end group 1   
	{0x3208,0xa0},// launch group 1
    ENDMARKER,
};

/* Office Colour Temperature : 3500K - 5000K ,荧光灯 */
static struct regval_list module_whitebance_fluorescent_regs[]=
{
	
	{0x3208,0x00},// start group 1	
	{0x5180,0xf6},//				
	{0x5195,0x06},//R Gain			
	{0x5196,0xb8},//				
	{0x5197,0x04},//G Gain			
	{0x5198,0x00},//				
	{0x5199,0x06},//B Gain			
	{0x519a,0x5f},//				
	{0x3208,0x10},// end group 1	
	{0x3208,0xa0},// launch group 1 
	
    ENDMARKER,
};

/* Home Colour Temperature : 2500K - 3500K ，白炽灯 */
static struct regval_list module_whitebance_incandescent_regs[]=
{
	
	{0x3208,0x00},// start group 1	 
	{0x5180,0xf6},//				 
	{0x5195,0x04},//R Gain			 
	{0x5196,0x90},//				 
	{0x5197,0x04},//G Gain			 
	{0x5198,0x00},//				 
	{0x5199,0x09},//B Gain			 
	{0x519a,0x20},//				 
	{0x3208,0x10},// end group 1	 
	{0x3208,0xa0},// launch group 1  
	
	ENDMARKER,
};


static struct regval_list module_scene_auto_regs[] =
{
	ENDMARKER,
};

/*
 * The exposure target setttings
 */

static struct regval_list module_exp_comp_pos4_regs[] = {
	{0x3a03,0x66},	 
	{0x3a04,0x62},	
	ENDMARKER,

};


static struct regval_list module_exp_comp_pos3_regs[] = {
	{0x3a03,0x62},	 
	{0x3a04,0x58},	
	ENDMARKER,

};

static struct regval_list module_exp_comp_pos2_regs[] = {
	{0x3a03,0x5a},	 
	{0x3a04,0x50},
	ENDMARKER,

};

static struct regval_list module_exp_comp_pos1_regs[] = {
	{0x3a03,0x54},	 
	{0x3a04,0x48},	
	ENDMARKER,

};

static struct regval_list module_exp_comp_zero_regs[] = {
	{0x3a03,0x4e},	 
	{0x3a04,0x40},  
	ENDMARKER,
};

static struct regval_list module_exp_comp_neg1_regs[] = {
	{0x3a03,0x44},	 
	{0x3a04,0x38},
	ENDMARKER,
};

static struct regval_list module_exp_comp_neg2_regs[] = {
	{0x3a03,0x3c},	 
	{0x3a04,0x30},
	ENDMARKER,
};

static struct regval_list module_exp_comp_neg3_regs[] = {
	{0x3a03,0x34},	 
	{0x3a04,0x28},
	ENDMARKER,
};
static struct regval_list module_exp_comp_neg4_regs[] = {
	{0x3a03,0x28},	 
	{0x3a04,0x24},
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

    {	.id = V4L2_CID_FLASH_LED_MODE, 
		.max = 3,
		.mask = 0x0,
		.def = 0,},  
	 {
	.id = V4L2_CID_POWER_LINE_FREQUENCY, 
	.max = V4L2_CID_POWER_LINE_FREQUENCY_AUTO, 
	.mask = 0x0,
	.def = V4L2_CID_POWER_LINE_FREQUENCY_AUTO,},
};


#endif /* __MODULE_DIFF_H__ */
