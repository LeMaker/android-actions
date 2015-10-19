#ifndef __HDMI_TX_DRV_H__
#define __HDMI_TX_DRV_H__
/******************************************************************
 this head file must keep the same with actsdk/psp/fwsp/include/asoc_ioctl.h,
 the node path £º"/dev/hdmi"
*******************************************************************/

#define u32 unsigned int 

#ifndef _IOC_WRITE
# define _IOC_WRITE	1U
#endif

#ifndef _IOC_READ
# define _IOC_READ	2U
#endif


#define _IOC(dir,type,nr,size) \
	(((dir)  << _IOC_DIRSHIFT) | \
	 ((type) << _IOC_TYPESHIFT) | \
	 ((nr)   << _IOC_NRSHIFT) | \
	 ((size) << _IOC_SIZESHIFT))


/**------------------------------------------------------------------------------*/
#define HDMI_TX_IOC_MAGIC               'h'
/*-----------------------------------------------------------------------------*/
/**
* @get the link status of device. 
* @parameter type : u32. pulg in : HDMI_PLUGIN, plug out : HDMI_PLUGOUT.
* @e.g:u32 status;
*      ioctl(fd, HDMI_GET_LINK_STATUS, &status);
*/
#define HDMI_GET_LINK_STATUS            _IOR(HDMI_TX_IOC_MAGIC, 0x90, u32)

/**
* @get the capability of device.please refer to the coment of struct sink_capabilities_t
* @parameter type:struct sink_capabilities_t.
* @e.g:struct  sink_capabilities_t  sink_capabilities;
*      ioctl(fd, HDMI_GET_SINK_CAPABILITY, &sink_capabilities);
*/
#define HDMI_GET_SINK_CAPABILITY        _IOR(HDMI_TX_IOC_MAGIC, 0x91, struct sink_capabilities_t)

/**
* @set video parameters.includes:resolution,frame rate,ar,interlace/progressive
* @parameter type : u32
* @e.g:u32 vid = VID1920x1080P_50_16VS9;
*      ioctl(fd, HDMI_SET_VIDEO_CONFIG, &vid);
*/
#define HDMI_SET_VIDEO_CONFIG           _IOW(HDMI_TX_IOC_MAGIC, 0x92, u32)

/**
* @set hdmi enable/disable.
* @parameter type:u32.  enable : 1, disable : 0
* @e.g:u32 enable = 1;
*      ioctl(fd, HDMI_SET_ENABLE, &enable);
*/
#define HDMI_SET_ENABLE                 _IOW(HDMI_TX_IOC_MAGIC, 0x98, u32)

/**
* @set 3d format.
* @parameter type : u32. 3d format : _3D, don't support 3d format : _3D_NOT
* @e.g:u32 _3d = _3D_NOT;
*      ioctl(fd, HDMI_SET_VIDEO_3D, &_3d);
*/
#define HDMI_SET_VIDEO_3D	_IOW(HDMI_TX_IOC_MAGIC, 0xA6, u32)

/**
* @set deep color mode.
* @parameter type : u32. DEEP_COLOR_24_BIT:24bit,DEEP_COLOR_30_BIT:30bit,DEEP_COLOR_36_BIT:36bit
@e.g:u32 deep_color = DEEP_COLOR_24_BIT;
*      ioctl(fd, HDMI_SET_DEEP_COLOR, &deep_color);
*/
#define HDMI_SET_DEEP_COLOR				_IOW(HDMI_TX_IOC_MAGIC, 0xA7, u32)

/**
* @set pixel encoding format.
* @parameter type : u32.:rgb mode : VIDEO_PEXEL_ENCODING_RGB,ycbcr 444 : VIDEO_PEXEL_ENCODING_YCbCr444 
* @e.g:u32 pixel_encoding = VIDEO_PEXEL_ENCODING_YCbCr444;
*      ioctl(fd, HDMI_SET_DEEP_COLOR, &pixel_encoding);
*/
#define HDMI_SET_PIXEL_ENCODING			_IOW(HDMI_TX_IOC_MAGIC, 0xA8, u32)

/**
* @set color coding format.
* @parameter type : u32. Normal YCC601 or YCC709 : YCC601_YCC709£¬xvYCC601 : XVYCC601, xvYCC709: XVYCC709
* @e.g:u32 color_xvycc = YCC601_YCC709;
*      ioctl(fd, HDMI_SET_COLOR_XVYCC, &color_xvycc);
*/
#define HDMI_SET_COLOR_XVYCC			_IOW(HDMI_TX_IOC_MAGIC, 0xA9, u32)

/**
* @set hdmi mode.
* @parameter type : u32. hdmi mode : HDMI_MODE_HDMI£¬dvi mode : HDMI_MODE_DVI
* @e.g:u32 hdmi_mode = HDMI_MODE_HDMI;
*      ioctl(fd, HDMI_SET_HDMI_MODE, &hdmi_mode);
*/
#define HDMI_SET_HDMI_MODE				_IOW(HDMI_TX_IOC_MAGIC, 0xAA, u32)

/**
* @set hdmi signal source.
* @parameter type : u32. video interface test display : VITD£¬the signal from de : DE
* @e.g:u32 hdmi_src = DE;
*      ioctl(fd, HDMI_SET_HDMI_SRC, &hdmi_src);
*/
#define HDMI_SET_HDMI_SRC				_IOW(HDMI_TX_IOC_MAGIC, 0xAB, u32)

enum VIDEO_ID_TABLE {	
	VID640x480P_60_4VS3 = 1,	
	VID720x480P_60_4VS3,	
	VID720x480P_60_16VS9,	
	VID1280x720P_60_16VS9,	
	VID1920x1080I_60_16VS9,	
	VID720x480I_60_4VS3,	
	VID720x480I_60_16VS9,	
	VID1440x480P_60_4VS3 = 14,	
	VID1440x480P_60_16VS9,	
	VID1920x1080P_60_16VS9,	
	VID720x576P_50_4VS3,	
	VID720x576P_50_16VS9,	
	VID1280x720P_50_16VS9,	
	VID1920x1080I_50_16VS9,	
	VID720x576I_50_4VS3,	
	VID720x576I_50_16VS9,	
	VID1440x576P_50_4VS3 = 29,	
	VID1440x576P_50_16VS9,	
	VID1920x1080P_50_16VS9,	
	VID1920x1080P_24_16VS9,	
	VID1920x1080P_25_16VS9,	
	VID1920x1080P_30_16VS9,	
	VID720x480P_59P94_4VS3 = 72,	
	VID720x480P_59P94_16VS9,	
	VID1280X720P_59P94_16VS9,	
	VID1920x1080I_59P94_16VS9,	
	VID720x480I_59P54_4VS3,	
	VID720x480I_59P54_16VS9,	
	VID1920x1080P_59P94_16VS9 = 86,	
	VID1920x1080P_29P97_16VS9 = 104,	
	VID_MAX
};

enum HDMI_MODE {
	HDMI_MODE_HDMI = 0,
	HDMI_MODE_DVI,
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

enum _3D_FORMAT{
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
#endif /*#ifndef __GL5201_TVOUT__H__*/