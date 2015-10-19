
/*
 * control the freq range of cpu when camera preview to avoiding system crash or hang 
 */

#include "CameraHalDebug.h"

#include <fcntl.h>
#include <utils/Mutex.h>
#include <cutils/properties.h>


#include <performance/IPerformanceService.h>
#include <camera/ICameraService.h>
#include <camera/ICameraService.h>
#include <binder/IBinder.h>

#include <utils/String16.h>
#include <utils/String8.h>
#include "CameraFreqAdapter.h"

using namespace android;

static int sFreqRangeRefCount = 0;
static Mutex sFreqRangeLock;
static bool sFreqRangeInited = false;
static bool sFreqRangeEnabled = true;


#define CAMERA_FREQ_MIN     (300*1000)
#define CAMERA_FREQ_MAX     (600*1000)

class performaceDeathTracker: public IBinder::DeathRecipient {
   public:
       performaceDeathTracker(){
       }

       virtual void binderDied(const wp<IBinder>& who){
           CAMHAL_LOGEA("Performace service dead..... ");
       }

};


static  sp<performaceDeathTracker> sPerformaceDeathTracker = new performaceDeathTracker();

static inline sp<IBinder> getCameraService()
{

    sp<ICameraService> cameraService = NULL;
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder = NULL;

    if(sm == NULL)
    {
        CAMHAL_LOGEB("%s, %s,get ServiceManager error",__FILE__,__func__);
        return NULL;
    }
    do {
        binder = sm->getService(String16("media.camera"));
        if (binder != 0) {
            break;
        }
        usleep(500000); // 0.5 s
    } while(true);

    return binder;
}

static inline sp<IBinder> getMoniterBinder()
{
    return getCameraService(); 
}


static void initFreqRangeAdapter()
{
    if(sFreqRangeInited == false)
    {
        char value[PROPERTY_VALUE_MAX];
        property_get("ro.camerahal.setfreqrange", value, "1");

        if(strcmp(value, "0") == 0)
        {
            sFreqRangeEnabled = false;
        }
        else
        {
            sFreqRangeEnabled = true;
        }

        sFreqRangeInited = true;
    }
    return ;
}        



static int setCpuFreqRange(unsigned int min, unsigned int max)
{
    int ret = 0;

    initFreqRangeAdapter();

    if(sFreqRangeEnabled)
    {
        sp<IPerformanceService> performanceSvc = IPerformanceService::connect();
        if(performanceSvc == NULL)
        {
            ret = -1;
            CAMHAL_LOGEB("%s, get service manager error", __func__);
            goto exit;
        }

        performanceSvc->asBinder()->linkToDeath(sPerformaceDeathTracker);

        ret = performanceSvc->setCpuFreqRange(getMoniterBinder(),min, max);
    }
exit:
    return ret;

}

static int restoreCpuFreqRange()
{
    int ret = 0;

    initFreqRangeAdapter();

    if(sFreqRangeEnabled)
    {
        sp<IPerformanceService> performanceSvc = IPerformanceService::connect();
        if(performanceSvc == NULL)
        {
            ret = -1;
            CAMHAL_LOGEB("%s,get service manager error", __func__);
            goto exit;
        }

        ret = performanceSvc->restoreCpuFreqRange(getMoniterBinder());
    }
exit:
    return ret;
}

int cameraSetCpuFreqRange()
{
    int ret = 0;

    Mutex::Autolock lock(sFreqRangeLock);
    if(sFreqRangeRefCount == 0)
    {
        ret = setCpuFreqRange(CAMERA_FREQ_MIN, CAMERA_FREQ_MAX);
    }
    sFreqRangeRefCount ++;

    return ret;
}
int CameraRestoreCpuFreqRange()
{
    int ret = 0;

    Mutex::Autolock lock(sFreqRangeLock);
    if(sFreqRangeRefCount > 0)
    {
        sFreqRangeRefCount--;
    }
    else
    {
        sFreqRangeRefCount=0;
    }
    if(sFreqRangeRefCount==0)
    {
        ret = restoreCpuFreqRange();
    }

    return ret;
}

int CameraForceRestoreCpuFreqRange()
{
    int ret = 0;

    Mutex::Autolock lock(sFreqRangeLock);
    if(sFreqRangeRefCount > 0)
    {
        ret = restoreCpuFreqRange();
    }
    sFreqRangeRefCount=0;

    return ret;
}

