/*
 * linux/drivers/video/owl/dss/de_atm9009.h
 * 
 * NOTE: SHOULD only be included by de_atm9009.c
 *
 * Copyright (C) 2014 Actions
 * Author: lipeng<lipeng@actions-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DE_ATM9009_H_
#define _DE_ATM9009_H_

#include "de.h"


/*================================================================
 *           Definition of registers and bit position
 *==============================================================*/


/* 
 * DE common registers
 */
#define DE_IRQENABLE				0x0000
#define DE_IRQSTATUS				0x0004
#define DE_MMU_EN				0x0008 
#define DE_MMU_BASE				0x000c
#define DE_QOS					0x0014
#define DE_MAX_OUTSTANDING			0x0018

#define DE_HIST(n)				(0x0020 + (n) * 4)
#define DE_TOTAL(n)				0x00a0

#define DE_PATH_BASE				0x0100 

#define DE_PATH_CTL(n)				(DE_PATH_BASE + (n) * 0x100 + 0x000)
 #define DE_PATH_VIDEO_ENABLE_BEGIN_BIT		0
 #define DE_PATH_CTL_IYUV_QEN_BIT		13
 #define DE_PATH_CTL_YUV_FMT_BIT		12
 #define DE_PATH_CTL_ILACE_BIT			9

#define DE_PATH_FCR(n)				(DE_PATH_BASE + (n) * 0x100 + 0x004)
 #define DE_PATH_FCR_BIT			0

#define DE_PATH_EN(n)				(DE_PATH_BASE + (n) * 0x100 + 0x008)
 #define DE_PATH_ENABLE_BIT			0

#define DE_PATH_BK(n)				(DE_PATH_BASE + (n) * 0x100 + 0x00c)

#define DE_PATH_SIZE(n)				(DE_PATH_BASE + (n) * 0x100 + 0x010)
 #define DE_PATH_SIZE_WIDTH			(1 << 12)
 #define DE_PATH_SIZE_HEIGHT			(1 << 12)
 #define DE_PATH_SIZE_WIDTH_BEGIN_BIT		0
 #define DE_PATH_SIZE_WIDTH_END_BIT		11
 #define DE_PATH_SIZE_HEIGHT_BEGIN_BIT		16
 #define DE_PATH_SIZE_HEIGHT_END_BIT		27

#define DE_PATH_GAMMA_IDX(n)			(DE_PATH_BASE + (n) * 0x100 + 0x040)
#define DE_PATH_GAMMA_RAM(n)			(DE_PATH_BASE + (n) * 0x100 + 0x044)

#define DE_PATH_DITHER				(0x0250)
#define DE_PATH_DITHER_ENABLE_BIT		0

/* DE overlay registers */
#define DE_OVL_BASE				0x0400

#define DE_OVL_CFG(n)				(DE_OVL_BASE + (n) * 0x100  +  0x0000)
 #define DE_OVL_CFG_FLIP_BIT			20
 #define DE_OVL_CFG_FMT_BEGIN_BIT		0         
 #define DE_OVL_CFG_FMT_END_BIT			3

#define DE_OVL_ISIZE(n)				(DE_OVL_BASE + (n) * 0x100  +  0x0004)
#define DE_OVL_OSIZE(n)				(DE_OVL_BASE + (n) * 0x100  +  0x0008)
#define DE_OVL_COOR(m, n)			(DE_PATH_BASE + (m) * 0x100 + 0x020 + (n) * 0x4)

#define DE_OVL_SR(n)				(DE_OVL_BASE + (n) * 0x100  +  0x000c)

#define DE_OVL_SCOEF0(n)			(DE_OVL_BASE + (n) * 0x100  +  0x0010)
#define DE_OVL_SCOEF1(n)			(DE_OVL_BASE + (n) * 0x100  +  0x0014)
#define DE_OVL_SCOEF2(n)			(DE_OVL_BASE + (n) * 0x100  +  0x0018)
#define DE_OVL_SCOEF3(n)			(DE_OVL_BASE + (n) * 0x100  +  0x001c)
#define DE_OVL_SCOEF4(n)			(DE_OVL_BASE + (n) * 0x100  +  0x0020)
#define DE_OVL_SCOEF5(n)			(DE_OVL_BASE + (n) * 0x100  +  0x0024)
#define DE_OVL_SCOEF6(n)			(DE_OVL_BASE + (n) * 0x100  +  0x0028)
#define DE_OVL_SCOEF7(n)			(DE_OVL_BASE + (n) * 0x100  +  0x002c)

#define DE_OVL_BA0(n)				(DE_OVL_BASE + (n) * 0x100  +  0x0030)
#define DE_OVL_BA1UV(n)				(DE_OVL_BASE + (n) * 0x100  +  0x0034)
#define DE_OVL_BA2V(n)				(DE_OVL_BASE + (n) * 0x100  +  0x0038)    
#define DE_OVL_3D_RIGHT_BA0(n)			(DE_OVL_BASE + (n) * 0x100  +  0x003C)
#define DE_OVL_3D_RIGHT_BA1UV(n)		(DE_OVL_BASE + (n) * 0x100  +  0x0040)
#define DE_OVL_3D_RIGHT_BA2V(n)			(DE_OVL_BASE + (n) * 0x100  +  0x0044)
#define DE_OVL_STR(n)				(DE_OVL_BASE + (n) * 0x100  +  0x0048)
#define DE_OVL_CRITICAL_CFG(n)			(DE_OVL_BASE + (n) * 0x100  +  0x004c)
#define DE_OVL_REMAPPING(n)			(DE_OVL_BASE + (n) * 0x100  +  0x0050)

#define DE_OVL_CSC(n)				(DE_OVL_BASE + (n) * 0x100  +  0x0054)
 #define DE_OVL_CSC_CON_BEGIN_BIT		0
 #define DE_OVL_CSC_CON_END_BIT			3
 #define DE_OVL_CSC_STA_BEGIN_BIT		4
 #define DE_OVL_CSC_STA_END_BIT			7
 #define DE_OVL_CSC_BRI_BEGIN_BIT		8
 #define DE_OVL_CSC_BRI_END_BIT			15
 #define DE_OVL_CSC_BYPASS_BIT			24

#define DE_OVL_ALPHA_CFG(m, n)			(DE_PATH_BASE + (m) * 0x100 + 0x030 + (n) * 0x4)
 #define DE_OVL_ALPHA_CFG_VALUE_BEGIN_BIT	0
 #define DE_OVL_ALPHA_CFG_VALUE_END_BIT		7    
 #define DE_OVL_ALPHA_CFG_ENABLE_BEGIN_BIT	8
 #define DE_OVL_ALPHA_CFG_ENABLE_END_BIT	9




#define DE_OUTPUT_CON                   	0x1000
 #define DE_OUTPUT_PATH2_DEVICE_BEGIN_BIT	0
 #define DE_OUTPUT_PATH2_DEVICE_END_BIT		1

#define DE_OUTPUT_STAT				0x1004

#define DE_WB_CON				0x1008
#define DE_WB_ADDR				0x100c
#define DE_WB_CNT				0x1010

#endif
