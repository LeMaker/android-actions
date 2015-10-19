/**
 * core.h - DesignWare USB3 DRD Core Header
 *
 * Copyright (C) 2010-2011 Texas Instruments Incorporated - http://www.ti.com
 *
 * Authors: Felipe Balbi <balbi@ti.com>,
 *	    Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2, as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */



#include <linux/usb/ch9.h>
#define usb_ep	usb_ep_boot
#define usb_request	usb_request_boot

#include <linux/usb/gadget.h>

//#undef 	usb_ep
//#undef 	usb_request

#ifndef __DRIVERS_USB_DWC3_CORE_H
#define __DRIVERS_USB_DWC3_CORE_H
//--------------USB3_Register-------------------------------------------//
//--------------Register Address---------------------------------------//
#define DWC3_BASE (0xb0400000)
#define ANA00 (0+0xCD00)
#define ANA01 (0+0xCD04)
#define ANA02 (0+0xCD08)
#define ANA03 (0+0xCD0C)
#define ANA04 (0+0xCD10)

#define ANA05 (0+0xCD14)
#define ANA06 (0+0xCD18)
#define ANA07 (0+0xCD1C)
#define ANA08 (0+0xCD20)
#define ANA09 (0+0xCD24)
#define ANA0A (0+0xCD28)
#define ANA0B (0+0xCD2C)
#define ANA0C (0+0xCD30)
#define ANA0D (0+0xCD34)
#define ANA0E (0+0xCD38)
#define ANA0F (0+0xCD3C)
#define DMR (0+0xCD40)
#define BACR (0+0xCD44)
#define IER (0+0xCD48)
#define BCSR (0+0xCD4C)
#define BPR (0+0xCD50)
#define BPNR2 (0+0xCD54)
#define BFNR (0+0xCD58)
#define BRNR2 (0+0xCD5C)
#define BENR (0+0xCD60)
#define REV0 (0+0xCD64)
#define REV1 (0+0xCD68)
#define REV2 (0+0xCD6C)
#define REV3 (0+0xCD70)
#define FLD0 (0+0xCD74)
#define FLD1 (0+0xCD78)
#define ANA1F (0+0xCD7C)
#define PAGE1_REG00 (0+0xCD80)
#define PAGE1_REG01 (0+0xCD84)
#define PAGE1_REG02 (0+0xCD88)
#define PAGE1_REG03 (0+0xCD8C)
#define PAGE1_REG04 (0+0xCD90)
#define PAGE1_REG05 (0+0xCD94)
#define PAGE1_REG06 (0+0xCD98)
#define PAGE1_REG07 (0+0xCD9C)
#define PAGE1_REG08 (0+0xCDA0)
#define PAGE1_REG09 (0+0xCDA4)
#define PAGE1_REG0A (0+0xCDA8)
#define PAGE1_REG0B (0+0xCDAC)
#define PAGE1_REG0C (0+0xCDB0)
#define PAGE1_REG0D (0+0xCDB4)
#define PAGE1_REG0E (0+0xCDB8)
#define PAGE1_REG0F (0+0xCDBC)
#define PAGE1_REG10 (0+0xCDC0)
#define USB2_P0_VDCTRL (0+0xCE00)
#define BACKDOOR (0+0xCE04)
#define EXT_CONTROL (0+0xCE08)
#define EFUSE_CTR (0+0xCE0C)
#define USB2_P1_VDCTRL (0+0xCE10) 

//#define USB3_USB2_P0_VDCTRL                         (DWC3_BASE+0xCD48)

#define USB3_ACTIONS_START        (0xcd00)
#define USB3_ACTIONS_END          (0xcd58)

#define DWC3_CDR_KIKD          (0x00)
#define DWC3_CDR_KP1           (0x04)
#define DWC3_TIMER_INIT        (0x08)
#define DWC3_CDR_CONTROL       (0x0c)
#define DWC3_RX_OFFSET_PS      (0x10)
#define DWC3_EQ_CONTROL        (0x14)
#define DWC3_RX_OOBS_SSC0      (0x18)
#define DWC3_CMU_SSC1          (0x1C)
#define DWC3_CMU_DEBUG_LDO     (0x20)
#define DWC3_TX_AMP_DEBUG      (0x24)
#define DWC3_Z0                (0x28)
#define DWC3_DMR_BACR          (0x2C)
#define DWC3_IER_BCSR          (0x30)
#define DWC3_BPR               (0x34)
#define DWC3_BFNR              (0x38)
#define DWC3_BENR_REV          (0x3C)
#define DWC3_FLD               (0x40)
#define DWC3_CMU_PLL2_BISTDEBUG    (0x44)

#define USB3_MOD_RST           (1 << 14)
#define CMU_BIAS_EN            (1 << 20)

#define BIST_QINIT(n)          ((n) << 24)
#define EYE_HEIGHT(n)          ((n) << 20)
#define PLL2_LOCK              (1 << 15)
#define PLL2_RS(n)             ((n) << 12)
#define PLL2_ICP(n)            ((n) << 10)
#define CMU_SEL_PREDIV         (1 << 9)
#define CMU_DIVX2              (1 << 8)
#define PLL2_DIV(n)            ((n) << 3)
#define PLL2_POSTDIV(n)        ((n) << 1)
#define PLL2_PU                (1 << 0)

#define CMU_BASE                                    0xB0160000
#define CMU_USBPLL                                  (CMU_BASE+0x0080)
#define CMU_DEVRST1                                 (CMU_BASE+0x00AC)
//=============== USB3_P0_CTL define=======================
#define     USB3_P0_CTL_LDOVREFSEL_SHIFT_IC1                                      28
#define     USB3_P0_CTL_PLLLDOEN_IC1                                              31
#define     USB3_P0_CTL_LDOVREFSEL_E                                          30
#define     USB3_P0_CTL_LDOVREFSEL_SHIFT                                      29
#define     USB3_P0_CTL_LDOVREFSEL_MASK                                       (0x3<<29)
#define     USB3_P0_CTL_PLLLDOEN                                              28
#define     USB3_P0_CTL_PLUGIN                                                25
#define     USB3_P0_CTL_CONDET_EN                                             24
#define     USB3_P0_CTL_WKUPBVLDEN                                            23
#define     USB3_P0_CTL_WKUPDPEN                                              22
#define     USB3_P0_CTL_WKUPIDUSEN                                            21
#define     USB3_P0_CTL_WKUPVBUSEN                                            20
#define     USB3_P0_CTL_WKUPBVLD                                              19
#define     USB3_P0_CTL_WKUPDP                                                18
#define     USB3_P0_CTL_WKUPID                                                17
#define     USB3_P0_CTL_WKUPVBUS                                              16
#define     USB3_P0_CTL_DMPUEN_P0                                             15
#define     USB3_P0_CTL_DPPUEN_P0                                             14
#define     USB3_P0_CTL_DMPDDIS_P0                                            13
#define     USB3_P0_CTL_DPPDDIS_P0                                            12
#define     USB3_P0_CTL_ID_P0                                                 11
#define     USB3_P0_CTL_BVALID_P0                                             10
#define     USB3_P0_CTL_SOFTID_P0                                             9
#define     USB3_P0_CTL_SOFTIDEN_P0                                           8
#define     USB3_P0_CTL_SOFTVBUS_P0                                           7
#define     USB3_P0_CTL_SOFTVBUSEN_P0                                         6
#define     USB3_P0_CTL_VBUS_P0                                               5
#define     USB3_P0_CTL_LS_P0_E                                               4
#define     USB3_P0_CTL_LS_P0_SHIFT                                           3
#define     USB3_P0_CTL_LS_P0_MASK                                            (0x3<<3)
#define     USB3_P0_CTL_VBUSTH_P0_E                                           2
#define     USB3_P0_CTL_VBUSTH_P0_SHIFT                                       0
#define     USB3_P0_CTL_VBUSTH_P0_MASK                                        (0x7<<0)

/*********************************************************************************/
/* Global constants */
#define DWC3_EP0_BOUNCE_SIZE	512
#define DWC3_ENDPOINTS_NUM	32
#define DWC3_XHCI_RESOURCES_NUM	2

#define DWC3_EVENT_SIZE		4	/* bytes */
#define DWC3_EVENT_MAX_NUM	64	/* 2 events/endpoint */
#define DWC3_EVENT_BUFFERS_SIZE	(DWC3_EVENT_SIZE * DWC3_EVENT_MAX_NUM)
#define DWC3_EVENT_TYPE_MASK	0xfe

#define DWC3_EVENT_TYPE_DEV	0
#define DWC3_EVENT_TYPE_CARKIT	3
#define DWC3_EVENT_TYPE_I2C	4

#define DWC3_DEVICE_EVENT_DISCONNECT		0
#define DWC3_DEVICE_EVENT_RESET			1
#define DWC3_DEVICE_EVENT_CONNECT_DONE		2
#define DWC3_DEVICE_EVENT_LINK_STATUS_CHANGE	3
#define DWC3_DEVICE_EVENT_WAKEUP		4
#define DWC3_DEVICE_EVENT_HIBER_REQ		5
#define DWC3_DEVICE_EVENT_EOPF			6
#define DWC3_DEVICE_EVENT_SOF			7
#define DWC3_DEVICE_EVENT_ERRATIC_ERROR		9
#define DWC3_DEVICE_EVENT_CMD_CMPL		10
#define DWC3_DEVICE_EVENT_OVERFLOW		11

#define DWC3_GEVNTCOUNT_MASK	0xfffc
#define DWC3_GSNPSID_MASK	0xffff0000
#define DWC3_GSNPSREV_MASK	0xffff

/* DWC3 registers memory space boundries */
#define DWC3_XHCI_REGS_START		0x0
#define DWC3_XHCI_REGS_END		0x7fff
#define DWC3_GLOBALS_REGS_START		0xc100
#define DWC3_GLOBALS_REGS_END		0xc6ff
#define DWC3_DEVICE_REGS_START		0xc700
#define DWC3_DEVICE_REGS_END		0xcbff
#define DWC3_OTG_REGS_START		0xcc00
#define DWC3_OTG_REGS_END		0xccff

/* Global Registers */
#define DWC3_GSBUSCFG0		0xc100
#define DWC3_GSBUSCFG1		0xc104
#define DWC3_GTXTHRCFG		0xc108
#define DWC3_GRXTHRCFG		0xc10c
#define DWC3_GCTL		0xc110
#define DWC3_GEVTEN		0xc114
#define DWC3_GSTS		0xc118
#define DWC3_GSNPSID		0xc120
#define DWC3_GGPIO		0xc124
#define DWC3_GUID		0xc128
#define DWC3_GUCTL		0xc12c
#define DWC3_GBUSERRADDR0	0xc130
#define DWC3_GBUSERRADDR1	0xc134
#define DWC3_GPRTBIMAP0		0xc138
#define DWC3_GPRTBIMAP1		0xc13c
#define DWC3_GHWPARAMS0		0xc140
#define DWC3_GHWPARAMS1		0xc144
#define DWC3_GHWPARAMS2		0xc148
#define DWC3_GHWPARAMS3		0xc14c
#define DWC3_GHWPARAMS4		0xc150
#define DWC3_GHWPARAMS5		0xc154
#define DWC3_GHWPARAMS6		0xc158
#define DWC3_GHWPARAMS7		0xc15c
#define DWC3_GDBGFIFOSPACE		0xc160
#define DWC3_GDBGLTSSM			0xc164
#define DWC3_GPRTBIMAP_HS0	0xc180
#define DWC3_GPRTBIMAP_HS1	0xc184
#define DWC3_GPRTBIMAP_FS0		0xc188
#define DWC3_GPRTBIMAP_FS1		0xc18c

#define DWC3_GUSB2PHYCFG(n)	(0xc200 + (n * 0x04))
#define DWC3_GUSB2I2CCTL(n)		(0xc240 + (n * 0x04))

#define DWC3_GUSB2PHYACC(n)	(0xc280 + (n * 0x04))

#define DWC3_GUSB3PIPECTL(n)	(0xc2c0 + (n * 0x04))

#define DWC3_GTXFIFOSIZ(n)	(0xc300 + (n * 0x04))
#define DWC3_GRXFIFOSIZ(n)	(0xc380 + (n * 0x04))

#define DWC3_GEVNTADRLO(n)	(0xc400 + (n * 0x10))
#define DWC3_GEVNTADRHI(n)	(0xc404 + (n * 0x10))
#define DWC3_GEVNTSIZ(n)	(0xc408 + (n * 0x10))
#define DWC3_GEVNTCOUNT(n)	(0xc40c + (n * 0x10))

#define DWC3_GHWPARAMS8		0xc600

/* Device Registers */
#define DWC3_DCFG		0xc700
#define DWC3_DCTL		0xc704
#define DWC3_DEVTEN		0xc708
#define DWC3_DSTS		0xc70c
#define DWC3_DGCMDPAR		0xc710
#define DWC3_DGCMD		0xc714
#define DWC3_DALEPENA		0xc720
#define DWC3_DEPCMDPAR2(n)	(0xc800 + (n * 0x10))
#define DWC3_DEPCMDPAR1(n)	(0xc804 + (n * 0x10))
#define DWC3_DEPCMDPAR0(n)	(0xc808 + (n * 0x10))
#define DWC3_DEPCMD(n)		(0xc80c + (n * 0x10))

/* OTG Registers */
#define DWC3_OCFG		0xcc00
#define DWC3_OCTL		0xcc04
#define DWC3_OEVT		0xcc08
#define DWC3_OEVTEN		0xcc0C
#define DWC3_OSTS		0xcc10

#define DWC3_BACKDOOR   0xcd4c

#define DWC3_FLADJ_30MHZ_MASK	((0x3f) << 10)
#define DWC3_FLADJ_30MHZ(n)	((n) << 10)

/* Bit fields */

/* Global Configuration Register */
#define DWC3_GCTL_PWRDNSCALE(n)	((n) << 19)
#define DWC3_GCTL_U2RSTECN		(1 << 16)
#define DWC3_GCTL_RAMCLKSEL(x)	(((x) & DWC3_GCTL_CLK_MASK) << 6)
#define DWC3_GCTL_CLK_BUS	(0)
#define DWC3_GCTL_CLK_PIPE	(1)
#define DWC3_GCTL_CLK_PIPEHALF	(2)
#define DWC3_GCTL_CLK_MASK	(3)

#define DWC3_GCTL_PRTCAP(n)	(((n) & (3 << 12)) >> 12)
#define DWC3_GCTL_PRTCAPDIR(n)	((n) << 12)
#define DWC3_GCTL_PRTCAP_HOST	1
#define DWC3_GCTL_PRTCAP_DEVICE	2
#define DWC3_GCTL_PRTCAP_OTG	3

#define DWC3_GCTL_CORESOFTRESET		(1 << 11)
#define DWC3_GCTL_SCALEDOWN(n)		((n) << 4)
#define DWC3_GCTL_SCALEDOWN_MASK 	DWC3_GCTL_SCALEDOWN(3)
#define DWC3_GCTL_DISSCRAMBLE		(1 << 3)
#define DWC3_GCTL_GBLHIBERNATIONEN	(1 << 1)
#define DWC3_GCTL_DSBLCLKGTNG		(1 << 0)

/* Global USB2 PHY Configuration Register */
#define DWC3_GUSB2PHYCFG_PHYSOFTRST	(1 << 31)
#define DWC3_GUSB2PHYCFG_SUSPHY		(1 << 6)

/* Global USB3 PIPE Control Register */
#define DWC3_GUSB3PIPECTL_PHYSOFTRST	(1 << 31)
#define DWC3_GUSB3PIPECTL_SUSPHY	(1 << 17)

/* Global TX Fifo Size Register */
#define DWC3_GTXFIFOSIZ_TXFDEF(n)	((n) & 0xffff)
#define DWC3_GTXFIFOSIZ_TXFSTADDR(n)	((n) & 0xffff0000)

/* Global HWPARAMS1 Register */
#define DWC3_GHWPARAMS1_EN_PWROPT(n)	(((n) & (3 << 24)) >> 24)
#define DWC3_GHWPARAMS1_EN_PWROPT_NO	0
#define DWC3_GHWPARAMS1_EN_PWROPT_CLK	1
#define DWC3_GHWPARAMS1_EN_PWROPT_HIB	2
#define DWC3_GHWPARAMS1_PWROPT(n)	((n) << 24)
#define DWC3_GHWPARAMS1_PWROPT_MASK	DWC3_GHWPARAMS1_PWROPT(3)

/* Global HWPARAMS4 Register */
#define DWC3_GHWPARAMS4_HIBER_SCRATCHBUFS(n)	(((n) & (0x0f << 13)) >> 13)
#define DWC3_MAX_HIBER_SCRATCHBUFS		15

/* Device Configuration Register */
#define DWC3_DCFG_LPM_CAP	(1 << 22)
#define DWC3_DCFG_DEVADDR(addr)	((addr) << 3)
#define DWC3_DCFG_DEVADDR_MASK	DWC3_DCFG_DEVADDR(0x7f)

#define DWC3_DCFG_SPEED_MASK	(7 << 0)
#define DWC3_DCFG_SUPERSPEED	(4 << 0)
#define DWC3_DCFG_HIGHSPEED	(0 << 0)
#define DWC3_DCFG_FULLSPEED2	(1 << 0)
#define DWC3_DCFG_LOWSPEED	(2 << 0)
#define DWC3_DCFG_FULLSPEED1	(3 << 0)

#define DWC3_DCFG_LPM_CAP	(1 << 22)

/* Device Control Register */
#define DWC3_DCTL_RUN_STOP	(1 << 31)
#define DWC3_DCTL_CSFTRST	(1 << 30)
#define DWC3_DCTL_LSFTRST	(1 << 29)

#define DWC3_DCTL_HIRD_THRES_MASK	(0x1f << 24)
#define DWC3_DCTL_HIRD_THRES(n)	((n) << 24)

#define DWC3_DCTL_APPL1RES	(1 << 23)

/* These apply for core versions 1.87a and earlier */
#define DWC3_DCTL_TRGTULST_MASK		(0x0f << 17)
#define DWC3_DCTL_TRGTULST(n)		((n) << 17)
#define DWC3_DCTL_TRGTULST_U2		(DWC3_DCTL_TRGTULST(2))
#define DWC3_DCTL_TRGTULST_U3		(DWC3_DCTL_TRGTULST(3))
#define DWC3_DCTL_TRGTULST_SS_DIS	(DWC3_DCTL_TRGTULST(4))
#define DWC3_DCTL_TRGTULST_RX_DET	(DWC3_DCTL_TRGTULST(5))
#define DWC3_DCTL_TRGTULST_SS_INACT	(DWC3_DCTL_TRGTULST(6))

/* These apply for core versions 1.94a and later */
#define DWC3_DCTL_KEEP_CONNECT	(1 << 19)
#define DWC3_DCTL_L1_HIBER_EN	(1 << 18)
#define DWC3_DCTL_CRS		(1 << 17)
#define DWC3_DCTL_CSS		(1 << 16)

#define DWC3_DCTL_INITU2ENA	(1 << 12)
#define DWC3_DCTL_ACCEPTU2ENA	(1 << 11)
#define DWC3_DCTL_INITU1ENA	(1 << 10)
#define DWC3_DCTL_ACCEPTU1ENA	(1 << 9)
#define DWC3_DCTL_TSTCTRL_MASK	(0xf << 1)

#define DWC3_DCTL_ULSTCHNGREQ_MASK	(0x0f << 5)
#define DWC3_DCTL_ULSTCHNGREQ(n) (((n) << 5) & DWC3_DCTL_ULSTCHNGREQ_MASK)

#define DWC3_DCTL_ULSTCHNG_NO_ACTION	(DWC3_DCTL_ULSTCHNGREQ(0))
#define DWC3_DCTL_ULSTCHNG_SS_DISABLED	(DWC3_DCTL_ULSTCHNGREQ(4))
#define DWC3_DCTL_ULSTCHNG_RX_DETECT	(DWC3_DCTL_ULSTCHNGREQ(5))
#define DWC3_DCTL_ULSTCHNG_SS_INACTIVE	(DWC3_DCTL_ULSTCHNGREQ(6))
#define DWC3_DCTL_ULSTCHNG_RECOVERY	(DWC3_DCTL_ULSTCHNGREQ(8))
#define DWC3_DCTL_ULSTCHNG_COMPLIANCE	(DWC3_DCTL_ULSTCHNGREQ(10))
#define DWC3_DCTL_ULSTCHNG_LOOPBACK	(DWC3_DCTL_ULSTCHNGREQ(11))

/* Device Event Enable Register */
#define DWC3_DEVTEN_VNDRDEVTSTRCVEDEN	(1 << 12)
#define DWC3_DEVTEN_EVNTOVERFLOWEN	(1 << 11)
#define DWC3_DEVTEN_CMDCMPLTEN		(1 << 10)
#define DWC3_DEVTEN_ERRTICERREN		(1 << 9)
#define DWC3_DEVTEN_SOFEN		(1 << 7)
#define DWC3_DEVTEN_EOPFEN		(1 << 6)
#define DWC3_DEVTEN_HIBERNATIONREQEVTEN	(1 << 5)
#define DWC3_DEVTEN_WKUPEVTEN		(1 << 4)
#define DWC3_DEVTEN_ULSTCNGEN		(1 << 3)
#define DWC3_DEVTEN_CONNECTDONEEN	(1 << 2)
#define DWC3_DEVTEN_USBRSTEN		(1 << 1)
#define DWC3_DEVTEN_DISCONNEVTEN	(1 << 0)

/* Device Status Register */
#define DWC3_DSTS_DCNRD			(1 << 29)

/* This applies for core versions 1.87a and earlier */
#define DWC3_DSTS_PWRUPREQ		(1 << 24)

/* These apply for core versions 1.94a and later */
#define DWC3_DSTS_RSS			(1 << 25)
#define DWC3_DSTS_SSS			(1 << 24)

#define DWC3_DSTS_COREIDLE		(1 << 23)
#define DWC3_DSTS_DEVCTRLHLT		(1 << 22)

#define DWC3_DSTS_USBLNKST_MASK		(0x0f << 18)
#define DWC3_DSTS_USBLNKST(n)		(((n) & DWC3_DSTS_USBLNKST_MASK) >> 18)

#define DWC3_DSTS_RXFIFOEMPTY		(1 << 17)

#define DWC3_DSTS_SOFFN_MASK		(0x3fff << 3)
#define DWC3_DSTS_SOFFN(n)		(((n) & DWC3_DSTS_SOFFN_MASK) >> 3)

#define DWC3_DSTS_CONNECTSPD		(7 << 0)

#define DWC3_DSTS_SUPERSPEED		(4 << 0)
#define DWC3_DSTS_HIGHSPEED		(0 << 0)
#define DWC3_DSTS_FULLSPEED2		(1 << 0)
#define DWC3_DSTS_LOWSPEED		(2 << 0)
#define DWC3_DSTS_FULLSPEED1		(3 << 0)

/* Device Generic Command Register */
#define DWC3_DGCMD_SET_LMP		0x01
#define DWC3_DGCMD_SET_PERIODIC_PAR	0x02
#define DWC3_DGCMD_XMIT_FUNCTION	0x03

/* These apply for core versions 1.94a and later */
#define DWC3_DGCMD_SET_SCRATCHPAD_ADDR_LO	0x04
#define DWC3_DGCMD_SET_SCRATCHPAD_ADDR_HI	0x05

#define DWC3_DGCMD_SELECTED_FIFO_FLUSH	0x09
#define DWC3_DGCMD_ALL_FIFO_FLUSH	0x0a
#define DWC3_DGCMD_SET_ENDPOINT_NRDY	0x0c
#define DWC3_DGCMD_RUN_SOC_BUS_LOOPBACK	0x10

#define DWC3_DGCMD_STATUS(n)		(((n) >> 15) & 1)
#define DWC3_DGCMD_CMDACT		(1 << 10)
#define DWC3_DGCMD_CMDIOC		(1 << 8)

/* Device Generic Command Parameter Register */
#define DWC3_DGCMDPAR_FORCE_LINKPM_ACCEPT	(1 << 0)
#define DWC3_DGCMDPAR_FIFO_NUM(n)		((n) << 0)
#define DWC3_DGCMDPAR_RX_FIFO			(0 << 5)
#define DWC3_DGCMDPAR_TX_FIFO			(1 << 5)
#define DWC3_DGCMDPAR_LOOPBACK_DIS		(0 << 0)
#define DWC3_DGCMDPAR_LOOPBACK_ENA		(1 << 0)

/* Device Endpoint Command Register */
#define DWC3_DEPCMD_PARAM_SHIFT		16
#define DWC3_DEPCMD_PARAM(x)		((x) << DWC3_DEPCMD_PARAM_SHIFT)
#define DWC3_DEPCMD_GET_RSC_IDX(x)     (((x) >> DWC3_DEPCMD_PARAM_SHIFT) & 0x7f)
#define DWC3_DEPCMD_STATUS(x)		(((x) >> 15) & 1)
#define DWC3_DEPCMD_HIPRI_FORCERM	(1 << 11)
#define DWC3_DEPCMD_CMDACT		(1 << 10)
#define DWC3_DEPCMD_CMDIOC		(1 << 8)

#define DWC3_DEPCMD_DEPSTARTCFG		(0x09 << 0)
#define DWC3_DEPCMD_ENDTRANSFER		(0x08 << 0)
#define DWC3_DEPCMD_UPDATETRANSFER	(0x07 << 0)
#define DWC3_DEPCMD_STARTTRANSFER	(0x06 << 0)
#define DWC3_DEPCMD_CLEARSTALL		(0x05 << 0)
#define DWC3_DEPCMD_SETSTALL		(0x04 << 0)
/* This applies for core versions 1.90a and earlier */
#define DWC3_DEPCMD_GETSEQNUMBER	(0x03 << 0)
/* This applies for core versions 1.94a and later */
#define DWC3_DEPCMD_GETEPSTATE		(0x03 << 0)
#define DWC3_DEPCMD_SETTRANSFRESOURCE	(0x02 << 0)
#define DWC3_DEPCMD_SETEPCONFIG		(0x01 << 0)

/* The EP number goes 0..31 so ep0 is always out and ep1 is always in */
#define DWC3_DALEPENA_EP(n)		(1 << n)

#define DWC3_DEPCMD_TYPE_CONTROL	0
#define DWC3_DEPCMD_TYPE_ISOC		1
#define DWC3_DEPCMD_TYPE_BULK		2
#define DWC3_DEPCMD_TYPE_INTR		3

#define BIT(x)		(1 << (x))


/* Structures */


/**
 * struct usb_ep - device side representation of USB endpoint
 * @name:identifier for the endpoint, such as "ep-a" or "ep9in-bulk"
 * @ops: Function pointers used to access hardware-specific operations.
 * @ep_list:the gadget's ep_list holds all of its endpoints
 * @maxpacket:The maximum packet size used on this endpoint.  The initial
 *	value can sometimes be reduced (hardware allowing), according to
 *      the endpoint descriptor used to configure the endpoint.
 * @max_streams: The maximum number of streams supported
 *	by this EP (0 - 16, actual number is 2^n)
 * @mult: multiplier, 'mult' value for SS Isoc EPs
 * @maxburst: the maximum number of bursts supported by this EP (for usb3)
 * @driver_data:for use by the gadget driver.
 * @address: used to identify the endpoint when finding descriptor that
 *	matches connection speed
 * @desc: endpoint descriptor.  This pointer is set before the endpoint is
 *	enabled and remains valid until the endpoint is disabled.
 * @comp_desc: In case of SuperSpeed support, this is the endpoint companion
 *	descriptor that is used to configure the endpoint
 *
 * the bus controller driver lists all the general purpose endpoints in
 * gadget->ep_list.  the control endpoint (gadget->ep0) is not in that list,
 * and is accessed only in response to a driver setup() callback.
 */
 #if 0
 #undef 	usb_ep
struct usb_ep {
	void			*driver_data;

	const char		*name;
	const struct usb_ep_ops	*ops;
	struct list_head	ep_list;
	unsigned		maxpacket:16;
	unsigned		max_streams:16;
	unsigned		mult:2;
	unsigned		maxburst:5;
	u8			address;
	const struct usb_endpoint_descriptor	*desc;
	const struct usb_ss_ep_comp_descriptor	*comp_desc;
};
 
 #undef 	usb_request
struct scatterlist {
	unsigned long	page_link;
	unsigned int	offset;
	unsigned int	length;
	dma_addr_t	dma_address;
};


struct usb_request {
	void			*buf;
	unsigned		length;
	dma_addr_t		dma;
    
	unsigned		no_interrupt:1;
	unsigned		zero:1;
	unsigned		short_not_ok:1;

	void			(*complete)(struct usb_ep *ep,
					struct usb_request *req);
	void			*context;
	struct list_head	list;

	int			status;
	unsigned		actual;
    
/*************add for dwc3***************/
    	struct scatterlist	*sg;
	unsigned		num_sgs;
	unsigned		num_mapped_sgs;

	unsigned		stream_id:16;
};

#endif

struct dwc3_trb;

/**
 * struct dwc3_event_buffer - Software event buffer representation
 * @list: a list of event buffers
 * @buf: _THE_ buffer
 * @length: size of this buffer
 * @lpos: event offset
 * @count: cache of last read event count register
 * @flags: flags related to this event buffer
 * @dma: dma_addr_t
 * @dwc: pointer to DWC controller
 */
struct dwc3_event_buffer {
	void			*buf;
	unsigned		length;
	unsigned int		lpos;
	unsigned int		count;
	unsigned int		flags;

#define DWC3_EVENT_PENDING	BIT(0)

	dma_addr_t		dma;

	struct dwc3		*dwc;
};

#define DWC3_EP_FLAG_STALLED	(1 << 0)
#define DWC3_EP_FLAG_WEDGED	(1 << 1)

#define DWC3_EP_DIRECTION_TX	true
#define DWC3_EP_DIRECTION_RX	false

#define DWC3_TRB_NUM		32
#define DWC3_TRB_MASK		(DWC3_TRB_NUM - 1)

/**
 * struct dwc3_ep - device side endpoint representation
 * @endpoint: usb endpoint
 * @request_list: list of requests for this endpoint
 * @req_queued: list of requests on this ep which have TRBs setup
 * @trb_pool: array of transaction buffers
 * @trb_pool_dma: dma address of @trb_pool
 * @free_slot: next slot which is going to be used
 * @busy_slot: first slot which is owned by HW
 * @desc: usb_endpoint_descriptor pointer
 * @dwc: pointer to DWC controller
 * @flags: endpoint flags (wedged, stalled, ...)
 * @current_trb: index of current used trb
 * @number: endpoint number (1 - 15)
 * @type: set to bmAttributes & USB_ENDPOINT_XFERTYPE_MASK
 * @resource_index: Resource transfer index
 * @interval: the intervall on which the ISOC transfer is started
 * @name: a human readable name e.g. ep1out-bulk
 * @direction: true for TX, false for RX
 * @stream_capable: true when streams are enabled
 */
struct dwc3_ep {
	struct usb_ep		endpoint;
	struct list_head	request_list;
	struct list_head	req_queued;

	struct dwc3_trb		*trb_pool;
	dma_addr_t		trb_pool_dma;
	u32			free_slot;
	u32			busy_slot;
	const struct usb_ss_ep_comp_descriptor *comp_desc;
	struct dwc3		*dwc;

	unsigned		flags;
#define DWC3_EP_ENABLED		(1 << 0)
#define DWC3_EP_STALL		(1 << 1)
#define DWC3_EP_WEDGE		(1 << 2)
#define DWC3_EP_BUSY		(1 << 4)
#define DWC3_EP_PENDING_REQUEST	(1 << 5)
#define DWC3_EP_MISSED_ISOC	(1 << 6)

	/* This last one is specific to EP0 */
#define DWC3_EP0_DIR_IN		(1 << 31)

	unsigned		current_trb;

	u8			number;
	u8			type;
	u8			resource_index;
	u32			interval;

	char			name[20];

	unsigned		direction:1;
	unsigned		stream_capable:1;
};

enum dwc3_phy {
	DWC3_PHY_UNKNOWN = 0,
	DWC3_PHY_USB3,
	DWC3_PHY_USB2,
};

enum dwc3_ep0_next {
	DWC3_EP0_UNKNOWN = 0,
	DWC3_EP0_COMPLETE,
	DWC3_EP0_NRDY_DATA,
	DWC3_EP0_NRDY_STATUS,
};

enum dwc3_ep0_state {
	EP0_UNCONNECTED		= 0,
	EP0_SETUP_PHASE,
	EP0_DATA_PHASE,
	EP0_STATUS_PHASE,
};

enum dwc3_link_state {
	/* In SuperSpeed */
	DWC3_LINK_STATE_U0		= 0x00, /* in HS, means ON */
	DWC3_LINK_STATE_U1		= 0x01,
	DWC3_LINK_STATE_U2		= 0x02, /* in HS, means SLEEP */
	DWC3_LINK_STATE_U3		= 0x03, /* in HS, means SUSPEND */
	DWC3_LINK_STATE_SS_DIS		= 0x04,
	DWC3_LINK_STATE_RX_DET		= 0x05, /* in HS, means Early Suspend */
	DWC3_LINK_STATE_SS_INACT	= 0x06,
	DWC3_LINK_STATE_POLL		= 0x07,
	DWC3_LINK_STATE_RECOV		= 0x08,
	DWC3_LINK_STATE_HRESET		= 0x09,
	DWC3_LINK_STATE_CMPLY		= 0x0a,
	DWC3_LINK_STATE_LPBK		= 0x0b,
	DWC3_LINK_STATE_RESET		= 0x0e,
	DWC3_LINK_STATE_RESUME		= 0x0f,
	DWC3_LINK_STATE_MASK		= 0x0f,
};

/* TRB Length, PCM and Status */
#define DWC3_TRB_SIZE_MASK	(0x00ffffff)
#define DWC3_TRB_SIZE_LENGTH(n)	((n) & DWC3_TRB_SIZE_MASK)
#define DWC3_TRB_SIZE_PCM1(n)	(((n) & 0x03) << 24)
#define DWC3_TRB_SIZE_TRBSTS(n)	(((n) & (0x0f << 28)) >> 28)

#define DWC3_TRBSTS_OK			0
#define DWC3_TRBSTS_MISSED_ISOC		1
#define DWC3_TRBSTS_SETUP_PENDING	2
#define DWC3_TRB_STS_XFER_IN_PROG	4

/* TRB Control */
#define DWC3_TRB_CTRL_HWO		(1 << 0)
#define DWC3_TRB_CTRL_LST		(1 << 1)
#define DWC3_TRB_CTRL_CHN		(1 << 2)
#define DWC3_TRB_CTRL_CSP		(1 << 3)
#define DWC3_TRB_CTRL_TRBCTL(n)		(((n) & 0x3f) << 4)
#define DWC3_TRB_CTRL_ISP_IMI		(1 << 10)
#define DWC3_TRB_CTRL_IOC		(1 << 11)
#define DWC3_TRB_CTRL_SID_SOFN(n)	(((n) & 0xffff) << 14)

#define DWC3_TRBCTL_NORMAL		DWC3_TRB_CTRL_TRBCTL(1)
#define DWC3_TRBCTL_CONTROL_SETUP	DWC3_TRB_CTRL_TRBCTL(2)
#define DWC3_TRBCTL_CONTROL_STATUS2	DWC3_TRB_CTRL_TRBCTL(3)
#define DWC3_TRBCTL_CONTROL_STATUS3	DWC3_TRB_CTRL_TRBCTL(4)
#define DWC3_TRBCTL_CONTROL_DATA	DWC3_TRB_CTRL_TRBCTL(5)
#define DWC3_TRBCTL_ISOCHRONOUS_FIRST	DWC3_TRB_CTRL_TRBCTL(6)
#define DWC3_TRBCTL_ISOCHRONOUS		DWC3_TRB_CTRL_TRBCTL(7)
#define DWC3_TRBCTL_LINK_TRB		DWC3_TRB_CTRL_TRBCTL(8)

/**
 * struct dwc3_trb - transfer request block (hw format)
 * @bpl: DW0-3
 * @bph: DW4-7
 * @size: DW8-B
 * @trl: DWC-F
 */
struct dwc3_trb {
	u32		bpl;
	u32		bph;
	u32		size;
	u32		ctrl;
} __packed;

/**
 * dwc3_hwparams - copy of HWPARAMS registers
 * @hwparams0 - GHWPARAMS0
 * @hwparams1 - GHWPARAMS1
 * @hwparams2 - GHWPARAMS2
 * @hwparams3 - GHWPARAMS3
 * @hwparams4 - GHWPARAMS4
 * @hwparams5 - GHWPARAMS5
 * @hwparams6 - GHWPARAMS6
 * @hwparams7 - GHWPARAMS7
 * @hwparams8 - GHWPARAMS8
 */
struct dwc3_hwparams {
	u32	hwparams0;
	u32	hwparams1;
	u32	hwparams2;
	u32	hwparams3;
	u32	hwparams4;
	u32	hwparams5;
	u32	hwparams6;
	u32	hwparams7;
	u32	hwparams8;
};

/* HWPARAMS0 */
#define DWC3_MODE(n)		((n) & 0x7)

#define DWC3_MODE_DEVICE	0
#define DWC3_MODE_HOST		1
#define DWC3_MODE_DRD		2
#define DWC3_MODE_HUB		3

#define DWC3_MDWIDTH(n)		(((n) & 0xff00) >> 8)

/* HWPARAMS1 */
#define DWC3_NUM_INT(n)		(((n) & (0x3f << 15)) >> 15)

/* HWPARAMS3 */
#define DWC3_NUM_IN_EPS_MASK	(0x1f << 18)
#define DWC3_NUM_EPS_MASK	(0x3f << 12)
#define DWC3_NUM_EPS(p)		(((p)->hwparams3 &		\
			(DWC3_NUM_EPS_MASK)) >> 12)
#define DWC3_NUM_IN_EPS(p)	(((p)->hwparams3 &		\
			(DWC3_NUM_IN_EPS_MASK)) >> 18)

/* HWPARAMS7 */
#define DWC3_RAM1_DEPTH(n)	((n) & 0xffff)

struct dwc3_request {
	struct usb_request	request;
	struct list_head	list;
	struct dwc3_ep		*dep;
	u32			start_slot;

	u8			epnum;
	struct dwc3_trb		*trb;
	dma_addr_t		trb_dma;

	unsigned		direction:1;
	unsigned		mapped:1;
	unsigned		queued:1;
};

/*
 * struct dwc3_scratchpad_array - hibernation scratchpad array
 * (format defined by hw)
 */
struct dwc3_scratchpad_array {
	__le64	dma_adr[DWC3_MAX_HIBER_SCRATCHBUFS];
};

/**
 * struct dwc3 - representation of our controller
 * @ctrl_req: usb control request which is used for ep0
 * @ep0_trb: trb which is used for the ctrl_req
 * @ep0_bounce: bounce buffer for ep0
 * @setup_buf: used while precessing STD USB requests
 * @ctrl_req_addr: dma address of ctrl_req
 * @ep0_trb: dma address of ep0_trb
 * @ep0_usb_req: dummy req used while handling STD USB requests
 * @ep0_bounce_addr: dma address of ep0_bounce
 * @lock: for synchronizing
 * @dev: pointer to our struct device
 * @xhci: pointer to our xHCI child
 * @event_buffer_list: a list of event buffers
 * @gadget: device side representation of the peripheral controller
 * @gadget_driver: pointer to the gadget driver
 * @regs: base address for our registers
 * @regs_size: address space size
 * @num_event_buffers: calculated number of event buffers
 * @u1u2: only used on revisions <1.83a for workaround
 * @maximum_speed: maximum speed requested (mainly for testing purposes)
 * @revision: revision register contents
 * @mode: mode of operation
 * @usb2_phy: pointer to USB2 PHY
 * @usb3_phy: pointer to USB3 PHY
 * @dcfg: saved contents of DCFG register
 * @gctl: saved contents of GCTL register
 * @is_selfpowered: true when we are selfpowered
 * @three_stage_setup: set if we perform a three phase setup
 * @ep0_bounced: true when we used bounce buffer
 * @ep0_expect_in: true when we expect a DATA IN transfer
 * @start_config_issued: true when StartConfig command has been issued
 * @setup_packet_pending: true when there's a Setup Packet in FIFO. Workaround
 * @needs_fifo_resize: not all users might want fifo resizing, flag it
 * @resize_fifos: tells us it's ok to reconfigure our TxFIFO sizes.
 * @isoch_delay: wValue from Set Isochronous Delay request;
 * @u2sel: parameter from Set SEL request.
 * @u2pel: parameter from Set SEL request.
 * @u1sel: parameter from Set SEL request.
 * @u1pel: parameter from Set SEL request.
 * @num_out_eps: number of out endpoints
 * @num_in_eps: number of in endpoints
 * @ep0_next_event: hold the next expected event
 * @ep0state: state of endpoint zero
 * @link_state: link state
 * @speed: device speed (super, high, full, low)
 * @mem: points to start of memory which is used for this struct.
 * @hwparams: copy of hwparams registers
 * @root: debugfs root folder pointer
 */
//typedef int	spinlock_t;
struct dwc3 {
	struct usb_ctrlrequest	*ctrl_req;
	struct dwc3_trb		*ep0_trb;
	void			*ep0_bounce;
	u8			*setup_buf;
	dma_addr_t		ctrl_req_addr;
	dma_addr_t		ep0_trb_addr;
	dma_addr_t		ep0_bounce_addr;
	struct dwc3_request	ep0_usb_req;

	/* device lock */
	spinlock_t		lock;

	struct device		*dev;
    
	enum usb_device_state 	state;
	//struct platform_device	*xhci;
	//struct resource		xhci_resources[DWC3_XHCI_RESOURCES_NUM];

	struct dwc3_event_buffer **ev_buffs;
	struct dwc3_ep		*eps[DWC3_ENDPOINTS_NUM];

	struct usb_gadget	gadget;
	struct usb_gadget_driver *gadget_driver;

	//struct usb_phy		*usb2_phy;
	//struct usb_phy		*usb3_phy;

	void __iomem		*regs;
	size_t			regs_size;

	/* used for suspend/resume */
	u32			dcfg;
	u32			gctl;

	u32			num_event_buffers;
	u32			u1u2;
	u32			maximum_speed;
	u32			revision;
	u32			mode;

#define DWC3_REVISION_173A	0x5533173a
#define DWC3_REVISION_175A	0x5533175a
#define DWC3_REVISION_180A	0x5533180a
#define DWC3_REVISION_183A	0x5533183a
#define DWC3_REVISION_185A	0x5533185a
#define DWC3_REVISION_187A	0x5533187a
#define DWC3_REVISION_188A	0x5533188a
#define DWC3_REVISION_190A	0x5533190a
#define DWC3_REVISION_194A	0x5533194a
#define DWC3_REVISION_200A	0x5533200a
#define DWC3_REVISION_202A	0x5533202a
#define DWC3_REVISION_210A	0x5533210a
#define DWC3_REVISION_220A	0x5533220a
#define DWC3_REVISION_230A	0x5533230a
#define DWC3_REVISION_240A	0x5533240a
#define DWC3_REVISION_250A	0x5533250a

	unsigned		is_selfpowered:1;
	unsigned		three_stage_setup:1;
	unsigned		ep0_bounced:1;
	unsigned		ep0_expect_in:1;
	unsigned		start_config_issued:1;
	unsigned		setup_packet_pending:1;
	unsigned		delayed_status:1;
	unsigned		needs_fifo_resize:1;
	unsigned		resize_fifos:1;
	unsigned		pullups_connected:1;

	enum dwc3_ep0_next	ep0_next_event;
	enum dwc3_ep0_state	ep0state;
	enum dwc3_link_state	link_state;

	u16			isoch_delay;
	u16			u2sel;
	u16			u2pel;
	u8			u1sel;
	u8			u1pel;

	u8			speed;

	u8			num_out_eps;
	u8			num_in_eps;

	void			*mem;

	struct dwc3_hwparams	hwparams;
	//struct dentry		*root;
	//struct debugfs_regset32	*regset;

	u8			test_mode;
	u8			test_mode_nr;
};

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */

struct dwc3_event_type {
	u32	is_devspec:1;
	u32	type:7;
	u32	reserved8_31:24;
} __packed;
struct dwc3_gadget_ep_cmd_params {
	u32	param2;
	u32	param1;
	u32	param0;
};
#define DWC3_DEPEVT_XFERCOMPLETE	0x01
#define DWC3_DEPEVT_XFERINPROGRESS	0x02
#define DWC3_DEPEVT_XFERNOTREADY	0x03
#define DWC3_DEPEVT_RXTXFIFOEVT		0x04
#define DWC3_DEPEVT_STREAMEVT		0x06
#define DWC3_DEPEVT_EPCMDCMPLT		0x07

/**
 * struct dwc3_event_depvt - Device Endpoint Events
 * @one_bit: indicates this is an endpoint event (not used)
 * @endpoint_number: number of the endpoint
 * @endpoint_event: The event we have:
 *	0x00	- Reserved
 *	0x01	- XferComplete
 *	0x02	- XferInProgress
 *	0x03	- XferNotReady
 *	0x04	- RxTxFifoEvt (IN->Underrun, OUT->Overrun)
 *	0x05	- Reserved
 *	0x06	- StreamEvt
 *	0x07	- EPCmdCmplt
 * @reserved11_10: Reserved, don't use.
 * @status: Indicates the status of the event. Refer to databook for
 *	more information.
 * @parameters: Parameters of the current event. Refer to databook for
 *	more information.
 */
struct dwc3_event_depevt {
	u32	one_bit:1;
	u32	endpoint_number:5;
	u32	endpoint_event:4;
	u32	reserved11_10:2;
	u32	status:4;

/* Within XferNotReady */
#define DEPEVT_STATUS_TRANSFER_ACTIVE	(1 << 3)

/* Within XferComplete */
#define DEPEVT_STATUS_BUSERR	(1 << 0)
#define DEPEVT_STATUS_SHORT	(1 << 1)
#define DEPEVT_STATUS_IOC	(1 << 2)
#define DEPEVT_STATUS_LST	(1 << 3)

/* Stream event only */
#define DEPEVT_STREAMEVT_FOUND		1
#define DEPEVT_STREAMEVT_NOTFOUND	2

/* Control-only Status */
#define DEPEVT_STATUS_CONTROL_DATA	1
#define DEPEVT_STATUS_CONTROL_STATUS	2

	u32	parameters:16;
} __packed;

/**
 * struct dwc3_event_devt - Device Events
 * @one_bit: indicates this is a non-endpoint event (not used)
 * @device_event: indicates it's a device event. Should read as 0x00
 * @type: indicates the type of device event.
 *	0	- DisconnEvt
 *	1	- USBRst
 *	2	- ConnectDone
 *	3	- ULStChng
 *	4	- WkUpEvt
 *	5	- Reserved
 *	6	- EOPF
 *	7	- SOF
 *	8	- Reserved
 *	9	- ErrticErr
 *	10	- CmdCmplt
 *	11	- EvntOverflow
 *	12	- VndrDevTstRcved
 * @reserved15_12: Reserved, not used
 * @event_info: Information about this event
 * @reserved31_24: Reserved, not used
 */
struct dwc3_event_devt {
	u32	one_bit:1;
	u32	device_event:7;
	u32	type:4;
	u32	reserved15_12:4;
	u32	event_info:8;
	u32	reserved31_24:8;
} __packed;

/**
 * struct dwc3_event_gevt - Other Core Events
 * @one_bit: indicates this is a non-endpoint event (not used)
 * @device_event: indicates it's (0x03) Carkit or (0x04) I2C event.
 * @phy_port_number: self-explanatory
 * @reserved31_12: Reserved, not used.
 */
struct dwc3_event_gevt {
	u32	one_bit:1;
	u32	device_event:7;
	u32	phy_port_number:4;
	u32	reserved31_12:20;
} __packed;

/**
 * union dwc3_event - representation of Event Buffer contents
 * @raw: raw 32-bit event
 * @type: the type of the event
 * @depevt: Device Endpoint Event
 * @devt: Device Event
 * @gevt: Global Event
 */
union dwc3_event {
	u32				raw;
	struct dwc3_event_type		type;
	struct dwc3_event_depevt	depevt;
	struct dwc3_event_devt		devt;
	struct dwc3_event_gevt		gevt;
};

/*
 * DWC3 Features to be used as Driver Data
 */

#define DWC3_HAS_PERIPHERAL		BIT(0)
#define DWC3_HAS_XHCI			BIT(1)
#define DWC3_HAS_OTG			BIT(3)

/*
 * USB function drivers should return USB_GADGET_DELAYED_STATUS if they
 * wish to delay the data/status stages of the control transfer till they
 * are ready. The control transfer will then be kept from completing till
 * all the function drivers that requested for USB_GADGET_DELAYED_STAUS
 * invoke usb_composite_setup_continue().
 */
#define USB_GADGET_DELAYED_STATUS       0x7fff	/* Impossibly large value */

/* big enough to hold our biggest descriptor */
#define USB_COMP_EP0_BUFSIZ	1024

#define USB_MS_TO_HS_INTERVAL(x)	(ilog2((x * 1000 / 125)) + 1)

/* DEPCFG parameter 1 */
#define DWC3_DEPCFG_INT_NUM(n)		((n) << 0)
#define DWC3_DEPCFG_XFER_COMPLETE_EN	(1 << 8)
#define DWC3_DEPCFG_XFER_IN_PROGRESS_EN	(1 << 9)
#define DWC3_DEPCFG_XFER_NOT_READY_EN	(1 << 10)
#define DWC3_DEPCFG_FIFO_ERROR_EN	(1 << 11)
#define DWC3_DEPCFG_STREAM_EVENT_EN	(1 << 13)
#define DWC3_DEPCFG_BINTERVAL_M1(n)	((n) << 16)
#define DWC3_DEPCFG_STREAM_CAPABLE	(1 << 24)
#define DWC3_DEPCFG_EP_NUMBER(n)	((n) << 25)
#define DWC3_DEPCFG_BULK_BASED		(1 << 30)
#define DWC3_DEPCFG_FIFO_BASED		(1 << 31)

/* DEPCFG parameter 0 */
#define DWC3_DEPCFG_EP_TYPE(n)		((n) << 1)
#define DWC3_DEPCFG_MAX_PACKET_SIZE(n)	((n) << 3)
#define DWC3_DEPCFG_FIFO_NUMBER(n)	((n) << 17)
#define DWC3_DEPCFG_BURST_SIZE(n)	((n) << 22)
#define DWC3_DEPCFG_DATA_SEQ_NUM(n)	((n) << 26)
/* This applies for core versions earlier than 1.94a */
#define DWC3_DEPCFG_IGN_SEQ_NUM		(1 << 31)
/* These apply for core versions 1.94a and later */
#define DWC3_DEPCFG_ACTION_INIT		(0 << 30)
#define DWC3_DEPCFG_ACTION_RESTORE	(1 << 30)
#define DWC3_DEPCFG_ACTION_MODIFY	(2 << 30)

/* DEPXFERCFG parameter 0 */
#define DWC3_DEPXFERCFG_NUM_XFER_RES(n)	((n) & 0xffff)

#define DWC3_EVENT_PENDING	BIT(0)

#define USB_IRQ_ID  55
#define	DMA_BASE_ADDR		0xB406C000
#define	REQ_DMA_ADDR_OFFSET		(1024*16)
#define	EP_BUF_LEN		(1024*5)
#define	EP_BUF_NUM		(3)
/**
 * enum irqreturn
 * @IRQ_NONE		interrupt was not from this device
 * @IRQ_HANDLED		interrupt was handled by this device
 * @IRQ_WAKE_THREAD	handler requests to wake the handler thread
 
 #ifdef IRQ_HANDLED
 #undef IRQ_HANDLED
 #endif
enum irqreturn {
	IRQ_NONE		= (0 << 0),
	IRQ_HANDLED		= (1 << 0),
	IRQ_WAKE_THREAD		= (1 << 1)
};

typedef enum irqreturn irqreturn_t;


#define dev_err(...) 
#define dev_dbg(...) 
#define dev_vdbg(...) */
//#define WARN_ON_ONCE(...) 
#define WARN_ON_ONCE(condition) do { \
  if (unlikely((condition)!=0)) { \
		 printk("Badness in %s at %s:%d\n", __FUNCTION__, __FILE__, __LINE__); \
	} \
 } while (0)
 
#define WARN(...) 
#define dev_WARN(...) 
//#define dev_WARN_ONCE(...)	                      
#define dev_WARN_ONCE(dev, condition, format, arg...) do { \
  if (unlikely((condition)!=0)) { \
		 printk("%p, Badness in %s at %s:%d\n", dev, __FUNCTION__, __FILE__, __LINE__); \
	} \
 } while (0)
#define pr_debug(...)	

#define IRQ_WAKE_THREAD	(1 << 1)

#define to_dwc3_ep(ep)		(container_of(ep, struct dwc3_ep, endpoint))
#define gadget_to_dwc(g)	(container_of(g, struct dwc3, gadget))
#define to_dwc3_request(r)	(container_of(r, struct dwc3_request, request))
#define act_writel 	writel
static inline struct dwc3_request *next_request(struct list_head *list)
{
	if (list_empty(list))
		return NULL;

	return list_first_entry(list, struct dwc3_request, list);
}
static inline u32 dwc3_readl(void __iomem *base, u32 offset)
{
	/*
	 * We requested the mem region starting from the Globals address
	 * space, see dwc3_probe in core.c.
	 * However, the offsets are given starting from xHCI address space.
	 */
	return readl(base + (offset/* - DWC3_GLOBALS_REGS_START*/));
}
inline int strlcat(char *dest, const char *src, int count)
{
	size_t dsize = strlen(dest);
	size_t len = strlen(src);
	size_t res = dsize + len;


	dest += dsize;
	count -= dsize;
	if (len >= count)
		len = count-1;
	memcpy(dest, src, len);
	dest[len] = 0;
	return res;
}
static inline void dwc3_writel(void __iomem *base, u32 offset, u32 value)
{
	/*
	 * We requested the mem region starting from the Globals address
	 * space, see dwc3_probe in core.c.
	 * However, the offsets are given starting from xHCI address space.
	 */
	writel(value, base + (offset /*- DWC3_GLOBALS_REGS_START*/));
}
static inline u32 dwc3_gadget_ep_get_transfer_index(struct dwc3 *dwc, u8 number)
{
	u32			res_id;

	res_id = dwc3_readl(dwc->regs, DWC3_DEPCMD(number));

	return DWC3_DEPCMD_GET_RSC_IDX(res_id);
}

/*************************dma func*************************************/
static inline int usb_gadget_map_request(struct usb_gadget *gadget,
		struct usb_request *req, int is_in)
{
	//unsigned long uncache_len;
    
	req->dma = (dma_addr_t)req->buf;
    	//printf("\n--------usb_gadget_map_request-----%x-----%d,%d--------\n",req->dma, req->length,uncache_len);
	return 0;
}	



#define usb_gadget_unmap_request(...)
#define sg_dma_address(sg)	(0)
#define sg_dma_len(sg)	(0)
#define sg_is_last(sg)	(0)
#define for_each_sg(sglist, sg, nr, __i)

static inline void dma_free_coherent(struct device *dev, size_t size,
		       void *addr, dma_addr_t handle)
{
	if(addr)
		free(addr);
}

static inline void *dma_alloc_coherent(struct device *dev, size_t size,
			 dma_addr_t *handle, gfp_t gfp)
{	
	*handle =(dma_addr_t)memalign(CONFIG_SYS_CACHELINE_SIZE, size);
	//printf("\n---------------dma_alloc_coherent-----%x--%d--------------\n",(*handle),size);
	return  ( void *)(*handle);

}

static inline void dwc3_gadget_move_request_queued(struct dwc3_request *req)
{
	struct dwc3_ep		*dep = req->dep;

	req->queued = true;
	list_move_tail(&req->list, &dep->req_queued);
}

void * devm_kzalloc(struct device *dev, int size, int gfp)
{
	void * ptr;
    
	ptr = malloc(size);
    	memset(ptr,0,size);
       return ptr;
}

#define platform_get_irq(...)	(0)
#define request_threaded_irq(...)	(0)

#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))
#define lower_32_bits(n) ((u32)(n))
#define IS_ALIGNED(x, a)		(((x) & ((typeof(x))(a) - 1)) == 0)
#define BUILD_BUG_ON_NOT_POWER_OF_2(...)	
#define IO_ADDRESS(n) ((u32)(n))
#define usb_add_gadget_udc(...) (0)
#define usb_del_gadget_udc(...) 


/* prototypes */
void dwc3_set_mode(struct dwc3 *dwc, u32 mode);
int dwc3_gadget_resize_tx_fifos(struct dwc3 *dwc);

int dwc3_host_init(struct dwc3 *dwc);
void dwc3_host_exit(struct dwc3 *dwc);
int dwc3_gadget_init(struct dwc3 *dwc);
void dwc3_gadget_exit(struct dwc3 *dwc);
int dwc3_gadget_prepare(struct dwc3 *dwc);
void dwc3_gadget_complete(struct dwc3 *dwc);
int dwc3_gadget_suspend(struct dwc3 *dwc);
int dwc3_gadget_resume(struct dwc3 *dwc);

int __dwc3_gadget_ep_set_halt(struct dwc3_ep *dep, int value);
int dwc3_send_gadget_ep_cmd(struct dwc3 *dwc, unsigned ep,
		unsigned cmd, struct dwc3_gadget_ep_cmd_params *params);
void dwc3_gadget_giveback(struct dwc3_ep *dep, struct dwc3_request *req,
		int status);
void dwc3_ep0_out_start(struct dwc3 *dwc);
int dwc3_send_gadget_generic_command(struct dwc3 *dwc, int cmd, u32 param);
int dwc3_gadget_set_test_mode(struct dwc3 *dwc, int mode);

static inline const char *dwc3_ep_event_string(u8 event)
{
        switch (event) {
        case DWC3_DEPEVT_XFERCOMPLETE:
                return "Transfer Complete";
        case DWC3_DEPEVT_XFERINPROGRESS:
                return "Transfer In-Progress";
        case DWC3_DEPEVT_XFERNOTREADY:
                return "Transfer Not Ready";
        case DWC3_DEPEVT_RXTXFIFOEVT:
                return "FIFO";
        case DWC3_DEPEVT_STREAMEVT:
                return "Stream";
        case DWC3_DEPEVT_EPCMDCMPLT:
                return "Endpoint Command Complete";
        }

        return "UNKNOWN";
}

#endif /* __DRIVERS_USB_DWC3_CORE_H */
