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
* @file OMXFocus.cpp
*
* This file contains functionality for handling focus configurations.
*
*/


#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"

#define AF_TIMEOUT 5000000 //5 seconds timeout
#define FOCUS_RATIO_THREADSHOLD 80
#define PRE_FLASHLIGHT_ON 1000000 //1 seconds timeout

#undef TRUE
#undef FALSE

namespace android
{

status_t OMXCameraAdapter::setParametersFocus(const CameraParameters &params,
        BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    const char *str = NULL;
    Vector< sp<CameraArea> > tempAreas;
    size_t MAX_FOCUS_AREAS;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mFocusAreasLock);

    str = params.get(CameraParameters::KEY_FOCUS_AREAS);

    MAX_FOCUS_AREAS = atoi(params.get(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS));

    if ( NULL != str )
    {
        ret = CameraArea::parseAreas(str, ( strlen(str) + 1 ), tempAreas);
    }

    if ( (NO_ERROR == ret) && CameraArea::areAreasDifferent(mFocusAreas, tempAreas) )
    {
        mFocusAreas.clear();
        mFocusAreas = tempAreas;
        if ( MAX_FOCUS_AREAS < mFocusAreas.size() )
        {
            CAMHAL_LOGEB("Focus areas supported %d, focus areas set %d",
                         MAX_FOCUS_AREAS,
                         mFocusAreas.size());
            ret = -EINVAL;
        }
        else
        {
            if ( !mFocusAreas.isEmpty() )
            {
                mFocusAreasset = true;
                setTouchFocus();
            }
        }
    }

    LOG_FUNCTION_NAME;

    return ret;
}

status_t OMXCameraAdapter::doAutoFocus()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;
    if(strcmp(mCapabilities->get(CameraProperties::FOCUS_SUPPORTED), CameraParameters::FALSE) == 0)
    {
        returnFocusStatus(CameraHalEvent::FOCUS_STATUS_SUCCESS);
        return NO_ERROR;
    }
    
    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component in Invalid state");
        returnFocusStatus(CameraHalEvent::FOCUS_STATUS_FAIL);
        return -EINVAL;
    }

    if ( OMX_StateExecuting != mComponentState )
    {
        CAMHAL_LOGEA("OMX component not in executing state");
        returnFocusStatus(CameraHalEvent::FOCUS_STATUS_FAIL);
        return NO_ERROR;
    }

    if ( 0 != mDoAFSem.Count() )
    {
        CAMHAL_LOGEB("Error mDoAFSem semaphore count %d", mDoAFSem.Count());
        returnFocusStatus(CameraHalEvent::FOCUS_STATUS_FAIL);
        return NO_INIT;
    }

    if( ((AF_ACTIVE & getState()) != AF_ACTIVE) && ((AF_ACTIVE & getNextState()) != AF_ACTIVE) )
    {
        CAMHAL_LOGDA("Auto focus got canceled before doAutoFocus could be called");
        return NO_ERROR;
    }

    if ( mParameters3A.Focus == OMX_IMAGE_FocusControlOff )
    {
        setFocusMode((OMX_IMAGE_FOCUSCONTROLTYPE)OMX_IMAGE_FocusControlOff );
        ret = returnFocusStatus(CameraHalEvent::FOCUS_STATUS_SUCCESS);
        return NO_ERROR;
    }

    //setFocusMode((OMX_IMAGE_FOCUSCONTROLTYPE)OMX_IMAGE_FocusControlSingle );
  	if(mParameters3A.Focus == OMX_IMAGE_FocusControlAuto)
  	{
        //ActionsCode(author:liuyiguang, add_code, turn on flash light before auto focus done!Only for 900A now)
#ifdef CAMERA_GS900A
        if(needFlashStrobe(mParameters3A))
        {
            mFlashStrobed = true;
            mFlashConvergenceFrame = mFlashConvergenceFrameConfig;
            startFlashStrobe();
            usleep(PRE_FLASHLIGHT_ON);
        }
#endif
         
  		//focus with continus mode, we consider it as success
  		ret = returnFocusStatus(CameraHalEvent::FOCUS_STATUS_SUCCESS);
  		return NO_ERROR;
  	}
  	else{
  		if(mFocusAreasset == true)
  		{
  			setFocusMode((OMX_IMAGE_FOCUSCONTROLTYPE)OMX_IMAGE_FocusControlZone);
  			mFocusAreasset = false;  
  		}
  		else{
  				setFocusMode((OMX_IMAGE_FOCUSCONTROLTYPE)OMX_IMAGE_FocusControlSingle);
  		}
  	}

    {
        Mutex::Autolock lock(mDoAFMutex);
        mCheckAF = true;
        ret = mDoAFCond.waitRelative(mDoAFMutex, ( nsecs_t ) AF_TIMEOUT * 1000);
    }
    if(ret != NO_ERROR) {
        CAMHAL_LOGEA("Autofocus callback timeout expired");
        ret = returnFocusStatus(CameraHalEvent::FOCUS_STATUS_FAIL);
    } 

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
 *
 * BUGFIX:  Fix autofocus stop status, don't change to OMX_IMAGE_FocusControlAuto, otherwise it will refocus.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXCameraAdapter::stopAutoFocus()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;
    {
        Mutex::Autolock lock(mDoAFMutex);
        if(mCheckAF == true)
        {
            mCheckAF = false;
            mDoAFCond.broadcast();
        }

        //ActionsCode(author:liuyiguang, change_code)
        ret = setFocusMode(OMX_IMAGE_FocusControlOff);
#if 0
        if (mIternalRecordingHint) {
            CAMHAL_LOGDA("dv: set focus mode off");
            ret = setFocusMode(OMX_IMAGE_FocusControlOff);
        }
        else {
            //for preview mode, revert focus mode to continous focus mode if focus off is not required
            if (mParameters3A.Focus == OMX_IMAGE_FocusControlOff) {
                CAMHAL_LOGDA("dc :set focus mode off");
                ret = setFocusMode(OMX_IMAGE_FocusControlOff);
            }
            else {
                CAMHAL_LOGDA("dc :set focus mode auto");
                ret = setFocusMode(OMX_IMAGE_FocusControlAuto);
            }
        }
#endif
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::getFocusMode(OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE &focusMode)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    OMX_INIT_STRUCT_PTR (&focusMode, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
    focusMode.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    eError =  OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                            OMX_IndexConfigFocusControl,
                            &focusMode);

    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while retrieving focus mode 0x%x", eError);
    }

    LOG_FUNCTION_NAME_EXIT;
    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setFocusMode(OMX_IMAGE_FOCUSCONTROLTYPE mode)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focus;
    size_t top, left, width, height, weight;
    OMX_CONFIG_BOOLEANTYPE bOMX;

    LOG_FUNCTION_NAME;

    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        return NO_INIT;
    }

    
    if(mode == OMX_IMAGE_FocusControlAuto
        || mode == OMX_IMAGE_FocusControlOff
            || mode == (OMX_IMAGE_FOCUSCONTROLTYPE)(OMX_IMAGE_FocusControlSingle)
           			|| mode == (OMX_IMAGE_FOCUSCONTROLTYPE)(OMX_IMAGE_FocusControlZone)  
                		|| mode == (OMX_IMAGE_FOCUSCONTROLTYPE)(OMX_IMAGE_FocusControlQuickCapture))
    {
        OMX_INIT_STRUCT_PTR (&focus, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
        focus.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
        focus.eFocusControl = (OMX_IMAGE_FOCUSCONTROLTYPE)mode;

        CAMHAL_LOGDB("Configuring focus mode 0x%x", focus.eFocusControl);
        eError = OMX_SetConfig( mCameraAdapterParameters.mHandleComp, OMX_IndexConfigFocusControl, &focus);
        if ( OMX_ErrorNone != eError )
        {
            CAMHAL_LOGEB("Error while configuring focus mode 0x%x", eError);
        }
        else
        {
            CAMHAL_LOGDA("Camera focus mode configured successfully");
        }
    }
 

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::cancelAutoFocus()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;
    ret = stopAutoFocus();

    LOG_FUNCTION_NAME_EXIT;

    return ret;

}

status_t OMXCameraAdapter::getFocusStatus(CameraHalEvent::FocusStatus &status)
{
    status_t ret = NO_ERROR;
    OMX_PARAM_FOCUSSTATUSTYPE eFocusStatus;
    OMX_U32 maxFAreas = 1;

    LOG_FUNCTION_NAME;

    ret = getFocusStatus(eFocusStatus);

    if ( NO_ERROR != ret )
    {
        CAMHAL_LOGEA("Focus status check failed!");
        status = CameraHalEvent::FOCUS_STATUS_FAIL;
        return NO_ERROR;
    }

    if (eFocusStatus.eFocusStatus == OMX_FocusStatusRequest)
    {
        status = CameraHalEvent::FOCUS_STATUS_PENDING;
    }
    else if (eFocusStatus.eFocusStatus == OMX_FocusStatusReached)
    {
        status = CameraHalEvent::FOCUS_STATUS_SUCCESS;
    }
    else
    {
        status = CameraHalEvent::FOCUS_STATUS_FAIL;
    }

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t OMXCameraAdapter::returnFocusStatus(CameraHalEvent::FocusStatus focusStatus)
{
    status_t ret = NO_ERROR;
    BaseCameraAdapter::AdapterState state, nextState;
    BaseCameraAdapter::getState(state);
    BaseCameraAdapter::getNextState(nextState);

    LOG_FUNCTION_NAME;

    if( ((AF_ACTIVE & state ) != AF_ACTIVE) && ((AF_ACTIVE & nextState ) != AF_ACTIVE) )
    {
        /// We don't send focus callback if focus was not started
        CAMHAL_LOGDA("Not sending focus callback because focus was not started");
        return NO_ERROR;
    }

    //Query current focus distance after AF is complete
    updateFocusDistances(mParameters);
    updateFocusLength(mParameters);

    ret =  BaseCameraAdapter::setState(CAMERA_CANCEL_AUTOFOCUS);
    if ( NO_ERROR == ret )
    {
        ret = stopAutoFocus();
    }
    if ( NO_ERROR == ret )
    {
        ret = BaseCameraAdapter::commitState();
    }
    else
    {
        ret |= BaseCameraAdapter::rollbackState();
    }

    if ( NO_ERROR == ret )
    {
        notifyFocusSubscribers(focusStatus);
    }

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t OMXCameraAdapter::checkFocusStatus()
{
    status_t ret = NO_ERROR;
    CameraHalEvent::FocusStatus status = CameraHalEvent::FOCUS_STATUS_INVALID;

    BaseCameraAdapter::AdapterState state,nextState;
    BaseCameraAdapter::getState(state);
    BaseCameraAdapter::getNextState(nextState);
    OMX_U32 focus_ratio = 0;

    LOG_FUNCTION_NAME;
    if(strcmp(mCapabilities->get(CameraProperties::FOCUS_SUPPORTED), CameraParameters::FALSE) == 0)
    {
        return NO_ERROR;
    }

    if ( mParameters3A.Focus == OMX_IMAGE_FocusControlOff)
    {
        return NO_ERROR;
    }

    if ((mParameters3A.Focus == OMX_IMAGE_FocusControlAuto)
            &&((AF_ACTIVE & state ) != AF_ACTIVE) ) {
        CAMHAL_LOGDA("continous focus modemode");
        if (((CAPTURE_ACTIVE & state ) != CAPTURE_ACTIVE) && ((CAPTURE_ACTIVE & nextState ) != CAPTURE_ACTIVE))
        {
            ret = getFocusRatio(focus_ratio);

            if (focus_ratio >= 0 && focus_ratio < FOCUS_RATIO_THREADSHOLD) {
                status = CameraHalEvent::FOCUS_STATUS_PENDING;
                mFocusPending = OMX_TRUE;
                notifyFocusSubscribers(status);
                CAMHAL_LOGDB("focus ratio = %lu, notify pending",focus_ratio);
            }
            else if(focus_ratio >= FOCUS_RATIO_THREADSHOLD){
                status = CameraHalEvent::FOCUS_STATUS_DONE;
                if (mFocusPending) {
                    CAMHAL_LOGDB("focus ratio = %lu, notify done",focus_ratio);
                    notifyFocusSubscribers(status);
                }
                mFocusPending = OMX_FALSE;
            }
        }
    }
    else {
        if ((AF_ACTIVE & state ) != AF_ACTIVE){
            return NO_ERROR;
        }

        {
            Mutex::Autolock lock(mDoAFMutex);
            if(mCheckAF == false)
            {
                return NO_ERROR;
            }
            ret = getFocusStatus(status);
            if(ret == NO_ERROR)
            {
                if(status == CameraHalEvent::FOCUS_STATUS_SUCCESS
                    || status == CameraHalEvent::FOCUS_STATUS_FAIL)
                {
                    if(mCheckAF == true)
                    {
                        mCheckAF = false;
                        mDoAFCond.broadcast();
                    }
                }
            }
        }

        if(status == CameraHalEvent::FOCUS_STATUS_SUCCESS
            || status == CameraHalEvent::FOCUS_STATUS_FAIL)
        {
            returnFocusStatus(status);
        }
    }

    return NO_ERROR;
}

status_t OMXCameraAdapter::getFocusStatus(OMX_PARAM_FOCUSSTATUSTYPE &eFocusStatus)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component in Invalid state");
        ret = -EINVAL;
    }

    if ( OMX_StateExecuting != mComponentState )
    {
        CAMHAL_LOGEA("OMX component not in executing state");
        ret = NO_ERROR;
    }

    if ( NO_ERROR == ret )
    {
        OMX_INIT_STRUCT_PTR (&eFocusStatus, OMX_PARAM_FOCUSSTATUSTYPE);
        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                               static_cast<OMX_INDEXTYPE>(OMX_IndexConfigCommonFocusStatus),
                               &eFocusStatus);
        if ( OMX_ErrorNone != eError )
        {
            CAMHAL_LOGEB("Error while retrieving focus status: 0x%x", eError);
            ret = -1;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::getFocusRatio(OMX_U32 &focus_ratio)
{
    OMX_PARAM_FOCUSSTATUSTYPE eFocusStatus;
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;


    LOG_FUNCTION_NAME;
    ret = getFocusStatus(eFocusStatus);
    focus_ratio = eFocusStatus.nFocusRatio;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::getMaxFAreas(OMX_U32 *maxfareas)
{

    status_t ret = NO_ERROR;
#if 0
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_CONFIG_FOCUSREGIONSTATUSTYPE eFocusStatus;
    LOG_FUNCTION_NAME;

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component in Invalid state");
        ret = -EINVAL;
    }

    if ( NO_ERROR == ret )
    {
        OMX_INIT_STRUCT (eFocusStatus, OMX_CONFIG_FOCUSREGIONSTATUSTYPE);

        eFocusStatus.nMaxFAreas = 0;
        eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                               static_cast<OMX_INDEXTYPE>(OMX_IndexConfigCommonFocusRegionStatus),
                               &eFocusStatus);
        if ( OMX_ErrorNone != eError )
        {
            CAMHAL_LOGEB("Error while retrieving focus status: 0x%x", eError);
            ret = -1;
        }
    }

    if ( NO_ERROR == ret )
    {
        *maxfareas = eFocusStatus.nMaxFAreas;
    }
    else
    {
        *maxfareas = 1;
    }
#endif
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::updateFocusDistances(CameraParameters &params)
{
    OMX_U32 focusNear, focusOptimal, focusFar, focusLen;
    status_t ret = NO_ERROR;

    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focusMode;

    LOG_FUNCTION_NAME;
#if 0
    ret = getFocusMode(focusMode);
    if(ret == NO_ERROR && focusMode.eFocusControl == OMX_IMAGE_FocusControlAuto)
    {
        ret = getFocusDistances(focusNear, focusOptimal, focusFar, focusLen);
        if ( NO_ERROR == ret)
        {
            ret = addFocusDistances(focusNear, focusOptimal, focusFar, params);
            if ( NO_ERROR != ret )
            {
                CAMHAL_LOGEB("Error in call to addFocusDistances() 0x%x", ret);
            }
        }
        else
        {
            CAMHAL_LOGEB("Error in call to getFocusDistances() 0x%x", ret);
        }
    }
    else
#endif
    {
        params.set(CameraParameters::KEY_FOCUS_DISTANCES, DEFAULT_FOCUS_DISTANCES);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::updateFocusLength(CameraParameters &params)
{
    OMX_U32 focusNear, focusOptimal, focusFar, focusLen;
    status_t ret = NO_ERROR;

    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE focusMode;

    LOG_FUNCTION_NAME;
#if 0
    if(ret == NO_ERROR && focusMode.eFocusControl == OMX_IMAGE_FocusControlAuto)
    {
        ret = getFocusDistances(focusNear, focusOptimal, focusFar, focusLen);
        if ( NO_ERROR == ret)
        {
            params.set(CameraParameters::KEY_FOCAL_LENGTH, focusLen);
        }
        else
        {
            CAMHAL_LOGEB("Error in call to getFocusDistances() 0x%x", ret);
        }
    }
    else
#endif
    {
        if(mSensorIndex == 0)
        {
            params.set(CameraParameters::KEY_FOCAL_LENGTH, DEFAULT_FOCAL_LENGTH_PRIMARY);
        }
        else
        {
            params.set(CameraParameters::KEY_FOCAL_LENGTH, DEFAULT_FOCAL_LENGTH_SECONDARY);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::getFocusDistances(OMX_U32 &near,OMX_U32 &optimal, OMX_U32 &far, OMX_U32 &len)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_ACT_CONFIG_FOCUSDISTANCETYPE focusDistance;

    LOG_FUNCTION_NAME;

    OMX_INIT_STRUCT_PTR (&focusDistance, OMX_ACT_CONFIG_FOCUSDISTANCETYPE);
    focusDistance.nPortIndex = OMX_ALL;
    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                               static_cast<OMX_INDEXTYPE>(OMX_ACT_IndexConfigFocusDistance),
                               &focusDistance);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while retrieving focus status: 0x%x", eError);
        ret = -1;
    }
    near = focusDistance.nFocusDistanceNear;
    optimal = focusDistance.nFocusDistanceOptimal;
    far = focusDistance.nFocusDistanceFar;
    len = focusDistance.nLensPosition;


    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::encodeFocusDistance(OMX_U32 dist, char *buffer, size_t length)
{
    status_t ret = NO_ERROR;
    uint32_t focusScale = 1000;
    float distFinal;

    LOG_FUNCTION_NAME;


    if ( NO_ERROR == ret )
    {
        if ( 0 == dist )
        {
            strncpy(buffer, CameraParameters::FOCUS_DISTANCE_INFINITY, ( length - 1 ));
        }
        else
        {
            distFinal = dist;
            distFinal /= focusScale;
            snprintf(buffer, ( length - 1 ) , "%5.3f", distFinal);
        }
    }

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t OMXCameraAdapter::addFocusDistances(OMX_U32 &near,
        OMX_U32 &optimal,
        OMX_U32 &far,
        CameraParameters& params)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NO_ERROR == ret )
    {
        ret = encodeFocusDistance(near, mFocusDistNear, FOCUS_DIST_SIZE);
        if ( NO_ERROR != ret )
        {
            CAMHAL_LOGEB("Error encoding near focus distance 0x%x", ret);
        }
    }

    if ( NO_ERROR == ret )
    {
        ret = encodeFocusDistance(optimal, mFocusDistOptimal, FOCUS_DIST_SIZE);
        if ( NO_ERROR != ret )
        {
            CAMHAL_LOGEB("Error encoding near focus distance 0x%x", ret);
        }
    }

    if ( NO_ERROR == ret )
    {
        ret = encodeFocusDistance(far, mFocusDistFar, FOCUS_DIST_SIZE);
        if ( NO_ERROR != ret )
        {
            CAMHAL_LOGEB("Error encoding near focus distance 0x%x", ret);
        }
    }

    if ( NO_ERROR == ret )
    {
        snprintf(mFocusDistBuffer, ( FOCUS_DIST_BUFFER_SIZE - 1) ,"%s,%s,%s", mFocusDistNear,
                 mFocusDistOptimal,
                 mFocusDistFar);

        params.set(CameraParameters::KEY_FOCUS_DISTANCES, mFocusDistBuffer);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
 *
 * BUGFIX:  Fix for Singal Focus.
 * BUGFIX:  MergeFix for zone focus.
 * BUGFIX:  Fix for the touch focus coordinate value for digital zoom.
 * BUGFIX:  Fix for the touch focus, if there is no preview data, exit touch focus.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXCameraAdapter::setTouchFocus()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    OMX_CONFIG_FOCUSREGIONCONTROLTYPE *focusRegionControl = NULL;
    unsigned int regionSize = 0;
    unsigned int dataLen = 0;

    LOG_FUNCTION_NAME;
    if(strcmp(mCapabilities->get(CameraProperties::FOCUS_SUPPORTED), CameraParameters::FALSE) == 0)
    {
        CAMHAL_LOGEA("setTouchFocus  CameraProperties::FOCUS_SUPPORTED");
        return NO_ERROR;
    }
    if(strcmp(mCapabilities->get(CameraProperties::MAX_REAL_FOCUS_AREAS), "0") == 0)
    {
        CAMHAL_LOGEA("setTouchFocus  CameraProperties::MAX_REAL_FOCUS_AREAS");
        return NO_ERROR;
    }

    //ActionsCode(author:liuyiguang, change_code)
    //if ( mParameters3A.Focus != OMX_IMAGE_FocusControlZone )
    if ( mParameters3A.Focus == OMX_IMAGE_FocusControlAuto )
    {
        CAMHAL_LOGEA("setTouchFocus  mParameters3A.Focus != OMX_IMAGE_FocusControlZone");
        return NO_ERROR;
    }

    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
    }

    if ( NO_ERROR == ret )
    {
        CAMHAL_LOGEA("setTouchFocus  NO_ERROR == ret ");
        regionSize = mFocusAreas.size();

        // If the area is the special case of (0, 0, 0, 0, 0), then
        // the algorithm needs nNumAreas to be set to 0,
        // in order to automatically choose the best fitting areas.
        if ( regionSize == 1 && mFocusAreas.itemAt(0)->isZeroArea() )
        {
            regionSize = 0;
            goto EXIT;
        }

        OMXCameraPortParameters * mPreviewData = NULL;
        mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
        //ActionsCode(author:liuyiguang, change_code, if there is no preview data, exit touch focus!)
        if (mPreviewData->mWidth == 0 || mPreviewData->mHeight == 0)
        {
            CAMHAL_LOGEA("No Preview Data, exit touch focus!");
            goto EXIT;
        }
        if (regionSize >= 1)
        {
            dataLen = sizeof(OMX_CONFIG_FOCUSREGIONCONTROLTYPE)+sizeof(OMX_MANUALFOCUSRECTTYPE)*(regionSize-1);
            focusRegionControl = reinterpret_cast<OMX_CONFIG_FOCUSREGIONCONTROLTYPE *>(new char[dataLen]);

            if (!focusRegionControl)
            {
                CAMHAL_LOGEB("Error allocating buffer for focus areas %d", eError);
                return -ENOMEM;
            }

            OMX_INIT_STRUCT_PTR (focusRegionControl, OMX_CONFIG_FOCUSREGIONCONTROLTYPE);

            focusRegionControl->nFAreas= regionSize;
            focusRegionControl->eFocusRegionsControl = OMX_FocusRegionControlManual;

            for ( unsigned int n = 0; n < regionSize; n++)
            {
                // transform the coordinates to 3A-type coordinates
                mFocusAreas.itemAt(n)->transfrom((size_t)mPreviewData->mWidth,
                                                 (size_t)mPreviewData->mHeight,
                                                 (size_t&)focusRegionControl->sManualFRegions[n].nRectX,
                                                 (size_t&)focusRegionControl->sManualFRegions[n].nRectY,
                                                 (size_t&)focusRegionControl->sManualFRegions[n].nRectWidth,
                                                 (size_t&)focusRegionControl->sManualFRegions[n].nRectHeight);

                //ActionsCode(author:liuyiguang, add_code, fix for the touch focus coordinate value for digital zoom)
                focusRegionControl->sManualFRegions[n].nRectX = (focusRegionControl->sManualFRegions[n].nRectX * mPreviewData->mZoomWidth)  / mPreviewData->mWidth  + mPreviewData->mZoomXOff;
                focusRegionControl->sManualFRegions[n].nRectY = (focusRegionControl->sManualFRegions[n].nRectY * mPreviewData->mZoomHeight) / mPreviewData->mHeight + mPreviewData->mZoomYOff;

                //TODO, rect size may need to convert for omx
                CAMHAL_LOGEB("Focus area %d : top = %d left = %d width = %d height = %d ",
                             n, (int)focusRegionControl->sManualFRegions[n].nRectX, (int)focusRegionControl->sManualFRegions[n].nRectY,
                             (int)focusRegionControl->sManualFRegions[n].nRectWidth, (int)focusRegionControl->sManualFRegions[n].nRectHeight);
            }

        }
        else
        {
            focusRegionControl = new OMX_CONFIG_FOCUSREGIONCONTROLTYPE();
            if (!focusRegionControl)
            {
                CAMHAL_LOGEB("Error allocating buffer for focus areas %d", eError);
                return -ENOMEM;
            }

            OMX_INIT_STRUCT_PTR (focusRegionControl, OMX_CONFIG_FOCUSREGIONCONTROLTYPE);

            //ActionsCode(author:liuyiguang, change_code)
            focusRegionControl->nFAreas= regionSize;
            focusRegionControl->eFocusRegionsControl = OMX_FocusRegionControlAuto;

            for ( unsigned int n = 0; n < regionSize; n++)
            {
                // transform the coordinates to 3A-type coordinates
                mFocusAreas.itemAt(n)->transfrom((size_t)mPreviewData->mWidth,
                                                 (size_t)mPreviewData->mHeight,
                                                 (size_t&)focusRegionControl->sManualFRegions[n].nRectX,
                                                 (size_t&)focusRegionControl->sManualFRegions[n].nRectY,
                                                 (size_t&)focusRegionControl->sManualFRegions[n].nRectWidth,
                                                 (size_t&)focusRegionControl->sManualFRegions[n].nRectHeight);

            }
        }

        eError =  OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                                (OMX_INDEXTYPE) OMX_IndexConfigCommonFocusRegionControl, focusRegionControl);

        if ( OMX_ErrorNone != eError )
        {
            CAMHAL_LOGEB("Error while setting Focus Areas configuration 0x%x", eError);
            ret = -EINVAL;
        }
    }

EXIT:
    if (NULL != focusRegionControl)
    {
        if(regionSize >= 1)
        {
            delete[] reinterpret_cast<char *>(focusRegionControl);
        }
        else if(regionSize == 1)
        {
            delete focusRegionControl;
        }
        else
        {
        }
        focusRegionControl = NULL;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

};
