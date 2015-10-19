/*
 * asoc_hdmi_packet.c
 *
 * Copyright (C) 2011 Actions Semiconductor, Inc
 * Author:  Geng A-nan <genganan@actions-semi.com >
 * Data: Monday August 22, 2008
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/seq_file.h>

#include "hdmi_ip.h"
#include "hdmi.h"
#include "../../dss/dss_features.h"
#include "../../dss/dss.h"
/* Internal SRAM allocation for Periodic Data Island Packet */

#define HDMI_RAMPKT_AVI_SLOT    0
#define HDMI_RAMPKT_AUDIO_SLOT  1
#define HDMI_RAMPKT_SPD_SLOT    2
#define HDMI_RAMPKT_GBD_SLOT    3
#define HDMI_RAMPKT_VS_SLOT     4
#define HDMI_RAMPKT_MPEG_SLOT   5

#define HDMI_RAMPKT_PERIOD      1


#define owl_hdmi_write(a, b)  hdmi.ip_data.ops->write_reg(&hdmi.ip_data, a, b)
#define owl_hdmi_read(a)      hdmi.ip_data.ops->read_reg(&hdmi.ip_data, a)

/* Packet Command */

int hdmi_EnableWriteRamPacket(void)
{
    
    int i;  
    unsigned int tmp_reg_value;
 
    tmp_reg_value = owl_hdmi_read(HDMI_OPCR);
    tmp_reg_value |= (1<<31);
    owl_hdmi_write(HDMI_OPCR,tmp_reg_value); 
    
	i=100;
    while(i--)
    {
        tmp_reg_value = owl_hdmi_read(HDMI_OPCR);
        tmp_reg_value = tmp_reg_value>>31;
        if (tmp_reg_value==0)
           break;
		udelay(1); 
    }
    
    return 0;
}

int hdmi_SetRamPacketPeriod(unsigned int no, int period)
{
    unsigned int tmp_reg_value;
    
    if(no>5 || no<0)
        return -1;
    if(period>0xf || period<0)
        return -1;

    tmp_reg_value = owl_hdmi_read(HDMI_RPCR);
    tmp_reg_value &= (~(1<<no));
    owl_hdmi_write(HDMI_RPCR,  tmp_reg_value);
    
    tmp_reg_value = owl_hdmi_read(HDMI_RPCR);
    tmp_reg_value &= (~(0xf<<(no*4+8)));
    owl_hdmi_write(HDMI_RPCR,  tmp_reg_value);


    if(period) {    // enable and set period
        tmp_reg_value = owl_hdmi_read(HDMI_RPCR);
        tmp_reg_value |= (period<<(no*4+8));
        owl_hdmi_write(HDMI_RPCR,  tmp_reg_value);

        tmp_reg_value = owl_hdmi_read(HDMI_RPCR);
        tmp_reg_value |= (1<<no);
        owl_hdmi_write(HDMI_RPCR,  tmp_reg_value);
        
    }

    return 0;
}

/*
    convert readable Data Island packet to RAM packet format,
    and write to RAM packet area
*/

int hdmi_SetRamPacket(unsigned int no, unsigned char *pkt)
{
    unsigned char tpkt[36];
    unsigned int reg[9];
    int i,j;
    unsigned int addr = 126 + no * 14;


    if(no > 5 || no < 0)
        return -1;

    //Packet Header
    tpkt[0] = pkt[0];
    tpkt[1] = pkt[1];
    tpkt[2] = pkt[2];
    tpkt[3] = 0;
    //Packet Word0
    tpkt[4] = pkt[3];
    tpkt[5] = pkt[4];
    tpkt[6] = pkt[5];
    tpkt[7] = pkt[6];
    //Packet Word1
    tpkt[8] = pkt[7];
    tpkt[9] = pkt[8];
    tpkt[10] = pkt[9];
    tpkt[11] = 0;
    //Packet Word2
    tpkt[12] = pkt[10];
    tpkt[13] = pkt[11];
    tpkt[14] = pkt[12];
    tpkt[15] = pkt[13];
    //Packet Word3
    tpkt[16] = pkt[14];
    tpkt[17] = pkt[15];
    tpkt[18] = pkt[16];
    tpkt[19] = 0;
    //Packet Word4
    tpkt[20] = pkt[17];
    tpkt[21] = pkt[18];
    tpkt[22] = pkt[19];
    tpkt[23] = pkt[20];
    //Packet Word5
    tpkt[24] = pkt[21];
    tpkt[25] = pkt[22];
    tpkt[26] = pkt[23];
    tpkt[27] = 0;
    //Packet Word6
    tpkt[28] = pkt[24];
    tpkt[29] = pkt[25];
    tpkt[30] = pkt[26];
    tpkt[31] = pkt[27];
    //Packet Word7
    tpkt[32] = pkt[28];
    tpkt[33] = pkt[29];
    tpkt[34] = pkt[30];
    tpkt[35] = 0;

    for(i=0;i<9;i++)        //for atm9009a change*******************
    {
        reg[i] = 0;
        for(j=0;j<4;j++)
        reg[i] |= (tpkt[i*4+j])<<(j*8);
    }
    
    owl_hdmi_write(HDMI_OPCR, (1<<8) | (addr&0xff));
    owl_hdmi_write(HDMI_ORP6PH, reg[0]);
    owl_hdmi_write(HDMI_ORSP6W0, reg[1]);
    owl_hdmi_write(HDMI_ORSP6W1, reg[2]);
    owl_hdmi_write(HDMI_ORSP6W2, reg[3]);
    owl_hdmi_write(HDMI_ORSP6W3, reg[4]);
    owl_hdmi_write(HDMI_ORSP6W4, reg[5]);
    owl_hdmi_write(HDMI_ORSP6W5, reg[6]);
    owl_hdmi_write(HDMI_ORSP6W6, reg[7]);
    owl_hdmi_write(HDMI_ORSP6W7, reg[8]);

    hdmi_EnableWriteRamPacket();

    return 0;
}

int hdmi_gen_spd_infoframe(void)
{
    static u8 pkt[32];
    unsigned int checksum=0;
    unsigned int i;
    static char spdname[8]="Vienna";
    static char spddesc[16]="DTV SetTop Box";

    // clear buffer 
    for(i=0;i<32;i++)
                pkt[i]=0;
        
    // header 
    pkt[0] = 0x80 | 0x03;   //HB0: Packet Type = 0x83
    pkt[1] = 1;             //HB1: version = 1
    pkt[2] = 0x1f & 25;     //HB2: len = 25 
    pkt[3] = 0x00;          //PB0: checksum = 0 

    // data
    // Vendor Name, 8 bytes 
    memcpy(&pkt[4], spdname, 8);
    // Product Description, 16 bytes
    memcpy(&pkt[12], spddesc, 16);
    // Source Device Information 
    pkt[28] = 0x1;          //Digital STB

    // count checksum
    for(i=0; i<31; i++)
        checksum += pkt[i];
    pkt[3] = (~checksum + 1)  & 0xff;

    // set to RAM Packet
    hdmi_SetRamPacket(HDMI_RAMPKT_SPD_SLOT, pkt);
    hdmi_SetRamPacketPeriod(HDMI_RAMPKT_SPD_SLOT, HDMI_RAMPKT_PERIOD);
    
    return 0;
}

/*
function:hdmi_gen_avi_infoframe
input:  colorformat :   0--RGB444;1--YCbCr422;2--YCbCr444
        AR          :   1--4:3;   2--16:9
return: 0
*/
int hdmi_gen_avi_infoframe(struct hdmi_ip_data *ip_data)
{
    static u8 pkt[32];
    u32 checksum=0, i=0;
    u8 AR=2;
    /* clear buffer */
    for(i=0;i<32;i++)
        pkt[i]=0;
    
    //1. header
    pkt[0] = 0x80 | 0x02;   //header = 0x82
    pkt[1] = 2;         //version = 2
    pkt[2] = 0x1f & 13; // len = 13
    pkt[3] = 0x00;      // checksum = 0 
    
    //2. data 
    //PB1--Y1:Y0=colorformat;R3:R1 is invalid ;no bar info and scan info
    pkt[4] = (ip_data->settings.pixel_encoding<<5) | (0<<4) | (0<<2) | (0); 

    //0--Normal YCC601 or YCC709; 1--xvYCC601; 2--xvYCC709
    if(ip_data->settings.color_xvycc == 0)
    {
        //PB2--Colorimetry:SMPTE 170M|ITU601; Picture aspect Ratio; same as picture aspect ratio  
        pkt[5] = (0x1<<6) | (AR<<4) | (0x8); 
        //PB3--No known non-uniform scaling
        pkt[6] = 0x0;
    }
    else if(ip_data->settings.color_xvycc == 1)
    {
        //PB2--Colorimetry:SMPTE 170M|ITU601; Picture aspect Ratio; same as picture aspect ratio  
        pkt[5] = (0x3<<6) | (AR<<4) | (0x8); 
        //PB3--xvYCC601;No known non-uniform scaling
        pkt[6] = 0x0;   
        
    }
    else
    {
        //PB2--Colorimetry:SMPTE 170M|ITU601; Picture aspect Ratio; same as picture aspect ratio  
        pkt[5] = (0x3<<6) | (AR<<4) | (0x8); 
        //PB3--xvYCC709;No known non-uniform scaling
        pkt[6] = 0x1<<4;    
    }

    //PB4--Video Id
    pkt[7] = ip_data->cfg.cm.code;
    //PB5--Pixel repeat time:
    pkt[8] = 0;
    //PB6--PB13: Bar Info, no bar info
    pkt[9] = 0;
    pkt[10] = 0;
    pkt[11] = 0;
    pkt[12] = 0;
    pkt[13] = 0;
    pkt[14] = 0;
    pkt[15] = 0;
    pkt[16] = 0;
    
    // count checksum
    for(i=0; i<31; i++)
        checksum += pkt[i];
    pkt[3] = (~checksum + 1) & 0xff;

    /* set to RAM Packet */
    hdmi_SetRamPacket(HDMI_RAMPKT_AVI_SLOT, pkt);
    hdmi_SetRamPacketPeriod(HDMI_RAMPKT_AVI_SLOT, HDMI_RAMPKT_PERIOD);

    return 0;
}

/*
function:hdmi_gen_gbd_infoframe
input:  Color_xvYCC :   0--Normal YCC601 or YCC709; 1--xvYCC601; 2--xvYCC709
        ColorDepth  :   0--24bit;   1--30bit;   2--36bit
return: 0
*/

int hdmi_gen_gbd_infoframe(struct hdmi_ip_data *ip_data)
{
#if 0   
    static u8 pkt[32];
    u32 checksum = 0;

    unsigned int deep_color = core->deep_color;
    unsigned int color_xvycc = core->color_xvycc;
    int i;

    /* clear buffer */
    for(i=0;i<32;i++)
        pkt[i]=0;
    
    //1. header
    pkt[0] = 0xa;       //header
    pkt[1] = (0x1<<7) | (0x0<<4) | (0x1);         //Next_Field = 1; GBD_Profile = P0; Affected Gamut seq num = 1;
    pkt[2] = (0x3<<4) | (0x1);                    //Only Packet in sequence; current Gamut seq num = 1;
    pkt[3] = 0x00;      // checksum = 0 
    
    //2. data 
    //PB1--Format Flag; GBD_Color_Precision; GBD_Color_Space
    pkt[4] = (0x1<<7) | (deep_color<<3) | (color_xvycc); 
    
    if (deep_color == 0)        //24bit
    {
        pkt[5] = 0x0;                   //min Red data
        pkt[6] = 0xfe;              //max Red data
        pkt[7] = 0x0;                   //min Green data
        pkt[8] = 0xfe;              //max Green data
        pkt[9] = 0x0;                   //min Blue data
        pkt[10] = 0xfe;             //max Blue data     
    }
    if (deep_color == 1)        //30bit
    {
        pkt[5] = 0x0;                   //min Red data: 0x0
        pkt[6] = 0x3f;              //max Red data: 0x3f8
        pkt[7] = 0x80;              //min Green data
        pkt[8] = 0x3;                   //max Green data
        pkt[9] = 0xf8;              //min Blue data
        pkt[10] = 0x0;              //max Blue data  
        pkt[11] = 0x3f;
        pkt[12] = 0x80;   
    }
    if (deep_color == 2)        //36bit
    {
        pkt[5] = 0x0;                   //min Red data: 0x0
        pkt[6] = 0xf;                 //max Red data: 0xfe0
        pkt[7] = 0xe0;              //min Green data
        pkt[8] = 0x0;                   //max Green data
        pkt[9] = 0xf;                 //min Blue data
        pkt[10] = 0xe0;             //max Blue data  
        pkt[11] = 0x0;
        pkt[12] = 0xf;
        pkt[13] = 0xe0;   
    }
        
    // count checksum
    for(i=0; i<31; i++)
        checksum += pkt[i];
    pkt[3] = (~checksum + 1) & 0xff;

    /* set to RAM Packet */
    
    hdmi_SetRamPacket(HDMI_RAMPKT_GBD_SLOT, pkt);
    hdmi_SetRamPacketPeriod(HDMI_RAMPKT_GBD_SLOT, HDMI_RAMPKT_PERIOD);
#endif
    return 0;
}

/*
* hdmi_gen_vs_infoframe(Vendor Specific)
* input:  3D format
* return: 0
*/

int hdmi_gen_vs_infoframe(struct hdmi_ip_data *ip_data)
{
    static u8 pkt[32];
    u32 checksum = 0;
    int i;
    
    /* clear buffer */
    for (i = 0; i < 32; i++)
        pkt[i] = 0;
    
    //1. header
    pkt[0] = 0x81;       //header
    pkt[1] = 0x1;        //Version

    if (ip_data->cfg.cm.code == 2) {

        pkt[2] = 0x6;        //length,  for Side-by-Side Half 3D

    } else {

        pkt[2] = 0x5;        //length,  for Frame 3D and Top-Bottom Half 3D

    }
    
    pkt[3] = 0x00;      // checksum = 0 
    
    //2. data 
    //PB1--PB3:24bit IEEE Registration Identifier
    pkt[4] = 0x03;
    pkt[5] = 0x0c;
    pkt[6] = 0x00; 
    
    //PB4: HDMI_Video_Format:000--no additional;001--extended resolution(4k*2k);010--3D format
    pkt[7] = 0x2<<5;       //3D format
    switch(ip_data->cfg.cm.code)
    {
     case   1:  pkt[8] = 0x0<<4;                //3D Frame
                pkt[9] = 0x0;
                break;
     case   2:  pkt[8] = 0x8<<4;                //Side-by-Side (Half)
                pkt[9] = 0x1<<4;
                break;
     case   3:  pkt[8] = 0x6<<4;                //Top-and-bottom (Half)
                pkt[9] = 0x0;
                break;
     default :  break;              
    }
        
    // count checksum
    for(i=0; i<31; i++)
        checksum += pkt[i];
    pkt[3] = (~checksum + 1) & 0xff;

    /* set to RAM Packet */
    
    hdmi_SetRamPacket(HDMI_RAMPKT_VS_SLOT, pkt);
    hdmi_SetRamPacketPeriod(HDMI_RAMPKT_VS_SLOT, HDMI_RAMPKT_PERIOD);

    return 0;
}

int  asoc_hdmi_gen_infoframe(struct hdmi_ip_data *ip_data)
{
    HDMI_DEBUG("asoc_hdmi_gen_infoframe\n");
    hdmi_gen_spd_infoframe();
    if (hdmi_gen_avi_infoframe(ip_data))
        return -EINVAL;
    

//  hdmi_gen_gbd_infoframe(ip_data);
//  hdmi_gen_vs_infoframe(ip_data);
    

    return 0;
}

