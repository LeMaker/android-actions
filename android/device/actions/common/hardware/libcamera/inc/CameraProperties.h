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




#ifndef CAMERA_PROPERTIES_H
#define CAMERA_PROPERTIES_H

#include <utils/KeyedVector.h>
#include <utils/Vector.h>
#include <utils/String8.h>
#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "cutils/properties.h"

namespace android
{

#define MAX_CAMERAS_SUPPORTED 2
#define MAX_SIMUL_CAMERAS_SUPPORTED 1
#define MAX_PROP_NAME_LENGTH 50
#define MAX_PROP_VALUE_LENGTH 2048

#define EXIF_MAKE_DEFAULT "default_make"
#define EXIF_MODEL_DEFAULT "default_model"

//5MP
//#define DEFAULT_MAX_PICTURE_WIDTH   2600
//#define DEFAULT_MAX_PICTURE_HEIGHT  2000 
//ActionsCode(author:liuyiguang, change_code)
//8MP
#define DEFAULT_MAX_PICTURE_WIDTH   3300
#define DEFAULT_MAX_PICTURE_HEIGHT  2500 

struct ExtendedResolution
{
    size_t width, height;
    size_t captureWidth, captureHeight;
    size_t cropWidth,cropHeight;
    bool   needCrop;
};

// Class that handles the Camera Properties
class CameraProperties
{
public:
    static const char INVALID[];
    static const char CAMERA_NAME[];
    static const char CAMERA_SENSOR_INDEX[];
    static const char ORIENTATION_INDEX[];
    static const char FACING_INDEX[];
    static const char S3D_SUPPORTED[];
    static const char SUPPORTED_PREVIEW_SIZES[];
    static const char SUPPORTED_PREVIEW_FORMATS[];
    static const char SUPPORTED_PREVIEW_FRAME_RATES[];
    static const char SUPPORTED_PICTURE_SIZES[];
    static const char SUPPORTED_PICTURE_FORMATS[];
    static const char SUPPORTED_THUMBNAIL_SIZES[];
    static const char SUPPORTED_WHITE_BALANCE[];
    static const char SUPPORTED_EFFECTS[];
    static const char SUPPORTED_ANTIBANDING[];
    static const char SUPPORTED_EXPOSURE_MODES[];
    static const char SUPPORTED_EV_MIN[];
    static const char SUPPORTED_EV_MAX[];
    static const char SUPPORTED_EV_STEP[];
    static const char SUPPORTED_ISO_VALUES[];
    static const char SUPPORTED_SCENE_MODES[];
    static const char SUPPORTED_FLASH_MODES[];
    static const char SUPPORTED_FOCUS_MODES[];
    static const char REQUIRED_PREVIEW_BUFS[];
    static const char REQUIRED_IMAGE_BUFS[];
    static const char SUPPORTED_ZOOM_RATIOS[];
    static const char SUPPORTED_ZOOM_STAGES[];
    static const char SMOOTH_ZOOM_SUPPORTED[];
    static const char ZOOM_SUPPORTED[];
    static const char PREVIEW_SIZE[];
    static const char PREVIEW_FORMAT[];
    static const char PREVIEW_FRAME_RATE[];
    static const char ZOOM[];
    static const char PICTURE_SIZE[];
    static const char PICTURE_FORMAT[];
    static const char JPEG_THUMBNAIL_SIZE[];
    static const char WHITEBALANCE[];
    static const char EFFECT[];
    static const char ANTIBANDING[];
    static const char EXPOSURE_MODE[];
    static const char EV_COMPENSATION[];
    static const char ISO_MODE[];
    static const char FOCUS_MODE[];
    static const char SCENE_MODE[];
    static const char FLASH_MODE[];
    static const char JPEG_QUALITY[];
    static const char BRIGHTNESS[];
    static const char SATURATION[];
    static const char SHARPNESS[];
    static const char CONTRAST[];
    static const char AUTOCONVERGENCE[];
    static const char AUTOCONVERGENCE_MODE[];
    static const char MANUALCONVERGENCE_VALUES[];
    static const char SENSOR_ORIENTATION[];
    static const char SENSOR_ORIENTATION_VALUES[];
    static const char REVISION[];
    static const char FOCAL_LENGTH[];
    static const char FOCUS_DISTANCES[];
    static const char HOR_ANGLE[];
    static const char VER_ANGLE[];
    static const char EXIF_MAKE[];
    static const char EXIF_MODEL[];
    static const char JPEG_THUMBNAIL_QUALITY[];
    static const char MAX_FOCUS_AREAS[];
    static const char MAX_REAL_FOCUS_AREAS[];
    static const char MAX_FD_HW_FACES[];
    static const char MAX_FD_SW_FACES[];

    static const char PARAMS_DELIMITER [];

    static const char S3D2D_PREVIEW[];
    static const char S3D2D_PREVIEW_MODES[];
    static const char VSTAB[];
    static const char VSTAB_SUPPORTED[];
    static const char PREVIEW_FRAMERATE_RANGE[];
    static const char IMAGE_FRAMERATE_RANGE[];



    static const char DEFAULT_VALUE[];

    static const char AUTO_EXPOSURE_LOCK[];
    static const char AUTO_EXPOSURE_LOCK_SUPPORTED[];
    static const char AUTO_WHITEBALANCE_LOCK[];
    static const char AUTO_WHITEBALANCE_LOCK_SUPPORTED[];
    static const char AUTO_FOCUS_LOCK[];
    static const char AUTO_FOCUS_LOCK_SUPPORTED[];
    
    static const char MAX_NUM_METERING_AREAS[];
    static const char METERING_AREAS[];
    static const char MAX_NUM_FOCUS_AREAS[];

    static const char VIDEO_SNAPSHOT_SUPPORTED[];

    static const char VIDEO_SIZE[];
    static const char SUPPORTED_VIDEO_SIZES[];
    static const char PREFERRED_PREVIEW_SIZE_FOR_VIDEO[];


    static const char IMAGE_FRAMERATE_RANGE_SUPPORTED[];
    static const char PREVIEW_FRAMERATE_RANGE_SUPPORTED[];
    static const char CONTRAST_SUPPORTED[];
    static const char SATURATION_SUPPORTED[];
    static const char BRIGHTNESS_SUPPORTED[];
    static const char SHARPNESS_SUPPORTED[];
    static const char DENOISE_SUPPORTED[];
    static const char ISO_SUPPORTED[];
    static const char FLASH_SUPPORTED[];
    static const char FOCUS_SUPPORTED[];
    static const char ANTIBANDING_SUPPORTED[];

    static const char SUPPORTED_CONTRAST_MIN[];
    static const char SUPPORTED_CONTRAST_MAX[];
    static const char SUPPORTED_CONTRAST_STEP[];

    static const char SUPPORTED_SATURATION_MIN[];
    static const char SUPPORTED_SATURATION_MAX[];
    static const char SUPPORTED_SATURATION_STEP[];

    static const char SUPPORTED_BRIGHTNESS_MIN[];
    static const char SUPPORTED_BRIGHTNESS_MAX[];
    static const char SUPPORTED_BRIGHTNESS_STEP[];

    static const char SUPPORTED_DENOISE_MIN[];
    static const char SUPPORTED_DENOISE_MAX[];
    static const char SUPPORTED_DENOISE_STEP[];

    static const char DENOISE[];

    static const char SCENE_SUPPORTED[];
    static const char EFFECT_SUPPORTED[];
    static const char WHITEBALANCE_SUPPORTED[];

    CameraProperties();
    ~CameraProperties();

    class FormatInfo
    {
    public:
        FormatInfo()
        {
            clear();
        }
        ~FormatInfo()
        {
            clear();
        }
        void clear()
        {
            mFormatMaps.clear();
            mFormatSupported.clear();
        }

        int addFormat(int format)
        {
            mFormatSupported.add(format);
            return 0;
        }
        int addFormatMap(int from, int to)
        {
            mFormatMaps.add(from,to);
            return 0;
        }
        bool hasFormat(int format)
        {
            for(unsigned int i = 0 ; i < mFormatSupported.size(); i++)
            {
                if(mFormatSupported.itemAt(i) == format)
                {
                    return true;
                }
            }
            return false;
        }
        int getMapValue(int key)
        {
            int value = -1;
            int index = mFormatMaps.indexOfKey(key);
            if(index >= 0)
            {
                value = mFormatMaps.valueAt(index);
            }
            return value;
        }

        int getMapKey(int value)
        {
            int key = -1;
            for(unsigned int i = 0 ; i < mFormatMaps.size(); i++)
            {
                if(mFormatMaps.valueAt(i) == value)
                {
                    return mFormatMaps.keyAt(i);
                }
            }
            return key;
        }
    private:
        KeyedVector<int,int> mFormatMaps;
        Vector<int> mFormatSupported;

    };

    // container class passed around for accessing properties
    class Properties
    {
    public:
        Properties()
        {
            mProperties = new DefaultKeyedVector<String8, String8>(String8(DEFAULT_VALUE));
            char property[PROPERTY_VALUE_MAX];
            property_get("ro.product.manufacturer", property, EXIF_MAKE_DEFAULT);
            property[0] = toupper(property[0]);
            set(EXIF_MAKE, property);
            property_get("ro.product.model", property, EXIF_MODEL_DEFAULT);
            property[0] = toupper(property[0]);
            set(EXIF_MODEL, property);

            mFaceDetectEnable = false;

            mMaxCaptureWidth = DEFAULT_MAX_PICTURE_WIDTH;
            mMaxCaptureHeight = DEFAULT_MAX_PICTURE_HEIGHT;
        }
        ~Properties()
        {
            delete mProperties;
            mExtendedImageResolution.clear();
            mExtendedVideoResolution.clear();
            mPreviewFormatInfos.clear();
        }
        ssize_t set(const char *prop, const char *value);
        ssize_t set(const char *prop, int value);
        const char* get(const char * prop);
        void dump();


        Vector<ExtendedResolution> mExtendedImageResolution;
        Vector<ExtendedResolution> mExtendedVideoResolution;

        FormatInfo mPreviewFormatInfos;
        bool mFaceDetectEnable;

        int mMaxCaptureWidth;
        int mMaxCaptureHeight;



    protected:
        const char* keyAt(unsigned int);
        const char* valueAt(unsigned int);

    private:
        DefaultKeyedVector<String8, String8>* mProperties;

    };

    ///Initializes the CameraProperties class
    status_t initialize();
    status_t initializeForced();
    status_t loadProperties();
    int camerasSupported();
    int getProperties(int cameraIndex, Properties** properties);

    void genProfiles(const char *temp_path, const char *profile_path);
    void genProfiles();

    void getSensorName(int index, char *buf, int size);    
    bool sensorDetectReady();
    bool sensorDriverReady();
    bool waitForSensorReady();

private:

    uint32_t mCamerasSupported;
    int mInitialized;
    mutable Mutex mLock;

    Properties mCameraProps[MAX_CAMERAS_SUPPORTED];

};

};

#endif //CAMERA_PROPERTIES_H

