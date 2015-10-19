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
* @file OMXCameraAdapter.cpp
*
* This file maps the Camera Hardware Interface to OMX.
*
*/
#include "CameraATraceTag.h"

#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"
#include "ErrorUtils.h"
#include "ActCameraParameters.h"
#include <signal.h>
#include <math.h>

#include <cutils/properties.h>

#include "OMXVce.h"

#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>

#include "CameraConfigs.h"
#include "CameraGpuOutstanding.h"
#include "CameraFreqAdapter.h"

//#define DUMP_FRAME
#define UNLIKELY( exp ) (__builtin_expect( (exp) != 0, false ))
static int mDebugFps = 0;
static int mDebugFcs = 0;

#undef TRUE
#undef FALSE

#define HERE(Msg) {CAMHAL_LOGEB("--===line %d, %s===--\n", __LINE__, Msg);}

namespace android
{

#undef LOG_TAG
///Maintain a separate tag for OMXCameraAdapter logs to isolate issues OMX specific
#define LOG_TAG "OMXCameraAdapter"

//frames skipped before recalculating the framerate
#define FPS_PERIOD 30

Mutex gAdapterLock;

static OMX_CALLBACKTYPE oCallbacks=
{
    &android::OMXCameraAdapterEventHandler,
    &android::OMXCameraAdapterEmptyBufferDone,
    &android::OMXCameraAdapterFillBufferDone,
};
static OMX_CALLBACKTYPE oCapCallbacks=
{
    &android::OMXCameraAdapterCapEventHandler,
    &android::OMXCameraAdapterCapEmptyBufferDone,
    &android::OMXCameraAdapterCapFillBufferDone,
};

#ifdef CAMERA_FRAME_STAT
struct filledBufferExtraInfo
{
    OMX_U32 portIndex;
    unsigned long long captureTime; 
    int captureIntervalTime;
};
static unsigned long long glastCaptureTime = 0;
static bool gbCaptureStarted= false;
#endif

static void watchdog_timout(void *obj, unsigned int msg)
{
    if(obj)
    {
        /**
        * NEW_FEATURE: Add buffer_state_dump function,when watchdog timeout happens.
        *ActionsCode(author:liyuan, change_code)
        */
	reinterpret_cast<OMXCameraAdapter *>(obj)->Hal_Dump_Bufs_Occupied();
        reinterpret_cast<OMXCameraAdapter *>(obj)->onWatchDogMsg(msg);
    }

}

/*--------------------Camera Adapter Class STARTS here-----------------------------*/

#ifndef CAMERA_USE_TEST_OMX
const char OMXCameraAdapter::CAMERA_OMX_NAME[] ="OMX.Action.Video.Camera";
#else
const char OMXCameraAdapter::CAMERA_OMX_NAME[] ="OMX.Action.Video.Camera.Test";
#endif

status_t OMXCameraAdapter::initialize(CameraProperties::Properties* caps)
{
    LOG_FUNCTION_NAME;

    const char *valstr = NULL;
    OMX_ACT_CONFIG_FlipParams mflipParam;
    CAMERA_SCOPEDTRACE("OMXCameraAdapter_initialize");
    char value[PROPERTY_VALUE_MAX];
    property_get("debug.camera.showfps", value, "0");
    mDebugFps = atoi(value);
    property_get("debug.camera.framecounts", value, "0");
    mDebugFcs = atoi(value);

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;

    mPending3Asettings = 0;//E3AsettingsAll;
    mPendingCaptureSettings = 0;

    if ( 0 != mInitSem.Count() )
    {
        CAMHAL_LOGEB("Error mInitSem semaphore count %d", mInitSem.Count());
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
    }

    if (mComponentState != OMX_StateLoaded && mComponentState != 0/*OMX_StateInvalid*/)
    {
        CAMHAL_LOGEB("Error mComponentState %d is invalid!", mComponentState);
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
    }

    ///Update the preview and image capture port indexes
    mCameraAdapterParameters.mPrevPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW;
    // temp changed in order to build OMX_CAMERA_PORT_VIDEO_OUT_IMAGE;
    mCameraAdapterParameters.mImagePortIndex = OMX_CAMERA_PORT_IMAGE_OUT_IMAGE;

    eError = OMX_Init();
    if (eError != OMX_ErrorNone)
    {
        CAMHAL_LOGEB("Error OMX_Init -0x%x", eError);
        return eError;
    }

    ///Get the handle to the OMX Component
    // Setup key parameters to send to Ducati during init

    // Initialize the callback handles
    {
    CAMERA_SCOPEDTRACE("OMXCameraAdapter_initialize_getHandle");
    eError = OMXCameraAdapter::OMXCameraGetHandle(&mCameraAdapterParameters.mHandleComp, (OMX_PTR)this, &oCallbacks);
    if(eError != OMX_ErrorNone)
    {
        CAMHAL_LOGEB("OMX_GetHandle -0x%x", eError);
    }
    GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
    }
    {
    CAMERA_SCOPEDTRACE("OMXCameraAdapter_initialize_disableallport");
    CAMHAL_LOGVB("OMX_GetHandle -0x%x sensor_index = %d", eError, mSensorIndex);
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                             OMX_CommandPortDisable,
                             OMX_ALL,
                             NULL);

    if(eError != OMX_ErrorNone)
    {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandPortDisable) -0x%x", eError);
    }
    GOTO_EXIT_IF((eError != OMX_ErrorNone), eError);
    }

    // Register for port enable event
    {
    CAMERA_SCOPEDTRACE("OMXCameraAdapter_initialize_portEnable");
        
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortEnable,
                           mCameraAdapterParameters.mPrevPortIndex,
                           mInitSem);
    if(ret != NO_ERROR)
    {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
    }

    // Enable PREVIEW Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                             OMX_CommandPortEnable,
                             mCameraAdapterParameters.mPrevPortIndex,
                             NULL);
    if(eError != OMX_ErrorNone)
    {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandPortEnable) -0x%x", eError);
    }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    // Wait for the port enable event to occur
    ret = mInitSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if ( NO_ERROR == ret )
    {
        CAMHAL_LOGDA("-Port enable event arrived");
    }
    else
    {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortEnable,
                           mCameraAdapterParameters.mPrevPortIndex,
                           NULL);
        CAMHAL_LOGEA("Timeout for enabling preview port expired!");
        goto EXIT;
    }
    }
    // Select the sensor
    {
    CAMERA_SCOPEDTRACE("OMXCameraAdapter_initialize_sensorselect");
    
    OMX_PARAM_SENSORSELECTTYPE sensorSelect;
    OMX_INIT_STRUCT_PTR (&sensorSelect, OMX_PARAM_SENSORSELECTTYPE);
    sensorSelect.eSensor = (OMX_SENSORSELECT) mSensorIndex;
    /**
    * NEW_FEATURE: Add UVC module support .
    *ActionsCode(author:liyuan, change_code)
    */
    if(get_UVC_ReplaceMode(&sensorSelect.uvcmode, mSensorIndex)==false){
	eError = OMX_ErrorStreamCorrupt;
	goto EXIT;
    }
    if((sensorSelect.uvcmode == OMX_UVC_AS_REAR &&mSensorIndex==0) ||
       (sensorSelect.uvcmode == OMX_UVC_AS_FRONT &&mSensorIndex==1)){
	CameraConfigs::setUVCModeActivate(sensorSelect.uvcmode);
    }
    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_ACT_IndexParamSensorSelect, &sensorSelect);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while selecting the sensor index as %d - 0x%x", mSensorIndex, eError);
        goto EXIT;
    }
    }

    //set sensor mode
    {
    CAMERA_SCOPEDTRACE("OMXCameraAdapter_initialize_sensormode");
        
    OMX_PARAM_SENSORMODETYPE sensorMode;
    OMX_INIT_STRUCT_PTR (&sensorMode, OMX_PARAM_SENSORMODETYPE);
    OMX_INIT_STRUCT_PTR (&sensorMode.sFrameSize, OMX_FRAMESIZETYPE);

    sensorMode.bOneShot = OMX_FALSE;
    sensorMode.nFrameRate = DEFAULT_FRAME_RATE;
    sensorMode.sFrameSize.nWidth = DEFAULT_PREVIEW_WIDTH;
    sensorMode.sFrameSize.nHeight = DEFAULT_PREVIEW_HEIGHT;
    sensorMode.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_IndexParamCommonSensorMode, &sensorMode);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while set sensor mode  0x%x", (unsigned int)eError);
       
        //eError = OMX_ErrorNone;
        //goto EXIT;
    }


    sensorMode.bOneShot = OMX_TRUE;
    sensorMode.nFrameRate = DEFAULT_FRAME_RATE;
    sensorMode.sFrameSize.nWidth = DEFAULT_PICTURE_WIDTH;
    sensorMode.sFrameSize.nHeight = DEFAULT_PICTURE_HEIGHT;
    sensorMode.nPortIndex = mCameraAdapterParameters.mImagePortIndex;
    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_IndexParamCommonSensorMode, &sensorMode);
    if ( OMX_ErrorNone != eError )
    {
        CAMHAL_LOGEB("Error while sset sensor mode  0x%x", (unsigned int)eError);
        
        //eError = OMX_ErrorNone;
        //goto EXIT;
    }

    }
    

    //printComponentVersion(mCameraAdapterParameters.mHandleComp);

    mOMXStateSwitch = false;

    mCaptureSignalled = false;
    mCaptureConfigured = false;
    mCaptureUsePreviewFrame = false;
    mCaptureUsePreviewFrameStarted = false;

    mFlashStrobed = false;
	mFocusAreasset = false;

    mFlashConvergenceFrameConfig = 4;

    mRecording = false;
    mComponentState = OMX_StateLoaded;

    mVstabEnabled = false;
    mVnfEnabled = false;
    mBurstFrames = 1;
    mCapturedFrames = 0;
    mPictureQuality = 100;
    mCurrentZoomIdx = 0;
    mTargetZoomIdx = 0;
    mPreviousZoomIndx = -1;
    mReturnZoomStatus = false;
    mZoomInc = 1;
    mZoomParameterIdx = 0;
    mSensorOverclock = false;

    mCheckAF = false;

    mIternalRecordingHint = false;

    mDeviceOrientation = 0;
    mCapabilities = caps;
    mZoomUpdating = false;
    mZoomUpdate = false;

    mSnapshot = CameraConfigs::getSnapshot(mSensorIndex) != 0;
    mHdrSupport = CameraConfigs::getHdrSupported(mSensorIndex) != 0;
    mFlashSupport = CameraConfigs::getFlashSupported(mSensorIndex) != 0;
    mHflip = CameraConfigs::getHflip(mSensorIndex) != 0;
    mVflip = CameraConfigs::getVflip(mSensorIndex) != 0;    	

    if(mFlashSupport)
    {
        valstr = mCapabilities->get(android::CameraProperties::FLASH_SUPPORTED);
        if(valstr != NULL)
        {
            if (strcmp(valstr, CameraParameters::FALSE) == 0)
            {
                mFlashSupport = false;
            }
        }
    }


    mEXIFData.mGPSData.mAltitudeValid = false;
    mEXIFData.mGPSData.mDatestampValid = false;
    mEXIFData.mGPSData.mLatValid = false;
    mEXIFData.mGPSData.mLongValid = false;
    mEXIFData.mGPSData.mMapDatumValid = false;
    mEXIFData.mGPSData.mProcMethodValid = false;
    mEXIFData.mGPSData.mVersionIdValid = false;
    mEXIFData.mGPSData.mTimeStampValid = false;
    mEXIFData.mModelValid = false;
    mEXIFData.mMakeValid = false;

    // mDeviceType init
    mDeviceType = 0;
    valstr = mCapabilities->get(android::CameraProperties::FACING_INDEX);
    if(valstr != NULL)
    {
        if (strcmp(valstr, (const char *) android::ActCameraParameters::FACING_FRONT) == 0)
        {
            mDeviceType = 1;
        }  
    }


    //get buffer nums
    OMX_PARAM_PORTDEFINITIONTYPE portDef;

    OMX_INIT_STRUCT_PTR (&portDef, OMX_PARAM_PORTDEFINITIONTYPE);

    portDef.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;

    eError = OMX_GetParameter (mCameraAdapterParameters.mHandleComp,
                               OMX_IndexParamPortDefinition, &portDef); 

    CAMHAL_LOGDB("preview buffer count  %d", ( int)portDef.nBufferCountActual);  
    if(portDef.nBufferCountActual >0 && portDef.nBufferCountActual<20)
    {
        char tmp_str[12];
        snprintf((char *)&tmp_str, 11, "%d", (int)portDef.nBufferCountActual);
        mCapabilities->set(CameraProperties::REQUIRED_PREVIEW_BUFS, (char *)&tmp_str);
    }
    OMX_INIT_STRUCT_PTR (&portDef, OMX_PARAM_PORTDEFINITIONTYPE);

    portDef.nPortIndex = mCameraAdapterParameters.mImagePortIndex;

    eError = OMX_GetParameter (mCameraAdapterParameters.mHandleComp,
                               OMX_IndexParamPortDefinition, &portDef); 
    CAMHAL_LOGDB("image buffer count  %d", ( int)portDef.nBufferCountActual);  
    if(portDef.nBufferCountActual >0 && portDef.nBufferCountActual<20)
    {
        char tmp_str[12];
        snprintf((char *)&tmp_str, 11, "%d", (int)portDef.nBufferCountActual);
        mCapabilities->set(CameraProperties::REQUIRED_IMAGE_BUFS, (char *)&tmp_str);
    }
	 mflipParam.nPortIndex = OMX_ALL;
   mflipParam.mhflip = (mHflip?OMX_TRUE:OMX_FALSE);
   mflipParam.mvflip = (mVflip?OMX_TRUE:OMX_FALSE);   
	 eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp, ( OMX_INDEXTYPE ) OMX_ACT_IndexConfigFlip, &mflipParam);
	
    // initialize command handling thread
    if(mCommandHandler.get() == NULL)
        mCommandHandler = new CommandHandler(this);

    if ( NULL == mCommandHandler.get() )
    {
        CAMHAL_LOGEA("Couldn't create command handler");
        return NO_MEMORY;
    }

    ret = mCommandHandler->run("CallbackThread", PRIORITY_URGENT_DISPLAY);
    if ( ret != NO_ERROR )
    {
        if( ret == INVALID_OPERATION)
        {
            CAMHAL_LOGDA("command handler thread already runnning!!");
            ret = NO_ERROR;
        }
        else
        {
            CAMHAL_LOGEA("Couldn't run command handlerthread");
            return ret;
        }
    }

    if(mAFHandler.get() == NULL)
        mAFHandler = new AFHandler(this);

    if ( NULL == mAFHandler.get() )
    {
        CAMHAL_LOGEA("Couldn't create af handler");
        return NO_MEMORY;
    }

    ret = mAFHandler->run("CallbackThread", PRIORITY_URGENT_DISPLAY);
    if ( ret != NO_ERROR )
    {
        if( ret == INVALID_OPERATION)
        {
            CAMHAL_LOGDA("af handler thread already runnning!!");
            ret = NO_ERROR;
        }
        else
        {
            CAMHAL_LOGEA("Couldn't run af handlerthread");
            return ret;
        }
    }

    // initialize omx callback handling thread
    if(mOMXCallbackHandler.get() == NULL)
        mOMXCallbackHandler = new OMXCallbackHandler(this);

    if ( NULL == mOMXCallbackHandler.get() )
    {
        CAMHAL_LOGEA("Couldn't create omx callback handler");
        return NO_MEMORY;
    }

    ret = mOMXCallbackHandler->run("OMXCallbackThread", PRIORITY_URGENT_DISPLAY);
    if ( ret != NO_ERROR )
    {
        if( ret == INVALID_OPERATION)
        {
            CAMHAL_LOGDA("omx callback handler thread already runnning!!");
            ret = NO_ERROR;
        }
        else
        {
            CAMHAL_LOGEA("Couldn't run omx callback handler thread");
            return ret;
        }
    }

    //Remove any unhandled events
    if (!mEventSignalQ.isEmpty())
    {
        for (unsigned int i = 0 ; i < mEventSignalQ.size(); i++ )
        {
            ActUtils::Message *msg = mEventSignalQ.itemAt(i);
            //remove from queue and free msg
            if ( NULL != msg )
            {
                free(msg);
            }
        }
        mEventSignalQ.clear();
    }


    //Setting this flag will that the first setParameter call will apply all 3A settings
    //and will not conditionally apply based on current values.
    mFirstTimeInit = true;

    mFaceDetectionRunning = false;
    mFaceDetectionPaused = false;

    memset(&mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex], 0, sizeof(OMXCameraPortParameters));
    memset(&mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex], 0, sizeof(OMXCameraPortParameters));

    ret = initFaceDetection();
    if ( NO_ERROR != ret )
    {
        CAMHAL_LOGEA("Couldn't mVceFaceDetect.init!");
        goto EXIT;
    }

    //Initialize 3A defaults
    ret = apply3ADefaults(mParameters3A);
    if ( NO_ERROR != ret )
    {
        CAMHAL_LOGEA("Couldn't apply 3A defaults!");
        goto EXIT;
    }

#ifdef CAMERA_WATCHDOG
    mWatchDog = new CameraWatchDog();
#endif

    LOG_FUNCTION_NAME_EXIT;
    return ErrorUtils::omxToAndroidError(eError);

EXIT:

    CAMHAL_LOGDB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return ErrorUtils::omxToAndroidError(eError);
}

void OMXCameraAdapter::performCleanupAfterError()
{
    if(mCameraAdapterParameters.mHandleComp)
    {
        ///Free the OMX component handle in case of error
        OMX_FreeHandle(mCameraAdapterParameters.mHandleComp);
        mCameraAdapterParameters.mHandleComp = NULL;
    }

    ///De-init the OMX
    OMX_Deinit();
    mComponentState = (OMX_STATETYPE)0/*OMX_StateInvalid*/;
}

OMXCameraAdapter::OMXCameraPortParameters *OMXCameraAdapter::getPortParams(CameraFrame::StreamType streamType, OMX_U32 &portIndex)
{
    OMXCameraAdapter::OMXCameraPortParameters *ret = NULL;

    switch ( streamType )
    {
    case CameraFrame::IMAGE_STREAM_TYPE:
        ret = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];
        portIndex = mCameraAdapterParameters.mImagePortIndex;
        break;
    case CameraFrame::PREVIEW_STREAM_TYPE:
        ret = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
        portIndex = mCameraAdapterParameters.mPrevPortIndex;
        break;
    default:
        break;
    };

    return ret;
}
void OMXCameraAdapter::onFillFrame(void * frameBuf, unsigned int frameTypes, CameraFrame::StreamType streamType)
{
}

void OMXCameraAdapter::onReturnFrame(void * frameBuf, unsigned int returnFrameType, unsigned int frameTypes, CameraFrame::StreamType streamType)
{
    /*
    if((CameraFrame::IMAGE_FRAME_SYNC & returnFrameType) || (CameraFrame::RAW_FRAME_SYNC & returnFrameType))
    {
        ALOGV("frameBuf=0x%x ", frameBuf);
        ALOGV("mCapturedFrames=%d ", mCapturedFrames);
        ALOGV("frameTypes=0x%x ", frameTypes);
        ALOGV("returnFrameType=0x%x ", returnFrameType);
        ALOGV("count=%d ", mStreamHub->getRefCountByFrameTypes((uint32_t)(CameraFrame::RAW_FRAME_SYNC|CameraFrame::IMAGE_FRAME_SYNC)));
    }*/
    //free exifs
    if (CameraFrame::IMAGE_FRAME_SYNC & returnFrameType)
    {
        freeExifObject(frameBuf);
    }
    if (((CameraFrame::IMAGE_FRAME_SYNC & returnFrameType) || (CameraFrame::RAW_FRAME_SYNC & returnFrameType)) 
        && ((BaseCameraAdapter::getState()& CameraAdapter::CAPTURE_ACTIVE))
        && (1 > mCapturedFrames)
        && (mStreamHub->getRefCountByFrameTypes((uint32_t)(CameraFrame::RAW_FRAME_SYNC|CameraFrame::IMAGE_FRAME_SYNC))==0))
    {
        ALOGV("endImageCallback\n");
        // Signal end of image capture
        endImageCallback();
        return ;
    }
}


status_t OMXCameraAdapter::fillThisBuffer(void* frameBuf, unsigned int frameTypes, CameraFrame::StreamType streamType)
{
    status_t ret = NO_ERROR;
    OMXCameraPortParameters *port = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);
    OMX_U32 portIndex = -1;

    if ( ( PREVIEW_ACTIVE & state ) != PREVIEW_ACTIVE )
    {
        return NO_INIT;
    }

    if ( NULL == frameBuf )
    {
        return -EINVAL;
    }



    if ( NO_ERROR == ret )
    {
        port = getPortParams(streamType, portIndex);
        if ( NULL == port )
        {
            CAMHAL_LOGEB("Invalid frameTypes 0x%x", frameTypes);
            ret = -EINVAL;
        }
    }

    if ( NO_ERROR == ret && (streamType == CameraFrame::IMAGE_STREAM_TYPE && mCapturedFrames<1))
    {
        CAMHAL_LOGDA("captured ended, do not fill image buffer");
        ret = -EINVAL;
    }

    if ( NO_ERROR == ret )
    {

        Mutex::Autolock lock(port->mLock);                        
        if(port->mPortEnable)
        {
            for ( int i = 0 ; i < port->mNumBufs ; i++)
            {
                if ( ((video_metadata_t *)(port->mBufferHeader[i]->pBuffer))->handle == frameBuf )
                {
                    CAMHAL_LOGDB("Queuing buffer on Preview port,header=0x%x, pBuffer= 0x%x", (uint32_t)port->mBufferHeader[i], (uint32_t)port->mBufferHeader[i]->pBuffer);
                    //port->mBufferHeader[i]->nFilledLen = 0;
                    eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp, port->mBufferHeader[i]);
                    if ( eError != OMX_ErrorNone )
                    {
                        CAMHAL_LOGEB("OMX_FillThisBuffer 0x%x", eError);
                        goto EXIT;
                    }
                    if( (portIndex == mCameraAdapterParameters.mPrevPortIndex))
                    {
#ifdef DEBUG_LOG
                        if(mBuffersWithOMX.indexOfKey((int)frameBuf)>=0)
                        {
                            CAMHAL_LOGEB("Buffer already with OMX!! 0x%p", frameBuf);
                            for(unsigned int i=0; i<mBuffersWithOMX.size(); i++)
                            {
                                CAMHAL_LOGEB("0x%x", mBuffersWithOMX.keyAt(i));
                            }
                        }
                        mBuffersWithOMX.add((int)frameBuf,1);
#endif
                        mFramesWithOMX++;
                    }
                    break;
                }
            }
        }
    }

    LOG_FUNCTION_NAME_EXIT;
    return ret;

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    //Since fillthisbuffer is called asynchronously, make sure to signal error to the app
    mErrorNotifier->errorNotify(CAMERA_ERROR_HARD);
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

/**
 *
 * BUGFIX:  Fix for NV21 enum value to OMX.
 * BUGFIX:  Fix PIXEL_FORMAT_YUV422I align format OMX_COLOR_FormatYCbYCr; Add support for PIXEL_FORMAT_YUV422SP.
 *
 ************************************
 *
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXCameraAdapter::setParameters(const CameraParameters &params)
{
    LOG_FUNCTION_NAME;

    const char * str = NULL;
    int mode = 0;
    status_t ret = NO_ERROR;
    bool updateImagePortParams = false;
    OMX_U32 minFramerate, maxFramerate, frameRate;
    const char *valstr = NULL;
    const char *oldstr = NULL;
    int w, h;
    int cw, ch;
    int cropW, cropH;
    int i;
    int bpp = 16;
    //ActionsCode(author:liuyiguang, change_code)
    int pixFormat;
    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);

    ///@todo Include more camera parameters
    if ( (valstr = params.getPreviewFormat()) != NULL )
    {
        if (strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV422I) == 0)
        {
            CAMHAL_LOGDA("YCbYCr format selected");
            //ActionsCode(author:liuyiguang, change_code, fix YUV422I align YCbYCr, YUYV)
            pixFormat = OMX_COLOR_FormatYCbYCr;
            bpp=16;
        }
        //ActionsCode(author:liuyiguang, add_code, add support for PIXEL_FORMAT_YUV422SP)
        else if (strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV422SP) == 0)
        {
            CAMHAL_LOGDA("CbYCrY format selected");
            pixFormat = OMX_COLOR_FormatCbYCrY;
            bpp=16;
        }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV420SP) == 0 )
        {
            CAMHAL_LOGDA("YUV420SP format selected");
            //ActionsCode(author:liuyiguang, change_code, for omx, NV21 is OMX_COLOR_FormatYVU420SemiPlanar)
            pixFormat = OMX_COLOR_FormatYVU420SemiPlanar;
            bpp =12;
        }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_YUV420P) == 0)
        {
            CAMHAL_LOGDA("YUV420SP format selected");
            //ActionsCode(author:liuyiguang, change_code, for omx, YV12 is OMX_COLOR_FormatYVU420Planar)
            pixFormat = OMX_COLOR_FormatYVU420Planar;
            bpp =12;
        }
        else if(strcmp(valstr, (const char *) CameraParameters::PIXEL_FORMAT_RGB565) == 0)
        {
            CAMHAL_LOGDA("RGB565 format selected");
            pixFormat = OMX_COLOR_Format16bitRGB565;
            bpp = 16;
        }
        /**
        *BUGFIX: cts camera format for yv12.
        *ActionsCode(author:liyuan, change_code)
        */
        else if(strcmp(valstr, (const char *) ActCameraParameters::PIXEL_FORMAT_YUV420P_YU12) == 0)
    	{
        	pixFormat = OMX_COLOR_FormatYUV420Planar;
            bpp =12;
					  
        }
        else if(strcmp(valstr, (const char *) ActCameraParameters::PIXEL_FORMAT_YUV420SP_NV12) == 0)
        {
        	pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            bpp =12;
        }
        else
        {
            CAMHAL_LOGDA("Invalid format, CbYCrY format selected as default");
            pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            bpp=16;
        }
    }
    else
    {
        CAMHAL_LOGEA("Preview format is NULL, defaulting to CbYCrY");
        pixFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    }

    OMXCameraPortParameters *cap;
    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    params.getPreviewSize(&w, &h);
    frameRate = params.getPreviewFrameRate();



    if ( 0 < frameRate )
    {
        if(frameRate != cap->mFrameRate)
        {
            setFramerate(frameRate);
        }
    }

    for(i =0; i < (int)mCapabilities->mExtendedVideoResolution.size(); i++)
    {
        const ExtendedResolution &extVideoRes = mCapabilities->mExtendedVideoResolution.itemAt(i);
        if(w == (int)extVideoRes.width && h == (int)extVideoRes.height)
        {
            cw = extVideoRes.captureWidth; 
            ch = extVideoRes.captureHeight; 
            if(extVideoRes.needCrop)
            {
                cropW = extVideoRes.cropWidth;
                cropH = extVideoRes.cropHeight;
            }
            else
            {
                cropW = extVideoRes.captureWidth;
                cropH = extVideoRes.captureHeight;
            }
            break;
        }
    }     
    if(i >= (int)mCapabilities->mExtendedVideoResolution.size())
    {
        cw = w;
        ch = h;
        cropW = w;
        cropH = h;
    }
    
    cap->mFrameRate = frameRate;

    cap->mPendingSize = true;
    cap->mPendingWidth = cw;
    cap->mPendingHeight = ch;
    cap->mPendingEncodeWidth = w;
    cap->mPendingEncodeHeight = h;   
    cap->mPendingCropWidth = cropW;
    cap->mPendingCropHeight = cropH;
    cap->mPendingStride = (cw*bpp)>>3;
    cap->mPendingBufSize = cap->mPendingStride * cap->mPendingHeight;

    //ActionsCode(author:liuyiguang, change_code)
    cap->mColorFormat = (OMX_COLOR_FORMATTYPE)pixFormat;
    cap->mCompressionFormat = OMX_IMAGE_CodingUnused;

    CAMHAL_LOGDB("Prev: cap.mColorFormat = %d", (int)cap->mColorFormat);
    CAMHAL_LOGDB("Prev: cap.mWidth = %d", (int)cap->mWidth);
    CAMHAL_LOGDB("Prev: cap.mHeight = %d", (int)cap->mHeight);
    CAMHAL_LOGDB("Prev: cap.mFrameRate = %d", (int)cap->mFrameRate);


    valstr = params.get(CameraParameters::KEY_RECORDING_HINT);
    if((valstr != NULL) && (strcmp(valstr, CameraParameters::TRUE) == 0))
    {
        //recording
        if(mIternalRecordingHint != true)
        {
            mIternalRecordingHint = true;
            if(strcmp(mCapabilities->get(CameraProperties::FOCUS_SUPPORTED), CameraParameters::TRUE) == 0)
            {
                mPending3Asettings |= SetFocus;
            }
        }
    }
    else
    {
        //preview
        if(mIternalRecordingHint != false)
        {
            mIternalRecordingHint = false;
            if(strcmp(mCapabilities->get(CameraProperties::FOCUS_SUPPORTED), CameraParameters::TRUE) == 0)
            {
                mPending3Asettings |= SetFocus;
            }
        }
    }
               

    ret |= setParametersCapture(params, state);

    ret |= setParameters3A(params, state);

    ret |= setParametersFocus(params, state);

    ret |= setParametersFD(params, state);

    ret |= setParametersZoom(params, state);

    ret |= setParametersEXIF(params, state);


    mParams = params;
    mFirstTimeInit = false;

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}



void OMXCameraAdapter::getParameters(CameraParameters& params)
{
    status_t ret = NO_ERROR;
    OMX_CONFIG_EXPOSUREVALUETYPE exp;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);
    const char *valstr = NULL;
    LOG_FUNCTION_NAME;

    const char *valstr_supported = NULL;


    if(strcmp(mCapabilities->get(CameraProperties::WHITEBALANCE_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        valstr = getLUTvalue_OMXtoHAL(mParameters3A.WhiteBallance, WBalLUT);
        valstr_supported = mParams.get(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE);
        if (valstr && valstr_supported && strstr(valstr_supported, valstr))
            params.set(CameraParameters::KEY_WHITE_BALANCE , valstr);
    }
    else
    {
        params.set(CameraParameters::KEY_WHITE_BALANCE , CameraParameters::WHITE_BALANCE_AUTO);
    }

    valstr = mParams.get(CameraParameters::KEY_FLASH_MODE);
    if(valstr)
    {
        params.set(CameraParameters::KEY_FLASH_MODE, valstr);
    }
    if(strcmp(mCapabilities->get(CameraProperties::CameraProperties::FLASH_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        valstr = getLUTvalue_OMXtoHAL(mParameters3A.FlashMode, FlashLUT);
        valstr_supported = mParams.get(CameraParameters::KEY_SUPPORTED_FLASH_MODES);
        if (valstr && valstr_supported && strstr(valstr_supported, valstr))
        {
            params.set(CameraParameters::KEY_FLASH_MODE, valstr);
        }
    }

    valstr = mParams.get(CameraParameters::KEY_FOCUS_MODE);
    if(valstr)
    {
        params.set(CameraParameters::KEY_FOCUS_MODE, valstr);
    }
    if(strcmp(mCapabilities->get(CameraProperties::CameraProperties::FOCUS_SUPPORTED), CameraParameters::TRUE) == 0)
    {
        valstr_supported = mParams.get(CameraParameters::KEY_SUPPORTED_FOCUS_MODES);
        if (valstr && valstr_supported && strstr(valstr_supported, valstr))
            params.set(CameraParameters::KEY_FOCUS_MODE, valstr);
    }

    if(strcmp(mCapabilities->get(CameraProperties::FOCUS_SUPPORTED), CameraParameters::TRUE) == 0)
    {

        if ( ( AF_ACTIVE & state ) ||
            ( NULL == mParameters.get(CameraParameters::KEY_FOCUS_DISTANCES) ) )
        {
            updateFocusDistances(params);
        }
        else
        {
            valstr = mParameters.get(CameraParameters::KEY_FOCUS_DISTANCES);
            if(NULL != valstr)
            {
                params.set(CameraParameters::KEY_FOCUS_DISTANCES, valstr);
            }
        }
        if ( ( AF_ACTIVE & state ) ||
            ( NULL == mParameters.get(CameraParameters::KEY_FOCAL_LENGTH) ) )
        {
            updateFocusLength(params);
        }
        else
        {
            valstr = mParameters.get(CameraParameters::KEY_FOCAL_LENGTH);
            if(NULL != valstr)
            {
                params.set(CameraParameters::KEY_FOCAL_LENGTH, valstr);
            }
        }
    }
    else
    {
        valstr = mParams.get(CameraParameters::KEY_FOCUS_MODE);
        if(valstr && (strcmp(valstr,CameraParameters::FOCUS_MODE_INFINITY) == 0))
        {
            params.set(CameraParameters::KEY_FOCUS_DISTANCES, "Infinity,Infinity,Infinity");
        }
    }

    {
        Mutex::Autolock lock(mZoomLock);
        //Immediate zoom should not be avaialable while smooth zoom is running
        if ( ZOOM_ACTIVE & state )
        {
            if ( mZoomParameterIdx != mCurrentZoomIdx )
            {
                mZoomParameterIdx += mZoomInc;
            }
            params.set( CameraParameters::KEY_ZOOM, mZoomParameterIdx);
            if ( ( mCurrentZoomIdx == mTargetZoomIdx ) &&
                    ( mZoomParameterIdx == mCurrentZoomIdx ) )
            {

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

            }

            CAMHAL_LOGDB("CameraParameters Zoom = %d", mCurrentZoomIdx);
        }
        else
        {
            params.set( CameraParameters::KEY_ZOOM, mCurrentZoomIdx);
        }
    }

    //Populate current lock status
    if ( mParameters3A.ExposureLock )
    {
        params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK,
                   CameraParameters::TRUE);
    }
    else
    {
        params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK,
                   CameraParameters::FALSE);
    }

    if ( mParameters3A.WhiteBalanceLock )
    {
        params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK,
                   CameraParameters::TRUE);
    }
    else
    {
        params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK,
                   CameraParameters::FALSE);
    }

    LOG_FUNCTION_NAME_EXIT;
}

status_t OMXCameraAdapter::setFormat(OMX_U32 port, OMXCameraPortParameters &portParams)
{
    size_t bufferCount;

    LOG_FUNCTION_NAME;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE portCheck;

    OMX_INIT_STRUCT_PTR (&portCheck, OMX_PARAM_PORTDEFINITIONTYPE);

    portCheck.nPortIndex = port;



    //reset pending size
    if(portParams.mPendingSize == true)
    {
        portParams.mPendingSize = false;
        portParams.mWidth = portParams.mPendingWidth;
        portParams.mHeight = portParams.mPendingHeight;
        portParams.mEncodeWidth = portParams.mPendingEncodeWidth;
        portParams.mEncodeHeight = portParams.mPendingEncodeHeight;
        portParams.mCropWidth = portParams.mPendingCropWidth;
        portParams.mCropHeight = portParams.mPendingCropHeight;
        portParams.mStride = portParams.mPendingStride;
        portParams.mBufSize = portParams.mPendingBufSize;
    }


    eError = OMX_GetParameter (mCameraAdapterParameters.mHandleComp,
                               OMX_IndexParamPortDefinition, &portCheck);
    if(eError!=OMX_ErrorNone)
    {
        CAMHAL_LOGEB("OMX_GetParameter - %x", eError);
    }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    if ( mCameraAdapterParameters.mPrevPortIndex == port )
    {
        //portCheck.eDomain = OMX_PortDomainVideo;
        portCheck.format.video.nFrameWidth      = portParams.mWidth;
        portCheck.format.video.nFrameHeight     = portParams.mHeight;
        portCheck.format.video.eColorFormat     = portParams.mColorFormat;
        portCheck.format.video.nStride          = portParams.mStride;


        portCheck.format.video.xFramerate       = portParams.mFrameRate<<16;
        portCheck.nBufferSize                   = portParams.mStride * portParams.mHeight;
        portCheck.nBufferCountActual = portParams.mNumBufs;
        mFocusThreshold = FOCUS_THRESHOLD * portParams.mFrameRate;
    }
    else if ( mCameraAdapterParameters.mImagePortIndex == port )
    {
        //portCheck.eDomain = OMX_PortDomainImage;
        portCheck.format.image.nFrameWidth      = portParams.mWidth;
        portCheck.format.image.nFrameHeight     = portParams.mHeight;


        portCheck.format.image.eColorFormat     = portParams.mColorFormat;
        portCheck.format.image.eCompressionFormat = portParams.mCompressionFormat;//OMX_IMAGE_CodingUnused;

        portCheck.format.image.nStride          =  portParams.mStride;
        portCheck.nBufferSize                   =  portParams.mStride * portParams.mHeight;
        portCheck.nBufferCountActual = portParams.mNumBufs;
    }
    else
    {
        CAMHAL_LOGEB("Unsupported port index 0x%x", (unsigned int)port);
    }

    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                              OMX_IndexParamPortDefinition, &portCheck);
    if(eError!=OMX_ErrorNone)
    {
        CAMHAL_LOGEB("OMX_SetParameter - %x", eError);
    }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    /* check if parameters are set correctly by calling GetParameter() */
    eError = OMX_GetParameter(mCameraAdapterParameters.mHandleComp,
                              OMX_IndexParamPortDefinition, &portCheck);
    if(eError!=OMX_ErrorNone)
    {
        CAMHAL_LOGEB("OMX_GetParameter - %x", eError);
    }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    portParams.mBufSize = portCheck.nBufferSize;
    if ( mCameraAdapterParameters.mPrevPortIndex == port )
    {
        portParams.mStride = portCheck.format.video.nStride;
    }
    else  if ( mCameraAdapterParameters.mImagePortIndex == port )
    {
        portParams.mStride = portCheck.format.image.nStride;
    }
    else
    {
    }
    initZoom(port);//


    if ( OMX_CAMERA_PORT_IMAGE_OUT_IMAGE == port )
    {
        CAMHAL_LOGDB("\n *** IMG Width = %ld", portCheck.format.image.nFrameWidth);
        CAMHAL_LOGDB("\n ***IMG Height = %ld", portCheck.format.image.nFrameHeight);

        CAMHAL_LOGDB("\n ***IMG IMG FMT = %x", portCheck.format.image.eColorFormat);
        CAMHAL_LOGDB("\n ***IMG portCheck.nBufferSize = %ld\n",portCheck.nBufferSize);
        CAMHAL_LOGDB("\n ***IMG portCheck.nBufferCountMin = %ld\n",
                     portCheck.nBufferCountMin);
        CAMHAL_LOGDB("\n ***IMG portCheck.nBufferCountActual = %ld\n",
                     portCheck.nBufferCountActual);
        CAMHAL_LOGDB("\n ***IMG portCheck.format.image.nStride = %ld\n",
                     portCheck.format.image.nStride);
    }
    else
    {
        CAMHAL_LOGDB("\n *** PRV Width = %ld", portCheck.format.video.nFrameWidth);
        CAMHAL_LOGDB("\n ***PRV Height = %ld", portCheck.format.video.nFrameHeight);

        CAMHAL_LOGDB("\n ***PRV IMG FMT = %x", portCheck.format.video.eColorFormat);
        CAMHAL_LOGDB("\n ***PRV portCheck.nBufferSize = %ld\n",portCheck.nBufferSize);
        CAMHAL_LOGDB("\n ***PRV portCheck.nBufferCountMin = %ld\n",
                     portCheck.nBufferCountMin);
        CAMHAL_LOGDB("\n ***PRV portCheck.nBufferCountActual = %ld\n",
                     portCheck.nBufferCountActual);
        CAMHAL_LOGDB("\n ***PRV portCheck.format.video.nStride = %ld\n",
                     portCheck.format.video.nStride);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);

EXIT:

    CAMHAL_LOGEB("Exiting function %s because of eError=%x", __FUNCTION__, eError);

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::setFramerate(OMX_U32 framerate)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;
    CAMHAL_LOGDB("setFramerate %d", (int)framerate);
    //port preview
    OMX_ACT_CONFIG_VIDEOPARAM videoParam;
    OMX_INIT_STRUCT_PTR (&videoParam, OMX_ACT_CONFIG_VIDEOPARAM);
    videoParam.nPortIndex = OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW;

    eError = OMX_GetConfig(mCameraAdapterParameters.mHandleComp,
                              (OMX_INDEXTYPE)OMX_ACT_IndexConfigVideoParam, &videoParam);
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    videoParam.nFramerate = framerate;

    eError = OMX_SetConfig(mCameraAdapterParameters.mHandleComp,
                              (OMX_INDEXTYPE)OMX_ACT_IndexConfigVideoParam, &videoParam);
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
EXIT:
    LOG_FUNCTION_NAME_EXIT;

    return ErrorUtils::omxToAndroidError(eError);
}

status_t OMXCameraAdapter::flushBuffers()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if ( 0 != mFlushSem.Count() )
    {
        CAMHAL_LOGEB("Error mFlushSem semaphore count %d", mFlushSem.Count());
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
    }

    LOG_FUNCTION_NAME;

    OMXCameraPortParameters * mPreviewData = NULL;
    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    ///Register for the FLUSH event
    ///This method just inserts a message in Event Q, which is checked in the callback
    ///The sempahore passed is signalled by the callback
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandFlush,
                           OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW,
                           mFlushSem);
    if(ret!=NO_ERROR)
    {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
    }

    ///Send FLUSH command to preview port
    eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp,
                              OMX_CommandFlush,
                              mCameraAdapterParameters.mPrevPortIndex,
                              NULL);

    if(eError!=OMX_ErrorNone)
    {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_CommandFlush)-0x%x", eError);
    }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    CAMHAL_LOGDA("Waiting for flush event");

    ///Wait for the FLUSH event to occur
    ret = mFlushSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == 0/*OMX_StateInvalid*/)
    {
        CAMHAL_LOGEA("Invalid State after Flush Exitting!!!");
        goto EXIT;
    }

    if ( NO_ERROR == ret )
    {
        CAMHAL_LOGDA("Flush event received");
    }
    else
    {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandFlush,
                           OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW,
                           NULL);
        CAMHAL_LOGDA("Flush event timeout expired");
        goto EXIT;
    }

    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

void OMXCameraAdapter::clearMetadataBufs(OMXCameraPortParameters &portparam)
{
    size_t i = 0;

    for(i = 0; i < portparam.mMetadataNum; i++)
    {
        free((void *)portparam.mMetadataBufs[i]);
        portparam.mMetadataBufs[i] = 0;
    }
    portparam.mMetadataNum = 0;
}

///API to give the buffers to Adapter
status_t OMXCameraAdapter::useBuffers(CameraMode mode, void* bufArr, int num, size_t length, unsigned int queueable)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    switch(mode)
    {
    case CAMERA_PREVIEW:
    case CAMERA_VIDEO:
        mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex].mNumBufs =  num;
        mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex].mMaxQueueable = queueable;
        ret = UseBuffersPreview(bufArr, num);
        break;

    case CAMERA_IMAGE_CAPTURE:
        mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex].mNumBufs = num;
        mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex].mMaxQueueable = queueable;
        ret = UseBuffersCapture(bufArr, num);
        break;

    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::switchToExecuting()
{
    status_t ret = NO_ERROR;
    ActUtils::Message msg;

    LOG_FUNCTION_NAME;

    mStateSwitchLock.lock();
    msg.command = CommandHandler::CAMERA_SWITCH_TO_EXECUTING;
    msg.arg1 = mErrorNotifier;
    ret = mCommandHandler->put(&msg);

    LOG_FUNCTION_NAME;

    return ret;
}

status_t OMXCameraAdapter::doSwitchToExecuting()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    LOG_FUNCTION_NAME;

    if ( (mComponentState == OMX_StateExecuting) || (mComponentState == 0/*OMX_StateInvalid*/) )
    {
        CAMHAL_LOGDA("Already in OMX_Executing state or OMX_StateInvalid state");
        mStateSwitchLock.unlock();
        return NO_ERROR;
    }

    if ( 0 != mSwitchToExecSem.Count() )
    {
        CAMHAL_LOGEB("Error mSwitchToExecSem semaphore count %d", mSwitchToExecSem.Count());
        goto EXIT;
    }

    ///Register for Preview port DISABLE  event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortDisable,
                           mCameraAdapterParameters.mPrevPortIndex,
                           mSwitchToExecSem);
    if ( NO_ERROR != ret )
    {
        CAMHAL_LOGEB("Error in registering Port Disable for event %d", ret);
        goto EXIT;
    }
    ///Disable Preview Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                             OMX_CommandPortDisable,
                             mCameraAdapterParameters.mPrevPortIndex,
                             NULL);
    ret = mSwitchToExecSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if (ret != NO_ERROR)
    {
        CAMHAL_LOGEB("Timeout PREVIEW PORT DISABLE %d", ret);
    }

    CAMHAL_LOGVB("PREV PORT DISABLED %d", ret);



    ///Register for IDLE state switch event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandStateSet,
                           OMX_StateIdle,
                           mSwitchToExecSem);
    if(ret!=NO_ERROR)
    {
        CAMHAL_LOGEB("Error in IDLE STATE SWITCH %d", ret);
        goto EXIT;
    }
    eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp ,
                              OMX_CommandStateSet,
                              OMX_StateIdle,
                              NULL);
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
    ret = mSwitchToExecSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if (ret != NO_ERROR)
    {
        CAMHAL_LOGEB("Timeout IDLE STATE SWITCH %d", ret);
        goto EXIT;
    }
    mComponentState = OMX_StateIdle;
    CAMHAL_LOGVB("OMX_SendCommand(OMX_StateIdle) 0x%x", eError);

    ///Register for EXECUTING state switch event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandStateSet,
                           OMX_StateExecuting,
                           mSwitchToExecSem);
    if(ret!=NO_ERROR)
    {
        CAMHAL_LOGEB("Error in EXECUTING STATE SWITCH %d", ret);
        goto EXIT;
    }
    eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp ,
                              OMX_CommandStateSet,
                              OMX_StateExecuting,
                              NULL);
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
    ret = mSwitchToExecSem.WaitTimeout(OMX_CMD_TIMEOUT);
    if (ret != NO_ERROR)
    {
        CAMHAL_LOGEB("Timeout EXEC STATE SWITCH %d", ret);
        goto EXIT;
    }
    mComponentState = OMX_StateExecuting;
    CAMHAL_LOGVB("OMX_SendCommand(OMX_StateExecuting) 0x%x", eError);

    mStateSwitchLock.unlock();

    LOG_FUNCTION_NAME_EXIT;
    return ret;

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    mStateSwitchLock.unlock();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::disablePreviewPort()
{
    status_t ret = NO_ERROR;
    OMX_CONFIG_PORTBOOLEANTYPE bOMX;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters * mPreviewData = NULL;
    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    CAMERA_SCOPEDTRACE("disablepreviewport");

    {
        Mutex::Autolock lock(mPreviewData->mLock);
        mPreviewData->mPortEnable = false;
    }
    ///Register for Preview port Disable event
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortDisable,
                           mCameraAdapterParameters.mPrevPortIndex,
                           mStopPreviewSem);

    ///Disable Preview Port
    eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                             OMX_CommandPortDisable,
                             mCameraAdapterParameters.mPrevPortIndex,
                             NULL);

    ///Free the OMX Buffers
    for ( int i = 0 ; i < mPreviewData->mNumBufs ; i++ )
    {
        eError = OMX_FreeBuffer(mCameraAdapterParameters.mHandleComp,
                                mCameraAdapterParameters.mPrevPortIndex,
                                mPreviewData->mBufferHeader[i]);

        if(eError!=OMX_ErrorNone)
        {
            CAMHAL_LOGEB("OMX_FreeBuffer - %x", eError);
        }
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
    }

    CAMHAL_LOGDA("Disabling preview port");
    ret = mStopPreviewSem.WaitTimeout(OMX_CMD_TIMEOUT);

     //should after port disable, otherwise onEmptyBufferDone will reference freed buffer
    clearMetadataBufs(*mPreviewData);

    //If somethiing bad happened while we wait
    if (mComponentState == 0/*OMX_StateInvalid*/)
    {
        CAMHAL_LOGEA("Invalid State after Disabling preview port Exitting!!!");
        goto EXIT;
    }

    if ( NO_ERROR == ret )
    {
        CAMHAL_LOGDA("Preview port disabled");
    }
    else
    {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandPortDisable,
                           mCameraAdapterParameters.mPrevPortIndex,
                           NULL);
        CAMHAL_LOGEA("Timeout expired on preview port disable");
        goto EXIT;
    }

EXIT:
    return (ret | ErrorUtils::omxToAndroidError(eError));
}


status_t OMXCameraAdapter::switchToLoaded()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mStateSwitchLock);
    CAMERA_SCOPEDTRACE("switchToLoaded");

    if ( mComponentState == OMX_StateLoaded  || mComponentState == 0/*OMX_StateInvalid*/)
    {
        CAMHAL_LOGDA("Already in OMX_Loaded state or OMX_StateInvalid state");
        return NO_ERROR;
    }

    if ( 0 != mSwitchToLoadedSem.Count() )
    {
        CAMHAL_LOGEB("Error mSwitchToLoadedSem semaphore count %d", mSwitchToLoadedSem.Count());
        goto EXIT;
    }

    ///Register for EXECUTING state transition.
    ///This method just inserts a message in Event Q, which is checked in the callback
    ///The sempahore passed is signalled by the callback
    if( mComponentState == OMX_StateExecuting)
    {
        CAMERA_SCOPEDTRACE("switchToLoaded-idle");
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandStateSet,
                                OMX_StateIdle,
                                mSwitchToLoadedSem);

        if(ret!=NO_ERROR)
        {
            CAMHAL_LOGEB("Error in registering for event %d", ret);
            goto EXIT;
        }

        eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp,
                                    OMX_CommandStateSet,
                                    OMX_StateIdle,
                                    NULL);

        if(eError!=OMX_ErrorNone)
        {
            CAMHAL_LOGEB("OMX_SendCommand(OMX_StateIdle) - %x", eError);
        }

        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        ///Wait for the EXECUTING ->IDLE transition to arrive

        CAMHAL_LOGDA("EXECUTING->IDLE state changed");
        ret = mSwitchToLoadedSem.WaitTimeout(OMX_CMD_TIMEOUT);


        //If somethiing bad happened while we wait
        if (mComponentState == 0/*OMX_StateInvalid*/)
        {
            CAMHAL_LOGEA("Invalid State after EXECUTING->IDLE Exitting!!!");
            goto EXIT;
        }

        if ( NO_ERROR == ret )
        {
            CAMHAL_LOGDA("EXECUTING->IDLE state changed");
        }
        else
        {
            ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandStateSet,
                                OMX_StateIdle,
                                NULL);
            CAMHAL_LOGEA("Timeout expired on EXECUTING->IDLE state change");
            goto EXIT;
        }

        mComponentState = OMX_StateIdle;
    }

    ///Register for LOADED state transition.
    ///This method just inserts a message in Event Q, which is checked in the callback
    ///The sempahore passed is signalled by the callback
    if (mComponentState == OMX_StateIdle)
    {
        //disable both port

        disablePreviewPort();
        disableImagePort();
        CAMERA_SCOPEDTRACE("switchToLoaded-loaded");

        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandStateSet,
                                OMX_StateLoaded,
                                mSwitchToLoadedSem);

        if(ret!=NO_ERROR)
        {
            CAMHAL_LOGEB("Error in registering for event %d", ret);
            goto EXIT;
        }

        eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp,
                                    OMX_CommandStateSet,
                                    OMX_StateLoaded,
                                    NULL);

        if(eError!=OMX_ErrorNone)
        {
            CAMHAL_LOGEB("OMX_SendCommand(OMX_StateLoaded) - %x", eError);
        }
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        CAMHAL_LOGDA("Switching IDLE->LOADED state");

        ret = mSwitchToLoadedSem.WaitTimeout(OMX_CMD_TIMEOUT);
        //If somethiing bad happened while we wait
        if (mComponentState == 0/*OMX_StateInvalid*/)
        {
            CAMHAL_LOGEA("Invalid State after IDLE->LOADED Exitting!!!");
            goto EXIT;
        }

        if ( NO_ERROR == ret )
        {
            CAMHAL_LOGDA("IDLE->LOADED state changed");
        }
        else
        {
            ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandStateSet,
                                OMX_StateLoaded,
                                NULL);
            CAMHAL_LOGEA("Timeout expired on IDLE->LOADED state change");
            goto EXIT;
        }

        mComponentState = OMX_StateLoaded;
    }

    ///Register for Preview port ENABLE event
    //
    {
        CAMERA_SCOPEDTRACE("switchToLoaded-enablepreviewport");

        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
        OMX_EventCmdComplete,
        OMX_CommandPortEnable,
        mCameraAdapterParameters.mPrevPortIndex,
        mSwitchToLoadedSem);

        if ( NO_ERROR != ret )
        {
            CAMHAL_LOGEB("Error in registering for event %d", ret);
            goto EXIT;
        }

        ///Enable Preview Port
        eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                    OMX_CommandPortEnable,
                                    mCameraAdapterParameters.mPrevPortIndex,
                                    NULL);


        CAMHAL_LOGDB("OMX_SendCommand(OMX_CommandStateSet) 0x%x", eError);

        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        CAMHAL_LOGDA("Enabling Preview port");
        ///Wait for state to switch to idle
        ret = mSwitchToLoadedSem.WaitTimeout(OMX_CMD_TIMEOUT);

        //If somethiing bad happened while we wait
        if (mComponentState == 0/*OMX_StateInvalid*/)
        {
            CAMHAL_LOGEA("Invalid State after Enabling Preview port Exitting!!!");
            goto EXIT;
        }

        if ( NO_ERROR == ret )
        {
            CAMHAL_LOGDA("Preview port enabled!");
        }
        else
        {
            ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                                OMX_EventCmdComplete,
                                OMX_CommandPortEnable,
                                mCameraAdapterParameters.mPrevPortIndex,
                                NULL);
            CAMHAL_LOGEA("Preview enable timedout");

            goto EXIT;
        }
    }

    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXCameraAdapter::UseBuffersPreview(void* bufArr, int num)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    int tmpHeight, tmpWidth;

    LOG_FUNCTION_NAME;

    CAMERA_SCOPEDTRACE("UseBuffersPreview");

    if(!bufArr)
    {
        CAMHAL_LOGEA("NULL pointer passed for buffArr");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }

    OMXCameraPortParameters * mPreviewData = NULL;
    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
    //ActionsCode(author:liuyiguang, change_code)
    //uint32_t *buffers = (uint32_t*)bufArr;
    long *buffers = (long *)bufArr;

    if ( 0 != mUsePreviewSem.Count() )
    {
        CAMHAL_LOGEB("Error mUsePreviewSem semaphore count %d", mUsePreviewSem.Count());
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
    }

    if(mPreviewData->mNumBufs != num)
    {
        CAMHAL_LOGEA("Current number of buffers doesnt equal new num of buffers passed!");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }

    mStateSwitchLock.lock();



    if ( mComponentState == OMX_StateLoaded )
    {
        ///Register for IDLE state switch event
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandStateSet,
                               OMX_StateIdle,
                               mUsePreviewSem);

        if(ret!=NO_ERROR)
        {
            CAMHAL_LOGEB("Error in registering for event %d", ret);
            goto EXIT;
        }

        ///Once we get the buffers, move component state to idle state and pass the buffers to OMX comp using UseBuffer
        eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp ,
                                  OMX_CommandStateSet,
                                  OMX_StateIdle,
                                  NULL);

        CAMHAL_LOGDB("OMX_SendCommand(OMX_CommandStateSet) 0x%x", eError);

        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        mComponentState = OMX_StateIdle;
    }
    else
    {
        ///Register for Preview port ENABLE event
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandPortEnable,
                               mCameraAdapterParameters.mPrevPortIndex,
                               mUsePreviewSem);

        if ( NO_ERROR != ret )
        {
            CAMHAL_LOGEB("Error in registering for event %d", ret);
            goto EXIT;
        }

        ///Enable Preview Port
        eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                 OMX_CommandPortEnable,
                                 mCameraAdapterParameters.mPrevPortIndex,
                                 NULL);
    }


    ///Configure DOMX to use either gralloc handles or vptrs
    StoreMetaDataInBuffersParams storeMetaDataParam;
    OMX_INIT_STRUCT_PTR (&storeMetaDataParam, StoreMetaDataInBuffersParams);

    storeMetaDataParam.nPortIndex = mCameraAdapterParameters.mPrevPortIndex;
    storeMetaDataParam.bStoreMetaData = OMX_TRUE;

    eError = OMX_SetParameter(mCameraAdapterParameters.mHandleComp,
                              (OMX_INDEXTYPE)OMX_IndexParameterStoreMediaData, &storeMetaDataParam);
    if(eError!=OMX_ErrorNone)
    {
        CAMHAL_LOGEB("OMX_SetParameter - %x", eError);
    }
    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    clearMetadataBufs(*mPreviewData);

    OMX_BUFFERHEADERTYPE *pBufferHdr;
    video_metadata_t *metadata;
    for(int index=0; index<num; index++)
    {
        metadata = (video_metadata_t *)malloc(sizeof(video_metadata_t));
        if(metadata == NULL)
        {
            CAMHAL_LOGDA("malloc metadata error");
            ret = NO_MEMORY;
            goto EXIT;
        }
        metadata->metadataBufferType = kMetadataBufferTypeCameraSource_act;
        metadata->handle = (void *) buffers[index];

        CAMHAL_LOGDB("OMX_UseBuffer(0x%x)", buffers[index]);
        eError = OMX_UseBuffer( mCameraAdapterParameters.mHandleComp,
                                &pBufferHdr,
                                mCameraAdapterParameters.mPrevPortIndex,
                                0,
                                sizeof(video_metadata_t),
                                //mPreviewData->mBufSize,
                                (OMX_U8*)metadata);
        if(eError!=OMX_ErrorNone)
        {
            CAMHAL_LOGEB("OMX_UseBuffer-eError=0x%x", eError);
        }
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

        CAMHAL_LOGDB("OMX_UseBuffer,pBufferHdr (0x%p)", pBufferHdr);
        CAMHAL_LOGDB("OMX_UseBuffer,pBufferHdr->pbuffer, (0x%p)", metadata);
        //pBufferHdr->pAppPrivate =  (OMX_PTR)pBufferHdr;
        pBufferHdr->nSize = sizeof(OMX_BUFFERHEADERTYPE);
        pBufferHdr->nVersion.s.nVersionMajor = SPECVERSIONMAJOR ;
        pBufferHdr->nVersion.s.nVersionMinor = SPECVERSIONMINOR ;
        pBufferHdr->nVersion.s.nRevision = SPECREVISION ;
        pBufferHdr->nVersion.s.nStep =  SPECSTEP;
        mPreviewData->mBufferHeader[index] = pBufferHdr;
        //ActionsCode(author:liuyiguang, change_code)
        //mPreviewData->mHostBufaddr[index] = (OMX_U32)buffers[index];
        //mPreviewData->mMetadataBufs[index] = (OMX_U32)metadata;
        mPreviewData->mHostBufaddr[index] = buffers[index];
        mPreviewData->mMetadataBufs[index] = (long)metadata;
    }

    mPreviewData->mMetadataNum = num;

    CAMHAL_LOGDA("Registering preview buffers");

    ret = mUsePreviewSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == 0/*OMX_StateInvalid*/)
    {
        CAMHAL_LOGEA("Invalid State after Registering preview buffers Exitting!!!");
        goto EXIT;
    }

    if ( NO_ERROR == ret )
    {
        CAMHAL_LOGDA("Preview buffer registration successfull");
    }
    else
    {
        if ( mComponentState == OMX_StateLoaded )
        {
            ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandStateSet,
                               OMX_StateIdle,
                               NULL);
        }
        else
        {
            ret |= SignalEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandPortEnable,
                               mCameraAdapterParameters.mPrevPortIndex,
                               NULL);
        }
        CAMHAL_LOGEA("Timeout expired on preview buffer registration");
        goto EXIT;
    }

    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));

    ///If there is any failure, we reach here.
    ///Here, we do any resource freeing and convert from OMX error code to Camera Hal error code
EXIT:
    mStateSwitchLock.unlock();

    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);

    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));
}

status_t OMXCameraAdapter::startPreview()
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters *mPreviewData = NULL;
    OMXCameraPortParameters *measurementData = NULL;
    //OMX_CONFIG_EXTRADATATYPE extraDataControl;

    LOG_FUNCTION_NAME;

    CAMERA_SCOPEDTRACE("omxstartPreview");

    startGPUOutstanding();
#ifdef CAMERA_SET_FREQ_RANGE
    cameraSetCpuFreqRange();
#endif

    if( 0 != mStartPreviewSem.Count() )
    {
        CAMHAL_LOGEB("Error mStartPreviewSem semaphore count %d", mStartPreviewSem.Count());
        ret = NO_INIT;
        goto EXIT;
    }



    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

#ifdef CAMERA_WATCHDOG
    //start watchdog
    if(mWatchDog != NULL)
    {
        mWatchDog->startWatchDog(this, watchdog_timout);
    }
#endif

    if( OMX_StateIdle == mComponentState )
    {
        ///Register for EXECUTING state transition.
        ///This method just inserts a message in Event Q, which is checked in the callback
        ///The sempahore passed is signalled by the callback
        ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandStateSet,
                               OMX_StateExecuting,
                               mStartPreviewSem);

        if(ret!=NO_ERROR)
        {
            CAMHAL_LOGEB("Error in registering for event %d", ret);
            goto EXIT;
        }

        ///Switch to EXECUTING state
        eError = OMX_SendCommand(mCameraAdapterParameters.mHandleComp,
                                 OMX_CommandStateSet,
                                 OMX_StateExecuting,
                                 NULL);

        if(eError!=OMX_ErrorNone)
        {
            CAMHAL_LOGEB("OMX_SendCommand(OMX_StateExecuting)-0x%x", eError);
        }

        CAMHAL_LOGDA("+Waiting for component to go into EXECUTING state");
        ret = mStartPreviewSem.WaitTimeout(OMX_CMD_TIMEOUT);

        //If somethiing bad happened while we wait
        if (mComponentState == 0/*OMX_StateInvalid*/)
        {
            CAMHAL_LOGEA("Invalid State after IDLE_EXECUTING Exitting!!!");
            goto EXIT;
        }

        if ( NO_ERROR == ret )
        {
            CAMHAL_LOGDA("+Great. Component went into executing state!!");
        }
        else
        {
            ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                               OMX_EventCmdComplete,
                               OMX_CommandStateSet,
                               OMX_StateExecuting,
                               NULL);
            CAMHAL_LOGDA("Timeout expired on executing state switch!");
            goto EXIT;
        }

        mComponentState = OMX_StateExecuting;

    }

    mStateSwitchLock.unlock();

    //reset frame rate estimates
    mFPS = 0.0f;
    mLastFPS = 0.0f;
   
    mFrameCount = 0;
    mLastFrameCount = 0;
    mIter = 1;
    mLastFPSTime = systemTime();

    apply3Asettings(mParameters3A);//------

    {
        Mutex::Autolock lock(mPreviewData->mLock);                
        mPreviewData->mPortEnable = true;
    }

    mFramesWithOMX = 0;
#ifdef DEBUG_LOG
    mBuffersWithOMX.clear();
#endif
    //Queue all the buffers on preview port
    for(int index=0; index< mPreviewData->mMaxQueueable; index++)
    {
        CAMHAL_LOGDB("Queuing buffer on Preview port,header=0x%x, pBuffer= 0x%x", (uint32_t)mPreviewData->mBufferHeader[index], (uint32_t)((video_metadata_t *)(mPreviewData->mBufferHeader[index]->pBuffer))->handle);
        eError = OMX_FillThisBuffer(mCameraAdapterParameters.mHandleComp,
                                    (OMX_BUFFERHEADERTYPE*)mPreviewData->mBufferHeader[index]);
        if(eError!=OMX_ErrorNone)
        {
            CAMHAL_LOGEB("OMX_FillThisBuffer-0x%x", eError);
        }
        mFramesWithOMX++;
#ifdef DEBUG_LOG
        CAMHAL_LOGDB("OMX_FillThisBuffer init-0x%x", (uint32_t)((video_metadata_t *)(mPreviewData->mBufferHeader[index]->pBuffer))->handle);
        mBuffersWithOMX.add((uint32_t)((video_metadata_t *)(mPreviewData->mBufferHeader[index]->pBuffer))->handle,1);
#endif
        GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);
    }


    if ( mPending3Asettings )
        apply3Asettings(mParameters3A);


    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:

    stopGPUOutstanding();
#ifdef CAMERA_SET_FREQ_RANGE
    CameraRestoreCpuFreqRange();
#endif

#ifdef CAMERA_WATCHDOG
    //stop watchdog
    if(mWatchDog != NULL)
    {
        mWatchDog->stopWatchDog();
    }
#endif

    CAMHAL_LOGDB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, ret, eError);
    performCleanupAfterError();
    mStateSwitchLock.unlock();


    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));

}

status_t OMXCameraAdapter::stopPreview()
{
    LOG_FUNCTION_NAME;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    status_t ret = NO_ERROR;

    CAMERA_SCOPEDTRACE("omxstopPreview");

    OMXCameraPortParameters *mCaptureData , *mPreviewData, *measurementData;
    mCaptureData = mPreviewData = measurementData = NULL;

    mPreviewData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
    mCaptureData = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex];

    if (mFlashStrobed) {
        mFlashStrobed = false;
        stopFlashStrobe();
    }

    if (mAdapterState == LOADED_PREVIEW_STATE)
    {
        // Something happened in CameraHal between UseBuffers and startPreview
        // this means that state switch is still locked..so we need to unlock else
        // deadlock will occur on the next start preview
        mStateSwitchLock.unlock();
        return NO_ERROR;
    }

    if ( mComponentState != OMX_StateExecuting )
    {
        CAMHAL_LOGEA("Calling StopPreview() when not in EXECUTING state");
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
    }

   
    mFrameCount = 0;

#ifdef CAMERA_VCE_OMX_FD
    if(mCapabilities->mFaceDetectEnable)
    {
        stopFaceDetection();
    }
#endif

    ret = cancelAutoFocus();
    if(ret!=NO_ERROR)
    {
        CAMHAL_LOGEB("Error canceling autofocus %d", ret);
        // Error, but we probably still want to continue to stop preview
    }

    CAMHAL_LOGDB("Average framerate: %f", mFPS);
#if 0
    {
    CAMERA_SCOPEDTRACE("switchToLoaded-idle");
    ret = RegisterForEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandStateSet,
                           OMX_StateIdle,
                           mSwitchToLoadedSem);

    if(ret!=NO_ERROR)
    {
        CAMHAL_LOGEB("Error in registering for event %d", ret);
        goto EXIT;
    }

    eError = OMX_SendCommand (mCameraAdapterParameters.mHandleComp,
                              OMX_CommandStateSet,
                              OMX_StateIdle,
                              NULL);

    if(eError!=OMX_ErrorNone)
    {
        CAMHAL_LOGEB("OMX_SendCommand(OMX_StateIdle) - %x", eError);
    }

    GOTO_EXIT_IF((eError!=OMX_ErrorNone), eError);

    ///Wait for the EXECUTING ->IDLE transition to arrive

    CAMHAL_LOGDA("EXECUTING->IDLE state changed");
    ret = mSwitchToLoadedSem.WaitTimeout(OMX_CMD_TIMEOUT);

    //If somethiing bad happened while we wait
    if (mComponentState == 0/*OMX_StateInvalid*/)
    {
        CAMHAL_LOGEA("Invalid State after EXECUTING->IDLE Exitting!!!");
        goto EXIT;
    }

    if ( NO_ERROR == ret )
    {
        CAMHAL_LOGDA("EXECUTING->IDLE state changed");
    }
    else
    {
        ret |= RemoveEvent(mCameraAdapterParameters.mHandleComp,
                           OMX_EventCmdComplete,
                           OMX_CommandStateSet,
                           OMX_StateIdle,
                           NULL);
        CAMHAL_LOGEA("Timeout expired on EXECUTING->IDLE state change");
        goto EXIT;
    }
    mComponentState = OMX_StateIdle;
    
    }
#endif
    

    //Avoid state switching of the OMX Component
    //{
    //CAMERA_SCOPEDTRACE("flushBuffers");
    //ret = flushBuffers();
    //}
    //if ( NO_ERROR != ret )
    //{
    //    CAMHAL_LOGEB("Flush Buffers failed 0x%x", ret);
    //    goto EXIT;
    //}

    if ( 0 != mStopPreviewSem.Count() )
    {
        CAMHAL_LOGEB("Error mStopPreviewSem semaphore count %d", mStopPreviewSem.Count());
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
    }
    
#if 0
    ret = disableImagePort();
    if ( NO_ERROR != ret )
    {
        CAMHAL_LOGEB("disable image port failed 0x%x", ret);
        goto EXIT;
    }
#endif

    ret = switchToLoaded();
    if (ret) {
        CAMHAL_LOGEB("camera component swith to loaded error %x", ret);
    }


    mFirstTimeInit = true;
    mPendingCaptureSettings = 0;
    mFramesWithOMX = 0;
    mFramesWithDisplay = 0;
    mFramesWithEncoder = 0;

#ifdef CAMERA_FRAME_STAT
    gbCaptureStarted = false;
#endif
    stopGPUOutstanding();

#ifdef CAMERA_SET_FREQ_RANGE
    CameraRestoreCpuFreqRange();
#endif

    //should after port disable, otherwise onEmptyBufferDone will reference freed buffer
    clearMetadataBufs(*mPreviewData);
#ifdef CAMERA_WATCHDOG
    //stop watchdog
    if(mWatchDog != NULL)
    {
        mWatchDog->stopWatchDog();
    }
#endif

    LOG_FUNCTION_NAME_EXIT;

    return (ret | ErrorUtils::omxToAndroidError(eError));

EXIT:
    stopGPUOutstanding();

#ifdef CAMERA_SET_FREQ_RANGE
    CameraRestoreCpuFreqRange();
#endif

#ifdef CAMERA_WATCHDOG
    //stop watchdog
    if(mWatchDog != NULL)
    {
        mWatchDog->stopWatchDog();
    }
#endif

    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__,ret, eError);
    performCleanupAfterError();

    //should after port disable, otherwise onEmptyBufferDone will reference freed buffer
    clearMetadataBufs(*mPreviewData);

    LOG_FUNCTION_NAME_EXIT;
    return (ret | ErrorUtils::omxToAndroidError(eError));

}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXCameraAdapter::printComponentVersion(OMX_HANDLETYPE handle)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_VERSIONTYPE compVersion;
    char compName[OMX_MAX_STRINGNAME_SIZE];
    char *currentUUID = NULL;
    size_t offset = 0;

    LOG_FUNCTION_NAME;

    if ( NULL == handle )
    {
        //ActionsCode(author:liuyiguang, change_code)
        CAMHAL_LOGEB("Invalid OMX Handle =0x%x",  ( long ) handle);
        ret = -EINVAL;
    }

    mCompUUID[0] = 0;

    if ( NO_ERROR == ret )
    {
        eError = OMX_GetComponentVersion(handle,
                                         compName,
                                         &compVersion,
                                         &mCompRevision,
                                         &mCompUUID
                                        );
        if ( OMX_ErrorNone != eError )
        {
            CAMHAL_LOGEB("OMX_GetComponentVersion returned 0x%x", eError);
            ret = BAD_VALUE;
        }
    }

    if ( NO_ERROR == ret )
    {
        CAMHAL_LOGVB("OMX Component name: [%s]", compName);
        CAMHAL_LOGVB("OMX Component version: [%u]", ( unsigned int ) compVersion.nVersion);
        CAMHAL_LOGVB("Spec version: [%u]", ( unsigned int ) mCompRevision.nVersion);
        CAMHAL_LOGVB("Git Commit ID: [%s]", mCompUUID);
        currentUUID = ( char * ) mCompUUID;
    }

    if ( NULL != currentUUID )
    {
        offset = strlen( ( const char * ) mCompUUID) + 1;
        //ActionsCode(author:liuyiguang, change_code)
        //if ( (int)currentUUID + (int)offset - (int)mCompUUID < OMX_MAX_STRINGNAME_SIZE )
        if ( (long)currentUUID + (long)offset - (long)mCompUUID < OMX_MAX_STRINGNAME_SIZE )
        {
            currentUUID += offset;
            CAMHAL_LOGVB("Git Branch: [%s]", currentUUID);
        }
        else
        {
            ret = BAD_VALUE;
        }
    }

    if ( NO_ERROR == ret )
    {
        offset = strlen( ( const char * ) currentUUID) + 1;

        //ActionsCode(author:liuyiguang, change_code)
        //if ( (int)currentUUID + (int)offset - (int)mCompUUID < OMX_MAX_STRINGNAME_SIZE )
        if ( (long)currentUUID + (long)offset - (long)mCompUUID < OMX_MAX_STRINGNAME_SIZE )
        {
            currentUUID += offset;
            CAMHAL_LOGVB("Build date and time: [%s]", currentUUID);
        }
        else
        {
            ret = BAD_VALUE;
        }
    }

    if ( NO_ERROR == ret )
    {
        offset = strlen( ( const char * ) currentUUID) + 1;

        //ActionsCode(author:liuyiguang, change_code)
        //if ( (int)currentUUID + (int)offset - (int)mCompUUID < OMX_MAX_STRINGNAME_SIZE )
        if ( (long)currentUUID + (long)offset - (long)mCompUUID < OMX_MAX_STRINGNAME_SIZE )
        {
            currentUUID += offset;
            CAMHAL_LOGVB("Build description: [%s]", currentUUID);
        }
        else
        {
            ret = BAD_VALUE;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::autoFocus()
{
    status_t ret = NO_ERROR;
    ActUtils::Message msg;

    LOG_FUNCTION_NAME;

    msg.command = AFHandler::CAMERA_PERFORM_AUTOFOCUS;
    msg.arg1 = mErrorNotifier;
    ret = mAFHandler->put(&msg);

EXIT:

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::takePicture()
{
    status_t ret = NO_ERROR;
    ActUtils::Message msg;

    LOG_FUNCTION_NAME;

    msg.command = CommandHandler::CAMERA_START_IMAGE_CAPTURE;
    msg.arg1 = mErrorNotifier;
    CAMHAL_LOGDA("before mCommandHandler->put");
    ret = mCommandHandler->put(&msg);
    CAMHAL_LOGDA("after mCommandHandler->put");

EXIT:
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}
status_t OMXCameraAdapter::endImageCallback()
{
    status_t ret = NO_ERROR;
    ActUtils::Message msg;

    LOG_FUNCTION_NAME;
#if 1
    msg.command = CommandHandler::CAMERA_END_IMAGE_CALLBACK;
    msg.arg1 = mErrorNotifier;
    CAMHAL_LOGDA("before mCommandHandler->put");
    ret = mCommandHandler->put(&msg);
    CAMHAL_LOGDA("after mCommandHandler->put");
#else
    mEndImageCaptureCallback(mEndCaptureData);
#endif

EXIT:
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::startVideoCapture()
{
#ifdef CAMERA_VCE_OMX_FD
    if(mCapabilities->mFaceDetectEnable)
    {
        pauseFaceDetection(true);
    }
#endif

    
    return BaseCameraAdapter::startVideoCapture();
}

status_t OMXCameraAdapter::stopVideoCapture()
{
#ifdef CAMERA_VCE_OMX_FD
    if(mCapabilities->mFaceDetectEnable)
    {
        pauseFaceDetection(false);
    }
#endif
    return BaseCameraAdapter::stopVideoCapture();
}

status_t OMXCameraAdapter::getFrameSize(size_t &width, size_t &height, int num)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMXCameraPortParameters *prevPortParam;
    LOG_FUNCTION_NAME;


    if ( mOMXStateSwitch )
    {
        ret = switchToLoaded();
        if ( NO_ERROR != ret )
        {
            CAMHAL_LOGEB("switchToLoaded() failed 0x%x", ret);
            goto exit;
        }

        mOMXStateSwitch = false;
    }
    prevPortParam = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];
    prevPortParam->mNumBufs = num ;
    ret = setFormat (mCameraAdapterParameters.mPrevPortIndex,
                     mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex]);
    if ( NO_ERROR != ret )
    {
        CAMHAL_LOGEB("setFormat() failed %d", ret);
        return ret;
    }

    width = prevPortParam->mWidth;
    height = prevPortParam->mHeight;

exit:

    CAMHAL_LOGDB("Required frame size %dx%d", width, height);
    LOG_FUNCTION_NAME_EXIT;

    return ret;

    return 0;
}



void OMXCameraAdapter::onOrientationEvent(uint32_t orientation, uint32_t tilt)
{
    LOG_FUNCTION_NAME;
    static const unsigned int DEGREES_TILT_IGNORE = 45;
    int device_orientation = 0;
    int mount_orientation = 0;
    const char *facing_direction = NULL;

    // if tilt angle is greater than DEGREES_TILT_IGNORE
    // we are going to ignore the orientation returned from
    // sensor. the orientation returned from sensor is not
    // reliable. Value of DEGREES_TILT_IGNORE may need adjusting
    if (tilt > DEGREES_TILT_IGNORE)
    {
        return;
    }

    if (mCapabilities)
    {
        if (mCapabilities->get(CameraProperties::ORIENTATION_INDEX))
        {
            mount_orientation = atoi(mCapabilities->get(CameraProperties::ORIENTATION_INDEX));
        }
        facing_direction = mCapabilities->get(CameraProperties::FACING_INDEX);
    }

    // calculate device orientation relative to the sensor orientation
    // front camera display is mirrored...needs to be accounted for when orientation
    // is 90 or 270...since this will result in a flip on orientation otherwise
    if (facing_direction && !strcmp(facing_direction, ActCameraParameters::FACING_FRONT) &&
            (orientation == 90 || orientation == 270))
    {
        device_orientation = (orientation - mount_orientation + 360) % 360;
    }
    else      // back-facing camera
    {
        device_orientation = (orientation + mount_orientation) % 360;
    }

    if (device_orientation != mDeviceOrientation)
    {
        setFaceDetectionOrientation(device_orientation);
        mDeviceOrientation = device_orientation;

    }
    CAMHAL_LOGVB("orientation = %d tilt = %d device_orientation = %d, mount_orientation=%d", orientation, tilt, mDeviceOrientation,mount_orientation);
    LOG_FUNCTION_NAME_EXIT;
}

/* Application callback Functions */
/*========================================================*/
/* @ fn SampleTest_EventHandler :: Application callback   */
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapterEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    LOG_FUNCTION_NAME;

    CAMHAL_LOGDB("Event %d", eEvent);

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    OMXCameraAdapter *oca = (OMXCameraAdapter*)pAppData;
    ret = oca->OMXCameraAdapterEventHandler(hComponent, eEvent, nData1, nData2, pEventData);

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

/* Application callback Functions */
/*========================================================*/
/* @ fn SampleTest_EventHandler :: Application callback   */
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapter::OMXCameraAdapterEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{

    LOG_FUNCTION_NAME;

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    CAMHAL_LOGDB("+OMX_Event %x, %d %d", eEvent, (int)nData1, (int)nData2);

    switch (eEvent)
    {
    case OMX_EventCmdComplete:
        CAMHAL_LOGDB("+OMX_EventCmdComplete %d %d", (int)nData1, (int)nData2);

        if (OMX_CommandStateSet == nData1)
        {
            mCameraAdapterParameters.mState = (OMX_STATETYPE) nData2;

        }
        else if (OMX_CommandFlush == nData1)
        {
            CAMHAL_LOGDB("OMX_CommandFlush received for port %d", (int)nData2);

        }
        else if (OMX_CommandPortDisable == nData1)
        {
            CAMHAL_LOGDB("OMX_CommandPortDisable received for port %d", (int)nData2);

        }
        else if (OMX_CommandPortEnable == nData1)
        {
            CAMHAL_LOGDB("OMX_CommandPortEnable received for port %d", (int)nData2);

        }
        else if (OMX_CommandMarkBuffer == nData1)
        {
            ///This is not used currently
        }

        CAMHAL_LOGDA("-OMX_EventCmdComplete");
        break;
#if 0
    case OMX_EventIndexSettingChanged:
        CAMHAL_LOGDB("OMX_EventIndexSettingChanged event received data1 0x%x, data2 0x%x",
                     ( unsigned int ) nData1, ( unsigned int ) nData2);
        break;
#endif

    case OMX_EventError:
        CAMHAL_LOGDB("OMX interface failed to execute OMX command %d", (int)nData1);
        CAMHAL_LOGDA("See OMX_INDEXTYPE for reference");
        if ( NULL != mErrorNotifier && ( ( OMX_U32 ) OMX_ErrorHardware == nData1 ) && mComponentState != 0/*OMX_StateInvalid*/)
        {
            CAMHAL_LOGEA("***Got Fatal Error Notification***\n");
            mComponentState = (OMX_STATETYPE)0/*OMX_StateInvalid*/;
            /*
            Remove any unhandled events and
            unblock any waiting semaphores
            */
            if ( !mEventSignalQ.isEmpty() )
            {
                for (unsigned int i = 0 ; i < mEventSignalQ.size(); i++ )
                {
                    CAMHAL_LOGEB("***Removing %d EVENTS***** \n", mEventSignalQ.size());
                    //remove from queue and free msg
                    ActUtils::Message *msg = mEventSignalQ.itemAt(i);
                    if ( NULL != msg )
                    {
                        Semaphore *sem  = (Semaphore*) msg->arg3;
                        if ( sem )
                        {
                            sem->Signal();
                        }
                        free(msg);
                    }
                }
                mEventSignalQ.clear();
            }
            ///Report Error to App
            mErrorNotifier->errorNotify(CAMERA_ERROR_FATAL);
        }
        break;

    case OMX_EventMark:
        break;

    case OMX_EventPortSettingsChanged:
        break;

    case OMX_EventBufferFlag:
        break;

    case OMX_EventResourcesAcquired:
        break;

    case OMX_EventComponentResumed:
        break;

    case OMX_EventDynamicResourcesAvailable:
        break;

    case OMX_EventPortFormatDetected:
        break;

    default:
        break;
    }

    ///Signal to the thread(s) waiting that the event has occured
    SignalEvent(hComponent, eEvent, nData1, nData2, pEventData);

    LOG_FUNCTION_NAME_EXIT;
    return eError;

EXIT:

    CAMHAL_LOGEB("Exiting function %s because of eError=%x", __FUNCTION__, eError);
    LOG_FUNCTION_NAME_EXIT;
    return eError;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
OMX_ERRORTYPE OMXCameraAdapter::SignalEvent(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    Mutex::Autolock lock(mEventLock);
    ActUtils::Message *msg;

    LOG_FUNCTION_NAME;

    if ( !mEventSignalQ.isEmpty() )
    {
        CAMHAL_LOGDA("Event queue not empty");

        for ( unsigned int i = 0 ; i < mEventSignalQ.size() ; i++ )
        {
            msg = mEventSignalQ.itemAt(i);
            if ( NULL != msg )
            {
                //ActionsCode(author:liuyiguang, change_code)
                //if( ( msg->command != 0 || msg->command == ( unsigned int ) ( eEvent ) )
                //        && ( !msg->arg1 || ( OMX_U32 ) msg->arg1 == nData1 )
                //        && ( !msg->arg2 || ( OMX_U32 ) msg->arg2 == nData2 )
                //        && msg->arg3)
                if( ( msg->command != 0 || msg->command == ( unsigned int ) ( eEvent ) )
                        && ( !msg->arg1 || ( long ) msg->arg1 == nData1 )
                        && ( !msg->arg2 || ( long ) msg->arg2 == nData2 )
                        && msg->arg3)
                {
                    Semaphore *sem  = (Semaphore*) msg->arg3;
                    CAMHAL_LOGDA("Event matched, signalling sem");
                    mEventSignalQ.removeAt(i);
                    //Signal the semaphore provided
                    sem->Signal();
                    free(msg);
                    break;
                }
            }
        }
    }
    else
    {
        CAMHAL_LOGDA("Event queue empty!!!");
    }

    LOG_FUNCTION_NAME_EXIT;

    return OMX_ErrorNone;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
OMX_ERRORTYPE OMXCameraAdapter::RemoveEvent(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    Mutex::Autolock lock(mEventLock);
    ActUtils::Message *msg;
    LOG_FUNCTION_NAME;

    if ( !mEventSignalQ.isEmpty() )
    {
        CAMHAL_LOGDA("Event queue not empty");

        for ( unsigned int i = 0 ; i < mEventSignalQ.size() ; i++ )
        {
            msg = mEventSignalQ.itemAt(i);
            if ( NULL != msg )
            {
                //ActionsCode(author:liuyiguang, change_code)
                //if( ( msg->command != 0 || msg->command == ( unsigned int ) ( eEvent ) )
                //        && ( !msg->arg1 || ( OMX_U32 ) msg->arg1 == nData1 )
                //        && ( !msg->arg2 || ( OMX_U32 ) msg->arg2 == nData2 )
                //        && msg->arg3)
                if( ( msg->command != 0 || msg->command == ( unsigned int ) ( eEvent ) )
                        && ( !msg->arg1 || ( long ) msg->arg1 == nData1 )
                        && ( !msg->arg2 || ( long ) msg->arg2 == nData2 )
                        && msg->arg3)
                {
                    Semaphore *sem  = (Semaphore*) msg->arg3;
                    CAMHAL_LOGDA("Event matched, signalling sem");
                    mEventSignalQ.removeAt(i);
                    free(msg);
                    break;
                }
            }
        }
    }
    else
    {
        CAMHAL_LOGEA("Event queue empty!!!");
    }
    LOG_FUNCTION_NAME_EXIT;

    return OMX_ErrorNone;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t OMXCameraAdapter::RegisterForEvent(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN Semaphore &semaphore)
{
    status_t ret = NO_ERROR;
    ssize_t res;
    Mutex::Autolock lock(mEventLock);

    LOG_FUNCTION_NAME;
    ActUtils::Message * msg = ( struct ActUtils::Message * ) malloc(sizeof(struct ActUtils::Message));
    if ( NULL != msg )
    {
        msg->command = ( unsigned int ) eEvent;
        //ActionsCode(author:liuyiguang, change_code)
        //msg->arg1 = ( void * ) nData1;
        //msg->arg2 = ( void * ) nData2;
        msg->arg1 = ( void * )(long) nData1;
        msg->arg2 = ( void * )(long) nData2;
        msg->arg3 = ( void * ) &semaphore;
        msg->arg4 =  ( void * ) hComponent;
        res = mEventSignalQ.add(msg);
        if ( NO_MEMORY == res )
        {
            CAMHAL_LOGEA("No ressources for inserting OMX events");
            free(msg);
            ret = -ENOMEM;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/*========================================================*/
/* @ fn SampleTest_EmptyBufferDone :: Application callback*/
/*========================================================*/
/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
OMX_ERRORTYPE OMXCameraAdapterEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader)
{
    LOG_FUNCTION_NAME;

    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if ( !pBuffHeader || !pBuffHeader->pBuffer )
    {
        CAMHAL_LOGDA("NULL Buffer from OMX");
        return OMX_ErrorNone;
    }     
    OMXCameraAdapter *oca = (OMXCameraAdapter*)pAppData;
    //ActionsCode(author:liuyiguang, change_code)
    //eError = oca->OMXCameraAdapterEmptyBufferDone(hComponent, pBuffHeader, (void *) pBuffHeader->nOutputPortIndex);
    eError = oca->OMXCameraAdapterEmptyBufferDone(hComponent, pBuffHeader, (void *)(long) pBuffHeader->nOutputPortIndex);

    LOG_FUNCTION_NAME_EXIT;
    return eError;
}


/*========================================================*/
/* @ fn SampleTest_EmptyBufferDone :: Application callback*/
/*========================================================*/
OMX_ERRORTYPE OMXCameraAdapter::OMXCameraAdapterEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader, void *handle)
{

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return OMX_ErrorNone;
}


/*========================================================*/
/* @ fn SampleTest_FillBufferDone ::  Application callback*/
/*========================================================*/
/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
OMX_ERRORTYPE OMXCameraAdapterFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader)
{
    ActUtils::Message msg;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    if ( !pBuffHeader || !pBuffHeader->pBuffer )
    {
        CAMHAL_LOGDA("NULL Buffer from OMX");
        return OMX_ErrorNone;
    }     

    OMXCameraAdapter *adapter =  ( OMXCameraAdapter * ) pAppData;
    if ( NULL != adapter )
    {
        msg.command = OMXCameraAdapter::OMXCallbackHandler::CAMERA_FILL_BUFFER_DONE;
        msg.arg1 = ( void * ) hComponent;
        msg.arg2 = ( void * ) pBuffHeader;
#ifdef CAMERA_FRAME_STAT
        if(!gbCaptureStarted)
        {
            gbCaptureStarted = true;

            glastCaptureTime = cameraGetMs();
        }

        filledBufferExtraInfo *extraInfo = new filledBufferExtraInfo();
        extraInfo->portIndex = pBuffHeader->nOutputPortIndex;
        extraInfo->captureTime = cameraGetMs();
        extraInfo->captureIntervalTime = extraInfo->captureTime -glastCaptureTime;
        glastCaptureTime = extraInfo->captureTime;
        msg.arg3 = ( void * )extraInfo;
#else
        //ActionsCode(author:liuyiguang, change_code)
        //msg.arg3 = ( void * )pBuffHeader->nOutputPortIndex;
        msg.arg3 = ( void * )(long)pBuffHeader->nOutputPortIndex;
#endif
        adapter->mOMXCallbackHandler->put(&msg);
    }

    return eError;
}

/*========================================================*/
/* @ fn SampleTest_FillBufferDone ::  Application callback*/
/*========================================================*/
/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 * BUGFIX:    Bug00260689.Change the taking picture size while recording,as the same size to preview.
 * BUGFIX:    Bug00252580.Relax the solution of encode.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
OMX_ERRORTYPE OMXCameraAdapter::OMXCameraAdapterFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader, void * extra)
{

    status_t  stat = NO_ERROR;
    status_t  res1, res2;
    OMXCameraPortParameters  *pPortParam;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    unsigned int refCount = 0;
    BaseCameraAdapter::AdapterState state, nextState;
    BaseCameraAdapter::getState(state);
    BaseCameraAdapter::getNextState(nextState);
    unsigned int mask = 0xFFFF;
    CameraFrame cameraFrame;

    bool snapshotFrame = false;
    int exifType;

    void *bufferHandle = NULL;
    bool needSendCaptureImage = false;
    int nextBufferIndex;
    OMXCameraPortParameters *imgCaptureParam = &(mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex]);

#ifdef CAMERA_FRAME_STAT
    filledBufferExtraInfo *extraInfo = (filledBufferExtraInfo*)extra;
    OMX_U32 portIndex = extraInfo->portIndex;
    unsigned long long captureTime = extraInfo->captureTime;
    int captureIntervalTime = extraInfo->captureIntervalTime;
    int captureScheTime = cameraGetMs() - extraInfo->captureTime;
    delete extraInfo;

#else
    //ActionsCode(author:liuyiguang, change_code)
    //OMX_U32 portIndex = (OMX_U32)extra;
    OMX_U32 portIndex = (long)extra;
#endif

#ifdef CAMERA_WATCHDOG
    if(mWatchDog != NULL)
    {
        mWatchDog->tickle();
    }
#endif    

    res1 = res2 = NO_ERROR;
    if(portIndex != OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW && portIndex != OMX_CAMERA_PORT_IMAGE_OUT_IMAGE)
    {
        CAMHAL_LOGEB("error portIndex from OMX,%d", (unsigned int)portIndex);
        return OMX_ErrorNone;
    }
    pPortParam = &(mCameraAdapterParameters.mCameraPortParams[portIndex]);

    {
        Mutex::Autolock lock(pPortParam->mLock);                        
        if(!pPortParam->mPortEnable)
        {
            CAMHAL_LOGDA("Buffer from disabled OMX port");
            return OMX_ErrorNone;
        }
        if ( !pBuffHeader || !pBuffHeader->pBuffer )
        {
            CAMHAL_LOGDA("NULL Buffer from OMX");
            return OMX_ErrorNone;
        }     
        if(!(pBuffHeader->nFlags&OMX_BUFFERFLAG_ENDOFFRAME))
        {
            CAMHAL_LOGDA("NULL Buffer from OMX");
            return OMX_ErrorNone;
        }
        bufferHandle = ( void * ) (((video_metadata_t *)pBuffHeader->pBuffer)->handle);

        initCameraFrame(cameraFrame, pBuffHeader, bufferHandle, pPortParam);

    }

#ifdef CAMERA_FRAME_STAT
    cameraFrame.mCaptureTime = captureTime;
    cameraFrame.mCaptureIntervalTime = captureIntervalTime;
    cameraFrame.mCaptureScheTime = captureScheTime;

#endif    


#ifdef DUMP_FRAME
    char value[PROPERTY_VALUE_MAX];
    property_get("debug.dump.camframe", value, "0");
    if (atoi(value) == 1) {
        void * src;
        Rect bounds;
        GraphicBufferMapper &mapper = GraphicBufferMapper::get();
        bounds.left = 0;
        bounds.top = 0;
        bounds.right = cameraFrame.mOrigWidth;
        bounds.bottom = cameraFrame.mOrigHeight;
        mapper.lock((buffer_handle_t)bufferHandle, CAMHAL_GRALLOC_LOCK_USAGE, bounds, &src);
        FILE *fp;
        char fname[64] = "/data/camera/sensor.yuv";
        fp = fopen(fname, "ab");
        if(fp == NULL)            {
            ALOGE("open file failed");
        }
        else
        {
            fwrite(src,1, cameraFrame.mLength, fp);
            fclose(fp);
        }
        mapper.unlock((buffer_handle_t)bufferHandle);
    }
#endif
    
    if (UNLIKELY(mDebugFps))
    {
        CAMHAL_LOGDB("Camera %d Frames, %f FPS", mFrameCount, mFPS);
    } 


    CAMHAL_LOGDB("OMXCameraAdapterFillBufferDone, port= %d", (int)portIndex);

    startTransaction(bufferHandle);

    if (portIndex == OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW)
    {

        CAMERA_SCOPEDTRACE("OMXCameraAdapterFillBufferDone_previewport");

        if ( (( PREVIEW_ACTIVE & state ) != PREVIEW_ACTIVE) &&  (( PREVIEW_ACTIVE & nextState ) != PREVIEW_ACTIVE))
        {
            CAMHAL_LOGDB("PREVIEW_ACTIVE is not active, state= 0x%x", state);
            eError = OMX_ErrorNone;
            goto EXIT;
        }


        recalculateFPS();

        if ( (nextState & CAPTURE_ACTIVE) )
        {
            mPending3Asettings |= SetFocus;
        }

        cameraFrame.mStreamType = CameraFrame::PREVIEW_STREAM_TYPE;

        mask = (unsigned int)CameraFrame::PREVIEW_FRAME_SYNC;
        mask |= (unsigned int)CameraFrame::PREVIEW_RAW_FRAME_SYNC;

        if (mRecording)
        {
            mask |= (unsigned int)CameraFrame::VIDEO_FRAME_SYNC;
            mFramesWithEncoder++;
        }
#ifdef CAMERA_VCE_OMX_FD
        if(mCapabilities->mFaceDetectEnable)
        {
            if ( mFaceDetectionRunning && !mFaceDetectionPaused )
            {
                mask |= (unsigned int)CameraFrame::FD_FRAME_SYNC;
            }
        }
#endif

        if( ((cameraFrame.mEncodeWidth-1)/cameraFrame.mOrigWidth>=MAX_CAMERA_SCALE) 
            || ((cameraFrame.mEncodeHeight-1)/cameraFrame.mOrigHeight>=MAX_CAMERA_SCALE))
        {
            cameraFrame.mQuirks |= CameraFrame::ENCODE_WITH_SW;
        }
        //vce do not support zoom out
        ////ActionsCode(author:liuyiguang, change_code, relax the solution of encode)
        if( (cameraFrame.mEncodeWidth < cameraFrame.mWidth / 2 ) 
            || (cameraFrame.mEncodeHeight < cameraFrame.mHeight / 2 ))
        {
            cameraFrame.mQuirks |= CameraFrame::ENCODE_WITH_SW;
        }

        mFramesWithDisplay++;

        mFramesWithOMX--;

#ifdef DEBUG_LOG
        if(mBuffersWithOMX.indexOfKey((int)bufferHandle)<0)
        {
            CAMHAL_LOGEB("Buffer was never with OMX!! 0x%p", bufferHandle);
            for(unsigned int i=0; i<mBuffersWithOMX.size(); i++)
            {
                CAMHAL_LOGEB("0x%x", mBuffersWithOMX.keyAt(i));
            }
        }
        mBuffersWithOMX.removeItem((int)bufferHandle);
#endif

        if(mDebugFcs)
        {
            CAMHAL_LOGDB("C[%d] D[%d] E[%d]", mFramesWithOMX, mFramesWithDisplay, mFramesWithEncoder);
        }
        stat = sendCallBacks(cameraFrame,  mask);

        stat |= advanceZoom();//--

        // On the fly update to 3A settings not working
        // Do not update 3A here if we are in the middle of a capture
        // or in the middle of transitioning to it
        if( mPending3Asettings && ((nextState & CAPTURE_ACTIVE) == 0))
        {
            apply3Asettings(mParameters3A);
        }
        
        stat |= checkFocusStatus();

#ifdef CAMERA_IMAGE_SNAPSHOT
        if(mCaptureUsePreviewFrame && mCaptureUsePreviewFrameStarted && (mCapturedFrames>=1))
        {
            if(mFlashStrobed)
            {
                if((--mFlashConvergenceFrame) <=0 )
                {
                    needSendCaptureImage = true;
                }
            }
            else 
            {
                needSendCaptureImage = true;
            }
        }
#endif


    }
    if( portIndex == OMX_CAMERA_PORT_IMAGE_OUT_IMAGE || needSendCaptureImage )
    {
        const char *valstr = NULL;

        if ( 1 > mCapturedFrames )
        {
            goto EXIT;
        }

        CAMERA_SCOPEDTRACE("OMXCameraAdapterFillBufferDone_imageport");
        if(needSendCaptureImage)
        {
            cameraFrame.mQuirks = 0;
            if(!(mAdapterState&VIDEO_ACTIVE))
            {
                //image capture status, the preview data size is same as image data size, so just copy the preview port parameters  
                cameraFrame.mEncodeWidth= imgCaptureParam->mEncodeWidth;
                cameraFrame.mEncodeHeight= imgCaptureParam->mEncodeHeight;
                {
                    Mutex::Autolock lock(mZoomLock);
                    cameraFrame.mWidth = imgCaptureParam->mZoomWidth;
                    cameraFrame.mHeight = imgCaptureParam->mZoomHeight;
                    cameraFrame.mXOff = imgCaptureParam->mZoomXOff;
                    cameraFrame.mYOff = imgCaptureParam->mZoomYOff;
                }
		/**
                * BUGFIX: fix for 5.1 cts recordinghint,when use RecordingHint mode, overide for ignore digtal zoom.
                *ActionsCode(author:liyuan, change_code)
                */	
		if(mIternalRecordingHint){
		    cameraFrame.mWidth = cameraFrame.mOrigWidth;
		    cameraFrame.mHeight = cameraFrame.mOrigHeight;
		    cameraFrame.mXOff = 0;
		    cameraFrame.mYOff = 0;
		}
            }
            else
            {
		/**
    		* BUGFIX: fix for 5.1 cts testVideoSnapshot fail . 
    		*ActionsCode(author:liyuan, change_code)
    		*/
		unsigned int videoEncodeWidth = cameraFrame.mEncodeWidth;
		unsigned int videoEncodeHeight = cameraFrame.mEncodeHeight;
                //video snapshot status, ingore digtal zoom 
                cameraFrame.mWidth = cameraFrame.mOrigWidth;
                cameraFrame.mHeight = cameraFrame.mOrigHeight;
                cameraFrame.mXOff = 0;
                cameraFrame.mYOff = 0;

#ifdef CAMERA_GS705A
                if((cameraFrame.mOrigWidth>1280 && cameraFrame.mOrigHeight > 720) ||
		   (videoEncodeWidth>1280 && videoEncodeHeight > 720))
                {
                    //5M
                    cameraFrame.mEncodeWidth = 2560;
                    cameraFrame.mEncodeHeight = 1920;
                }
                else if((cameraFrame.mOrigWidth > 640 && cameraFrame.mOrigHeight > 480) ||
			(videoEncodeWidth>640 && videoEncodeHeight > 480))
                {
                    //2M
                    cameraFrame.mEncodeWidth = 1600;
                    cameraFrame.mEncodeHeight = 1200;
                }
                else if((cameraFrame.mOrigWidth<=640 && cameraFrame.mOrigHeight <= 480) ||
			(videoEncodeWidth<=640 && videoEncodeHeight <=480))
                {
                    //640x480
                    cameraFrame.mEncodeWidth = 640;
                    cameraFrame.mEncodeHeight = 480;
                }
                else
                {
                    cameraFrame.mEncodeWidth = cameraFrame.mOrigWidth;
                    cameraFrame.mEncodeHeight = cameraFrame.mOrigHeight;
                }
#endif

                //ActionsCode(author:liuyiguang, change_code for gs900a only, fix for video snapshot failed in cts test!)
//#ifdef CAMERA_GS900A
//                cameraFrame.mEncodeWidth = cameraFrame.mOrigWidth;
//                cameraFrame.mEncodeHeight = cameraFrame.mOrigHeight;
//#endif
            }
        }
        else
        {
            cameraFrame.mStreamType = CameraFrame::IMAGE_STREAM_TYPE;
        }

        //vce don't support downscale
        CAMHAL_LOGDB("cameraFrame.mEncodeWidth=%d",cameraFrame.mEncodeWidth);
        CAMHAL_LOGDB("cameraFrame.mEncodeHeight=%d",cameraFrame.mEncodeHeight);
        CAMHAL_LOGDB("cameraFrame.mWidth=%d",cameraFrame.mWidth);
        CAMHAL_LOGDB("cameraFrame.mHeight=%d",cameraFrame.mHeight);
        CAMHAL_LOGDB("cameraFrame.mOrigWidth=%d",cameraFrame.mOrigWidth);
        CAMHAL_LOGDB("cameraFrame.mOrigHeight=%d",cameraFrame.mOrigHeight);
        if( ((cameraFrame.mEncodeWidth-1)/cameraFrame.mOrigWidth>=MAX_CAMERA_SCALE) 
            || ((cameraFrame.mEncodeHeight-1)/cameraFrame.mOrigHeight>=MAX_CAMERA_SCALE))
        {
            cameraFrame.mQuirks |= CameraFrame::ENCODE_WITH_SW;
        }

        //vce do not support zoom out
        //ActionsCode(author:liuyiguang, change_code, relax the solution of encode)
        if( (cameraFrame.mEncodeWidth < cameraFrame.mWidth / 2) 
            || (cameraFrame.mEncodeHeight < cameraFrame.mHeight / 2))
        {
            cameraFrame.mQuirks |= CameraFrame::ENCODE_WITH_SW;
        }

        mask = (unsigned int) CameraFrame::IMAGE_FRAME_SYNC;
        mask |= (unsigned int) CameraFrame::RAW_FRAME_SYNC;


        if(strcmp(mPictureFormatFromClient, CameraParameters::PIXEL_FORMAT_JPEG)==0)
        {
            cameraFrame.mQuirks |= CameraFrame::ENCODE_RAW_TO_JPEG;

            void *exif;
            getExif(reinterpret_cast<void *>(bufferHandle),&exif, &exifType);
            cameraFrame.mQuirks |= CameraFrame::HAS_EXIF_DATA;
            cameraFrame.mQuirks |= (exifType&(CameraFrame::HAS_EXIF_DATA_LIBJPEG|CameraFrame::HAS_EXIF_DATA_VCE));
            cameraFrame.mCookie2 = (void*) exif;
        }

        if((mCapturedFrames>0) && !mCaptureSignalled)
        {
            mCaptureSignalled = true;
            mCaptureSem.Signal();
        }

        if( ( CAPTURE_ACTIVE & state ) != CAPTURE_ACTIVE )
        {
            goto EXIT;
        }


        CAMHAL_LOGDB("Captured Frames: %d", mCapturedFrames);
        
        notifyShutterSubscribers();
        stat = sendCallBacks(cameraFrame, mask);

        //NOTE: updating mCapturedFrames should be after sendCallBacks
        mCapturedFrames--;

        if(mFlashStrobed && (mCapturedFrames<=0) )
        {
            stopFlashStrobe();
            mFlashStrobed = false;
        }

    }
EXIT:
    endTransaction(bufferHandle);
    

    return eError;

ERROR:
    endTransaction(bufferHandle);

    CAMHAL_LOGEB("Exiting function %s because of ret %d eError=%x", __FUNCTION__, stat, eError);

    if ( NO_ERROR != stat )
    {
        if ( NULL != mErrorNotifier )
        {
            mErrorNotifier->errorNotify(CAMERA_ERROR_UNKNOWN);
        }
    }

    return eError;
}

status_t OMXCameraAdapter::recalculateFPS()
{
    float currentFPS;

    mFrameCount++;

    if ( ( mFrameCount % FPS_PERIOD ) == 0 )
    {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFPSTime;
        currentFPS =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFPSTime = now;
        mLastFrameCount = mFrameCount;

        if ( 1 == mIter )
        {
            mFPS = currentFPS;
        }
        else
        {
            //cumulative moving average
            mFPS = mLastFPS + (currentFPS - mLastFPS)/mIter;
        }

        mLastFPS = mFPS;
        mIter++;
    }

    return NO_ERROR;
}

status_t OMXCameraAdapter::sendCallBacks(CameraFrame frame, unsigned int mask)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if (ret != NO_ERROR)
    {
        CAMHAL_LOGDB("Error in setInitFrameRefCount %d", ret);
    }
    else
    {
        ret = sendFrameToSubscribers(&frame, mask);
    }
    CAMHAL_LOGDB("B 0x%p T %llu", frame.mBuffer, frame.mTimestamp);


    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::initCameraFrame( CameraFrame &frame,
        OMX_IN OMX_BUFFERHEADERTYPE *pBuffHeader, void *handle,
        OMXCameraPortParameters *port)
{
    status_t ret = NO_ERROR;

    video_metadata_t *meta = (video_metadata_t *)pBuffHeader->pBuffer;

    LOG_FUNCTION_NAME;

    frame.mFormat = CameraFrame::getFrameFormatFromOmx(port->mColorFormat);
    frame.mBuffer = handle;
    frame.mLength = port->mBufSize;// * port->mHeight;//pBuffHeader->nFilledLen;
    frame.mStride = port->mStride;
    frame.mOffset = pBuffHeader->nOffset;
    {
        Mutex::Autolock lock(mZoomLock);
        frame.mWidth = port->mZoomWidth;
        frame.mHeight = port->mZoomHeight;
        frame.mXOff = port->mZoomXOff;
        frame.mYOff = port->mZoomYOff;
    }
#ifndef CAMERA_ANDROID16 
#ifdef CAMERA_HDR
    if(mHdrSupport && meta->crop_w != 0 && meta->crop_h != 0)
    {
        if((int)frame.mXOff < meta->off_x)
        {
            frame.mXOff = SIZE_ALIGN_UP_16(meta->off_x);
        }
        if((int)frame.mYOff < meta->off_y)
        {
            frame.mYOff = SIZE_ALIGN_UP_16(meta->off_y);
        }
        if(frame.mWidth > meta->crop_w +  meta->off_x - frame.mXOff)
        {
            frame.mWidth = SIZE_ALIGN_DOWN_16(meta->crop_w +  meta->off_x - frame.mXOff);
        }
        if(frame.mHeight > meta->crop_h + meta->off_y - frame.mYOff)
        {
            frame.mHeight = SIZE_ALIGN_DOWN_16(meta->crop_h + meta->off_y - frame.mYOff);
        }
    }
#endif
#endif
    CAMHAL_LOGDB("frame.mLength = %d", frame.mLength);
    CAMHAL_LOGDB("frame.mStride = %d", frame.mStride);
    frame.mVaddr = NULL;

    frame.mOrigWidth= port->mWidth;
    frame.mOrigHeight= port->mHeight;

    frame.mEncodeWidth= port->mEncodeWidth;
    frame.mEncodeHeight= port->mEncodeHeight;

    frame.mTimestamp = pBuffHeader->nTimeStamp ;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

bool OMXCameraAdapter::CommandHandler::Handler()
{
    ActUtils::Message msg;
    volatile int forever = 1;
    status_t stat;
    ErrorNotifier *errorNotify = NULL;

    LOG_FUNCTION_NAME;

    while ( forever )
    {
        stat = NO_ERROR;
        CAMHAL_LOGDA("Handler: waiting for messsage...");
        ActUtils::MessageQueue::waitForMsg(&mCommandMsgQ, NULL, NULL, -1);
        {
            Mutex::Autolock lock(mLock);
            mCommandMsgQ.get(&msg);
        }
        CAMHAL_LOGDB("msg.command = %d", msg.command);
        switch ( msg.command )
        {
        case CommandHandler::CAMERA_START_IMAGE_CAPTURE:
            {
                stat = mCameraAdapter->startImageCapture();
                break;
            }

        case CommandHandler::COMMAND_EXIT:
            {
                CAMHAL_LOGDA("Exiting command handler");
                forever = 0;
                break;
            }
        case CommandHandler::CAMERA_SWITCH_TO_EXECUTING:
            {
                stat = mCameraAdapter->doSwitchToExecuting();
                break;
            }
        case CommandHandler::CAMERA_END_IMAGE_CALLBACK:
            {
                if ( NULL != mCameraAdapter->mEndImageCaptureCallback)
                {
                    mCameraAdapter->mEndImageCaptureCallback(mCameraAdapter->mEndCaptureData);
                }
            }
        }

    }

    LOG_FUNCTION_NAME_EXIT;

    return false;
}

bool OMXCameraAdapter::AFHandler::Handler()
{
    ActUtils::Message msg;
    volatile int forever = 1;
    status_t stat;
    ErrorNotifier *errorNotify = NULL;

    LOG_FUNCTION_NAME;

    while ( forever )
    {
        stat = NO_ERROR;
        CAMHAL_LOGDA("Handler: waiting for messsage...");
        ActUtils::MessageQueue::waitForMsg(&mCommandMsgQ, NULL, NULL, -1);
        {
            Mutex::Autolock lock(mLock);
            mCommandMsgQ.get(&msg);
        }
        CAMHAL_LOGDB("msg.command = %d", msg.command);
        switch ( msg.command )
        {
        case AFHandler::CAMERA_PERFORM_AUTOFOCUS:
            {
                stat = mCameraAdapter->doAutoFocus();
                break;
            }
        case AFHandler::COMMAND_EXIT:
            {
                CAMHAL_LOGDA("Exiting command handler");
                forever = 0;
                break;
            }
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return false;
}

bool OMXCameraAdapter::OMXCallbackHandler::Handler()
{
    ActUtils::Message msg;
    volatile int forever = 1;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    while(forever)
    {
        ActUtils::MessageQueue::waitForMsg(&mCommandMsgQ, NULL, NULL, -1);
        {
            Mutex::Autolock lock(mLock);
            mCommandMsgQ.get(&msg);
        }


        switch ( msg.command )
        {
        case OMXCallbackHandler::CAMERA_FILL_BUFFER_DONE:
            {
                ret = mCameraAdapter->OMXCameraAdapterFillBufferDone(( OMX_HANDLETYPE ) msg.arg1,
                        ( OMX_BUFFERHEADERTYPE *) msg.arg2, msg.arg3);
                break;
            }
        case CommandHandler::COMMAND_EXIT:
            {
                CAMHAL_LOGDA("Exiting OMX callback handler");
                forever = 0;
                break;
            }
        }
    }

    LOG_FUNCTION_NAME_EXIT;
    return false;
}



OMX_OTHER_EXTRADATATYPE *OMXCameraAdapter::getExtradata(OMX_OTHER_EXTRADATATYPE *extraData, OMX_EXTRADATATYPE type)
{

    // Required extradata type wasn't found
    return NULL;
}

OMXCameraAdapter::OMXCameraAdapter(size_t sensor_index): mComponentState (OMX_StateLoaded)
{
    LOG_FUNCTION_NAME;

    mSensorIndex = sensor_index;
    mPictureRotation = 0;

    mDoAFSem.Create(0);
    mInitSem.Create(0);
    mFlushSem.Create(0);
    mUsePreviewDataSem.Create(0);
    mUsePreviewSem.Create(0);
    mUseCaptureSem.Create(0);
    mStartPreviewSem.Create(0);
    mStopPreviewSem.Create(0);
    mStartCaptureSem.Create(0);
    mStopCaptureSem.Create(0);
    mSwitchToLoadedSem.Create(0);
    mCaptureSem.Create(0);

    mSwitchToExecSem.Create(0);

    mCameraAdapterParameters.mHandleComp = 0;

    mUserSetExpLock = OMX_FALSE;
    mUserSetWbLock = OMX_FALSE;

    mFramesWithOMX = 0;
    mFramesWithDisplay = 0;
    mFramesWithEncoder = 0;

    LOG_FUNCTION_NAME_EXIT;
}

OMXCameraAdapter::~OMXCameraAdapter()
{
    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(gAdapterLock);

#ifdef CAMERA_WATCHDOG
    //stop watchdog
    if(mWatchDog != NULL)
    {
        mWatchDog->stopWatchDog();
    }
#endif

    destroyFaceDetection();
    //Return to OMX Loaded state
    switchToLoaded();



    //Remove any unhandled events
    {
    Mutex::Autolock lock(mEventLock);    
    if ( !mEventSignalQ.isEmpty() )
    {
        for (unsigned int i = 0 ; i < mEventSignalQ.size() ; i++ )
        {
            ActUtils::Message *msg = mEventSignalQ.itemAt(i);
            //remove from queue and free msg
            if ( NULL != msg )
            {
                Semaphore *sem  = (Semaphore*) msg->arg3;
                sem->Signal();
                free(msg);

            }
        }
        mEventSignalQ.clear();
    }
    }

    //Exit and free ref to command handling thread
    if ( NULL != mCommandHandler.get() )
    {
        ActUtils::Message msg;
        msg.command = CommandHandler::COMMAND_EXIT;
        msg.arg1 = mErrorNotifier;
        mCommandHandler->clearCommandQ();
        mCommandHandler->put(&msg);
        mCommandHandler->requestExitAndWait();
        mCommandHandler.clear();
    }

    //Exit and free ref to command handling thread
    if ( NULL != mAFHandler.get() )
    {
        ActUtils::Message msg;
        msg.command = CommandHandler::COMMAND_EXIT;
        msg.arg1 = mErrorNotifier;
        mAFHandler->clearCommandQ();
        mAFHandler->put(&msg);
        mAFHandler->requestExitAndWait();
        mAFHandler.clear();
    }
    //Exit and free ref to callback handling thread
    if ( NULL != mOMXCallbackHandler.get() )
    {
        ActUtils::Message msg;
        msg.command = OMXCallbackHandler::COMMAND_EXIT;
        //Clear all messages pending first
        mOMXCallbackHandler->clearCommandQ();
        mOMXCallbackHandler->put(&msg);
        mOMXCallbackHandler->requestExitAndWait();
        mOMXCallbackHandler.clear();
    }

    clearMetadataBufs(mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex]);
    clearMetadataBufs(mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mImagePortIndex]);

    freeExifObjects();

    ///De-init the OMX
    if( (mComponentState==OMX_StateLoaded) || (mComponentState==0/*OMX_StateInvalid*/))
    {
        ///Free the handle for the Camera component
        if(mCameraAdapterParameters.mHandleComp)
        {
            OMX_FreeHandle(mCameraAdapterParameters.mHandleComp);
            mCameraAdapterParameters.mHandleComp = NULL;
        }

        OMX_Deinit();
    }
    else
    {
        CAMHAL_LOGEA("Error when OMX_Deinit, OMX Component is not in OMX_StateLoaded or OMX_StateInvalid state");
    }

    LOG_FUNCTION_NAME_EXIT;
}

extern "C" CameraAdapter* CameraAdapter_Factory(size_t sensor_index)
{
    CameraAdapter *adapter = NULL;
    Mutex::Autolock lock(gAdapterLock);

    LOG_FUNCTION_NAME;

    adapter = new OMXCameraAdapter(sensor_index);
    if ( adapter )
    {
        CAMHAL_LOGDB("New OMX Camera adapter instance created for sensor %d",sensor_index);
    }
    else
    {
        CAMHAL_LOGEA("Camera adapter create failed!");
    }

    LOG_FUNCTION_NAME_EXIT;

    return adapter;
}

OMX_ERRORTYPE OMXCameraAdapter::OMXCameraGetHandle(OMX_HANDLETYPE *handle, OMX_PTR pAppData,  OMX_CALLBACKTYPE *pCallbacks)
{
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

    int retries = 5;
    while(eError!=OMX_ErrorNone && --retries>=0)
    {

        CAMERA_SCOPEDTRACE("OMXCameraGetHandle");
        // Get Handle
        CAMHAL_LOGVA("OMX_GetHandle before!!!");
        eError = OMX_GetHandle(handle, (OMX_STRING)CAMERA_OMX_NAME, pAppData, pCallbacks);
        CAMHAL_LOGVA("OMX_GetHandle after!!!");
        if (eError != OMX_ErrorNone)
        {
            CAMHAL_LOGEB("OMX_GetHandle -0x%x", eError);
            //Sleep for 100 mS
            usleep(100000);
        }
        else
        {
            break;
        }
    }

    return eError;
}

/**
* NEW_FEATURE: Add buffer_state_dump function,when watchdog timeout happens.
*ActionsCode(author:liyuan, change_code)
*/
void OMXCameraAdapter::Hal_Dump_Bufs_Occupied()
{
	int imagebufNum=0,previewbufNum=0,BufNumOccupied=0;
	int i,mask=1;
	char * UsrType[CameraFrame::MAX_FRAME_TYPE] ={
		"PREVIEW_FRAME_SYNC","PREVIEW_RAW_FRAME_SYNC","VIDEO_FRAME_SYNC",
		"FD_FRAME_SYNC","RAW_FRAME_SYNC","IMAGE_FRAME_SYNC"};
	imagebufNum = mStreamHub->mStreams[CameraFrame::IMAGE_STREAM_TYPE].getStreamBuffersSize();
	previewbufNum = mStreamHub->mStreams[CameraFrame::PREVIEW_STREAM_TYPE].getStreamBuffersSize();	
	
	ALOGD("***Hal_Dump_Bufs_Occupied:  [img alloc:%d] / [prev alloc:%d]\n",imagebufNum,previewbufNum);
	ALOGD("***bufs passed to drv : %d\n",mFramesWithOMX);

	for(i=0; i < CameraFrame::MAX_FRAME_TYPE; i++,mask<<=1)
	{
		BufNumOccupied = mStreamHub->getRefCountByFrameTypes((uint32_t)(mask));
		ALOGD("***bufrefs Occupied by %s : %d\n",UsrType[i],BufNumOccupied);
	}
}

void OMXCameraAdapter::onWatchDogMsg(int msg)
{
    if(msg == CameraWatchDog::WATCHDOG_MSG_TIMEOUT)
    {
        mErrorNotifier->errorNotify(CAMERA_ERROR_FATAL);
    }
}

OMX_ERRORTYPE OMXCameraAdapterCapEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData)
{
    LOG_FUNCTION_NAME;

    CAMHAL_LOGEB("Event %d", eEvent);

    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if(eEvent == OMX_EventCmdComplete)
    {
        switch(nData1)
        {
        case OMX_ACT_IndexParamSensorSelect:

            break;
        }
    }

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

OMX_ERRORTYPE OMXCameraAdapterCapEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader)
{
    LOG_FUNCTION_NAME;

    OMX_ERRORTYPE eError = OMX_ErrorNone;


    LOG_FUNCTION_NAME_EXIT;
    return eError;
}
OMX_ERRORTYPE OMXCameraAdapterCapFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader)
{
    ActUtils::Message msg;
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    return eError;
}

/**
* NEW_FEATURE: Add UVC module support .
*ActionsCode(author:liyuan, change_code)
*/
OMX_UVCMODE OMXCameraAdapter::mUVCReplaceMode = OMX_UVC_NONE;
/**
* NEW_FEATURE: Add UVC module support .
*ActionsCode(author:liyuan, change_code)
*/
bool OMXCameraAdapter::query_UVC_ReplaceMode(){
    bool ret = false;
    int mode =0;
	
    //fix video3 as uvc device by agreement.
    const char *uvc_dev = "/dev/video3";

    if (access(uvc_dev, R_OK) == 0)
    {
		mode = CameraConfigs::getUVCReplaceMode();
			
		if(mode==1){
			mUVCReplaceMode = OMX_UVC_AS_REAR;				
		}else if(mode==2){
			mUVCReplaceMode = OMX_UVC_AS_FRONT;					
		}else{
			mUVCReplaceMode = OMX_UVC_NONE;
		}
			
		ret = true;
		
    }else{
		mUVCReplaceMode = OMX_UVC_NONE;
	}
	
	ALOGD("UVCReplaceMode = %d\n",mUVCReplaceMode);
    return ret;
}
/**
* NEW_FEATURE: Add UVC module support .
*ActionsCode(author:liyuan, change_code)
*/
bool OMXCameraAdapter::get_UVC_ReplaceMode(OMX_UVCMODE *mode, int SensorIndex){
    const char *uvc_dev = "/dev/video3";

    if((mUVCReplaceMode == OMX_UVC_AS_REAR &&SensorIndex==0) ||
       (mUVCReplaceMode == OMX_UVC_AS_FRONT &&SensorIndex==1)){
	if (access(uvc_dev, R_OK) != 0){
		ALOGE("uvc does not exist now, while config UVCReplaceMode = %d\n",mUVCReplaceMode);
		return false;
	}
    }

    *mode = mUVCReplaceMode;
    ALOGD("get UVCReplaceMode = %d\n",mUVCReplaceMode);
    return true;
}

extern "C" int CameraAdapter_Capabilities(CameraProperties::Properties* properties_array,
        const unsigned int starting_camera,
        const unsigned int max_camera,
        int *XMLNeedsUpdate)
{
    int num_cameras_supported = 0;
    int camera_iter = 0;
    CameraProperties::Properties* properties = NULL;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    OMX_HANDLETYPE handle = NULL;
    OMX_ACT_CAPTYPE caps;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(gAdapterLock);

    if (!properties_array)
    {
        CAMHAL_LOGEB("invalid param: properties = 0x%p", properties_array);
        LOG_FUNCTION_NAME_EXIT;
        return -EINVAL;
    }

    eError = OMX_Init();
    if (eError != OMX_ErrorNone)
    {
        CAMHAL_LOGEB("Error OMX_Init -0x%x", eError);
        return eError;
    }

    eError = OMXCameraAdapter::OMXCameraGetHandle(&handle, NULL, &oCapCallbacks);
    if (eError != OMX_ErrorNone)
    {
        CAMHAL_LOGEB("OMX_GetHandle -0x%x", eError);
        goto EXIT;
    }
    /**
    * NEW_FEATURE: Add UVC module support .
    *ActionsCode(author:liyuan, change_code)
    */
    if(OMXCameraAdapter::query_UVC_ReplaceMode()==false){
	ALOGD("uvc driver is not usable.");
    }

    // Continue selecting sensor and then querying OMX Camera for it's capabilities
    // When sensor select returns an error, we know to break and stop
    while ((starting_camera + camera_iter) < max_camera)
    {
        // get and fill capabilities
        properties = properties_array + starting_camera + num_cameras_supported;
        ret = OMXCameraAdapter::getCaps(properties, handle, camera_iter);


        if(ret != NO_ERROR)
        {
		   camera_iter++;
           continue;
        }

        // need to fill facing information
        // assume that only sensor 0 is back facing,1 is front facing, other is default to back facing
        if (camera_iter == 0)
        {
            properties->set(CameraProperties::FACING_INDEX, ActCameraParameters::FACING_BACK);
        }
        else if(camera_iter == 1)
        {
            properties->set(CameraProperties::FACING_INDEX, ActCameraParameters::FACING_FRONT);
        }
        else
        {
            properties->set(CameraProperties::FACING_INDEX, ActCameraParameters::FACING_BACK);
        }

		camera_iter++;
        num_cameras_supported++;
		*XMLNeedsUpdate=1;
    }
    ALOGD("num_cameras_supported=%d",  num_cameras_supported);

EXIT:
    // clean up
    if(handle)
    {
        OMX_FreeHandle(handle);
        handle=NULL;
    }
    OMX_Deinit();

    LOG_FUNCTION_NAME_EXIT;

    return num_cameras_supported;
}

};


/*--------------------Camera Adapter Class ENDS here-----------------------------*/
