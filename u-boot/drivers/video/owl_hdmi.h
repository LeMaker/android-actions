/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009 Actions Semi Inc.
 */
#ifndef __OWL_HDMI_H__
#define __OWL_HDMI_H__


#define HDMI_DRV_DEBUG 1
#if (HDMI_DRV_DEBUG == 1)
#define HDMI_DRV_PRINT(fmt, args...)	\
	do { printf("hdmi: " fmt, ##args); } while (0)
#else
#define HDMI_DRV_PRINT(fmt, args...) do {} while (0)
#endif
/********************************************************/
/* HDMI_VICTL */
/* each line pixel data scaler 2:1 in progressive format */
/* enable scaler 2:1 */
#define HDMI_VICTL_PDIV2_DISABLE_SCALER     (0 << 29)
#define HDMI_VICTL_PDIV2_ENABLE_SCALER      (1 << 29)

/********************************************************/
/* HDMI_AICHSTABYTE0to3 */
/* for 60958-3 consumer applications */
#define HDMI_AICHSTABYTE0TO3_60958_3_CLOCKACCU(x)   (((x) & 0x3) << 28)
#define HDMI_AICHSTABYTE0TO3_60958_3_SAMPFRE(x)     (((x) & 0xF) << 24)
#define HDMI_AICHSTABYTE0TO3_60958_3_CATEGCODE(x)   (((x) & 0xFF) << 8)
#define HDMI_AICHSTABYTE0TO3_60958_3_CHANSTAM(x)    (((x) & 0x3) << 6)
#define HDMI_AICHSTABYTE0TO3_60958_3_ADDFORMAT(x)   (((x) & 0x7) << 3)
#define HDMI_AICHSTABYTE0TO3_60958_3_CPBIT          (1 << 2)
#define HDMI_AICHSTABYTE0TO3_60958_3_USERTYPE(x)    (((x) & 0x3) << 0)
/* for 60958-4 professional applications */
#define HDMI_AICHSTABYTE0TO3_60958_4_MULTICHMC      (1 << 31)
#define HDMI_AICHSTABYTE0TO3_60958_4_MULTICHAM(x)   (((x) & 0x7) << 28)
#define HDMI_AICHSTABYTE0TO3_60958_4_MULTICHAN(x)   (((x) & 0xF) << 24)
#define HDMI_AICHSTABYTE0TO3_60958_4_ALIGNLEVEL(x)  (((x) & 0x3) << 22)
#define HDMI_AICHSTABYTE0TO3_60958_4_TRANSWL(x)     (((x) & 0x7) << 19)
#define HDMI_AICHSTABYTE0TO3_60958_4_AUXISAMB(x)    (((x) & 0x7) << 16)
#define HDMI_AICHSTABYTE0TO3_60958_4_ENUSERBIT(x)   (((x) & 0xF) << 12)
#define HDMI_AICHSTABYTE0TO3_60958_4_ENCHMODE(x)    (((x) & 0xF) << 8)
#define HDMI_AICHSTABYTE0TO3_60958_4_ENSAMFRE(x)    (((x) & 0x3) << 6)
#define HDMI_AICHSTABYTE0TO3_60958_4_LOCKINDI       (1 << 5)
#define HDMI_AICHSTABYTE0TO3_60958_4_EASPE(x)       (((x) & 0x7) << 2)
#define HDMI_AICHSTABYTE0TO3_60958_4_USERTYPE(x)    (((x) & 0x3) << 0)

/********************************************************/
/* HDMI_CR */
#define HDMI_CR_ENHPINT                             (1 << 31)
#define HDMI_CR_PHPLG                               (1 << 30)
#define HDMI_CR_HPLGSTATUS                          (1 << 29)
#define HDMI_CR_HPDENABLE                           (1 << 28)
#define HDMI_CR_HPDEBOUNCE(x)                       (((x) & 0xF) << 24)
#define HDMI_CR_PKTGB2V_ENABLE                      (1 << 6)
#define HDMI_CR_FIFO_FILL                           (1 << 4)
#define HDMI_CR_ENABLEHDMI                          (1 << 0)

/********************************************************/
/* HDMI_SCHCR */
#define HDMI_SCHCR_DSCS                             (1 << 30)
#define HDMI_SCHCR_HDMI_CHANNEL_INVERT              (1 << 29)
#define HDMI_SCHCR_HDMI_BIT_INVERT                  (1 << 28)
#define HDMI_SCHCR_HVAD                             (0xF << 24)
#define HDMI_SCHCR_HPN                              (0xF << 20)
#define HDMI_SCHCR_DEEPCOLOR_MASK                   (0x3 << 16)
#define HDMI_SCHCR_DEEPCOLOR(x)                     (((x) & 0x3) << 16)
#define HDMI_SCHCR_HDMI_3D_FRAME_FLAG               (1 << 8)
#define HDMI_SCHCR_VSYNCPOLIN                       (1 << 7)
#define HDMI_SCHCR_HSYNCPOLIN                       (1 << 6)
#define HDMI_SCHCR_PIXELENCFMT(x)                   (((x) & 0x3) << 4)
#define HDMI_SCHCR_VSYNCPOLINV                      (1 << 2)
#define HDMI_SCHCR_HSYNCPOLINV                      (1 << 1)
#define HDMI_SCHCR_HDMI_MODESEL                     (1 << 0)

/********************************************************/
/* HDMI_ICR */
#define HDMI_ICR_ENAUDIO                            (1 << 25)
#define HDMI_ICR_ENVITD                             (1 << 24)
#define HDMI_ICR_VITD(x)                            (((x) & 0xFFFFFF) << 0)

/********************************************************/
/* HDMI_ACRPCR */
#define HDMI_ACRPCR_DISABLECRP                      (1 << 31)
#define HDMI_ACRPCR_CTS_SOURCE                      (1 << 30)
#define HDMI_ACRPCR_N_VALUE(x)                      (((x) & 0xFFFFF) << 0)
/********************************************************/
/* HDMI_OPCR */
#define HDMI_OPCR_ENRBPKTSRAM                       (1 << 31)
#define HDMI_OPCR_SRAMRDSTATUS                      (1 << 30)
#define HDMI_OPCR_WRDES                             (1 << 9)
#define HDMI_OPCR_RPRWCMD                           (1 << 8)
#define HDMI_OPCR_RPADD(x)                          (((x) & 0xFF) << 0)
/********************************************************/
/* HDMI_CRCDOR */
/********************************************************/
/* TMDS_EODR0 */
#define TMDS_EODR0_TMDS_ENCEN                       (1 << 31)
#define TMDS_EODR0_TMDS_CH2_OUT(x)                  (((x) & 0x3FF) << 20)
#define TMDS_EODR0_TMDS_CH1_OUT(x)                  (((x) & 0x3FF) << 10)
#define TMDS_EODR0_TMDS_CH0_OUT(x)                  (((x) & 0x3FF) << 0)
/********************************************************/
/* HDMI_TX_1 */
#define HDMI_TX_1_REG_TX_VGATE(x)                   (((x) & 0x7) << 29)
#define HDMI_TX_1_REG_TX_PLL_VG(x)                  (((x) & 0x7) << 26)
#define HDMI_TX_1_REG_TX_PLL_RS(x)                  (((x) & 0x3) << 24)
#define HDMI_TX_1_REG_TX_PLL_PU                     (1 << 23)
#define HDMI_TX_1_REG_TX_PLL_ICP(x)                 (((x) & 0x3) << 20)
#define HDMI_TX_1_REG_PLL_VCO_SCALE(x)              (((x) & 0x3) << 18)
#define HDMI_TX_1_REG_TX_PLL_FBAND(x)               (((x) & 0x3) << 16)
#define HDMI_TX_1_REG_TX_PLL_CS(x)                  (((x) & 0x3) << 14)
#define HDMI_TX_1_REG_TX_PLL_EDGE                   (1 << 13)
#define HDMI_TX_1_REG_TX_SET_VC(x)                  (((x) & 0x3) << 11)
#define HDMI_TX_1_REG_TX_FORCE_VC                   (1 << 10)
#define HDMI_TX_1_REG_TX_EMPH(x)                    (((x) & 0x3) << 8)
#define HDMI_TX_1_REG_TX_EN_EMPH                    (1 << 7)
#define HDMI_TX_1_REG_TX_IBIAS(x)                   (((x) & 0x3) << 4)
#define HDMI_TX_1_REG_TX_AMP(x)                     (((x) & 0xF) << 0)

/********************************************************/
/* HDMI_TX_2 */
#define HDMI_TX_2_REG_CKA_HDMI_EN                   (1 << 28)
#define HDMI_TX_2_REG_TX_CK5XP_PU                   (1 << 27)
#define HDMI_TX_2_REG_LDO_TMDS_VSEL(x)              (((x) & 0x7) << 24)
#define HDMI_TX_2_REG_TX_CK5XP_DUTY(x)              (((x) & 0x3) << 22)
#define HDMI_TX_2_REG_TX_CK5XN_DUTY(x)              (((x) & 0x3) << 20)
#define HDMI_TX_2_REG_TX_RT_SEL(x)                  (((x) & 0x3) << 18)
#define HDMI_TX_2_REG_TX_RT_EN                      (1 << 17)
#define HDMI_TX_2_REG_TX_BYPASS_PLL                 (1 << 16)
#define HDMI_TX_2_REG_TST_SEL(x)                    (((x) & 0x3) << 14)
#define HDMI_TX_2_REG_PLL_TST_EN                    (1 << 13)
#define HDMI_TX_2_REG_TX_TST_EN                     (1 << 12)
#define HDMI_TX_2_REG_TX_PU(x)                      (((x) & 0xF) << 8)
#define HDMI_TX_2_REG_TX_SLEW(x)                    (((x) & 0x3) << 6)
#define HDMI_TX_2_REG_TX_DRIVER(x)                  (((x) & 0x3) << 4)
#define HDMI_TX_2_REG_TX_PLL_LOCK                   (1 << 0)

/********************************************************/
/* CMU_TVOUTPLL*/
#define CMU_TVOUTPLL_PLLFSS(x)                      (((x) & 0x3) << 0)
#define CMU_TVOUTPLL_PLLEN                          (1 << 3)
#define CMU_TVOUTPLL_CLKPIXDIV(x)                   (((x) & 0x3) << 4)
#define CMU_TVOUTPLL_CVBS_CLK108M_EN                (1 << 8)
#define CMU_TVOUTPLL_CLK_CVBSDIV                    (1 << 12)
#define CMU_TVOUTPLL_DPCLM(x)                          (((x) & 0x3) << 16)
#define CMU_TVOUTPLL_DPLLEN                         (1 << 19)


#define CMU_DEVCLKEN1_HDMI                          (0x1 << 3)
#define CMU_DEVCLKEN0_HDMIA                         (0x1 << 22)
#define CMU_DEVRST1_HDMI                            (0x1 << 2)

/* Internal SRAM allocation for Periodic Data Island Packet */

#define HDMI_RAMPKT_AVI_SLOT	0
#define HDMI_RAMPKT_AUDIO_SLOT	1
#define HDMI_RAMPKT_SPD_SLOT	2
#define HDMI_RAMPKT_GBD_SLOT	3
/*#define HDMI_RAMPKT_ACP_SLOT	4*/
#define HDMI_RAMPKT_VS_SLOT	    4
#define HDMI_RAMPKT_MPEG_SLOT	5

#define HDMI_RAMPKT_PERIOD		1


enum HDMI_MODE {
	HDMI_MODE_DVI,
	HDMI_MODE_HDMI,
	HDMI_MODE_MAX
};

enum HDMI_PLUGGING {
	HDMI_PLUGOUT = 0,
	HDMI_PLUGIN,
	HDMI_PLUGGING_MAX
};
enum SRC_SEL {
	VITD = 0,
	DE,
	SRC_MAX
};

enum PIXEL_ENCODING {
	VIDEO_PEXEL_ENCODING_RGB = 0,
	VIDEO_PEXEL_ENCODING_YCbCr444 = 2,
	VIDEO_PEXEL_ENCODING_MAX
};

enum DEEP_COLOR {
	DEEP_COLOR_24_BIT = 0,
	DEEP_COLOR_30_BIT,
	DEEP_COLOR_36_BIT,
	DEEP_COLOR_MAX
};

enum COLOR_XVYCC {
	YCC601_YCC709 = 0,
	XVYCC601,
	XVYCC709,
	XVYCC_MAX
};

enum _3D_FORMAT {
	_3D_NOT = 0,
	_3D,
	_3D_FORMAT_MAX
};

enum DATA_BLOCK_TYPE {
	AUDIO_DATA_BLOCK = 1,
	VIDEO_DATA_BLOCK = 2,
	VENDOR_SPECIFIC_DATA_BLOCK = 3,
	SPEAKER_ALLOOCATION_DATA_BLOCK = 4,
	VESA_DTC_DATA_BLOCK = 5,
	USE_EXTENDED_TAG = 7
};

struct asoc_videomode{
	int valid;
	int vid;
	struct fb_videomode mode;
};

struct sink_capabilities_t {
	/*hdmi_mode is used to verify whether sink support hdmi or not */
	unsigned int hdmi_mode;
	/*sink_3d_cap is used to veriry whether sink support 3D or not */
	unsigned int sink_3d_cap;
	/*sink physical address is used for hdmi cec funtion */
	unsigned int sink_phy_addr;
	/*
	 * audio capabilites
	 * for now(20090817), only Linear PCM(IEC60958) considered
	 */

	/*
	 * maximum audio channel number
	 * it should be (<=8)
	 */
	unsigned int max_channel_cap;

	/*
	 * audio sampling rate
	 *
	 * for each bit, if the value is 1 one sampling rate is supported.
	 * if 0, not supported.
	 * bit0: when the value is 1, 32kHz    is supported. when 0, 32kHz
	 * is not supported.
	 * bit1: when the value is 1, 44.1kHz  is supported. when 0, 44.1kHz
	 * is not supported.
	 * bit2: when the value is 1, 48kHz    is supported. when 0, 48kHz
	 * is not supported.
	 * bit3: when the value is 1, 88.2kHz  is supported. when 0, 88.2kHz
	 * is not supported.
	 * bit4: when the value is 1, 96kHz    is supported. when 0, 96kHz
	 * is not supported.
	 * bit5: when the value is 1, 176.4kHz is supported. when 0, 176.4kHz
	 * is not supported.
	 * bit6: when the value is 1, 192kHz   is supported. when 0, 192kHz
	 * is not supported.
	 * bit7~31: reserved
	 */
	unsigned int sampling_rate_cap;

	/*
	 * audio sample size
	 *
	 * for each bit, if the value is 1 one sampling size is supported.
	 * if 0, not supported.
	 * bit0: when the value is 1, 16-bit is supported. when 0, 16-bit
	 * is not supported.
	 * bit1: when the value is 1, 20-bit is supported. when 0, 20-bit
	 * is not supported.
	 * bit2: when the value is 1, 24-bit is supported. when 0, 24-bit
	 * is not supported.
	 * bit3~31: reserved
	 */
	unsigned int sampling_size_cap;

	/*
	 * speaker allocation information
	 *
	 * bit0: when the value is 1, FL/FR   is supported. when 0, FL/FR
	 * is not supported.
	 * bit1: when the value is 1, LFE     is supported. when 0, LFE
	 * is not supported.
	 * bit2: when the value is 1, FC      is supported. when 0, FC
	 * is not supported.
	 * bit3: when the value is 1, RL/RR   is supported. when 0, RL/RR
	 * is not supported.
	 * bit4: when the value is 1, RC      is supported. when 0, RC
	 * is not supported.
	 * bit5: when the value is 1, FLC/FRC is supported. when 0, FLC/FRC
	 * is not supported.
	 * bit6: when the value is 1, RLC/RRC is supported. when 0, RLC/RRC
	 * is not supported.
	 * bit7~31: reserved
	 *
	 * NOTICE:
	 *      FL/FR, RL/RR, FLC/FRC, RLC/RRC should be presented in pairs.
	 */
	unsigned int speader_allo_info;

	/*
	 * video capabilites
	 */

	/*
	 * pixel encoding (byte3(starting from 0) of CEA Extension Version3)
	 * Only pixel encodings of RGB, YCBCR4:2:2, and YCBCR4:4:4 may be
	 * used on HDMI.  All HDMI Sources and Sinks shall be capable of
	 * supporting RGB pixel encoding. If an HDMI Sink supports either
	 * YCBCR4:2:2 or YCBCR4:4:4 then both shall be supported. An HDMI
	 * Source may determine the pixel-encodings that are supported by
	 * the Sink through the use of the E-EDID. If the Sink indicates that
	 * it supports YCBCR-formatted video data and if the Source can deliver
	 * YCBCR data, then it can enable the transfer of this data across
	 * the link.
	 *
	 * bit0: when the value is 1, RGB is supported. when 0, RGB is not
	 * supported.
	 * bit1: when the value is 1, YCBCR4:4:4 is supported. when 0,
	 * YCBCR4:4:4 is not supported.
	 * bit2: when the value is 1, YCBCR4:2:2 is supported. when 0,
	 * YCBCR4:2:2 is not supported.
	 * bit3~31: reserved
	 */
	unsigned int pixel_encoding;

	/*
	 * video formats
	 *
	 * all 32 bits of VideoFormatInfo[0] and the former 27 bits of
	 * VideoFormatInfo[1] are valid bits. the value of each bit indicates
	 * whether one video format is supported by sink device. video format
	 * ID can be found in enum video_id_code. the bit postion corresponds
	 * to the video format ID in enum video_id_code. For example,
	 * bit  0 of VideoFormatInfo[0]: VIDEO_ID_640x480P_60Hz_4VS3
	 * bit 31 of VideoFormatInfo[0]: VIDEO_ID_1920x1080P_24Hz_16VS9
	 * bit  0 of VideoFormatInfo[1]: VIDEO_ID_1920x1080P_25Hz_16VS9
	 * bit 26 of VideoFormatInfo[1]: VIDEO_ID_720x480I_240Hz_16VS9
	 * when it is 1, the video format is supported, when 0, the video format
	 * is not supported.
	 *
	 *  notice:
	 *        Follwings is video format supported by our hdmi source,
	 *        please send supported video source to HDMI module
	 *                VIDEO_ID_640x480P_60Hz_4VS3
	 *                VIDEO_ID_720x480P_60Hz_4VS3
	 *                VIDEO_ID_720x480P_60Hz_16VS9
	 *                VIDEO_ID_720x576P_50Hz_4VS3
	 *                VIDEO_ID_720x576P_50Hz_16VS9
	 *                VIDEO_ID_1280x720P_60Hz_16VS9
	 *                VIDEO_ID_1280x720P_50Hz_16VS9
	 *                VIDEO_ID_720x480I_60Hz_4VS3
	 *                VIDEO_ID_720x480I_60Hz_16VS9
	 *                VIDEO_ID_720x576I_50Hz_4VS3
	 *                VIDEO_ID_720x576I_50Hz_16VS9
	 *                VIDEO_ID_1440x480P_60Hz_4VS3
	 *                VIDEO_ID_1440x480P_60Hz_16VS9
	 *                VIDEO_ID_1440x576P_50Hz_4VS3
	 *                VIDEO_ID_1440x576P_50Hz_16VS9
	 *  for 480P/576P, 4:3 is recommended, but 16:9 can be displayed.
	 *  for 720P, only 16:9 format.
	 */
	unsigned int video_formats[4];
};

/**
 * hdmi_settings - include the vaviable part of video
 * name : video format,include resolution ratio, ar, frequency
 * pixel_encoding : rgb,yuv422,etc
 */
struct hdmi_video_settings {
	unsigned int vid;
	unsigned int hdmi_mode;
	unsigned int hdmi_src;
	unsigned int pixel_encoding;
	unsigned int color_xvycc;
	unsigned int _3d;
	unsigned int deep_color;
};

/*audio*/
struct hdmi_audio_settings {

	unsigned int audio_channel;
	unsigned int audio_fs;
	unsigned int audio60958;

};

struct hdmi_sink_info {
	struct hdmi_video_settings v_settings;
	struct hdmi_audio_settings a_settings;
	struct sink_capabilities_t sink_cap;
};

struct video_parameters_t {
	unsigned int vid;
	unsigned int pixel_repeat;
	/*1--4:3; 2--16:9 */
	unsigned int ar;
	unsigned int colorimetry;
	unsigned int scan_mode;

	int VICTL;
	int VIVSYNC;
	int VIVHSYNC;
	int VIALSEOF;
	int VIALSEEF;
	int VIADLSE;
	int DIPCCR;
	/*Vsync & Hsync Active Low */
	int VHSYNC_P;
	/*Vsync and Hsync Invert---Active low */
	int VHSYNC_INV;
};

struct audio_parameters_t {

	unsigned int audio60958;
	unsigned int audio_channel;
	int ASPCR;
	int ACACR;
	int AUDIOCA;

};

enum __owl_tv_mode_t {
	OWL_TV_MOD_720P_50HZ           = 1,
    OWL_TV_MOD_720P_60HZ           = 2,
    OWL_TV_MOD_1080P_50HZ          = 3,
    OWL_TV_MOD_1080P_60HZ          = 4, 
    OWL_TV_MOD_576P                = 5,
    OWL_TV_MOD_480P                = 6,
    OWL_TV_MOD_DVI                 = 7,
    OWL_TV_MOD_PAL                 = 8,
    OWL_TV_MOD_NTSC                = 9,
    OWL_TV_MOD_4K_30HZ             = 10,
    OWL_TV_MODE_NUM               =  10,
};

struct video_parameters_t video_parameters[] = {
	/*VID  ar  SCAN_MODE  VICTL  VIVHSYNC  VIALSEEF  DIPCCR  VHSync_Inv
	 *PIXEL_REP  colorimetry  VIVSYNC  VIALSEOF  VIADLSE  VHSync_P */
	 //480 50HZ 
	{OWL_TV_MOD_480P, 0x0, 0x01, 0x01, 0x0, 0x035920c0, 0x00000000,
	 0x0b00503d, 0x00209029, 0x00000000, 0x03490079, 0x00000701, 0x00000000,
	 0x00000006},
	//576 50HZ 
	{OWL_TV_MOD_576P, 0x0, 0x01, 0x01, 0x0, 0x035f2700, 0x00000000,
	 0x0427003f, 0x0026b02b, 0x00000000, 0x03530083, 0x00000701, 0x00000000,
	 0x00000006},
    //720p 50HZ
	{OWL_TV_MOD_720P_50HZ, 0x0, 0x10, 0x10, 0x0, 0x07bb2ed0, 0x00000000,
	 0x042ed027, 0x002e8018, 0x00000000, 0x06030103, 0x00001107, 0x00000000,
	 0x00000000},
	 //720p 60HZ
	 {OWL_TV_MOD_720P_60HZ, 0x0, 0x10, 0x10, 0x0, 0x06712ed0, 0x00000000,
	 0x042ed027, 0x002e8018, 0x00000000, 0x06030103, 0x00001107, 0x00000000,
	 0x00000000},
	 //1080p 50HZ
	{OWL_TV_MOD_1080P_50HZ, 0x0, 0x10, 0x10, 0x0, 0xa4f4640, 0x00000000,
	 0x0446402b, 0x00460028, 0x00000000, 0x083f00bf, 0x00001107, 0x00000000,
	 0x00000000},
	//1080p 60HZ
	{OWL_TV_MOD_1080P_60HZ, 0x0, 0x10, 0x10, 0x0, 0x08974640, 0x00000000,
	 0x0446402b, 0x00460028, 0x00000000, 0x083f00bf, 0x00001107, 0x00000000,
	 0x00000000},	 
	//4kx2k 
	{OWL_TV_MOD_4K_30HZ, 0x0, 0x10, 0x10, 0x0, 0x112f8c90, 0x00000000,
	 0x11007057, 0x8c9059, 0x00000000, 0x0fb000b0, 0x00001107, 0x00000000,
	 0x00000000}, 	
	 
	  //DVI
	 {OWL_TV_MOD_DVI, 0x0, 0x10, 0x10, 0x0, 0x06712ed0, 0x00000000,
	 0x042ed027, 0x002e8018, 0x00000000, 0x06030103, 0x00001107, 0x00000000,
	 0x00000000},
};

static const struct asoc_videomode hdmi_display_modes[] = {
	[0] = {
		.valid = 1,
		.vid = OWL_TV_MOD_480P,
		.mode = {
			.name = "HDMI_720x480P_60_4VS3",
			.refresh = 60,
			.xres = 720,
			.yres = 480,
			.pixclock = 37000,	/*pico second, 1.e-12s */
			.left_margin = 60,
			.right_margin = 16,
			.upper_margin = 30,
			.lower_margin = 6,
			.hsync_len = 62,
			.vsync_len = 9,
			.sync = 0,
			.vmode = FB_VMODE_NONINTERLACED,
			.flag = FB_MODE_IS_STANDARD,
		}
	},
	[1] = {
		.valid = 1,
		.vid = OWL_TV_MOD_576P,
		.mode = {
			.name = "HDMI720x576P_50_4VS3",
			.refresh = 50,
			.xres = 720,
			.yres = 576,
			.pixclock = 37037,	/*pico second, 1.e-12s */
			.left_margin = 68,
			.right_margin = 12,
			.upper_margin = 39,
			.lower_margin = 5,
			.hsync_len = 64,
			.vsync_len = 5,
			.sync = 0,
			.vmode = FB_VMODE_NONINTERLACED,
			.flag = FB_MODE_IS_STANDARD,
		}
	},	
	[2] = {
		.valid = 1,
		.vid = OWL_TV_MOD_720P_50HZ,
		.mode = {
			.name = "HDMI_1280x720P_50_16VS9",
			.refresh = 50,
			.xres = 1280,
			.yres = 720,
			.pixclock = 13468,	/*pico second, 1.e-12s */
			.left_margin = 220,
			.right_margin = 440,
			.upper_margin = 20,
			.lower_margin = 5,
			.hsync_len = 40,
			.vsync_len = 5,
			.sync = 0,
			.vmode = FB_VMODE_NONINTERLACED,
			.flag = FB_MODE_IS_STANDARD,
		}
	},
	[3] = {
		.valid = 1,
		.vid = OWL_TV_MOD_720P_60HZ,
		.mode = {
			.name = "HDMI_1280x720P_60_16VS9",
			.refresh = 60,
			.xres = 1280,
			.yres = 720,
			.pixclock = 13468,	/*pico second, 1.e-12s */
			.left_margin = 220,
			.right_margin = 110,
			.upper_margin = 20,
			.lower_margin = 5,
			.hsync_len = 40,
			.vsync_len = 5,
			.sync = 0,
			.vmode = FB_VMODE_NONINTERLACED,
			.flag = FB_MODE_IS_STANDARD,
		}
	},
	[4] = {
		.valid = 1,
		.vid = OWL_TV_MOD_1080P_50HZ,
		.mode = {
			.name = "HDMI_1920x1080P_50_16VS9",
			.refresh = 50,
			.xres = 1920,
			.yres = 1080,
			.pixclock = 6734,	/*pico second, 1.e-12s */
			.left_margin = 148,
			.right_margin = 528,
			.upper_margin = 36,
			.lower_margin = 4,
			.hsync_len = 44,
			.vsync_len = 5,
			.sync = 0,
			.vmode = FB_VMODE_NONINTERLACED,
			.flag = FB_MODE_IS_STANDARD,
		}
	},
	
	[5] = {
		.valid = 1,
		.vid = OWL_TV_MOD_1080P_60HZ,
		.mode = {
			.name = "HDMI_1920x1080P_60_16VS9",
			.refresh = 60,
			.xres = 1920,
			.yres = 1080,
			.pixclock = 6734,	/*pico second, 1.e-12s */
			.left_margin = 148,
			.right_margin = 88,
			.upper_margin = 36,
			.lower_margin = 4,
			.hsync_len = 44,
			.vsync_len = 5,
			.sync = 0,
			.vmode = FB_VMODE_NONINTERLACED,
			.flag = FB_MODE_IS_STANDARD,
		}
	},	
	[6] = {
		.valid = 1,
		.vid = OWL_TV_MOD_4K_30HZ,
		.mode = {
			.name = "VID3840x2160p_30_16VS9",
			.refresh = 30,
			.xres = 3840,
			.yres = 2160,
			.pixclock = 4018,	/*pico second, 1.e-12s */
			.left_margin = 0,
			.right_margin = 0,
			.upper_margin = 0,
			.lower_margin = 0,
			.hsync_len = 0,
			.vsync_len = 0,
			.sync = 0,
			.vmode = FB_VMODE_NONINTERLACED,
			.flag = FB_MODE_IS_STANDARD,
		}
	},
	[7] = {
		.valid = 1,
		.vid = OWL_TV_MOD_DVI,
		.mode = {
			.name = "HDMI_DVI_MOD",
			.refresh = 60,
			.xres = 1280,
			.yres = 720,
			.pixclock = 13468,	/*pico second, 1.e-12s */
			.left_margin = 220,
			.right_margin = 110,
			.upper_margin = 20,
			.lower_margin = 5,
			.hsync_len = 40,
			.vsync_len = 5,
			.sync = 0,
			.vmode = FB_VMODE_NONINTERLACED,
			.flag = FB_MODE_IS_STANDARD,
		}
	},
};

struct audio_parameters_t audio_parameters[] = {
	/* audio60958       ASPCR               AudioCA
	   audio_channel            ACACR */
	{0x01, 0x001, 0x00000011, 0x00fac688, 0x00},
	{0x01, 0x010, 0x0002d713, 0x00004008, 0x04},
	{0x01, 0x011, 0x0003df1b, 0x00004608, 0x06},
	{0x01, 0x100, 0x0003df3b, 0x00034608, 0x0a},
	{0x01, 0x101, 0x0003df3f, 0x0002c688, 0x0b},
	{0x01, 0x110, 0x0007ff7f, 0x001ac688, 0x0f},
	{0x01, 0x111, 0x0007ffff, 0x00fac688, 0x13},
	{0x00, 0 /*NULL*/, 0x7f87c003, 0x00fac688, 0 /*NULL*/},
};





/*struct hdmi_tx_info {
	struct mutex tx_lock;
	struct asoc_display_device *hdmi_dev_for_fb;
	struct asoc_videomode *hdmi_display_mode;
	atomic_t state;
};*/

extern struct hdmi_video_settings default_video_settings;
extern struct video_parameters_t video_parameters[];
extern struct hdmi_audio_settings default_a_settings;
extern struct audio_parameters_t audio_parameters[];

void *hdmi_get_vid(struct hdmi_video_settings *v_general);
void *hdmi_get_aid(struct hdmi_audio_settings *a_general);

static inline void act_clearl(unsigned int val, unsigned int reg)
{
	*(volatile unsigned int *)(reg) &= ~val;
}
static inline void act_setl(unsigned int val, unsigned int reg)
{
	*(volatile unsigned int *)(reg) |= val;
}
void image_on(void);
int cec_init(void);
int send_message(unsigned char, unsigned char *, unsigned char);
extern int hdmi_init(void);
extern int check_hdmi_mode(int mode,int i2cbus);
#endif