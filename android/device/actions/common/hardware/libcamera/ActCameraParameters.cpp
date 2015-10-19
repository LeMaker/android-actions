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



#include "CameraHalDebug.h"

#include <string.h>
#include <stdlib.h>
#include <ActCameraParameters.h>

#include "CameraHal.h"

namespace android
{

const char ActCameraParameters::KEY_SATURATION[] = "saturation";
const char ActCameraParameters::KEY_BRIGHTNESS[] = "brightness";
const char ActCameraParameters::KEY_EXPOSURE_MODE[] = "exposure";
const char ActCameraParameters::KEY_SUPPORTED_EXPOSURE[] = "exposure-mode-values";
const char ActCameraParameters::KEY_CONTRAST[] = "contrast";
const char ActCameraParameters::KEY_SHARPNESS[] = "sharpness";
const char ActCameraParameters::KEY_DENOISE[] = "denoise";
const char ActCameraParameters::KEY_ISO[] = "iso";
const char ActCameraParameters::KEY_SUPPORTED_ISO_VALUES[] = "iso-mode-values";
const char ActCameraParameters::KEY_AUTO_FOCUS_LOCK[] = "auto-focus-lock";

const char ActCameraParameters::ZOOM_SUPPORTED[] = "true";
const char ActCameraParameters::ZOOM_UNSUPPORTED[] = "false";

const char ActCameraParameters::KEY_EXIF_MODEL[] = "exif-model";
const char ActCameraParameters::KEY_EXIF_MAKE[] = "exif-make";

const char ActCameraParameters::KEY_GPS_MAPDATUM[] = "gps-mapdatum";
const char ActCameraParameters::KEY_GPS_VERSION[] = "gps-version";
const char ActCameraParameters::KEY_GPS_DATESTAMP[] = "gps-datestamp";

// ACT extensions to add iso values
const char ActCameraParameters::ISO_MODE_AUTO[] = "auto";
const char ActCameraParameters::ISO_MODE_100[] = "100";
const char ActCameraParameters::ISO_MODE_200[] = "200";
const char ActCameraParameters::ISO_MODE_400[] = "400";
const char ActCameraParameters::ISO_MODE_800[] = "800";
const char ActCameraParameters::ISO_MODE_1000[] = "1000";
const char ActCameraParameters::ISO_MODE_1200[] = "1200";
const char ActCameraParameters::ISO_MODE_1600[] = "1600";


//ACT values for camera direction
const char ActCameraParameters::FACING_FRONT[]="front";
const char ActCameraParameters::FACING_BACK[]="back";

//ACT extensions to add sensor orientation parameters
const char ActCameraParameters::ORIENTATION_SENSOR_NONE[] = "0";
const char ActCameraParameters::ORIENTATION_SENSOR_90[] = "90";
const char ActCameraParameters::ORIENTATION_SENSOR_180[] = "180";
const char ActCameraParameters::ORIENTATION_SENSOR_270[] = "270";


const char ActCameraParameters::KEY_MIN_CONTRAST[] = "min-contrast";
const char ActCameraParameters::KEY_MAX_CONTRAST[] = "max-contrast";
const char ActCameraParameters::KEY_CONTRAST_STEP[] = "contrast-step";

const char ActCameraParameters::KEY_MIN_SATURATION[] = "min-staturation";
const char ActCameraParameters::KEY_MAX_SATURATION[] = "max-staturation";
const char ActCameraParameters::KEY_SATURATION_STEP[] = "staturation-step";

const char ActCameraParameters::KEY_MIN_BRIGHTNESS[] = "min-brightness";
const char ActCameraParameters::KEY_MAX_BRIGHTNESS[] = "max-brightness";
const char ActCameraParameters::KEY_BRIGHTNESS_STEP[] = "brightness-step";

const char ActCameraParameters::KEY_MIN_DENOISE[] = "min-denoise";
const char ActCameraParameters::KEY_MAX_DENOISE[] = "max-denoise";
const char ActCameraParameters::KEY_DENOISE_STEP[] = "denoise-step";

const char ActCameraParameters::KEY_RECORD_VIDEO_WIDTH[]= "record-video-width";
const char ActCameraParameters::KEY_RECORD_VIDEO_HEIGHT[]= "record-video-height";

const char ActCameraParameters::PIXEL_FORMAT_YUV420SP_NV12[] = "yuv420sp_nv12";
const char ActCameraParameters::PIXEL_FORMAT_YUV420P_YU12[] ="yuv420p_yu12";
};

