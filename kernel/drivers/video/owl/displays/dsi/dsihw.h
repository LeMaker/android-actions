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

#ifndef __OWL_DSIHW_H
#define __OWL_DSIHW_H

#include <linux/platform_device.h>
#include <video/owldss.h>


#define GL5206

#ifdef GL5209

	#define    DSIHW_REG_MEM_BASE                                                0xb0220000
	#define    DSIHW_REG_MEM_END                                                 0xb022008a
	
	#define     DSI_CTRL                                                          (0x0000)
	#define     DSI_SIZE                                                          (0x0004)
	#define     DSI_COLOR                                                         (0x0008)
	#define     DSI_VIDEO_CFG                                                     (0x000C)
	#define     DSI_RGBHT0                                                        (0x0010)
	#define     DSI_RGBHT1                                                        (0x0014)
	#define     DSI_RGBVT0                                                        (0x0018)
	#define     DSI_RGBVT1                                                        (0x001c)
	#define     DSI_TIMEOUT                                                       (0x0020)
	#define     DSI_TR_STA                                                        (0x0024)
	#define     DSI_INT_EN                                                        (0x0028)
	#define     DSI_ERROR_REPORT                                                  (0x002c)
	#define     DSI_FIFO_ODAT                                                     (0x0030)
	#define     DSI_FIFO_IDAT                                                     (0x0034)
	#define     DSI_IPACK                                                         (0x0038)
	#define     DSI_PACK_CFG                                                      (0x0040)
	#define     DSI_PACK_HEADER                                                   (0x0044)
	#define     DSI_TX_TRIGGER                                                    (0x0048)
	#define     DSI_RX_TRIGGER                                                    (0x004c)
	#define     DSI_LANE_CTRL                                                     (0x0050)
	#define     DSI_LANE_STA                                                      (0x0054)
	#define     DSI_PHY_T0                                                        (0x0060)
	#define     DSI_PHY_T1                                                        (0x0064)
	#define     DSI_PHY_T2                                                        (0x0068)
	#define     DSI_APHY_DEBUG0                                                   (0x0070)
	#define     DSI_APHY_DEBUG1                                                   (0x0074)
	#define     DSI_SELF_TEST                                                     (0x0078)
	#define     DSI_LANE_SWAP                                                     (0x007c)
	#define     DSI_PHY_CTRL                                                      (0x0080)
	#define     DSI_FT_TEST                                                       (0x0088)
			
#endif


#ifdef GL5206

	#define    DSIHW_REG_MEM_BASE                                                0xb0220000
	#define    DSIHW_REG_MEM_END                                                 0xb022008a
	
	#define     DSI_CTRL                                                          (0x0000)
	#define     DSI_SIZE                                                          (0x0004)
	#define     DSI_COLOR                                                         (0x0008)
	#define     DSI_VIDEO_CFG                                                     (0x000C)
	#define     DSI_RGBHT0                                                        (0x0010)
	#define     DSI_RGBHT1                                                        (0x0014)
	#define     DSI_RGBVT0                                                        (0x0018)
	#define     DSI_RGBVT1                                                        (0x001c)
	#define     DSI_TIMEOUT                                                       (0x0020)
	#define     DSI_TR_STA                                                        (0x0024)
	#define     DSI_INT_EN                                                        (0x0028)
	#define     DSI_ERROR_REPORT                                                  (0x002c)
	#define     DSI_FIFO_ODAT                                                     (0x0030)
	#define     DSI_FIFO_IDAT                                                     (0x0034)
	#define     DSI_IPACK                                                         (0x0038)
	#define     DSI_PACK_CFG                                                      (0x0040)
	#define     DSI_PACK_HEADER                                                   (0x0044)
	#define     DSI_TX_TRIGGER                                                    (0x0048)
	#define     DSI_RX_TRIGGER                                                    (0x004c)
	#define     DSI_LANE_CTRL                                                     (0x0050)
	#define     DSI_LANE_STA                                                      (0x0054)
	#define     DSI_PHY_T0                                                        (0x0060)
	#define     DSI_PHY_T1                                                        (0x0064)
	#define     DSI_PHY_T2                                                        (0x0068)
	#define     DSI_APHY_DEBUG0                                                   (0x0070)
	#define     DSI_APHY_DEBUG1                                                   (0x0074)
	#define     DSI_SELF_TEST                                                     (0x0078)
	#define     DSI_PIN_MAP                                                    	  (0x007c)
	#define     DSI_PHY_CTRL                                                      (0x0080)
	#define     DSI_FT_TEST                                                       (0x0088)	
	
	
	
	#define    DMA_REG_MEM_BASE                                                0xB0260000
	#define    DMA_REG_MEM_END                                                 0xB0260c00	
	#define    n   4
	
	#define    DMA_IRQ_PD0                                                		(0x0000)	
	
	#define     DMA0_BASE                                                         0x0100
	#define     DMA0_MODE                                                         (DMA0_BASE*n+0x0000)
	#define     DMA0_SOURCE                                                       (DMA0_BASE*n+0x0004)
	#define     DMA0_DESTINATION                                                  (DMA0_BASE*n+0x0008)
	#define     DMA0_FRAME_LEN                                                    (DMA0_BASE*n+0x000C)
	#define     DMA0_FRAME_CNT                                                    (DMA0_BASE*n+0x0010)
	#define     DMA0_REMAIN_FRAME_CNT                                             (DMA0_BASE*n+0x0014)
	#define     DMA0_REMAIN_CNT                                                   (DMA0_BASE*n+0x0018)
	#define     DMA0_SOURCE_STRIDE                                                (DMA0_BASE*n+0x001C)
	#define     DMA0_DESTINATION_STRIDE                                           (DMA0_BASE*n+0x0020)
	#define     DMA0_START                                                        (DMA0_BASE*n+0x0024)
	#define     DMA0_PAUSE                                                        (DMA0_BASE*n+0x0028)
	#define     DMA0_CHAINED_CTL                                                  (DMA0_BASE*n+0x002C)
	#define     DMA0_CONSTANT                                                     (DMA0_BASE*n+0x0030)
	#define     DMA0_LINKLIST_CTL                                                 (DMA0_BASE*n+0x0034)
	#define     DMA0_NEXT_DESCRIPTOR                                              (DMA0_BASE*n+0x0038)
	#define     DMA0_CURRENT_DESCRIPTOR_NUM                                       (DMA0_BASE*n+0x003C)
	#define     DMA0_INT_CTL                                                      (DMA0_BASE*n+0x0040)
	#define     DMA0_INT_STATUS                                                   (DMA0_BASE*n+0x0044)
	#define     DMA0_CURRENT_SOURCE_POINTER                                       (DMA0_BASE*n+0x0048)
	#define     DMA0_CURRENT_DESTINATION_POINTER                                  (DMA0_BASE*n+0x004C)

#endif

#ifdef GL5203
	
#endif

#endif
