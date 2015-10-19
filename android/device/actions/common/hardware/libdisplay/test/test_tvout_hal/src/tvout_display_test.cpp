#include <utils/Log.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>


#include <dlfcn.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <cutils/properties.h>

#include "DisplayParameters.h"
#include <jni.h>
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include "de.h"
#include "hdmi.h"
#include "cvbs.h"

namespace android {

#define MAX_DISPLAYER_SUPPORT  10

// ----------------------------------------------------------------------------
#define DISPLAY_NAME_MAX_LEN  256
#define DEFAULT_ALPHA 240
#define DEFAULT_COLOR_KEY  0

#define FORMAT_NTSC  "ntsc"
#define FORMAT_PAL    "pal"

/** 
   *cvbs has two type: PAL , NTSC
   */

#define TYPE_LCD      1
#define TYPE_CVBS   2
#define TYPE_HDMI   8

//the following definition should keep same with DisplayService.java

#define  CABLE_HDMI_ON   0x1	
#define  CABLE_CVBS_ON     0x2

#define  CABLE_CVBS_PLUG_IN         0x1
#define  CABLE_CVBS_PLUG_OUT       0x2
#define  CABLE_HDMI_PLUG_IN        0x100
#define  CABLE_HDMI_PLUG_OUT      0x200

#define DEIO_GET_VIDEO_CHANGED 0x5712


struct lcd_displayer_device_t {


};

struct hdmi_displayer_device_t {
	struct hdmi_device_t *hdmi_device;
	struct sink_capabilities_t sink_caps;
	
};

struct  cvbs_displayer_device_t {
	struct cvbs_device_t *cvbs_device;
	/*ntsc/pal*/
	char format[64];
};


//define Displayer Data Structure
struct displayer_t{
	/*displayer info */
	int width;
	int height;
	int dpi ;
	/* type of displayer: cvbs or hdmi*/
	int type;
	int id;
	char name[DISPLAY_NAME_MAX_LEN];
	void *opaque;
	
};

/*****  control variable in this module ******************/
static displayer_t displayers[MAX_DISPLAYER_SUPPORT];
static int displayer_count=0;
static int current_display=-1;
static int initialized=0;

/*********  parameter used by setParam&getParam   **********/
char display_info_buf[256];
char display_params_buf[512];
char sink_cap_buf[512];

static int alpha=DEFAULT_ALPHA;
static int colorkey=DEFAULT_COLOR_KEY;
static char colorkey_enable[64];
static char format[64];
static char video_scale[128];
static int hdmi_res_width;
static int hdmi_res_height;
static int hdmi_res_hz;
static int scale_x=0;
static int scale_y=0;

/*********   all displayer we support  ********************/
static struct de_control_device_t *dedevice = NULL;
static cvbs_displayer_device_t cvbs_displayer;
static hdmi_displayer_device_t hdmi_displayer;
static lcd_displayer_device_t    lcd_displayer;

/********    internal function ****************************/
static int cvbs_open(struct cvbs_displayer_device_t * cvbs_displayer);
static int cvbs_enable(struct cvbs_displayer_device_t *cvbs_displayer, bool enable);
static int cvbs_close(struct cvbs_displayer_device_t *cvbs_displayer);
static bool cvbs_cable_connected(struct cvbs_displayer_device_t *cvbs_displayer);
static int hdmi_open(struct hdmi_displayer_device_t *hdmi_displayer);
static int hdmi_enable(struct hdmi_displayer_device_t *hdmi_displayer, bool enable);
static int hdmi_close(struct hdmi_displayer_device_t *hdmi_displayer);
static bool hdmi_cable_connected(struct hdmi_displayer_device_t *hdmi_displayer);
static int get_real_id(int dp_id);
static int mapDisplayIdFromHalToDrvier(int displayId);
static void update_virtual_tv_config(void);
static void update_virtual_tv_scale_info(int xscale, int yscale);


static int check_de_opened(int fd, int *pstatus){
	return ioctl(fd, DEIO_GET_VIDEO_CHANGED, pstatus);
}

void actions_server_DisplayService_cable_monitor(void) {
	
	int fd_de=-1;
	int ret_val = -1;
	int fd_max;
	fd_set fds;
	int val = -1;
	int last_cable_state_hdmi=0;
	int last_cable_state_cvbs=0;
	int cable_state_hdmi=0;
	int cable_state_cvbs=0;
	int cable_status;
	int de_opened;
	
	fd_de = open(DE_HARDWARE_DEVICE,O_RDWR);
   	if (fd_de < 0) {
        	ALOGW("Monitor: open fd_de device error! (%d %s)", errno, strerror(errno));
  		return;
    	}
	fd_max=fd_de;
	if(dedevice->de_get_tv_cable_status(dedevice, &cable_status)==0){
		if(cable_status & (1<<HAL_HDMI_CABLE_STATUS_BIT)){
				last_cable_state_hdmi=1;
		} else {
				last_cable_state_hdmi=0;
		}
		
		if(cable_status&(1<<HAL_CVBS_CABLE_STATUS_BIT)){
				last_cable_state_cvbs=1;
		}else{
				last_cable_state_cvbs=0;
		}	
	}
		
   	while (1) {

		 FD_ZERO(&fds);
	        FD_SET(fd_de, &fds);
	        ret_val = select(fd_max + 1, &fds, NULL, NULL, NULL);
	        if(ret_val <0) {
	            ALOGW("monitor: select error!\n");
		    continue;
	        } 
		 ALOGD("select activiate");
		 if(FD_ISSET(fd_de, &fds)){
			de_opened=-1;
			/*ret_val=check_de_opened(fd_de, &de_opened);
			if(ret_val==0){
				//de的2x缩放功能打开/关闭了
				notify_eventhub_change(de_opened);
				continue;
			}*/
			cable_status=0;
			if(!dedevice||dedevice->de_get_tv_cable_status(dedevice, &cable_status)){
				ALOGW("de_get_tv_cable_status: failed err=%s \n", strerror(errno));
				continue;
			}
			ALOGD("cable_status=%d \n", cable_status);
			
			// hdmi cable status
			if(cable_status&(1<<HAL_HDMI_CABLE_STATUS_BIT)){
				cable_state_hdmi=1;
			}else{
				cable_state_hdmi=0;
			}
			
			if(cable_state_hdmi>last_cable_state_hdmi){
				ALOGW("hdmi cable plug in %d\n", CABLE_HDMI_PLUG_IN);
			}else if(cable_state_hdmi<last_cable_state_hdmi){
				ALOGW("hdmi cable plug out %d\n", CABLE_HDMI_PLUG_OUT);
			}
			
			last_cable_state_hdmi=cable_state_hdmi;

			//cvbs cable status
			if(cable_status&(1<<HAL_CVBS_CABLE_STATUS_BIT)){
				cable_state_cvbs=1;
			}else{
				cable_state_cvbs=0;
			}
			if(cable_state_cvbs>last_cable_state_cvbs){
				ALOGW("cvbs cable plug in %d\n", CABLE_HDMI_PLUG_IN);
			}else if(cable_state_cvbs<last_cable_state_cvbs){
				ALOGW("cvbs cable plug out %d\n", CABLE_HDMI_PLUG_OUT);
			}
			last_cable_state_cvbs=cable_state_cvbs;
	
		}
		
    }
}
	
	

/** helper APIs */
static inline int de_control_open(const struct hw_module_t* module,
        struct de_control_device_t** device) {
    return module->methods->open(module,
            DE_HARDWARE_MODULE_ID, (struct hw_device_t**)device);
}


int actions_server_DisplayService_init(void)
{  
	de_module_t* module;
	displayer_t *dpy;
	int if_type;
	
	if(initialized){
		return true;
	}
	
	ALOGD("enter actions_server_DisplayService_init\n");
	
	displayer_count=0;

	if (hw_get_module(DE_HARDWARE_MODULE_ID, (const hw_module_t**)&module) == 0) {
		ALOGI(": display engine found.");
		if (de_control_open(&module->common, &dedevice) == 0) {
			ALOGI(": got display engine operations.dedevice:%x", (unsigned int)dedevice);
		}else{
			dedevice=0;		
			return false;
		}
	} else {
          dedevice=0; 
	   return false;
	}
	
	// init tvout
	dpy=&displayers[displayer_count];
	cvbs_module_t *cvbsmodule;	
	if (hw_get_module(CVBS_HARDWARE_MODULE_ID, (const hw_module_t**)&cvbsmodule) == 0) {
		ALOGI(": cvbs engine found.");
	 	ALOGI( "module->methods->open=%#x",
			(unsigned int)cvbsmodule->common.methods->open);
		if (cvbs_dev_open(&cvbsmodule->common, &cvbs_displayer.cvbs_device) == 0) {
			ALOGI(": Got cvbs operations.tvout_device:%x", (unsigned int)cvbs_displayer.cvbs_device);
			dpy->type= TYPE_CVBS; 
			sprintf(dpy->name, "cvbs");
			strcpy(cvbs_displayer.format, "ntsc");
			strcpy(format, cvbs_displayer.format);
			dpy->opaque=&cvbs_displayer;
			dpy->width=720;
			dpy->height=576;
			dpy->dpi=160;
			displayer_count++;
			dpy->id=displayer_count;
        	}
     	} 	

   	//init hdmi
	dpy=&displayers[displayer_count];
	hdmi_module_t *hdmimodule;	
	if (hw_get_module(HDMI_HARDWARE_MODULE_ID, (const hw_module_t**)&hdmimodule) == 0) {
		ALOGI(": hdmi engine found.");
		if (hdmi_dev_open(&hdmimodule->common, &hdmi_displayer.hdmi_device) == 0) {
			ALOGI(": Got hdmi operations.hdmi:%x", (unsigned int)hdmi_displayer.hdmi_device);
		
			dpy->opaque=&hdmi_displayer;
			dpy->width=1280;
			dpy->height=720;
			dpy->dpi=160;
			dpy->type= TYPE_HDMI;
			sprintf(dpy->name, "hdmi");
			displayer_count++;
			dpy->id=displayer_count;	  
		}
     	} 

  // init lcd
    dpy=&displayers[displayer_count];
    {
	dpy->opaque=&lcd_displayer;
	dpy->width=800;
	dpy->height=480;
	dpy->dpi=160;
	dpy->type= TYPE_LCD;
	sprintf(dpy->name, "lcd0");
	displayer_count++;
	dpy->id=displayer_count;

    }
    dedevice->de_get_tv_display_scale(dedevice, &scale_x, &scale_y);
    initialized = 1;
    return true;	 

}

 static int mapDisplayIdFromHalToDrvier(int displayId){
	switch(displayers[displayId].type){
		case TYPE_CVBS:
			return HAL_DE_CVBS_DISPLAYER;
		case TYPE_HDMI:
			return HAL_DE_HDMI_DISPLAYER;
		case TYPE_LCD:
			return HAL_DE_LCD_DISPLAYER;
		default:
			return -1;
	}

}

/**
 * return number of displayers
 */
int actions_server_DisplayService_getDisplayerCount(void)
{
	return displayer_count;

}

char * actions_server_DisplayService_getDisplayerInfo(int id)
{

	displayer_t *dpy;
	
	if(id>=displayer_count){
		return display_info_buf;
	}
	
	dpy = &displayers[id];
	if(dpy->opaque==NULL){
		return display_info_buf;
	}
	sprintf(display_info_buf, "id=%d;width=%d;height=%d;name=%s",
		dpy->id, dpy->width,dpy->height,dpy->name);

	return display_info_buf;
}

void  actions_server_DisplayService_getHdmiCap(void)
{
	 ALOGE("enter actions_server_DisplayService_getHdmiCap\n");

	 hdmi_displayer.hdmi_device->get_capability(hdmi_displayer.hdmi_device, sink_cap_buf);
	 ALOGE("sink_cap_buf:%s\n", sink_cap_buf);
}

 
/*
 *  get display configurations
 *
 */
static void actions_server_DisplayService_getDisplayerParam(void)
{
	// 2. push KEY_AUDIO_CHAN
	sprintf(display_params_buf, "format=%s;colorkey=%s;color=%d;video_scale=%s;hdmi_res_width=%d; \
	hdmi_res_height=%d;hdmi_res_hz=%d;scale-x=%d;scale-y=%d",
		format,colorkey_enable,colorkey, video_scale,
		hdmi_res_width, hdmi_res_height, hdmi_res_hz,
		scale_x, scale_y);
}


 /**
 *  设置如下项
 *  format: NTSC/PAL
 *  colorkey:
 *  alpha
 *
 */
/*static jboolean actions_server_DisplayService_setDisplayerParam(JNIEnv *env, jclass clazz, jstring params)
{
	DisplayParameters disParam;
	const char *valuestr;
	int valueint;
	int tmp;
	if(dedevice==0){
		return false;
	}
	 const jchar* str = env->GetStringCritical(params, 0);    
	 String8 params8;  

	 if (params) {        
	 	params8 = String8(str, env->GetStringLength(params));        
		env->ReleaseStringCritical(params, str);    
	}

	disParam.unflatten(params8);
	// loop all option : 1. format
	if((valuestr=disParam.get(DisplayParameters::KEY_FORMAT))!=0){
		//todo: try loop  displayers !!!
		strcpy(format, valuestr);
		strcpy(cvbs_displayer.format, format);
		if(cvbs_displayer.cvbs_device!=0){
			if(!strcmp(valuestr, FORMAT_NTSC)==0){
				cvbs_displayer.cvbs_device->set_mode(cvbs_displayer.cvbs_device,
					HAL_TVOUT_NTSC);
			}else{
				cvbs_displayer.cvbs_device->set_mode(cvbs_displayer.cvbs_device,
					HAL_TVOUT_PAL);
			}
		}	
	}

	// 2. KEY_AUDIO_CHAN
	if((valuestr=disParam.get(DisplayParameters::KEY_AUDIO_CHAN))!=0){

		
	}

	// 3. KEY_VIDEO_SCALE
	if((valuestr=disParam.get(DisplayParameters::KEY_VIDEO_SCALE))!=0){

		strcpy(video_scale,valuestr);
		ALOGW("to do set scale mode");
		if(!strcmp(valuestr, "fullscreen")){
			//dedevice->set_scale_mode(dedevice, 0);
		}else if(!strcmp(valuestr, "uniform")){
			//dedevice->set_scale_mode(dedevice, 0);
		}else if(!strcmp(valuestr, "origion")){
			//dedevice->set_scale_mode(dedevice, 0);
		}
	}

	// 4. KEY_COLORKEY
	
	valueint=disParam.getInt(DisplayParameters::KEY_COLORKEY);

	colorkey=valueint;
	ALOGW("to do set colorkey");
	

	// 5. KEY_COLOR
	if((valuestr=disParam.get(DisplayParameters::KEY_COLOR))!=0){

		ALOGW("disable/enablecolorkey not implemented");
		if(strcmp(valuestr, "true")==0){
			if(dedevice!=NULL){
				 
			}
		}else if(strcmp(valuestr, "false")){

		}
	}
	// 6. colorkey opacity
	valueint=disParam.getInt(DisplayParameters::KEY_ALPHA);
	ALOGW("set colorkey opacity not implemented");


	// 7. hdmi config
	valueint=disParam.getInt(DisplayParameters::KEY_HDMI_RES_WIDTH);
	if(valueint>0){
		hdmi_res_width=valueint;
	}
	valueint=disParam.getInt(DisplayParameters::KEY_HDMI_RES_HEIGHT);
	if(valueint>0){
		hdmi_res_height=valueint;
	}
	valueint=disParam.getInt(DisplayParameters::KEY_HDMI_RES_HZ);
	if(valueint>0){
		hdmi_res_hz=valueint;
	}

	// 8. scale-x,scale-y:
	int scaleXParam;
	int scaleYPparam;
	scaleXParam=disParam.getInt(DisplayParameters::KEY_SCALE_X);
	scaleYPparam=disParam.getInt(DisplayParameters::KEY_SCALE_Y);
	if(scaleXParam>=0 && scaleYPparam>=0){
		if(dedevice){
			ALOGD("want to set to scaleXParam=%d scaleYPparam=%d \n", scaleXParam,
			scaleYPparam);
			dedevice->de_set_tv_display_scale(dedevice, scaleXParam, scaleYPparam);
			dedevice->de_get_tv_display_scale(dedevice, &scale_x, &scale_y);
			ALOGD(" really scale_x=%d scale_y=%d \n", scale_x, scale_y);
		}
	}

	// 9 hdmi config
	int resWidth;
	int resHeight;
	float resHz;
	int resProgressive;
	int resAspect;
	resWidth=disParam.getInt(DisplayParameters::KEY_HDMI_RES_WIDTH);
	resHeight=disParam.getInt(DisplayParameters::KEY_HDMI_RES_HEIGHT);
	resHz=disParam.getFloat(DisplayParameters::KEY_HDMI_RES_HZ);
	resProgressive=disParam.getInt(DisplayParameters::KEY_HDMI_RES_PG);
	resAspect=disParam.getInt(DisplayParameters::KEY_HDMI_RES_ASPECT);
	if(resWidth>0 && resHeight>0){
		if(hdmi_displayer.hdmi_device!=0){
			hdmi_displayer.hdmi_device->set_mode(hdmi_displayer.hdmi_device,
				resWidth, resHeight, resHz, resProgressive, resAspect);
		}
	}
	
	return true;


}*/
void actions_server_DisplayService_setTvScale(int scaleXParam, int scaleYPparam)
 {
	if(scaleXParam>=0 && scaleYPparam>=0){
		if(dedevice){
			ALOGD("want to set to scaleXParam=%d scaleYPparam=%d \n", scaleXParam,scaleYPparam);
			dedevice->de_set_tv_display_scale(dedevice, scaleXParam, scaleYPparam);
			dedevice->de_get_tv_display_scale(dedevice, &scale_x, &scale_y);
			ALOGD(" really scale_x=%d scale_y=%d \n", scale_x, scale_y);
		}
	}
	
 }
/**
 *  设置如下项
 *  format: NTSC/PAL
 *  colorkey:
 *  alpha
 *
 */
bool actions_server_DisplayService_setCvbsParam(const char *params)
{
	ALOGD("enter actions_server_DisplayService_setCvbsParam\n");
	if(cvbs_displayer.cvbs_device!=0){
			if(!strcmp(params, FORMAT_NTSC)){
				cvbs_displayer.cvbs_device->set_mode(cvbs_displayer.cvbs_device,
					HAL_TVOUT_NTSC);
			}else{
				cvbs_displayer.cvbs_device->set_mode(cvbs_displayer.cvbs_device,
					HAL_TVOUT_PAL);
			}
			return true;
			
	}	
	return false;
}

bool actions_server_DisplayService_setHdmiParam(int vid)
{
		ALOGD("enter actions_server_DisplayService_setHdmiParam\n");
		if(hdmi_displayer.hdmi_device!=0){
			ALOGD("hdmi_displayer.hdmi_device->set_vid:%x\n", hdmi_displayer.hdmi_device->set_vid);
			hdmi_displayer.hdmi_device->set_vid(hdmi_displayer.hdmi_device, vid);
			ALOGD("after hdmi_displayer.hdmi_device->set_vid\n");	
			return true;
		
		}
		
		return false;
}

 void actions_server_DisplayService_setDisplaySingleMode(int mode)
{
	ALOGD("enter actions_server_DisplayService_setDisplaySingleMode\n");
	ALOGD("dedevice->de_set_display_mode_single:%x\n", dedevice->de_set_display_mode_single);
	if (dedevice->de_set_display_mode_single)
		dedevice->de_set_display_mode_single(dedevice, mode);
	else
		ALOGD("dedevice->de_set_display_mode_single is null\n");
}

 void actions_server_DisplayService_setDisplayMode(int mode)
{
	ALOGD("enter actions_server_DisplayService_setDisplayMode\n");
	ALOGD("dedevice->de_set_display_mode:%x\n", dedevice->de_set_display_mode);
	if (dedevice->de_set_display_mode)
		dedevice->de_set_display_mode(dedevice, mode);
	else
		ALOGD("dedevice->de_set_display_mode is null\n");
}


static void actions_server_DisplayService_setHdmiVid(JNIEnv *env, jclass clazz,jint vid)
{
	 ALOGE("enter actions_server_DisplayService_setDisplayMode\n");
	 //ALOGE("dedevice->de_set_display_mode:%d\n",dedevice->de_set_display_mode);
	//dedevice->de_set_display_mode(dedevice, mode);
	 hdmi_displayer.hdmi_device->set_vid(hdmi_displayer.hdmi_device, vid);
}


/**
 * select output displayer
 *  notes: each bytes of id represent a displayer
 */
static bool actions_server_DisplayService_closeSingleOutputDisplayer(int id)
{
	if(id<0 || id>=displayer_count){
		return false;
	}

	switch(displayers[id].type){
		case TYPE_CVBS:
			cvbs_close((cvbs_displayer_device_t *)displayers[id].opaque);
			break;
		case TYPE_HDMI:
			hdmi_close((hdmi_displayer_device_t *)displayers[id].opaque);			
			break;
		case TYPE_LCD:
			break;
		default:
			break;
	}
	

	return true;
}

static int get_real_id(int dp_id)
{
	int i;
	for(i=0; i<displayer_count; i++){
		if(displayers[i].id==dp_id){
			break;
		}
	}
	return i;
}
static bool actions_server_DisplayService_closeOutputDisplayer(int id)
{
	int single_id;
	if(current_display<0 ){
		return false;
	}
	for(int i=0; i<(int)sizeof(int); i++){
		single_id=((0xFFU<<(i*8))&id)>>(i*8);
		single_id=get_real_id(single_id);
		if(single_id>=displayer_count ||single_id<0){
			continue;
		}
		
		actions_server_DisplayService_closeSingleOutputDisplayer(single_id);
	}
	current_display=-1;
	return true;
}

/**
  *@return ,  如果设置成功，则返回true, 否则返回false
  *
  */
static bool  actions_server_DisplayService_setSingleOutputDisplayer(int id)
{
	if(id<0 ||id>=displayer_count){
		return false;
	}
	//disable current Displayer
       // 1. check valid
	
	if(displayers[id].type<=0){
		return false;
	}

	// 3. open new
	switch(displayers[id].type){
			if(cvbs_open((cvbs_displayer_device_t *)displayers[id].opaque)<0){
				return false;
			}
			break;

		case TYPE_HDMI:
			if(hdmi_open((hdmi_displayer_device_t *)displayers[id].opaque)<0){
				return false;
			}
			break;
			
		case TYPE_LCD:
			break;
			
		default:
			break;
	}
	return true;
}

bool actions_server_DisplayService_setOutputDisplayer(int id)
{
	int j;
	int single_id;
	int drv_dpy_no;
	int sucess_id=0;
	displayer_config displayer_config;
	displayer_config.displayer_id=0;
	
	if(dedevice==0){
		return false;
	}
	
	/* check if we are same*/
	if(id==current_display){
		return true;
	}
	
	ALOGD("id=%d \n", id);
	actions_server_DisplayService_closeOutputDisplayer(current_display);
	
	for(int i=0,j=0; i<(int)sizeof(int); i++){
		single_id=((0xFFU<<(i*8))&id)>>(i*8);
		single_id=get_real_id(single_id);
		if(single_id>=displayer_count ||single_id<0){
			continue;
		}
		
		if(actions_server_DisplayService_setSingleOutputDisplayer(single_id)){
			drv_dpy_no=mapDisplayIdFromHalToDrvier(single_id);
			if(drv_dpy_no>=0){
				sucess_id=(sucess_id|((0xFFU<<(j*8))));
				j++;
				displayer_config.displayer_id |=drv_dpy_no;
			}
		}else{
			ALOGW("failed to set displayer  =%d \n", single_id);
		}
	}
	if(sucess_id==0){
		return false;
	}
	dedevice->set_displayer(dedevice, &displayer_config);
	current_display = id;
	return true;

}

bool actions_server_DisplayService_setOutputDisplayer1(char *id)
{
	int j;
	int single_id;
	int drv_dpy_no;
	int tmp_current_display = 0;
	int sucess_id=0;
	displayer_config displayer_config;
	displayer_config.displayer_id=0;
	
	if(dedevice==0){
		return false;
	}
	
	char *result = NULL;
	char delims[] = "_";
	result = strtok(id, delims);
	while(result != NULL) {
		if (!strcmp("cvbs", result)) {
	
			tmp_current_display |= (displayers[0].id << 0);
			
		} else if(!strcmp("hdmi", result)) {
		
			tmp_current_display |= (displayers[1].id << 8);
			
		} else if(!strcmp("lcd0", result)) {
		
			tmp_current_display |= (displayers[2].id << 16);
			
		}
		result = strtok(NULL, delims);
	}

	/* check if we are same*/
	//if (tmp_current_display == current_display)
	//{
	//    return true;
	// }
	ALOGD("tmp_current_display=%x \n", tmp_current_display);
	actions_server_DisplayService_closeOutputDisplayer(current_display);
	for(int i=0,j=0; i<(int)sizeof(int); i++){
		single_id=((0xFFU<<(i*8))&tmp_current_display)>>(i*8);
		single_id=get_real_id(single_id);
		ALOGD("single_id=%d \n", single_id);
		if(single_id>=displayer_count ||single_id<0){
			continue;
		}
		
		if(actions_server_DisplayService_setSingleOutputDisplayer(single_id)){
			drv_dpy_no=mapDisplayIdFromHalToDrvier(single_id);
			if(drv_dpy_no>=0){
				sucess_id=(sucess_id|((0xFFU<<(j*8))));
				j++;
				displayer_config.displayer_id |=drv_dpy_no;
			}
		}else{
			ALOGW("failed to set displayer  =%d \n", single_id);
		}
	}
	if(sucess_id==0){
		return false;
	}
	if (dedevice) {
		if (dedevice->set_displayer) {
	dedevice->set_displayer(dedevice, &displayer_config);
	current_display = tmp_current_display;
		} else {
                   ALOGE("WARNING:dedevice->set_displayer is null\n");
		     return false;
	      }
       } else {
            ALOGE("WARNING:dedevice is null\n");
	     return false;
	}
	return true;

}

static bool actions_server_DisplayService_enableSingleOutput(int dpy_id, bool enable)
{
	int ret=0;
	if(dpy_id<0 || dpy_id >=displayer_count){
		return false;
	}
	
	switch(displayers[dpy_id].type){
		case TYPE_CVBS:
			ret=cvbs_enable((cvbs_displayer_device_t *)displayers[dpy_id].opaque, enable);	
			break;
			
		case TYPE_HDMI:
			ret=hdmi_enable((hdmi_displayer_device_t *)displayers[dpy_id].opaque, enable);
			break;
			
		default:
			break;
	}
	
	if(ret<0){
		return false;
	}
	return true;
}


/**
 * enable/disable current display
 */
bool actions_server_DisplayService_enableOutput(bool enable)
{
	int single_id;
	if(current_display<0 ){
		return false;
	}
	for(int i=0; i<(int)sizeof(int); i++){
		single_id=((0xFFU<<(i*8))&current_display)>>(i*8);
		single_id=get_real_id(single_id);
		if(single_id>=displayer_count ||single_id<0){
			continue;
		}
		
		if(!actions_server_DisplayService_enableSingleOutput(single_id, enable)){
			return false;
		}
	}
	return true;
	
}


static jboolean actions_server_DisplayService_setDualDisplay(JNIEnv *env, jclass clazz, jboolean dual)
{
	
	return false;
}

/**
 * 返回tv连接电缆的连接情况
 *
 */
int actions_server_DisplayService_getCableState(void)
{
	int flag=0;
	
	for(int i = 0; i < displayer_count; i++){
		switch(displayers[i].type){
			case TYPE_CVBS:
				if(cvbs_cable_connected((cvbs_displayer_device_t *)displayers[i].opaque)){
					flag |=CABLE_CVBS_ON;
				}
				break;
			case TYPE_HDMI:
				if(hdmi_cable_connected((hdmi_displayer_device_t *)displayers[i].opaque)){
					flag |= CABLE_HDMI_ON;
				}
				break;
			default:
				break;
		}
	}
	return flag;

}


/**
 * 打开hdmi，作为显示设备
 *
 */
static int hdmi_open(struct hdmi_displayer_device_t *hdmi_displayer)
{
	hdmi_device_t  *hdmi_device;
	//displayer_config displayer_config;
	video_setting_t vid_setting;
	int retval;

	hdmi_device = hdmi_displayer->hdmi_device;
	if(0==hdmi_device){
	       return -2;
	}
		
	//hdmi_device->get_capability(hdmi_device, &hdmi_displayer->sink_caps);	
	retval=hdmi_device->resolve_config(hdmi_device, &vid_setting );
	if(retval<0){
		return -3;
	}
	
	//displayer_config.displayer_id = HAL_DE_HDMI_DISPLAYER;
	//return dedevice->set_displayer(dedevice, &displayer_config);
	return 0;
}

static int hdmi_enable(struct hdmi_displayer_device_t *hdmi_displayer, bool enable){

	int ret;
	if(hdmi_displayer==0 ||hdmi_displayer->hdmi_device==0){
		return -1;
	}
	
	if(enable){
		ret=hdmi_displayer->hdmi_device->enable(hdmi_displayer->hdmi_device);

	} else {
		ret=hdmi_displayer->hdmi_device->disable(hdmi_displayer->hdmi_device);
	}
	return ret;
	
}

static bool hdmi_cable_connected(struct hdmi_displayer_device_t *hdmi_displayer){

	int ret;
	if(hdmi_displayer==0 ||hdmi_displayer->hdmi_device==0){
		return false;
	}
	ret=hdmi_displayer->hdmi_device->is_connected(hdmi_displayer->hdmi_device);
	if(ret<1){
		return false;
	}else{
		return true;
	}
}


static int hdmi_close(struct hdmi_displayer_device_t *hdmi_displayer){

	hdmi_displayer->hdmi_device->disable(hdmi_displayer->hdmi_device);
	return 0;
}


/**
 * 打开cvbs，作为显示设备
 *
 */
static int cvbs_open(struct cvbs_displayer_device_t * cvbs_displayer)
{
	struct cvbs_device_t  *cvbs_device;

	cvbs_device = cvbs_displayer->cvbs_device;
	if(0==cvbs_device){
	       return -2;
	}

	if(!strcmp(cvbs_displayer->format,FORMAT_NTSC)){
		cvbs_device->set_mode(cvbs_device, HAL_TVOUT_NTSC);
	}else{
		cvbs_device->set_mode(cvbs_device, HAL_TVOUT_PAL);
	}
	
	
	return 0;
	
}

static bool cvbs_cable_connected(struct cvbs_displayer_device_t *cvbs_displayer){

	int ret;
	if(cvbs_displayer==0 ||cvbs_displayer->cvbs_device==0){
		return false;
	}
	ret=cvbs_displayer->cvbs_device->is_connected(cvbs_displayer->cvbs_device);
	if(ret<1){
		return false;
	}else{
		return true;
	}
}

static int cvbs_enable(struct cvbs_displayer_device_t *cvbs_displayer, bool enable){

	int ret;
	if(cvbs_displayer==0 ||cvbs_displayer->cvbs_device==0){
		return -1;
	}
	if(enable){
		ret=cvbs_displayer->cvbs_device->enable(cvbs_displayer->cvbs_device);
	}else{
	
		ret=cvbs_displayer->cvbs_device->disable(cvbs_displayer->cvbs_device);
	}
	return ret;
	
}


static int cvbs_close(struct cvbs_displayer_device_t *cvbs_displayer){
	cvbs_displayer->cvbs_device->disable(cvbs_displayer->cvbs_device);
	return 0;
}
};  // namespace

