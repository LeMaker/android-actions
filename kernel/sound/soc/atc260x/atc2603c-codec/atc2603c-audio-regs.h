/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009 Actions Semi Inc.
 */

#ifndef __ATC2603C_AUDIO_REGS_H__
#define __ATC2603C_AUDIO_REGS_H__

/* AUDIOINOUT_CTL */
#define AUDIOINOUT_CTL_INEN		(0x1 << 1)
#define AUDIOINOUT_CTL_LB		(0x1 << 4)
#define AUDIOINOUT_CTL_IMS(x)		(((x) & 0x3) << 5)
#define AUDIOINOUT_CTL_OMD		(0x1 << 7)
#define AUDIOINOUT_CTL_OEN		(0x1 << 8)
#define AUDIOINOUT_CTL_OCIEN		(0x1 << 9)
#define AUDIOINOUT_CTL_OHSCIEN		(0x1 << 10)
#define AUDIOINOUT_CTL_MDD		(0x1 << 11)
#define AUDIOINOUT_CTL_EIDR		(0x1 << 12)

/* DAC_DIGITALCTL */
#define DAC_DIGITALCTL_DEFL		(0x1 << 0)
#define DAC_DIGITALCTL_DEFL_SFT         (0)
#define DAC_DIGITALCTL_DEFR		(0x1 << 1)
#define DAC_DIGITALCTL_DEFR_SFT		(1)
#define DAC_DIGITALCTL_DESW		(0x1 << 2)
#define DAC_DIGITALCTL_DEC		(0x1 << 3)
#define DAC_DIGITALCTL_DESL		(0x1 << 4)
#define DAC_DIGITALCTL_DESR		(0x1 << 5)
#define DAC_DIGITALCTL_DESBL		(0x1 << 6)
#define DAC_DIGITALCTL_DESBR		(0x1 << 7)
#define DAC_DIGITALCTL_DMFL		(0x1 << 8)
#define DAC_DIGITALCTL_DMFR		(0x1 << 9)
#define DAC_DIGITALCTL_DMSW		(0x1 << 10)
#define DAC_DIGITALCTL_DMC		(0x1 << 11)
#define DAC_DIGITALCTL_DMSL		(0x1 << 12)
#define DAC_DIGITALCTL_DMSR		(0x1 << 13)
#define DAC_DIGITALCTL_DMSBL		(0x1 << 14)
#define DAC_DIGITALCTL_DMSBR		(0x1 << 15)

/* DAC_VOLUMECTL0 */
#define DAC_VOLUMECTL0_DACFL_VOLUME(x)	(((x) & 0xff) << 0)
#define DAC_VOLUMECTL0_DACFL_VOLUME_SFT	(0)
#define DAC_VOLUMECTL0_DACFR_VOLUME(x)	(((x) & 0xff) << 8)
#define DAC_VOLUMECTL0_DACFR_VOLUME_SFT	(8)


/* DAC_ANALOG0 */
#define DAC_ANALOG0_OPGIB(x)		(((x) & 0x7) << 0)
#define DAC_ANALOG0_KFEN		(0x1 << 3)
#define DAC_ANALOG0_OPVBIB(x)		(((x) & 0x3) << 4)
#define DAC_ANALOG0_OPDTSIB(x)		(((x) & 0x3) << 6)
#define DAC_ANALOG0_OPDAIB(x)		(((x) & 0x7) << 8)
#define DAC_ANALOG0_OPDAVB(x)		(((x) & 0x3) << 12)
#define DAC_ANALOG0_PAIB(x)		(((x) & 0x3) << 14)

/* DAC_ANALOG1 */
#define DAC_ANALOG1_VOLUME(x)		(((x) & 0x3f) << 0)
#define DAC_ANALOG1_VOLUME_SFT		(0)
#define DAC_ANALOG1_PASW		(0x1 << 6)
#define DAC_ANALOG1_PASW_SFT		(6)
#define DAC_ANALOG1_ZERODT		(0x1 << 7)
#define DAC_ANALOG1_PAIQ(x)		(((x) & 0x3) << 8)
#define DAC_ANALOG1_DACFL_FRMUTE	(0x1 << 10)
#define DAC_ANALOG1_DACFL_FRMUTE_SFT	(10)
#define DAC_ANALOG1_DACSW_CMUTE		(0x1 << 11)
#define DAC_ANALOG1_DACSL_SRMUTE	(0x1 << 12)
#define DAC_ANALOG1_DACSBL_SBRMUTE	(0x1 << 13)
#define DAC_ANALOG1_DACFMMUTE		(0x1 << 14)
#define DAC_ANALOG1_DACFMMUTE_SFT	(14)
#define DAC_ANALOG1_DACMICMUTE		(0x1 << 15)
#define DAC_ANALOG1_DACMICMUTE_SFT	(15)

/* DAC_ANALOG2 */
#define DAC_ANALOG2_OPVROOSIB(x)	(((x) & 0x7) << 0)
#define DAC_ANALOG2_DDATPR		(0x1 << 3)
#define DAC_ANALOG2_OPVROEN		(0x1 << 4)
#define DAC_ANALOG2_DDOVV		(0x1 << 5)
#define DAC_ANALOG2_CLDMIX(x)		(((x) & 0x3) << 6)
#define DAC_ANALOG2_PAVDC		(0x1 << 8)
#define DAC_ANALOG2_ATP2CE		(0x1 << 9)
#define DAC_ANALOG2_P2IB		(0x1 << 10)
#define DAC_ANALOG2_DACI		(0x1 << 11)
#define DAC_ANALOG2_ZERODETECT		(0x1 << 15)

/* DAC_ANALOG3 */
#define DAC_ANALOG3_DACEN_FR		(0x1)
#define DAC_ANALOG3_DACEN_FR_SFT	(0)
#define DAC_ANALOG3_DACEN_FL		(0x1 << 1)
#define DAC_ANALOG3_DACEN_FL_SFT	(1)
#define DAC_ANALOG3_PAEN_FR_FL		(0x1 << 2)
#define DAC_ANALOG3_PAEN_FR_FL_SFT	(2)
#define DAC_ANALOG3_PAOSEN_FR_FL	(0x1 << 3)
#define DAC_ANALOG3_PAOSEN_FR_FL_SFT	(3)

/*
#define DAC_ANALOG3_OPVROIB(x)		(((x) & 0x7) << 0)
#define DAC_ANALOG3_OPCM1IB(x)		(((x) & 0x3) << 3)
#define DAC_ANALOG3_ATPLP2_SBR_SBL	(0x1 << 5)
#define DAC_ANALOG3_ATPLP2_SR_SL	(0x1 << 6)
#define DAC_ANALOG3_ATPLP2_SW_C		(0x1 << 7)
#define DAC_ANALOG3_ATPLP2_FR_FL	(0x1 << 8)
#define DAC_ANALOG3_BIASEN		(0x1 << 9)
#define DAC_ANALOG3_EIDEN		(0x1 << 10)
#define DAC_ANALOG3_VLCHD		(0x1 << 13)
#define DAC_ANALOG3_OVLS		(0x1 << 14)
#define DAC_ANALOG3_EIDS		(0x1 << 15)
*/

/* ADC_DIGITALCTL */
#define ADC_DIGITALCTL_ADGC0		(((x) & 0xf) << 6)
#define ADC_DIGITALCTL_ADGC0_SFT	(6)

/* ADC0_HPFCTL */
#define ADC0_HPFCTL_HPF0REN		(0x1 << 0)
#define ADC0_HPFCTL_HPF0LEN		(0x1 << 1)
#define ADC0_HPFCTL_HPF0DW		(0x1 << 2)
#define ADC0_HPFCTL_WNHPF0CUT(x)	(((x) & 0x7) << 3)
#define ADC0_HPFCTL_SRSEL0(x)		(((x) & 0x3) << 6)


/* ADC_CTL */
#define ADC_CTL_ATAD_MTA_FTA_SFT	(0)
#define ADC_CTL_AD0REN			(0x1 << 3)
#define ADC_CTL_AD0REN_SFT		(3)
#define ADC_CTL_AD0LEN			(0x1 << 4)
#define ADC_CTL_AD0LEN_SFT		(4)
#define ADC_CTL_MIC0FDSE		(0x1 << 5)
#define ADC_CTL_MIC0FDSE_SFT		(5)
#define ADC_CTL_MIC0REN			(0x1 << 6)
#define ADC_CTL_MIC0REN_SFT		(6)
#define ADC_CTL_MIC0LEN			(0x1 << 7)
#define ADC_CTL_MIC0LEN_SFT		(7)
#define ADC_CTL_FMREN			(0x1 << 13)
#define ADC_CTL_FMREN_SFT		(13)
#define ADC_CTL_FMLEN			(0x1 << 14)
#define ADC_CTL_FMLEN_SFT		(14)



/* AGC_CTL0 */
#define AGC_CTL0_AMP0GR1(x)		(((x) & 0x7) << 0)
#define AGC_CTL0_AMP0GR1_SET		(0)
#define AGC_CTL0_IMICSHD		(0x1 << 7)
#define AGC_CTL0_AMP1G0R(x)		(((x) & 0xf) << 8)
#define AGC_CTL0_AMP1G0R_SFT		(8)
#define AGC_CTL0_AMP1G0L(x)		(((x) & 0xf) << 12)
#define AGC_CTL0_AMP1G0L_SFT		(12)

#define AGC_CTL0_AMP1GR1_MSK		(0x7 << 0)
#define AGC_CTL0_AMP1GR_MSK		(0xf << 8)
#define AGC_CTL0_AMP1GL_MSK		(0xf << 12)

#define AGC_CTL0_VMICINEN		(0x1 << 3)
#define AGC_CTL0_VMICINEN_SFT		(3)
#define AGC_CTL0_VMICEXST		(((x) & 0x3) << 4)
#define AGC_CTL0_VMICEXST_SFT		(4)
#define AGC_CTL0_VMICEXEN		(0x1 << 6)
#define AGC_CTL0_VMICEXEN_SFT		(6)

/* ADC_ANALOG0 */
#define ADC_ANALOG0_VRDABC(x)		(((x) & 0x7) << 0)
#define ADC_ANALOG0_OPBC23(x)		(((x) & 0x3) << 3)
#define ADC_ANALOG0_OPBC1(x)		(((x) & 0x7) << 5)
#define ADC_ANALOG0_IVSRMSTN(x)		(((x) & 0x7) << 13)

/* ADC_ANALOG1 */
#define ADC_ANALOG1_FMBC(x)		(((x) & 0x3) << 0)
#define ADC_ANALOG1_FD1BUFBC(x)		(((x) & 0x3) << 2)
#define ADC_ANALOG1_FD2BC(x)		(((x) & 0x3) << 4)
#define ADC_ANALOG1_FD1BC(x)		(((x) & 0x3) << 6)
#define ADC_ANALOG1_ADCBIAS		(0x1 << 10)
#define ADC_ANALOG1_LPFBUFBC(x)		(((x) & 0x3) << 11)
#define ADC_ANALOG1_LPFBC(x)		(((x) & 0x7) << 13)

/* I2S_CTL */
#define I2S_CTL_I2STEN			(0x1 << 0)
#define I2S_CTL_I2SREN			(0x1 << 1)
#define I2S_CTL_I2STOWL			(0x1 << 2)
#define I2S_CTL_I2STTDMRF		(0x1 << 3)
#define I2S_CTL_I2STXM(x)		(((x) & 0x7) << 4)
#define I2S_CTL_I2SRXM(x)		(((x) & 0x3) << 8)
#define I2S_CTL_I2SRCS			(0x1 << 10)
#define I2S_CTL_I2SPM(x)		(((x) & 0x3) << 11)

/* I2S_FIFOCTL */
#define I2S_FIFOCTL_I2STFR		(0x1 << 0)
#define I2S_FIFOCTL_I2STFDEN		(0x1 << 1)
#define I2S_FIFOCTL_I2STFIEN		(0x1 << 2)
#define I2S_FIFOCTL_I2STFIP		(0x1 << 3)
#define I2S_FIFOCTL_I2STFDCF(x)		(((x) & 0x7) << 4)
#define I2S_FIFOCTL_I2STFDRF		(0x1 << 7)
#define I2S_FIFOCTL_I2STFFF		(0x1 << 8)
#define I2S_FIFOCTL_I2SRFR		(0x1 << 9)
#define I2S_FIFOCTL_I2SRFDEN		(0x1 << 10)
#define I2S_FIFOCTL_I2SRFIEN		(0x1 << 11)
#define I2S_FIFOCTL_I2SRFIP		(0x1 << 12)
#define I2S_FIFOCTL_I2SRFDCF(x)		(((x) & 0x3) << 13)
#define I2S_FIFOCTL_I2STXKA		(0x1 << 15)
#define I2S_FIFOCTL_I2SRFDRF		(0x1 << 16)
#define I2S_FIFOCTL_I2SRFEF		(0x1 << 17)
#define I2S_FIFOCTL_I2STFSS		(0x1 << 18)
#define I2S_FIFOCTL_KMCMMI2ST(x)	(((x) & 0x3) << 19)

/* SPDIF_HDMI_CTL */
#define SPDIF_HDMI_CTL_SPDFR		(0x1 << 0)
#define SPDIF_HDMI_CTL_HDMIFR		(0x1 << 1)
#define SPDIF_HDMI_CTL_SPDFIP		(0x1 << 2)
#define SPDIF_HDMI_CTL_SPDFFF		(0x1 << 3)
#define SPDIF_HDMI_CTL_SPDFDEN		(0x1 << 4)
#define SPDIF_HDMI_CTL_SPDFIEN		(0x1 << 5)
#define SPDIF_HDMI_CTL_HDMFIP		(0x1 << 6)
#define SPDIF_HDMI_CTL_HDMFFF		(0x1 << 7)
#define SPDIF_HDMI_CTL_HDMFDEN		(0x1 << 8)
#define SPDIF_HDMI_CTL_HDMFIEN		(0x1 << 9)
#define SPDIF_HDMI_CTL_SPDEN		(0x1 << 10)
#define SPDIF_HDMI_CTL_SPDKA		(0x1 << 11)
#define SPDIF_HDMI_CTL_HDMKA		(0x1 << 12)
#define SPDIF_HDMI_CTL_SPDFSS		(0x1 << 13)
#define SPDIF_HDMI_CTL_HDMFSS		(0x1 << 14)
#define SPDIF_HDMI_CTL_KMCMMHDM(x)	(((x) & 0x3) << 15)

/* CMU_AUDIOPLL */
#define CMU_AUDIOPLL_AUDIOPLLS		(0x1 << 0)
#define CMU_AUDIOPLL_APEN		(0x1 << 4)
#define CMU_AUDIOPLL_I2STX_CLK(x)	(((x) & 0xf) << 16)
#define CMU_AUDIOPLL_I2SRX_CLK(x)	(((x) & 0xf) << 20)
#define CMU_AUDIOPLL_HDMIA_CLK(x)	(((x) & 0xf) << 24)
#define CMU_AUDIOPLL_SPDIF_CLK(x)	(((x) & 0xf) << 28)

/* CMU_DEVCLKEN */
#define CMU_DEVCLKEN0_I2STX		(0x1 << 20)
#define CMU_DEVCLKEN0_I2SRX		(0x1 << 21)
#define CMU_DEVCLKEN0_HDMIA		(0x1 << 22)
#define CMU_DEVCLKEN0_SPDIF		(0x1 << 23)

#define CMU_DEVRST0_AUDIO		(0x1 << 17)

#define GL5302_DEVRST_AUDIO_CLK_EN	(0x1 << 10)
#define GL5302_DEVRST_AUDIO_RST		(0x1 << 4)

#endif  /* ifndef __ATC2603C_AUDIO_REGS_H__ */
