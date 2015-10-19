
#ifndef __CAMERA_CONFIG_H__
#define __CAMERA_CONFIG_H__

#include <utils/Vector.h>

namespace android
{

typedef struct
{
    int width;
    int height;
}CameraResCfg;

//ES--EV and Scene
enum
{
    ES_SELECTOR_NONE=0,
    ES_SELECTOR_EV,
    ES_SELECTOR_SCENE,
    ES_SELECTOR_BOTH,
};


class CameraConfigs
{
public:

    static int getCameraRotation();
    static Vector<CameraResCfg> getPrevExRes(int id);
    static Vector<CameraResCfg> getImageExRes(int id);

    static int getDefaultPrevRes(int id, CameraResCfg *res);
    static int getDefaultImageRes(int id, CameraResCfg *res);

    static int getDefaultFps(int id);

    static int getFlashSupported(int id);
		static int getHflip(int id);
		static int getVflip(int id);			

    static float getHorAngle(int id);
    static float getVerAngle(int id);
    
    static int getHdrSupported(int id);

    static float getHdrMin();
    static float getHdrMax();
    

    static int getFocusAreaFake();
    static int getAutoFocusFake();

    static int getSnapshot(int id);

    static int getEvSceneSelector();

    static int getQcifPreview();
    
    static int getSingleVSize();
    /**
    * NEW_FEATURE: Add face detect config control interface.
    *ActionsCode(author:liyuan, change_code)
    */
    static int getEnableFaceDetect();
    /**
    * NEW_FEATURE: Add UVC module support .
    *ActionsCode(author:liyuan, change_code)
    */
    static int getUVCReplaceMode();
	static void setUVCModeActivate(int val);

	
private:
    static const char BACK_PREV_EXRES[];
    static const char BACK_IMAGE_EXRES[];
    static const char FRONT_PREV_EXRES[];
    static const char FRONT_IMAGE_EXRES[];
    static const char BCAK_DEFAULT_PREV_RES[];
    static const char BCAK_DEFAULT_IMAGE_RES[];
    static const char FRONT_DEFAULT_PREV_RES[];
    static const char FRONT_DEFAULT_IMAGE_RES[];
    static const char SENSOR_ROTATION[];
    static const char BACK_DEFAULT_FPS[];
    static const char FRONT_DEFAULT_FPS[];
    
    static const char BACK_FLASH[];
    static const char FRONT_FLASH[];
    
    static const char BACK_HFLIP[];
    static const char FRONT_HFLIP[];
    static const char BACK_VFLIP[];
    static const char FRONT_VFLIP[];    

    static const char BACK_HOR_ANGLE[];
    static const char BACK_VER_ANGLE[];
    static const char FRONT_HOR_ANGLE[];
    static const char FRONT_VER_ANGLE[];

    static const char BACK_HDR[];
    static const char FRONT_HDR[];

    static const char HDR_EV_MIN[];    
    static const char HDR_EV_MAX[];    
    
    static const char FOCUS_AREA_FAKE[];    
    static const char AUTO_FOCUS_FAKE[];    
        
    static const char SNAPSHOT_BACK[];    
    static const char SNAPSHOT_FRONT[];    

    //FIXME: ev and scene are exclusive in sensor driver, so use 
    //this option to select ev or scene.
    //values: both, ev, scene, none
    static const char EV_SCENE_SELECTOR[];    

    static const char QCIF_PREVIEW[];  
    
    static const char SINGLE_VIDEO_SIZE[];
    /**
    * NEW_FEATURE: Add face detect config control interface.
    *ActionsCode(author:liyuan, change_code)
    */
    static const char ENABLE_FD[];
    /**
    * NEW_FEATURE: Add UVC module support .
    *ActionsCode(author:liyuan, change_code)
    */
    static const char UVC_REPLACEMODE[];
	static const char UVC_CURRENTACTID[];

    
};

}

#endif

