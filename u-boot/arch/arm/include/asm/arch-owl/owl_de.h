/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009 Actions Semi Inc.
*/

#ifndef __OWL_DE_H__
#define __OWL_DE_H__

#include <asm/arch/actions_reg_owl.h>

#define     PATH_OFF_CTL			0x0000
#define     PATH_OFF_BK				0x0020
#define     PATH_OFF_SIZE			0x0024
#define     PATH_OFF_E_COOR			0x0028
#define     PATH_OFF_GAMMA_IDX			0x002C
#define     PATH_OFF_GAMMA_RAM			0x0030
#define     PATH_OFF_CURSOR_FB			0x0034
#define     PATH_OFF_CURSOR_STR			0x0038

#define     PATH_LEN                             		0x0100
#define     PATHx_BASE(x)                            	(PATH0_BASE + x * PATH_LEN)

#define     PATHx_CTL(x)                       		(PATHx_BASE(x) + PATH_OFF_CTL)
#define     PATHx_BK(x)                       		(PATHx_BASE(x) + PATH_OFF_BK)
#define     PATHx_SIZE(x)                       	(PATHx_BASE(x) + PATH_OFF_SIZE)
#define     PATHx_E_COOR(x)                       	(PATHx_BASE(x) + PATH_OFF_E_COOR)
#define     PATHx_GAMMA_IDX(x)                 (PATHx_BASE(x) + PATH_OFF_GAMMA_IDX)
#define     PATHx_GAMMA_RAM(x)                (PATHx_BASE(x) + PATH_OFF_GAMMA_RAM)
#define     PATHx_CURSOR_FB(x)                  (PATHx_BASE(x) + PATH_OFF_CURSOR_FB)
#define     PATHx_CURSOR_STR(x)                (PATHx_BASE(x) + PATH_OFF_CURSOR_STR)

#define     VIDEO_OFF_CFG                            0x0000
#define     VIDEO_OFF_ISIZE                          0x0004
#define     VIDEO_OFF_OSIZE                          0x0008
#define     VIDEO_OFF_SR                             0x000C
#define     VIDEO_OFF_SCOEF0                         0x0010
#define     VIDEO_OFF_SCOEF1                         0x0014
#define     VIDEO_OFF_SCOEF2                         0x0018
#define     VIDEO_OFF_SCOEF3                         0x001C
#define     VIDEO_OFF_SCOEF4                         0x0020
#define     VIDEO_OFF_SCOEF5                         0x0024
#define     VIDEO_OFF_SCOEF6                         0x0028
#define     VIDEO_OFF_SCOEF7                         0x002C
#define     VIDEO_OFF_FB_0                           0x0030
#define     VIDEO_OFF_FB_1                           0x0034
#define     VIDEO_OFF_FB_2                           0x0038
#define     VIDEO_OFF_FB_RIGHT_0                     0x003C
#define     VIDEO_OFF_FB_RIGHT_1                     0x0040
#define     VIDEO_OFF_FB_RIGHT_2                     0x0044
#define     VIDEO_OFF_STR                            0x0048
#define     VIDEO_OFF_CRITICAL                       0x004C
#define     VIDEO_OFF_REMAPPING                      0x0050
#define     VIDEO_OFF_COOR                           0x0054
#define     VIDEO_OFF_ALPHA                          0x0058
#define     VIDEO_OFF_CKMAX                          0x005C
#define     VIDEO_OFF_CKMIN                          0x0060
#define     VIDEO_OFF_BLEND                          0x0064

#define     VIDEO_LEN                             		0x0100
#define     VIDEOx_BASE(x)                           	(VIDEO0_BASE + x * VIDEO_LEN)

#define     VIDEOx_CFG(x)                       		(VIDEOx_BASE(x) + VIDEO_OFF_CFG)
#define     VIDEOx_ISIZE(x)                       		(VIDEOx_BASE(x) + VIDEO_OFF_ISIZE)
#define     VIDEOx_OSIZE(x)                       		(VIDEOx_BASE(x) + VIDEO_OFF_OSIZE)
#define     VIDEOx_SR(x)                       			(VIDEOx_BASE(x) + VIDEO_OFF_SR)
#define     VIDEOx_SCOEF0(x)                       	(VIDEOx_BASE(x) + VIDEO_OFF_SCOEF0)
#define     VIDEOx_SCOEF1(x)                       	(VIDEOx_BASE(x) + VIDEO_OFF_SCOEF1)
#define     VIDEOx_SCOEF2(x)                       	(VIDEOx_BASE(x) + VIDEO_OFF_SCOEF2)
#define     VIDEOx_SCOEF3(x)                       	(VIDEOx_BASE(x) + VIDEO_OFF_SCOEF3)
#define     VIDEOx_SCOEF4(x)                       	(VIDEOx_BASE(x) + VIDEO_OFF_SCOEF4)
#define     VIDEOx_SCOEF5(x)                       	(VIDEOx_BASE(x) + VIDEO_OFF_SCOEF5)
#define     VIDEOx_SCOEF6(x)                       	(VIDEOx_BASE(x) + VIDEO_OFF_SCOEF6)
#define     VIDEOx_SCOEF7(x)                       	(VIDEOx_BASE(x) + VIDEO_OFF_SCOEF7)
#define     VIDEOx_FB_0(x)                       		(VIDEOx_BASE(x) + VIDEO_OFF_FB_0)
#define     VIDEOx_FB_1(x)                       		(VIDEOx_BASE(x) + VIDEO_OFF_FB_1)
#define     VIDEOx_FB_2(x)                       		(VIDEOx_BASE(x) + VIDEO_OFF_FB_2)
#define     VIDEOx_FB_RIGHT_0(x)                      (VIDEOx_BASE(x) + VIDEO_OFF_FB_RIGHT_0)
#define     VIDEOx_FB_RIGHT_1(x)                      (VIDEOx_BASE(x) + VIDEO_OFF_FB_RIGHT_1)
#define     VIDEOx_FB_RIGHT_2(x)                      (VIDEOx_BASE(x) + VIDEO_OFF_FB_RIGHT_2)
#define     VIDEOx_STR(x)                       		(VIDEOx_BASE(x) + VIDEO_OFF_STR)
#define     VIDEOx_CRITICAL(x)                       	(VIDEOx_BASE(x) + VIDEO_OFF_CRITICAL)
#define     VIDEOx_REMAPPING(x)                       (VIDEOx_BASE(x) + VIDEO_OFF_REMAPPING)
#define     VIDEOx_COOR(x)                       		(VIDEOx_BASE(x) + VIDEO_OFF_COOR)
#define     VIDEOx_ALPHA(x)                       		(VIDEOx_BASE(x) + VIDEO_OFF_ALPHA)
#define     VIDEOx_CKMAX(x)                       		(VIDEOx_BASE(x) + VIDEO_OFF_CKMAX)
#define     VIDEOx_CKMIN(x)                       		(VIDEOx_BASE(x) + VIDEO_OFF_CKMIN)
#define     VIDEOx_BLEND(x)                       		(VIDEOx_BASE(x) + VIDEO_OFF_BLEND)

/******************************************************************************/


/******************************************************************************/
/*DE_INTEN*/
/*bit 5-31 Reserved*/
#define DE_INTEN_HDMI          (0x1 << 4)
#define DE_INTEN_CVBS          (0x1 << 3)
#define DE_INTEN_DSI           (0x1 << 2)
#define DE_INTEN_LCD1          (0x1 << 1)
#define DE_INTEN_LCD0          (0x1 << 0)
/******************************************************************************/

/*DE_STAT*/
/*bit 13-31 Reserved*/
#define DE_STAT_IMG5_VB          (0x1 << 12)
#define DE_STAT_IMG4_VB          (0x1 << 11)
#define DE_STAT_IMG3_VB          (0x1 << 10)
#define DE_STAT_IMG2_VB          (0x1 << 9)
#define DE_STAT_IMG1_VB          (0x1 << 8)
/*bit 5-7 Reserved*/
#define DE_STAT_IMG5_PRE          (0x1 << 4)
#define DE_STAT_IMG4_PRE          (0x1 << 3)
#define DE_STAT_IMG3_PRE          (0x1 << 2)
#define DE_STAT_IMG2_PRE          (0x1 << 1)
#define DE_STAT_IMG1_PRE          (0x1 << 0)
/******************************************************************************/

/*DE_CRITICAL_NUM*/
/*bit 8-31 Reserved*/
#define DE_CRITICAL_NUM_ON(x)      (((x) & 0xf) << 4)
#define DE_CRITICAL_NUM_OFF(x)     (((x) & 0xf) << 0)

/******************************************************************************/
/* PATHx_CTL */
/*bit 30-31 Reserved*/
#define PATH_CTL_FCR                      (0x1 << 29)
#define PATH_CTL_PATH_ENABLE              (0x1 << 28)
/*bit 25-27 Reserved*/
#define PATH_CTL_CURSOR_ENABLE            (0x1 << 24)
#define PATH_CTL_VIDEO4_ENABLE            (0x1 << 23)
#define PATH_CTL_VIDEO3_ENABLE            (0x1 << 22)
#define PATH_CTL_VIDEO2_ENABLE            (0x1 << 21)
#define PATH_CTL_VIDEO1_ENABLE            (0x1 << 20)
/*bit 17-19 Reserved*/
#define PATH_CTL_RGB_YUV_EN               (0x1 << 16)
#define PATH_CTL_YUV_FORMAT_MASK	         (0x0 << 15)
#define PATH_CTL_YUV_FORMAT_BIT601	       (0x0 << 15)
#define PATH_CTL_YUV_FORMAT_BIT709	       (0x1 << 15)
#define PATH_CTL_DITHER_B                 (0x1 << 14)
#define PATH_CTL_DITHER_G                 (0x1 << 13)
#define PATH_CTL_DITHER_R                 (0x1 << 12)
#define PATH_CTL_INTERLACE_ENABLE				 (0x1 << 11)
#define PATH_CTL_EN_3D										 (0x1 << 10)

#define PATH_CTL_GAMMA_ENABLE             (0x1 << 9)
#define PATH_CTL_DITHER_ENABLE						 (0x1 << 8)
/*bit 0-7 Reserved*/


/******************************************************************************/
/* PATHx_BK */
/*bit 24-31 Reserved*/
#define PATH_BK_R(x)  					(((x) & 0xff) << 16)
#define PATH_BK_G(x)  					(((x) & 0xff) << 8)
#define PATH_BK_B(x)  					(((x) & 0xff) << 0)

/******************************************************************************/
/* PATHx_SIZE */
/*bit 27-31 Reserved*/
#define PATH_SIZE_HEIGHT(x)  	(((x) & 0x7ff) << 16)
/*bit 11-15 Reserved*/
#define PATH_SIZE_WIDTH(x)  	  (((x) & 0x7ff) << 0)

/******************************************************************************/
/* PATHx_E_COOR */
/*bit 27-31 Reserved*/
#define PATH_E_COOR_Y(x)  	    (((x) & 0x7ff) << 16)
/*bit 11-15 Reserved*/
#define PATH_E_COOR_X(x)  	    (((x) & 0x7ff) << 0)

/******************************************************************************/
/* PATHx_GAMMA_IDX */
/*bit 15-31 Reserved*/
#define PATH_GAMMA_IDX_BUSY						 (0x1 << 14)
#define PATH_GAMMA_IDX_RAM_OP(x)        (((x) & 0x3) << 12)
/*bit 8-11 Reserved*/
#define PATH_GAMMA_IDX_RAM_IDX_(x)      (((x) & 0xff) << 0)

/******************************************************************************/
/* PATHx_GAMMA_RAM */

/******************************************************************************/
/* PATHx_CURSOR_FB */
#define PATH_CURSOR_FB_ADDR(x)          (((x) & 0x1fffffff) << 0)
/*bit 0-2 Reserved*/

/******************************************************************************/
/* PATHx_CURSOR_STR */
/*bit 12-31 Reserved*/
#define PATH_CURSOR_FB_STR(x)          (((x) & 0xfff) << 0)

/******************************************************************************/
/*VIDEOx_CFG*/
#define VIDEO_CFG_FMT_CHANGE(x)  					(((x) & 0x3) << 30)
#define VIDEO_CFG_CSC_IYUV_QEN 						(1  << 29)
#define VIDEO_CFG_CSC_IYUV_FMT 						(1  << 28)
#define VIDEO_CFG_CRITICAL(x)  						(((x) & 0x3) << 26)
#define VIDEO_CFG_YSUB_MASK	 						(0x3 << 24)
#define VIDEO_CFG_YSUB(x)				 						(((x) & 0x3) << 24)
#define VIDEO_CFG_XSUB_MASK	 						(0x3 << 22)
#define VIDEO_CFG_XSUB(x)			 						(((x) & 0x3) << 22)
#define VIDEO_CFG_XFLIP				 						(1  << 21)
#define VIDEO_CFG_YFLIP 			 						(1  << 20)
#define VIDEO_CFG_BRI(x)			 						(((x) & 0xff) << 12)
#define VIDEO_CFG_SAT(x)			 						(((x) & 0xf) << 8)
#define VIDEO_CFG_CON(x)       						(((x) & 0xf) << 4)
/*bit 3 Reserved*/
#define VIDEO_CFG_FMT_MASK                (0x7 << 0)
#define VIDEO_CFG_FMT(x)       						(((x) & 0x7) << 0)

/******************************************************************************/
/*VIDEOx_ISIZE*/
/*bit 27-31 Reserved*/
#define VIDEO_ISIZE_HEIGHT(x)  	  (((x) & 0x7ff) << 16)
/*bit 11-15 Reserved*/
#define VIDEO_ISIZE_WIDTH(x)  	  (((x) & 0x7ff) << 0)

/******************************************************************************/
/*VIDEOx_OSIZE*/
/*bit 27-31 Reserved*/
#define VIDEO_OSIZE_HEIGHT(x)  	  (((x) & 0x7ff) << 16)
/*bit 11-15 Reserved*/
#define VIDEO_OSIZE_WIDTH(x)  	  (((x) & 0x7ff) << 0)

/******************************************************************************/
/*VIDEOx_SR*/
#define VIDEO_SR_VSR(x)  	        (((x) & 0xffff) << 16)
#define VIDEO_SR_HSR(x)  	        (((x) & 0xffff) << 0)


/******************************************************************************/
/*VIDEOx_FB_0*/

/******************************************************************************/
/*VIDEOx_FB_1*/

/******************************************************************************/
/*VIDEOx_FB_2*/

/******************************************************************************/
/*VIDEOx_FB_RIGHT_0*/

/******************************************************************************/
/*VIDEOx_FB_RIGHT_1*/

/******************************************************************************/
/*VIDEOx_FB_RIGHT_2*/

/******************************************************************************/
/*VIDEOx_STR*/
/*bit 31 Reserved*/
#define VIDEO_STRV_MASK           (0x3FF << 21)
#define VIDEO_STRV(x)             (((x) & 0x3FF) << 21)
#define VIDEO_STRU_MASK           (0x3FF << 11)
#define VIDEO_STRU(x)             (((x) & 0x3FF) << 11)
#define VIDEO_STRY_MASK           (0x7FF << 0)
#define VIDEO_STRY(x)             (((x) & 0x7FF) << 0)

/*bit 22-31 Reserved*/
#define VIDEO_STRUV_MASK          (0x7FF << 11)
#define VIDEO_STRUV(x)            (((x) & 0x7FF) << 11)

/*bit 13-31 Reserved*/
#define VIDEO_STR_MASK            (0x01FFF << 0)
#define VIDEO_STR(x)              (((x) & 0x01FFF) << 0)

/******************************************************************************/
/*VIDEOx_CRITICAL_CFG*/
#define VIDEO_CRITI_COEF(x)  	        (((x) & 0x1ff) << 0)

/******************************************************************************/
/*VIDEOx_REMAPPING*/


/******************************************************************************/
/*VIDEOx_COOR*/
/*bit 27-31 Reserved*/
#define VIDEO_COOR_Y(x)  	  (((x) & 0x7ff) << 16)
/*bit 11-15 Reserved*/
#define VIDEO_COOR_X(x)  	  (((x) & 0x7ff) << 0)

/******************************************************************************/
/*VIDEOx_ALPHA_CFG*/
/*bit 8-31 Reserved*/
#define VIDEO_ALPHA_A(x)  	  (((x) & 0xff) << 0)

/******************************************************************************/
/*VIDEOx_CKMAX*/
/*bit 24-31 Reserved*/
#define VIDEO_CKMAX_R(x)  	  (((x) & 0xff) << 16)
#define VIDEO_CKMAX_G(x)  	  (((x) & 0xff) << 8)
#define VIDEO_CKMAX_B(x)  	  (((x) & 0xff) << 0)

/******************************************************************************/
/*VIDEOx_CKMIN*/
/*bit 24-31 Reserved*/
#define VIDEO_CKMIN_R(x)  	  (((x) & 0xff) << 16)
#define VIDEO_CKMIN_G(x)  	  (((x) & 0xff) << 8)
#define VIDEO_CKMIN_B(x)  	  (((x) & 0xff) << 0)

/******************************************************************************/
/*VIDEOx_BLEND*/
/*bit 2-31 Reserved*/
#define VIDEO_BLEND_CK_EN     	      ((0x1) << 1)
#define VIDEO_BLEND_ALPHA_EN      	  ((0x1) << 0)

/******************************************************************************/
/*OUTPUT_CTL*/
/*bit 14-31 Reserved*/
#define OUTPUT_CTL_DBG_SEL(x)  	      (((x) & 0xf) << 15)
#define OUTPUT_CTL_TIC_OK  	          ((0x1) << 14)
#define OUTPUT_CTL_TIC_ST  	          ((0x1) << 13)
#define OUTPUT_CTL_TIC_SEL  	        ((0x1) << 12)
/*bit 7-11 Reserved*/
#define OUTPUT_CTL_PATH1_SEL_MASK   (0x7 << 4)
#define OUTPUT_CTL_PATH1_SEL_SHIFT   4
#define OUTPUT_CTL_PATH1_SEL(x)  	    (((x) & 0x7) << OUTPUT_CTL_PATH1_SEL_SHIFT)
/*bit 3 Reserved*/
#define OUTPUT_CTL_PATH0_SEL_MASK   (0x7 << 0)
#define OUTPUT_CTL_PATH0_SEL_SHIFT   0
#define OUTPUT_CTL_PATH0_SEL(x)  	    (((x) & 0x7) << OUTPUT_CTL_PATH0_SEL_SHIFT)

#endif
