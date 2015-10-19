/*
 * control the outstanding of GPU, Avoiding the lcd flash when camera preview 
 */

#define GPU_OUTSTANDING_ENTER "13" 
#define GPU_OUTSTANDING_EXIT "0"

#define GPU_OUTSTANDING_2D_PATH "/sys/devices/system/cpu/cpufreq/gpufreq/gpu2doutstanding"
#define GPU_OUTSTANDING_3D_PATH "/sys/devices/system/cpu/cpufreq/gpufreq/gpu3doutstanding"

#include <fcntl.h>
#include "CameraHalDebug.h"

#include <utils/Mutex.h>
#include <cutils/properties.h>

using namespace android;

static int sOutstandingRefCount = 0;
static Mutex sOutstandingLock;
static bool sOutstandingInited = false;
static bool sSetOutstandingEnabled = true;

static void initGPUOutstanding()
{
#ifdef CAMERA_SET_GPUOUTSTANDING
    if(sOutstandingInited == false)
    {
        char value[PROPERTY_VALUE_MAX];
        property_get("ro.camerahal.setgpuoutstanding", value, "0");

        if(strcmp(value, "0") == 0)
        {
            sSetOutstandingEnabled = false;
        }
        else
        {
            sSetOutstandingEnabled = true;
        }

        sOutstandingInited = true;
    }
    return ;
#else
    return;
#endif
}

static int writeToFile(const char *path, const char *val)
{
    int fd = -1;

    size_t size = 0;
    size_t retCount = -1;

    size = strlen(val);
    fd = open(path, O_RDWR);
    if(fd < 0)
    {
        ALOGE("can not open %s !", path);
        return -1;
    }

    retCount = write(fd, val, size);

    if(retCount != size)
    {
        ALOGE("write gpu outstanding error(%s, %s)!", path, val);

        retCount = -1;
    }

    close(fd);

    return retCount;
}
int setGPUOutstanding(const char* val)
{
#ifdef CAMERA_SET_GPUOUTSTANDING

    int gpu_dev = -1;
    int ret =0;

    initGPUOutstanding();

    if(sSetOutstandingEnabled)
    {
        ret = writeToFile(GPU_OUTSTANDING_2D_PATH, val);
        ret |= writeToFile(GPU_OUTSTANDING_3D_PATH, val);
    }
    return ret;
#else
    return 0;
#endif
}

int startGPUOutstanding()
{
#ifdef CAMERA_SET_GPUOUTSTANDING

    int ret = 0;
    Mutex::Autolock lock(sOutstandingLock);
    if(sOutstandingRefCount == 0)
    {
        ret = setGPUOutstanding(GPU_OUTSTANDING_ENTER);
    }
    sOutstandingRefCount ++;
    return ret;
#else
    return 0;
#endif
}

int stopGPUOutstanding()
{
#ifdef CAMERA_SET_GPUOUTSTANDING

    int ret = 0;
    Mutex::Autolock lock(sOutstandingLock);
    if(sOutstandingRefCount>0)
    {
        sOutstandingRefCount --;
    }
    else
    {
        sOutstandingRefCount = 0;
    }
    if(sOutstandingRefCount == 0)
    {
        ret = setGPUOutstanding(GPU_OUTSTANDING_EXIT);
    }
    return ret;
#else
    return 0;
#endif
}

int clearGPUOutstanding()
{
#ifdef CAMERA_SET_GPUOUTSTANDING
    int ret = 0;
    Mutex::Autolock lock(sOutstandingLock);
    if(sOutstandingRefCount>0)
    {
        ret = setGPUOutstanding(GPU_OUTSTANDING_EXIT);
    }
    sOutstandingRefCount = 0;
    return ret;
#else
    return 0;
#endif
}



