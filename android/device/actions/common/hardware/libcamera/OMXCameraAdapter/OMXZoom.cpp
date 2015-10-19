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
* @file OMXZoom.cpp
*
* This file contains functionality for handling zoom configurations.
*
*/

#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"

namespace android
{

#define CROP_WIDTH_MIN (128)
#define CROP_HEIGHT_MIN (96)

const int32_t OMXCameraAdapter::ZOOM_STEPS [] =
{
    65536, 68157, 70124, 72745,
    75366, 77988, 80609, 83231,
    86508, 89784, 92406, 95683,
    99615, 102892, 106168, 110100,
    114033, 117965, 122552, 126484,
    131072, 135660, 140247, 145490,
    150733, 155976, 161219, 167117,
    173015, 178913, 185467, 192020,
    198574, 205783, 212992, 220201,
    228065, 236585, 244449, 252969,
    262144,
};

#define ZOOM_STAGES (sizeof(ZOOM_STEPS)/sizeof(ZOOM_STEPS[0]))


status_t OMXCameraAdapter::setParametersZoom(const CameraParameters &params,
        BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;
    Mutex::Autolock lock(mZoomLock);

    LOG_FUNCTION_NAME;

    //Immediate zoom should not be avaialable while smooth zoom is running
    if ( ( ZOOM_ACTIVE & state ) != ZOOM_ACTIVE )
    {
        int zoom = params.getInt(CameraParameters::KEY_ZOOM);
        if( ( zoom >= 0 ) && ( zoom < static_cast<int>(ZOOM_STAGES) ) )
        {
            mTargetZoomIdx = zoom;

            //Immediate zoom should be applied instantly ( CTS requirement )
            mCurrentZoomIdx = mTargetZoomIdx;
            if(!mZoomUpdating)
            {
                doZoom(mCurrentZoomIdx);
                mZoomUpdating = true;
            }
            else
            {
                mZoomUpdate = true;
            }


            CAMHAL_LOGDB("Zoom by App %d", zoom);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}
status_t OMXCameraAdapter::setPortZoom(unsigned int portIndex, int index)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters *cap;
    int unalinged_col = 0;

    LOG_FUNCTION_NAME;

    if (  ( 0 > index) || ( ( static_cast<int>(ZOOM_STAGES - 1) ) < index ) )
    {
        CAMHAL_LOGEB("Zoom index %d out of range", index);
        ret = -EINVAL;
    }

    if ( portIndex  == mCameraAdapterParameters.mPrevPortIndex )
    {
        cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
    }
    else if ( portIndex  == mCameraAdapterParameters.mImagePortIndex )
    {
        cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
    }
    else
    {
        ret = -EINVAL;
        CAMHAL_LOGEB("setPortZoom  error portindex =%d !!!!!", portIndex);
        goto exit;

    }

    /**
    * BUGFIX: fix for 5.1 cts verifier camera FOV Calibration, its preview center is unaligned to center line .
    * this is because the current lowlevel(omxcamera + isp drv) relaxed the width align requirement to be 16 pixels, so just update to 16 align to keep the comformity with lowlevel support here.
    *ActionsCode(author:liyuan, change_code)
    */
    /*
       isp requires the input width to be 32 down aligend,
       when input width is not aligned, isp will crop the unaligned col and make the output width to be 32 aligned,
       thus, for isp input with unaligned width, the valid data area is (0,0,SIZE_ALIGN_DOWN_32(width),height)
     */
    //unalinged_col = cap->mWidth - SIZE_ALIGN_DOWN_32(cap->mWidth);
    unalinged_col = cap->mWidth - SIZE_ALIGN_DOWN_16(cap->mWidth);
    if (unalinged_col) {
        //cap->mCropWidth = SIZE_ALIGN_DOWN_32(cap->mCropWidth);
        cap->mCropWidth = SIZE_ALIGN_DOWN_16(cap->mCropWidth);
    }

    if(index>0)
    {
        cap->mZoomWidth = (cap->mCropWidth<<16)/ZOOM_STEPS[index];
        cap->mZoomWidth= SIZE_ALIGN_DOWN_16(cap->mZoomWidth);
        cap->mZoomHeight = (cap->mCropHeight<<16)/ZOOM_STEPS[index];
        cap->mZoomHeight= SIZE_ALIGN_DOWN_16(cap->mZoomHeight);

        if(cap->mZoomWidth < CROP_WIDTH_MIN )
        {
            cap->mZoomWidth =CROP_WIDTH_MIN;
        }
        if(cap->mZoomHeight < CROP_HEIGHT_MIN)
        {
            cap->mZoomHeight = CROP_HEIGHT_MIN;
        }
        cap->mZoomXOff= (cap->mWidth-cap->mZoomWidth - unalinged_col)>>1;
        cap->mZoomXOff= SIZE_ALIGN_DOWN_16(cap->mZoomXOff);
        cap->mZoomYOff = (cap->mHeight-cap->mZoomHeight)>>1;
        cap->mZoomYOff= SIZE_ALIGN_DOWN_16(cap->mZoomYOff);
    }
    else
    {
        cap->mZoomWidth = cap->mCropWidth;
        cap->mZoomHeight = cap->mCropHeight;
        cap->mZoomXOff= (cap->mWidth-cap->mZoomWidth - unalinged_col)>>1;
        cap->mZoomXOff= SIZE_ALIGN_DOWN_16(cap->mZoomXOff);
        cap->mZoomYOff = (cap->mHeight-cap->mZoomHeight)>>1;
        cap->mZoomYOff= SIZE_ALIGN_DOWN_16(cap->mZoomYOff);

    }

    CAMHAL_LOGDB("cap->mCropWidth= %d", (int)cap->mCropWidth);
    CAMHAL_LOGDB("cap->mCropHeight= %d", (int)cap->mCropHeight);
    CAMHAL_LOGDB("cap->mZoomWidth= %d", (int)cap->mZoomWidth);
    CAMHAL_LOGDB("cap->mZoomHeight= %d", (int)cap->mZoomHeight);
    
exit:
    return ret;
}
status_t OMXCameraAdapter::initZoom(unsigned int portIndex)
{
    status_t ret = NO_ERROR;
    Mutex::Autolock lock(mZoomLock);
    setPortZoom(portIndex, mCurrentZoomIdx);
    return ret;
}

status_t OMXCameraAdapter::doZoom(int index)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;
    CAMHAL_LOGDB("doZoom index =%d !!!!!", index);
    if ( 0/*OMX_StateInvalid*/ == mComponentState )
    {
        CAMHAL_LOGEA("OMX component is in invalid state");
        ret = -1;
    }

    if (  ( 0 > index) || ( ( static_cast<int>(ZOOM_STAGES - 1) ) < index ) )
    {
        CAMHAL_LOGEB("Zoom index %d out of range", index);
        ret = -EINVAL;
    }

    if (static_cast<int>(mPreviousZoomIndx) == index )
    {
        return NO_ERROR;
    }

    if ( NO_ERROR == ret )
    {
        setPortZoom(mCameraAdapterParameters.mPrevPortIndex, index);
        setPortZoom(mCameraAdapterParameters.mImagePortIndex, index);
        
        CAMHAL_LOGDA("Digital zoom applied successfully");
        mPreviousZoomIndx = index;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::advanceZoom()
{
    status_t ret = NO_ERROR;
    AdapterState state;
    Mutex::Autolock lock(mZoomLock);

    BaseCameraAdapter::getState(state);
    if ( mReturnZoomStatus )
    {
        mCurrentZoomIdx +=mZoomInc;
        mTargetZoomIdx = mCurrentZoomIdx;
        mReturnZoomStatus = false;
        ret = doZoom(mCurrentZoomIdx);
        notifyZoomSubscribers(mCurrentZoomIdx, true);
    }
    else if ( mCurrentZoomIdx != mTargetZoomIdx )
    {
        if ( ZOOM_ACTIVE & state )
        {
            if ( mCurrentZoomIdx < mTargetZoomIdx )
            {
                mZoomInc = 1;
            }
            else
            {
                mZoomInc = -1;
            }

            mCurrentZoomIdx += mZoomInc;
        }
        else
        {
            mCurrentZoomIdx = mTargetZoomIdx;
        }
        CAMHAL_LOGDA("advanceZoom");
        ret = doZoom(mCurrentZoomIdx);

        if ( ZOOM_ACTIVE & state )
        {
            if ( mCurrentZoomIdx == mTargetZoomIdx )
            {
                CAMHAL_LOGDB("[Goal Reached] Smooth Zoom notify currentIdx = %d, targetIdx = %d",
                             mCurrentZoomIdx,
                             mTargetZoomIdx);

                if ( NO_ERROR == ret )
                {

                    ret =  BaseCameraAdapter::setState(CAMERA_STOP_SMOOTH_ZOOM);

                    if ( NO_ERROR == ret )
                    {
                        ret = BaseCameraAdapter::commitState();
                    }
                    else
                    {
                        ret |= BaseCameraAdapter::rollbackState();
                    }

                }
                mReturnZoomStatus = false;
                notifyZoomSubscribers(mCurrentZoomIdx, true);
            }
            else
            {
                CAMHAL_LOGDB("[Advancing] Smooth Zoom notify currentIdx = %d, targetIdx = %d",
                             mCurrentZoomIdx,
                             mTargetZoomIdx);
                notifyZoomSubscribers(mCurrentZoomIdx, false);
            }
        }
    }
    else if ( (mCurrentZoomIdx == mTargetZoomIdx ) &&
              ( ZOOM_ACTIVE & state ) )
    {
        ret = BaseCameraAdapter::setState(CameraAdapter::CAMERA_STOP_SMOOTH_ZOOM);

        if ( NO_ERROR == ret )
        {
            ret = BaseCameraAdapter::commitState();
        }
        else
        {
            ret |= BaseCameraAdapter::rollbackState();
        }

    }

    if(mZoomUpdate)
    {
        doZoom(mTargetZoomIdx);
        mZoomUpdate = false;
        mZoomUpdating = true;
    }
    else
    {
        mZoomUpdating = false;
    }


    return ret;
}

status_t OMXCameraAdapter::startSmoothZoom(int targetIdx)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mZoomLock);

    CAMHAL_LOGDB("Start smooth zoom target = %d, mCurrentIdx = %d",
                 targetIdx,
                 mCurrentZoomIdx);

    if ( ( targetIdx >= 0 ) && ( targetIdx < static_cast<int>(ZOOM_STAGES) ) )
    {
        mTargetZoomIdx = targetIdx;
        mZoomParameterIdx = mCurrentZoomIdx;
        mReturnZoomStatus = false;
    }
    else
    {
        CAMHAL_LOGEB("Smooth value out of range %d!", targetIdx);
        ret = -EINVAL;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::stopSmoothZoom()
{
    status_t ret = NO_ERROR;
    Mutex::Autolock lock(mZoomLock);

    LOG_FUNCTION_NAME;

    if ( mTargetZoomIdx != mCurrentZoomIdx )
    {
        if ( mCurrentZoomIdx < mTargetZoomIdx )
        {
            mZoomInc = 1;
        }
        else
        {
            mZoomInc = -1;
        }
        mReturnZoomStatus = true;
        CAMHAL_LOGDB("Stop smooth zoom mCurrentZoomIdx = %d, mTargetZoomIdx = %d",
                     mCurrentZoomIdx,
                     mTargetZoomIdx);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

};
