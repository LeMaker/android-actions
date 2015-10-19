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
* @file CameraHal.cpp
*
* This file maps the Camera Hardware Interface to V4L2.
*
*/

#include "CameraATraceTag.h"

#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "ANativeWindowDisplayAdapter.h"
#include "ActCameraParameters.h"
#include "CameraProperties.h"
#include <cutils/properties.h>

#include <poll.h>
#include <math.h>

#include "MemoryManager.h"


#define CAMERA_FIX_IMAGE_BUFFER


namespace android
{

extern "C" CameraAdapter* CameraAdapter_Factory(size_t);

/*****************************************************************************/

////Constant definitions and declarations
////@todo Have a CameraProperties class to store these parameters as constants for every camera
////       Currently, they are hard-coded

const int CameraHal::NO_BUFFERS_PREVIEW = MAX_CAMERA_BUFFERS;
const int CameraHal::NO_BUFFERS_IMAGE_CAPTURE = MAX_CAMERA_CAPTURE_BUFFERS;

const uint32_t MessageNotifier::EVENT_BIT_FIELD_POSITION = 0;
const uint32_t FrameNotifier::FRAME_BIT_FIELD_POSITION = 0;


/******************************************************************************/

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

struct timeval CameraHal::mStartPreview;
struct timeval CameraHal::mStartFocus;
struct timeval CameraHal::mStartCapture;

#endif

static void orientation_cb(uint32_t orientation, uint32_t tilt, void* cookie)
{
    CameraHal *camera = NULL;

    if (cookie)
    {
        camera = (CameraHal*) cookie;
        camera->onOrientationEvent(orientation, tilt);
    }

}
/*-------------Camera Hal Interface Method definitions STARTS here--------------------*/

/**
  Callback function to receive orientation events from SensorListener
 */
void CameraHal::onOrientationEvent(uint32_t orientation, uint32_t tilt)
{
    LOG_FUNCTION_NAME;

    if ( NULL != mCameraAdapter )
    {
        mCameraAdapter->onOrientationEvent(orientation, tilt);
    }

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Set the notification and data callbacks

   @param[in] notify_cb Notify callback for notifying the app about events and errors
   @param[in] data_cb   Buffer callback for sending the preview/raw frames to the app
   @param[in] data_cb_timestamp Buffer callback for sending the video frames w/ timestamp
   @param[in] user  Callback cookie
   @return none

 */
void CameraHal::setCallbacks(camera_notify_callback notify_cb,
                             camera_data_callback data_cb,
                             camera_data_timestamp_callback data_cb_timestamp,
                             camera_request_memory get_memory,
                             void *user)
{
    LOG_FUNCTION_NAME;

    if ( NULL != mAppCallbackNotifier.get() )
    {
        mAppCallbackNotifier->setCallbacks(this,
                                           notify_cb,
                                           data_cb,
                                           data_cb_timestamp,
                                           get_memory,
                                           user);//AppCallbackNotifier::setCallbacks
    }

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Enable a message, or set of messages.

   @param[in] msgtype Bitmask of the messages to enable (defined in include/ui/Camera.h)
   @return none

 */
void CameraHal::enableMsgType(int32_t msgType)
{
    LOG_FUNCTION_NAME;


    if ( ( msgType & CAMERA_MSG_SHUTTER ) && ( !mShutterEnabled ) )
    {
        msgType &= ~CAMERA_MSG_SHUTTER;
    }

    // ignoring enable focus message from camera service
    // we will enable internally in autoFocus call
    msgType &= ~(CAMERA_MSG_FOCUS | CAMERA_MSG_FOCUS_MOVE);

    {
        Mutex::Autolock lock(mLock);
        mMsgEnabled |= msgType;
    }

    if(mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME)
    {
        if(mDisplayPaused)
        {
            CAMHAL_LOGDA("Preview currently paused...will enable preview callback when restarted");
            msgType &= ~CAMERA_MSG_PREVIEW_FRAME;
        }
        else
        {
            CAMHAL_LOGDA("Enabling Preview Callback");
        }
    }
    else
    {
        CAMHAL_LOGDB("Preview callback not enabled %x", msgType);
    }


    ///Configure app callback notifier with the message callback required
    mAppCallbackNotifier->enableMsgType (msgType);

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Disable a message, or set of messages.

   @param[in] msgtype Bitmask of the messages to disable (defined in include/ui/Camera.h)
   @return none

 */
void CameraHal::disableMsgType(int32_t msgType)
{
    LOG_FUNCTION_NAME;

    {
        Mutex::Autolock lock(mLock);
        mMsgEnabled &= ~msgType;
    }

    if( msgType & CAMERA_MSG_PREVIEW_FRAME)
    {
        CAMHAL_LOGDA("Disabling Preview Callback");
    }

    ///Configure app callback notifier
    mAppCallbackNotifier->disableMsgType (msgType);

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Query whether a message, or a set of messages, is enabled.

   Note that this is operates as an AND, if any of the messages queried are off, this will
   return false.

   @param[in] msgtype Bitmask of the messages to query (defined in include/ui/Camera.h)
   @return true If all message types are enabled
          false If any message type

 */
int CameraHal::msgTypeEnabled(int32_t msgType)
{
    LOG_FUNCTION_NAME;
    Mutex::Autolock lock(mLock);
    LOG_FUNCTION_NAME_EXIT;
    return (mMsgEnabled & msgType);
}
//
int CameraHal::msgTypeEnabledNoLock(int32_t msgType)
{
    //avoid testPreviewPictureSizesCombination to fail    
    if(msgType & CAMERA_MSG_PREVIEW_FRAME)
    {
        return (mMsgEnabled & msgType) && mPreviewCBEnabled;
    }
    return (mMsgEnabled & msgType);
}

/**
   @brief Set the camera parameters.

   @param[in] params Camera parameters to configure the camera
   @return NO_ERROR
   @todo Define error codes

 */
int CameraHal::setParameters(const char* parameters)
{

    LOG_FUNCTION_NAME;

    CameraParameters params;

    CAMERA_SCOPEDTRACE("setParameters");

    CAMHAL_LOGDB("setParameters===%s",parameters);
    
    /*
    if(strlen(parameters) >1024)
    {
        CAMHAL_LOGDB("setParameters===%s",parameters+800);
        CAMHAL_LOGDB("setParameters===%s",parameters+1600);
    }
    */
    

    String8 str_params(parameters);
    params.unflatten(str_params);

    LOG_FUNCTION_NAME_EXIT;

    return setParameters(params);
}

/**
* BUGFIX: relax restriction for the diffrent resolutions with yv12 format
* TODO: when yv12 in display module is supported, changes should be revert!
*ActionsCode(author:liyuan, change_code)
*/
int CameraHal::setPreviewFormatParameter(const char *f, int w)
{
    if ( isParameterValid(f, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FORMATS)) )
    {
        int previewFormat ;
        int mappedFormat;
        int videoFormat;

        mPreviewFormat = getPixFormatConstant(f);
        previewFormat = CameraFrame::getFrameFormat(f);
        mappedFormat = mCameraProperties->mPreviewFormatInfos.getMapValue(previewFormat);
        if(previewFormat < 0 || mappedFormat < 0)
        {
            ALOGE("set preview format failed!");
            return BAD_VALUE;
        }
        mMappedPreviewFormat = CameraFrame::getCameraFormat((CameraFrame::FrameFormat)mappedFormat);
        /**
        * BUGFIX: cts camera format for yv12.
	* BUGFIX: relax restriction for the diffrent resolutions with yv12 format.
        * NOTE: temp solution, TODO: when yv12 in display module is supported,here should be deleted!
        *ActionsCode(author:liyuan, change_code)
        */
        if((mMappedPreviewFormat == CameraParameters::PIXEL_FORMAT_YUV420P ||
	    mMappedPreviewFormat == ActCameraParameters::PIXEL_FORMAT_YUV420P_YU12))
	{
	    if(w/2 != SIZE_ALIGN_UP_16(w/2)){
	    	mMappedPreviewFormat = ActCameraParameters::PIXEL_FORMAT_YUV420SP_NV12;
		//ALOGD("mMappedPreviewFormat:%s", mMappedPreviewFormat);
	    }
	    mNeedfmtrefresh= true;
			
	}else{
	    mNeedfmtrefresh= false;
	}

	 mParameters.setPreviewFormat(mMappedPreviewFormat);
        CAMHAL_LOGDB("f:%s, previewformat:%s, id:%d, mapFormat:%s, id:%d",
                f, mPreviewFormat, previewFormat, mMappedPreviewFormat, mappedFormat);

        //set KEY_VIDEO_FRAME_FORMAT to preview format
        videoFormat = getHwVideoEncodeFormatPreferred(mappedFormat);
        mVideoFormat = CameraFrame::getCameraFormat((CameraFrame::FrameFormat)videoFormat);
        mParameters.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, mVideoFormat);
        ALOGD("mVideoFormat=%s",mVideoFormat);

        mAppCallbackNotifier->setPreviewFormat((CameraFrame::FrameFormat)previewFormat);
        mAppCallbackNotifier->setVideoFormat((CameraFrame::FrameFormat)videoFormat);

    }
    else
    {
        CAMHAL_LOGEB("Invalid preview format.Supported: %s",  mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FORMATS));
        return BAD_VALUE;
    }
    return NO_ERROR;

}

/**
   @brief Set the camera parameters.

   @param[in] params Camera parameters to configure the camera
   @return NO_ERROR
   @todo Define error codes

 */
/**
 *
 * BUGFIX:  Fix the calculation of DV record resolution,avoid to use videobuffer mode.
 * OPTIMIZE:   Use vce resize to raise the conversion speed.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
int CameraHal::setParameters(const CameraParameters& params)
{

    LOG_FUNCTION_NAME;

    int w, h;
    int w_orig, h_orig;
    int framerate,minframerate;
    int maxFPS, minFPS;
    const char *valstr = NULL;
    int varint = 0;
    status_t ret = NO_ERROR;
    CameraParameters oldParams = mParameters;
    // Needed for KEY_RECORDING_HINT
    bool restartPreviewRequired = false;
    bool updateRequired = false;

    {
        Mutex::Autolock lock(mLock);

        //lock to protect mParameters
        Mutex::Autolock param_lock(mParametersLock);

        /*
        ALOGD("set previewsizes=%s",params.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES));
        ALOGD("set videosizes=%s",params.get(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES));
        ALOGD("set previewsize=%s",params.get(CameraParameters::KEY_PREVIEW_SIZE));
        ALOGD("set videosize=%s",params.get(CameraParameters::KEY_VIDEO_SIZE));
        */

        ///Ensure that preview is not enabled when the below parameters are changed.
        if(!previewEnabledInner())
        {
            if ((valstr = params.get(CameraParameters::KEY_VIDEO_STABILIZATION)) != NULL)
            {
                // make sure we support vstab...if we don't and application is trying to set
                // vstab then return an error
                if (strcmp(mCameraProperties->get(CameraProperties::VSTAB_SUPPORTED),
                           CameraParameters::TRUE) == 0)
                {
                    CAMHAL_LOGDB("VSTAB %s",valstr);
                    mParameters.set(CameraParameters::KEY_VIDEO_STABILIZATION, valstr);
                }
                else if (strcmp(valstr, CameraParameters::TRUE) == 0)
                {
                    CAMHAL_LOGEB("ERROR: Invalid VSTAB: %s", valstr);
                    return BAD_VALUE;
                }
                else
                {
                    mParameters.set(CameraParameters::KEY_VIDEO_STABILIZATION,
                                    CameraParameters::FALSE);
                }
            }
        }

        //reset mUseVideoBuffers
        mUseVideoBuffers = false;
        CAMHAL_LOGIB("PreviewFormat %s", params.getPreviewFormat());
	//For BUGFIX: relax restriction for the diffrent resolutions with yv12 format
	params.getPreviewSize(&w, &h);

        if ((valstr = params.getPreviewFormat()) != NULL)
        {
            if ( isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FORMATS)))
            {
		/**
		* BUGFIX: relax restriction for the diffrent resolutions with yv12 format
		* TODO: when yv12 in display module is supported, changes should be revert!
		*ActionsCode(author:liyuan, change_code)
		*/
                if(mPreviewFormat == NULL || strcmp(mPreviewFormat,valstr)!= 0 || mNeedfmtrefresh == true)
                {
                    if(setPreviewFormatParameter(valstr, w) < 0)
                    {
                        return BAD_VALUE;
                    }
                    restartPreviewRequired |= true;
                }
            }
            else
            {
                CAMHAL_LOGEB("Invalid preview format.Supported: %s",  valstr);
                return BAD_VALUE;
            }
        }
        
        if(strcmp(mMappedPreviewFormat, mVideoFormat)!=0)
        {
            mUseVideoBuffers = true;
        }

        if(strcmp(mCameraProperties->get(CameraProperties::SUPPORTED_VIDEO_SIZES), "") != 0)
        {
            params.getVideoSize(&w, &h);
            if (w == -1 && h == -1)
            {
                CAMHAL_LOGEA("Unable to get video size");
                return BAD_VALUE;
            }


            if ( !isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_VIDEO_SIZES)))
            {
                CAMHAL_LOGEB("ERROR: Invalid video size %dx%d", w, h);
                return BAD_VALUE;
            } 
        }

        params.getPreviewSize(&w, &h);
        if (w == -1 && h == -1)
        {
            CAMHAL_LOGEA("Unable to get preview size");
            return BAD_VALUE;
        }

        if ( !isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_SIZES)))
        {
            //support for 720p recording
            CAMHAL_LOGEB("ERROR: Invalid preview size %dx%d", w, h);
            return BAD_VALUE;
        } 

        int oldWidth, oldHeight;
        mParameters.getPreviewSize(&oldWidth, &oldHeight);


        if ( ( oldWidth != w ) || ( oldHeight != h ) )
        {
            restartPreviewRequired |= true;
        }
        mParameters.setPreviewSize(w,h);

        if(strcmp(mCameraProperties->get(CameraProperties::SUPPORTED_VIDEO_SIZES), "") != 0)
        {
            params.getVideoSize(&mVideoWidth, &mVideoHeight);
            mParameters.setVideoSize(mVideoWidth,mVideoHeight);
        }
        else
        {
            params.getPreviewSize(&mVideoWidth, &mVideoHeight);
        }

        //set KEY_NATIVE_BUFFER_SIZE
        int i;
        for(i =0; i < (int)mCameraProperties->mExtendedVideoResolution.size(); i++)
        {
            const ExtendedResolution &extVideoRes = mCameraProperties->mExtendedVideoResolution.itemAt(i);
            if(w == (int)extVideoRes.width && h == (int)extVideoRes.height)
            {
                mParameters.set(ActCameraParameters::KEY_RECORD_VIDEO_WIDTH, extVideoRes.captureWidth);
                mParameters.set(ActCameraParameters::KEY_RECORD_VIDEO_HEIGHT, extVideoRes.captureHeight);

                //ActionsCode(author:liuyiguang, add_code)
                if (!mVceResize)
                {
                    mVceResize = true;
                }
                //scale down
                //ActionsCode(author:liuyiguang, change_code)
                //It is not a good idea to use video buffers mode, for in that mode, the framerate is too low, about 19 fps in evb borad!
                if((int)extVideoRes.cropWidth / 2 > mVideoWidth || (int)extVideoRes.cropHeight / 2 > mVideoHeight)
                {
                    mUseVideoBuffers = true;
                    mParameters.set(ActCameraParameters::KEY_RECORD_VIDEO_WIDTH, mVideoWidth);
                    mParameters.set(ActCameraParameters::KEY_RECORD_VIDEO_HEIGHT, mVideoHeight);
                }
                break;
            }
        }    
        if(i >= (int)mCameraProperties->mExtendedVideoResolution.size())
        {
            if(w > mVideoWidth || h > mVideoHeight)
            {
                //for video size, the videosize is not same with preview size
                mUseVideoBuffers = true;
                mParameters.set(ActCameraParameters::KEY_RECORD_VIDEO_WIDTH, mVideoWidth);
                mParameters.set(ActCameraParameters::KEY_RECORD_VIDEO_HEIGHT, mVideoHeight);
            }
            else if((w%32 && (strcmp(mVideoFormat,CameraParameters::PIXEL_FORMAT_YUV420P)==0))
                || (w%32 && (strcmp(mVideoFormat,ActCameraParameters::PIXEL_FORMAT_YUV420P_YU12)==0))
                || (w%16 && (strcmp(mVideoFormat,CameraParameters::PIXEL_FORMAT_YUV420SP)==0))
                || (w%16 && (strcmp(mVideoFormat,ActCameraParameters::PIXEL_FORMAT_YUV420SP_NV12)==0))
                ) 
            {
                //for 176x144, width is not aligned to 32 at yuv420p
                mUseVideoBuffers = true;
                mParameters.set(ActCameraParameters::KEY_RECORD_VIDEO_WIDTH, w);
                mParameters.set(ActCameraParameters::KEY_RECORD_VIDEO_HEIGHT, h);

            }
            else
            {
                mParameters.set(ActCameraParameters::KEY_RECORD_VIDEO_WIDTH, w);
                mParameters.set(ActCameraParameters::KEY_RECORD_VIDEO_HEIGHT, h);
            }
        }

        CAMHAL_LOGIB("PreviewResolution by App %d x %d", w, h);

        // Handle RECORDING_HINT to Set/Reset Video Mode Parameters
        valstr = params.get(CameraParameters::KEY_RECORDING_HINT);
        if(valstr != NULL)
        {
            if(strcmp(valstr, CameraParameters::TRUE) == 0)
            {
                CAMHAL_LOGDB("Recording Hint is set to %s", valstr);
                mParameters.set(CameraParameters::KEY_RECORDING_HINT, valstr);

            }
            else if(strcmp(valstr, CameraParameters::FALSE) == 0)
            {
                CAMHAL_LOGDB("Recording Hint is set to %s", valstr);
                mParameters.set(CameraParameters::KEY_RECORDING_HINT, valstr);
            }
            else
            {
                CAMHAL_LOGEA("Invalid RECORDING_HINT");
                return BAD_VALUE;
            }
        }
        else
        {
            // This check is required in following case.
            // If VideoRecording activity sets KEY_RECORDING_HINT to TRUE and
            // ImageCapture activity doesnot set KEY_RECORDING_HINT to FALSE (i.e. simply NULL),
            // then Video Mode parameters may remain present in ImageCapture activity as well.
            CAMHAL_LOGDA("Recording Hint is set to NULL");
            mParameters.set(CameraParameters::KEY_RECORDING_HINT, CameraParameters::FALSE);
        }

        if ((valstr = params.get(CameraParameters::KEY_FOCUS_MODE)) != NULL)
        {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_FOCUS_MODES)))
            {
                CAMHAL_LOGDB("Focus mode set %s", valstr);
                mParameters.set(CameraParameters::KEY_FOCUS_MODE, valstr);
            }
            else
            {
                CAMHAL_LOGEB("ERROR: Invalid FOCUS mode = %s", valstr);
                return BAD_VALUE;
            }
        }

        ///Below parameters can be changed when the preview is running
        if ( (valstr = params.getPictureFormat()) != NULL )
        {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_FORMATS)))
            {
                mParameters.setPictureFormat(valstr);
            }
            else
            {
                CAMHAL_LOGEB("ERROR: Invalid picture format: %s",valstr);
                return BAD_VALUE;
            }
        }

        params.getPictureSize(&w, &h);
        if ( isResolutionValid(w, h, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_SIZES)))
        {
            mParameters.setPictureSize(w, h);
        }
        else
        {
            CAMHAL_LOGEB("ERROR: Invalid picture resolution %dx%d", w, h);
            return BAD_VALUE;
        }

        CAMHAL_LOGIB("Picture Size by App %d x %d", w, h);


        framerate = params.getPreviewFrameRate();
        valstr = params.get(CameraParameters::KEY_PREVIEW_FPS_RANGE);
        CAMHAL_LOGDB("FRAMERATE %d", framerate);

        CAMHAL_LOGVB("Passed FRR: %s, Supported FRR %s", valstr
                     , mCameraProperties->get(CameraProperties::PREVIEW_FRAMERATE_RANGE_SUPPORTED));
        CAMHAL_LOGVB("Passed FR: %d, Supported FR %s", framerate
                     , mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES));


        //Perform parameter validation
        if(!isParameterValid(valstr
                             , mCameraProperties->get(CameraProperties::PREVIEW_FRAMERATE_RANGE_SUPPORTED))
                || !isParameterValid(framerate,
                                     mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES)))
        {
                //return error for cts when fps is invalid
                return BAD_VALUE;
        }

        // Variable framerate ranges have higher priority over
        // deprecated constant FPS. "KEY_PREVIEW_FPS_RANGE" should
        // be cleared by the client in order for constant FPS to get
        // applied.
        if ( strcmp(valstr, mCameraProperties->get(CameraProperties::PREVIEW_FRAMERATE_RANGE))  != 0)
        {
            // APP wants to set FPS range
            //Set framerate = MAXFPS
            CAMHAL_LOGDA("APP IS CHANGING FRAME RATE RANGE");
            params.getPreviewFpsRange(&minFPS, &maxFPS);

            if ( ( 0 > minFPS ) || ( 0 > maxFPS ) )
            {
                CAMHAL_LOGEA("ERROR: FPS Range is negative!");
                return BAD_VALUE;
            }

            framerate = maxFPS /CameraHal::VFR_SCALE;
        }
        else
        {
            /*
            if ( framerate != atoi(mCameraProperties->get(CameraProperties::PREVIEW_FRAME_RATE)) )
            {

                selectFPSRange(framerate, &minFPS, &maxFPS);
                CAMHAL_LOGDB("Select FPS Range %d %d", minFPS, maxFPS);
                framerate = maxFPS /CameraHal::VFR_SCALE;
            }*/
           
        }
        mParameters.setPreviewFrameRate(framerate);
        mParameters.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, params.get(CameraParameters::KEY_PREVIEW_FPS_RANGE));

        if ((valstr = params.get(CameraParameters::KEY_WHITE_BALANCE)) != NULL)
        {
            if ( isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_WHITE_BALANCE)))
            {
                CAMHAL_LOGDB("White balance set %s", valstr);
                mParameters.set(CameraParameters::KEY_WHITE_BALANCE, valstr);
            }
            else
            {
                CAMHAL_LOGEB("ERROR: Invalid white balance  = %s", valstr);
                return BAD_VALUE;
            }
        }

        if(strcmp(mCameraProperties->get(CameraProperties::CONTRAST_SUPPORTED), CameraParameters::TRUE) == 0)
        {
            if ((valstr = params.get(ActCameraParameters::KEY_CONTRAST)) != NULL)
            {
                int min, max,step, v;
                const char *p;

                p = mCameraProperties->get(CameraProperties::SUPPORTED_CONTRAST_MIN);
                min = atoi(p);
                p = mCameraProperties->get(CameraProperties::SUPPORTED_CONTRAST_MAX);
                max = atoi(p);
                p = mCameraProperties->get(CameraProperties::SUPPORTED_CONTRAST_STEP);
                step = atoi(p);
                v = atoi(valstr);

                if(v>=min && v<=max)
                {
                    CAMHAL_LOGDB("Contrast set %s", valstr);
                    mParameters.set(ActCameraParameters::KEY_CONTRAST, valstr);
                }
                else
                {
                    CAMHAL_LOGEB("ERROR: Invalid contrast  = %s", valstr);
                }
            }
        }


        if(strcmp(mCameraProperties->get(CameraProperties::SATURATION_SUPPORTED), CameraParameters::TRUE) == 0)
        {
            if ((valstr = params.get(ActCameraParameters::KEY_SATURATION)) != NULL)
            {
                int min, max,step, v;
                const char *p;

                p = mCameraProperties->get(CameraProperties::SUPPORTED_SATURATION_MIN);
                min = atoi(p);
                p = mCameraProperties->get(CameraProperties::SUPPORTED_SATURATION_MAX);
                max = atoi(p);
                p = mCameraProperties->get(CameraProperties::SUPPORTED_SATURATION_STEP);
                step = atoi(p);
                v = atoi(valstr);

                if(v>=min && v<=max)
                {
                    CAMHAL_LOGDB("Saturation set %s", valstr);
                    mParameters.set(ActCameraParameters::KEY_SATURATION, valstr);
                }
                else
                {
                    CAMHAL_LOGEB("ERROR: Invalid contrast  = %s", valstr);
                }
            }
        }

        if(strcmp(mCameraProperties->get(CameraProperties::BRIGHTNESS_SUPPORTED), CameraParameters::TRUE) == 0)
        {
            if ((valstr = params.get(ActCameraParameters::KEY_BRIGHTNESS)) != NULL)
            {
                int min, max,step, v;
                const char *p;

                p = mCameraProperties->get(CameraProperties::SUPPORTED_BRIGHTNESS_MIN);
                min = atoi(p);
                p = mCameraProperties->get(CameraProperties::SUPPORTED_BRIGHTNESS_MAX);
                max = atoi(p);
                p = mCameraProperties->get(CameraProperties::SUPPORTED_BRIGHTNESS_STEP);
                step = atoi(p);
                v = atoi(valstr);

                if(v>=min && v<=max)
                {
                    CAMHAL_LOGDB("Brightness set %s", valstr);
                    mParameters.set(ActCameraParameters::KEY_BRIGHTNESS, valstr);
                }
                else
                {
                    CAMHAL_LOGEB("ERROR: Invalid brightness  = %s", valstr);
                }
            }
        }

        if(strcmp(mCameraProperties->get(CameraProperties::DENOISE_SUPPORTED), CameraParameters::TRUE) == 0)
        {
            if( (valstr = params.get(ActCameraParameters::KEY_DENOISE)) != NULL)
            {
                int min, max,step, v;
                const char *p;

                p = mCameraProperties->get(CameraProperties::SUPPORTED_DENOISE_MIN);
                min = atoi(p);
                p = mCameraProperties->get(CameraProperties::SUPPORTED_DENOISE_MAX);
                max = atoi(p);
                p = mCameraProperties->get(CameraProperties::SUPPORTED_DENOISE_STEP);
                step = atoi(p);
                v = atoi(valstr);

                if(v>=min && v<=max)
                {
                    CAMHAL_LOGDB("De Noise set %s", valstr);
                    mParameters.set(ActCameraParameters::KEY_DENOISE, valstr);
                }
                else
                {
                    CAMHAL_LOGEB("ERROR: Invalid denoise  = %s", valstr);
                }
            }
        }

        if ((valstr = params.get(CameraParameters::KEY_ANTIBANDING)) != NULL)
        {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_ANTIBANDING)))
            {
                CAMHAL_LOGDB("Antibanding set %s", valstr);
                mParameters.set(CameraParameters::KEY_ANTIBANDING, valstr);
            }
            else
            {
                CAMHAL_LOGDB("ERROR: Invalid Antibanding = %s", valstr);
				//Fix for Android L
                //return BAD_VALUE;
            }
        }

        if(strcmp(mCameraProperties->get(CameraProperties::ISO_SUPPORTED), CameraParameters::TRUE) == 0)
        {
            if ((valstr = params.get(ActCameraParameters::KEY_ISO)) != NULL) {
                if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_ISO_VALUES))) {
                    CAMHAL_LOGDB("ISO set %s", valstr);
                    mParameters.set(ActCameraParameters::KEY_ISO, valstr);
                } else {
                    CAMHAL_LOGEB("ERROR: Invalid ISO = %s", valstr);
                    return BAD_VALUE;
                }
            }
        }

        if( (valstr = params.get(CameraParameters::KEY_FOCUS_AREAS)) != NULL )
        {
            CAMHAL_LOGDB("Focus areas position set %s",valstr);
            mParameters.set(CameraParameters::KEY_FOCUS_AREAS, valstr);
        }


        if( (valstr = params.get(CameraParameters::KEY_EXPOSURE_COMPENSATION)) != NULL)
        {
            CAMHAL_LOGDB("Exposure compensation set %s", valstr);
            mParameters.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, valstr);
        }

        if ((valstr = params.get(CameraParameters::KEY_SCENE_MODE)) != NULL)
        {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_SCENE_MODES)))
            {
                CAMHAL_LOGDB("Scene mode set %s", valstr);
                doesSetParameterNeedUpdate(valstr,
                                           mParameters.get(CameraParameters::KEY_SCENE_MODE),
                                           updateRequired);
                mParameters.set(CameraParameters::KEY_SCENE_MODE, valstr);
            }
            else
            {
                CAMHAL_LOGEB("ERROR: Invalid Scene mode = %s", valstr);
                return BAD_VALUE;
            }
        }

        if(strcmp(mCameraProperties->get(CameraProperties::FLASH_SUPPORTED), CameraParameters::TRUE) == 0)
        {
            if ((valstr = params.get(CameraParameters::KEY_FLASH_MODE)) != NULL)
            {
                if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_FLASH_MODES)))
                {
                    CAMHAL_LOGDB("Flash mode set %s", valstr);
                    mParameters.set(CameraParameters::KEY_FLASH_MODE, valstr);
                }
                else
                {
                    CAMHAL_LOGEB("ERROR: Invalid Flash mode = %s", valstr);
                    return BAD_VALUE;
                }
            }
        }

        if ((valstr = params.get(CameraParameters::KEY_EFFECT)) != NULL)
        {
            if (isParameterValid(valstr, mCameraProperties->get(CameraProperties::SUPPORTED_EFFECTS)))
            {
                CAMHAL_LOGDB("Effect set %s", valstr);
                mParameters.set(CameraParameters::KEY_EFFECT, valstr);
            }
            else
            {
                CAMHAL_LOGEB("ERROR: Invalid Effect = %s", valstr);
                return BAD_VALUE;
            }
        }

        varint = params.getInt(CameraParameters::KEY_ROTATION);
        if( varint >=0 )
        {
            CAMHAL_LOGDB("Rotation set %d", varint);
            mParameters.set(CameraParameters::KEY_ROTATION, varint);
        }

        varint = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
        if( varint >= 0 )
        {
            CAMHAL_LOGDB("Jpeg quality set %d", varint);
            mParameters.set(CameraParameters::KEY_JPEG_QUALITY, varint);
        }

        varint = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
        if( varint >=0 )
        {
            CAMHAL_LOGDB("Thumbnail width set %d", varint);
            mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, varint);
        }

        varint = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
        if( varint >=0 )
        {
            CAMHAL_LOGDB("Thumbnail width set %d", varint);
            mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, varint);
        }

        varint = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
        if( varint >=0 )
        {
            CAMHAL_LOGDB("Thumbnail quality set %d", varint);
            mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, varint);
        }

        if( (valstr = params.get(CameraParameters::KEY_GPS_LATITUDE)) != NULL )
        {
            CAMHAL_LOGDB("GPS latitude set %s", valstr);
            mParameters.set(CameraParameters::KEY_GPS_LATITUDE, valstr);
        }
        else
        {
            mParameters.remove(CameraParameters::KEY_GPS_LATITUDE);
        }

        if( (valstr = params.get(CameraParameters::KEY_GPS_LONGITUDE)) != NULL )
        {
            CAMHAL_LOGDB("GPS longitude set %s", valstr);
            mParameters.set(CameraParameters::KEY_GPS_LONGITUDE, valstr);
        }
        else
        {
            mParameters.remove(CameraParameters::KEY_GPS_LONGITUDE);
        }

        if( (valstr = params.get(CameraParameters::KEY_GPS_ALTITUDE)) != NULL )
        {
            CAMHAL_LOGDB("GPS altitude set %s", valstr);
            mParameters.set(CameraParameters::KEY_GPS_ALTITUDE, valstr);
        }
        else
        {
            mParameters.remove(CameraParameters::KEY_GPS_ALTITUDE);
        }

        if( (valstr = params.get(CameraParameters::KEY_GPS_TIMESTAMP)) != NULL )
        {
            CAMHAL_LOGDB("GPS timestamp set %s", valstr);
            mParameters.set(CameraParameters::KEY_GPS_TIMESTAMP, valstr);
        }
        else
        {
            mParameters.remove(CameraParameters::KEY_GPS_TIMESTAMP);
        }

        if( (valstr = params.get(ActCameraParameters::KEY_GPS_DATESTAMP)) != NULL )
        {
            CAMHAL_LOGDB("GPS datestamp set %s", valstr);
            mParameters.set(ActCameraParameters::KEY_GPS_DATESTAMP, valstr);
        }
        else
        {
            mParameters.remove(ActCameraParameters::KEY_GPS_DATESTAMP);
        }

        if( (valstr = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD)) != NULL )
        {
            CAMHAL_LOGDB("GPS processing method set %s", valstr);
            mParameters.set(CameraParameters::KEY_GPS_PROCESSING_METHOD, valstr);
        }
        else
        {
            mParameters.remove(CameraParameters::KEY_GPS_PROCESSING_METHOD);
        }

        if( (valstr = params.get(ActCameraParameters::KEY_GPS_MAPDATUM )) != NULL )
        {
            CAMHAL_LOGDB("GPS MAPDATUM set %s", valstr);
            mParameters.set(ActCameraParameters::KEY_GPS_MAPDATUM, valstr);
        }
        else
        {
            mParameters.remove(ActCameraParameters::KEY_GPS_MAPDATUM);
        }

        if( (valstr = params.get(ActCameraParameters::KEY_GPS_VERSION)) != NULL )
        {
            CAMHAL_LOGDB("GPS MAPDATUM set %s", valstr);
            mParameters.set(ActCameraParameters::KEY_GPS_VERSION, valstr);
        }
        else
        {
            mParameters.remove(ActCameraParameters::KEY_GPS_VERSION);
        }

        if( (valstr = params.get(ActCameraParameters::KEY_EXIF_MODEL)) != NULL )
        {
            CAMHAL_LOGDB("EXIF Model set %s", valstr);
            mParameters.set(ActCameraParameters::KEY_EXIF_MODEL, valstr);
        }

        if( (valstr = params.get(ActCameraParameters::KEY_EXIF_MAKE)) != NULL )
        {
            CAMHAL_LOGDB("EXIF Make set %s", valstr);
            mParameters.set(ActCameraParameters::KEY_EXIF_MAKE, valstr);
        }

        valstr = params.get(CameraParameters::KEY_ZOOM);
        if ( valstr != NULL && (strchr(valstr,'.')==0))
        {
            varint = params.getInt(CameraParameters::KEY_ZOOM);
            if ( ( varint >= 0 ) && ( varint <= mMaxZoomSupported ) )
            {
                CAMHAL_LOGDB("Zoom set %s", valstr);
                doesSetParameterNeedUpdate(valstr,
                                           mParameters.get(CameraParameters::KEY_ZOOM),
                                           updateRequired);
                mParameters.set(CameraParameters::KEY_ZOOM, valstr);
            }
            else
            {
                CAMHAL_LOGEB("ERROR: Invalid Zoom: %s", valstr);
                return BAD_VALUE;
            }
        }

        if( (valstr = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK)) != NULL )
        {
            CAMHAL_LOGDB("Auto Exposure Lock set %s", valstr);
            doesSetParameterNeedUpdate(valstr,
                                       mParameters.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK),
                                       updateRequired);
            mParameters.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, valstr);
        }

        if( (valstr = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK)) != NULL )
        {
            CAMHAL_LOGDB("Auto WhiteBalance Lock set %s", valstr);
            doesSetParameterNeedUpdate(valstr,
                                       mParameters.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK),
                                       updateRequired);
            mParameters.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, valstr);
        }
        if( (valstr = params.get(CameraParameters::KEY_METERING_AREAS)) != NULL )
        {
            CAMHAL_LOGDB("Metering areas position set %s", valstr);
            mParameters.set(CameraParameters::KEY_METERING_AREAS, valstr);
        }

        //always setParameters to camera adapter, avoiding parameter inconsistence when getparameter
        //if ( (NULL != mCameraAdapter) && (mPreviewEnabled || updateRequired) )
        if ( (NULL != mCameraAdapter)  )
        {
            ret |= mCameraAdapter->setParameters(mParameters);
        }

    }

    //On fail restore old parameters
    if ( NO_ERROR != ret )
    {
        mParameters = oldParams;
    }

    // Restart Preview if needed by KEY_RECODING_HINT only if preview is already running.
    // If preview is not started yet, Video Mode parameters will take effect on next startPreview()
    if (restartPreviewRequired && previewEnabledInner() && !mRecordingEnabled)
    {
        CAMHAL_LOGDA("Restarting Preview");
        ret = restartPreview();
    }
    else if (restartPreviewRequired && !previewEnabledInner() &&
             mDisplayPaused && !mRecordingEnabled)
    {
        CAMHAL_LOGDA("Stopping Preview");
        forceStopPreview();
    }

    if (ret != NO_ERROR)
    {
        CAMHAL_LOGEA("Failed to restart Preview");
        return ret;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::allocPreviewBufs(int width, int height, const char* previewFormat,
                                     unsigned int buffercount, unsigned int &max_queueable)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if(mDisplayAdapter.get() == NULL)
    {
        // Memory allocation of preview buffers is now placed in gralloc
        // CameraHal should not allocate preview buffers without DisplayAdapter
        return NO_MEMORY;
    }

    if(!mPreviewBufs)
    {
        ///@todo Pluralise the name of this method to allocateBuffers
        mPreviewLength = 0;
        mPreviewBufs = (int32_t *) mDisplayAdapter->allocateBuffer(width, height,
                       previewFormat,
                       mPreviewLength,
                       buffercount);// 1 

        if (NULL == mPreviewBufs )
        {
            CAMHAL_LOGEA("Couldn't allocate preview buffers");
            return NO_MEMORY;
        }

        mPreviewOffsets = (uint32_t *) mDisplayAdapter->getOffsets();// 2得到容器
        if ( NULL == mPreviewOffsets )
        {
            CAMHAL_LOGEA("Buffer mapping failed");
            return BAD_VALUE;
        }

#if 0
        mPreviewVaddrs = (void *) mDisplayAdapter->getVaddrs();
        if ( NULL == mPreviewOffsets )
        {
            CAMHAL_LOGEA("Buffer mapping failed");
            return BAD_VALUE;
        }
#else
        mPreviewVaddrs = NULL;
#endif

        mBufProvider = (BufferProvider*) mDisplayAdapter.get();

        ret = mDisplayAdapter->maxQueueableBuffers(max_queueable);// 3
        if (ret != NO_ERROR)
        {
            return ret;
        }

    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;

}

status_t CameraHal::freePreviewBufs()
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    CAMHAL_LOGDB("mPreviewBufs = 0x%x", (unsigned int)mPreviewBufs);
    if(mPreviewBufs)
    {
        ///@todo Pluralise the name of this method to freeBuffers
        mBufProvider = (BufferProvider*) mDisplayAdapter.get();
        if(mBufProvider != NULL)
        {
            ret = mBufProvider->freeBuffer(mPreviewBufs);
        }
        mPreviewBufs = NULL;
        LOG_FUNCTION_NAME_EXIT;
        return ret;
    }
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}


status_t CameraHal::allocImageBufs(unsigned int width, unsigned int height, size_t size, const char* format, unsigned int bufferCount)
{

    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    if( NULL != mImageBufs )
    {
        //ret = freeImageBufs();
        //mImageBufs = NULL;
        return NO_ERROR;
    }

    if ( NO_ERROR == ret )
    {
        int32_t stride;
        buffer_handle_t *bufsArr = new buffer_handle_t [bufferCount];
        int halFormat;
        halFormat = CameraFrame::getHalFormat(format);
        if (bufsArr != NULL)
        {
            for (unsigned int i = 0; i< bufferCount; i++)
            {
                GraphicBufferAllocator &GrallocAlloc = GraphicBufferAllocator::get();
                buffer_handle_t buf;
                ret = GrallocAlloc.alloc(SIZE_ALIGN_UP_32(width), height, halFormat, CAMHAL_GRALLOC_USAGE, &buf, &stride);
                if (ret != NO_ERROR)
                {
                    CAMHAL_LOGEA("Couldn't allocate video buffers using Gralloc");
                    ret = -NO_MEMORY;
                    for (unsigned int j=0; j< i; j++)
                    {
                        buf = (buffer_handle_t)bufsArr[j];
                        GrallocAlloc.free(buf);
                    }
                    delete [] bufsArr;
                    goto exit;
                }
                bufsArr[i] = buf;
                CAMHAL_LOGVB("*** Gralloc Handle =%p ***", buf);
            }

            mImageBufs = (int32_t *)bufsArr;
            mImageBufCount = bufferCount;
            mImageLength =CameraFrame::getFrameLength(CameraFrame::getFrameFormat(format), width, height);  
        }
        else
        {
            CAMHAL_LOGEA("Couldn't allocate video buffers ");
            ret = -NO_MEMORY;
        }
    }

exit:
    LOG_FUNCTION_NAME;

    return ret;

}

status_t CameraHal::allocVideoBufs(uint32_t width, uint32_t height, const char *format, uint32_t bufferCount)
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    if( NULL != mVideoBufs )
    {
        ret = freeVideoBufs(mVideoBufs);
        mVideoBufs = NULL;
    }

    if ( NO_ERROR == ret )
    {
        int32_t stride;
        buffer_handle_t *bufsArr = new buffer_handle_t [bufferCount];
        int halFormat;
        halFormat = CameraFrame::getHalFormat(format);
        if (bufsArr != NULL)
        {
            for (unsigned int i = 0; i< bufferCount; i++)
            {
                GraphicBufferAllocator &GrallocAlloc = GraphicBufferAllocator::get();
                buffer_handle_t buf;
                ret = GrallocAlloc.alloc(width, height, halFormat, CAMHAL_GRALLOC_USAGE, &buf, &stride);
                if (ret != NO_ERROR)
                {
                    CAMHAL_LOGEA("Couldn't allocate video buffers using Gralloc");
                    ret = -NO_MEMORY;
                    for (unsigned int j=0; j< i; j++)
                    {
                        buf = (buffer_handle_t)bufsArr[j];
                        GrallocAlloc.free(buf);
                    }
                    delete [] bufsArr;
                    goto exit;
                }
                bufsArr[i] = buf;
                CAMHAL_LOGVB("*** Gralloc Handle =%p ***", buf);
            }
            mVideoBufs = (int32_t *)bufsArr;
        }
        else
        {
            CAMHAL_LOGEA("Couldn't allocate video buffers ");
            ret = -NO_MEMORY;
        }
    }

exit:
    LOG_FUNCTION_NAME;

    return ret;
}

void _endImageCapture(void *param)
{
    LOG_FUNCTION_NAME;
    if ( NULL != param )
    {
        CameraHal *c = reinterpret_cast<CameraHal *>(param);
        c->signalEndImageCapture();
    }
    LOG_FUNCTION_NAME_EXIT;
}

void endImageCapture( void *userData)
{
    LOG_FUNCTION_NAME;

    _endImageCapture(userData);

    LOG_FUNCTION_NAME_EXIT;
}


void releaseImageBuffers(void *userData)
{
    LOG_FUNCTION_NAME;

#ifndef CAMERA_FIX_IMAGE_BUFFER

    if (NULL != userData)
    {
        CameraHal *c = reinterpret_cast<CameraHal *>(userData);
        c->freeImageBufs();
    }
#endif

    LOG_FUNCTION_NAME_EXIT;
}

status_t CameraHal::signalEndImageCapture()
{
    status_t ret = NO_ERROR;
    int w,h;
    CameraParameters adapterParams = mParameters;


    LOG_FUNCTION_NAME;
    Mutex::Autolock lock(mLock);

    if(!(mCameraAdapter->getState()& CameraAdapter::CAPTURE_ACTIVE))
    {
        return ret;
    }

    mAppCallbackNotifier->stopImageCallbacks();
    mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_IMAGE_CAPTURE);
    mTakePictureWait.signal();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::freeImageBufs()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if(mImageBufs == NULL)
    {
        return ret;
    }

    if(mAppCallbackNotifier.get())
    {
        mAppCallbackNotifier->waitforImageBufs();
    }
    buffer_handle_t *pBuf = (buffer_handle_t*)mImageBufs;

    GraphicBufferAllocator &GrallocAlloc = GraphicBufferAllocator::get();
    for(unsigned int i = 0; i < mImageBufCount; i++)
    {
        buffer_handle_t ptr = *pBuf++;
        GrallocAlloc.free(ptr);
    }
    delete[] mImageBufs;
    mImageBufs = NULL;
    mImageBufCount = 0;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::freeVideoBufs(void *bufs)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    buffer_handle_t *pBuf = (buffer_handle_t*)bufs;
    int count = atoi(mCameraProperties->get(CameraProperties::REQUIRED_PREVIEW_BUFS));
    if(pBuf == NULL)
    {
        CAMHAL_LOGEA("NULL pointer passed to freeVideoBuffer");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }

    GraphicBufferAllocator &GrallocAlloc = GraphicBufferAllocator::get();

    for(int i = 0; i < count; i++)
    {
        buffer_handle_t ptr = *pBuf++;
        CAMHAL_LOGVB("Free Video Gralloc Handle %p", ptr);
        GrallocAlloc.free(ptr);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
   @brief Start preview mode.

   @param none
   @return NO_ERROR Camera switched to VF mode
   @todo Update function header with the different errors that are possible

 */
/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t CameraHal::startPreview()
{

    status_t ret = NO_ERROR;
    CameraAdapter::BuffersDescriptor desc;
    CameraFrame frame;
    const char *valstr = NULL;
    unsigned int required_buffer_count;
    unsigned int max_queueble_buffers;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
    gettimeofday(&mStartPreview, NULL);
#endif

    LOG_FUNCTION_NAME;

    CAMERA_SCOPEDTRACE("startPreview");

    mPreviewCBEnabled = true;    

    if ( mPreviewEnabled )
    {
        CAMHAL_LOGDA("Preview already running");
        ALOGD("Preview already running");
        LOG_FUNCTION_NAME_EXIT;
        return ALREADY_EXISTS;
    }
	//
    if ( NULL != mCameraAdapter )
    {
        ret = mCameraAdapter->setParameters(mParameters);
    }
	
	//
    required_buffer_count = atoi(mCameraProperties->get(CameraProperties::REQUIRED_PREVIEW_BUFS));
    CAMHAL_LOGDB("required_buffer_count %d", required_buffer_count);

    if ((mPreviewStartInProgress == false) && (mDisplayPaused == false))
    {
        //ActionsCode(author:liuyiguang, change_code)
        //ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_QUERY_RESOLUTION_PREVIEW,( int ) &frame, required_buffer_count);
        ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_QUERY_RESOLUTION_PREVIEW,( long ) &frame, required_buffer_count);
        if ( NO_ERROR != ret )
        {
            CAMHAL_LOGEB("Error: CAMERA_QUERY_RESOLUTION_PREVIEW %d", ret);
            return ret;
        }

        ///Update the current preview width and height
        mPreviewWidth = frame.mWidth;
        mPreviewHeight = frame.mHeight;
        CAMHAL_LOGDB("CAMERA_QUERY_RESOLUTION_PREVIEW %d,%d",mPreviewWidth, mPreviewHeight);
    }

    ///If we don't have the preview callback enabled and display adapter,
    if(!mSetPreviewWindowCalled || (mDisplayAdapter.get() == NULL))
    {
        CAMHAL_LOGDA("Preview not started. Preview in progress flag set");
        mPreviewStartInProgress = true;
        //ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_SWITCH_TO_EXECUTING);
        
        if ( NO_ERROR != ret )
        {
            CAMHAL_LOGEB("Error: CAMERA_SWITCH_TO_EXECUTING %d", ret);
            return ret;
        }
        return NO_ERROR;
    }
	//
    if( (mDisplayAdapter.get() != NULL) && ( !mPreviewEnabled ) && ( mDisplayPaused ) )
    {
        CAMHAL_LOGDA("Preview is in paused state");

        mDisplayPaused = false;
        mPreviewEnabled = true;
        if ( NO_ERROR == ret )
        {
            ret = mDisplayAdapter->pauseDisplay(mDisplayPaused);

            if ( NO_ERROR != ret )
            {
                CAMHAL_LOGEB("Display adapter resume failed %x", ret);
            }
        }
        //restart preview callbacks
        if(mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME)
        {
            mAppCallbackNotifier->enableMsgType (CAMERA_MSG_PREVIEW_FRAME);
        }
        return ret;
    }



    ///Allocate the preview buffers
    ret = allocPreviewBufs(mPreviewWidth, mPreviewHeight, mMappedPreviewFormat, required_buffer_count, max_queueble_buffers);

    if ( NO_ERROR != ret )
    {
        CAMHAL_LOGEA("Couldn't allocate buffers for Preview");
        goto error;
    }


    ///Pass the buffers to Camera Adapter
    desc.mBuffers = mPreviewBufs;//
    desc.mVaddrs = mPreviewVaddrs;//
    desc.mOffsets = mPreviewOffsets;//
    desc.mFd = mPreviewFd;
    desc.mLength = mPreviewLength;
    desc.mCount = ( size_t ) required_buffer_count;
    desc.mMaxQueueable = (size_t) max_queueble_buffers;

    CAMHAL_LOGVB("mPreviewLength =%d",mPreviewLength);
    //ActionsCode(author:liuyiguang, change_code)
    //ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS_PREVIEW,
    //                                  ( int ) &desc);
    ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS_PREVIEW,
                                      ( long ) &desc);//--将buf提交到omx和驱动

    if ( NO_ERROR != ret )
    {
        CAMHAL_LOGEB("Failed to register preview buffers: 0x%x", ret);
        freePreviewBufs();
        return ret;
    }

    mAppCallbackNotifier->startPreviewCallbacks(mParameters, mPreviewBufs, mPreviewOffsets, mPreviewFd, mPreviewLength, required_buffer_count);

    ///Start the callback notifier
    ret = mAppCallbackNotifier->start();

    if( ALREADY_EXISTS == ret )
    {
        //Already running, do nothing
        CAMHAL_LOGDA("AppCallbackNotifier already running");
        ret = NO_ERROR;
    }
    else if ( NO_ERROR == ret )
    {
        CAMHAL_LOGDA("Started AppCallbackNotifier..");
        mAppCallbackNotifier->setMeasurements(mMeasurementEnabled);
    }
    else
    {
        CAMHAL_LOGDA("Couldn't start AppCallbackNotifier");
        goto error;
    }

    ///Enable the display adapter if present, actual overlay enable happens when we post the buffer
    if(mDisplayAdapter.get() != NULL)
    {
        CAMHAL_LOGDA("Enabling display");
        bool isS3d = false;
        DisplayAdapter::S3DParameters s3dParams;
        int width, height;


#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

        ret = mDisplayAdapter->enableDisplay(mPreviewWidth, mPreviewHeight, &mStartPreview, isS3d ? &s3dParams : NULL);

#else

        ret = mDisplayAdapter->enableDisplay(mPreviewWidth, mPreviewHeight, NULL, isS3d ? &s3dParams : NULL);

#endif

        if ( ret != NO_ERROR )
        {
            CAMHAL_LOGEA("Couldn't enable display");
            goto error;
        }

    }

    ///Send START_PREVIEW command to adapter
    CAMHAL_LOGDA("Starting CameraAdapter preview mode");

    ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_PREVIEW);

    if(ret!=NO_ERROR)
    {
        CAMHAL_LOGEA("Couldn't start preview w/ CameraAdapter");
        goto error;
    }
    CAMHAL_LOGDA("Started preview");

    mPreviewEnabled = true;
    mPreviewStartInProgress = false;
    return ret;

error:

    CAMHAL_LOGEA("Performing cleanup after error");

    //Do all the cleanup
    mAppCallbackNotifier->stop();
    mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_PREVIEW);
    if(mDisplayAdapter.get() != NULL)
    {
        mDisplayAdapter->disableDisplay(false);
    }
    freePreviewBufs();
    freeImageBufs();
    mPreviewStartInProgress = false;
    mPreviewEnabled = false;

#ifdef CAMERA_IGNORE_STOPPREVIEW_AFTER_CAPTURE
    mAfterTakePicture = false;
#endif


    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
   @brief Sets ANativeWindow object.

   Preview buffers provided to CameraHal via this object. DisplayAdapter will be interfacing with it
   to render buffers to display.

   @param[in] window The ANativeWindow object created by Surface flinger
   @return NO_ERROR If the ANativeWindow object passes validation criteria
   @todo Define validation criteria for ANativeWindow object. Define error codes for scenarios

 */
status_t CameraHal::setPreviewWindow(struct preview_stream_ops *window)
{
    status_t ret = NO_ERROR;
    CameraAdapter::BuffersDescriptor desc;

    LOG_FUNCTION_NAME;
    mSetPreviewWindowCalled = true;

    CAMERA_SCOPEDTRACE("setPreviewWindow");

    ///If the Camera service passes a null window, we destroy existing window and free the DisplayAdapter
    if(!window)
    {
        if(mDisplayAdapter.get() != NULL)
        {
            if(mAppCallbackNotifier.get())
            {
                mAppCallbackNotifier->waitforImageBufs();
            }
            ///NULL window passed, destroy the display adapter if present
            CAMHAL_LOGDA("NULL window passed, destroying display adapter");
            mDisplayAdapter.clear();
            ///@remarks If there was a window previously existing, we usually expect another valid window to be passed by the client
            ///@remarks so, we will wait until it passes a valid window to begin the preview again
            mSetPreviewWindowCalled = false;
        }
        CAMHAL_LOGDA("NULL ANativeWindow passed to setPreviewWindow");
        return NO_ERROR;
    }
    else if(mDisplayAdapter.get() == NULL)
    {
        // Need to create the display adapter since it has not been created
        // Create display adapter
        mDisplayAdapter = new ANativeWindowDisplayAdapter();
        ret = NO_ERROR;
        if(!mDisplayAdapter.get() || ((ret=mDisplayAdapter->initialize())!=NO_ERROR))
        {
            if(ret!=NO_ERROR)
            {
                mDisplayAdapter.clear();
                CAMHAL_LOGEA("DisplayAdapter initialize failed");
                LOG_FUNCTION_NAME_EXIT;
                return ret;
            }
            else
            {
                CAMHAL_LOGEA("Couldn't create DisplayAdapter");
                LOG_FUNCTION_NAME_EXIT;
                return NO_MEMORY;
            }
        }

        // DisplayAdapter needs to know where to get the CameraFrames from inorder to display
        // Since CameraAdapter is the one that provides the frames, set it as the frame provider for DisplayAdapter
        mDisplayAdapter->setFrameProvider(mCameraAdapter);

        // Any dynamic errors that happen during the camera use case has to be propagated back to the application
        // via CAMERA_MSG_ERROR. AppCallbackNotifier is the class that  notifies such errors to the application
        // Set it as the error handler for the DisplayAdapter
        mDisplayAdapter->setErrorHandler(mAppCallbackNotifier.get());

        // Update the display adapter with the new window that is passed from CameraService
        ret  = mDisplayAdapter->setPreviewWindow(window);
        if(ret!=NO_ERROR)
        {
            CAMHAL_LOGEB("DisplayAdapter setPreviewWindow returned error %d", ret);
        }

        if(mPreviewStartInProgress)
        {
            CAMHAL_LOGDA("setPreviewWindow called when preview running");
            // Start the preview since the window is now available
            ret = startPreview();
        }
    }
    else
    {    
        bool restart_preview = false;
        if (previewEnabledInner()) {
            ALOGD("preview should be stopped before setting new window");
            forceStopPreview();
            restart_preview = true;

        }
        // Update the display adapter with the new window that is passed from CameraService
        ret = mDisplayAdapter->setPreviewWindow(window);
        if (!previewEnabledInner()&&restart_preview){
            ALOGD("preview restared after new window is set");
            ret = startPreview();
            if (ret) {
                CAMHAL_LOGEA("start preview failed");
            }
            restart_preview = false;
        }
    }
    LOG_FUNCTION_NAME_EXIT;

    return ret;

}


/**
   @brief Stop a previously started preview.

   @param none
   @return none

 */
/**
 *
 * BUGFIX: Call cancelPicture while stopPreview, if it is not in takePicture, this will be no-ops. 
 *
 ************************************
 *
 * ActionsCode(author:liuyiguang, change_code)
 */
void CameraHal::stopPreview()
{
    LOG_FUNCTION_NAME;

    if( (!previewEnabledInner() && !mDisplayPaused) || mRecordingEnabled)
    {
        LOG_FUNCTION_NAME_EXIT;
        return;
    }

    bool imageCaptureRunning = (mCameraAdapter->getState()& CameraAdapter::CAPTURE_ACTIVE) &&
                               !(mCameraAdapter->getNextState() & CameraAdapter::CAPTURE_ACTIVE);
    if(mDisplayPaused && !imageCaptureRunning)
    {
        // Display is paused, which essentially means there is no preview active.
        // Note: this is done so that when stopPreview is called by client after
        // an image capture, we do not de-initialize the camera adapter and
        // restart over again.

        return;
    }


    CAMERA_SCOPEDTRACE("stopPreview");

#ifdef CAMERA_IGNORE_STOPPREVIEW_AFTER_CAPTURE
    if(mAfterTakePicture)
    {
        if (mCameraAdapter->getState() & CameraAdapter::PREVIEW_ACTIVE)
        {
            // according to javadoc...FD should be stopped in stopPreview
            // and application needs to call startFaceDection again
            // to restart FD
            mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_FD);
        }
        mCameraAdapter->rollbackToPreviewState();
        mAfterTakePicture = false;
        CAMHAL_LOGDA("stopPreview after takepicture,just return");
        return;
    }
#endif
    
    forceStopPreview();

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Returns true if preview is enabled

   @param none
   @return true If preview is running currently
         false If preview has been stopped

 */
bool CameraHal::previewEnabledInner()
{
    LOG_FUNCTION_NAME;

    return (mPreviewEnabled || mPreviewStartInProgress);
}

bool CameraHal::previewEnabled()
{
    LOG_FUNCTION_NAME;
    bool ret = (mPreviewEnabled || mPreviewStartInProgress);
    if(ret)
    {
        mPreviewCBEnabled = true;
    }

    return ret;
}

/**
   @brief Start record mode.

  When a record image is available a CAMERA_MSG_VIDEO_FRAME message is sent with
  the corresponding frame. Every record frame must be released by calling
  releaseRecordingFrame().

   @param none
   @return NO_ERROR If recording could be started without any issues
   @todo Update the header with possible error values in failure scenarios

 */
status_t CameraHal::startRecording( )
{
    int w, h;
    const char *valstr = NULL;
    bool restartPreviewRequired = false;
    status_t ret = NO_ERROR;
    int count = 0;

    LOG_FUNCTION_NAME;


#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    gettimeofday(&mStartPreview, NULL);

#endif

    if(!previewEnabledInner())
    {
        return NO_INIT;
    }

    if (restartPreviewRequired)
    {
        ret = restartPreview();
    }


    //FIXME wait for capture to preview state, the statemachine maybe failed from capture to recording
    count = 0;
    while((mCameraAdapter->getState()& CameraAdapter::CAPTURE_ACTIVE))
    {   
        if(count++>=100)
        {
            break;
        }
        usleep(10000); //10ms
    }

    if ( NO_ERROR == ret )
    {
        int count = atoi(mCameraProperties->get(CameraProperties::REQUIRED_PREVIEW_BUFS));
        CAMHAL_LOGDB("Video Width=%d Height=%d", mVideoWidth, mVideoHeight);
        ALOGD("mUseVideoBuffers=%d", mUseVideoBuffers);

	/*
	* Condition1: MetaDataBufferMode used and extra ion videobuf is needed
	* ActionsCode(author:liyuan, add comments)
	*/
        if(mUseVideoBuffers && mAppCallbackNotifier->getMetaDataBufferMode())
        {
            //TODO:mVideoFormat
            ret = allocVideoBufs(mVideoWidth, mVideoHeight, mVideoFormat, count);
            if ( NO_ERROR != ret )
            {
                return ret;
            }
            mAppCallbackNotifier->useVideoBuffers(true);
            mAppCallbackNotifier->setVideoRes(mVideoWidth, mVideoHeight);
            //mPreviewFd is unused
            ret = mAppCallbackNotifier->initSharedVideoBuffers(mParameters, mPreviewBufs, mPreviewOffsets, mPreviewFd, mPreviewLength, count, mVideoBufs);

        }
        else
        {
	    /*
	    * Condition2: MetaDataBufferMode used and no extra ion videobuf is needed
	    * Condition3: Non MetaDataBufferMode used
	    * ActionsCode(author:liyuan, add comments)
	    */
            mAppCallbackNotifier->useVideoBuffers(false);
            mAppCallbackNotifier->setVideoRes(mVideoWidth, mVideoHeight);
            //mPreviewFd is unused
            ret = mAppCallbackNotifier->initSharedVideoBuffers(mParameters, mPreviewBufs, mPreviewOffsets, mPreviewFd, mPreviewLength, count, NULL);
        }
    }

    if ( NO_ERROR == ret )
    {
        ret = mAppCallbackNotifier->startRecording();
    }

    if ( NO_ERROR == ret )
    {
        ///Buffers for video capture (if different from preview) are expected to be allocated within CameraAdapter
        ret =  mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_VIDEO);
    }

    if ( NO_ERROR == ret )
    {
        mRecordingEnabled = true;
    }

#ifdef CAMERA_IGNORE_STOPPREVIEW_AFTER_CAPTURE
    mAfterTakePicture = false;
#endif
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
   @brief Set the camera parameters specific to Video Recording.

   This function checks for the camera parameters which have to be set for recording.
   Video Recording needs CAPTURE_MODE to be VIDEO_MODE. This function sets it.
   This function also enables Video Recording specific functions like VSTAB & VNF.

   @param none
   @return true if preview needs to be restarted for VIDEO_MODE parameters to take effect.
   @todo Modify the policies for enabling VSTAB & VNF usecase based later.

 */
bool CameraHal::setVideoModeParameters(const CameraParameters& params)
{
    const char *valstr = NULL;
    bool restartPreviewRequired = false;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;


    LOG_FUNCTION_NAME_EXIT;

    return restartPreviewRequired;
}

/**
   @brief Reset the camera parameters specific to Video Recording.

   This function resets CAPTURE_MODE and disables Recording specific functions like VSTAB & VNF.

   @param none
   @return true if preview needs to be restarted for VIDEO_MODE parameters to take effect.

 */
bool CameraHal::resetVideoModeParameters()
{
    const char *valstr = NULL;
    bool restartPreviewRequired = false;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    // ignore this if we are already recording
    if (mRecordingEnabled)
    {
        return false;
    }

    LOG_FUNCTION_NAME_EXIT;

    return restartPreviewRequired;
}

/**
   @brief Restart the preview with setParameter.

   This function restarts preview, for some VIDEO_MODE parameters to take effect.

   @param none
   @return NO_ERROR If recording parameters could be set without any issues

 */
status_t CameraHal::restartPreview()
{
    const char *valstr = NULL;
    char tmpvalstr[30];
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    forceStopPreview();

    {
        Mutex::Autolock lock(mLock);
        mCameraAdapter->setParameters(mParameters);
    }

    ret = startPreview();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
   @brief Stop a previously started recording.

   @param none
   @return none

 */
void CameraHal::stopRecording()
{
    CameraAdapter::AdapterState currentState;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mLock);

    if (!mRecordingEnabled )
    {
        return;
    }

    currentState = mCameraAdapter->getState();
    if (currentState&CameraAdapter::CAPTURE_ACTIVE)
    {
        mAppCallbackNotifier->stopImageCallbacks();
        mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_IMAGE_CAPTURE);
    }

    mAppCallbackNotifier->stopRecording();

    mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_VIDEO);

    mRecordingEnabled = false;

    if ( mAppCallbackNotifier->getUseVideoBuffers() )
    {
        freeVideoBufs(mVideoBufs);
        if (mVideoBufs)
        {
            CAMHAL_LOGVB(" FREEING mVideoBufs %p", mVideoBufs);
            delete [] mVideoBufs;
        }
        mVideoBufs = NULL;
    }

#ifdef CAMERA_IGNORE_STOPPREVIEW_AFTER_CAPTURE
    mAfterTakePicture = false;
#endif

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Returns true if recording is enabled.

   @param none
   @return true If recording is currently running
         false If recording has been stopped

 */
int CameraHal::recordingEnabled()
{
    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return mRecordingEnabled;
}

/**
   @brief Release a record frame previously returned by CAMERA_MSG_VIDEO_FRAME.

   @param[in] mem MemoryBase pointer to the frame being released. Must be one of the buffers
               previously given by CameraHal
   @return none

 */
void CameraHal::releaseRecordingFrame(const void* mem)
{
    LOG_FUNCTION_NAME;

    //CAMHAL_LOGDB(" 0x%x", mem->pointer());

    if ( ( mRecordingEnabled ) && mem != NULL)
    {
        mAppCallbackNotifier->releaseRecordingFrame(mem);
    }

    LOG_FUNCTION_NAME_EXIT;

    return;
}

/**
   @brief Start auto focus

   This call asynchronous.
   The notification callback routine is called with CAMERA_MSG_FOCUS once when
   focusing is complete. autoFocus() will be called again if another auto focus is
   needed.

   @param none
   @return NO_ERROR
   @todo Define the error codes if the focus is not locked

 */
/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t CameraHal::autoFocus()
{
    status_t ret = NO_ERROR;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    gettimeofday(&mStartFocus, NULL);

#endif

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mLock);

    mMsgEnabled |= CAMERA_MSG_FOCUS;

    if ( NULL == mCameraAdapter )
    {
        ret = -1;
        goto EXIT;
    }

    CameraAdapter::AdapterState state;
    ret = mCameraAdapter->getState(state);
    if (ret != NO_ERROR)
    {
        goto EXIT;
    }

    if (state & CameraAdapter::AF_ACTIVE)
    {
        CAMHAL_LOGEA("Ignoring start-AF (already in progress)");
        goto EXIT;
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    //pass the autoFocus timestamp along with the command to camera adapter
    //ActionsCode(author:liuyiguang, change_code)
    //ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_PERFORM_AUTOFOCUS, ( int ) &mStartFocus);
    ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_PERFORM_AUTOFOCUS, ( long ) &mStartFocus);

#else

    ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_PERFORM_AUTOFOCUS);

#endif
    //ignore error
    ret = NO_ERROR;

EXIT:
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
   @brief Cancels auto-focus function.

   If the auto-focus is still in progress, this function will cancel it.
   Whether the auto-focus is in progress or not, this function will return the
   focus position to the default. If the camera does not support auto-focus, this is a no-op.


   @param none
   @return NO_ERROR If the cancel succeeded
   @todo Define error codes if cancel didnt succeed

 */
status_t CameraHal::cancelAutoFocus()
{
    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mLock);
    CameraParameters adapterParams = mParameters;
    mMsgEnabled &= ~CAMERA_MSG_FOCUS;

    if( NULL != mCameraAdapter )
    {
        adapterParams.set(ActCameraParameters::KEY_AUTO_FOCUS_LOCK, CameraParameters::FALSE);
        mCameraAdapter->setParameters(adapterParams);
        mCameraAdapter->sendCommand(CameraAdapter::CAMERA_CANCEL_AUTOFOCUS);
        mAppCallbackNotifier->flushEventQueue();
    }

    LOG_FUNCTION_NAME_EXIT;
    return NO_ERROR;
}

void CameraHal::setEventProvider(int32_t eventMask, MessageNotifier * eventNotifier)
{

    LOG_FUNCTION_NAME;

    if ( NULL != mEventProvider )
    {
        mEventProvider->disableEventNotification(CameraHalEvent::ALL_EVENTS);
        delete mEventProvider;
        mEventProvider = NULL;
    }

    mEventProvider = new EventProvider(eventNotifier, this, eventCallbackRelay);
    if ( NULL == mEventProvider )
    {
        CAMHAL_LOGEA("Error in creating EventProvider");
    }
    else
    {
        mEventProvider->enableEventNotification(eventMask);
    }

    LOG_FUNCTION_NAME_EXIT;
}

void CameraHal::eventCallbackRelay(CameraHalEvent* event)
{
    LOG_FUNCTION_NAME;

    CameraHal *appcbn = ( CameraHal * ) (event->mCookie);
    appcbn->eventCallback(event );

    LOG_FUNCTION_NAME_EXIT;
}

void CameraHal::eventCallback(CameraHalEvent* event)
{
    LOG_FUNCTION_NAME;

    if ( NULL != event )
    {
        switch( event->mEventType )
        {
        case CameraHalEvent::EVENT_FOCUS_LOCKED:
        case CameraHalEvent::EVENT_FOCUS_ERROR:
            {

                break;
            }
        default:
            {
                break;
            }
        };
    }

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Take a picture.

   @param none
   @return NO_ERROR If able to switch to image capture
   @todo Define error codes if unable to switch to image capture

 */
/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t CameraHal::takePicture( )
{
    status_t ret = NO_ERROR;
    CameraFrame frame;
    CameraAdapter::BuffersDescriptor desc;
    int burst;
    const char *valstr = NULL;
    unsigned int bufferCount = NO_BUFFERS_IMAGE_CAPTURE;

    Mutex::Autolock lock(mLock);

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    gettimeofday(&mStartCapture, NULL);

#endif

    LOG_FUNCTION_NAME;

    if(!previewEnabledInner() && !mDisplayPaused)
    {
        LOG_FUNCTION_NAME_EXIT;
        CAMHAL_LOGEA("Preview not started...");
        return NO_INIT;
    }

    // return error if we are already capturing
    while ( (mCameraAdapter->getState() & CameraAdapter::CAPTURE_ACTIVE) ||
            (mCameraAdapter->getNextState() & CameraAdapter::CAPTURE_ACTIVE))
    {
    
        if(mTakePictureWait.waitRelative(mLock, ms2ns(500)))
        {
            CAMHAL_LOGEA("Timeout! Already capturing an image...");
            return NO_INIT;
        }
    }

    CAMERA_SCOPEDTRACE("takePicture");

    bufferCount = atoi(mCameraProperties->get(CameraProperties::REQUIRED_IMAGE_BUFS));
    CAMHAL_LOGDB("image bufferCount %d", bufferCount);


    if ( (NO_ERROR == ret) && (NULL != mCameraAdapter) )
    {
        if ( NO_ERROR == ret )
        {
            //ActionsCode(author:liuyiguang, change_code)
            //ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE,
            //                                  ( int ) &frame,
            //                                  bufferCount);
            ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE,
                                              ( long ) &frame,
                                              bufferCount);
        }

        if ( NO_ERROR != ret )
        {
            CAMHAL_LOGEB("CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE returned error 0x%x", ret);
        }
    }

    if ( NO_ERROR == ret )
    {
        mParameters.getPictureSize(( int * ) &frame.mWidth,
                                   ( int * ) &frame.mHeight);

#ifndef CAMERA_FIX_IMAGE_BUFFER
        //if camera image buffer is fixed, defalut format is yuv420sp
        ret = allocImageBufs(frame.mWidth,
                             frame.mHeight,
                             frame.mLength,
                             CameraParameters::PIXEL_FORMAT_YUV420SP,
                             bufferCount);
#else
        //picture raw data format is same as preview format. 
        ret = allocImageBufs(MAX_PICTURE_WIDTH,
                             MAX_PICTURE_HEIGHT,
                             (MAX_PICTURE_WIDTH*MAX_PICTURE_HEIGHT*MAX_PICTURE_BPP)>>3,
                             mParameters.getPreviewFormat(),
                             bufferCount);

#endif
        if ( NO_ERROR != ret )
        {
            CAMHAL_LOGEB("allocImageBufs returned error 0x%x", ret);
            return NO_MEMORY;

        }
    }

    if (  (NO_ERROR == ret) && ( NULL != mCameraAdapter ) )
    {
        desc.mBuffers = mImageBufs;
        desc.mVaddrs = NULL;
        desc.mOffsets = mImageOffsets;
        desc.mFd = mImageFd;
        desc.mLength = mImageLength;
        desc.mCount = ( size_t ) bufferCount;
        desc.mMaxQueueable = ( size_t ) bufferCount;

        //ActionsCode(author:liuyiguang, change_code)
        //ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS_IMAGE_CAPTURE,
        //                                  ( int ) &desc);
        ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_USE_BUFFERS_IMAGE_CAPTURE,
                                          ( long ) &desc);
    }

    mAppCallbackNotifier->startImageCallbacks(mParameters, mImageBufs, mImageOffsets, mImageFd, mImageLength, bufferCount);


    if ( ( NO_ERROR == ret ) && ( NULL != mCameraAdapter ) )
    {

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

        //pass capture timestamp along with the camera adapter command
        //ActionsCode(author:liuyiguang, change_code)
        //ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_IMAGE_CAPTURE,  (int) &mStartCapture);
        ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_IMAGE_CAPTURE,  (long) &mStartCapture);

#else

        ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_IMAGE_CAPTURE);

#endif

    }

#ifdef CAMERA_IGNORE_STOPPREVIEW_AFTER_CAPTURE
    mAfterTakePicture = true;
#endif
    //if not video snapshot
    if(!(mCameraAdapter->getState() & CameraAdapter::VIDEO_ACTIVE))
    {
        mPreviewCBEnabled = false;
    }
    

    return ret;
}

/**
   @brief Cancel a picture that was started with takePicture.

   Calling this method when no picture is being taken is a no-op.

   @param none
   @return NO_ERROR If cancel succeeded. Cancel can succeed if image callback is not sent
   @todo Define error codes

 */
status_t CameraHal::cancelPicture( )
{
    LOG_FUNCTION_NAME;
    CAMERA_SCOPEDTRACE("cancelPicture");

    Mutex::Autolock lock(mLock);

    mAppCallbackNotifier->stopImageCallbacks();
    mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_IMAGE_CAPTURE);

#ifdef CAMERA_IGNORE_STOPPREVIEW_AFTER_CAPTURE
    mAfterTakePicture = false;
#endif
    

    return NO_ERROR;
}

/**
   @brief Return the camera parameters.

   @param none
   @return Currently configured camera parameters

 */
char* CameraHal::getParameters()
{
    String8 params_str8;
    char* params_string;
    const char * valstr = NULL;

    LOG_FUNCTION_NAME;

    CAMERA_SCOPEDTRACE("getParameters");

    //lock to protect mParameters
	Mutex::Autolock lock(mParametersLock);

    if( NULL != mCameraAdapter )
    {
        mCameraAdapter->getParameters(mParameters);
    }

    CameraParameters mParams = mParameters;

    //override the preview format
    mParams.setPreviewFormat(mPreviewFormat);

    mParams.remove(ActCameraParameters::KEY_AUTO_FOCUS_LOCK);
    params_str8 = mParams.flatten();

    // camera service frees this string...
    params_string = (char*) malloc(sizeof(char) * (params_str8.length()+1));
    strcpy(params_string, params_str8.string());
    CAMHAL_LOGDB("getParameters===%s",params_string);
    
    /*
    if(params_str8.length()+1 >1024)
    {
        CAMHAL_LOGDB("getParameters===%s",params_string+800);
        CAMHAL_LOGDB("getParameters===%s",params_string+1600);
    }
    */
    

    LOG_FUNCTION_NAME_EXIT;

    ///Return the current set of parameters

    return params_string;
}

void CameraHal::putParameters(char *parms)
{
    free(parms);
}

/**
   @brief Send command to camera driver.

   @param none
   @return NO_ERROR If the command succeeds
   @todo Define the error codes that this function can return

 */
status_t CameraHal::sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    CAMERA_SCOPEDTRACE("sendCommand");

    if ( ( NO_ERROR == ret ) && ( NULL == mCameraAdapter ) )
    {
        CAMHAL_LOGEA("No CameraAdapter instance");
        return BAD_VALUE;
    }

    ///////////////////////////////////////////////////////
    // Following commands do NOT need preview to be started
    ///////////////////////////////////////////////////////
    switch(cmd)
    {
    case CAMERA_CMD_ENABLE_FOCUS_MOVE_MSG:
        bool enable = static_cast<bool>(arg1);
        Mutex::Autolock lock(mLock);
        if (enable)
        {
            mMsgEnabled |= CAMERA_MSG_FOCUS_MOVE;
        }
        else
        {
            mMsgEnabled &= ~CAMERA_MSG_FOCUS_MOVE;
        }
        return NO_ERROR;
        break;
    }
    if ( ( NO_ERROR == ret ) && ( !previewEnabledInner() ))
    {
        CAMHAL_LOGEA("Preview is not running");
        ret = BAD_VALUE;
    }
    ///////////////////////////////////////////////////////
    // Following commands NEED preview to be started
    ///////////////////////////////////////////////////////

    if ( NO_ERROR == ret )
    {
        switch(cmd)
        {
        case CAMERA_CMD_START_SMOOTH_ZOOM:

            ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_SMOOTH_ZOOM, arg1);

            break;
        case CAMERA_CMD_STOP_SMOOTH_ZOOM:

            ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_SMOOTH_ZOOM);
            //for cts
            ret = NO_ERROR;
            break;

        case CAMERA_CMD_START_FACE_DETECTION:

            ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_START_FD);

            break;

        case CAMERA_CMD_STOP_FACE_DETECTION:

            ret = mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_FD);

            break;

        default:
            break;
        };
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
   @brief Release the hardware resources owned by this object.

   Note that this is *not* done in the destructor.

   @param none
   @return none

 */
void CameraHal::release()
{
    LOG_FUNCTION_NAME;
    CAMERA_SCOPEDTRACE("release");
    ///@todo Investigate on how release is used by CameraService. Vaguely remember that this is called
    ///just before CameraHal object destruction
    deinitialize();
    LOG_FUNCTION_NAME_EXIT;
}


/**
   @brief Dump state of the camera hardware

   @param[in] fd    File descriptor
   @param[in] args  Arguments
   @return NO_ERROR Dump succeeded
   @todo  Error codes for dump fail

 */
status_t  CameraHal::dump(int fd) const
{
    LOG_FUNCTION_NAME;
    ///Implement this method when the h/w dump function is supported on Ducati side
    return NO_ERROR;
}

/*-------------Camera Hal Interface Method definitions ENDS here--------------------*/




/*-------------Camera Hal Internal Method definitions STARTS here--------------------*/

/**
   @brief Constructor of CameraHal

   Member variables are initialized here.  No allocations should be done here as we
   don't use c++ exceptions in the code.

 */
CameraHal::CameraHal(int cameraId)
{
    LOG_FUNCTION_NAME;

    ///Initialize all the member variables to their defaults
    mPreviewEnabled = false;
    mPreviewBufs = NULL;
    mImageBufs = NULL;
    mImageBufCount = 0;
    mBufProvider = NULL;
    mPreviewStartInProgress = false;
    mVideoBufs = NULL;
    mVideoBufProvider = NULL;
    mRecordingEnabled = false;
    mDisplayPaused = false;
    mSetPreviewWindowCalled = false;
    mMsgEnabled = 0;
    mAppCallbackNotifier = NULL;
    mMemoryManager = NULL;
    mCameraAdapter = NULL;
    mBracketingEnabled = false;
    mBracketingRunning = false;
    mEventProvider = NULL;
    mBracketRangePositive = 1;
    mBracketRangeNegative = 1;
    mMaxZoomSupported = 0;
    mShutterEnabled = true;
    mMeasurementEnabled = false;
    mPreviewDataBufs = NULL;
    mCameraProperties = NULL;
    mCurrentTime = 0;
    mFalsePreview = 0;
    mImageOffsets = NULL;
    mImageLength = 0;
    mImageFd = 0;
    mVideoOffsets = NULL;
    mVideoFd = 0;
    mVideoLength = 0;
    mPreviewDataOffsets = NULL;
    mPreviewDataFd = 0;
    mPreviewDataLength = 0;
    mPreviewFd = 0;
    mPreviewWidth = 0;
    mPreviewHeight = 0;
    mPreviewLength = 0;
    mPreviewOffsets = NULL;
    mPreviewVaddrs = NULL;
    mPreviewRunning = 0;
    mPreviewStateOld = 0;
    mRecordingEnabled = 0;
    mRecordEnabled = 0;
    mSensorListener = NULL;
    mVideoWidth = 0;
    mVideoHeight = 0;

    mPreviewFormat = NULL;

    mVideoFormat = NULL;
    mNeedfmtrefresh = false;

    //ActionsCode(author:liuyiguang, add_code)
    mVceResize = false;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    //Initialize the CameraHAL constructor timestamp, which is used in the
    // PPM() method as time reference if the user does not supply one.
    gettimeofday(&ppm_start, NULL);

#endif

    mCameraIndex = cameraId;

    mUseVideoBuffers = false;

#ifdef CAMERA_IGNORE_STOPPREVIEW_AFTER_CAPTURE
    mAfterTakePicture = false;
#endif
    
    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Destructor of CameraHal

   This function simply calls deinitialize() to free up memory allocate during construct
   phase
 */
CameraHal::~CameraHal()
{
    LOG_FUNCTION_NAME;

    CAMERA_SCOPEDTRACE("~CameraHal");
    ///Call de-initialize here once more - it is the last chance for us to relinquish all the h/w and s/w resources
    deinitialize();

    if ( NULL != mEventProvider )
    {
        mEventProvider->disableEventNotification(CameraHalEvent::ALL_EVENTS);
        delete mEventProvider;
        mEventProvider = NULL;
    }

    /// Free the callback notifier
    mAppCallbackNotifier.clear();

    /// Free the display adapter
    mDisplayAdapter.clear();

    if ( NULL != mCameraAdapter )
    {
        int strongCount = mCameraAdapter->getStrongCount();

        mCameraAdapter->decStrong(mCameraAdapter);

        mCameraAdapter = NULL;
    }


    freePreviewBufs();
    freeImageBufs();

    /// Free the memory manager
    mMemoryManager.clear();

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Initialize the Camera HAL

   Creates CameraAdapter, AppCallbackNotifier, DisplayAdapter and MemoryManager

   @param None
   @return NO_ERROR - On success
         NO_MEMORY - On failure to allocate memory for any of the objects
   @remarks Camera Hal internal function

 */

status_t CameraHal::initialize(CameraProperties::Properties* properties)
{
    LOG_FUNCTION_NAME;

    CAMERA_SCOPEDTRACE("initialize");
    int sensor_index = 0;

    ///Initialize the event mask used for registering an event provider for AppCallbackNotifier
    ///Currently, registering all events as to be coming from CameraAdapter
    int32_t eventMask = CameraHalEvent::ALL_EVENTS;

    // Get my camera properties
    mCameraProperties = properties;

    if(!mCameraProperties)
    {
        goto fail_loop;
    }

    // Dump the properties of this Camera
    // will only print if DEBUG macro is defined
    {
    CAMERA_SCOPEDTRACE("CameraAdapter_Factory");
        
    mCameraProperties->dump();

    if (strcmp(CameraProperties::DEFAULT_VALUE, mCameraProperties->get(CameraProperties::CAMERA_SENSOR_INDEX)) != 0 )
    {
        sensor_index = atoi(mCameraProperties->get(CameraProperties::CAMERA_SENSOR_INDEX));
    }

    CAMHAL_LOGDB("Sensor index %d", sensor_index);

    mCameraAdapter = CameraAdapter_Factory(sensor_index);

    if ( ( NULL == mCameraAdapter ) || (mCameraAdapter->initialize(properties)!=NO_ERROR))
    {
        CAMHAL_LOGEA("Unable to create or initialize CameraAdapter");
        mCameraAdapter = NULL;
        goto fail_loop;
    }
    }

    mCameraAdapter->incStrong(mCameraAdapter);
    mCameraAdapter->registerImageReleaseCallback(releaseImageBuffers, (void *) this);
    mCameraAdapter->registerEndCaptureCallback(endImageCapture, (void *)this);

    if(!mAppCallbackNotifier.get())
    {
        /// Create the callback notifier
        mAppCallbackNotifier = new AppCallbackNotifier();
        if( ( NULL == mAppCallbackNotifier.get() ) || ( mAppCallbackNotifier->initialize() != NO_ERROR))
        {
            CAMHAL_LOGEA("Unable to create or initialize AppCallbackNotifier");
            goto fail_loop;
        }
    }

    if(!mMemoryManager.get())
    {
        /// Create Memory Manager
        mMemoryManager = new MemoryManager();
        if( ( NULL == mMemoryManager.get() ) || ( mMemoryManager->initialize() != NO_ERROR))
        {
            CAMHAL_LOGEA("Unable to create or initialize MemoryManager");
            goto fail_loop;
        }
    }

    ///Setup the class dependencies...

    ///AppCallbackNotifier has to know where to get the Camera frames and the events like auto focus lock etc from.
    ///CameraAdapter is the one which provides those events
    ///Set it as the frame and event providers for AppCallbackNotifier
    ///@remarks  setEventProvider API takes in a bit mask of events for registering a provider for the different events
    ///         That way, if events can come from DisplayAdapter in future, we will be able to add it as provider
    ///         for any event
    mAppCallbackNotifier->setEventProvider(eventMask, mCameraAdapter);
    mAppCallbackNotifier->setFrameProvider(mCameraAdapter);

    ///Any dynamic errors that happen during the camera use case has to be propagated back to the application
    ///via CAMERA_MSG_ERROR. AppCallbackNotifier is the class that  notifies such errors to the application
    ///Set it as the error handler for CameraAdapter
    mCameraAdapter->setErrorHandler(mAppCallbackNotifier.get());

    ///Start the callback notifier
    if(mAppCallbackNotifier->start() != NO_ERROR)
    {
        CAMHAL_LOGEA("Couldn't start AppCallbackNotifier");
        goto fail_loop;
    }

    CAMHAL_LOGDA("Started AppCallbackNotifier..");
    mAppCallbackNotifier->setMeasurements(mMeasurementEnabled);

    ///Initialize default parameters
    initDefaultParameters();


    if ( setParameters(mParameters) != NO_ERROR )
    {
        CAMHAL_LOGEA("Failed to set default parameters?!");
    }

#ifdef CAMERA_SENSOR_LISTENER
    // register for sensor events
    mSensorListener = new SensorListener();
    if (mSensorListener.get())
    {
        if (mSensorListener->initialize() == NO_ERROR)
        {
            mSensorListener->setCallbacks(orientation_cb, this);
            mSensorListener->enableSensor(SensorListener::SENSOR_ORIENTATION);
        }
        else
        {
            CAMHAL_LOGEA("Error initializing SensorListener. not fatal, continuing");
            mSensorListener.clear();
            mSensorListener = NULL;
        }
    }
#endif

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;

fail_loop:

    ///Free up the resources because we failed somewhere up
    deinitialize();
    LOG_FUNCTION_NAME_EXIT;

    return NO_MEMORY;

}

bool CameraHal::isResolutionValid(unsigned int width, unsigned int height, const char *supportedResolutions)
{
    bool ret = true;
    status_t status = NO_ERROR;
    char tmpBuffer[PARAM_BUFFER + 1];
    char *pos = NULL;

    LOG_FUNCTION_NAME;

    if ( NULL == supportedResolutions )
    {
        CAMHAL_LOGEA("Invalid supported resolutions string");
        ret = false;
        goto exit;
    }

    status = snprintf(tmpBuffer, PARAM_BUFFER, "%dx%d", width, height);
    if ( 0 > status )
    {
        CAMHAL_LOGEA("Error encountered while generating validation string");
        ret = false;
        goto exit;
    }

    pos = strstr(supportedResolutions, tmpBuffer);
    if ( NULL == pos )
    {
        ret = false;
    }
    else
    {
        ret = true;
    }

exit:

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

bool CameraHal::isParameterValid(const char *param, const char *supportedParams)
{
    bool ret = true;
    char *pos = NULL;

    LOG_FUNCTION_NAME;

    if ( NULL == supportedParams )
    {
        CAMHAL_LOGEA("Invalid supported parameters string");
        ret = false;
        goto exit;
    }

    if ( NULL == param )
    {
        CAMHAL_LOGEA("Invalid parameter string");
        ret = false;
        goto exit;
    }

    pos = strstr(supportedParams, param);
    if ( NULL == pos )
    {
        ret = false;
    }
    else
    {
        ret = true;
    }

exit:

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

bool CameraHal::isParameterValid(int param, const char *supportedParams)
{
    bool ret = true;
    char *pos = NULL;
    status_t status;
    char tmpBuffer[PARAM_BUFFER + 1];

    LOG_FUNCTION_NAME;

    if ( NULL == supportedParams )
    {
        CAMHAL_LOGEA("Invalid supported parameters string");
        ret = false;
        goto exit;
    }

    status = snprintf(tmpBuffer, PARAM_BUFFER, "%d", param);
    if ( 0 > status )
    {
        CAMHAL_LOGEA("Error encountered while generating validation string");
        ret = false;
        goto exit;
    }

    pos = strstr(supportedParams, tmpBuffer);
    if ( NULL == pos )
    {
        ret = false;
    }
    else
    {
        ret = true;
    }

exit:

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t CameraHal::doesSetParameterNeedUpdate(const char* new_param, const char* old_param, bool& update)
{
    if (!new_param || !old_param)
    {
        return BAD_VALUE;
    }

    // if params mismatch we should update parameters for camera adapter
    if ((strcmp(new_param, old_param) != 0))
    {
        update = true;
    }

    return NO_ERROR;
}

status_t CameraHal::parseResolution(const char *resStr, int &width, int &height)
{
    status_t ret = NO_ERROR;
    char *ctx, *pWidth, *pHeight;
    const char *sep = "x";
    char *tmp = NULL;

    LOG_FUNCTION_NAME;

    if ( NULL == resStr )
    {
        return BAD_VALUE;
    }

    CAMHAL_LOGVB("parseResolution resStr=%s", resStr);
    //This fixes "Invalid input resolution"
    char *resStr_copy = (char *)malloc(strlen(resStr) + 1);
    if ( NULL!=resStr_copy )
    {
        if ( NO_ERROR == ret )
        {
            strcpy(resStr_copy, resStr);
            pWidth = strtok_r( (char *) resStr_copy, sep, &ctx);

            if ( NULL != pWidth )
            {
                width = atoi(pWidth);
            }
            else
            {
                CAMHAL_LOGEB("Invalid input resolution %s", resStr);
                ret = BAD_VALUE;
            }
        }

        if ( NO_ERROR == ret )
        {
            pHeight = strtok_r(NULL, sep, &ctx);

            if ( NULL != pHeight )
            {
                height = atoi(pHeight);
            }
            else
            {
                CAMHAL_LOGEB("Invalid input resolution %s", resStr);
                ret = BAD_VALUE;
            }
        }

        free(resStr_copy);
        resStr_copy = NULL;
    }
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

void CameraHal::insertSupportedParams()
{
    char tmpBuffer[PARAM_BUFFER + 1];

    LOG_FUNCTION_NAME;

    CameraParameters &p = mParameters;

    mMaxZoomSupported = atoi(mCameraProperties->get(CameraProperties::SUPPORTED_ZOOM_STAGES));

    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_SIZES));

    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, mCameraProperties->get(CameraProperties::SUPPORTED_PICTURE_FORMATS));

    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_SIZES));

    p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_VIDEO_SIZES));
    
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FORMATS));

    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, mCameraProperties->get(CameraProperties::SUPPORTED_PREVIEW_FRAME_RATES));
    p.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, mCameraProperties->get(CameraProperties::SUPPORTED_THUMBNAIL_SIZES));
    p.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE, mCameraProperties->get(CameraProperties::SUPPORTED_WHITE_BALANCE));
    p.set(CameraParameters::KEY_SUPPORTED_EFFECTS, mCameraProperties->get(CameraProperties::SUPPORTED_EFFECTS));
    p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, mCameraProperties->get(CameraProperties::SUPPORTED_SCENE_MODES));
    if(strcmp(mCameraProperties->get(CameraProperties::FLASH_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        p.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES, mCameraProperties->get(CameraProperties::SUPPORTED_FLASH_MODES));
    }
    p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES, mCameraProperties->get(CameraProperties::SUPPORTED_FOCUS_MODES));
    p.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING, mCameraProperties->get(CameraProperties::SUPPORTED_ANTIBANDING));
    p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, mCameraProperties->get(CameraProperties::SUPPORTED_EV_MAX));
    p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, mCameraProperties->get(CameraProperties::SUPPORTED_EV_MIN));
    p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, mCameraProperties->get(CameraProperties::SUPPORTED_EV_STEP));
    p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES, mCameraProperties->get(CameraProperties::SUPPORTED_SCENE_MODES));
    if(strcmp(mCameraProperties->get(CameraProperties::ISO_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        p.set(ActCameraParameters::KEY_SUPPORTED_ISO_VALUES, mCameraProperties->get(CameraProperties::SUPPORTED_ISO_VALUES));
    }
    p.set(CameraParameters::KEY_ZOOM_RATIOS, mCameraProperties->get(CameraProperties::SUPPORTED_ZOOM_RATIOS));
    p.set(CameraParameters::KEY_MAX_ZOOM, mCameraProperties->get(CameraProperties::SUPPORTED_ZOOM_STAGES));

    p.set(CameraParameters::KEY_ZOOM_SUPPORTED, mCameraProperties->get(CameraProperties::ZOOM_SUPPORTED));
    p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, mCameraProperties->get(CameraProperties::SMOOTH_ZOOM_SUPPORTED));
    p.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, mCameraProperties->get(CameraProperties::VSTAB_SUPPORTED));
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, mCameraProperties->get(CameraProperties::PREVIEW_FRAMERATE_RANGE_SUPPORTED));
    p.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, mCameraProperties->get(CameraProperties::AUTO_EXPOSURE_LOCK_SUPPORTED));
    p.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, mCameraProperties->get(CameraProperties::AUTO_WHITEBALANCE_LOCK_SUPPORTED));
    p.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, mCameraProperties->get(CameraProperties::VIDEO_SNAPSHOT_SUPPORTED));


    LOG_FUNCTION_NAME_EXIT;

}

void CameraHal::initDefaultParameters()
{
    //Purpose of this function is to initialize the default current and supported parameters for the currently
    //selected camera.

    CameraParameters &p = mParameters;
    int currentRevision, adapterRevision;
    status_t ret = NO_ERROR;
    int width, height;

    LOG_FUNCTION_NAME;

    ret = parseResolution(mCameraProperties->get(CameraProperties::PREVIEW_SIZE), width, height);

    if ( NO_ERROR == ret )
    {
        p.setPreviewSize(width, height);
    }
    else
    {
        p.setPreviewSize(DEFAULT_PREVIEW_WIDTH, DEFAULT_PREVIEW_HEIGHT);
    }

    if(strcmp(mCameraProperties->get(CameraProperties::VIDEO_SIZE), "") != 0)
    {
        ret = parseResolution(mCameraProperties->get(CameraProperties::VIDEO_SIZE), width, height);

        if ( NO_ERROR == ret )
        {
            p.setVideoSize(width, height);
        }
        else
        {
            p.setVideoSize(DEFAULT_PREVIEW_WIDTH, DEFAULT_PREVIEW_HEIGHT);
        }
    }

    ret = parseResolution(mCameraProperties->get(CameraProperties::PICTURE_SIZE), width, height);

    if ( NO_ERROR == ret )
    {
        p.setPictureSize(width, height);
    }
    else
    {
        p.setPictureSize(DEFAULT_PICTURE_WIDTH, DEFAULT_PICTURE_HEIGHT);
    }

    ret = parseResolution(mCameraProperties->get(CameraProperties::JPEG_THUMBNAIL_SIZE), width, height);

    if ( NO_ERROR == ret )
    {
        p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, width);
        p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, height);
    }
    else
    {
        p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, DEFAULT_THUMB_WIDTH);
        p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, DEFAULT_THUMB_HEIGHT);
    }

    insertSupportedParams();

    //Insert default values
    p.setPreviewFrameRate(atoi(mCameraProperties->get(CameraProperties::PREVIEW_FRAME_RATE)));

    mPreviewFormat = "";
    p.setPreviewFormat(mCameraProperties->get(CameraProperties::PREVIEW_FORMAT));
    CAMHAL_LOGIB("p.getPreviewFormat():%s", p.getPreviewFormat());

    p.setPictureFormat(mCameraProperties->get(CameraProperties::PICTURE_FORMAT));
    p.set(CameraParameters::KEY_JPEG_QUALITY, mCameraProperties->get(CameraProperties::JPEG_QUALITY));
    p.set(CameraParameters::KEY_WHITE_BALANCE, mCameraProperties->get(CameraProperties::WHITEBALANCE));
    p.set(CameraParameters::KEY_EFFECT,  mCameraProperties->get(CameraProperties::EFFECT));
    p.set(CameraParameters::KEY_ANTIBANDING, mCameraProperties->get(CameraProperties::ANTIBANDING));
    if(strcmp(mCameraProperties->get(CameraProperties::FLASH_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        p.set(CameraParameters::KEY_FLASH_MODE, mCameraProperties->get(CameraProperties::FLASH_MODE));
    }
    p.set(CameraParameters::KEY_FOCUS_MODE, mCameraProperties->get(CameraProperties::FOCUS_MODE));
    p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, mCameraProperties->get(CameraProperties::EV_COMPENSATION));
    if(strcmp(mCameraProperties->get(CameraProperties::ISO_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        p.set(ActCameraParameters::KEY_ISO, mCameraProperties->get(CameraProperties::ISO_MODE));
    }
    p.set(CameraParameters::KEY_SCENE_MODE, mCameraProperties->get(CameraProperties::SCENE_MODE));
    p.set(CameraParameters::KEY_ZOOM, mCameraProperties->get(CameraProperties::ZOOM));
    if(strcmp(mCameraProperties->get(CameraProperties::CONTRAST_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        p.set(ActCameraParameters::KEY_CONTRAST, mCameraProperties->get(CameraProperties::CONTRAST));

        p.set(ActCameraParameters::KEY_MIN_CONTRAST, mCameraProperties->get(CameraProperties::SUPPORTED_CONTRAST_MIN));
        p.set(ActCameraParameters::KEY_MAX_CONTRAST, mCameraProperties->get(CameraProperties::SUPPORTED_CONTRAST_MAX));
        p.set(ActCameraParameters::KEY_CONTRAST_STEP, mCameraProperties->get(CameraProperties::SUPPORTED_CONTRAST_STEP));
    }
    if(strcmp(mCameraProperties->get(CameraProperties::SATURATION_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        p.set(ActCameraParameters::KEY_SATURATION, mCameraProperties->get(CameraProperties::SATURATION));

        p.set(ActCameraParameters::KEY_MIN_SATURATION, mCameraProperties->get(CameraProperties::SUPPORTED_SATURATION_MIN));
        p.set(ActCameraParameters::KEY_MAX_SATURATION, mCameraProperties->get(CameraProperties::SUPPORTED_SATURATION_MAX));
        p.set(ActCameraParameters::KEY_SATURATION_STEP, mCameraProperties->get(CameraProperties::SUPPORTED_SATURATION_STEP));
    }
    if(strcmp(mCameraProperties->get(CameraProperties::BRIGHTNESS_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        p.set(ActCameraParameters::KEY_BRIGHTNESS, mCameraProperties->get(CameraProperties::BRIGHTNESS));

        p.set(ActCameraParameters::KEY_MIN_BRIGHTNESS, mCameraProperties->get(CameraProperties::SUPPORTED_BRIGHTNESS_MIN));
        p.set(ActCameraParameters::KEY_MAX_BRIGHTNESS, mCameraProperties->get(CameraProperties::SUPPORTED_BRIGHTNESS_MAX));
        p.set(ActCameraParameters::KEY_BRIGHTNESS_STEP, mCameraProperties->get(CameraProperties::SUPPORTED_BRIGHTNESS_STEP));
    }

    if(strcmp(mCameraProperties->get(CameraProperties::DENOISE_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        p.set(ActCameraParameters::KEY_DENOISE, mCameraProperties->get(CameraProperties::DENOISE));
        p.set(ActCameraParameters::KEY_MIN_DENOISE, mCameraProperties->get(CameraProperties::SUPPORTED_DENOISE_MIN));
        p.set(ActCameraParameters::KEY_MAX_DENOISE, mCameraProperties->get(CameraProperties::SUPPORTED_DENOISE_MAX));
        p.set(ActCameraParameters::KEY_DENOISE_STEP, mCameraProperties->get(CameraProperties::SUPPORTED_DENOISE_STEP));
    }

    p.set(CameraParameters::KEY_VIDEO_STABILIZATION, mCameraProperties->get(CameraProperties::VSTAB));
    p.set(CameraParameters::KEY_FOCAL_LENGTH, mCameraProperties->get(CameraProperties::FOCAL_LENGTH));
    p.set(CameraParameters::KEY_FOCUS_DISTANCES, mCameraProperties->get(CameraProperties::FOCUS_DISTANCES));
    p.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, mCameraProperties->get(CameraProperties::HOR_ANGLE));
    p.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, mCameraProperties->get(CameraProperties::VER_ANGLE));
    p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE,mCameraProperties->get(CameraProperties::PREVIEW_FRAMERATE_RANGE));
    p.set(ActCameraParameters::KEY_EXIF_MAKE, mCameraProperties->get(CameraProperties::EXIF_MAKE));
    p.set(ActCameraParameters::KEY_EXIF_MODEL, mCameraProperties->get(CameraProperties::EXIF_MODEL));
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, mCameraProperties->get(CameraProperties::JPEG_THUMBNAIL_QUALITY));
    p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, mCameraProperties->get(CameraProperties::PREVIEW_FORMAT));
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, mCameraProperties->get(CameraProperties::MAX_FD_HW_FACES));
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, mCameraProperties->get(CameraProperties::MAX_FD_SW_FACES));

    // Only one area a.k.a Touch AF for now.
    // TODO: Add support for multiple focus areas.
    p.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, mCameraProperties->get(CameraProperties::MAX_FOCUS_AREAS));
    p.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, mCameraProperties->get(CameraProperties::AUTO_EXPOSURE_LOCK));
    p.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, mCameraProperties->get(CameraProperties::AUTO_WHITEBALANCE_LOCK));
    p.set(CameraParameters::KEY_MAX_NUM_METERING_AREAS, mCameraProperties->get(CameraProperties::MAX_NUM_METERING_AREAS));

    p.set(CameraParameters::KEY_RECORDING_HINT, CameraParameters::FALSE);

    p.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, mCameraProperties->get(CameraProperties::PREFERRED_PREVIEW_SIZE_FOR_VIDEO));
    
    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Stop a previously started preview.
   @param none
   @return none

 */
void CameraHal::forceStopPreview()
{
    LOG_FUNCTION_NAME;


    if(mDisplayAdapter.get() != NULL)
    {
        CAMERA_SCOPEDTRACE("disableDisplay");
        ///Stop the buffer display first
        mDisplayAdapter->disableDisplay();
    }

    if(mAppCallbackNotifier.get() != NULL)
    {
        CAMERA_SCOPEDTRACE("stopPreviewCallbacks");
        //Stop the callback sending
        mAppCallbackNotifier->stop();
        mAppCallbackNotifier->stopPreviewCallbacks();
    }

    if ( NULL != mCameraAdapter )
    {
        CAMERA_SCOPEDTRACE("rollbackToInitializedState");
        // only need to send these control commands to state machine if we are
        // passed the LOADED_PREVIEW_STATE
        if (mCameraAdapter->getState() & CameraAdapter::PREVIEW_ACTIVE)
        {
            // according to javadoc...FD should be stopped in stopPreview
            // and application needs to call startFaceDection again
            // to restart FD
            mCameraAdapter->sendCommand(CameraAdapter::CAMERA_STOP_FD);
        }

        {
            mCameraAdapter->rollbackToInitializedState();
        }

    }

    freePreviewBufs();

    mPreviewEnabled = false;
    mDisplayPaused = false;
    mPreviewStartInProgress = false;

    mPreviewCBEnabled = false;        

    LOG_FUNCTION_NAME_EXIT;
}

/**
   @brief Deallocates memory for all the resources held by Camera HAL.

   Frees the following objects- CameraAdapter, AppCallbackNotifier, DisplayAdapter,
   and Memory Manager

   @param none
   @return none

 */
void CameraHal::deinitialize()
{
    LOG_FUNCTION_NAME;

    if(mRecordingEnabled)
    {
        stopRecording();
    }
    if ( mPreviewEnabled || mDisplayPaused )
    {
        forceStopPreview();
    }

    mSetPreviewWindowCalled = false;

#ifdef CAMERA_SENSOR_LISTENER
    if (mSensorListener.get())
    {
        mSensorListener->disableSensor(SensorListener::SENSOR_ORIENTATION);
        mSensorListener.clear();
        mSensorListener = NULL;
    }
#endif

    LOG_FUNCTION_NAME_EXIT;

}

status_t CameraHal::storeMetaDataInBuffers(bool enable)
{
    LOG_FUNCTION_NAME;

    return mAppCallbackNotifier->useMetaDataBufferMode(enable);

    LOG_FUNCTION_NAME_EXIT;
}

void CameraHal::selectFPSRange(int framerate, int *min_fps, int *max_fps)
{
    char * ptr;
    char supported[MAX_PROP_VALUE_LENGTH];
    int fpsrangeArray[2];
    int i = 0;

    LOG_FUNCTION_NAME;
    size_t size = strlen(mCameraProperties->get(CameraProperties::PREVIEW_FRAMERATE_RANGE_SUPPORTED))+1;
    strncpy(supported, mCameraProperties->get(CameraProperties::PREVIEW_FRAMERATE_RANGE_SUPPORTED), size);

    ptr = strtok (supported," (,)");

    while (ptr != NULL)
    {
        fpsrangeArray[i]= atoi(ptr)/CameraHal::VFR_SCALE;
        if (i == 1)
        {
            if (framerate <= fpsrangeArray[1] && framerate >= fpsrangeArray[0])
            {
                CAMHAL_LOGDB("SETTING FPS RANGE min = %d max = %d \n", fpsrangeArray[0], fpsrangeArray[1]);
                *min_fps = fpsrangeArray[0]*CameraHal::VFR_SCALE;
                *max_fps = fpsrangeArray[1]*CameraHal::VFR_SCALE;
                break;
            }
        }
        ptr = strtok (NULL, " (,)");
        i++;
        i%=2;
    }

    LOG_FUNCTION_NAME_EXIT;

}

void CameraHal::setPreferredPreviewRes(int width, int height)
{
    LOG_FUNCTION_NAME;


    LOG_FUNCTION_NAME_EXIT;
}

void CameraHal::resetPreviewRes(CameraParameters *mParams, int width, int height)
{
    LOG_FUNCTION_NAME;


    LOG_FUNCTION_NAME_EXIT;
}

};


