/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
* @file CameraProperties.cpp
*
* This file maps the CameraHardwareInterface to the Camera interfaces on OMAP4 (mainly OMX).
*
*/

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "CameraHalDebug.h"

//#include "CameraHal.h"
#include <utils/threads.h>

#include "CameraProperties.h"
#include "CameraConfigs.h"
#include "OMXCameraAdapter.h"
#include "MediaProfileGenerator/MediaProfileGenerator.h"

namespace android
{

static unsigned int gFirstRun = 1;

/*********************************************************
 CameraProperties - public function implemetation
**********************************************************/

CameraProperties::CameraProperties() 
{
    LOG_FUNCTION_NAME;

    mCamerasSupported = 0;
    mInitialized = 0;

    LOG_FUNCTION_NAME_EXIT;
}

CameraProperties::~CameraProperties()
{
    LOG_FUNCTION_NAME;
    

    LOG_FUNCTION_NAME_EXIT;
}
                     

// Initializes the CameraProperties class
status_t CameraProperties::initialize()
{
    LOG_FUNCTION_NAME;

    status_t ret;

    Mutex::Autolock lock(mLock);

    if(mInitialized)
        return NO_ERROR;

    ret = loadProperties();

    mInitialized = 1;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}
// Initializes the CameraProperties class
status_t CameraProperties::initializeForced()
{
    LOG_FUNCTION_NAME;

    status_t ret;

    Mutex::Autolock lock(mLock);

    ret = loadProperties();

    mInitialized = 1;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}
/**
* NEW_FEATURE: Add reupdate xml for UVC hotplug support .
*ActionsCode(author:liyuan, change_code)
*/
extern "C" int CameraAdapter_Capabilities(CameraProperties::Properties* properties_array,
        const unsigned int starting_camera,
        const unsigned int max_camera,
        int *XMLNeedsUpdate);

///Loads all the Camera related properties
status_t CameraProperties::loadProperties()
{
    LOG_FUNCTION_NAME;

    status_t ret = NO_ERROR;
	int XMLNeedsUpdate =0;
    if(gFirstRun){
        if(waitForSensorReady() != NO_ERROR)
        {
            ALOGD("waitForSensorReady failed\n");
        }
    }

    // adapter updates capabilities and we update camera count
    mCamerasSupported = CameraAdapter_Capabilities(mCameraProps, 0, MAX_CAMERAS_SUPPORTED,&XMLNeedsUpdate);

    if((int)mCamerasSupported < 0)
    {
        CAMHAL_LOGEA("error while getting capabilities");
        ret = UNKNOWN_ERROR;
    }
    else if (mCamerasSupported > MAX_CAMERAS_SUPPORTED)
    {
        CAMHAL_LOGEA("returned too many adapaters");
        ret = UNKNOWN_ERROR;
    }
    else
    {
#ifdef CAMERA_PROFILES_AUTOGEN

        if(gFirstRun||XMLNeedsUpdate){
            genProfiles();
        }
#endif
        for (unsigned int i = 0; i < mCamerasSupported; i++)
        {
            mCameraProps[i].dump();
        }

    }
    gFirstRun = 0;

    CAMHAL_LOGDB("mCamerasSupported = %d", mCamerasSupported);
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

// Returns the number of Cameras found
int CameraProperties::camerasSupported()
{
    LOG_FUNCTION_NAME;
    return mCamerasSupported;
}

// Parse string like "640x480" or "10000,20000"
static int parse_pair(const char *str, int *first, int *second, char delim,
                      char **endptr = NULL)
{
    // Find the first integer.
    char *end;
    int w = (int)strtol(str, &end, 10);
    // If a delimeter does not immediately follow, give up.
    if (*end != delim) {
        ALOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }

    // Find the second integer, immediately after the delimeter.
    int h = (int)strtol(end+1, &end, 10);

    *first = w;
    *second = h;

    if (endptr) {
        *endptr = end;
    }

    return 0;
}

static void parseSizesList(const char *sizesStr, Vector<CameraFrameSize> &sizes)
{
    if (sizesStr == 0) {
        return;
    }

    char *sizeStartPtr = (char *)sizesStr;

    while (true) {
        int width, height;
        int success = parse_pair(sizeStartPtr, &width, &height, 'x',
                                 &sizeStartPtr);
        if (success == -1 || (*sizeStartPtr != ',' && *sizeStartPtr != '\0')) {
            ALOGE("Picture sizes string \"%s\" contains invalid character.", sizesStr);
            return;
        }
        sizes.push(CameraFrameSize(width, height));

        if (*sizeStartPtr == '\0') {
            return;
        }
        sizeStartPtr++;
    }
}
void CameraProperties::genProfiles()
{
    //generator profiles
    char value[PROPERTY_VALUE_MAX];
    const char *defaultXmlFile = "/etc/camcorder_profiles.xml";
    const char *xmlFile;
    xmlFile = defaultXmlFile;
    if (property_get("camcorder.settings.xml", value, NULL) > 0) {
        xmlFile = (char *)&value;
    }
    //ALOGD("genarated mediaprofiles.xml is located in xmlFile = %s", xmlFile);

    genProfiles("/etc/camcorder_profiles_template.xml", xmlFile );
}

/**
 *
 * MERGEFIX:  Fix for add new resolution.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
/**
* NEW_FEATURE: Add UVC module support .
*ActionsCode(author:liyuan, change_code)
*/
void CameraProperties::genProfiles(const char *temp_path, const char *profile_path)
{
    int id = 0;
    unsigned int i = 0;
    unsigned int j = 0;
    const char *valstr = NULL;
    const char *videosizestr = NULL;
    Vector<CameraFrameSize> sizes;
    Vector<CameraFrameSize> addedSizes;
    status_t ret = NO_ERROR;
    char sensor0[48];
    char sensor1[48];
    /**
    * NEW_FEATURE: Add UVC module support .
    *ActionsCode(author:liyuan, change_code)
    */
    OMX_UVCMODE uvcmode = OMX_UVC_NONE;
    OMXCameraAdapter::get_UVC_ReplaceMode(&uvcmode, -1);

    if(uvcmode==OMX_UVC_AS_REAR){
	strcpy(sensor0, "uvcvideo.ko");
    	getSensorName(1, (char *)&sensor1, 48);
		
    }else if(uvcmode==OMX_UVC_AS_FRONT){
	getSensorName(0, (char *)&sensor0, 48);
	strcpy(sensor1, "uvcvideo.ko");
		
    }else{
	getSensorName(0, (char *)&sensor0, 48);
    	getSensorName(1, (char *)&sensor1, 48);
    }

    ALOGD("sensor %d name=%s", 0, (char *)&sensor0);
    ALOGD("sensor %d name=%s", 1, (char *)&sensor1);

    if(MediaProfileGenerator::checkMediaProfile(profile_path, (char *)&sensor0, (char *)&sensor1) &&
	   uvcmode == OMX_UVC_NONE)
    {
        ALOGD("MediaProfile exist!");
        return;
    }

    MediaProfileGenerator *generator = NULL;
    generator = new MediaProfileGenerator();
    if(generator == NULL)
    {
        ALOGE("new MediaProfileGenerator failed!");
        goto exit;
    }
    ret = generator->loadTemplate(temp_path);
    if(ret < 0)
    {
        ALOGE("load Mediaprofile template failed\n");
        goto exit;
    }
    ret = generator->setCameraNum(mCamerasSupported);

    ret |= generator->setSensorName(0, (char *)&sensor0);
    ret |= generator->setSensorName(1, (char *)&sensor1);
    if(ret < 0)
    {
        ALOGE("setSensorName failed\n");
        goto exit;
    }

    for (i = 0; i < mCamerasSupported; i++)
    {       
        sizes.clear();
        addedSizes.clear();

        videosizestr = mCameraProps[i].get(CameraProperties::SUPPORTED_VIDEO_SIZES);
        if(videosizestr == NULL
            || strcmp(videosizestr, "")==0)
        {
            videosizestr = mCameraProps[i].get(CameraProperties::SUPPORTED_PREVIEW_SIZES);
        }
        if(videosizestr == NULL
            || strcmp(videosizestr, "")==0)
        {
            ALOGE("videosize is NULL!");
            goto exit;
        }

        parseSizesList(videosizestr, sizes);

        for(j = 0; j < sizes.size(); j++)
        {
			CameraFrameSize size = sizes.itemAt(j);  
            //ActionsCode(author:liuyiguang, add_code)
			//if((size.width == 320 && size.height == 240)
			//|| (size.width == 352 && size.height == 288)
			//|| (size.width == 720 && size.height == 480)
			//|| (size.width == 1280 && size.height == 720)
			//|| (size.width == 1920 && size.height == 1080)
			//)
			if((size.width == 320 && size.height == 240)
			|| (size.width == 352 && size.height == 288)
			|| (size.width == 640 && size.height == 480)
			|| (size.width == 704 && size.height == 480)
			|| (size.width == 720 && size.height == 480)
			|| (size.width == 1280 && size.height == 720)
            || (size.width == 1280 && size.height == 960)
            || (size.width == 1600 && size.height == 1200)
			|| (size.width == 1920 && size.height == 1080)
			)
			{
                ALOGE("addedSizes.push(%dx%d)", size.width, size.height);
                addedSizes.push(size);
            }
        }

        if(CameraConfigs::getSingleVSize() == 0 )
        {
            for(j = 0; j < addedSizes.size(); j++)
            {
                CameraFrameSize size = addedSizes.itemAt(j);         
                generator->addVideoSize(i, size.width, size.height);
            }
        }
        else
        {
            int maxIndex = -1;
            int maxSizeValue = 0;
            //find max size
            for(j = 0; j < addedSizes.size(); j++)
            {
                CameraFrameSize size = addedSizes.itemAt(j);
            
                if(size.width * size.height > maxSizeValue)
                {
                    maxSizeValue = size.width * size.height;
                    maxIndex = j;
                }
            }
            if(maxIndex >= 0 && maxIndex < (int)addedSizes.size())
            {
                CameraFrameSize size = addedSizes.itemAt(maxIndex);         
                generator->addVideoSize(i, size.width, size.height);
            }
        }
        
    }

    ret |= generator->genMediaProfile(profile_path);

    if(ret)
    {
        ALOGD("generate media profile failed!");
    }
    else
    {
        ALOGD("generate media profile successfully!");
    }
    
    exit:
    if(generator != NULL)
    {
        delete generator;
    }
    return;
}

#define READ_DATA(fd, data, size, rsize) do{  \
    int read_len = size;               \
    int bytes_read= 0;                \
    int ret = 0;                        \
    while(bytes_read < read_len)      \
    {                                   \
        ret = read(fd, (data) + bytes_read, read_len- bytes_read);    \
        if(ret > 0)                     \
        {                               \
            bytes_read+=ret;           \
        }                               \
        else                            \
        {                               \
            break;                      \
        }                               \
    }                                   \
    *(rsize) = bytes_read;            \
}while(0)

void CameraProperties::getSensorName(int index, char *buf, int size)
{
    const char *valstr = NULL;
    int sensor_index = 0;
    char name[64];
    const char *sysfs_path= NULL;
    int fd = -1;
    int readsize;

    if(buf == NULL || size <= 0)
    {
        return ;
    }
    memset((char *)&name, 0, 64);

    if(index >= 2)
    {
        goto failed;
    }

    sensor_index = index;

    //back
    if(sensor_index == 0)
    {
        sysfs_path = "/sys/rear_camera/rear_name";
    }
    else
    {
        sysfs_path = "/sys/front_camera/front_name";
    }

    fd = open(sysfs_path, O_RDONLY );
    if(fd < 0)
    {
        ALOGE("open file (%s) error!\n", sysfs_path);
        goto failed;
    }

    READ_DATA(fd, (char *)&name, 64-1, (&readsize));
    if(readsize <=0)
    {
        ALOGE("file (%s) is empty!\n", sysfs_path);
        goto failed;
    }

    name[readsize] = '\0';
	//ALOGD("sensor name is %s",name);
#if 0
    //size of "camera_.ko"
    if(strlen((char *)&name) <= strlen("camera_.ko"))
    {
        ALOGD("sensor name lenght check failed");
        goto failed;
    }

    if(!((strncmp((char *)&name, "camera_", 7) == 0)
        && (strncmp(((char *)&name+strlen((char *)&name)-3), ".ko", 3) == 0)))
    {
        ALOGD("sensor name check failed(%s)", (char *)&name);
        goto failed;
    }

    if(size < (int)(strlen((char *)&name) - strlen("camera_.ko") + 1))
    {
        ALOGD("buf length is too short");
        goto failed;
    }
	#endif
    strlcpy(buf, (char *)&name, strlen((char *)&name) +1);
	

    if(fd >= 0)
    {
        close(fd);
    }
    return;

failed:
    if(fd >= 0)
    {
        close(fd);
    }
    buf[0] = '\0';
    return;


}

bool CameraProperties::sensorDetectReady()
{
    bool ret = false;
	//return true;
    char status[4];
    int fd = -1;
    int readsize;
    const char *sysfs_path= "/sys/rear_camera/status";

    //if status node is not exist, we assume the driver is ready for backward compatibility
    fd = open(sysfs_path, O_RDONLY );
    if(fd < 0)
    {
        ALOGE("open file (%s) error!\n", sysfs_path);
        ret = true;
        goto exit;
    }
    READ_DATA(fd, (char *)&status, 4-1, (&readsize));
    if(readsize <=0)
    {
        ALOGE("file (%s) is empty!\n", sysfs_path);
        ret = true;
        goto exit;
    }
    status[readsize] = '\0';

    ret = atoi((char *)&status) == 1 ? true : false;

exit:
    if(fd >= 0)
    {
        close(fd);
    }
    return ret;
}
bool CameraProperties::sensorDriverReady()
{
    bool ret = false;
    char sensor0[48];
    char sensor1[48];

    const char *back_dev = "/dev/video0";
    const char *front_dev = "/dev/video1";

    getSensorName(0, (char *)&sensor0, 48);
    getSensorName(1, (char *)&sensor1, 48);

    if(strlen(sensor0) > 0)
    {
        if (access(back_dev, R_OK) != 0)
        {
            ret = false;
            ALOGE("back driver is not ready");
            goto exit;
        }

    }
    
    //if back sensor are same with front sensor, it will:
    //sensor0 == sensor1, 
    //and only have /dev/video0 
    if(strlen(sensor1) > 0 && (strcmp(sensor0, sensor1) != 0))
    {
        if (access(front_dev, R_OK) != 0)
        {
            ret = false;
            ALOGE("front driver is not ready");
            goto exit;
        }
    }

    ret = true;

exit:
    return ret;
}
bool CameraProperties::waitForSensorReady()
{
    const int step_time = 20*1000;
    const int max_detect_time = 40*1000*1000;
    const int max_detect_count = max_detect_time/step_time;
    const int max_load_driver_time = 4*1000*1000;
    const int max_load_driver_count = max_load_driver_time/step_time;
    int count = 0;
    bool ret = true;

    //wait for sensor detect ready
    while(!sensorDetectReady())
    {
        if(count >= max_detect_count)
        {
            ALOGD("sensorDetectReady timeout\n");
            ret = false;
            break;
        }
        usleep(step_time);
        count++;
    }
    if(!ret)
    {
        return ret;
    }

    //wait for driver insmod ready
    count = 0;
    while(!sensorDriverReady())
    {
        if(count >= max_load_driver_count)
        {
            ALOGD("sensorDriverReady timeout\n");
            ret = false;
            break;
        }
        usleep(step_time);
        count++;
    }

    return ret;
}


};
