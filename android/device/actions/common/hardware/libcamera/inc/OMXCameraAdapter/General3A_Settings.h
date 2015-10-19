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
* @file General3A_Settings.h
*
* This file maps the Camera Hardware Interface to OMX.
*
*/


#ifndef GENERAL_3A_SETTINGS_H
#define GENERAL_3A_SETTINGS_H

#include "ACT_OMX_IVCommon.h"
#include "ACT_OMX_IVCommon.h"
#include "ActCameraParameters.h"


namespace android
{

struct userToOMX_LUT
{
    const char * userDefinition;
    int         omxDefinition;
};

struct LUTtype
{
    int size;
    const userToOMX_LUT *Table;
};

const userToOMX_LUT isoUserToOMX[] =
{
    { ActCameraParameters::ISO_MODE_AUTO, 0 },
    { ActCameraParameters::ISO_MODE_100, 100 },
    { ActCameraParameters::ISO_MODE_200, 200 },
    { ActCameraParameters::ISO_MODE_400, 400 },
    { ActCameraParameters::ISO_MODE_800, 800 },
    { ActCameraParameters::ISO_MODE_1000, 1000 },
    { ActCameraParameters::ISO_MODE_1200, 1200 },
    { ActCameraParameters::ISO_MODE_1600, 1600 },
};

const userToOMX_LUT effects_UserToOMX [] =
{
    { CameraParameters::EFFECT_NONE, OMX_ImageFilterNone },
    { CameraParameters::EFFECT_NEGATIVE, OMX_ImageFilterNegative },
    { CameraParameters::EFFECT_SOLARIZE,  OMX_ImageFilterSolarize },
    { CameraParameters::EFFECT_SEPIA,  OMX_ImageFilterACTSEPIA },
    { CameraParameters::EFFECT_MONO,  OMX_ImageFilterACTBW },

};

const userToOMX_LUT whiteBal_UserToOMX [] =
{
    { CameraParameters::WHITE_BALANCE_AUTO, OMX_WhiteBalControlAuto },
    { CameraParameters::WHITE_BALANCE_DAYLIGHT, OMX_WhiteBalControlSunLight },
    { CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT, OMX_WhiteBalControlCloudy },
    { CameraParameters::WHITE_BALANCE_FLUORESCENT, OMX_WhiteBalControlFluorescent },
    { CameraParameters::WHITE_BALANCE_INCANDESCENT, OMX_WhiteBalControlIncandescent },
    { CameraParameters::WHITE_BALANCE_TWILIGHT, OMX_WhiteBalControlHorizon },
    { CameraParameters::WHITE_BALANCE_SHADE, OMX_WhiteBalControlShade },
};

const userToOMX_LUT antibanding_UserToOMX [] =
{
    { CameraParameters::ANTIBANDING_OFF, OMX_FlickerRejectionOff },
    { CameraParameters::ANTIBANDING_AUTO, OMX_FlickerRejectionAuto },
    { CameraParameters::ANTIBANDING_50HZ, OMX_FlickerRejection50 },
    { CameraParameters::ANTIBANDING_60HZ, OMX_FlickerRejection60 }
};

const userToOMX_LUT focus_UserToOMX [] =
{
    { CameraParameters::FOCUS_MODE_INFINITY, OMX_IMAGE_FocusControlOff },
    { CameraParameters::FOCUS_MODE_AUTO, OMX_IMAGE_FocusControlSingle },    
    { CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE, OMX_IMAGE_FocusControlAuto },
};

const userToOMX_LUT exposure_UserToOMX [] =
{
    { CameraParameters::SCENE_MODE_AUTO, OMX_ExposureControlAuto },
    { CameraParameters::SCENE_MODE_NIGHT, OMX_ExposureControlActHouse },
    //{ CameraParameters::SCENE_MODE_NIGHT, OMX_ExposureControlNight },
    { CameraParameters::SCENE_MODE_BEACH, OMX_ExposureControlBeach },
    { CameraParameters::SCENE_MODE_SNOW, OMX_ExposureControlSnow },
    { CameraParameters::SCENE_MODE_SPORTS, OMX_ExposureControlSports },
    //{ CameraParameters::SCENE_MODE_SUNSET, OMX_ExposureControlActSunset },
    { CameraParameters::SCENE_MODE_LANDSCAPE, OMX_ExposureControlActSunset },
    //{ CameraParameters::SCENE_MODE_LANDSCAPE, OMX_ExposureControlActLandscape },
    { CameraParameters::SCENE_MODE_ACTION, OMX_ExposureControlActAction },
    { CameraParameters::SCENE_MODE_PORTRAIT, OMX_ExposureControlActPortrait },
    { CameraParameters::SCENE_MODE_NIGHT_PORTRAIT, OMX_ExposureControlActNight_Portrait },
    { CameraParameters::SCENE_MODE_THEATRE, OMX_ExposureControlActTheatre },
    { CameraParameters::SCENE_MODE_STEADYPHOTO, OMX_ExposureControlActStreadyPhoto },
    { CameraParameters::SCENE_MODE_FIREWORKS, OMX_ExposureControlActFireworks },
    { CameraParameters::SCENE_MODE_PARTY, OMX_ExposureControlActParty },
    { CameraParameters::SCENE_MODE_CANDLELIGHT, OMX_ExposureControlActCandlelight },
    { CameraParameters::SCENE_MODE_BARCODE, OMX_ExposureControlActBarcode },
    { CameraParameters::SCENE_MODE_HDR, OMX_ExposureControlActHDR },
};

const userToOMX_LUT flash_UserToOMX [] =
{
    { CameraParameters::FLASH_MODE_OFF           ,OMX_IMAGE_FlashControlOff             },
    { CameraParameters::FLASH_MODE_ON            ,OMX_IMAGE_FlashControlOn              },
    { CameraParameters::FLASH_MODE_AUTO          ,OMX_IMAGE_FlashControlAuto            },
    { CameraParameters::FLASH_MODE_TORCH         ,OMX_IMAGE_FlashControlTorch           },
    { CameraParameters::FLASH_MODE_RED_EYE        ,OMX_IMAGE_FlashControlRedEyeReduction },

};

const LUTtype ExpLUT =
{
    sizeof(exposure_UserToOMX)/sizeof(exposure_UserToOMX[0]),
    exposure_UserToOMX
};

const LUTtype WBalLUT =
{
    sizeof(whiteBal_UserToOMX)/sizeof(whiteBal_UserToOMX[0]),
    whiteBal_UserToOMX
};

const LUTtype FlickerLUT =
{
    sizeof(antibanding_UserToOMX)/sizeof(antibanding_UserToOMX[0]),
    antibanding_UserToOMX
};

const LUTtype FlashLUT =
{
    sizeof(flash_UserToOMX)/sizeof(flash_UserToOMX[0]),
    flash_UserToOMX
};

const LUTtype EffLUT =
{
    sizeof(effects_UserToOMX)/sizeof(effects_UserToOMX[0]),
    effects_UserToOMX
};

const LUTtype FocusLUT =
{
    sizeof(focus_UserToOMX)/sizeof(focus_UserToOMX[0]),
    focus_UserToOMX
};

const LUTtype IsoLUT =
{
    sizeof(isoUserToOMX)/sizeof(isoUserToOMX[0]),
    isoUserToOMX
};

/*
*   class Gen3A_settings
*   stores the 3A settings
*   also defines the look up tables
*   for mapping settings from Hal to OMX
*/
class Gen3A_settings
{
public:

    int Exposure;
    int WhiteBallance;
    int Flicker;
    int SceneMode;
    int Effect;
    int Focus;
    int EVCompensation;
    int Contrast;
    int Saturation;
    int Sharpness;
    int ISO;
    int FlashMode;
    int Denoise;

    unsigned int Brightness;
    OMX_BOOL ExposureLock;
    OMX_BOOL FocusLock;
    OMX_BOOL WhiteBalanceLock;
};

/*
*   Flags raised when a setting is changed
*/
enum E3ASettingsFlags
{
    SetSceneMode            = 1 << 0,
    SetEVCompensation       = 1 << 1,
    SetWhiteBallance        = 1 << 2,
    SetFlicker              = 1 << 3,
    SetExposure             = 1 << 4,
    SetSharpness            = 1 << 5,
    SetBrightness           = 1 << 6,
    SetContrast             = 1 << 7,
    SetISO                  = 1 << 8,
    SetSaturation           = 1 << 9,
    SetEffect               = 1 << 10,
    SetFocus                = 1 << 11,
    SetExpMode              = 1 << 14,
    SetFlash                = 1 << 15,
    SetExpLock              = 1 << 16,
    SetWBLock               = 1 << 17,
    SetMeteringAreas        = 1 << 18,
    SetDenoise              = 1 << 19,

    E3aSettingMax,
    E3AsettingsAll = ( ((E3aSettingMax -1 ) << 1) -1 ) /// all possible flags raised
};

};

#endif //GENERAL_3A_SETTINGS_H
