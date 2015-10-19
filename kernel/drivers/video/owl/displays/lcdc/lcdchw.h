/*
 * linux/drivers/video/owl/dss/lcdchw.h
 *
 * Copyright (C) 2011 Actions
 * Author: Hui Wang <wanghui@actions-semi.com>
 *
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

#ifndef __ASOC_LCDCHW_REG_H
#define __ASOC_LCDCHW_REG_H


#define ATM7059A 

#ifdef ATM9009A
    #define    LCDCHW_REG_MEM_BASE                                                  0xE02A0000
    #define    LCDCHW_REG_MEM_END                                                   0xE02A020c
    
    #define    LCDCHW_CTL                                                           (0x0000)
	#define    LCDCHW_SIZE                                                          (0x0004)
	#define    LCDCHW_STATUS                                                        (0x0008)
	#define    LCDCHW_TIM0                                                          (0x000C)
	#define    LCDCHW_TIM1                                                          (0x0010)
	#define    LCDCHW_TIM2                                                          (0x0014)
	#define    LCDCHW_COLOR                                                         (0x0018)
	#define    LCDCHW_IMG_XPOS                                                      (0x001C)
	#define    LCDCHW_IMG_YPOS                                                      (0x0020)
	#define    LCDCHW_LVDS_CTL                                                      (0x0200)
	#define    LCDCHW_LVDS_ALG_CTL0                                                 (0x0204)
	#define    LCDCHW_LVDS_DEBUG                                                    (0x0208)
			
#endif


#ifdef ATM7059A
	#define    LCDCHW_REG_MEM_BASE                                                  0xB02A0000
    #define    LCDCHW_REG_MEM_END                                                   0xB02A020C
    
    #define    LCDCHW_CTL                                                           (0x0000)
	#define    LCDCHW_SIZE                                                          (0x0004)
	#define    LCDCHW_STATUS                                                        (0x0008)
	#define    LCDCHW_TIM0                                                          (0x000C)
	#define    LCDCHW_TIM1                                                          (0x0010)
	#define    LCDCHW_TIM2                                                          (0x0014)
	#define    LCDCHW_COLOR                                                         (0x0018)
	#define    LCDCHW_CPU_CTL                                                       (0x001c)
    #define    LCDCHW_CPU_CMD                                                       (0x0020)
	#define    LCDCHW_IMG_XPOS                                                      (0x002c)
	#define    LCDCHW_IMG_YPOS                                                      (0x0030)
	#define    LCDCHW_LVDS_CTL                                                      (0x0200)
	#define    LCDCHW_LVDS_ALG_CTL0                                                 (0x0204)
	#define    LCDCHW_LVDS_DEBUG                                                    (0x0208)

#endif

#ifdef ATM7039C
	#define    LCDCHW_REG_MEM_BASE                                                  0xB02A0000
    #define    LCDCHW_REG_MEM_END                                                   0xB02A020C
    
    #define    LCDCHW_CTL                                                           (0x0000)
	#define    LCDCHW_SIZE                                                          (0x0004)
	#define    LCDCHW_STATUS                                                        (0x0008)
	#define    LCDCHW_TIM0                                                          (0x000C)
	#define    LCDCHW_TIM1                                                          (0x0010)
	#define    LCDCHW_TIM2                                                          (0x0014)
	#define    LCDCHW_COLOR                                                         (0x0018)
	#define    LCDCHW_CPU_CTL                                                       (0x001c)
    #define    LCDCHW_CPU_CMD                                                       (0x0020)
	#define    LCDCHW_IMG_XPOS                                                      (0x002c)
	#define    LCDCHW_IMG_YPOS                                                      (0x0030)
	#define    LCDCHW_LVDS_CTL                                                      (0x0200)
	#define    LCDCHW_LVDS_ALG_CTL0                                                 (0x0204)
	#define    LCDCHW_LVDS_DEBUG                                                    (0x0208)
#endif

#endif
