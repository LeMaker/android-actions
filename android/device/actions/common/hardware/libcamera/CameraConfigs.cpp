
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <errno.h>
#include <assert.h>
#include <dlfcn.h>
#include <ctype.h>

#include "CameraHalDebug.h"

#include "cutils/properties.h"

#include "CameraResTable.h"
#include "CameraConfigs.h"

namespace android
{

const char CameraConfigs::BACK_PREV_EXRES[] = "ro.camerahal.prevres0";
const char CameraConfigs::BACK_IMAGE_EXRES[] = "ro.camerahal.imageres0";
const char CameraConfigs::FRONT_PREV_EXRES[] = "ro.camerahal.prevres1";
const char CameraConfigs::FRONT_IMAGE_EXRES[] = "ro.camerahal.imageres1";
const char CameraConfigs::BCAK_DEFAULT_PREV_RES[] = "ro.camerahal.prevresdft0";
const char CameraConfigs::BCAK_DEFAULT_IMAGE_RES[] = "ro.camerahal.imageresdft0";
const char CameraConfigs::FRONT_DEFAULT_PREV_RES[] ="ro.camerahal.prevresdft1";
const char CameraConfigs::FRONT_DEFAULT_IMAGE_RES[] = "ro.camerahal.imageresdft1";
const char CameraConfigs::SENSOR_ROTATION[] = "ro.camerahal.configorientation";
const char CameraConfigs::BACK_DEFAULT_FPS[] = "ro.camerahal.fpsdft0";
const char CameraConfigs::FRONT_DEFAULT_FPS[] = "ro.camerahal.fpsdft1";

const char CameraConfigs::BACK_FLASH[] = "ro.camerahal.flash0";
const char CameraConfigs::FRONT_FLASH[] = "ro.camerahal.flash1";

const char CameraConfigs::BACK_HFLIP[] = "sys.camerahal.hflip0";
const char CameraConfigs::BACK_VFLIP[] = "sys.camerahal.vflip0";	
const char CameraConfigs::FRONT_HFLIP[] = "sys.camerahal.hflip1";
const char CameraConfigs::FRONT_VFLIP[] = "sys.camerahal.vflip1";	
	
const char CameraConfigs::BACK_HOR_ANGLE[] ="ro.camerahal.hangle0";
const char CameraConfigs::BACK_VER_ANGLE[] ="ro.camerahal.vangle0";
const char CameraConfigs::FRONT_HOR_ANGLE[] = "ro.camerahal.hangle1";
const char CameraConfigs::FRONT_VER_ANGLE[] = "ro.camerahal.vangle1";

const char CameraConfigs::BACK_HDR[] = "ro.camerahal.hdr0";
const char CameraConfigs::FRONT_HDR[] = "ro.camerahal.hdr1";

const char CameraConfigs::HDR_EV_MIN[] = "ro.camerahal.hdr_ev_min";
const char CameraConfigs::HDR_EV_MAX[] = "ro.camerahal.hdr_ev_max";

const char CameraConfigs::FOCUS_AREA_FAKE[] = "ro.camerahal.focusarea_fake";    
const char CameraConfigs::AUTO_FOCUS_FAKE[] = "ro.camerahal.autofocus_fake";    

const char CameraConfigs::SNAPSHOT_BACK[] = "ro.camerahal.snapshot0";    
const char CameraConfigs::SNAPSHOT_FRONT[] = "ro.camerahal.snapshot1";    

const char CameraConfigs::EV_SCENE_SELECTOR[] = "ro.camerahal.ev_scene";//    

const char CameraConfigs::QCIF_PREVIEW[] = "ro.camerahal.qcif_preview";   

const char CameraConfigs::SINGLE_VIDEO_SIZE[] = "ro.camerahal.single_vsize"; 
/**
* NEW_FEATURE: Add face detect config control interface.
*ActionsCode(author:liyuan, change_code)
*/
const char CameraConfigs::ENABLE_FD[] = "ro.camerahal.enable_fd"; 
/**
* NEW_FEATURE: Add UVC module support .
*ActionsCode(author:liyuan, change_code)
*/
const char CameraConfigs::UVC_REPLACEMODE[] = "ro.camerahal.uvc_replacemode"; 

const char CameraConfigs::UVC_CURRENTACTID[] = "sys.camerahal.uvc_actmode_id"; 


int CameraConfigs::getCameraRotation()
{
    char value[PROPERTY_VALUE_MAX];

    property_get(CameraConfigs::SENSOR_ROTATION, value, "0");

    if(strcmp(value, "0") != 0 
        && strcmp(value, "90") != 0
        && strcmp(value, "180") != 0
        && strcmp(value, "270") != 0)
    {
        memset(value, 0 , PROPERTY_VALUE_MAX);
        value[0] = '0';
    }      

    return atoi(value);
}

static char * myTrim(char *str)
{
    char *last,*cur;
    if(str==NULL)
    {
        return NULL;
    }
    for( ;isspace(*str); str++);
    for(last=cur=str; *cur!='\0'; cur++)
    {
        if(!isspace(*cur))
        {
            last=cur;
        }
    }
    *(++last)=0;
    return str;
}

static char * myUpcase(char *str)
{
    char *cur;
    if(str==NULL)
    {
        return NULL;
    }
    for(cur = str; *cur != '\0'; cur++)
    {
        if(*cur>='a' && *cur <= 'z')
        {
            *cur += ('A'-'a');
        }
    }
    
    return str;
} 

static int parserResList(char *prop, Vector<CameraResCfg> *reslist)
{
    char *a;
    char *b;
    const CameraResItem *resItem;
    CameraResCfg res;

    reslist->clear();
    a = prop;

    if(a == NULL || a[0]=='\0')
    {
        return 0;
    }

    for (;;) {
        b = strchr(a, ',');
        if (b == NULL) {

            a =  myTrim(a);
            a =  myUpcase(a);
            if(a != NULL)
            {
                //ALOGD("parserResList:%s",a);
                resItem = CameraResTable::getResByName(a);
                if(resItem)
                {
                    res.width = resItem->width;
                    res.height = resItem->height;
                    reslist->add(res);
                }
            }

            break;
        }

        *b = 0;
        a =  myTrim(a);
        a =  myUpcase(a);
        if(a != NULL)
        {
            //ALOGD("parserResList:%s",a);
            resItem = CameraResTable::getResByName(a);
            if(resItem)
            {
                res.width = resItem->width;
                res.height = resItem->height;
                reslist->add(res);
            }
        }
        a = b+1;
    }

    return 0;
    
}
static int parserRes(char *prop, CameraResCfg *res)
{
    char *a;
    char *b;
    const  CameraResItem *resItem;

    a = prop;

    if(a == NULL || a[0]=='\0')
    {
        return -1;
    }
    a = myTrim(a);
    a = myUpcase(a);

    //ALOGD("parserRes:%s",a);
    resItem = CameraResTable::getResByName(a);
    if(resItem)
    {
        res->width = resItem->width;
        res->height = resItem->height;
        return 0;
    }

    return -1;
}


Vector<CameraResCfg> CameraConfigs::getPrevExRes(int id)
{
    char value[PROPERTY_VALUE_MAX];
    Vector<CameraResCfg> resList;

    if(id == 0)
    {
        property_get(CameraConfigs::BACK_PREV_EXRES, value, "VGA");
    }
    else
    {
        property_get(CameraConfigs::FRONT_PREV_EXRES, value, "VGA");
    }
    //ALOGD("getPrevExRes:%s",value);

    parserResList(value, &resList);
    return resList; 

}

Vector<CameraResCfg> CameraConfigs::getImageExRes(int id)
{
    char value[PROPERTY_VALUE_MAX];
    Vector<CameraResCfg> resList;

    if(id == 0)
    {
        property_get(CameraConfigs::BACK_IMAGE_EXRES, value, "VGA");
    }
    else
    {
        property_get(CameraConfigs::FRONT_IMAGE_EXRES, value, "VGA");
    }

    //ALOGD("getImageExRes:%s",value);
    parserResList(value, &resList);
    return resList; 
}

int CameraConfigs::getDefaultPrevRes(int id, CameraResCfg *res)
{
    char value[PROPERTY_VALUE_MAX];
    int ret = 0;

    if(id == 0)
    {
        property_get(CameraConfigs::BCAK_DEFAULT_PREV_RES, value, "");
    }
    else
    {
        property_get(CameraConfigs::FRONT_DEFAULT_PREV_RES, value, "");
    }

    ret = parserRes(value, res);
    return ret; 
}

int CameraConfigs::getDefaultImageRes(int id,  CameraResCfg *res)
{
    char value[PROPERTY_VALUE_MAX];
    int ret = 0;

    if(id == 0)
    {
        property_get(CameraConfigs::BCAK_DEFAULT_IMAGE_RES, value, "");
    }
    else
    {
        property_get(CameraConfigs::FRONT_DEFAULT_IMAGE_RES, value, "");
    }

    ret = parserRes(value, res);
    return ret; 
}


int CameraConfigs::getDefaultFps(int id)
{
    char value[PROPERTY_VALUE_MAX];
    int fps;

    if(id == 0)
    {
        property_get(CameraConfigs::BACK_DEFAULT_FPS, value, "30");
    }
    else
    {
        property_get(CameraConfigs::FRONT_DEFAULT_FPS, value, "30");

    }

    fps = atoi(value);
    if(fps<5 || fps>60)
    {
        fps = 0;
    }

    return fps;
}

int CameraConfigs::getFlashSupported(int id)
{
    char value[PROPERTY_VALUE_MAX];
    int flash = 1;

    if(id == 0)
    {
        property_get(CameraConfigs::BACK_FLASH, value, "1");
    }
    else
    {
        property_get(CameraConfigs::FRONT_FLASH, value, "1");
    }

    flash = atoi(value);

    return flash;
}
int CameraConfigs::getHflip(int id)
{
    char value[PROPERTY_VALUE_MAX];
    int hflip = 0;

    if(id == 0)
    {
        property_get(CameraConfigs::BACK_HFLIP, value, "0");
    }
    else
    {
        property_get(CameraConfigs::FRONT_HFLIP, value, "0");
    }

    hflip = atoi(value);

    return hflip;
}
int CameraConfigs::getVflip(int id)
{
    char value[PROPERTY_VALUE_MAX];
    int vflip = 0;

    if(id == 0)
    {
        property_get(CameraConfigs::BACK_VFLIP, value, "0");
    }
    else
    {
        property_get(CameraConfigs::FRONT_VFLIP, value, "0");
    }

    vflip = atoi(value);

    return vflip;
}

float CameraConfigs::getHorAngle(int id)
{
    char value[PROPERTY_VALUE_MAX];
    float angle = 100.0;

    if(id == 0)
    {
        property_get(CameraConfigs::BACK_HOR_ANGLE, value, "100");
    }
    else
    {
        property_get(CameraConfigs::FRONT_HOR_ANGLE, value, "100");
    }

    angle = atof(value);

    return angle;
}

float CameraConfigs::getVerAngle(int id)
{
    char value[PROPERTY_VALUE_MAX];
    float angle = 100.0;

    if(id == 0)
    {
        property_get(CameraConfigs::BACK_VER_ANGLE, value, "100");
    }
    else
    {
        property_get(CameraConfigs::FRONT_VER_ANGLE, value, "100");
    }

    angle = atof(value);

    return angle;
}

/**
 *
 * BUGFIX:  Fix for HDR, open HDR function default.
 *
 ************************************
 *
 * ActionsCode(author:liuyiguang, change_code)
 */
int CameraConfigs::getHdrSupported(int id)
{
    char value[PROPERTY_VALUE_MAX];
    int hdr = 0;

    if(id == 0)
    {
        property_get(CameraConfigs::BACK_HDR, value, "1");
    }
    else
    {
        property_get(CameraConfigs::FRONT_HDR, value, "1");
    }

    hdr = atoi(value);

    return hdr;
}

float CameraConfigs::getHdrMin()
{
    char value[PROPERTY_VALUE_MAX];
    float min = -2.0;

    property_get(CameraConfigs::HDR_EV_MIN, value, "-2.0");

    min = atof(value);

    return min;
}

float CameraConfigs::getHdrMax()
{
    char value[PROPERTY_VALUE_MAX];
    float max = 2.0;

    property_get(CameraConfigs::HDR_EV_MAX, value, "2.0");

    max = atof(value);

    return max;
}

int CameraConfigs::getFocusAreaFake()
{
    char value[PROPERTY_VALUE_MAX];
    int fake = 0;

    property_get(CameraConfigs::FOCUS_AREA_FAKE, value, "0");

    fake = atoi(value);

    return fake;
}


int CameraConfigs::getAutoFocusFake()
{
    char value[PROPERTY_VALUE_MAX];
    int fake = 1;

    //default to Fake AutoFocus
    property_get(CameraConfigs::AUTO_FOCUS_FAKE, value, "1");

    fake = atoi(value);

    return fake;
}

int CameraConfigs::getSnapshot(int id)
{
    char value[PROPERTY_VALUE_MAX];
    int snapshot = 0;

    if(id == 0)
    {
        property_get(CameraConfigs::SNAPSHOT_BACK, value, "1");
    }
    else
    {
        property_get(CameraConfigs::SNAPSHOT_FRONT, value, "1");
    }

    snapshot = atoi(value);

    return snapshot;
}

/**
 *
 * BUGFIX:  Support for exposure value setting, set the sel equal to ES_SELECTOR_EV.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
int CameraConfigs::getEvSceneSelector()
{
    char value[PROPERTY_VALUE_MAX];
    int sel = ES_SELECTOR_SCENE;

    //ActionsCode(author:liuyiguang, change_code)
    //property_get(CameraConfigs::EV_SCENE_SELECTOR, value, "scene");
    property_get(CameraConfigs::EV_SCENE_SELECTOR, value, "ev");

    if(strncmp(value, "none", 4)==0)
    {
        sel = ES_SELECTOR_NONE;
    }
    else if(strncmp(value, "scene", 4)==0)
    {
        sel = ES_SELECTOR_SCENE;
    }
    else if(strncmp(value, "ev", 2)==0)
    {
        sel = ES_SELECTOR_EV;
    }
    else if(strncmp(value, "both", 4)==0)
    {
        sel = ES_SELECTOR_BOTH;
    }

    return sel;
}

int CameraConfigs::getQcifPreview()
{
    //Do not enable videosize. the videosnapshot test will be failed. 
#if 0
    char value[PROPERTY_VALUE_MAX];
    int enable = 1;

    property_get(CameraConfigs::QCIF_PREVIEW, value, "0");

    enable = atoi(value);

    return enable;
#else
    return 1;
#endif
}

int CameraConfigs::getSingleVSize()
{
    char value[PROPERTY_VALUE_MAX];
    int single = 1;

    property_get(CameraConfigs::SINGLE_VIDEO_SIZE, value, "1");

    single = atoi(value);

    return single;
}
/**
* NEW_FEATURE: Add face detect config control interface.
*ActionsCode(author:liyuan, change_code)
*/
int CameraConfigs::getEnableFaceDetect()
{
    char value[PROPERTY_VALUE_MAX];
    int enable = 1;

    property_get(CameraConfigs::ENABLE_FD, value, "1");
    enable = atoi(value);
    return enable;
}
/**
* NEW_FEATURE: Add UVC module support .
*ActionsCode(author:liyuan, change_code)
*/
int CameraConfigs::getUVCReplaceMode()
{
    char value[PROPERTY_VALUE_MAX];
    int mode = 0;

    property_get(CameraConfigs::UVC_REPLACEMODE, value, "0");
    mode = atoi(value);
    return mode;
}
void CameraConfigs::setUVCModeActivate(int val)
{
	if(val==0){
    	property_set(CameraConfigs::UVC_CURRENTACTID, "0");
	}else if(val==1){
    	property_set(CameraConfigs::UVC_CURRENTACTID, "1");
	}else if(val==2){
		property_set(CameraConfigs::UVC_CURRENTACTID, "2");
	}
    return;
}

};

