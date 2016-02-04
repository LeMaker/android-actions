/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009 Actions Semi Inc.
*/
/*******************************************************************************/

#ifndef __GL5206_TVOUT_H__
#define __GL5206_TVOUT_H__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/
/* TVOUT_EN */
/* bit[31:1]  Reserved */
#define TVOUT_EN_CVBS_EN					(0x1 << 0)

/* TVOUT_OCR */
/* bit[2:0],bit[6:5],bit[22:13],bit[31:29]  Reserved */
#define TVOUT_OCR_PI_IRQEN					(0x1 << 12)
#define TVOUT_OCR_PO_IRQEN					(0x1 << 11)
#define TVOUT_OCR_INACEN					(0x1 << 10)
#define TVOUT_OCR_PO_ADEN					(0x1 << 9)
#define TVOUT_OCR_PI_ADEN					(0x1 << 8)
#define TVOUT_OCR_INREN						(0x1 << 7)
#define TVOUT_OCR_DACOUT					(0x1 << 4)
#define TVOUT_OCR_DAC3						(0x1 << 3)

/* TVOUT_STA */
/* bit[2:0],bit[6:4]  Reserved */
#define TVOUT_STA_DAC3DLS					(0x1 << 7)
#define TVOUT_STA_DAC3ILS					(0x1 << 3)


/* TVOUT_CCR */
/* bit[31:4]  Reserved */
#define TVOUT_CCR_COTCTL_MASK				(0xF << 0)
#define TVOUT_CCR_COTCTL(x)					(((x) & 0xF) << 0)

/* TVOUT_BCR */
/* bit[31:10]  Reserved */
#define TVOUT_BCR_BRGCTL_MASK				(0x3FF << 0)
#define TVOUT_BCR_BRGCTL(x)					(((x) & 0x3FF) << 0)

/* TVOUT_CSCR */
/* bit[31:6]  Reserved */
#define TVOUT_CSCR_CSATCTL_MASK				(0x3F << 0)
#define TVOUT_CSCR_CSATCTL(x)				(((x) & 0x3F) << 0)

/*+++++++++TVOUT_PRL+++++++++*/
/* bit[31:12],bit[7:0]  Reserved */
#define TVOUT_PRL_CPN_MASK				(0xF << 8)
#define TVOUT_PRL_CPN(x)				(((x) & 0xF) << 8)

/*+++++++++TVOUT_VFALD+++++++++*/
/* bit[31:12],bit[7:0]  Reserved */
#define TVOUT_PRL_VFALD_MASK				(0xF << 8)
#define TVOUT_PRL_VFALD(x)				(((x) & 0xF) << 8)

/* CVBS_MSR */
/* bit[31:7]  Reserved */
#define CVBS_MSR_SCEN						(0x1 << 6)
#define CVBS_MSR_APNS_MASK					(0x1 << 5)
#define CVBS_MSR_APNS_BT470					(0x0 << 5)
#define CVBS_MSR_APNS_BT656					(0x1 << 5)
#define CVBS_MSR_CVCKS						(0x1 << 4)
#define CVBS_MSR_CVBS_MASK					(0xF << 0)
#define CVBS_MSR_CVBS_NTSC_M					(0x0 << 0)
#define CVBS_MSR_CVBS_NTSC_J					(0x1 << 0)
#define CVBS_MSR_CVBS_PAL_NC					(0x2 << 0)
#define CVBS_MSR_CVBS_PAL_BGH					(0x3 << 0)
#define CVBS_MSR_CVBS_PAL_D					(0x4 << 0)
#define CVBS_MSR_CVBS_PAL_I					(0x5 << 0)
#define CVBS_MSR_CVBS_PAL_M					(0x6 << 0)
#define CVBS_MSR_CVBS_PAL_N					(0x7 << 0)

/* CVBS_AL_SEPO */
/* bit[31:26]  Reserved */
#define CVBS_AL_SEPO_ALEP_MASK				(0x3FF << 16)
#define CVBS_AL_SEPO_ALEP(x)				(((x) & 0x3FF) << 16)
/* bit[15:10]  Reserved */
#define CVBS_AL_SEPO_ALSP_MASK				(0x3FF << 0)
#define CVBS_AL_SEPO_ALSP(x)				(((x) & 0x3FF) << 0)

/* CVBS_AL_SEPE */
/* bit[31:26]  Reserved */
#define CVBS_AL_SEPE_ALEPEF_MASK			(0x3FF << 16)
#define CVBS_AL_SEPE_ALEPEF(x)				(((x) & 0x3FF) << 16)
/* bit[15:10]  Reserved */
#define CVBS_AL_SEPE_ALSPEF_MASK			(0x3FF << 0)
#define CVBS_AL_SEPE_ALSPEF(x)				(((x) & 0x3FF) << 0)

/* CVBS_AD_SEP */
/* bit[31:26]  Reserved */
#define CVBS_AD_SEP_ADEP_MASK				(0x3FF << 16)
#define CVBS_AD_SEP_ADEP(x)					(((x) & 0x3FF) << 16)
/* bit[15:10]  Reserved */
#define CVBS_AD_SEP_ADSP_MASK				(0x3FF << 0)
#define CVBS_AD_SEP_ADSP(x)					(((x) & 0x3FF) << 0)

/* CVBS_HUECR */
/* bit[31:8]  Reserved */
#define CVBS_HUECR_HUECTL_MASK				(0xFF << 0)
#define CVBS_HUECR_HUECTL(x)				(((x) & 0xFF) << 0)

/* CVBS_SCPCR */
/* bit[31:12]  Reserved */
#define CVBS_SCPCR_SCPC_MASK				(0xFFF << 0)
#define CVBS_SCPCR_SCPC(x)					(((x) & 0xFFF) << 0)

/* CVBS_SCFCR */
/* bit[31:5]  Reserved */
#define CVBS_SCFCR_SCFCR_MASK				(0x1F << 0)
#define CVBS_SCFCR_SCFCR(x)					(((x) & 0x1F) << 0)

/* CVBS_CBACR */
/* bit[31:4]  Reserved */
#define CVBS_CBACR_CBURAM_MASK				(0xF << 0)
#define CVBS_CBACR_CBURAM(x)				(((x) & 0xF) << 0)

/* CVBS_SACR */
/* bit[31:7]  Reserved */
#define CVBS_SACR_SYNCAM_MASK				(0x7F << 0)
#define CVBS_SACR_SYNCAM(x)					(((x) & 0x7F) << 0)


/* ++++++++BT_MSR0+++++++ */
/* bit[31:1]  Reserved */
#define BT_MSR0_BTEN					(0x1 << 0)

/* ++++++++BT_MSR1+++++++ */
/* bit[31:3],bit[0] Reserved */
#define BT_MSR1_PNMS					(0x1 << 1)
#define BT_MSR1_PAES					(0x1 << 2)


/* +++++++BT_AL_SEPO+++++++++ */
/* bit[31:25]  Reserved */
#define BT_AL_SEPO_LEPO_MASK				(0x1FF << 16)
#define BT_AL_SEPO_LEPO(x)				(((x) & 0x1FF) << 16)
/* bit[15:8]  Reserved */
#define BT_AL_SEPO_LSPO_MASK				(0xFF << 0)
#define BT_AL_SEPO_LSPO(x)				(((x) & 0xFF) << 0)

/* +++++++BT_AL_SEPE+++++++++ */
/* bit[31:26]  Reserved */
#define BT_AL_SEPE_LEPO_MASK				(0x3FF << 16)
#define BT_AL_SEPE_LEPO(x)				(((x) & 0x3FF) << 16)
/* bit[15:10]  Reserved */
#define BT_AL_SEPE_LSPO_MASK				(0x3FF << 0)
#define BT_AL_SEPE_LSPO(x)				(((x) & 0x3FF) << 0)

/* +++++++BT_AP_SEP+++++++++ */
/* bit[31:26]  Reserved */
#define BT_AP_SEP_ADEP_MASK				(0x3FF << 16)
#define BT_AP_SEP_ADEP(x)				(((x) & 0x3FF) << 16)
/* bit[15:10]  Reserved */
#define BT_AP_SEP_ADSP_MASK				(0x3FF << 0)
#define BT_AP_SEP_ADSP(x)				(((x) & 0x3FF) << 0)



/* TVOUT_DCR */
/* bit[31:24],bit[18:4]   Reserved */
#define TVOUT_DCR_CVBS_STA_MASK				(0x1F << 19)
#define TVOUT_DCR_OSIGS_MASK				(0x1F << 0)

/* TVOUT_DDCR */
/* bit[31:21]  Reserved */
#define TVOUT_DDCR_DCKCAES_MASK				(0x1 << 20)
#define TVOUT_DDCR_DCKCAES_PE				(0x0 << 20)
#define TVOUT_DDCR_DCKCAES_NE				(0x1 << 20)
/* bit[19:18]  Reserved */
#define TVOUT_DDCR_YDLVLS_MASK				(0x3 << 16)
#define TVOUT_DDCR_YDLVLS(X)				(((x) & 0x3) << 16)
#define TVOUT_DDCR_SCOTEN					(0x1 << 11)
#define TVOUT_DDCR_CORMODE_MASK				(0x3 << 9)
#define TVOUT_DDCR_CORMODE(x)				(((x) & 0x3) << 9)
#define TVOUT_DDCR_COREN					(0x1 << 8)
#define TVOUT_DDCR_HDACUR_MASK				(0xF << 4)
#define TVOUT_DDCR_HDACUR(x)				(((x) & 0xF) << 4)
/* bit[3:1]  Reserved */
#define TVOUT_DDCR_TDACEN					(0x1 << 0)

/* TVOUT_DCORCTL 变化*/
#define TVOUT_DCORCTL_CORAMP_MASK			(0xFFFF << 16)
#define TVOUT_DCORCTL_CORAMP(x)				(((x) & 0xFFFF) << 16)
#define TVOUT_DCORCTL_CORFREQ_MASK			(0xFFFF << 0)
#define TVOUT_DCORCTL_CORFREQ(x)			(((x) & 0xFFFF) << 0)

/* TVOUT_DRCR 变化*/
/* bit[31:21]  Reserved */
#define TVOUT_DRCR_ROMTEN				(0x1 << 24)
#define TVOUT_DRCR_ROMDAT_MASK				(0xFFFFFF << 0)

/*CMU_TVOUTPLL*/
#define CMU_TVOUTPLL_PLL1_DPLLEN        (1 << 19)
#define CMU_TVOUTPLL_PLL0_DPCLM(x)      (((x) << 16) & 0x3)

#define CMU_TVOUTPLL_PLL1EN            (1 << 11)
#define CMU_TVOUTPLL_CVBS_PLL1FSS(x)   (((x) & 0x7)  << 8)
#define CMU_TVOUTPLL_CVBS_CLK108M_EN	(1 << 7)
#define CMU_TVOUTPLL_TK0SS            	(1 << 6)
/* bit2 reserved */
#define CMU_TVOUTPLL_CLK0(x)		(((x) << 4) & 0x3)
#define CMU_TVOUTPLL_PLL0EN             (1 << 3)
/* bit2 reserved */
#define CMU_TVOUTPLL_PLL0FSS(x)       (((x) << 0) & 0x3)

#ifdef __cplusplus
}
#endif

#endif  /* ifndef __ATV230x_TVOUT_H__ */
