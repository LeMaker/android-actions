/*
 * Asoc hdmi driver
 *
 * Copyright (C) 2011 Actions Semiconductor, Inc
 * Author:  chenbo <chenbo@actions-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <libfdt_env.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>

#include <asm/arch/actions_reg_owl.h>
#include <asm/arch/owl_lcd.h>
#include <asm/io.h>
#include <asm/arch/pwm_backlight.h>
#include <asm/gpio.h>

#include <common.h>
#include <malloc.h>

#include <video_fb.h>
#include <owl_dss.h>
#include <linux/list.h>
#include <linux/fb.h>

#include "owl_hdmi.h"

#ifdef CONFIG_CMD_EXT4
#include <ext4fs.h>
#include <mmc.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

/*edid*/
#define BIT0                        (1 << 0)
#define BIT1                        (1 << 1)
#define BIT2                        (1 << 2)
#define BIT3                        (1 << 3)
#define BIT4                        (1 << 4)
#define BIT5                        (1 << 5)
#define BIT6                        (1 << 6)
#define BIT7                        (1 << 7)
struct hdmi_sink_info sink_info = {
    .v_settings = {
        .vid = OWL_TV_MOD_1080P_60HZ,
        .hdmi_mode = HDMI_MODE_HDMI,
        .hdmi_src = DE,
        .pixel_encoding = VIDEO_PEXEL_ENCODING_RGB,
        .color_xvycc = YCC601_YCC709,
        .deep_color = DEEP_COLOR_24_BIT,
        ._3d = _3D_NOT,
    },
    .a_settings = {
        .audio_channel = 1,
        .audio_fs = 2,
        .audio60958 = 1,
    },
    .sink_cap = {
        .hdmi_mode = HDMI_MODE_HDMI,
        .sink_phy_addr = 0,
        .sink_3d_cap = 0,
    },
};
/*---------------------general---------------------*/
/* Packet Command */
int hdmi_SetRamPacketPeriod(unsigned char no, int period)
{
    if (no > 5 || no < 0)
        return -1;

    if (period > 0xf || period < 0)
        return -1;

    writel(readl(HDMI_RPCR) &  ~(1 << no), HDMI_RPCR);
    writel(readl(HDMI_RPCR) & ~(0xf << (no * 4 + 8)), HDMI_RPCR);

    if (period) {

        writel(readl(HDMI_RPCR) |  (period << (no * 4 + 8)),
                HDMI_RPCR);
        writel(readl(HDMI_RPCR) | (1 << no), HDMI_RPCR);
    }

    return 0;
}

/*
   convert readable Data Island packet to RAM packet format,
   and write to RAM packet area
   */

int hdmi_SetRamPacket(unsigned char no, unsigned char *pkt)
{
    unsigned char tpkt[36];
    unsigned long *reg = (unsigned long *)tpkt;
    unsigned char addr = 126 + no * 14;
    int i;

    if (no > 5 || no < 0)
        return -1;
    /*Packet Header*/
    tpkt[0] = pkt[0];
    tpkt[1] = pkt[1];
    tpkt[2] = pkt[2];
    tpkt[3] = 0;
    /*Packet Word0*/
    tpkt[4] = pkt[3];
    tpkt[5] = pkt[4];
    tpkt[6] = pkt[5];
    tpkt[7] = pkt[6];
    /*Packet Word1*/
    tpkt[8] = pkt[7];
    tpkt[9] = pkt[8];
    tpkt[10] = pkt[9];
    tpkt[11] = 0;
    /*Packet Word2*/
    tpkt[12] = pkt[10];
    tpkt[13] = pkt[11];
    tpkt[14] = pkt[12];
    tpkt[15] = pkt[13];
    /*Packet Word3*/
    tpkt[16] = pkt[14];
    tpkt[17] = pkt[15];
    tpkt[18] = pkt[16];
    tpkt[19] = 0;
    /*Packet Word4*/
    tpkt[20] = pkt[17];
    tpkt[21] = pkt[18];
    tpkt[22] = pkt[19];
    tpkt[23] = pkt[20];
    /*Packet Word5*/
    tpkt[24] = pkt[21];
    tpkt[25] = pkt[22];
    tpkt[26] = pkt[23];
    tpkt[27] = 0;
    /*Packet Word6*/
    tpkt[28] = pkt[24];
    tpkt[29] = pkt[25];
    tpkt[30] = pkt[26];
    tpkt[31] = pkt[27];
    /*Packet Word7*/
    tpkt[32] = pkt[28];
    tpkt[33] = pkt[29];
    tpkt[34] = pkt[30];
    tpkt[35] = 0;

    writel((1<<8) | (addr&0xff), HDMI_OPCR);
    writel(reg[0], HDMI_ORP6PH);
    writel(reg[1], HDMI_ORSP6W0);
    writel(reg[2], HDMI_ORSP6W1);
    writel(reg[3], HDMI_ORSP6W2);
    writel(reg[4], HDMI_ORSP6W3);
    writel(reg[5], HDMI_ORSP6W4);
    writel(reg[6], HDMI_ORSP6W5);
    writel(reg[7], HDMI_ORSP6W6);
    writel(reg[8], HDMI_ORSP6W7);

    //hdmi_EnableWriteRamPacket
    
    writel(readl(HDMI_OPCR) | HDMI_OPCR_ENRBPKTSRAM, HDMI_OPCR);

    while (readl(HDMI_OPCR) & HDMI_OPCR_ENRBPKTSRAM) {
        for (i = 0; i < 10; i++)
            ;
    }

    return 0;
}

/*
function:hdmi_gen_avi_infoframe
input:  colorformat :   0--RGB444;1--YCbCr422;2--YCbCr444
AR          :   1--4:3;   2--16:9
return: 0
*/

int hdmi_gen_avi_infoframe(struct hdmi_video_settings *settings)
{
    static unsigned char pkt[32];
    unsigned int checksum = 0;
    int i;

    struct video_parameters_t *v_parameters = NULL;

    v_parameters = (struct video_parameters_t *)hdmi_get_vid(settings);

    if (!v_parameters) {

        HDMI_DRV_PRINT("[%s]don't support this format!\n", __func__);
        return -1;

    }
    /* clear buffer */
    for (i = 0; i < 32; i++)
        pkt[i] = 0;

    pkt[0] = 0x80 | 0x02;
    pkt[1] = 2;
    pkt[2] = 0x1f & 13;
    pkt[3] = 0x00;

    /*PB1--Y1:Y0=colorformat;R3:R1 is invalid ;no bar info and scan info*/
    pkt[4] = (settings->pixel_encoding << 5) | (0 << 4) | (0 << 2) | (0);

    if (settings->color_xvycc == 0) {
        /*PB2--Colorimetry:SMPTE 170M|ITU601; Picture aspect Ratio;
         * same as picture aspect ratio*/
        pkt[5] = (v_parameters->colorimetry << 6) |
            (v_parameters->ar << 4) | (0x8);
        /*PB3--No known non-uniform scaling*/
        pkt[6] = 0x0;

    } else {

        /*PB2--Colorimetry:SMPTE 170M|ITU601; Picture aspect Ratio;
         * same as picture aspect ratio*/
        pkt[5] = (0x3<<6) | (v_parameters->ar << 4) | (0x8);

        if (settings->color_xvycc == 1) {

            /*PB3--xvYCC601;No known non-uniform scaling*/
            pkt[6] = 0x0;

        } else {
            /*PB3--xvYCC709;No known non-uniform scaling*/
            pkt[6] = 0x1<<4;
        }
    }

    /*PB4--Video Id*/
    if (v_parameters->vid > 70)
        pkt[7] = v_parameters->vid - 70;
    else
        pkt[7] = v_parameters->vid;
    /*PB5--Pixel repeat time:*/
    pkt[8] = v_parameters->pixel_repeat;
    /*PB6--PB13: Bar Info, no bar info*/
    pkt[9] = 0;
    pkt[10] = 0;
    pkt[11] = 0;
    pkt[12] = 0;
    pkt[13] = 0;
    pkt[14] = 0;
    pkt[15] = 0;
    pkt[16] = 0;

    for (i = 0; i < 31; i++)
        checksum += pkt[i];
    pkt[3] = (~checksum + 1) & 0xff;

    /* set to RAM Packet */
    hdmi_SetRamPacket(HDMI_RAMPKT_AVI_SLOT, pkt);
    hdmi_SetRamPacketPeriod(HDMI_RAMPKT_AVI_SLOT, HDMI_RAMPKT_PERIOD);

    return 0;
}

int hdmi_gen_audio_infoframe(struct hdmi_audio_settings *audio_general)
{
    static unsigned char pkt[32];
    unsigned int checksum = 0;
    int i;

    struct audio_parameters_t *a_parameters = NULL;

    HDMI_DRV_PRINT("[%s start]\n", __func__);
    a_parameters = (struct audio_parameters_t *)hdmi_get_aid(audio_general);

    if (!a_parameters) {
        HDMI_DRV_PRINT("[%s]audio info error!\n", __func__);
        return -1;
    }
    for (i = 0; i < 32; i++)
        pkt[i] = 0;

    pkt[0] = 0x80 | 0x04;
    pkt[1] = 1;
    pkt[2] = 0x1f & 10;
    pkt[3] = 0x00;

    if (a_parameters->audio60958 == 1) {
        /*PB1: Audio coding Type is refer to stream header
         * and 5 audio channel*/
        pkt[4] = a_parameters->audio_channel & 0x7;
        /* uncompressed format*/
        pkt[5] = 0x0;
        pkt[6] = 0x0;

        /* channel/speaker allocation:*/
        pkt[7] = a_parameters->AUDIOCA;
    }

    if (a_parameters->audio60958 == 0) {
        /*20091218by genganan to test 61937*/
        pkt[4] = 0;
        /*20091218by genganan to test 61937*/
        pkt[7] = 0;

    }
    /* Down-mix Inhibits Flag and level shift value
     *PB5: premitted or no information about Down-mix; 0dB level shift*/
    pkt[8] = (0x0 << 7) | (0x0 << 3);

    /* count checksum */
    for (i = 0; i < 31; i++)
        checksum += pkt[i];
    pkt[3] = (~checksum + 1)  & 0xff;

    hdmi_SetRamPacket(HDMI_RAMPKT_AUDIO_SLOT, pkt);
    hdmi_SetRamPacketPeriod(HDMI_RAMPKT_AUDIO_SLOT, HDMI_RAMPKT_PERIOD);

    HDMI_DRV_PRINT("[%s finished]\n", __func__);

    return 0;
}

/*
function:hdmi_gen_gbd_infoframe
input:  Color_xvYCC :   0--Normal YCC601 or YCC709; 1--xvYCC601; 2--xvYCC709
ColorDepth  :   0--24bit;   1--30bit;   2--36bit
return: 0
*/

int hdmi_gen_gbd_infoframe(struct hdmi_video_settings *video_general)
{
    static unsigned char pkt[32];
    unsigned int checksum = 0;
    unsigned char deep_color = video_general->deep_color;
    unsigned char color_xvycc = video_general->color_xvycc;
    int i;

    /* clear buffer */
    for (i = 0; i < 32; i++)
        pkt[i] = 0;

    pkt[0] = 0xa;
    /*Next_Field = 1; GBD_Profile = P0; Affected Gamut seq num = 1;*/
    pkt[1] = (0x1<<7) | (0x0<<4) | (0x1);
    /*Only Packet in sequence; current Gamut seq num = 1;*/
    pkt[2] = (0x3<<4) | (0x1);
    pkt[3] = 0x00;

    /*PB1--Format Flag; GBD_Color_Precision; GBD_Color_Space*/
    pkt[4] = (0x1<<7) | (deep_color<<3) | (color_xvycc);

    if (deep_color == 0) {
        pkt[5] = 0x0;               /*min Red data*/
        pkt[6] = 0xfe;              /*max Red data*/
        pkt[7] = 0x0;               /*min Green data*/
        pkt[8] = 0xfe;              /*max Green data*/
        pkt[9] = 0x0;               /*min Blue data*/
        pkt[10] = 0xfe;             /*max Blue data*/
    }
    /*30bit*/
    if (deep_color == 1) {
        pkt[5] = 0x0;               /*min Red data: 0x0*/
        pkt[6] = 0x3f;              /*max Red data: 0x3f8*/
        pkt[7] = 0x80;              /*min Green data*/
        pkt[8] = 0x3;               /*max Green data*/
        pkt[9] = 0xf8;              /*min Blue data*/
        pkt[10] = 0x0;              /*max Blue data*/
        pkt[11] = 0x3f;
        pkt[12] = 0x80;
    }
    /*36bit*/
    if (deep_color == 2) {
        pkt[5] = 0x0;               /*min Red data: 0x0*/
        pkt[6] = 0xf;               /*max Red data: 0xfe0*/
        pkt[7] = 0xe0;              /*min Green data*/
        pkt[8] = 0x0;               /*max Green data*/
        pkt[9] = 0xf;               /*min Blue data*/
        pkt[10] = 0xe0;             /*max Blue data*/
        pkt[11] = 0x0;
        pkt[12] = 0xf;
        pkt[13] = 0xe0;
    }

    for (i = 0; i < 31; i++)
        checksum += pkt[i];
    pkt[3] = (~checksum + 1) & 0xff;

    /* set to RAM Packet */

    hdmi_SetRamPacket(HDMI_RAMPKT_GBD_SLOT, pkt);
    hdmi_SetRamPacketPeriod(HDMI_RAMPKT_GBD_SLOT, HDMI_RAMPKT_PERIOD);

    return 0;
}

/*
 * hdmi_gen_vs_infoframe(Vendor Specific)
 * input:  3D format
 * return: 0
 */

int hdmi_gen_vs_infoframe(struct hdmi_video_settings *video_general)
{
    static unsigned char pkt[32];
    unsigned int checksum = 0;
    int i;

    /* clear buffer */
    for (i = 0; i < 32; i++)
        pkt[i] = 0;

    pkt[0] = 0x81;
    pkt[1] = 0x1;

    if (video_general->vid == 2) {
        /*length,  for Side-by-Side Half 3D*/
        pkt[2] = 0x6;

    } else {
        /*length,  for Frame 3D and Top-Bottom Half 3D*/
        pkt[2] = 0x5;

    }

    pkt[3] = 0x00;

    /*PB1--PB3:24bit IEEE Registration Identifier*/
    pkt[4] = 0x03;
    pkt[5] = 0x0c;
    pkt[6] = 0x00;

    /*PB4: HDMI_Video_Format:000--no additional;001--extended
     * resolution(4k*2k);010--3D format*/
    /*3D format*/
    pkt[7] = 0x2<<5;
    switch (video_general->vid) {
        /*3D Frame*/
    case   1:
        pkt[8] = 0x0<<4;
        pkt[9] = 0x0;
        break;
        /*Side-by-Side (Half)*/
    case   2:
        pkt[8] = 0x8<<4;
        pkt[9] = 0x1<<4;
        break;
        /*Top-and-bottom (Half)*/
    case   3:
        pkt[8] = 0x6<<4;
        pkt[9] = 0x0;
        break;
    default:
        break;
    }

    /* count checksum*/
    for (i = 0; i < 31; i++)
        checksum += pkt[i];
    pkt[3] = (~checksum + 1) & 0xff;

    /* set to RAM Packet */

    hdmi_SetRamPacket(HDMI_RAMPKT_VS_SLOT, pkt);
    hdmi_SetRamPacketPeriod(HDMI_RAMPKT_VS_SLOT, HDMI_RAMPKT_PERIOD);

    return 0;
}


void *hdmi_get_vid(struct hdmi_video_settings *settings)
{
    int format_index;

    format_index = sizeof(video_parameters) /
        sizeof(struct video_parameters_t) - 1;

    while (format_index >= 0) {

        if (settings->vid == video_parameters[format_index].vid)
            return (void *)&video_parameters[format_index];

        --format_index;
    };

    return NULL;

}

void *hdmi_get_aid(struct hdmi_audio_settings *a_general)
{
    int index;

    index = sizeof(audio_parameters) /
        sizeof(struct audio_parameters_t) - 1;

    while (index >= 0) {

        if (a_general->audio60958 == audio_parameters[index].audio60958) {
            if (!audio_parameters[index].audio_channel) {
                return (void *)&audio_parameters[index];
            } else {
                if (a_general->audio_channel ==
                    audio_parameters[index].audio_channel) {
                    return (void *)&audio_parameters[index];
                }
            }
        }
        --index;
    }

    return NULL;
}


void *hdmi_get_mode(struct hdmi_video_settings *settings)
{
    int index;

    index = sizeof(hdmi_display_modes) / sizeof(struct asoc_videomode) - 1;

    while (index >= 0) {
		printf("settings->vid %d hdmi_display_modes[index].mode.vid %d \n",settings->vid,hdmi_display_modes[index].vid);
        if (settings->vid == hdmi_display_modes[index].vid)
            return (void *)&hdmi_display_modes[index];
        --index;
    }

    return NULL;
}


/*****************************************************************
 * CMU_TVOUTPLL is used to generate clock for HDMI and CVBS, the
 * meaning of some fields of this register is listed as follows.
 * PLLFSS:
 *     00:100.8M 01:297M  02:108M
 * CLKPIXDIV:
 *     00:/1   01:/2    02:/4
 * DPCLM:
 *     00:*1   01:*1.25 02:*1.5
 *
 * ***************************************************************/
s32 hdmi_set_clk(int vid, int deep_color, int _3d)
{
    
    switch (vid) {      
    case OWL_TV_MOD_576P:
    case OWL_TV_MOD_480P :  
        if (_3d == _3D) {            
            writel(0x00030008, CMU_TVOUTPLL);            
        } else {                     
            if (deep_color == DEEP_COLOR_24_BIT) {
                writel(0x00010008, CMU_TVOUTPLL);
            } else if (deep_color == DEEP_COLOR_30_BIT) {
                writel(0x00110008, CMU_TVOUTPLL);
            }else if (deep_color == DEEP_COLOR_36_BIT) {
                writel(0x00210008, CMU_TVOUTPLL);
            }                      
        }
        break;
    case OWL_TV_MOD_720P_60HZ: 
    case OWL_TV_MOD_720P_50HZ:
        if (_3d == _3D) {           
            writel(0x00060008, CMU_TVOUTPLL);            
        } else {
            if (deep_color == DEEP_COLOR_24_BIT) {
                writel(0x00040008, CMU_TVOUTPLL); 
            } else if (deep_color == DEEP_COLOR_30_BIT) {
                writel(0x00140008, CMU_TVOUTPLL);
            }else if (deep_color == DEEP_COLOR_36_BIT) {
                writel(0x00240008, CMU_TVOUTPLL);
            }
            
        }
        break;

    case OWL_TV_MOD_1080P_60HZ://need check
    case OWL_TV_MOD_1080P_50HZ:
        if (_3d == _3D) { 
            writel(0x00070008, CMU_TVOUTPLL);            
        } else {
            if (deep_color == DEEP_COLOR_24_BIT) {
                writel(0x00060008, CMU_TVOUTPLL);
            } else if (deep_color == DEEP_COLOR_30_BIT) {
                writel(0x00160008, CMU_TVOUTPLL);
            }else if (deep_color == DEEP_COLOR_36_BIT) {
                writel(0x00260008, CMU_TVOUTPLL);
            }            
        }
        break;      
    case OWL_TV_MOD_4K_30HZ:
        if (deep_color == DEEP_COLOR_24_BIT) {
            writel(0x00070008, CMU_TVOUTPLL);
        } else if (deep_color == DEEP_COLOR_30_BIT) {
            writel(0x00160008, CMU_TVOUTPLL);
        } else if (deep_color == DEEP_COLOR_36_BIT) {
            writel(0x00260008, CMU_TVOUTPLL);
        }
        break;        
    default:           
        writel(0x00040008, CMU_TVOUTPLL);            
        break;
    }
    // enable 24M HZ 
    act_setl(1<<23, CMU_TVOUTPLL); 
}

void hdmi_phy_enable()
{
	writel((readl(HDMI_TX_2) | 0x00020f00), HDMI_TX_2);
}

s32 hdmi_set_tdms_ldo(int vid, int deep_color, int _3d)
{
	int hdmi_tx_1_value = 0;
	int hdmi_tx_2_value = 0;   
    switch (vid) {      
    case OWL_TV_MOD_576P:
    case OWL_TV_MOD_480P :  
        if (_3d == _3D) {
        	hdmi_tx_1_value =  0x81982983; 
        	hdmi_tx_2_value =  0x18f80f89;               
        } else {
            hdmi_tx_1_value =  0x819c2983; 
        	hdmi_tx_2_value =  0x18f80f89;                   
        }
        break;
    case OWL_TV_MOD_720P_60HZ: 
    case OWL_TV_MOD_720P_50HZ:
        if (_3d == _3D) {            
            hdmi_tx_1_value =  0x81902983; 
        	hdmi_tx_2_value =  0x18f80f89;             
        } else {
            if (deep_color == DEEP_COLOR_24_BIT) {
                hdmi_tx_1_value =  0x81942983; 
        		hdmi_tx_2_value =  0x18f80f89;   
            } else if (deep_color == DEEP_COLOR_30_BIT) {
                hdmi_tx_1_value =  0x81942983; 
        		hdmi_tx_2_value =  0x18f80f89;  
            }else if (deep_color == DEEP_COLOR_36_BIT) {
                hdmi_tx_1_value =  0x81942983; 
        		hdmi_tx_2_value =  0x18f80f89;  
            }
            
        }
        break;

    case OWL_TV_MOD_1080P_60HZ://need check
    case OWL_TV_MOD_1080P_50HZ:
        if (_3d == _3D) { 
            hdmi_tx_1_value =  0xa2b0285a; 
        	hdmi_tx_2_value =  0x18fa0f39;              
        } else {
            if (deep_color == DEEP_COLOR_24_BIT) {
                hdmi_tx_1_value =  0x81902983; 
        		hdmi_tx_2_value =  0x18f80f89;  
            } else if (deep_color == DEEP_COLOR_30_BIT) {
                hdmi_tx_1_value =  0xa2b0285a; 
        		hdmi_tx_2_value =  0x18fa0f39;  
            }else if (deep_color == DEEP_COLOR_36_BIT) {
                hdmi_tx_1_value =  0xa2b0285a; 
        		hdmi_tx_2_value =  0x18fa0f39;  
            }
            
        }
        break;      
    case OWL_TV_MOD_4K_30HZ:
        if (deep_color == DEEP_COLOR_24_BIT) {
            hdmi_tx_1_value =  0xa2b0285a; 
        	hdmi_tx_2_value =  0x18fa0f39;   
        } else if (deep_color == DEEP_COLOR_30_BIT) {
            hdmi_tx_1_value =  0x81900983; 
        	hdmi_tx_2_value =  0x18f80089;   
        } else if (deep_color == DEEP_COLOR_36_BIT) {
            hdmi_tx_1_value =  0x81900983; 
        	hdmi_tx_2_value =  0x18f80089;   
        }
        break;        
    default:
        hdmi_tx_1_value =  0x81982983; 
        hdmi_tx_2_value =  0x18f80089;              
        break;
    }   
    writel((hdmi_tx_2_value & 0xfffdf0ff), HDMI_TX_2);
    udelay(500);
    writel(hdmi_tx_1_value, HDMI_TX_1);
}

int hdmi_set_outmode(struct hdmi_video_settings *settings)
{

    if (settings->hdmi_mode >= HDMI_MODE_MAX) {

        HDMI_DRV_PRINT("[%s]invalid hdmi mode!\n", __func__);
        return -1;

    } else if (settings->hdmi_mode == HDMI_MODE_HDMI) {

        act_setl(HDMI_SCHCR_HDMI_MODESEL, HDMI_SCHCR);

    } else if (settings->hdmi_mode == HDMI_MODE_DVI) {

        act_clearl(HDMI_SCHCR_HDMI_MODESEL, HDMI_SCHCR);

    } else {

        HDMI_DRV_PRINT("[%s]don'n support this mode!\n", __func__);
        return -1;

    }

    return 0;
}

static void hdmi_enable_module(void)
{
    act_setl(CMU_DEVCLKEN1_HDMI, CMU_DEVCLKEN1);
    readl(CMU_DEVCLKEN1);
    act_setl(HDMI_CR_ENHPINT | HDMI_CR_PHPLG | HDMI_CR_HPDENABLE, HDMI_CR);
    act_setl(HDMI_CR_HPDEBOUNCE(0xF), HDMI_CR);
    act_setl(HDMI_CR_FIFO_FILL | HDMI_CR_PKTGB2V_ENABLE, HDMI_CR);
}

static void hdmi_disable_module(void)
{
    act_clearl(CMU_DEVCLKEN1_HDMI, CMU_DEVCLKEN1);
    readl(CMU_DEVCLKEN1);
    
}

void hdmi_enable_output(void)
{
    act_setl(TMDS_EODR0_TMDS_ENCEN, TMDS_EODR0);
    act_setl(HDMI_TX_2_REG_TX_PU(0xf), HDMI_TX_2);
    act_setl(HDMI_CR_ENABLEHDMI, HDMI_CR);
}

void hdmi_disable_output(void)
{

    act_clearl(1<<23, CMU_TVOUTPLL); 
    act_clearl(TMDS_EODR0_TMDS_ENCEN, TMDS_EODR0);
    act_clearl(HDMI_TX_2_REG_TX_PU(0xf), HDMI_TX_2);
    act_clearl(HDMI_TX_2_REG_TX_RT_EN, HDMI_TX_2);

    act_clearl(HDMI_CR_ENABLEHDMI, HDMI_CR);
}

/**********************************************************
 * This function is used to set the value of AVMUTE flag
 * and the depth of the color of pixel.
 * *******************************************************/
void hdmi_gcp_cfg(struct hdmi_video_settings *v_general)
{
    int deep_color = v_general->deep_color;

    if (deep_color == 0) {
        /*Clear AVMute flag and enable GCP transmission */
        writel(0x0 | 0x80000002, HDMI_GCPCR);
    }

    if (deep_color == 1)
        writel(0x40000050 | 0x80000002, HDMI_GCPCR);

    if (deep_color == 2)
        writel(0x40000060 | 0x80000002, HDMI_GCPCR);
}

/*---------------------audio_interface---------------------*/
void hdmi_audio_enable(void)
{
    writel(readl(HDMI_ICR) | 0x02000000, HDMI_ICR);
}

void hdmi_acr_packet(struct hdmi_audio_settings *a_general)
{
    int audio_fs = a_general->audio_fs;

    switch (audio_fs) {

    case 1:
        /*32kHz--4.096MHz */
        writel(0x1000 | 0x80000000, HDMI_ACRPCR);
        break;

    case 2:
        /*44.1kHz--5.6448MHz */
        writel(0x1880 | 0x80000000, HDMI_ACRPCR);
        break;

    case 3:
        /*48kHz--6.144MHz */
        writel(0x1800 | 0x80000000, HDMI_ACRPCR);
        break;

    case 4:
        /*88.2kHz--11.2896MHz */
        writel(0x3100 | 0x80000000, HDMI_ACRPCR);
        break;

    case 5:
        /*96kHz--12.288MHz */
        writel(0x3000 | 0x80000000, HDMI_ACRPCR);
        break;

    case 6:
        /*176.4kHz--22.5792MHz */
        writel(0x3000 | 0x80000000, HDMI_ACRPCR);
        break;

    case 7:
        /*192kHz--24.576MHz */
        writel(0x6000 | 0x80000000, HDMI_ACRPCR);
        break;

    case 8:
        /*352.8kHz--45.1584MHz */
        writel(0x3100 | 0x80000000, HDMI_ACRPCR);
        break;

    case 9:
        /*384kHz--49.152MHz */
        writel(0x3000 | 0x80000000, HDMI_ACRPCR);
        break;

    default:
        break;
    }

}


/*audio general config*/
void hdmi_audio_general_cfg(struct hdmi_audio_settings *a_general)
{
    int audio60958 = a_general->audio60958;
    int audio_fs = a_general->audio_fs;
    int audio_channel = a_general->audio_channel;
    int ASPCR = 0;
    int ACACR = 0;
    int AUDIOCA = 0;

    HDMI_DRV_PRINT("[%s start]\n", __func__);


        //: set HDMIA_CLK to one default clk which tv can support
        writel(0x5000010, CMU_AUDIOPLL);

    writel(0x80000000, HDMI_ACRPCR);

    if (audio60958 == 1) {

        switch (audio_channel) {

        case 1:
            /*mode1:0(0x0<<9)|(0x0<<8)|(0x11)
               (0<<0)|(1<<3)|(2<<6)|(3<<9)|(4<<12)|(5<<15)|(6<<18)|(7<<21)
               Channel2-1:FR;FL */
            ASPCR = 0x00000011;
            ACACR = 0xfac688;
            AUDIOCA = 0x0;
            break;

        case 2:
            /*(0x5<<15)|(0x1<<14)|(0x5<<10)|(0x1<<9)|(0x1<<8)|(0x13)
               (0x4<<12)|(0x0<<9)|(0x0<<6)|(0x1<<3)|(0x0<<0)
               Channel 5-1: FC;*;*;FR;FL */
            ASPCR = 0x0002d713;
            ACACR = 0x4008;
            AUDIOCA = 0x4;
            break;

        case 3:
            /*(0x7<<15)|(0x1<<14)|(0x7<<10)|(0x1<<9)|(0x1<<8)|(0x1b<<0)
               (0x4<<12)|(0x3<<9)|(0x0<<6)|(0x1<<3)|(0x0<<0)
               Channel 5-1: RC;FC;*;FR;FL */
            ASPCR = 0x0003df1b;
            ACACR = 0x4608;
            AUDIOCA = 0x6;
            break;

        case 4:
            /*(0x7<<15)|(0x1<<14)|(0x7<<10)|(0x1<<9)|(0x1<<8)|(0x3b<<0)
               (0x5<<15)|(0x4<<12)|(0x3<<9)|(0x0<<6)|(0x1<<3)|(0x0<<0)
               Channel 6-1: RR;RL;FC;*;FR;FL */
            ASPCR = 0x0003df3b;
            ACACR = 0x34608;
            AUDIOCA = 0xa;
            break;

        case 5:
            /*(0x7<<15)|(0x1<<14)|(0x7<<10)|(0x1<<9)|(0x1<<8)|(0x3F<<0)
               (0x5<<15)|(0x4<<12)|(0x3<<9)|(0x2<<6)|(0x1<<3)|(0x0<<0)
               Channel 6-1: RR;RL;FC;LFE;FR;FL */
            ASPCR = 0x0003df3f;
            ACACR = 0x2c688;
            AUDIOCA = 0xB;
            break;

        case 6:
            /*(0xf<<15)|(0x1<<14)|(0xf<<10)|(0x1<<9)|(0x1<<8)|(0x7F<<0)
               (6<<18)|(0x5<<15)|(0x4<<12)|(0x3<<9)|(0x2<<6)|(0x1<<3)|(0x0<<0)
               Channel 7-1: RC;RR;RL;FC;LFE;FR;FL */
            ASPCR = 0x0007ff7f;
            ACACR = 0x1ac688;
            AUDIOCA = 0xf;
            break;

        case 7:
            /*(0xf<<15)|(0x1<<14)|(0xf<<10)|(0x1<<9)|(0x1<<8)|(0xff<<0)
               (0<<0)|(1<<3)|(2<<6)|(3<<9)|(4<<12)|(5<<15)|(6<<18)|(7<<21)
               Channel 8-1: RRC;RLC;RR;RL;FC;LFE;FR;FL */
            ASPCR = 0x0007ffff;
            ACACR = 0xfac688;
            AUDIOCA = 0x13;
            break;

        default:
            break;
        }

    } else {
        /*(0xf<<27)|(0xf<<23)|(0xf<<15)|(0x1<<14)||(0x3<<0)
           Force SPMode3, All SP active | (1<<3)|(0<<0) */
        ASPCR = 0x7f87c003;
        ACACR = 0xfac688;
    }

    if (audio60958 == 1) {
        ASPCR &= 0x007fffff;
        if (audio_fs > 7) {
            /*High bitrate audio */
            ASPCR |= 0x80000000;
        }

        writel(ASPCR, HDMI_ASPCR);
    }

    writel(ACACR, HDMI_ACACR);

    HDMI_DRV_PRINT("[%s finished]\n", __func__);

}


/*audio interface channel status config*/
void hdmi_aichsta_cfg(struct hdmi_audio_settings *a_general)
{
    int audio_fs = a_general->audio_fs;
    int audio60958 = a_general->audio60958;

    HDMI_DRV_PRINT("[%s start]\n", __func__);

    switch (audio_fs) {

    case 1:
        /*Set Audio Interface---32kHz, 24bit, IEC60958-3
           channel Status setting:
           24bits,32kHz,IEC60958-3
           original sampling frequency 32kHz, support 24bit */
        writel(0x03000000, HDMI_AICHSTABYTE0TO3);
        writel(0xcb, HDMI_AICHSTABYTE4TO7);
        break;

    case 2:
        /*Set Audio Interface---44.1kHz, 24bit, IEC60958-3
           channel Status setting:
           16bits, 44.1kHz,IEC60958-3
           original sampling frequency 44.1kHz, support 24bit */
        writel(0x00000000, HDMI_AICHSTABYTE0TO3);
        writel(0xf2, HDMI_AICHSTABYTE4TO7);
        break;

    case 3:
        /*Set Audio Interface---48kHz, 24bit, IEC60958-3
           channel Status setting:
           24bits,48kHz,IEC60958-3
           original sampling frequency 48kHz, support 24bit */
        writel(0x02000000, HDMI_AICHSTABYTE0TO3);
        writel(0xdb, HDMI_AICHSTABYTE4TO7);
        break;

    case 4:
        /*Set Audio Interface---88.2kHz, 24bit, IEC60958-3
           channel Status setting:
           24bits,88.2kHz,IEC60958-3
           original sampling frequency 88.2kHz, support 24bit */
        writel(0x08000000, HDMI_AICHSTABYTE0TO3);
        writel(0x7b, HDMI_AICHSTABYTE4TO7);
        break;

    case 5:
        /*Set Audio Interface---96kHz, 24bit, IEC60958-3
           channel Status setting:
           24bits,96kHz,IEC60958-3
           original sampling frequency 96kHz, support 24bit */
        writel(0x0a000000, HDMI_AICHSTABYTE0TO3);
        writel(0x5b, HDMI_AICHSTABYTE4TO7);
        break;

    case 6:
        /*Set Audio Interface---176.4kHz, 24bit, IEC60958-3
           channel Status setting:
           24bits,176.4kHz,IEC60958-3
           original sampling frequency 176.4kHz, suppo */
        writel(0x0c000000, HDMI_AICHSTABYTE0TO3);
        writel(0x3b, HDMI_AICHSTABYTE4TO7);
        break;

    case 7:
        /*Set Audio Interface---192kHz, 24bit, IEC60958-3
           channel Status setting:
           24bits,192kHz,IEC60958-3
           original sampling frequency 192kHz, support */
        writel(0x0e000000, HDMI_AICHSTABYTE0TO3);
        writel(0x1b, HDMI_AICHSTABYTE4TO7);
        break;

    default:
        /*Set Audio Interface---352.8/384kHz, 24bit, IEC60958-3
           channel Status setting:
           24bits,Not indicated,IEC60958-3
           original sampling frequency Not indicated, support 24bit */
        if (audio_fs > 7) {
            writel(0x01000000, HDMI_AICHSTABYTE0TO3);
            writel(0x0b, HDMI_AICHSTABYTE4TO7);
        }

        break;
    }

    if (audio60958 == 0) {

        writel((readl(HDMI_AICHSTABYTE0TO3) &
                ~(HDMI_AICHSTABYTE0TO3_60958_4_USERTYPE(0x3))) |
               HDMI_AICHSTABYTE0TO3_60958_4_USERTYPE(0x2),
               HDMI_AICHSTABYTE0TO3);

    } else {

        writel(readl(HDMI_AICHSTABYTE0TO3) &
               ~(HDMI_AICHSTABYTE0TO3_60958_4_USERTYPE(0x3)),
               HDMI_AICHSTABYTE0TO3);
    }

    writel(0x0, HDMI_AICHSTABYTE8TO11);
    writel(0x0, HDMI_AICHSTABYTE12TO15);
    writel(0x0, HDMI_AICHSTABYTE16TO19);
    writel(0x0, HDMI_AICHSTABYTE20TO23);

    HDMI_DRV_PRINT("[%s finished]\n", __func__);

}


/*audio channel config*/
void hdmi_audioch_cfg(struct hdmi_audio_settings *a_general)
{
    int audio_channel = a_general->audio_channel;
    int audio60958 = a_general->audio60958;

    HDMI_DRV_PRINT("[%s start]\n", __func__);

    switch (audio_channel) {

    case 0:
        writel(0x21, HDMI_AICHSTASCN);
        break;

    case 1:
        /*2Channel, channel1 is left channel and channel5 is right channel */
        writel(0x20001, HDMI_AICHSTASCN);
        break;

    case 2:
        /*3Channel,channel1&3 are left channel,channel2 is right channel */
        writel(0x121, HDMI_AICHSTASCN);
        break;

    case 3:
        /*4Channel */
        writel(0x2121, HDMI_AICHSTASCN);
        break;

    case 4:
        /*5Channel */
        writel(0x12121, HDMI_AICHSTASCN);
        break;

    case 5:
        /*6Channel */
        writel(0x212121, HDMI_AICHSTASCN);
        break;

    case 6:
        /*7Channel */
        writel(0x1212121, HDMI_AICHSTASCN);
        break;

    case 7:
        /*8Channel */
        writel(0x21212121, HDMI_AICHSTASCN);
        break;

    default:
        break;
    }

    if (audio60958 == 0)
        writel(0x21, HDMI_AICHSTASCN);

    HDMI_DRV_PRINT("[%s finished]\n", __func__);

}


/*---------------------video_interface---------------------*/
void hdmi_ldo_and_pll_cfg(struct hdmi_video_settings *settings)
{
    int _3d = settings->_3d;
    int deep_color = settings->deep_color;
    int vid = settings->vid;
    hdmi_set_tdms_ldo(vid ,deep_color, _3d);
    hdmi_set_clk(vid ,deep_color, _3d);
    HDMI_DRV_PRINT("[%s finished]\n", __func__);
}


s32 hdmi_video_general_cfg(struct hdmi_video_settings *settings)
{
    int repeat;
    struct video_parameters_t *v_parameters = NULL;

    HDMI_DRV_PRINT("[%s start]\n", __func__);

    v_parameters = (struct video_parameters_t *)hdmi_get_vid(settings);

    if (v_parameters) {

        repeat = v_parameters->pixel_repeat;
        /*pixel repeat */
        if (repeat) {

            writel(v_parameters->VICTL |
                   HDMI_VICTL_PDIV2_ENABLE_SCALER, HDMI_VICTL);

        } else {

            writel(v_parameters->VICTL, HDMI_VICTL);

        }

        /*video interface */
        writel(v_parameters->VIVSYNC, HDMI_VIVSYNC);
        writel(v_parameters->VIVHSYNC, HDMI_VIVHSYNC);
        writel(v_parameters->VIALSEOF, HDMI_VIALSEOF);
        writel(v_parameters->VIALSEEF, HDMI_VIALSEEF);
        writel(v_parameters->VIADLSE, HDMI_VIADLSE);

        writel(readl(HDMI_SCHCR) |
               (v_parameters->VHSYNC_P + v_parameters->VHSYNC_INV),
               HDMI_SCHCR);

        if (sink_info.sink_cap.hdmi_mode == HDMI_MODE_HDMI) {
            writel(readl(HDMI_SCHCR) | 0x1, HDMI_SCHCR);
            HDMI_DRV_PRINT("HDMI_SCHCR(0x%x) value is 0x%x\n",
                       HDMI_SCHCR, readl(HDMI_SCHCR));
        }

        printf("[%s]sink_info.sink_cap.pixel_encoding is 0x%x\n",
               __func__, sink_info.sink_cap.pixel_encoding);
        /*pixel_coding(RGB default) */
        if ((settings->pixel_encoding == VIDEO_PEXEL_ENCODING_YCbCr444)
            && (sink_info.sink_cap.pixel_encoding & BIT1)) {

            writel(readl(HDMI_SCHCR) | 0x20, HDMI_SCHCR);

        }

        /*deep color,8bit default */
        if (settings->deep_color == DEEP_COLOR_24_BIT) {

            writel(readl(HDMI_SCHCR) &
                   ~HDMI_SCHCR_DEEPCOLOR_MASK, HDMI_SCHCR);

        } else if (settings->deep_color == DEEP_COLOR_30_BIT) {

            writel((readl(HDMI_SCHCR) &
                    ~HDMI_SCHCR_DEEPCOLOR_MASK) |
                   HDMI_SCHCR_DEEPCOLOR(0x1), HDMI_SCHCR);

        } else if (settings->deep_color == DEEP_COLOR_36_BIT) {

            writel((readl(HDMI_SCHCR) &
                    ~HDMI_SCHCR_DEEPCOLOR_MASK) |
                   HDMI_SCHCR_DEEPCOLOR(0x2), HDMI_SCHCR);
        }

        /*3d support */
        if (settings->_3d == _3D) {

            writel(readl(HDMI_SCHCR) |
                   HDMI_SCHCR_HDMI_3D_FRAME_FLAG, HDMI_SCHCR);

        } else {

            writel(readl(HDMI_SCHCR) &
                   ~HDMI_SCHCR_HDMI_3D_FRAME_FLAG, HDMI_SCHCR);

        }
        HDMI_DRV_PRINT("set dipccr: %#x\n", v_parameters->DIPCCR);
        /*Max 18 and 4 Island packets in Vertical and horizontal Blanking Interval */
        writel(v_parameters->DIPCCR, HDMI_DIPCCR);

    } else {

        HDMI_DRV_PRINT("[%s]don't support this format!\n", __func__);

        return -1;

    }

    HDMI_DRV_PRINT("[%s finished]\n", __func__);

    return 0;
}



void hdmi_audio_cfg(struct hdmi_audio_settings *a_settings)
{
    hdmi_aichsta_cfg(a_settings);
    hdmi_audioch_cfg(a_settings);
    hdmi_audio_general_cfg(a_settings);
    hdmi_acr_packet(a_settings);
    /*enable CRP */
    act_clearl(HDMI_ACRPCR_DISABLECRP, HDMI_ACRPCR);
    hdmi_gen_audio_infoframe(a_settings);
    hdmi_audio_enable();
    HDMI_DRV_PRINT("[%s finished]\n", __func__);
}

s32 hdmi_video_cfg(struct hdmi_video_settings *settings)
{
    s32 ret;
    
    hdmi_ldo_and_pll_cfg(settings);
    
    ret = hdmi_video_general_cfg(settings);
    /*packet */
    hdmi_gcp_cfg(settings);
    hdmi_gen_avi_infoframe(settings);
    if (settings->color_xvycc != YCC601_YCC709)
        hdmi_gen_gbd_infoframe(settings);

    if (settings->_3d != _3D_NOT)
        hdmi_gen_vs_infoframe(settings);

    if (settings->hdmi_src == VITD)
        act_setl(HDMI_ICR_VITD(0x809050) | HDMI_ICR_ENVITD, HDMI_ICR);
    else
        act_clearl(HDMI_ICR_ENVITD, HDMI_ICR);
    
    hdmi_phy_enable();

    return ret;
}


/*----------------------hdmi general cfg--------------------*/
void hdmi_general_cfg(struct hdmi_sink_info *sink_info)
{
    /*video */
    hdmi_video_cfg(&sink_info->v_settings);
    if (HDMI_MODE_HDMI == sink_info->v_settings.hdmi_mode)
        hdmi_audio_cfg(&sink_info->a_settings);

    hdmi_set_outmode(&sink_info->v_settings);
    HDMI_DRV_PRINT("[%s finished]\n", __func__);
}



int hdmi_get_plug_state(void)
{
    unsigned int status_val;

    act_setl(HDMI_CR_ENHPINT | HDMI_CR_PHPLG | HDMI_CR_HPDENABLE, HDMI_CR);

    status_val = readl(HDMI_CR) & HDMI_CR_HPLGSTATUS;
    
    mdelay(2);
    
    status_val &= readl(HDMI_CR) & HDMI_CR_HPLGSTATUS;

    if (status_val) 
        return HDMI_PLUGIN;        
    else 
        return HDMI_PLUGOUT;
}

static struct display_ops hdmi_ops = {
    .enable = hdmi_enable_output,
    .disable = hdmi_disable_output,
};


struct data_fmt_param {
    const char *name;
    s32 data_fmt;
};

static int valide_vid(int vid)
{
    switch(vid) {
        case OWL_TV_MOD_480P:
        case OWL_TV_MOD_576P:
        case OWL_TV_MOD_720P_50HZ:
        case OWL_TV_MOD_720P_60HZ:
        case OWL_TV_MOD_1080P_50HZ:
        case OWL_TV_MOD_1080P_60HZ:
        case OWL_TV_MOD_4K_30HZ:
        case OWL_TV_MOD_DVI:
            return 1;
        default:
            return 0;
    }

}

static struct data_fmt_param date_fmts[] = {
	{"1280x720p-50", OWL_TV_MOD_720P_50HZ},
	{"1280x720p-60", OWL_TV_MOD_720P_60HZ},
	{"1920x1080p-50", OWL_TV_MOD_1080P_50HZ},
	{"1920x1080p-60", OWL_TV_MOD_1080P_60HZ},
	{"720x576p-60", OWL_TV_MOD_576P},
	{"720x480p-60", OWL_TV_MOD_480P},
	{"DVI", OWL_TV_MOD_DVI},
	{"PAL", OWL_TV_MOD_PAL},
	{"NTSC", OWL_TV_MOD_NTSC},
	{"4K30HZ", OWL_TV_MOD_4K_30HZ},
};

static u32 string_to_data_fmt(const char *name)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(date_fmts); i++) {
		if (!strcmp(date_fmts[i].name, name))
			return date_fmts[i].data_fmt;
	}

	return -1;
}
static int fdtdec_get_hdmi_par(int *  bootable, int *  bootrotate, int * bootvid , int * channel_invert,int * bit_invert ,int *i2cb)
{
    int dev_node;
    const char *resolution ;
    int len;
    unsigned int vid = 0;
    int ret = 0;

    debug("%s\n", __func__);

    dev_node = fdtdec_next_compatible(gd->fdt_blob, 0, COMPAT_ACTIONS_OWL_HDMI);
   
    if (dev_node <= 0) {
        printf("%s: failed get default vid ,we used  default \n",__func__);
        return -1;
    }
    
    *bootable = fdtdec_get_int(gd->fdt_blob, dev_node, "bootable", 0);
    
    if(*bootable != 0)
    {
    	*bootrotate = fdtdec_get_int(gd->fdt_blob, dev_node, "bootrotate", 0);
    	
    	*i2cb = fdtdec_get_int(gd->fdt_blob, dev_node, "i2cbus", 0); 
    	
    	*channel_invert = fdtdec_get_int(gd->fdt_blob, dev_node, "channel_invert", 0);
    	
    	*bit_invert = fdtdec_get_int(gd->fdt_blob, dev_node, "bit_invert", 0);
    	
    	resolution = fdt_getprop(gd->fdt_blob, dev_node, "default_resolution", &len);
	    debug("%s: resolution  = %s\n",__func__, resolution);
	
	    vid = string_to_data_fmt(resolution);
	    
	    if (valide_vid(vid)){
	        * bootvid = vid;
	    } else {    	
	    	* bootvid = -1; 
	        printf("%s: not support %s ,we used default vid  = %d\n",__func__, resolution, bootvid);
	    }
    }
    return ret;
}


#ifdef CONFIG_CMD_EXT4
#define EXT4_CACHE_PART 5


const char* HDMI_SETTING_MODE_PATH = "setting/setting_hdmi_mode";

int read_usr_cfg_file(const char* file_name, char* buf)
{
    static disk_partition_t info;
    block_dev_desc_t *dev_desc = NULL;
    int dev = 0;
    int part = EXT4_CACHE_PART;
    int err;
    int index=0;
    loff_t len;
    loff_t actread;
    const char* ifname;
    
    printf("read_usr_cfg_file\n");

    ifname = getenv("devif");
    if ( ifname == NULL ) {
        ifname = "nand";
        printf("get devif fail\n");
    }
    dev = get_boot_dev_num();
    
    dev_desc = get_dev(ifname, dev);
    if (dev_desc == NULL) {
        printf("Failed to find %s%d\n", ifname, dev);
        return 1;
    }

    if (get_partition_info(dev_desc, part, &info)) {
        printf("** get_partition_info %s%d:%d\n",
                ifname, dev, part);

        if (part != 0) {
            printf("** Partition %d not valid on device %d **\n",
                    part, dev_desc->dev);
            return -1;
        }

        info.start = 0;
        info.size = dev_desc->lba;
        info.blksz = dev_desc->blksz;
        info.name[0] = 0;
        info.type[0] = 0;
        info.bootable = 0;
#ifdef CONFIG_PARTITION_UUIDS
        info.uuid[0] = 0;
#endif
    }

    ext4fs_set_blk_dev(dev_desc, &info);

    if (!ext4fs_mount(info.size)) {
        printf("Failed to mount %s%d:%d\n",
            ifname, dev, part);
        ext4fs_close();
        return 1;
    }

    err = ext4fs_open(file_name, &len);
    if (err < 0) {
        printf("** File not found: %s\n", file_name);
        ext4fs_close();
        return 1;
    }

    

    
    err = ext4fs_read(buf, 64, &actread);
    if (err < 0) {
        printf("** File read error: %s\n", file_name);
        ext4fs_close();
        return 1;
    }
    ext4fs_close();
    
    printf("buf=%s", buf);
    
  
    return 0;
}
#endif


static int skip_atoi(char *s)
{
#define is_digit(c)    ((c) >= '0' && (c) <= '9')
   int i = 0;
   while (is_digit(*s))
       i = i * 10 + *((s)++) - '0';
   return i;
}

int hdmi_init(void)
{
    struct asoc_videomode *v_mode = NULL;
    struct hdmi_video_settings settings;
    int i=0; 
    char buf[256]={0};
    char * bootargs ;
    int bootable,bootrotate,bootvid,channel_invert,bit_invert,i2cb;

    struct hdmi_sink_info* psink_info = &sink_info;
    
    if (fdtdec_get_hdmi_par(&bootable,&bootrotate,&bootvid,&channel_invert,&bit_invert,&i2cb)) {
       printf("%s: error, fdtdec_get_hdmi_par: fdt No hdmi par, and now do nothing temply\n", __func__);
    }
    
    if(bootable != 0 && hdmi_get_plug_state() == 1 && (bootrotate == 0 || bootrotate == 3)){   
    	     
		bootvid = check_hdmi_mode(bootvid,i2cb);
		
	    if (valide_vid(bootvid)){
	        psink_info->v_settings.vid = bootvid;
	        printf("%s: read vid  success = %d\n",__func__, bootvid);
	    } else {
	        printf("%s: first time used config vid  = %d\n",__func__, psink_info->v_settings.vid );
	    }
	
		if(bootvid == 7)
		{
			sink_info.v_settings.hdmi_mode = HDMI_MODE_DVI;
		}
				
	    settings = sink_info.v_settings;
	    act_setl(1<<28 , USB3_P0_CTL);
	    act_setl(1<<3|1<<0 , CMU_DEVCLKEN1);
	    act_setl(HDMI_CR_ENHPINT | HDMI_CR_PHPLG | HDMI_CR_HPDENABLE, HDMI_CR);
	    act_setl(HDMI_CR_HPDEBOUNCE(0xF), HDMI_CR);
	    act_setl(HDMI_CR_FIFO_FILL | HDMI_CR_PKTGB2V_ENABLE, HDMI_CR);
		
		if(channel_invert != 0)
		{
			act_setl(HDMI_SCHCR_HDMI_CHANNEL_INVERT, HDMI_SCHCR);
		}
		
		if(bit_invert != 0)
		{
		    act_setl(HDMI_SCHCR_HDMI_BIT_INVERT, HDMI_SCHCR);	
		}
	    
	    HDMI_DRV_PRINT("[%s start]\n", __func__);
	    
	    v_mode = (struct asoc_videomode *)hdmi_get_mode(&settings);
	    
	    hdmi_general_cfg(&sink_info);
	
	    /* transmit hdmi vid through bootargs. */
		
	    bootargs = getenv("bootargs.add");
	    	    
		if (bootargs == NULL)
			sprintf(buf, "hdmi_vid=%d",settings.vid);
		else
			sprintf(buf, "%s hdmi_vid=%d",bootargs,settings.vid);
		
		printf("hdmi_init bootargs %s \n",buf);
		
		setenv("bootargs.add", buf);
	    	
		
		HDMI_DRV_PRINT("[%s owl_display_register v_mode->mode %p settings.vid %d \n", __func__,v_mode->mode,settings.vid);
	    owl_display_register(HDMI_DISPLAYER,"hdmi",&hdmi_ops, &(v_mode->mode),24,bootrotate);
	}
	
    return 0;
}


