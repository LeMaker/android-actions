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
* @file OMXDefaults.cpp
*
* This file contains definitions are OMX Camera defaults
*
*/
#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"

namespace android
{

#define __STRINGIFY(s) __STRING(s)

// OMX Camera defaults
const char OMXCameraAdapter::DEFAULT_ANTIBANDING[] = "off";
const char OMXCameraAdapter::DEFAULT_BRIGHTNESS[] = "0";
const char OMXCameraAdapter::DEFAULT_CONTRAST[] = "0";
const char OMXCameraAdapter::DEFAULT_SATURATION[] = "0";
const char OMXCameraAdapter::DEFAULT_DENOISE[] = "0";
const char OMXCameraAdapter::DEFAULT_EFFECT[] = "none";
const char OMXCameraAdapter::DEFAULT_EV_COMPENSATION[] = "0";
//ActionsCode(author:liuyiguang, change_code)
const char OMXCameraAdapter::DEFAULT_EV_STEP[] = "0";
//const char OMXCameraAdapter::DEFAULT_EV_STEP[] = "1";
const char OMXCameraAdapter::DEFAULT_EXPOSURE_MODE[] = "auto";
const char OMXCameraAdapter::DEFAULT_FLASH_MODE[] = "off";
const char OMXCameraAdapter::DEFAULT_FOCUS_MODE_PREFERRED[] = "auto";
const char OMXCameraAdapter::DEFAULT_FOCUS_MODE[] = "infinity";
const char OMXCameraAdapter::DEFAULT_PREVIEW_FRAMERATE_RANGE[] = "30000,30000";
const char OMXCameraAdapter::DEFAULT_IMAGE_FRAMERATE_RANGE[]="30000,30000";
const char OMXCameraAdapter::DEFAULT_ISO_MODE[] = "auto";

const char OMXCameraAdapter::DEFAULT_JPEG_QUALITY[] = "95";
const char OMXCameraAdapter::DEFAULT_THUMBNAIL_QUALITY[] = "60";
const char OMXCameraAdapter::DEFAULT_THUMBNAIL_SIZE[] = "128x96";
const char OMXCameraAdapter::DEFAULT_PICTURE_FORMAT[] = "jpeg";
const char OMXCameraAdapter::DEFAULT_PICTURE_SIZE[] = "1600x1200";
#ifdef CAMERA_RUN_IN_EMULATOR
const char OMXCameraAdapter::DEFAULT_PREVIEW_FORMAT[] = "rgb565";
#else
const char OMXCameraAdapter::DEFAULT_PREVIEW_FORMAT[] = "yuv420sp";
#endif
const char OMXCameraAdapter::DEFAULT_FRAMERATE[] = "30";
const char OMXCameraAdapter::DEFAULT_PREVIEW_SIZE[] = "800x600";
const char OMXCameraAdapter::DEFAULT_NUM_PREV_BUFS[] = "6";
const char OMXCameraAdapter::DEFAULT_NUM_PIC_BUFS[] = "2";
#ifdef CAMERA_FOCUS_AREA
const char OMXCameraAdapter::DEFAULT_MAX_FOCUS_AREAS[] = "1";
const char OMXCameraAdapter::DEFAULT_FOCUS_AREAS[] = "(0,0,0,0,0)";
#else
const char OMXCameraAdapter::DEFAULT_MAX_FOCUS_AREAS[] = "0";
const char OMXCameraAdapter::DEFAULT_FOCUS_AREAS[] = "(0,0,0,0,0)";
#endif
const char OMXCameraAdapter::DEFAULT_SCENE_MODE[] = "auto";
const char OMXCameraAdapter::DEFAULT_SHARPNESS[] = "0";
const char OMXCameraAdapter::DEFAULT_VSTAB[] = "false";
const char OMXCameraAdapter::DEFAULT_VSTAB_SUPPORTED[] = "false";
const char OMXCameraAdapter::DEFAULT_WB[] = "auto";
const char OMXCameraAdapter::DEFAULT_ZOOM[] = "0";

#ifdef CAMERA_VCE_OMX_FD
const char OMXCameraAdapter::DEFAULT_MAX_FD_HW_FACES[] = MAX_FD_HW_NUM_STR;
const char OMXCameraAdapter::DEFAULT_MAX_FD_SW_FACES[] = "0";
#else
const char OMXCameraAdapter::DEFAULT_MAX_FD_HW_FACES[] = "0";
const char OMXCameraAdapter::DEFAULT_MAX_FD_SW_FACES[] = "0";
#endif
const char OMXCameraAdapter::DEFAULT_FOCAL_LENGTH_PRIMARY[] = "3.43";
const char OMXCameraAdapter::DEFAULT_FOCAL_LENGTH_SECONDARY[] = "1.95";
const char OMXCameraAdapter::DEFAULT_FOCUS_DISTANCES[] = "1.000000,34.000000,Infinity";
const char OMXCameraAdapter::DEFAULT_HOR_ANGLE[] = "100";
const char OMXCameraAdapter::DEFAULT_VER_ANGLE[] = "100";
const char OMXCameraAdapter::DEFAULT_AE_LOCK[] = "false";
const char OMXCameraAdapter::DEFAULT_AWB_LOCK[] = "false";
const char OMXCameraAdapter::DEFAULT_MAX_NUM_METERING_AREAS[] = "0";
const char OMXCameraAdapter::DEFAULT_LOCK_SUPPORTED[] = "true";
const char OMXCameraAdapter::DEFAULT_LOCK_UNSUPPORTED[] = "false";
#ifdef CAMERA_VIDEO_SNAPSHOT
const char OMXCameraAdapter::DEFAULT_VIDEO_SNAPSHOT_SUPPORTED[] = "true";
#else
const char OMXCameraAdapter::DEFAULT_VIDEO_SNAPSHOT_SUPPORTED[] = "false";
#endif
const char OMXCameraAdapter::DEFAULT_VIDEO_SIZE[] = "800x600";
const char OMXCameraAdapter::DEFAULT_PREFERRED_PREVIEW_SIZE_FOR_VIDEO[] = "1600x1200";

const char OMXCameraAdapter::DEFAULT_PREVIEW_SIZE_SEC[] = "640x480";
const char OMXCameraAdapter::DEFAULT_PICTURE_SIZE_SEC[] = "640x480";
const char OMXCameraAdapter::DEFAULT_FRAMERATE_SEC[] = "30";
const char OMXCameraAdapter::DEFAULT_PREVIEW_FRAMERATE_RANGE_SEC[] = "30000,30000";
const char OMXCameraAdapter::DEFAULT_IMAGE_FRAMERATE_RANGE_SEC[] = "30000,30000";
const char OMXCameraAdapter::DEFAULT_VIDEO_SIZE_SEC[] = "640x480";
const char OMXCameraAdapter::DEFAULT_PREFERRED_PREVIEW_SIZE_FOR_VIDEO_SEC[] = "640x480";

const char OMXCameraAdapter::DEFAULT_SENSOR_ROTATION[] = "0";
const char OMXCameraAdapter::DEFAULT_SENSOR_ROTATION_SEC[] = "0";

};

