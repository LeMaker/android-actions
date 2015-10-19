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
* @file OMX3A.cpp
*
* This file contains functionality for handling 3A configurations.
*
*/

#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"

#include "CameraConfigs.h"


#undef TRUE
#undef FALSE


#define METERING_AREAS_RANGE 0xFF

namespace android
{

/**
 *
 * BUGFIX:  Support for exposure value setting, fix write value to omx-cam.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXCameraAdapter::setParameters3A(const CameraParameters &params,
        BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    int mode = 0;
    const char *str = NULL;
    BaseCameraAdapter::AdapterState nextState;
    BaseCameraAdapter::getNextState(nextState);

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(m3ASettingsUpdateLock);

    if(strcmp(mCapabilities->get(CameraProperties::SCENE_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        str = params.get(CameraParameters::KEY_SCENE_MODE);
        mode = getLUTvalue_HALtoOMX( str, ExpLUT);
        if ( mFirstTimeInit || ((str != NULL) && ( mParameters3A.SceneMode != mode )) )
        {
            if ( 0 <= mode )
            {
                mParameters3A.SceneMode = mode;
                mPending3Asettings |= SetSceneMode;


                if((OMX_EXPOSURECONTROLTYPE)mParameters3A.SceneMode != OMX_ExposureControlAuto)
                {
                    if(strcmp(mCapabilities->get(CameraProperties::WHITEBALANCE_SUPPORTED), CameraParameters::TRUE) == 0)
                    {
                        mParameters3A.WhiteBallance = OMX_WhiteBalControlAuto;
                        mPending3Asettings |= (SetWhiteBallance);
                    }

                    if(strcmp(mCapabilities->get(CameraProperties::FLASH_SUPPORTED), CameraParameters::TRUE) == 0)
                    {
                        if((OMX_EXPOSURECONTROLTYPE)mParameters3A.SceneMode == (OMX_EXPOSURECONTROLTYPE)OMX_ExposureControlActHouse)
                        {
                            mParameters3A.FlashMode = OMX_IMAGE_FlashControlOn;
                            mPending3Asettings |= (SetFlash);

                        }
                        else if((OMX_EXPOSURECONTROLTYPE)mParameters3A.SceneMode == (OMX_EXPOSURECONTROLTYPE)OMX_ExposureControlActSunset)
                        {
                            mParameters3A.FlashMode = OMX_IMAGE_FlashControlOff;
                            mPending3Asettings |= (SetFlash);
                        }
                    }
                }

            }
        }
    }


    if(strcmp(mCapabilities->get(CameraProperties::WHITEBALANCE_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        if(mParameters3A.SceneMode == OMX_ExposureControlAuto)
        {
            str = params.get(CameraParameters::KEY_WHITE_BALANCE);
            mode = getLUTvalue_HALtoOMX( str, WBalLUT);
            if (mFirstTimeInit || ((str != NULL) && (mode != mParameters3A.WhiteBallance)))
            {

                mParameters3A.WhiteBallance = mode;
                CAMHAL_LOGDB("Whitebalance mode %d", mode);
                if ( 0 <= mParameters3A.WhiteBallance )
                {
                    mPending3Asettings |= SetWhiteBallance;
                }
            }
        }
    }
    if(strcmp(mCapabilities->get(CameraProperties::CONTRAST_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        if ( 0 <= params.getInt(ActCameraParameters::KEY_CONTRAST) )
        {
            if ( mFirstTimeInit ||
                ( (mParameters3A.Contrast  + CONTRAST_OFFSET) !=
                  params.getInt(ActCameraParameters::KEY_CONTRAST)) )
            {
                mParameters3A.Contrast = params.getInt(ActCameraParameters::KEY_CONTRAST) - CONTRAST_OFFSET;
                CAMHAL_LOGDB("Contrast %d", mParameters3A.Contrast);
                mPending3Asettings |= SetContrast;
            }
        }
    }
    if(strcmp(mCapabilities->get(CameraProperties::SHARPNESS_SUPPORTED), CameraParameters::TRUE) == 0)
    {

        if ( 0 <= params.getInt(ActCameraParameters::KEY_SHARPNESS) )
        {
            if ( mFirstTimeInit ||
                ((mParameters3A.Sharpness + SHARPNESS_OFFSET) !=
                 params.getInt(ActCameraParameters::KEY_SHARPNESS)))
            {
                mParameters3A.Sharpness = params.getInt(ActCameraParameters::KEY_SHARPNESS) - SHARPNESS_OFFSET;
                CAMHAL_LOGDB("Sharpness %d", mParameters3A.Sharpness);
                mPending3Asettings |= SetSharpness;
            }
        }
    }
    if(strcmp(mCapabilities->get(CameraProperties::SATURATION_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        if ( 0 <= params.getInt(ActCameraParameters::KEY_SATURATION) )
        {
            if ( mFirstTimeInit ||
                ((mParameters3A.Saturation + SATURATION_OFFSET) !=
                 params.getInt(ActCameraParameters::KEY_SATURATION)) )
            {
                mParameters3A.Saturation = params.getInt(ActCameraParameters::KEY_SATURATION) - SATURATION_OFFSET;
                CAMHAL_LOGDB("Saturation %d", mParameters3A.Saturation);
                mPending3Asettings |= SetSaturation;
            }
        }
    }
    if(strcmp(mCapabilities->get(CameraProperties::BRIGHTNESS_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        if ( 0 <= params.getInt(ActCameraParameters::KEY_BRIGHTNESS) )
        {
            if ( mFirstTimeInit ||
                (( mParameters3A.Brightness !=
                   ( unsigned int ) params.getInt(ActCameraParameters::KEY_BRIGHTNESS))) )
            {
                mParameters3A.Brightness = (unsigned)params.getInt(ActCameraParameters::KEY_BRIGHTNESS);
                CAMHAL_LOGDB("Brightness %d", mParameters3A.Brightness);
                mPending3Asettings |= SetBrightness;
            }
        }
    }

    if(strcmp(mCapabilities->get(CameraProperties::DENOISE_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        if ( 0 <= params.getInt(ActCameraParameters::KEY_DENOISE) )
        {
            if ( mFirstTimeInit ||
                ((mParameters3A.Denoise ) !=
                 params.getInt(ActCameraParameters::KEY_DENOISE)) )
            {
                mParameters3A.Denoise = params.getInt(ActCameraParameters::KEY_DENOISE);
                CAMHAL_LOGDB("Denoise %d", mParameters3A.Denoise);
                mPending3Asettings |= SetDenoise;
            }
        }
    }


    if(strcmp(mCapabilities->get(CameraProperties::ANTIBANDING_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        str = params.get(CameraParameters::KEY_ANTIBANDING);
        mode = getLUTvalue_HALtoOMX(str,FlickerLUT);
        if ( mFirstTimeInit || ( ( str != NULL ) && ( mParameters3A.Flicker != mode ) ))
        {
            mParameters3A.Flicker = mode;
            CAMHAL_LOGDB("Flicker %d", mParameters3A.Flicker);
            if ( 0 <= mParameters3A.Flicker )
            {
                mPending3Asettings |= SetFlicker;
            }
        }
    }

    if(strcmp(mCapabilities->get(CameraProperties::ISO_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        str = params.get(ActCameraParameters::KEY_ISO);
        mode = getLUTvalue_HALtoOMX(str, IsoLUT);
        CAMHAL_LOGVB("ISO mode arrived in HAL : %s", str);
        if ( mFirstTimeInit || (  ( str != NULL ) && ( mParameters3A.ISO != mode )) )
        {
            mParameters3A.ISO = mode;
            CAMHAL_LOGDB("ISO %d", mParameters3A.ISO);
            if ( 0 <= mParameters3A.ISO )
            {
                mPending3Asettings |= SetISO;
            }
        }
    }

    if((mParameters3A.SceneMode == OMX_ExposureControlAuto) && strcmp(mCapabilities->get(CameraProperties::FOCUS_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        str = params.get(CameraParameters::KEY_FOCUS_MODE);
        mode = getLUTvalue_HALtoOMX(str, FocusLUT);
        if ( (mFirstTimeInit || ((str != NULL) && (mParameters3A.Focus != mode))))
        {
            mPending3Asettings |= SetFocus;

            mParameters3A.Focus = mode;

            CAMHAL_LOGDB("Focus %x", mParameters3A.Focus);
        }
    }

    str = params.get(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    if ( mFirstTimeInit ||
        (( str != NULL ) &&
         (mParameters3A.EVCompensation !=
          params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION))))
    {
        CAMHAL_LOGDB("Setting EV Compensation to %d",
            params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION));

        //ActionsCode(author:liuyiguang, change_code)
        //mParameters3A.EVCompensation = params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
        mParameters3A.EVCompensation = params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION) * INT_ALIGN;
        mPending3Asettings |= SetEVCompensation;
    }

    if((mParameters3A.SceneMode == OMX_ExposureControlAuto) && strcmp(mCapabilities->get(CameraProperties::CameraProperties::FLASH_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        str = params.get(CameraParameters::KEY_FLASH_MODE);
        mode = getLUTvalue_HALtoOMX( str, FlashLUT);
        if (  mFirstTimeInit || (( str != NULL ) && ( mParameters3A.FlashMode != mode )) )
        {
            if ( 0 <= mode )
            {
                mParameters3A.FlashMode = mode;
                mPending3Asettings |= SetFlash;
            }
        }
    }

    CAMHAL_LOGVB("Flash Setting %s", str);
    CAMHAL_LOGVB("FlashMode %d", mParameters3A.FlashMode);

    if(strcmp(mCapabilities->get(CameraProperties::EFFECT_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        str = params.get(CameraParameters::KEY_EFFECT);
        mode = getLUTvalue_HALtoOMX( str, EffLUT);
        if (  mFirstTimeInit || (( str != NULL ) && ( mParameters3A.Effect != mode )) )
        {
            mParameters3A.Effect = mode;
            CAMHAL_LOGDB("Effect %d", mParameters3A.Effect);
            if ( 0 <= mParameters3A.Effect )
            {
                mPending3Asettings |= SetEffect;
            }
        }
    }

    str = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED);
    if ( (str != NULL) && (!strcmp(str, "true")) )
    {
        OMX_BOOL lock = OMX_FALSE;
        mUserSetExpLock = OMX_FALSE;
        str = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
        if ( (strcmp(str, "true")) == 0)
        {
            CAMHAL_LOGVA("Locking Exposure");
            lock = OMX_TRUE;
            mUserSetExpLock = OMX_TRUE;
        }
        else
        {
            CAMHAL_LOGVA("UnLocking Exposure");
        }

        if (mParameters3A.ExposureLock != lock)
        {
            mParameters3A.ExposureLock = lock;
            CAMHAL_LOGDB("ExposureLock %d", lock);
            mPending3Asettings |= SetExpLock;
        }
    }

    str = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED);
    if ( (str != NULL) && (!strcmp(str, "true")) )
    {
        OMX_BOOL lock = OMX_FALSE;
        mUserSetWbLock = OMX_FALSE;
        str = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);
        if ( (strcmp(str, "true")) == 0)
        {
            CAMHAL_LOGVA("Locking WhiteBalance");
            lock = OMX_TRUE;
            mUserSetWbLock = OMX_TRUE;
        }
        else
        {
            CAMHAL_LOGVA("UnLocking WhiteBalance");
        }
        if (mParameters3A.WhiteBalanceLock != lock)
        {
            mParameters3A.WhiteBalanceLock = lock;
            CAMHAL_LOGDB("WhiteBalanceLock %d", lock);
            mPending3Asettings |= SetWBLock;
        }
    }

    if(strcmp(mCapabilities->get(CameraProperties::AUTO_FOCUS_LOCK_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        str = params.get(ActCameraParameters::KEY_AUTO_FOCUS_LOCK);
        if (str && (strcmp(str, CameraParameters::TRUE) == 0) && (mParameters3A.FocusLock != OMX_TRUE))
        {
            CAMHAL_LOGVA("Locking Focus");
            mParameters3A.FocusLock = OMX_TRUE;
            setFocusLock(mParameters3A);
        }
        else if (str && (strcmp(str, CameraParameters::FALSE) == 0) && (mParameters3A.FocusLock != OMX_FALSE))
        {
            CAMHAL_LOGVA("UnLocking Focus");
            mParameters3A.FocusLock = OMX_FALSE;
            setFocusLock(mParameters3A);
        }
    }

#if 0
    str = params.get(CameraParameters::KEY_METERING_AREAS);
    if ( (str != NULL) )
    {
        size_t MAX_METERING_AREAS;
        Vector< sp<CameraArea> > tempAreas;


        MAX_METERING_AREAS = atoi(params.get(CameraParameters::KEY_MAX_NUM_METERING_AREAS));

        Mutex::Autolock lock(mMeteringAreasLock);

        ret = CameraArea::parseAreas(str, ( strlen(str) + 1 ), tempAreas);

        CAMHAL_LOGVB("areAreasDifferent? = %d",
            CameraArea::areAreasDifferent(mMeteringAreas, tempAreas));

        if ( (NO_ERROR == ret) && CameraArea::areAreasDifferent(mMeteringAreas, tempAreas) )
        {
            mMeteringAreas.clear();
            mMeteringAreas = tempAreas;

            if ( MAX_METERING_AREAS >= mMeteringAreas.size() )
            {
                CAMHAL_LOGDB("Setting Metering Areas %s",
                    params.get(CameraParameters::KEY_METERING_AREAS));

                mPending3Asettings |= SetMeteringAreas;
            }
            else
            {
                CAMHAL_LOGEB("Metering areas supported %d, metering areas set %d",
                    MAX_METERING_AREAS, mMeteringAreas.size());
                ret = -EINVAL;
            }
        }

    }
#endif
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

int OMXCameraAdapter::getLUTvalue_HALtoOMX(const char * HalValue, LUTtype LUT)
{
    int LUTsize = LUT.size;
    if( HalValue )
        for(int i = 0; i < LUTsize; i++)
            if( 0 == strcmp(LUT.Table[i].userDefinition, HalValue) )
                return LUT.Table[i].omxDefinition;

    return -ENOENT;
}

const char* OMXCameraAdapter::getLUTvalue_OMXtoHAL(int OMXValue, LUTtype LUT)
{
    int LUTsize = LUT.size;
    for(int i = 0; i < LUTsize; i++)
        if( LUT.Table[i].omxDefinition == OMXValue )
        {
            return LUT.Table[i].userDefinition;
        }

    return NULL;
}

status_t OMXCameraAdapter::apply3ADefaults(Gen3A_settings &Gen3A)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Gen3A.Effect = getLUTvalue_HALtoOMX(getDefaultValue(mCapabilities, CameraProperties::EFFECT), EffLUT);
    if(strcmp(mCapabilities->get(CameraProperties::CameraProperties::EFFECT_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        ret |= setEffect(Gen3A);
    }

    Gen3A.FlashMode = getLUTvalue_HALtoOMX(getDefaultValue(mCapabilities, CameraProperties::FLASH_MODE), FlashLUT);
    if(strcmp(mCapabilities->get(CameraProperties::CameraProperties::FLASH_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        ret |= setFlashMode(Gen3A);
    }

    Gen3A.SceneMode = getLUTvalue_HALtoOMX(getDefaultValue(mCapabilities, CameraProperties::SCENE_MODE), ExpLUT);
    if(strcmp(mCapabilities->get(CameraProperties::CameraProperties::SCENE_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        ret |= setScene(Gen3A);
    }

    Gen3A.EVCompensation = atoi(OMXCameraAdapter::DEFAULT_EV_COMPENSATION);
    ret |= setEVCompensation(Gen3A);

    Gen3A.Focus = getLUTvalue_HALtoOMX(getDefaultValue(mCapabilities, CameraProperties::FOCUS_MODE), FocusLUT);
    if(strcmp(mCapabilities->get(CameraProperties::FOCUS_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        ret |= setFocusMode(Gen3A);
    }

    Gen3A.ISO = getLUTvalue_HALtoOMX(OMXCameraAdapter::DEFAULT_ISO_MODE, IsoLUT);
    if(strcmp(mCapabilities->get(CameraProperties::ISO_SUPPORTED), CameraParameters::TRUE) == 0) 
    {
        ret |= setISO(Gen3A);
    }

    Gen3A.Flicker = getLUTvalue_HALtoOMX(getDefaultValue(mCapabilities, CameraProperties::ANTIBANDING), FlickerLUT);
    if(strcmp(mCapabilities->get(CameraProperties::ANTIBANDING_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        ret |= setFlicker(Gen3A);
    }

    Gen3A.Brightness = atoi(OMXCameraAdapter::DEFAULT_BRIGHTNESS);
    if(strcmp(mCapabilities->get(CameraProperties::BRIGHTNESS_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        ret |= setBrightness(Gen3A);
    }

    Gen3A.Saturation = atoi(OMXCameraAdapter::DEFAULT_SATURATION) - SATURATION_OFFSET;
    if(strcmp(mCapabilities->get(CameraProperties::SATURATION_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        ret |= setSaturation(Gen3A);
    }

    Gen3A.Sharpness = atoi(OMXCameraAdapter::DEFAULT_SHARPNESS) - SHARPNESS_OFFSET;
    if(strcmp(mCapabilities->get(CameraProperties::SHARPNESS_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        ret |= setSharpness(Gen3A);
    }

    Gen3A.Contrast = atoi(OMXCameraAdapter::DEFAULT_CONTRAST) - CONTRAST_OFFSET;
    if(strcmp(mCapabilities->get(CameraProperties::CONTRAST_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        ret |= setContrast(Gen3A);
    }

    Gen3A.WhiteBallance = getLUTvalue_HALtoOMX(getDefaultValue(mCapabilities, CameraProperties::WHITEBALANCE), WBalLUT);
    if(strcmp(mCapabilities->get(CameraProperties::WHITEBALANCE_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        ret |= setWBMode(Gen3A);
    }

    Gen3A.ExposureLock = OMX_FALSE;
    if(strcmp(mCapabilities->get(CameraProperties::AUTO_EXPOSURE_LOCK_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        ret |= setExposureLock(Gen3A);
    }

    Gen3A.WhiteBalanceLock = OMX_FALSE;
    if(strcmp(mCapabilities->get(CameraProperties::AUTO_WHITEBALANCE_LOCK), CameraParameters::TRUE) == 0)
    {
        ret |= setWhiteBalanceLock(Gen3A);
    }

    Gen3A.Denoise= atoi(OMXCameraAdapter::DEFAULT_DENOISE) ;
    if(strcmp(mCapabilities->get(CameraProperties::DENOISE_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        ret |= setDenoise(Gen3A);
    }

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

status_t OMXCameraAdapter::setExposureMode(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSURECONTROLTYPE exp;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&exp, OMX_CONFIG_EXPOSURECONTROLTYPE);
    exp.nPortIndex = OMX_ALL;
    exp.eExposureControl = (OMX_EXPOSURECONTROLTYPE)Gen3A.Exposure;

    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonExposure,
                            &exp);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring exposure mode 0x%x", eError);
    }
    else
    {
        CAMHAL_LOGDA("Camera exposure mode configured successfully");
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}


/**
 *
 * BUGFIX:  Fix the closing problem for flash light.
 *
 ************************************
 *
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXCameraAdapter::setFlashMode(Gen3A_settings& Gen3A)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_PARAM_FLASHCONTROLTYPE flash;

    LOG_FUNCTION_NAME;

#if 0
    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&flash, OMX_IMAGE_PARAM_FLASHCONTROLTYPE);
    flash.nPortIndex = OMX_CAMERA_PORT_IMAGE_OUT_IMAGE;

    flash.eFlashControl = ( OMX_IMAGE_FLASHCONTROLTYPE ) Gen3A.FlashMode;

    CAMHAL_LOGDB("Configuring flash mode 0x%x", flash.eFlashControl);
    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE) OMX_IndexConfigFlashControl,
                            &flash);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring flash mode 0x%x", eError);
    }
    else
    {
        CAMHAL_LOGDA("Camera flash mode configured successfully");
    }
#else
    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&flash, OMX_IMAGE_PARAM_FLASHCONTROLTYPE);
    flash.nPortIndex = OMX_CAMERA_PORT_IMAGE_OUT_IMAGE;

    flash.eFlashControl = ( OMX_IMAGE_FLASHCONTROLTYPE )OMX_IMAGE_FlashControlOff;

    CAMHAL_LOGDB("Configuring flash mode 0x%x", flash.eFlashControl);
    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE) OMX_IndexConfigFlashControl,
                            &flash);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring flash mode 0x%x", eError);
    }
    else
    {
        CAMHAL_LOGDA("Camera flash mode configured successfully");
    }


    if( ( OMX_IMAGE_FLASHCONTROLTYPE ) Gen3A.FlashMode == OMX_IMAGE_FlashControlTorch)
    {
        //ActionsCode(author:liuyiguang, add_code)
        mFlashStrobed = true;
        startFlashStrobe();
    }
    else
    {
        stopFlashStrobe();
        //ActionsCode(author:liuyiguang, add_code)
        mFlashStrobed = false;
    }

#endif

    LOG_FUNCTION_NAME_EXIT;
    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::getFlashMode(Gen3A_settings& Gen3A)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_PARAM_FLASHCONTROLTYPE flash;

    LOG_FUNCTION_NAME;

#if 0
    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&flash, OMX_IMAGE_PARAM_FLASHCONTROLTYPE);
    flash.nPortIndex = OMX_CAMERA_PORT_IMAGE_OUT_IMAGE;

    eError =  OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE) OMX_IndexConfigFlashControl,
                            &flash);

    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while getting flash mode 0x%x", eError);
    }
    else
    {
        Gen3A.FlashMode = flash.eFlashControl;
        CAMHAL_LOGDB("Gen3A.FlashMode 0x%x", Gen3A.FlashMode);
    }
#else
#endif

    LOG_FUNCTION_NAME_EXIT;
    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setFocusMode(Gen3A_settings& Gen3A)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    if ((state & AF_ACTIVE) == 0) 
    { 

        if (mIternalRecordingHint) {
            CAMHAL_LOGDA("dv: set focus mode off");
            ret = setFocusMode(OMX_IMAGE_FocusControlOff);
        }
        else {
            //continous auto focus/ single focus/ focus off mode is supported
            //single mode must be set when autofocus is callbed, do not set when setparameter
            if(Gen3A.Focus == OMX_IMAGE_FocusControlAuto
                || Gen3A.Focus == OMX_IMAGE_FocusControlOff)
            {
                    CAMHAL_LOGDB("dc: set focus mode %d @ set3A_Param except single mode which is set @ doAutoFocus",(int)(Gen3A.Focus));
                    ret = setFocusMode((OMX_IMAGE_FOCUSCONTROLTYPE)Gen3A.Focus);
            }
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::getFocusMode(Gen3A_settings& Gen3A)
{
    status_t ret = NO_ERROR;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focus;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::setScene(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSURECONTROLTYPE exp;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

#ifndef CAMERA_ANDROID16
#ifdef CAMERA_HDR
    if((OMX_EXPOSURECONTROLTYPE)Gen3A.SceneMode == (OMX_EXPOSURECONTROLTYPE)OMX_ExposureControlActHDR)
    {
        OMX_ACT_CONFIG_HDR_EVParams hdr;

        int ev_min = (int)(CameraConfigs::getHdrMin() * ((float)(1<<Q16_OFFSET)));
        int ev_max = (int)(CameraConfigs::getHdrMin() * ((float)(1<<Q16_OFFSET)));

        OMX_INIT_STRUCT_PTR (&hdr, OMX_ACT_CONFIG_HDR_EVParams);
        hdr.nPortIndex = OMX_CAMERA_PORT_IMAGE_OUT_IMAGE;
        hdr.bAutoMode = OMX_TRUE;
        hdr.nHighLightEV =(ev_max);
        hdr.nNormalLightEV =0;
        hdr.nLowLightEV =(ev_min);


        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
            (OMX_INDEXTYPE)OMX_ACT_IndexConfigHDRParam,
            &hdr);
        if ( OMX_ErrorNone != eError )
        {
            CAMHAL_LOGEB("Error while configuring hdr param, eError=0x%x", eError);
        }
        else
        {
            CAMHAL_LOGDA("Camera hdr param configured successfully");
        }
    }
#endif
#endif

    OMX_INIT_STRUCT_PTR (&exp, OMX_CONFIG_EXPOSURECONTROLTYPE);
    exp.nPortIndex = OMX_ALL;
    exp.eExposureControl = (OMX_EXPOSURECONTROLTYPE)Gen3A.SceneMode;

    eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
        OMX_IndexConfigCommonExposure,
        &exp);
        
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring exposure mode 0x%x, eError=0x%x", Gen3A.SceneMode, eError);
    }
    else
    {
        CAMHAL_LOGDA("Camera exposure mode configured successfully");
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setEVCompensation(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE expValues;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&expValues, OMX_CONFIG_EXPOSUREVALUETYPE);
    expValues.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                   OMX_IndexConfigCommonExposureValue,
                   &expValues);
    CAMHAL_LOGDB("old EV Compensation for OMX = 0x%x", (int)expValues.xEVCompensation);
    CAMHAL_LOGDB("EV Compensation for HAL = %d", Gen3A.EVCompensation);

    expValues.xEVCompensation = ( Gen3A.EVCompensation * ( 1 << Q16_OFFSET ) )  / 10;
    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonExposureValue,
                            &expValues);
    CAMHAL_LOGDB("new EV Compensation for OMX = 0x%x", (int)expValues.xEVCompensation);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring EV Compensation 0x%x error = 0x%x",
                     ( unsigned int ) expValues.xEVCompensation,
                     eError);
    }
    else
    {
        CAMHAL_LOGDB("EV Compensation 0x%x configured successfully",
                     ( unsigned int ) expValues.xEVCompensation);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::getEVCompensation(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE expValues;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&expValues, OMX_CONFIG_EXPOSUREVALUETYPE);
    expValues.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                  OMX_IndexConfigCommonExposureValue,
                  &expValues);

    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while getting EV Compensation error = 0x%x", eError);
    }
    else
    {
        Gen3A.EVCompensation = (10 * expValues.xEVCompensation) / (1 << Q16_OFFSET);
        CAMHAL_LOGDB("Gen3A.EVCompensation 0x%x", Gen3A.EVCompensation);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setWBMode(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_WHITEBALCONTROLTYPE wb;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&wb, OMX_CONFIG_WHITEBALCONTROLTYPE);
    wb.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    wb.eWhiteBalControl = ( OMX_WHITEBALCONTROLTYPE ) Gen3A.WhiteBallance;

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonWhiteBalance,
                            &wb);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring Whitebalance mode 0x%x error = 0x%x",
                     ( unsigned int ) wb.eWhiteBalControl,
                     eError);
    }
    else
    {
        CAMHAL_LOGDB("Whitebalance mode 0x%x configured successfully",
                     ( unsigned int ) wb.eWhiteBalControl);
    }

    LOG_FUNCTION_NAME_EXIT;

    return eError;
}

status_t OMXCameraAdapter::getWBMode(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_WHITEBALCONTROLTYPE wb;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&wb, OMX_CONFIG_WHITEBALCONTROLTYPE);
    wb.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                           OMX_IndexConfigCommonWhiteBalance,
                           &wb);

    if (OMX_ErrorNone != eError)
    {
        CAMHAL_LOGEB("Error while getting Whitebalance mode error = 0x%x", eError);
    }
    else
    {
        Gen3A.WhiteBallance = wb.eWhiteBalControl;
        CAMHAL_LOGDB("Gen3A.WhiteBallance 0x%x", Gen3A.WhiteBallance);
    }

    LOG_FUNCTION_NAME_EXIT;

    return eError;
}

status_t OMXCameraAdapter::setFlicker(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_FLICKERREJECTIONTYPE flicker;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&flicker, OMX_CONFIG_FLICKERREJECTIONTYPE);
    flicker.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    flicker.eFlickerRejection = static_cast<OMX_FLICKERREJECTIONTYPE>(Gen3A.Flicker);

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_IndexConfigFlickerRejection,
                            &flicker );
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring Flicker mode 0x%x error = 0x%x",
                     ( unsigned int ) flicker.eFlickerRejection,
                     eError);
    }
    else
    {
        CAMHAL_LOGDB("Flicker mode 0x%x configured successfully",
                     ( unsigned int ) flicker.eFlickerRejection);
    }

    LOG_FUNCTION_NAME_EXIT;
    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setBrightness(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_BRIGHTNESSTYPE brightness;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&brightness, OMX_CONFIG_BRIGHTNESSTYPE);
    brightness.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    brightness.nBrightness = Gen3A.Brightness;

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonBrightness,
                            &brightness);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring Brightness 0x%x error = 0x%x",
                     ( unsigned int ) brightness.nBrightness,
                     eError);
    }
    else
    {
        CAMHAL_LOGDB("Brightness 0x%x configured successfully",
                     ( unsigned int ) brightness.nBrightness);
    }

    LOG_FUNCTION_NAME_EXIT;
    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setContrast(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_CONTRASTTYPE contrast;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&contrast, OMX_CONFIG_CONTRASTTYPE);
    contrast.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    contrast.nContrast = Gen3A.Contrast;

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonContrast,
                            &contrast);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring Contrast 0x%x error = 0x%x",
                     ( unsigned int ) contrast.nContrast,
                     eError);
    }
    else
    {
        CAMHAL_LOGDB("Contrast 0x%x configured successfully",
                     ( unsigned int ) contrast.nContrast);
    }

    LOG_FUNCTION_NAME_EXIT;
    return eError;
}

status_t OMXCameraAdapter::setSharpness(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_SHARPNESSTYPE procSharpness;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&procSharpness, OMX_SHARPNESSTYPE);
    procSharpness.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    procSharpness.nSharpness= Gen3A.Sharpness;

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_IndexConfigSharpness,
                            &procSharpness);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring Sharpness 0x%x error = 0x%x",
                     ( unsigned int ) procSharpness.nSharpness,
                     eError);
    }
    else
    {
        CAMHAL_LOGDB("Sharpness 0x%x configured successfully",
                     ( unsigned int ) procSharpness.nSharpness);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::getSharpness(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_SHARPNESSTYPE procSharpness;

    LOG_FUNCTION_NAME;

    if (0/*OMX_StateInvalid*/ == mComponentState)
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&procSharpness, OMX_SHARPNESSTYPE);
    procSharpness.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                           (OMX_INDEXTYPE)OMX_IndexConfigSharpness,
                           &procSharpness);

    if (OMX_ErrorNone != eError)
    {
        CAMHAL_LOGEB("Error while configuring Sharpness error = 0x%x", eError);
    }
    else
    {
        Gen3A.Sharpness = procSharpness.nSharpness;
        CAMHAL_LOGDB("Gen3A.Sharpness 0x%x", Gen3A.Sharpness);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setSaturation(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_SATURATIONTYPE saturation;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&saturation, OMX_CONFIG_SATURATIONTYPE);
    saturation.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    saturation.nSaturation = Gen3A.Saturation;

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonSaturation,
                            &saturation);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring Saturation 0x%x error = 0x%x",
                     ( unsigned int ) saturation.nSaturation,
                     eError);
    }
    else
    {
        CAMHAL_LOGDB("Saturation 0x%x configured successfully",
                     ( unsigned int ) saturation.nSaturation);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::getSaturation(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_SATURATIONTYPE saturation;

    LOG_FUNCTION_NAME;

    if (0/*OMX_StateInvalid*/ == mComponentState)
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&saturation, OMX_CONFIG_SATURATIONTYPE);
    saturation.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonSaturation,
                            &saturation);

    if (OMX_ErrorNone != eError)
    {
        CAMHAL_LOGEB("Error while getting Saturation error = 0x%x", eError);
    }
    else
    {
        Gen3A.Saturation = saturation.nSaturation;
        CAMHAL_LOGDB("Gen3A.Saturation 0x%x", Gen3A.Saturation);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setISO(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE expValues;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&expValues, OMX_CONFIG_EXPOSUREVALUETYPE);
    expValues.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                   OMX_IndexConfigCommonExposureValue,
                   &expValues);

    if( 0 == Gen3A.ISO )
    {
        expValues.bAutoSensitivity = OMX_TRUE;
    }
    else
    {
        expValues.bAutoSensitivity = OMX_FALSE;
        expValues.nSensitivity = Gen3A.ISO;
    }

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonExposureValue,
                            &expValues);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring ISO 0x%x error = 0x%x",
                     ( unsigned int ) expValues.nSensitivity,
                     eError);
    }
    else
    {
        CAMHAL_LOGDB("ISO 0x%x configured successfully",
                     ( unsigned int ) expValues.nSensitivity);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::getISO(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_EXPOSUREVALUETYPE expValues;

    LOG_FUNCTION_NAME;

    if (0/*OMX_StateInvalid*/ == mComponentState)
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&expValues, OMX_CONFIG_EXPOSUREVALUETYPE);
    expValues.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                   OMX_IndexConfigCommonExposureValue,
                   &expValues);

    if (OMX_ErrorNone != eError)
    {
        CAMHAL_LOGEB("Error while getting ISO error = 0x%x", eError);
    }
    else
    {
        Gen3A.ISO = expValues.nSensitivity;
        CAMHAL_LOGDB("Gen3A.ISO %d", Gen3A.ISO);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setDenoise(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE denoise;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&denoise, OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE);
    denoise.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    denoise.nLevel= Gen3A.Denoise;
    denoise.bAuto = OMX_FALSE;

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_ACT_IndexConfigImageDeNoiseLevel,
                            &denoise);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring denoise 0x%x error = 0x%x",
                     ( unsigned int ) denoise.nLevel,
                     eError);
    }
    else
    {
        CAMHAL_LOGDB("denoise 0x%x configured successfully",
                     ( unsigned int ) denoise.nLevel);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::getDenoise(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE denoise;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&denoise, OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE);
    denoise.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_ACT_IndexConfigImageDeNoiseLevel,
                            &denoise);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring denoise 0x%x error = 0x%x",
                     ( unsigned int ) denoise.nLevel,
                     eError);
    }
    else
    {
        Gen3A.Denoise = denoise.nLevel;
        CAMHAL_LOGDB("denoise 0x%x configured successfully",
                     ( unsigned int ) denoise.nLevel);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setEffect(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_IMAGEFILTERTYPE effect;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&effect, OMX_CONFIG_IMAGEFILTERTYPE);
    effect.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    effect.eImageFilter = (OMX_IMAGEFILTERTYPE ) Gen3A.Effect;

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigCommonImageFilter,
                            &effect);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring Effect 0x%x error = 0x%x",
                     ( unsigned int )  effect.eImageFilter,
                     eError);
    }
    else
    {
        CAMHAL_LOGDB("Effect 0x%x configured successfully",
                     ( unsigned int )  effect.eImageFilter);
    }

    LOG_FUNCTION_NAME_EXIT;
    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setWhiteBalanceLock(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_LOCKTYPE lock;

    LOG_FUNCTION_NAME

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&lock, OMX_IMAGE_CONFIG_LOCKTYPE);
    lock.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    if(Gen3A.WhiteBalanceLock)
    {
        lock.eImageLock= OMX_IMAGE_LockImmediate;
    }
    else
    {
        lock.eImageLock= OMX_IMAGE_LockOff;
    }

    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_IndexConfigImageWhiteBalanceLock,
                            &lock);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring WhiteBalance Lock error = 0x%x", eError);
    }
    else
    {
        CAMHAL_LOGDB("WhiteBalance Lock configured successfully %d ", lock.eImageLock);
    }
    LOG_FUNCTION_NAME_EXIT
    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setExposureLock(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_LOCKTYPE lock;

    LOG_FUNCTION_NAME

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&lock, OMX_IMAGE_CONFIG_LOCKTYPE);
    lock.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    if(Gen3A.ExposureLock)
    {
        lock.eImageLock= OMX_IMAGE_LockImmediate;
    }
    else
    {
        lock.eImageLock= OMX_IMAGE_LockOff;
    }
    eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_IndexConfigImageExposureLock,
                            &lock);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring Exposure Lock error = 0x%x", eError);
    }
    else
    {
        CAMHAL_LOGDB("Exposure Lock configured successfully %d ", lock.eImageLock);
    }
    LOG_FUNCTION_NAME_EXIT
    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setFocusLock(Gen3A_settings& Gen3A)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_LOCKTYPE lock;

    LOG_FUNCTION_NAME

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&lock, OMX_IMAGE_CONFIG_LOCKTYPE);
    lock.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    if(Gen3A.FocusLock)
    {
        lock.eImageLock= OMX_IMAGE_LockImmediate;
    }
    else
    {
        lock.eImageLock= OMX_IMAGE_LockOff;
    }
    eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                           (OMX_INDEXTYPE)OMX_IndexConfigImageFocusLock,
                           &lock);

    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while configuring Focus Lock error = 0x%x", eError);
    }
    else
    {
        CAMHAL_LOGDB("Focus Lock configured successfully %d ", lock.eImageLock);
    }

    LOG_FUNCTION_NAME_EXIT

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::set3ALock(OMX_BOOL toggleExp, OMX_BOOL toggleWb, OMX_BOOL toggleFocus)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_LOCKTYPE lock;
    OMX_BOOL value;

    LOG_FUNCTION_NAME

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&lock, OMX_IMAGE_CONFIG_LOCKTYPE);
    lock.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    mParameters3A.ExposureLock = toggleExp;
    mParameters3A.FocusLock = toggleFocus;
    mParameters3A.WhiteBalanceLock = toggleWb;

    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_IndexConfigImageExposureLock,
                            &lock);

    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error GetConfig Exposure Lock error = 0x%x", eError);
        goto EXIT;
    }
    else
    {
        const char *lock_state_exp = toggleExp ? CameraParameters::TRUE : CameraParameters::FALSE;

        CAMHAL_LOGDA("Exposure Lock GetConfig successfull");
        value = (lock.eImageLock == OMX_IMAGE_LockOff)? OMX_FALSE: OMX_TRUE;
        /* Apply locks only when not applied already */
        if ( value  != toggleExp )
        {
            setExposureLock(mParameters3A);
        }

        mParams.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, lock_state_exp);
    }

    OMX_INIT_STRUCT_PTR (&lock, OMX_IMAGE_CONFIG_LOCKTYPE);
    lock.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_IndexConfigImageFocusLock,
                            &lock);

    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error GetConfig Focus Lock error = 0x%x", eError);
        goto EXIT;
    }
    else
    {
        CAMHAL_LOGDB("Focus Lock GetConfig successfull bLock(%d)", lock.eImageLock);

        /* Apply locks only when not applied already */
        value = (lock.eImageLock == OMX_IMAGE_LockOff)? OMX_FALSE: OMX_TRUE;
        if ( value  != toggleFocus )
        {
            setFocusLock(mParameters3A);
        }
    }

    OMX_INIT_STRUCT_PTR (&lock, OMX_IMAGE_CONFIG_LOCKTYPE);
    lock.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    eError = OMX_GetConfig( mCameraAdapterParameters.mHandleComp,
                            (OMX_INDEXTYPE)OMX_IndexConfigImageWhiteBalanceLock,
                            &lock);

    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error GetConfig WhiteBalance Lock error = 0x%x", eError);
        goto EXIT;
    }
    else
    {
        const char *lock_state_wb = toggleWb ? CameraParameters::TRUE : CameraParameters::FALSE;
        CAMHAL_LOGDA("WhiteBalance Lock GetConfig successfull");

        /* Apply locks only when not applied already */
        value = (lock.eImageLock == OMX_IMAGE_LockOff)? OMX_FALSE: OMX_TRUE;
        if ( value != toggleWb )
        {
            setWhiteBalanceLock(mParameters3A);
        }

        mParams.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, lock_state_wb);
    }
    LOG_FUNCTION_NAME_EXIT;
EXIT:
    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setMeteringAreas(Gen3A_settings& Gen3A)
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t OMXCameraAdapter::apply3Asettings( Gen3A_settings& Gen3A )
{
    status_t ret = NO_ERROR;
    unsigned int currSett; // 32 bit
    int portIndex;
    bool captureState = false;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(m3ASettingsUpdateLock);

    if(mRecording)
    {
        return ret;
    }

    ret  = canSetCapture3A(captureState);
    if((ret != NO_ERROR) || (captureState == true))
    {
        return ret;
    }


    for( currSett = 1; currSett < E3aSettingMax; currSett <<= 1)
    {
        if( currSett & mPending3Asettings )
        {
            switch( currSett )
            {
            case SetSceneMode:
                {
                    ret |= setScene(Gen3A);
                    break;
                }
            case SetEVCompensation:
                {
                    ret |= setEVCompensation(Gen3A);
                    break;
                }

            case SetWhiteBallance:
                {
                    ret |= setWBMode(Gen3A);
                    break;
                }

            case SetFlicker:
                {
                    ret |= setFlicker(Gen3A);
                    break;
                }

            case SetBrightness:
                {
                    ret |= setBrightness(Gen3A);
                    break;
                }

            case SetContrast:
                {
                    ret |= setContrast(Gen3A);
                    break;
                }

            case SetSharpness:
                {
                    ret |= setSharpness(Gen3A);
                    break;
                }

            case SetSaturation:
                {
                    ret |= setSaturation(Gen3A);
                    break;
                }

            case SetISO:
                {
                    ret |= setISO(Gen3A);
                    break;
                }

            case SetEffect:
                {
                    ret |= setEffect(Gen3A);
                    break;
                }

            case SetFocus:
                {
                    ret |= setFocusMode(Gen3A);
                    break;
                }

            case SetFlash:
                {
                    ret |= setFlashMode(Gen3A);
                    break;
                }

            case SetExpLock:
                {
                    ret |= setExposureLock(Gen3A);
                    break;
                }

            case SetWBLock:
                {
                    ret |= setWhiteBalanceLock(Gen3A);
                    break;
                }
            case SetMeteringAreas:
                {
                    ret |= setMeteringAreas(Gen3A);
                }
                break;

            case SetDenoise:
                {
                    ret |= setDenoise(Gen3A);
                    break;
                }
            default:
                CAMHAL_LOGEB("this setting (0x%x) is still not supported in CameraAdapter ",
                             currSett);
                break;
            }
            mPending3Asettings &= ~currSett;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

};
