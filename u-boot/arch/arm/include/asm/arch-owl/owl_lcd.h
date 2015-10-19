/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009 Actions Semi Inc.
*/
/******************************************************************************/

/******************************************************************************/
#ifndef __OWL_LCD_H__
#define __OWL_LCD_H__

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
#define     LCD0_BASE																														 0xB02A0000
#define     LCD1_BASE																														 (LCD0_BASE+0x0100)
#define     LCD_OFF_CTL                                                          0x0000
#define     LCD_OFF_SIZE                                                         0x0004
#define     LCD_OFF_STATUS                                                       0x0008
#define     LCD_OFF_TIM0                                                         0x000C
#define     LCD_OFF_TIM1                                                         0x0010
#define     LCD_OFF_TIM2                                                         0x0014
#define     LCD_OFF_COLOR                                                        0x0018
#define     LCD_OFF_CPU_CTL                                                      0x001c
#define     LCD_OFF_CPU_CMD                                                      0x0020
#define     LCD_OFF_TEST_P0                                                      0x0024
#define     LCD_OFF_TEST_P1                                                      0x0028
#define     LCD_OFF_IMG_XPOS                                                     0x002c
#define     LCD_OFF_IMG_YPOS                                                     0x0030

#define     LCD_LEN                             		0x0100
#define     LCDx_BASE(x)                            (LCD_BASE + x * LCD_LEN)

#define     LCDx_CTL(x)                       			(LCDx_BASE(x) + LCD_OFF_CTL)
#define     LCDx_SIZE(x)                       			(LCDx_BASE(x) + LCD_OFF_SIZE)
#define     LCDx_STATUS(x)                       		(LCDx_BASE(x) + LCD_OFF_STATUS)
#define     LCDx_TIM0(x)                       			(LCDx_BASE(x) + LCD_OFF_TIM0)
#define     LCDx_TIM1(x)                       			(LCDx_BASE(x) + LCD_OFF_TIM1)
#define     LCDx_TIM2(x)                       			(LCDx_BASE(x) + LCD_OFF_TIM2)
#define     LCDx_COLOR(x)                       		(LCDx_BASE(x) + LCD_OFF_COLOR)
#define     LCDx_CPU_CTL(x)                       		(LCDx_BASE(x) + LCD_OFF_CPU_CTL)
#define     LCDx_CPU_CMD(x)                       		(LCDx_BASE(x) + LCD_OFF_CPU_CMD)
#define     LCDx_TEST_P0(x)                       		(LCDx_BASE(x) + LCD_OFF_TEST_P0)
#define     LCDx_TEST_P1(x)                       		(LCDx_BASE(x) + LCD_OFF_TEST_P1)
#define     LCDx_IMG_XPOS(x)                       	(LCDx_BASE(x) + LCD_OFF_IMG_XPOS)
#define     LCDx_IMG_YPOS(x)                       	(LCDx_BASE(x) + LCD_OFF_IMG_YPOS)


/******************************************************************************/
/*LCD_CTRL0*/
#define LCD_CTL_SEL_MASK							(0x1 << 31)
#define LCD_CTL_SEL_RGBI							(0x0 << 31)
#define LCD_CTL_SEL_CPUI							(0x1 << 31)

#define LCD_CTL_SELF_RST_EN						(0x1 << 30)
#define LCD_CTL_3D_EN						      (0x1 << 29)
#define LCD_CTL_DCRGB_EN						  (0x1 << 28)

/*bit 21-27 Reserved*/
#define LCD_CTL_PD_IDL_STA						(0x1 << 20)
/*bit 19 Reserved*/
#define LCD_CTL_IF_MASK								(0x7 << 16)
#define LCD_CTL0_IF_24P								(0x0 << 16)
#define LCD_CTL_IF_18P								(0x1 << 16)
#define LCD_CTL_IF_16P								(0x2 << 16)
#define LCD_CTL_IF_8P               	(0x3 << 16)
#define LCD_CTL_IF_24S              	(0x4 << 16)
#define LCD_CTL_IF_18S              	(0x5 << 16)

#define LCD_CTL_CC_ODD_MASK          	(0x7 << 13)
#define LCD_CTL_CC_ODD_RGB           	(0x0 << 13)
#define LCD_CTL_CC_ODD_RBG           	(0x1 << 13)
#define LCD_CTL_CC_ODD_GRB           	(0x2 << 13)
#define LCD_CTL_CC_ODD_GBR           	(0x3 << 13)
#define LCD_CTL_CC_ODD_BRG           	(0x4 << 13)
#define LCD_CTL_CC_ODD_BGR           	(0x5 << 13)

#define LCD_CTL_CC_EVEN_MASK          (0x7 << 10)
#define LCD_CTL_CC_EVEN_RGB           (0x0 << 10)
#define LCD_CTL_CC_EVEN_RBG           (0x1 << 10)
#define LCD_CTL_CC_EVEN_GRB           (0x2 << 10)
#define LCD_CTL_CC_EVEN_GBR           (0x3 << 10)
#define LCD_CTL_CC_EVEN_BRG           (0x4 << 10)
#define LCD_CTL_CC_EVEN_BGR           (0x5 << 10)

#define LCD_CTL_PAD_MASK              (0x3 << 8)
#define LCD_CTL_PAD_NO                (0x0 << 8)
#define LCD_CTL_PAD_XAFTER            (0x1 << 8)
#define LCD_CTL_PAD_XBEFORE           (0x2 << 8)

#define LCD_CTL_VOM_MASK              (0x3 << 6)
#define LCD_CTL_VOM_DMA               (0x0 << 6)
#define LCD_CTL_VOM_DE                (0x1 << 6)
#define LCD_CTL_VOM_DEFCOLOR          (0x2 << 6)

/*bit 2-5 Reserved*/

#define LCD_CTL_RB_SWAP_EN            (0x1 << 1)
#define LCD_CTL_EN                    (0x1 << 0)


/******************************************************************************/
/*LCD_SIZE*/
/*bit 27-31 Reserved*/
#define LCD_SIZE_Y_MASK                 (0x7FF << 16)
#define LCD_SIZE_Y(x)                   (((x) & 0x7FF) << 16)
/*bit 11-15 Reserved*/
#define LCD_SIZE_X_MASK                 (0x7FF << 0)
#define LCD_SIZE_X(x)                   (((x) & 0x7FF) << 0)

/******************************************************************************/
/*LCD_STATUS*/
#define LCD_STATUS_VBI                  (0x1 << 31)
#define LCD_STATUS_VBE                  (0x1 << 30)
#define LCD_STATUS_HBI                  (0x1 << 29)
#define LCD_STATUS_HBE                  (0x1 << 28)
#define LCD_STATUS_AVSI                 (0x1 << 27)
#define LCD_STATUS_AVSE                 (0x1 << 26)
#define LCD_STATUS_FEIP                 (0x1 << 25)
#define LCD_STATUS_FEIE                 (0x1 << 24)
/*bit 22-23 Reserved*/
#define LCD_STATUS_CX_MASK              (0x7FF << 11)
#define LCD_STATUS_CY_MASK              (0x7FF << 0)

/******************************************************************************/
/*LCD_RGBTIMING0*/
/*bit 8-31 Reserved*/
#define LCD_RGBTIMING0_PREL_EN          (1 << 13)
#define LCD_RGBTIMING0_PREL_CNT_MASK    (0X1F << 8)
#define LCD_RGBTIMING0_PREL_CNT(x)      (((x) & 0X1F) << 8)
#define LCD_RGBTIMING0_VSYNC_INV        (0x1 << 7)
#define LCD_RGBTIMING0_HSYNC_INV        (0x1 << 6)
#define LCD_RGBTIMING0_DCLK_INV         (0x1 << 5)
#define LCD_RGBTIMING0_LDE_INV          (0x1 << 4)
/*bit 0-3 Reserved*/

/******************************************************************************/
/*LCD_RGBTIMING1*/
/*bit 30-31 Reserved*/
#define LCD_RGBTIMING1_HSPW_MASK        (0x3FF << 20)
#define LCD_RGBTIMING1_HSPW(x)          (((x) & 0x3FF) << 20)
#define LCD_RGBTIMING1_HFP_MASK         (0x3FF << 10)
#define LCD_RGBTIMING1_HFP(x)           (((x) & 0x3FF) << 10)
#define LCD_RGBTIMING1_HBP_MASK         (0x3FF << 0)
#define LCD_RGBTIMING1_HBP(x)           (((x) & 0x3FF) << 0)

/******************************************************************************/
/*LCD_RGBTIMING2*/
/*bit 29-31 Reserved*/
#define LCD_RGBTIMING2_VSPW_MASK        (0x1FF << 20)
#define LCD_RGBTIMING2_VSPW(x)          (((x) & 0x1FF) << 20)
#define LCD_RGBTIMING2_VFP_MASK         (0x3FF << 10)
#define LCD_RGBTIMING2_VFP(x)           (((x) & 0x3FF) << 10)
#define LCD_RGBTIMING2_VBP_MASK         (0x3FF << 0)
#define LCD_RGBTIMING2_VBP(x)           (((x) & 0x3FF) << 0)

/******************************************************************************/
/*LCD_COLOR*/
/*bit 24-31 Reserved*/
#define LCD_COLOR_R_MASK                (0xFF << 16)
#define LCD_COLOR_R(x)                  (((x) & 0xFF) << 16)
#define LCD_COLOR_G_MASK                (0xFF << 8)
#define LCD_COLOR_G(x)                  (((x) & 0xFF) << 8)
#define LCD_COLOR_B_MASK                (0xFF << 0)
#define LCD_COLOR_B(x)                  (((x) & 0xFF) << 0)

/******************************************************************************/
/*LVDS_CTL*/
/*bit 21-31 Reserved*/
#define LVDS_CTL_E_RSV_1								(0x1 << 20)

#define LVDS_CTL_E_DE_MASK							(0x3 << 18)
#define LVDS_CTL_E_DE_0									(0x0 << 18)
#define LVDS_CTL_E_DE_1									(0x1 << 18)
#define LVDS_CTL_E_DE_DE								(0x2 << 18)

#define LVDS_CTL_E_VS_MASK							(0x3 << 16)
#define LVDS_CTL_E_VS_0									(0x0 << 16)
#define LVDS_CTL_E_VS_1									(0x1 << 16)
#define LVDS_CTL_E_VS_VS								(0x2 << 16)

#define LVDS_CTL_E_HS_MASK							(0x3 << 14)
#define LVDS_CTL_E_HS_0									(0x0 << 14)
#define LVDS_CTL_E_HS_1									(0x1 << 14)
#define LVDS_CTL_E_HS_HS								(0x2 << 14)

#define LVDS_CTL_O_RSV_1								(0x1 << 13)

#define LVDS_CTL_O_DE_MASK							(0x3 << 11)
#define LVDS_CTL_O_DE_0									(0x0 << 11)
#define LVDS_CTL_O_DE_1									(0x1 << 11)
#define LVDS_CTL_O_DE_DE								(0x2 << 11)

#define LVDS_CTL_O_VS_MASK							(0x3 << 9)
#define LVDS_CTL_O_VS_0									(0x0 << 9)
#define LVDS_CTL_O_VS_1									(0x1 << 9)
#define LVDS_CTL_O_VS_VS								(0x2 << 9)

#define LVDS_CTL_O_HS_MASK							(0x3 << 7)
#define LVDS_CTL_O_HS_0									(0x0 << 7)
#define LVDS_CTL_O_HS_1									(0x1 << 7)
#define LVDS_CTL_O_HS_HS								(0x2 << 7)

#define LVDS_CTL_MIRROR									(0x1 << 6)
#define LVDS_CTL_CH_SWAP								(0x1 << 5)

#define LVDS_CTL_MAPPING_MASK						(0x3 << 3)
#define LVDS_CTL_MAPPING_TB1						(0x0 << 3)
#define LVDS_CTL_MAPPING_TB2						(0x1 << 3)

#define LVDS_CTL_CHANNEL_MASK						(0x1 << 2)
#define LVDS_CTL_CHANNEL_SINGLE					(0x0 << 2)
#define LVDS_CTL_CHANNEL_DUAL						(0x1 << 2)

#define LVDS_CTL_FORMAT_MASK						(0x1 << 1)
#define LVDS_CTL_FORMAT_18B							(0x0 << 1)
#define LVDS_CTL_FORMAT_24B							(0x1 << 1)

#define LVDS_CTL_EN											(0x1 << 0)

/******************************************************************************/
/*LCD_CPUCON*/
/*bit 12-31 Reserved*/
#define LCD_CPUCON_LCD_ID_MASK          (0x1 << 11)
#define LCD_CPUCON_LCD_ID_0		          (0x0 << 11)
#define LCD_CPUCON_LCD_ID_1   		      (0x1 << 11)

#define LCD_CPUCON_RS_HIGH              (0x1 << 10)

#define LCD_CPUCON_WDCS_MASK            (0x3 << 8)
#define LCD_CPUCON_WDCS_IDX_CMD         (0x0 << 8)
#define LCD_CPUCON_WDCS_DATA            (0x2 << 8)
/*bit 7 Reserved*/
#define LCD_CPUCON_FORMATS_MASK         (0x7 << 4)
#define LCD_CPUCON_FORMATS_565_1TRANS   (0x0 << 4)
#define LCD_CPUCON_FORMATS_666_1TRANS   (0x1 << 4)
#define LCD_CPUCON_FORMATS_565_2TRANS  	(0x2 << 4)
#define LCD_CPUCON_FORMATS_666_2TRANS   (0x3 << 4)
#define LCD_CPUCON_FORMATS_888_3TRANS		(0x4 << 4)
#define LCD_CPUCON_FORMATS_666_3TRANS   (0x5 << 4)
/*bit 1-3 Reserved*/
#define LCD_CPUCON_ST                   (0x1 << 0)

/******************************************************************************/
/*LCD_CPUCOM*/
/*bit 18-31 Reserved*/
#define LCD_CPUCOM_COMMAND_MASK         (0x3FFFF << 0)
#define LCD_CPUCOM_COMMAND(x)           (((x) & 0x3FFFF) << 0)

/******************************************************************************/
/*LCD_TESTPAR0*/
/*bit 17-31 Reserved*/
#define LCD_TESTPAR0_EN				         (0x1 << 16)

#define LCD_TESTPAR0_PRD_MASK     		 (0x3 << 12)
#define LCD_TESTPAR0_PRD_BK_PORCH      (0x0 << 12)
#define LCD_TESTPAR0_PRD_ACTIVE_PRD    (0x1 << 12)
#define LCD_TESTPAR0_PRD_FRT_PORCH     (0x2 << 12)
/*bit 10-11 Reserved*/
#define LCD_TESTPAR0_LINE_MASK         (0x3FF << 0)
#define LCD_TESTPAR0_LINE(x)           (((x) & 0x3FF) << 0)

/******************************************************************************/
/*LCD_TESTPAR1*/
/*bit 17-31 Reserved*/
#define LCD_TESTPAR1_EN				         (0x1 << 16)
/*bit 12-15 Reserved*/
#define LCD_TESTPAR1_DCLKNUM_MASK      (0xFFF << 0)
#define LCD_TESTPAR1_DCLKNUM(x)        (((x) & 0xFFF) << 0)


/******************************************************************************/
/*LCD_IMGXPOS*/
/*bit 27-31 Reserved*/
#define LCD_IMGXPOS_START_MASK         (0x7FF << 16)
#define LCD_IMGXPOS_START(x)    		   (((x) & 0x7FF) << 16)
/*bit 11-15 Reserved*/
#define LCD_IMGXPOS_END_MASK	         (0x7FF << 0)
#define LCD_IMGXPOS_END(x)    	     	 (((x) & 0x7FF) << 0)


/******************************************************************************/
/*LCD_IMGYPOS*/
/*bit 27-31 Reserved*/
#define LCD_IMGYPOS_START_MASK         (0x7FF << 16)
#define LCD_IMGYPOS_START(x)    		   (((x) & 0x7FF) << 16)
/*bit 11-15 Reserved*/
#define LCD_IMGYPOS_END_MASK	         (0x7FF << 0)
#define LCD_IMGYPOS_END(x)    	     	 (((x) & 0x7FF) << 0)

/******************************************************************************/
/*LVDS_ANALOG_CTL0*/
/*bit 25-31 Reserved*/
#define LVDS_ANALOG_CTL0_IBPOWL	         (0x1 << 24)
#define LVDS_ANALOG_CTL0_PLLPOWL         (0x1 << 23)
#define LVDS_ANALOG_CTL0_SIBXL	         (0x1 << 22)

#define LVDS_ANALOG_CTL0_SIBGENL_MASK    (0x7 << 19)
#define LVDS_ANALOG_CTL0_SIBGENL(x)	     (((x) & 0x7) << 19)

#define LVDS_ANALOG_CTL0_SLVDSIL_MASK    (0x3 << 17)
#define LVDS_ANALOG_CTL0_SLVDSIL(x)	     (((x) & 0x3) << 17)

#define LVDS_ANALOG_CTL0_ENIB40UX2L      (0x1 << 16)

#define LVDS_ANALOG_CTL0_SVOCML_MASK     (0x7 << 13)
#define LVDS_ANALOG_CTL0_SVOCML(x)	     (((x) & 0x7) << 13)

#define LVDS_ANALOG_CTL0_SBGL_MASK 		   (0x3 << 11)
#define LVDS_ANALOG_CTL0_SBGL(x)	    	 (((x) & 0x3) << 11)

#define LVDS_ANALOG_CTL0_SPLLIL_MASK     (0x7 << 8)
#define LVDS_ANALOG_CTL0_SPLLIL(x)	     (((x) & 0x7) << 8)

#define LVDS_ANALOG_CTL0_SPLLRL_MASK 		 (0x3 << 6)
#define LVDS_ANALOG_CTL0_SPLLRL(x)	     (((x) & 0x3) << 6)

#define LVDS_ANALOG_CTL0_WDMODEL_MASK 	 (0x3 << 4)
#define LVDS_ANALOG_CTL0_WDMODEL(x)	     (((x) & 0x3) << 4)

#define LVDS_ANALOG_CTL0_PLLPOLARL       (0x1 << 3)

#define LVDS_ANALOG_CTL0_STSTIL_MASK     (0x7 << 0)
#define LVDS_ANALOG_CTL0_STSTIL(x)	   	 (((x) & 0x7) << 0)


/******************************************************************************/
/*LVDS_ANALOG_CTL1*/
#define TLL_TX_MODE											 (0x0)
#define TLL_RX_MODE											 (0x1)
#define LVDS_TX_MODE										 (0x2)

#define LVDS_ANALOG_CTL1_EANMODEL_MASK 	 (0x3 << 30)
#define LVDS_ANALOG_CTL1_EANMODEL(x)	   (((x) & 0x3) << 30)
#define LVDS_ANALOG_CTL1_EAPMODEL_MASK 	 (0x3 << 28)
#define LVDS_ANALOG_CTL1_EAPMODEL(x)	   (((x) & 0x3) << 28)
#define LVDS_ANALOG_CTL1_EBNMODEL_MASK 	 (0x3 << 26)
#define LVDS_ANALOG_CTL1_EBNMODEL(x)	   (((x) & 0x3) << 26)
#define LVDS_ANALOG_CTL1_EBPMODEL_MASK 	 (0x3 << 24)
#define LVDS_ANALOG_CTL1_EBPMODEL(x)	   (((x) & 0x3) << 24)
#define LVDS_ANALOG_CTL1_ECNMODEL_MASK 	 (0x3 << 22)
#define LVDS_ANALOG_CTL1_ECNMODEL(x)	   (((x) & 0x3) << 22)
#define LVDS_ANALOG_CTL1_ECPMODEL_MASK 	 (0x3 << 20)
#define LVDS_ANALOG_CTL1_ECPMODEL(x)	   (((x) & 0x3) << 20)
#define LVDS_ANALOG_CTL1_EDNMODEL_MASK 	 (0x3 << 18)
#define LVDS_ANALOG_CTL1_EDNMODEL(x)	   (((x) & 0x3) << 18)
#define LVDS_ANALOG_CTL1_EDPMODEL_MASK 	 (0x3 << 16)
#define LVDS_ANALOG_CTL1_EDPMODEL(x)	   (((x) & 0x3) << 16)
#define LVDS_ANALOG_CTL1_EENMODEL_MASK 	 (0x3 << 14)
#define LVDS_ANALOG_CTL1_EENMODEL(x)	   (((x) & 0x3) << 14)
#define LVDS_ANALOG_CTL1_EEPMODEL_MASK 	 (0x3 << 12)
#define LVDS_ANALOG_CTL1_EEPMODEL(x)	   (((x) & 0x3) << 12)
#define LVDS_ANALOG_CTL1_OANMODEL_MASK 	 (0x3 << 10)
#define LVDS_ANALOG_CTL1_OANMODEL(x)	   (((x) & 0x3) << 10)
#define LVDS_ANALOG_CTL1_OAPMODEL_MASK 	 (0x3 << 8)
#define LVDS_ANALOG_CTL1_OAPMODEL(x)	   (((x) & 0x3) << 8)
#define LVDS_ANALOG_CTL1_OBNMODEL_MASK 	 (0x3 << 6)
#define LVDS_ANALOG_CTL1_OBNMODEL(x)	   (((x) & 0x3) << 6)
#define LVDS_ANALOG_CTL1_OBPMODEL_MASK 	 (0x3 << 4)
#define LVDS_ANALOG_CTL1_OBPMODEL(x)	   (((x) & 0x3) << 4)
#define LVDS_ANALOG_CTL1_OCNMODEL_MASK 	 (0x3 << 2)
#define LVDS_ANALOG_CTL1_OCNMODEL(x)	   (((x) & 0x3) << 2)
#define LVDS_ANALOG_CTL1_OCPMODEL_MASK 	 (0x3 << 0)
#define LVDS_ANALOG_CTL1_OCPMODEL(x)	   (((x) & 0x3) << 0)


/******************************************************************************/
/*LVDS_ANALOG_CTL2*/
/*bit 16-31 Reserved*/
#define LVDS_ANALOG_CTL2_ODNMODEL_MASK 	 (0x3 << 14)
#define LVDS_ANALOG_CTL2_ODNMODEL(x)	   (((x) & 0x3) << 14)
#define LVDS_ANALOG_CTL2_ODPMODEL_MASK 	 (0x3 << 12)
#define LVDS_ANALOG_CTL2_ODPMODEL(x)	   (((x) & 0x3) << 12)
#define LVDS_ANALOG_CTL2_OENMODEL_MASK 	 (0x3 << 10)
#define LVDS_ANALOG_CTL2_OENMODEL(x)	   (((x) & 0x3) << 10)
#define LVDS_ANALOG_CTL2_OEPMODEL_MASK 	 (0x3 << 8)
#define LVDS_ANALOG_CTL2_OEPMODEL(x)	   (((x) & 0x3) << 8)
/*bit 7 Reserved*/
#define LVDS_ANALOG_CTL2_POLARL			 	 	 (0x1 << 6)
#define LVDS_ANALOG_CTL2_ELVDSPOWL 			 (0x1 << 5)
#define LVDS_ANALOG_CTL2_OLVDSPOWL		 	 (0x1 << 4)
#define LVDS_ANALOG_CTL2_STTLIL				 	 (0x1 << 3)
#define LVDS_ANALOG_CTL2_ECKPOLARL		 	 (0x1 << 2)
#define LVDS_ANALOG_CTL2_OCKPOLARL		 	 (0x1 << 1)
#define LVDS_ANALOG_CTL2_ENVBPBL			 	 (0x1 << 0)


/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
