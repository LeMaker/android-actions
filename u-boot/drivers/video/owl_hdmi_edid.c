/*
 * hdmi_edid.c
 *
 * HDMI OWL IP driver Library
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: Guo Long  <guolong@actions-semi.com>
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
 
 #include <libfdt_env.h>
#include <fdtdec.h>
#include <fdt.h>
#include <libfdt.h>

#include <asm/arch/actions_reg_owl.h>
#include <asm/io.h>
#include <asm/gpio.h>

#include <common.h>
#include <malloc.h>
#include <i2c.h>
//#include "owl_hdmi.h"

#ifdef CONFIG_CMD_EXT4
#include <ext4fs.h>
#include <mmc.h>
#endif

//#undef debug
//#define debug printf

static int ibus;

#define HDMI_DDC_ADDR		(0x60 >> 1)
#define DDC_EDID_ADDR		(0xa0 >> 1)
 
enum VIDEO_ID_TABLE {
	VID640x480P_60_4VS3 = 1,
	VID720x480P_60_4VS3,
	VID1280x720P_60_16VS9 = 4,
	VID1920x1080I_60_16VS9,
	VID720x480I_60_4VS3,
	VID1920x1080P_60_16VS9 = 16,
	VID720x576P_50_4VS3,
	VID1280x720P_50_16VS9 = 19,
	VID1920x1080I_50_16VS9,
	VID720x576I_50_4VS3,
	VID1440x576P_50_4VS3 = 29,
	VID1920x1080P_50_16VS9 = 31,
	VID1920x1080P_24_16VS9,
	VID1280x720P_60_DVI = 126,
	VID_MAX
}; 

enum hdmi_core_hdmi_dvi {
	HDMI_DVI = 0,
	HDMI_HDMI = 1
};

enum tv_mode {
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

typedef struct
{
	int mode;
	int vid;
} modes;

/* The list of HDMI modes in preference order */
static const modes hdmi_mode[] =
{
	{OWL_TV_MOD_1080P_60HZ, VID1920x1080P_60_16VS9},
	{OWL_TV_MOD_1080P_50HZ, VID1920x1080P_50_16VS9},
	{OWL_TV_MOD_720P_60HZ, VID1280x720P_60_16VS9},
	{OWL_TV_MOD_720P_50HZ, VID1280x720P_50_16VS9},
	{OWL_TV_MOD_576P, VID720x576P_50_4VS3},
	{OWL_TV_MOD_480P, VID720x480P_60_4VS3},
	{OWL_TV_MOD_DVI,  VID1280x720P_60_DVI},
};

#define MODE_COUNT (sizeof(hdmi_mode)/sizeof(hdmi_mode[0]))

struct hdmi_edid{

	unsigned char 		EDID_Buf[1024];
	unsigned char 		Device_Support_VIC[512];
	unsigned char      isHDMI;
	unsigned char      YCbCr444_Support;
	int     video_formats[4];
	int    read_ok;
};

struct hdmi_edid edid;


#define VID1920x1080P_24_16VS9_3D_FP  (VID1920x1080P_24_16VS9 +0x80)
#define VID1280x720P_50_16VS9_3D_FP   (VID1280x720P_50_16VS9  +0x80)
#define VID1280x720P_60_16VS9_3D_FP   (VID1280x720P_60_16VS9  +0x80)

static int ddc_read(char segment_index, char segment_offset, char * pbuf)
{
	int ret;
	int retry_num = 0;

	I2C_SET_BUS(ibus);
	
retry:
	if (retry_num++ >= 3) {
		debug("%s, read error after %dth retry\n",
		      __func__, retry_num - 1);
		return -1;
	}

	debug("%s, retry = %d\n", __func__, retry_num);

	/* set segment index */
	ret = i2c_write(HDMI_DDC_ADDR, segment_index, 1, NULL, 0);
	/*
	 * skip return value checking, because this command has no ACK,
	 * but u-boot i2c framework will return ERROR
	 */

	/* read data */
	ret = i2c_read(DDC_EDID_ADDR, segment_offset, 1, pbuf, 128);
	if (ret < 0) {
		debug("%s, fail to read EDID data(%d)\n", __func__, ret);
		goto retry;
	}
	debug("%s, finished\n", __func__);

	return ret;
}

static int get_edid_data(unsigned char block,unsigned char *buf)
{
	unsigned char i;
    unsigned char * pbuf = buf + 128*block;
    unsigned char offset = (block&0x01)? 128:0;
  
	if(ddc_read(block>>1,offset,pbuf)<0)
	{
		printf("read edid error!!!\n");
		return -1;
	}
//	edid_test(offset, pbuf);
	////////////////////////////////////////////////////////////////////////////
    debug("Sink : EDID bank %d:\n",block);

	debug(" 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F\n");
	debug(" ===============================================================================================\n");

	for (i = 0; i < 8; i++) 
	{
		debug(" %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x\n",
				pbuf[i*16 + 0 ],pbuf[i*16 + 1 ],pbuf[i*16 + 2 ],pbuf[i*16 + 3 ],
				pbuf[i*16 + 4 ],pbuf[i*16 + 5 ],pbuf[i*16 + 6 ],pbuf[i*16 + 7 ],
				pbuf[i*16 + 8 ],pbuf[i*16 + 9 ],pbuf[i*16 + 10],pbuf[i*16 + 11],
				pbuf[i*16 + 12],pbuf[i*16 + 13],pbuf[i*16 + 14],pbuf[i*16 + 15]
				);
	}
    debug(" ===============================================================================================\n");

    return 0;
	
}

/////////////////////////////////////////////////////////////////////
// parse_edid()
// Check EDID check sum and EDID 1.3 extended segment.
/////////////////////////////////////////////////////////////////////
int edid_checksum(unsigned char block,unsigned char *buf)
{
    int i = 0, CheckSum = 0;
	unsigned char *pbuf = buf + 128*block;
	
    for( i = 0, CheckSum = 0 ; i < 128 ; i++ )
	{
        CheckSum += pbuf[i] ; 
        CheckSum &= 0xFF ;
    }

	if( CheckSum != 0 )
	{
		printf("EDID block %d checksum error\n",block);
		return -1 ;
	}
	return 0;
}

int edid_header_check(unsigned char *pbuf)
{
	if( pbuf[0] != 0x00 ||
	    pbuf[1] != 0xFF ||
	    pbuf[2] != 0xFF ||
	    pbuf[3] != 0xFF ||
	    pbuf[4] != 0xFF ||
	    pbuf[5] != 0xFF ||
	    pbuf[6] != 0xFF ||
	    pbuf[7] != 0x00)
	{
    	printf("EDID block0 header error\n");
        return -1 ;
    }
	return 0;
}

int edid_version_check(unsigned char *pbuf)
{
    debug("EDID version: %d.%d ",pbuf[0x12],pbuf[0x13]) ;
    if( (pbuf[0x12]!= 0x01) || (pbuf[0x13]!=0x03))
	{
		printf("Unsupport EDID format,EDID parsing exit\n");
		return -1;
    }
	return 0;
}

int parse_dtd_block(struct hdmi_edid *edid, unsigned char *pbuf)
{
	int 	pclk,sizex,Hblanking,sizey,Vblanking,Hsync_offset,Hsync_plus,
			Vsync_offset,Vsync_plus,H_image_size,V_image_size,H_Border,
			V_Border,pixels_total,frame_rate;
    pclk 		= ( (int)pbuf[1]	<< 8) + pbuf[0];
    sizex 		= (((int)pbuf[4] 	<< 4) & 0x0f00) + pbuf[2];
    Hblanking 	= (((int)pbuf[4] 	<< 8) & 0x0f00) + pbuf[3];
    sizey 		= (((int)pbuf[7] 	<< 4) & 0x0f00) + pbuf[5];
    Vblanking 	= (((int)pbuf[7] 	<< 8) & 0x0f00) + pbuf[6];
    Hsync_offset= (((int)pbuf[11] << 2) & 0x0300) + pbuf[8];
    Hsync_plus 	= (((int)pbuf[11] << 4) & 0x0300) + pbuf[9];
    Vsync_offset= (((int)pbuf[11] << 2) & 0x0030) + (pbuf[10] >> 4);
    Vsync_plus 	= (((int)pbuf[11] << 4) & 0x0030) + (pbuf[8] & 0x0f);
    H_image_size= (((int)pbuf[14] << 4) & 0x0f00) + pbuf[12];
    V_image_size= (((int)pbuf[14] << 8) & 0x0f00) + pbuf[13];
    H_Border 	=  pbuf[15];
	V_Border 	=  pbuf[16];

	pixels_total = (sizex + Hblanking) * (sizey + Vblanking);

	if( (pbuf[0] == 0) && (pbuf[1] == 0) && (pbuf[2] == 0))
	{
		return 0;
	}
	
	if(pixels_total == 0){
		return 0;
	}
	else
	{
		frame_rate = (pclk * 10000) /pixels_total;
	}

    if ((frame_rate == 59) || (frame_rate == 60))
	{
        if ((sizex== 720) && (sizey == 240))
        {
        	edid->Device_Support_VIC[VID720x480I_60_4VS3] = 1;
        }
        if ((sizex== 720) && (sizey == 480))
        {
        	edid->Device_Support_VIC[VID720x480P_60_4VS3] = 1;
        }
        if ((sizex== 1280) && (sizey == 720))
        {
            edid->Device_Support_VIC[VID1280x720P_60_16VS9] = 1;
        }
        if ((sizex== 1920) && (sizey == 540))
        {
            edid->Device_Support_VIC[VID1920x1080I_60_16VS9] = 1;
        }
        if ((sizex== 1920) && (sizey == 1080))
        {
            edid->Device_Support_VIC[VID1920x1080P_60_16VS9] = 1;
        }
    }
	else if ((frame_rate == 49) || (frame_rate == 50))
	{
        if ((sizex== 720) && (sizey == 288))
        {
        	edid->Device_Support_VIC[VID720x576I_50_4VS3] = 1;
        }
        if ((sizex== 720) && (sizey == 576))
        {
        	edid->Device_Support_VIC[VID720x576P_50_4VS3] = 1;
        }
        if ((sizex== 1280) && (sizey == 720))
        {
            edid->Device_Support_VIC[VID1280x720P_50_16VS9] = 1;
        }          
        if ((sizex== 1920) && (sizey == 540))
        {
            edid->Device_Support_VIC[VID1920x1080I_50_16VS9] = 1;
        }
        if ((sizex== 1920) && (sizey == 1080))
        {
            edid->Device_Support_VIC[VID1920x1080P_50_16VS9] = 1;
        }
    }
	else if ((frame_rate == 23) || (frame_rate == 24))
	{
        if ((sizex== 1920) && (sizey == 1080))
        {
            edid->Device_Support_VIC[VID1920x1080P_24_16VS9] = 1;
        }
    }
	debug("PCLK=%d\tXsize=%d\tYsize=%d\tFrame_rate=%d\n",
		  pclk*10000,sizex,sizey,frame_rate);
		  
    return 0;
}

int parse_videodata_block(struct hdmi_edid *edid, unsigned char *pbuf,unsigned char size)
{
	int i=0;
	while(i<size)
	{
		edid->Device_Support_VIC[pbuf[i] &0x7f] = 1;
		if(pbuf[i] &0x80)
		{
		   debug("parse_videodata_block: VIC %d(native) support\n", pbuf[i]&0x7f);
		}
		else
		{
		   debug("parse_videodata_block: VIC %d support\n", pbuf[i]);
		}
		i++;
	}
	
	////////////////////////////////////////////////////////////////////////////
    debug("parse_videodata_block : Device_Support_VIC :\n");

	debug(" 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F\n");
	debug(" ===============================================================================================\n");

	for (i = 0; i < 8; i++) 
	{
		debug(" %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x  %2.2x\n",
				edid->Device_Support_VIC[i*16 + 0 ],edid->Device_Support_VIC[i*16 + 1 ],edid->Device_Support_VIC[i*16 + 2 ],edid->Device_Support_VIC[i*16 + 3 ],
				edid->Device_Support_VIC[i*16 + 4 ],edid->Device_Support_VIC[i*16 + 5 ],edid->Device_Support_VIC[i*16 + 6 ],pbuf[i*16 + 7 ],
				edid->Device_Support_VIC[i*16 + 8 ],edid->Device_Support_VIC[i*16 + 9 ],edid->Device_Support_VIC[i*16 + 10],pbuf[i*16 + 11],
				edid->Device_Support_VIC[i*16 + 12],edid->Device_Support_VIC[i*16 + 13],edid->Device_Support_VIC[i*16 + 14],pbuf[i*16 + 15]
				);
	}
    debug(" ===============================================================================================\n");
   
	return 0;
}

int parse_audiodata_block(unsigned char *pbuf,unsigned char size)
{
	unsigned char sum = 0;
	
	while(sum < size)
	{
    	if( (pbuf[sum]&0xf8) == 0x08)
    	{
			debug("parse_audiodata_block: max channel=%d\n",(pbuf[sum]&0x7)+1);
			debug("parse_audiodata_block: SampleRate code=%x\n",pbuf[sum+1]);
			debug("parse_audiodata_block: WordLen code=%x\n",pbuf[sum+2]);
    	}
    	sum += 3;
	}
	return 0;
}

int parse_hdmi_vsdb(struct hdmi_edid *edid, unsigned char * pbuf,unsigned char size)
{
	unsigned char index = 8;

	if( (pbuf[0] ==0x03) &&	(pbuf[1] ==0x0c) &&	(pbuf[2] ==0x00) )	//check if it's HDMI VSDB
	{
		edid->isHDMI = HDMI_HDMI;
		debug("Find HDMI Vendor Specific DataBlock\n");
	}
	else
	{
		edid->isHDMI = HDMI_DVI;
		return 0;
	}
	
	if(size <=8)
		return 0;

	if((pbuf[7]&0x20) == 0 )
		return 0;
	if((pbuf[7]&0x40) == 1 )
		index = index +2;
	if((pbuf[7]&0x80) == 1 )
		index = index +2;

	if(pbuf[index]&0x80)		//mandatary format support
	{
		edid->Device_Support_VIC[VID1920x1080P_24_16VS9_3D_FP] = 1;
		edid->Device_Support_VIC[VID1280x720P_50_16VS9_3D_FP] = 1;
		edid->Device_Support_VIC[VID1280x720P_60_16VS9_3D_FP] = 1;
		debug("3D_present\n");
	}
	else
	{
		return 0;
	}
	
	if( ((pbuf[index]&0x60) ==1) || ((pbuf[index]&0x60) ==2) )
	{
		debug("3D_multi_present\n");
	}
	
	index += (pbuf[index+1]&0xe0) + 2;
	if(index > (size+1) )
	   	return 0;
	   	
	debug("3D_multi_present byte(%2.2x,%2.2x)\n",pbuf[index],pbuf[index+1]);

	return 0;
}

int parse_edid(struct hdmi_edid *edid)
{
    //collect the EDID ucdata of segment 0
    unsigned char BlockCount ;
    int i,offset ;

    debug("parse_edid\n");

    memset(edid->Device_Support_VIC,0,sizeof(edid->Device_Support_VIC));
    memset(edid->EDID_Buf,0,sizeof(edid->EDID_Buf));
	memset(edid->video_formats,0,sizeof(edid->video_formats));
	
    edid->isHDMI = HDMI_HDMI;
    edid->YCbCr444_Support = 0;
	edid->read_ok = 0;

    if( get_edid_data(0, edid->EDID_Buf) != 0)
	{
		printf("get_edid_data error!!!\n");
		goto err0;
	}

	if( edid_checksum(0, edid->EDID_Buf) != 0)
	{
		printf("edid_checksum error!!!\n");
		goto err0;
	}

	if( edid_header_check(edid->EDID_Buf)!= 0)
	{
		printf("edid_header_check error!!!\n");
		goto err0;
	}

	if( edid_version_check(edid->EDID_Buf)!= 0)
	{
		printf("edid_version_check error!!!\n");
		goto err0;
	}
	
	parse_dtd_block(edid, edid->EDID_Buf + 0x36);	

	parse_dtd_block(edid, edid->EDID_Buf + 0x48);

    BlockCount = edid->EDID_Buf[0x7E];

    if( BlockCount > 0 )
    {
	    if ( BlockCount > 4 )
	    {
	        BlockCount = 4 ;
	    }
	    for( i = 1 ; i <= BlockCount ; i++ )
	    {
	        get_edid_data(i, edid->EDID_Buf) ;  
	        if( edid_checksum(i, edid->EDID_Buf)!= 0)
	        {
	        	return 0;
	        }

			if((edid->EDID_Buf[0x80*i+0]==2)/*&&(edid->EDID_Buf[0x80*i+1]==1)*/)
			{
				//add by matthew 20120809 to add rgb/yuv detect
				if( (edid->EDID_Buf[0x80*i+1]>=1))
				{
						if(edid->EDID_Buf[0x80*i+3]&0x20)
						{
							edid->YCbCr444_Support = 1;
							debug("device support YCbCr44 output\n");
						}
				}
				//end by matthew 20120809
				
				offset = edid->EDID_Buf[0x80*i+2];
				if(offset > 4)		//deal with reserved data block
				{
					unsigned char bsum = 4;
					while(bsum < offset)
					{
						unsigned char tag = edid->EDID_Buf[0x80*i+bsum]>>5;
						unsigned char len = edid->EDID_Buf[0x80*i+bsum]&0x1f;
						if( (len >0) && ((bsum + len + 1) > offset) )
						{
						    debug("len or bsum size error\n");
							return 0;
						}else
						{
							if( tag == 1)		//ADB
							{
								parse_audiodata_block(edid->EDID_Buf+0x80*i+bsum+1,len);
							}
							else if( tag == 2)	//VDB
							{
								parse_videodata_block(edid, edid->EDID_Buf+0x80*i+bsum+1,len);
							}
							else if( tag == 3)	//vendor specific 
							{
								parse_hdmi_vsdb(edid, edid->EDID_Buf+0x80*i+bsum+1,len);
							}
						}

						bsum += (len +1);
					}
					
				}else
				{
					debug("no data in reserved block%d\n",i);
				}
				
				if(offset >= 4)		//deal with 18-byte timing block
				{
					if(offset == 4)
					{
						edid->isHDMI = HDMI_DVI;
						debug("dvi mode\n");
					}				
					while(offset < (0x80-18))
					{
						parse_dtd_block(edid, edid->EDID_Buf + 0x80*i + offset);	
						offset += 18;
					}
					debug("deal with 18-byte timing block\n");

				}else
				{
					debug("no datail timing in block%d\n",i);
				}
			}

	    }
    }
	
	for(i=0;i<128;i++)
	{
		if(edid->Device_Support_VIC[i]==1)
		{
			edid->video_formats[i/32] |= (1<<(i%32));
		}
	}
	edid->video_formats[0] |= 0x04;
	edid->read_ok = 1;
	debug("edid->video_formats[0] = 0x%x\n", edid->video_formats[0]);
	debug("edid->video_formats[1] = 0x%x\n", edid->video_formats[1]);	
	debug("edid->video_formats[2] = 0x%x\n", edid->video_formats[2]);	
	debug("edid->video_formats[3] = 0x%x\n", edid->video_formats[3]);	
    return 0 ;

err0:
	edid->video_formats[0] = 0;
	edid->video_formats[1] = 0;
	edid->video_formats[2] = 0;
	edid->video_formats[3] = 0;
	printf("read edid err0\n");
	return -1 ;
}

int check_hdmi_mode(int mode,int i2cbus)
{
	int i=0;
	i = MODE_COUNT;
	ibus=i2cbus;
	
	
	parse_edid(&edid);		
	
	if(mode > 0){
		for(i=0;i<MODE_COUNT;i++){
			if(edid.video_formats[0] & (1 << mode)){
				return mode;
			}
		}
	}
	for(i=0;i<MODE_COUNT;i++){
		if(edid.video_formats[0]&(1<<hdmi_mode[i].vid)){
			printf("find support mode %d \n", hdmi_mode[i].mode);
			return hdmi_mode[i].mode;
		}
	}
	
	return OWL_TV_MOD_720P_60HZ;
}
