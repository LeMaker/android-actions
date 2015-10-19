#ifndef __TVOUT_HAL_PRIVATE_H__
#define __TVOUT_HAL_PRIVATE_H__
/******************************************************************
 此部分头文件与actsdk/psp/fwsp/include/asoc_ioctl.h保持一致,
 cvbs设备结点："/dev/cvbs",ypbpr设备结点："/dev/ypbpr"
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

#define TVOUT_IOC_MAGIC               't'
/*功能：设置CVBS制式。参数类型:struct tv_settings*/
#define CVBS_CONFIGURE                  _IOW(TVOUT_IOC_MAGIC, 0x96, struct tv_settings)             
/*功能：获取CVBS插拔线状态，1：拔出，2：插入。参数类型：u32  */
#define CVBS_GET_STATUS                 _IOR(TVOUT_IOC_MAGIC, 0x97, u32) 
/*功能：获取CVBS默认制式。参数类型：U32*/
#define CVBS_GET_CURRENT_MODE             
/*功能：设置CVBS使能/禁用,参数为1时使能，否则禁用。参数类型：u32  */  
#define CVBS_SET_ENABLE                 _IOW(TVOUT_IOC_MAGIC, 0x98, u32)              

enum TV_MODE_TYPE {
	/* following three modes are used by CVBS OUTPUT*/
	TV_MODE_PAL = 0,
	TV_MODE_NTSC = 1,
	TV_MODE_MAX = 2
};

enum TV_MODE {
	TVOUT_CVBS = 1,
	TVOUT_YPBPR,
	TVOUT_MAX
};

enum TV_ENCODER_TYPE {
	TV_ENCODER_INTERNAL,
	TV_ENCODER_EXTERNAL,
	TV_ENCODER_MAX
};

enum TVOUT_DEV_STATUS {
	TVOUT_DEV_PLUGOUT = 1,
	TVOUT_DEV_PLUGIN,
	TVOUT_DEV_MAX
};

struct tv_settings {
	u32 tv_mode;		/* enum TV_MODE_TYPE */
	u32 encoder_type;	/* enum TV_ENCODER_TYPE */
};
#endif /*#ifndef __TVOUT_HAL_PRIVATE_H__*/