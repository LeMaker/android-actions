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




#ifndef TI_CAMERA_PARAMETERS_H
#define TI_CAMERA_PARAMETERS_H

#include <utils/KeyedVector.h>
#include <utils/String8.h>

namespace android
{

///TI Specific Camera Parameters
class ActCameraParameters
{
public:
    static const  char KEY_SATURATION[];
    static const  char KEY_BRIGHTNESS[];
    static const  char KEY_EXPOSURE_MODE[];
    static const  char KEY_SUPPORTED_EXPOSURE[];
    static const  char KEY_CONTRAST[];
    static const  char KEY_SHARPNESS[];
    static const  char KEY_DENOISE[];
    static const  char KEY_ISO[];
    static const  char KEY_SUPPORTED_ISO_VALUES[];
    static const char  KEY_AUTO_FOCUS_LOCK[];

//extensions for zoom
    static const char ZOOM_SUPPORTED[];
    static const char ZOOM_UNSUPPORTED[];

//extensions for setting EXIF tags
    static const char KEY_EXIF_MODEL[];
    static const char KEY_EXIF_MAKE[];

//extensions for additional GPS data
    static const char  KEY_GPS_MAPDATUM[];
    static const char  KEY_GPS_VERSION[];
    static const char  KEY_GPS_DATESTAMP[];

//extensions to add iso values
    static const char ISO_MODE_AUTO[];
    static const char ISO_MODE_100[];
    static const char ISO_MODE_200[];
    static const char ISO_MODE_400[];
    static const char ISO_MODE_800[];
    static const char ISO_MODE_1000[];
    static const char ISO_MODE_1200[];
    static const char ISO_MODE_1600[];

//extensions to add sensor orientation parameters
    static const char ORIENTATION_SENSOR_NONE[];
    static const char ORIENTATION_SENSOR_90[];
    static const char ORIENTATION_SENSOR_180[];
    static const char ORIENTATION_SENSOR_270[];


//values for camera direction
    static const char FACING_FRONT[];
    static const char FACING_BACK[];

    static const char KEY_MIN_CONTRAST[];
    static const char KEY_MAX_CONTRAST[];
    static const char KEY_CONTRAST_STEP[];

    static const char KEY_MIN_SATURATION[];
    static const char KEY_MAX_SATURATION[];
    static const char KEY_SATURATION_STEP[];

    static const char KEY_MIN_BRIGHTNESS[];
    static const char KEY_MAX_BRIGHTNESS[];
    static const char KEY_BRIGHTNESS_STEP[];

    static const char KEY_MIN_DENOISE[];
    static const char KEY_MAX_DENOISE[];
    static const char KEY_DENOISE_STEP[];


    static const  char KEY_RECORD_VIDEO_WIDTH[];
    static const  char KEY_RECORD_VIDEO_HEIGHT[];

    static const  char PIXEL_FORMAT_YUV420SP_NV12[];
    static const  char PIXEL_FORMAT_YUV420P_YU12[];

};

};

#endif

