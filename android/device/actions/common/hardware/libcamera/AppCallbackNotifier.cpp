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





#include "CameraATraceTag.h"

#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "video_mediadata.h"
#include "Encoder_libjpeg.h"
#include <media/hardware/MetadataBufferType.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include "Resize.h"
#include "Converters.h"

#include "OMXVce.h"

#include "EncoderJpegVce.h"

#include "CameraGpuOutstanding.h"
#include "CameraFreqAdapter.h"

#include "vce_resize.h"


#define ALIGN16(n) (((n)+0xf)&(~0xf))


namespace android
{

const int AppCallbackNotifier::NOTIFIER_TIMEOUT = -1;

/**
  * NotificationHandler class
  */

///Initialization function for AppCallbackNotifier
status_t AppCallbackNotifier::initialize()
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;

    mMeasurementEnabled = false;

#ifdef CAMERA_VCE_OMX_JPEG
    mVceJpegEncoder = new OMXVceJpegEncoder();
    mVceJpegEncoder->setVceObserver(&mImageJpegObserver);
    ret =mVceJpegEncoder->init();
    if ( NO_ERROR != ret )
    {
        CAMHAL_LOGEA("Couldn't mVceJpegEncoder.init!");
        goto EXIT;
    }

#endif

#ifdef CAMERA_VCE_OMX_RESIZE
    mVceImageResize= new OMXVceImageResize();
    mVceImageResize->setVceObserver(&mImageResizeObserver);
    ret =mVceImageResize->init();
    if ( NO_ERROR != ret )
    {
        CAMHAL_LOGEA("Couldn't mVceImageResize.init!");
        goto EXIT;
    }
    mResizeType = UNUSED_RESIZE;


#endif

    ///Create the app notifier thread
    mNotificationThread = new NotificationThread(this);
    if(!mNotificationThread.get())
    {
        CAMHAL_LOGEA("Couldn't create Notification thread");
        return NO_MEMORY;
    }

    ///Start the display thread
    ret = mNotificationThread->run("NotificationThread", PRIORITY_URGENT_DISPLAY);
    if(ret!=NO_ERROR)
    {
        CAMHAL_LOGEA("Couldn't run NotificationThread");
        mNotificationThread.clear();
        return ret;
    }


    ///Create the jpeg thread
    mJpegThread = new JpegThread(this);
    if(!mJpegThread.get())
    {
        CAMHAL_LOGEA("Couldn't create Jpeg thread");
        return NO_MEMORY;
    }

    ///Start the display thread
    ret = mJpegThread->run("JpegThread", PRIORITY_URGENT_DISPLAY);
    if(ret!=NO_ERROR)
    {
        CAMHAL_LOGEA("Couldn't runJpegThread");
        mJpegThread.clear();
        return ret;
    }


    ///Create the resize thread
    mResizeThread = new ResizeThread(this);
    if(!mResizeThread.get())
    {
        CAMHAL_LOGEA("Couldn't create Resize thread");
        return NO_MEMORY;
    }

    ///Start the display thread
    ret = mResizeThread->run("ResizeThread", PRIORITY_URGENT_DISPLAY);
    if(ret!=NO_ERROR)
    {
        CAMHAL_LOGEA("Couldn't run ResizeThread");
        mResizeThread.clear();
        return ret;
    }
    

    mFrameProvider = NULL;

    mUseMetaDataBufferMode = false;
    mRawAvailable = false;

    mVideoMemory = NULL;

    mPreviewMemory = NULL;
    mPreAllocPreviewCBbufsdone = false;

    mPreviewBufCount = 0;
    mPreviewBufSize = 0;

    mPreviewing = false;

    mFirstRecordingFrame = false;
    mTimeStampDelta = 0;


    mNotifierState = AppCallbackNotifier::NOTIFIER_STOPPED;
    mRecording = false;

    mNotifierThreadSem.Create(0);
    mJpegThreadSem.Create(0);
    mResizeThreadSem.Create(0);
    

    mPreviewFormat = CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP;
    mVideoFormat = CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP;

    mUseCameraHalVideoBuffers = false;
    
    //ActionsCode(author:liuyiguang, add_code)
    mVceResizeHandle = NULL;
	

    LOG_FUNCTION_NAME_EXIT;
EXIT:
    return ret;
}

void AppCallbackNotifier::setCallbacks(CameraHal* cameraHal,
                                       camera_notify_callback notify_cb,
                                       camera_data_callback data_cb,
                                       camera_data_timestamp_callback data_cb_timestamp,
                                       camera_request_memory get_memory,
                                       void *user)
{
    Mutex::Autolock lock(mLock);

    LOG_FUNCTION_NAME;

    mCameraHal = cameraHal;
    mNotifyCb = notify_cb;
    mDataCb = data_cb;
    mDataCbTimestamp = data_cb_timestamp;
    mRequestMemory = get_memory;//
    mCallbackCookie = user;

    LOG_FUNCTION_NAME_EXIT;
}

void AppCallbackNotifier::setMeasurements(bool enable)
{

    LOG_FUNCTION_NAME;


    LOG_FUNCTION_NAME_EXIT;
}

void AppCallbackNotifier::setPreviewFormat(CameraFrame::FrameFormat format)
{
    LOG_FUNCTION_NAME;

    mPreviewFormat = format;

    LOG_FUNCTION_NAME_EXIT;
}

void AppCallbackNotifier::setVideoFormat(CameraFrame::FrameFormat format)
{
    LOG_FUNCTION_NAME;

    mVideoFormat = format;

    LOG_FUNCTION_NAME_EXIT;
}
//All sub-components of Camera HAL call this whenever any error happens
void AppCallbackNotifier::errorNotify(int error)
{
    LOG_FUNCTION_NAME;

    CAMHAL_LOGEB("AppCallbackNotifier received error %d", error);

    // If it is a fatal error abort here!
    if((error == CAMERA_ERROR_FATAL) || (error == CAMERA_ERROR_HARD))
    {
        //We kill media server if we encounter these errors as there is
        //no point continuing and apps also don't handle errors other
        //than media server death always.
        
        clearGPUOutstanding();
#ifdef CAMERA_SET_FREQ_RANGE
        CameraForceRestoreCpuFreqRange();
#endif
        abort();
        return;
    }

    if (  ( NULL != mCameraHal ) &&
            ( NULL != mNotifyCb ) &&
            ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_ERROR) ) )
    {
        CAMHAL_LOGEB("AppCallbackNotifier mNotifyCb %d", error);
        mNotifyCb(CAMERA_MSG_ERROR, CAMERA_ERROR_UNKNOWN, 0, mCallbackCookie);
    }

    LOG_FUNCTION_NAME_EXIT;
}

bool AppCallbackNotifier::notificationThread()
{
    bool shouldLive = true;
    status_t ret;

    LOG_FUNCTION_NAME;

    //CAMHAL_LOGDA("Notification Thread waiting for message");
    ret = ActUtils::MessageQueue::waitForMsg(&mNotificationThread->msgQ(),
            &mEventQ,
            &mFrameQ,
            AppCallbackNotifier::NOTIFIER_TIMEOUT);

    //CAMHAL_LOGDA("Notification Thread received message");

    if (mNotificationThread->msgQ().hasMsg())
    {
        ///Received a message from CameraHal, process it
        CAMHAL_LOGDA("Notification Thread received message from Camera HAL");
        shouldLive = processNotificationMessage();
        if(!shouldLive)
        {
            CAMHAL_LOGDA("Notification Thread exiting.");
            return shouldLive;
        }
    }
    if(mNotifierState == AppCallbackNotifier::NOTIFIER_STOPPED)
    {
        return shouldLive; 
    }

    if(mEventQ.hasMsg())
    {
        ///Received an event from one of the event providers
        CAMHAL_LOGDA("Notification Thread received an event from event provider (CameraAdapter)");
        notifyEvent();
    }

    if(mFrameQ.hasMsg())
    {
        ///Received a frame from one of the frame providers
        //CAMHAL_LOGDA("Notification Thread received a frame from frame provider (CameraAdapter)");
        notifyFrame();
    }

    LOG_FUNCTION_NAME_EXIT;
    return shouldLive;
}
bool AppCallbackNotifier::jpegThread(JpegThread * thread)
{
    bool shouldLive = true;
    status_t ret;

    LOG_FUNCTION_NAME;

    //CAMHAL_LOGDA("Notification Thread waiting for message");
    ret = ActUtils::MessageQueue::waitForMsg(&mJpegThread->msgQ(),
            &mJpegQ,
            NULL,
            AppCallbackNotifier::NOTIFIER_TIMEOUT);

    //CAMHAL_LOGDA("Notification Thread received message");

    if (mJpegThread->msgQ().hasMsg())
    {
        ///Received a message from CameraHal, process it
        CAMHAL_LOGDA("Jpeg Thread received message from Camera HAL");
        shouldLive = processJpegMessage(thread);
        if(!shouldLive)
        {
            CAMHAL_LOGDA("Jpeg Thread exiting.");
            return shouldLive;
        }
    }

    if(mJpegQ.hasMsg())
    {
        ///Received an event from one of the event providers
        CAMHAL_LOGDA("Jpeg Thread received an event from event provider (CameraAdapter)");
        jpegEncode(thread);
    } 
    LOG_FUNCTION_NAME_EXIT;
    return shouldLive;
}

bool AppCallbackNotifier::resizeThread(ResizeThread * thread)
{
    bool shouldLive = true;
    status_t ret;

    LOG_FUNCTION_NAME;

    //CAMHAL_LOGDA("Notification Thread waiting for message");
    ret = ActUtils::MessageQueue::waitForMsg(&mResizeThread->msgQ(),
            &mResizeQ,
            NULL,
            AppCallbackNotifier::NOTIFIER_TIMEOUT);

    //CAMHAL_LOGDA("Notification Thread received message");

    if (mResizeThread->msgQ().hasMsg())
    {
        ///Received a message from CameraHal, process it
        CAMHAL_LOGDA("Resize Thread received message from Camera HAL");
        shouldLive = processResizeMessage(thread);
        if(!shouldLive)
        {
            CAMHAL_LOGDA("Resize Thread exiting.");
            return shouldLive;
        }
    }

    if(mResizeQ.hasMsg())
    {
        ///Received an event from one of the event providers
        CAMHAL_LOGDA("Resize Thread received an event from event provider (CameraAdapter)");
        resizeFrame(thread);
    } 
    LOG_FUNCTION_NAME_EXIT;
    return shouldLive;
}

void AppCallbackNotifier::notifyEvent()
{
    ///Receive and send the event notifications to app
    ActUtils::Message msg;
    LOG_FUNCTION_NAME;
    {
        Mutex::Autolock lock(mEventQLock);
        if(!mEventQ.isEmpty())
        {
            mEventQ.get(&msg);
        }
        else
        {
            return;
        }
    }
    bool ret = true;
    CameraHalEvent *evt = NULL;
    CameraHalEvent::FocusEventData *focusEvtData;
    CameraHalEvent::ZoomEventData *zoomEvtData;
    CameraHalEvent::FaceEventData faceEvtData;

    if(mNotifierState != AppCallbackNotifier::NOTIFIER_STARTED)
    {
        return;
    }

    switch(msg.command)
    {
    case AppCallbackNotifier::NOTIFIER_CMD_PROCESS_EVENT:

        evt = ( CameraHalEvent * ) msg.arg1;

        if ( NULL == evt )
        {
            CAMHAL_LOGEA("Invalid CameraHalEvent");
            return;
        }

        switch(evt->mEventType)
        {
        case CameraHalEvent::EVENT_SHUTTER:

            if ( ( NULL != mCameraHal ) &&
                    ( NULL != mNotifyCb ) &&
                    ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_SHUTTER) ) )
            {
                mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
            }
            mRawAvailable = false;

            break;

        case CameraHalEvent::EVENT_FOCUS_LOCKED:
        case CameraHalEvent::EVENT_FOCUS_ERROR:

            focusEvtData = &evt->mEventData->focusEvent;
            if ( ( focusEvtData->focusStatus == CameraHalEvent::FOCUS_STATUS_SUCCESS ) &&
                    ( NULL != mCameraHal ) &&
                    ( NULL != mNotifyCb ) &&
                    ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_FOCUS) ) )
            {
                mCameraHal->disableMsgType(CAMERA_MSG_FOCUS);
                mNotifyCb(CAMERA_MSG_FOCUS, true, 0, mCallbackCookie);
            }
            else if ( ( focusEvtData->focusStatus == CameraHalEvent::FOCUS_STATUS_FAIL ) &&
                      ( NULL != mCameraHal ) &&
                      ( NULL != mNotifyCb ) &&
                      ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_FOCUS) ) )
            {
                mCameraHal->disableMsgType(CAMERA_MSG_FOCUS);
                mNotifyCb(CAMERA_MSG_FOCUS, false, 0, mCallbackCookie);
            }
            else if ( ( focusEvtData->focusStatus == CameraHalEvent::FOCUS_STATUS_PENDING ) &&
                      ( NULL != mCameraHal ) &&
                      ( NULL != mNotifyCb ) &&
                      ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_FOCUS_MOVE) ) )
            {
                mNotifyCb(CAMERA_MSG_FOCUS_MOVE, true, 0, mCallbackCookie);
            }
            else if ( ( focusEvtData->focusStatus == CameraHalEvent::FOCUS_STATUS_DONE ) &&
                      ( NULL != mCameraHal ) &&
                      ( NULL != mNotifyCb ) &&
                      ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_FOCUS_MOVE) ) )
            {
                mNotifyCb(CAMERA_MSG_FOCUS_MOVE, false, 0, mCallbackCookie);
            }

            break;

        case CameraHalEvent::EVENT_ZOOM_INDEX_REACHED:

            zoomEvtData = &evt->mEventData->zoomEvent;

            if ( ( NULL != mCameraHal ) &&
                    ( NULL != mNotifyCb) &&
                    ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_ZOOM) ) )
            {
                mNotifyCb(CAMERA_MSG_ZOOM, zoomEvtData->currentZoomIndex, zoomEvtData->targetZoomIndexReached, mCallbackCookie);
            }

            break;

        case CameraHalEvent::EVENT_FACE:

            faceEvtData = evt->mEventData->faceEvent;

            if ( ( NULL != mCameraHal ) &&
                    ( NULL != mNotifyCb) &&
                    ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_PREVIEW_METADATA) ) )
            {
                // WA for an issue inside CameraService
                camera_memory_t *tmpBuffer = mRequestMemory(-1, 1, 1, NULL);

                mDataCb(CAMERA_MSG_PREVIEW_METADATA,
                        tmpBuffer,
                        0,
                        faceEvtData->getFaceResult(),
                        mCallbackCookie);

                faceEvtData.clear();

                if ( NULL != tmpBuffer )
                {
                    tmpBuffer->release(tmpBuffer);
                }

            }

            break;

        case CameraHalEvent::EVENT_RAW_IMAGE_NOTIFY:
            mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
            break;

        case CameraHalEvent::EVENT_ERROR_NOTIFY:
            if (  ( NULL != mCameraHal ) &&
                ( NULL != mNotifyCb ) &&
                ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_ERROR) ) )
            {
                mNotifyCb(CAMERA_MSG_ERROR, CAMERA_ERROR_UNKNOWN, 0, mCallbackCookie);
            }
            break;

        case CameraHalEvent::ALL_EVENTS:
            break;
        default:
            break;
        }

        break;
    }

    if ( NULL != evt )
    {
        delete evt;
    }


    LOG_FUNCTION_NAME_EXIT;

}

#ifdef CAMERA_VCE_OMX_RESIZE
//static int tmp = 0;
void AppCallbackNotifier::vceResize(CameraFrame* frame,unsigned char *src, unsigned char *dest, int resizeType, bool align)
{
    OMX_U32 format = -1;

    /*
    ALOGD("frame->mOrigWidth=%d\n",  frame->mOrigWidth);
    ALOGD("frame->mOrigHeight=%d\n",  frame->mOrigHeight);
    ALOGD("frame->mEncodeWidth=%d\n",  frame->mEncodeWidth);
    ALOGD("frame->mEncodeHeight=%d\n",  frame->mEncodeHeight);
    ALOGD("mPreviewBufCount=%d\n",  mPreviewBufCount);

    ALOGD("frame->mXOff=%d\n",  frame->mXOff);
    ALOGD("frame->mYOff=%d\n",  frame->mYOff);
    ALOGD("frame->mWidth=%d\n",  frame->mWidth);
    ALOGD("frame->mHeight=%d\n",  frame->mHeight);
    */

    //Capture buffer maybe change, so CAPTURE_RESIZE should reinit everytime
    if(mResizeType != resizeType || resizeType == CAPTURE_RESIZE)
    {
        mVceImageResize->stopEncode();
        if(frame->mFormat == CameraFrame::CAMERA_FRAME_FORMAT_YUV420P)
        {
            format = OMX_COLOR_FormatYVU420Planar;
        }
        else if(frame->mFormat == CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12)
        {
            format = OMX_COLOR_FormatYUV420Planar;
        }
        else if(frame->mFormat == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP)
        {
            format = OMX_COLOR_FormatYVU420SemiPlanar;
        }
        else if(frame->mFormat == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12)
        {
            format = OMX_COLOR_FormatYUV420SemiPlanar;
        }
        else
        {
            format = OMX_COLOR_FormatUnused;
        }

        if(resizeType == PRVIEW_RESIZE)
        {
            mVceImageResize->setImageSize(frame->mOrigWidth, frame->mOrigHeight);
            mVceImageResize->setOutputImageSize(frame->mEncodeWidth, frame->mEncodeHeight);

            mVceImageResize->setImageFormat((int)format);
            mVceImageResize->setImageInputCnt(mPreviewBufCount);
            mVceImageResize->setPortParameters();
            mVceImageResize->useBuffers(OMXVceImageInputPort, mPreviewBufSize, (void **)mPreviewBufs, mPreviewBufCount);
            mVceImageResize->prepareEncode();
        }
        else if(resizeType == CAPTURE_RESIZE)
        {
            mVceImageResize->setImageSize(frame->mOrigWidth, frame->mOrigHeight);
            mVceImageResize->setOutputImageSize(frame->mEncodeWidth, frame->mEncodeHeight);
            mVceImageResize->setImageFormat((int)format);
            mVceImageResize->setImageInputCnt(mImageBufCount);
            mVceImageResize->setPortParameters();
            mVceImageResize->useBuffers(OMXVceImageInputPort, mImageBufSize, (void **)mImageBufs, mImageBufCount);
            mVceImageResize->prepareEncode();

        }

    }
    mResizeType = resizeType;
    ImageResizeParam param;

    param.xoff=frame->mXOff;
    param.yoff=frame->mYOff;
    param.width=frame->mWidth;
    param.height=frame->mHeight;

    param.inData = frame->mBuffer;
    CAMHAL_LOGDB("inData=0x%x\n",  (unsigned int)param.inData);

    mVceImageResize->encode(&param);

    CAMHAL_LOGDB("outData=0x%x\n",  (unsigned int)param.outData);
    CAMHAL_LOGDB("outDataOffset=%d\n",  param.outDataOffset);
    CAMHAL_LOGDB("outDataSize=%d\n",  param.outDataSize);

    if(param.outData != NULL)
    {
        //the vce output format is yuv420sp
        frame->mFormat = getHwVceOutFormat(format);//CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP;

        cameraFrameConvertForVce(*frame, ((unsigned char *)param.outData) + param.outDataOffset, dest, param.outDataSize, align);
    }

    mVceImageResize->freeEncodeData(param.outData);
    return;
}

/**
 *
 * OPTIMIZE:   Use vce resize to raise the conversion speed.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, add_code)
 */
void AppCallbackNotifier::vceResizeImprove(CameraFrame* frame,unsigned char *src, unsigned char *dest)
{
    int ret = -1;
    void* src_phy = NULL;
    VR_Cmd_t param;

    if (!frame->mBuffer)
    {
        CAMHAL_LOGDA(" frame->mBuffer is NULL!!");
        return;
    }

    buffer_handle_t srcBufferHandle = (buffer_handle_t)frame->mBuffer;
    hw_module_t const * module;
    struct gralloc_module_t const *gralloc_module;    
    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);    
    gralloc_module = reinterpret_cast<struct gralloc_module_t const*>(module);
    ret = gralloc_module->getPhysAddr(gralloc_module, srcBufferHandle, (void**)&src_phy);
    if (ret != 0)
    {
        CAMHAL_LOGDA(" get srcBufferHandle is NULL!!");
        return;
    }

    //set input camera frame format to vce
    switch (frame->mFormat)
    {
        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            param.srcformat = FORMAT_YUV420P;
        break;
        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            param.srcformat = FORMAT_YUV420SP;
        break;
        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            param.srcformat = FORMAT_YVU420P;
        break;
        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            param.srcformat = FORMAT_YVU420SP;
        break;
        default:
            CAMHAL_LOGDA("Src CameraFrame Format is not support!!");
        break;
    }

    //set output camera frame format to vce
    switch (frame->mConvFormat)
    {
        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            CAMHAL_LOGDB("VCE do not support this Dest CameraFrame Format now!!");
            param.dstformat = FORMAT_YUV420P;
        break;
        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            CAMHAL_LOGDB("VCE do not support this Dest CameraFrame Format now!!");
            param.dstformat = FORMAT_YUV420SP;
        break;
        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            param.dstformat = FORMAT_YVU420P;
        break;
        case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            param.dstformat = FORMAT_YVU420SP;
        break;
        default:
            CAMHAL_LOGDA("Dest CameraFrame Format is not support!!");
        break;
    }

    param.src_addr = (unsigned long)src_phy;
    param.srcstride = frame->mOrigWidth;
    param.srcheight = frame->mOrigHeight;
    param.cropx = frame->mXOff;
    param.cropy = frame->mYOff;
    param.cropw = frame->mWidth;
    param.croph = frame->mHeight;
    param.dst_addr = (unsigned long)dest;
    param.bvir_addr = USE_VIRTUAL_ADDRESS;
    param.dstw = frame->mEncodeWidth;
    param.dsth = frame->mEncodeHeight;

    if (mVceResizeHandle)
    {
        //Run vce resize command here!
        ret = VceReSize_Cmd(mVceResizeHandle, VR_CROP, &param);
        if (ret != 0)
        {
            CAMHAL_LOGDA("Vce excute resize error!!");
            return;
        }
    }

    return;
}
#endif

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
void AppCallbackNotifier::encodePictureFrame(CameraFrame* frame, bool canceled)
{
    camera_memory_t* picture = NULL;
    void *dest = NULL, *src = NULL;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    Rect bounds;

    if(canceled)
    {
        CAMHAL_LOGDA("jpeg canceled");
        goto exit;
    }

    if(mNotifierState != AppCallbackNotifier::NOTIFIER_STARTED)
    {
        CAMHAL_LOGDA("mNotifierState is not in started state");
        goto exit;
    }
    if(!frame)
    {
        CAMHAL_LOGDA("Null frame");
        goto exit;
    }
           
    
    if(frame->mFormat == CameraFrame::CAMERA_FRAME_FORMAT_JPEG || frame->mFormat == CameraFrame::CAMERA_FRAME_FORMAT_PNG)
    {
        bounds.left = 0;
        bounds.top = 0;
        bounds.right = frame->mOrigWidth;
        bounds.bottom = frame->mOrigHeight;

        mapper.lock((buffer_handle_t)frame->mBuffer, CAMHAL_GRALLOC_LOCK_USAGE, bounds, &src);
        picture = mRequestMemory(-1, frame->mLength, 1, NULL);

        //ActionsCode(author:liuyiguang, change_code)
        //src = (void *) ((unsigned int)src + frame->mOffset); 
        src = (void *) ((long)src + frame->mOffset); 

        memcpy(picture->data, src, frame->mLength );
        mapper.unlock((buffer_handle_t)frame->mBuffer);
    }
    else
    {
#if 0
        bounds.left = 0;
        bounds.top = 0;
        bounds.right = frame->mOrigWidth;
        bounds.bottom = frame->mOrigHeight;

        mapper.lock((buffer_handle_t)frame->mBuffer, CAMHAL_GRALLOC_LOCK_USAGE, bounds, &src);

        saveFile((unsigned char*)src, frame->mOrigWidth,frame->mOrigHeight,12);

        mapper.unlock((buffer_handle_t)frame->mBuffer);
#endif

#ifdef CAMERA_VCE_OMX_JPEG
        if(CameraFrame::ENCODE_WITH_SW & frame->mQuirks)
        {
            picture = jpegEncodeSW(frame);
        }
        else
        {
            picture = jpegEncodeHW(frame);
        }
#else
        picture = jpegEncodeSW(frame);
#endif
    }
     
exit:
    
    if ( NULL != frame && !canceled)
    {
        sendToFrameQ(AppCallbackNotifier::NOTIFIER_CMD_PROCESS_MEMORY_FRAME, frame,picture,0);
    }
    
    return;
}
status_t AppCallbackNotifier::convertEXIF_libjpeg(CameraHalExif* exif,ExifElementsTable* exifTable,CameraFrame *frame)
{
    status_t ret = NO_ERROR;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    struct timeval sTv;
    struct tm *pTime;

    LOG_FUNCTION_NAME;


    if ((NO_ERROR == ret) && strcmp((char *)&(exif->exifmodel),"")!=0)
    {
        ret = exifTable->insertElement(TAG_MODEL, (char *)&(exif->exifmodel));
    }

    if ((NO_ERROR == ret) && strcmp((char *)&(exif->exifmake),"")!=0)
    {
        ret = exifTable->insertElement(TAG_MAKE, (char *)&(exif->exifmake));
    }

    if ((NO_ERROR == ret))
    {
        unsigned int numerator = 0, denominator = 0;
        numerator = exif->focalLengthH;
        denominator = exif->focalLengthL;
        if (numerator || denominator)
        {
            char temp_value[256]; // arbitrarily long string
            snprintf(temp_value,
                     sizeof(temp_value)/sizeof(char),
                     "%u/%u", numerator, denominator);
            ret = exifTable->insertElement(TAG_FOCALLENGTH, temp_value);

        }
    }

    if (NO_ERROR == ret)
    {
        ret = exifTable->insertElement(TAG_DATETIME, (char *)&exif->dateTime);
    }

    if (NO_ERROR == ret)
    {
        char temp_value[5];
        snprintf(temp_value, sizeof(temp_value)/sizeof(char), "%u", frame->mEncodeWidth);
        ret = exifTable->insertElement(TAG_IMAGE_WIDTH, temp_value);
    }

    if (NO_ERROR == ret)
    {
        char temp_value[5];
        snprintf(temp_value, sizeof(temp_value)/sizeof(char), "%u", frame->mEncodeHeight);
        ret = exifTable->insertElement(TAG_IMAGE_LENGTH, temp_value);
    }
    if(exif->bGPS)
    {

        if ((NO_ERROR == ret))
        {
            char temp_value[256]; // arbitrarily long string
            snprintf(temp_value,
                sizeof(temp_value)/sizeof(char) - 1,
                "%d/%d,%d/%d,%d/%d",
                exif->gpsLATH[0], exif->gpsLATL[0],
                exif->gpsLATH[1], exif->gpsLATL[1],
                exif->gpsLATH[2], exif->gpsLATL[2]);
            ret = exifTable->insertElement(TAG_GPS_LAT, temp_value);
            if(exif->gpsLATREF == 1)
            {
                ret = exifTable->insertElement(TAG_GPS_LAT_REF, GPS_SOUTH_REF);
            }
            else
            {
                ret = exifTable->insertElement(TAG_GPS_LAT_REF, GPS_NORTH_REF);
            }
        }

        if ((NO_ERROR == ret))
        {
            char temp_value[256]; // arbitrarily long string
            snprintf(temp_value,
                sizeof(temp_value)/sizeof(char) - 1,
                "%d/%d,%d/%d,%d/%d",
                exif->gpsLONGH[0], exif->gpsLONGL[0],
                exif->gpsLONGH[1], exif->gpsLONGL[1],
                exif->gpsLONGH[2], exif->gpsLONGL[2]);
            ret = exifTable->insertElement(TAG_GPS_LONG, temp_value);
            if(exif->gpsLONGREF == 1)
            {
                ret = exifTable->insertElement(TAG_GPS_LONG_REF, GPS_WEST_REF);
            }
            else
            {
                ret = exifTable->insertElement(TAG_GPS_LONG_REF, GPS_EAST_REF);
            }
        }

        if ((NO_ERROR == ret))
        {
            char temp_value[256]; // arbitrarily long string
            snprintf(temp_value,
                sizeof(temp_value)/sizeof(char) - 1,
                "%d/%d",
                exif->gpsALTH, exif->gpsALTL);
            ret = exifTable->insertElement(TAG_GPS_ALT, temp_value);
            if(exif->gpsALTREF == 1)
            {
                ret = exifTable->insertElement(TAG_GPS_ALT_REF, "1");
            }
            else
            {
                ret = exifTable->insertElement(TAG_GPS_ALT_REF, "0");
            }
        }

        if ((NO_ERROR == ret) && (strcmp((char *)&(exif->gpsProcessMethod),"")!=0))
        {
            ret = exifTable->insertElement(TAG_GPS_PROCESSING_METHOD, (char *)&(exif->gpsProcessMethod));
        }

        if (NO_ERROR == ret)
        {
            char temp_value[256]; // arbitrarily long string
            snprintf(temp_value,
                sizeof(temp_value)/sizeof(char) - 1,
                "%d/%d,%d/%d,%d/%d",
                exif->gpsTimeH[0], exif->gpsTimeL[0],
                exif->gpsTimeH[1], exif->gpsTimeL[1],
                exif->gpsTimeH[2], exif->gpsTimeL[2]);
            ret = exifTable->insertElement(TAG_GPS_TIMESTAMP, temp_value);
        }

        if (NO_ERROR == ret) 
        {
            ret = exifTable->insertElement(TAG_GPS_DATESTAMP, (char *)&(exif->gpsDatestamp));
        }
    }

    if (NO_ERROR == ret)
    {
       const char* exif_orient =
               ExifElementsTable::degreesToExifOrientation( exif->imageOri);
        ret = exifTable->insertElement(TAG_ORIENTATION, exif_orient);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 * OPTIMIZE:   Use vce resize to raise the conversion speed.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
void AppCallbackNotifier::resizeRawFrame(CameraFrame* frame, bool canceled)
{
    camera_memory_t* picture = NULL;
    void* dest = NULL, *src = NULL, *tmp = NULL;
    int index = -1;
    camera_memory_t* tmpbuf = NULL;
    int vBuf = 0;
    int size = 0;
    //FIXME: vce uv stride is not aligned to 16 (yuv420p), not compatible with android 
    bool align = true;

    LOG_FUNCTION_NAME;
    if(canceled)
    {
        CAMHAL_LOGDA("resize canceled");
        goto exit;
    }
    if(mNotifierState != AppCallbackNotifier::NOTIFIER_STARTED)
    {
        CAMHAL_LOGDA("Error!mNotifierState  is not in started state");
        goto exit;
    }

    if (!frame->mBuffer)
    {
        CAMHAL_LOGDA("Error! One of the buffer is NULL");
        goto exit;
    }

    if(frame->mFrameType == CameraFrame::VIDEO_FRAME_SYNC)
    {
    	/*
    	* SYNC_FROM: revert to 702x's buffer operation way, which is reasonable.
    	*ActionsCode(author:liyuan, change_code)
    	*/
        if(!mUseCameraHalVideoBuffers)
        {
            //ActionsCode(author:liuyiguang, change_code)
            //index = mVideoMemoryMap.valueFor(reinterpret_cast<uint32_t>(frame->mBuffer));
            index = mVideoMemoryMap.valueFor(reinterpret_cast<long>(frame->mBuffer));
            if(index >= (int)mVideoMemoryCount)
            {
                goto exit;
            }
      
            picture = mVideoMemory;

        }
        else
        {
            picture = NULL;
        }
    }
    else if(frame->mFrameType == CameraFrame::PREVIEW_RAW_FRAME_SYNC)
    {
        index = mPreviewMemoryIndex;
        mPreviewMemoryIndex = (mPreviewMemoryIndex+1)%mPreviewMemoryCount;
        picture = mPreviewMemory;
    }
    else
    {   
        size = frame->getEncodeFrameLength();
        picture = mRequestMemory(-1, size, 1, NULL);
        index = 0;
    }

    if (NULL != picture || (frame->mFrameType == CameraFrame::VIDEO_FRAME_SYNC && mUseCameraHalVideoBuffers ))
    {
    	/*
    	* SYNC_FROM: revert to 702x's buffer operation way, which is reasonable.
    	*ActionsCode(author:liyuan, change_code)
    	*/
        if(frame->mFrameType == CameraFrame::VIDEO_FRAME_SYNC)
        {
        	if(mUseCameraHalVideoBuffers)
		{
			vBuf = mVideoMap.valueFor((uint32_t) frame->mBuffer);
			GraphicBufferMapper &mapper = GraphicBufferMapper::get();
			Rect bounds;
			bounds.left = 0;
			bounds.top = 0;
			bounds.right = frame->mEncodeWidth;
			bounds.bottom = frame->mEncodeHeight;
					
			mapper.lock((buffer_handle_t)vBuf, CAMHAL_GRALLOC_VIDEO_LOCK_USAGE, bounds, &dest);

		}
		else
		{
#ifdef CAMERA_RUN_IN_EMULATOR
			tmpbuf = mRequestMemory(-1, (frame->mEncodeWidth*frame->mEncodeHeight*2), 1, NULL);
			dest = tmpbuf->data;
			tmp = reinterpret_cast<unsigned char *>(mVideoBufs[index]);
#else
			dest = reinterpret_cast<unsigned char *>(mVideoBufs[index]);				
#endif
		}
		

        }else if(frame->mFrameType == CameraFrame::PREVIEW_RAW_FRAME_SYNC){
            dest =  reinterpret_cast<unsigned char *>(mPreviewMemoryBufs[index]);
			
        }else{
            dest = picture->data;

        }

        if ( NULL != dest )
        {
            GraphicBufferMapper &mapper = GraphicBufferMapper::get();
            Rect bounds;
            bounds.left = 0;
            bounds.top = 0;
            bounds.right = frame->mOrigWidth;
            bounds.bottom = frame->mOrigHeight;

            void *vaddr;
            mapper.lock((buffer_handle_t)frame->mBuffer, CAMHAL_GRALLOC_LOCK_USAGE, bounds, &vaddr);
            //ActionsCode(author:liuyiguang, change_code)
            //src = (void *) ((unsigned int)vaddr + frame->mOffset);
            src = (void *) (vaddr + frame->mOffset);
            
            /*
            ALOGD("frame->mOrigWidth=%d",frame->mOrigWidth);
            ALOGD("frame->mOrigHeight=%d",frame->mOrigHeight);
            ALOGD("frame->mEncodeWidth=%d",frame->mEncodeWidth);
            ALOGD("frame->mEncodeHeight=%d",frame->mEncodeHeight);
            ALOGD("frame->mWidth=%d",frame->mWidth);
            ALOGD("frame->mHeight=%d",frame->mHeight);
            ALOGD("frame->mXOff=%d",frame->mXOff);
            ALOGD("frame->mYOff=%d",frame->mYOff);
            ALOGD("frame->mFormat=%d",frame->mFormat);
            ALOGD("frame->mConvFormat=%d",frame->mConvFormat);
            */
            if(frame->mFrameType == CameraFrame::VIDEO_FRAME_SYNC)
            {
                align = getHwVideoFormatAligned();
            }

            if( (frame->mWidth == frame->mEncodeWidth) 
                && (frame->mHeight == frame->mEncodeHeight)
                &&(frame->mOrigWidth == frame->mEncodeWidth) 
                && (frame->mOrigHeight == frame->mEncodeHeight)
                &&(frame->mXOff == 0)
                &&(frame->mYOff == 0)
                )
            {            
                cameraFrameConvert(*frame, reinterpret_cast<unsigned char *>(src), (unsigned char *)dest, frame->mLength, align);
            }
            else
            {
#ifdef CAMERA_VCE_OMX_RESIZE
                if(CameraFrame::ENCODE_WITH_SW & frame->mQuirks)
                {
                    cameraFrameResize(*frame, reinterpret_cast<unsigned char *>(src), reinterpret_cast<unsigned char *>(dest), align);
                }
                else
                {       
		    /*
    		    * SYNC_FROM: revert to 702x's buffer operation way, which is reasonable.
   		    *ActionsCode(author:liyuan, change_code)
   		    */
		    if(frame->mStreamType == CameraFrame::PREVIEW_STREAM_TYPE)
                    {
                         //vceResize(frame, (unsigned char *)src,(unsigned char *)dest,PRVIEW_RESIZE, align);
                         //ActionsCode(author:liuyiguang, add_code)
                         vceResizeImprove(frame, (unsigned char *)src,(unsigned char *)dest);
                    }
                    else
                    {
                         //vceResize(frame, (unsigned char *)src,(unsigned char *)dest,CAPTURE_RESIZE, align);
                         //ActionsCode(author:liuyiguang, add_code)
                         vceResizeImprove(frame, (unsigned char *)src,(unsigned char *)dest);
		    }
					
                }
#else
                cameraFrameResize(*frame, reinterpret_cast<unsigned char *>(src), reinterpret_cast<unsigned char *>(dest), align);
#endif
            }

#ifdef CAMERA_RUN_IN_EMULATOR
            if(frame->mFrameType == CameraFrame::VIDEO_FRAME_SYNC)
            {
                RGB565ToNV12( reinterpret_cast<unsigned char *>(dest), reinterpret_cast<unsigned char *>(tmp), frame->mEncodeWidth, frame->mEncodeHeight);
                tmpbuf->release(tmpbuf);
            }
#endif

            mapper.unlock((buffer_handle_t)frame->mBuffer);
        }
    }

exit:
    if ( NULL != frame && !canceled)
    {
        if(frame->mFrameType == CameraFrame::VIDEO_FRAME_SYNC && mUseCameraHalVideoBuffers )
        {
            if( vBuf != 0)
            {
                if(dest != NULL)
                {
                    //saveFile((unsigned char*)dest, frame->mEncodeWidth, frame->mEncodeHeight, 12);
                    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
                    //ActionsCode(author:liuyiguang, change_code)
                    //mapper.unlock((buffer_handle_t)vBuf);
                    mapper.unlock((buffer_handle_t)(long)vBuf);
                }


                sendToFrameQ(AppCallbackNotifier::NOTIFIER_CMD_PROCESS_FRAME, frame, NULL, NULL);
            }
        }
        else
        {
            //ActionsCode(author:liuyiguang, change_code)
            //sendToFrameQ(AppCallbackNotifier::NOTIFIER_CMD_PROCESS_MEMORY_FRAME, frame,picture,(void *)index);
            sendToFrameQ(AppCallbackNotifier::NOTIFIER_CMD_PROCESS_MEMORY_FRAME, frame,picture,(void *)(long)index);
        }
    }
    LOG_FUNCTION_NAME_EXIT;
    return;


}
camera_memory_t *AppCallbackNotifier::jpegEncodeSW(CameraFrame* frame)
{
    void *mainOutBuf = NULL;
    void *tnOutBuf = NULL;
    int encode_quality = 100, tn_quality = 100;
    int tn_width, tn_height;
    int size = 0;
    camera_memory_t* picture = NULL;

    void* exif_data = NULL;

    ExifElementsTable * exifTable = NULL;

    Encoder_libjpeg *encoder = NULL;

    void *src = NULL;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    Rect bounds;
    

    LOG_FUNCTION_NAME;

    CAMERA_SCOPEDTRACE("jpegEncodeSW");

    CameraParameters parameters;
    char *params = mCameraHal->getParameters();
    const String8 strParams(params);

    parameters.unflatten(strParams);

    encode_quality = parameters.getInt(CameraParameters::KEY_JPEG_QUALITY);
    if (encode_quality < 0 || encode_quality > 100)
    {
        encode_quality = 100;
    }

    tn_quality = parameters.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    if (tn_quality < 0 || tn_quality > 100)
    {
        tn_quality = 100;
    }

    tn_width = parameters.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    tn_height = parameters.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);

    if (params != NULL)
    {
        mCameraHal->putParameters(params);
    }

    if (((CameraFrame::HAS_EXIF_DATA|CameraFrame::HAS_EXIF_DATA_LIBJPEG) & frame->mQuirks) == (CameraFrame::HAS_EXIF_DATA|CameraFrame::HAS_EXIF_DATA_LIBJPEG))
    {
        exif_data = frame->mCookie2;
    }
    else
    {
        exif_data = frame->mCookie2;
        if(exif_data)
        {
            exifTable  = new ExifElementsTable();
            convertEXIF_libjpeg((CameraHalExif*)exif_data,exifTable ,frame);
            exif_data = exifTable;
        }
    }

    size = frame->getFrameLength((CameraFrame::FrameFormat)frame->mFormat, frame->mEncodeWidth, frame->mEncodeHeight);

    Encoder_libjpeg::params *main_jpeg = NULL, *tn_jpeg = NULL;

    mainOutBuf =malloc(size);
    if(mainOutBuf == NULL)
    {
        goto exit;
    }

    main_jpeg = (Encoder_libjpeg::params*)
        malloc(sizeof(Encoder_libjpeg::params));
    if(main_jpeg == NULL)
    {
        goto exit;
    }

    bounds.left = 0;
    bounds.top = 0;
    bounds.right = frame->mOrigWidth;
    bounds.bottom = frame->mOrigHeight;

    mapper.lock((buffer_handle_t)frame->mBuffer, CAMHAL_GRALLOC_LOCK_USAGE, bounds, &src);

    if (main_jpeg)
    {
        main_jpeg->src = (uint8_t*) src;
        main_jpeg->src_size = frame->mLength;
        main_jpeg->src_offset = frame->mOffset;
        main_jpeg->dst = (uint8_t*)mainOutBuf;
        main_jpeg->dst_size = size;
        main_jpeg->quality = encode_quality;
        main_jpeg->orig_width = frame->mOrigWidth;
        main_jpeg->orig_height = frame->mOrigHeight;
        main_jpeg->in_width = frame->mWidth; // use stride here
        main_jpeg->in_height = frame->mHeight;
        main_jpeg->out_width = frame->mEncodeWidth;
        main_jpeg->out_height = frame->mEncodeHeight;
        main_jpeg->left_crop = frame->mXOff;
        main_jpeg->top_crop = frame->mYOff;
        main_jpeg->start_offset = frame->mOffset;
        main_jpeg->format = frame->mFormat;
        main_jpeg->jpeg_size = 0;
    }
    CAMHAL_LOGVB("main_jpeg->orig_width= %d", main_jpeg->orig_width);
    CAMHAL_LOGVB("main_jpeg->orig_height= %d", main_jpeg->orig_height);
    CAMHAL_LOGVB("main_jpeg->left_crop= %d", main_jpeg->left_crop);
    CAMHAL_LOGVB("main_jpeg->top_crop= %d", main_jpeg->top_crop);
    CAMHAL_LOGVB("main_jpeg->in_width= %d", main_jpeg->in_width);
    CAMHAL_LOGVB("main_jpeg->in_height= %d", main_jpeg->in_height);
    CAMHAL_LOGVB("main_jpeg->out_width= %d", main_jpeg->out_width);
    CAMHAL_LOGVB("main_jpeg->out_height= %d", main_jpeg->out_height);
    CAMHAL_LOGVB("main_jpeg->format= %d", main_jpeg->format);
    CAMHAL_LOGVB("main_jpeg->quality= %d", main_jpeg->quality);

    if(exif_data)
    {
        int thumbsize = 0;

        thumbsize = CameraFrame::getFrameLength((CameraFrame::FrameFormat)frame->mFormat, tn_width, tn_height);

        if ((tn_width > 0) && (tn_height > 0))
        {
            tn_jpeg = (Encoder_libjpeg::params*)
                malloc(sizeof(Encoder_libjpeg::params));
            // if malloc fails just keep going and encode main jpeg
            if (!tn_jpeg)
            {
                goto exit;
            }

            tnOutBuf =  malloc(thumbsize);
            if(tnOutBuf == NULL)
            {
                goto exit;
            } 
        }
        else
        {
            CAMHAL_LOGEB("tn_width = %d,tn_height=%d", tn_width,tn_height);
        }

        if (tn_jpeg)
        {
            tn_jpeg->src = (uint8_t*) src;
            tn_jpeg->src_size = frame->mLength;
            tn_jpeg->src_offset = frame->mOffset;
            tn_jpeg->dst = (uint8_t*)tnOutBuf;
            tn_jpeg->dst_size = thumbsize;
            tn_jpeg->quality = encode_quality;
            tn_jpeg->orig_width = frame->mOrigWidth;
            tn_jpeg->orig_height = frame->mOrigHeight;
            tn_jpeg->in_width = frame->mWidth; // use stride here
            tn_jpeg->in_height = frame->mHeight;
            tn_jpeg->out_width = tn_width;
            tn_jpeg->out_height = tn_height;
            tn_jpeg->left_crop = frame->mXOff;
            tn_jpeg->top_crop = frame->mYOff;
            tn_jpeg->start_offset = frame->mOffset;
            tn_jpeg->format = frame->mFormat;
            tn_jpeg->jpeg_size = 0;

        }
    }

    encoder = new Encoder_libjpeg(main_jpeg,
        tn_jpeg,
        exif_data);

    encoder->encode();
    if( main_jpeg->jpeg_size <= 0)
    {
        errorNotify(CAMERA_ERROR_HARD);
        goto exit;
    }

    if(exif_data && tn_jpeg && tn_jpeg->jpeg_size > 0)
    {
        ExifElementsTable* exif = (ExifElementsTable*) exif_data;
        Section_t* exif_section = NULL;

        exif->insertExifToJpeg((unsigned char*) main_jpeg->dst, main_jpeg->jpeg_size);

        if(tn_jpeg)
        {
            exif->insertExifThumbnailImage((const char*)tn_jpeg->dst,
                (int)tn_jpeg->jpeg_size);
        }

        exif_section = FindSection(M_EXIF);

        if (exif_section)
        {
            picture = mRequestMemory(-1, main_jpeg->jpeg_size + exif_section->Size, 1, NULL);
            if (picture && picture->data)
            {
                exif->saveJpeg((unsigned char*) picture->data, main_jpeg->jpeg_size + exif_section->Size);
            }
        }
    }    

    if(picture == NULL)
    {
         picture = mRequestMemory(-1, main_jpeg->jpeg_size, 1, NULL);

         if (picture && picture->data)
         {
             memcpy(picture->data, main_jpeg->dst, main_jpeg->jpeg_size);
         }
    }

exit:

    if(src != NULL)
    {
        mapper.unlock((buffer_handle_t)frame->mBuffer);
    }

    if(mainOutBuf)
    {
        free(mainOutBuf);
    }

    if(main_jpeg)
    {
        free(main_jpeg);
    }
    if (tn_jpeg)
    {
        free(tn_jpeg);
    }

    if(tnOutBuf)
    {
        free(tnOutBuf);
    }   

    if(encoder)
    {
        delete encoder;
    }

    if(exifTable)
    {
        delete exifTable;
    }

    LOG_FUNCTION_NAME_EXIT;

    return picture;

}
camera_memory_t *AppCallbackNotifier::jpegEncodeHW(CameraFrame* frame)
{
#ifdef CAMERA_VCE_OMX_JPEG
    int encode_quality = 100, tn_quality = 100;
    int tn_width, tn_height;
    camera_memory_t* picture = NULL;

    unsigned int i;
    ImageJpegEncoderParam *vceJpegParam = NULL;

    EncoderJpegVce *encoder = NULL;

    void* exif_data = NULL;

    LOG_FUNCTION_NAME;

    CAMERA_SCOPEDTRACE("jpegEncodeHW");
    CameraParameters parameters;
    char *params = mCameraHal->getParameters();
    const String8 strParams(params);
    parameters.unflatten(strParams);

    encode_quality = parameters.getInt(CameraParameters::KEY_JPEG_QUALITY);
    if (encode_quality < 0 || encode_quality > 100)
    {
        encode_quality = 100;
    }

    tn_quality = parameters.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    if (tn_quality < 0 || tn_quality > 100)
    {
        tn_quality = 100;
    }

    tn_width = parameters.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    tn_height = parameters.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);

    if (params != NULL)
    {
        mCameraHal->putParameters(params);
    }
    if (((CameraFrame::HAS_EXIF_DATA|CameraFrame::HAS_EXIF_DATA_VCE) & frame->mQuirks) == (CameraFrame::HAS_EXIF_DATA|CameraFrame::HAS_EXIF_DATA_VCE))
    {
        exif_data = frame->mCookie2;
    }  

    vceJpegParam = (ImageJpegEncoderParam*)
        malloc(sizeof(ImageJpegEncoderParam));
    if(vceJpegParam == NULL)
    {
        goto exit;
    }

    CAMHAL_LOGVB("mImageBufCount= %d", mImageBufCount);

    if(frame->mStreamType == CameraFrame::IMAGE_STREAM_TYPE)
    {
        for(i = 0; i < mImageBufCount; i++)
        {
            vceJpegParam->buffers[i]=mImageBufs[i];

            CAMHAL_LOGVB("vceJpegParam->buffers[%d]=%p ", i, mImageBufs[i]);
        }
        vceJpegParam->buffersCount = mImageBufCount;
        vceJpegParam->bufferSize= mImageBufSize;
    }
    else if(frame->mStreamType == CameraFrame::PREVIEW_STREAM_TYPE)
    {
        for(i = 0; i < mPreviewBufCount; i++)
        {
            vceJpegParam->buffers[i]=mPreviewBufs[i];

            CAMHAL_LOGVB("vceJpegParam->buffers[%d]=%p ", i, mImageBufs[i]);
        }
        vceJpegParam->buffersCount = mPreviewBufCount;
        vceJpegParam->bufferSize= mPreviewBufSize;
    }
    vceJpegParam->inData= frame->mBuffer;
    vceJpegParam->outData = NULL;
    vceJpegParam->outDataSize=0;
    vceJpegParam->origWidth=frame->mOrigWidth;
    vceJpegParam->origHeight=frame->mOrigHeight;
    vceJpegParam->outputWidth=frame->mEncodeWidth;
    vceJpegParam->outputHeight=frame->mEncodeHeight;
    vceJpegParam->xoff=frame->mXOff;
    vceJpegParam->yoff=frame->mYOff;
    vceJpegParam->width=frame->mWidth;
    vceJpegParam->height=frame->mHeight;
    vceJpegParam->quality=encode_quality;
    vceJpegParam->thumbWidth=tn_width;
    vceJpegParam->thumbHeight=tn_height;
    
    CAMHAL_LOGVB("vceJpegParam->origWidth= %d", vceJpegParam->origWidth);
    CAMHAL_LOGVB("vceJpegParam->origHeight= %d", vceJpegParam->origHeight);
    CAMHAL_LOGVB("vceJpegParam->xoff= %d", vceJpegParam->xoff);
    CAMHAL_LOGVB("vceJpegParam->yoff= %d", vceJpegParam->yoff);
    CAMHAL_LOGVB("vceJpegParam->width= %d", vceJpegParam->width);
    CAMHAL_LOGVB("vceJpegParam->height= %d", vceJpegParam->height);
    CAMHAL_LOGVB("vceJpegParam->outputWidth= %d", vceJpegParam->outputWidth);
    CAMHAL_LOGVB("vceJpegParam->outputHeight= %d", vceJpegParam->outputHeight);
    
    
    if(frame->mFormat == CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12)
    {
        vceJpegParam->format = OMX_COLOR_FormatYUV420Planar;
    }
    else if(frame->mFormat == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12)
    {
        vceJpegParam->format = OMX_COLOR_FormatYUV420SemiPlanar;
    }
    if(frame->mFormat == CameraFrame::CAMERA_FRAME_FORMAT_YUV420P)
    {
        vceJpegParam->format = OMX_COLOR_FormatYVU420Planar;
    }
    else if(frame->mFormat == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP)
    {
        vceJpegParam->format = OMX_COLOR_FormatYVU420SemiPlanar;
    }
    CAMHAL_LOGVB("vceJpegParam->format= %d", vceJpegParam->format);
    vceJpegParam->coding = OMX_VIDEO_CodingMJPEG;
    CAMHAL_LOGVA("VCE  JPEG ENCODER!!!!");
    encoder = new EncoderJpegVce(vceJpegParam,
        (CameraHalExif*)exif_data,
        mVceJpegEncoder);
    encoder->encode();
    if(vceJpegParam->outDataSize > 0)
    {
        picture = mRequestMemory(-1, vceJpegParam->outDataSize, 1, NULL);

        if (picture && picture->data)
        {
            memcpy(picture->data, (unsigned char *)vceJpegParam->outData+vceJpegParam->outDataOffset, vceJpegParam->outDataSize);
        }
    }
    else
    {
        errorNotify(CAMERA_ERROR_HARD);
    }

exit:
    if(vceJpegParam)
    {
        free(vceJpegParam);
    }
    if(encoder)
    {
        delete encoder;
    }

    LOG_FUNCTION_NAME_EXIT;
    return picture;
#else
    return jpegEncodeSW(frame);
#endif
}

status_t AppCallbackNotifier::dummyRaw()
{
    LOG_FUNCTION_NAME;

    if ( NULL == mRequestMemory )
    {
        CAMHAL_LOGEA("Can't allocate memory for dummy raw callback!");
        return NO_INIT;
    }

    if ( ( NULL != mCameraHal ) &&
            ( NULL != mDataCb) &&
            ( NULL != mNotifyCb ) )
    {

        if ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_RAW_IMAGE) )
        {
            camera_memory_t *dummyRaw = mRequestMemory(-1, 1, 1, NULL);

            if ( NULL == dummyRaw )
            {
                CAMHAL_LOGEA("Dummy raw buffer allocation failed!");
                return NO_MEMORY;
            }

            mDataCb(CAMERA_MSG_RAW_IMAGE, dummyRaw, 0, NULL, mCallbackCookie);

            dummyRaw->release(dummyRaw);
        }
        else if ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_RAW_IMAGE_NOTIFY) )
        {
            mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
void AppCallbackNotifier::notifyFrame()
{
    ///Receive and send the frame notifications to app
    ActUtils::Message msg;
    CameraFrame *frame;
    MemoryHeapBase *heap;
    MemoryBase *buffer = NULL;
    sp<MemoryBase> memBase;
    void *buf = NULL;
    bool bFreeFrame = true;

    LOG_FUNCTION_NAME;

    {
        Mutex::Autolock lock(mFrameQLock);
        if(!mFrameQ.isEmpty())
        {
            mFrameQ.get(&msg);
        }
        else
        {
            return;
        }
    }

    frame = NULL;
    switch(msg.command)
    {
    case AppCallbackNotifier::NOTIFIER_CMD_PROCESS_FRAME:

        frame = (CameraFrame *) msg.arg1;
        if(!frame)
        {
            break;
        }
        
        if ( ( CameraFrame::VIDEO_FRAME_SYNC == frame->mFrameType ) &&
                  ( NULL != mCameraHal ) &&
                  ( NULL != mDataCbTimestamp) &&
                  ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_VIDEO_FRAME)  ) )
        {
            
            if(mRecording)
            {
                if(mUseMetaDataBufferMode)
                {
                    //ActionsCode(author:liuyiguang, change_code)
                    //camera_memory_t *videoMedatadaBufferMemory =
                    //    (camera_memory_t *) mVideoMetadataBufferMemoryMap.valueFor((uint32_t) frame->mBuffer);
                    camera_memory_t *videoMedatadaBufferMemory =
                        (camera_memory_t *) (long)mVideoMetadataBufferMemoryMap.valueFor((long) frame->mBuffer);
                    video_metadata_t *videoMetadataBuffer = (video_metadata_t *) videoMedatadaBufferMemory->data;

                    if( (NULL == videoMedatadaBufferMemory) || (NULL == videoMetadataBuffer) || (NULL == frame->mBuffer) )
                    {
                        CAMHAL_LOGEA("Error! One of the video buffers is NULL");
                        break;
                    }
                    {
                        videoMetadataBuffer->metadataBufferType = (int) kMetadataBufferTypeCameraSource;
                        if(mUseCameraHalVideoBuffers)
                        {
                            //ActionsCode(author:liuyiguang, change_code)
                            //videoMetadataBuffer->handle = (void*)mVideoMap.valueFor((uint32_t) frame->mBuffer);
                            videoMetadataBuffer->handle = (void*)(long)mVideoMap.valueFor((long) frame->mBuffer);
                            videoMetadataBuffer->off_x = 0;
                            videoMetadataBuffer->off_y = 0;
                            videoMetadataBuffer->crop_w= frame->mEncodeWidth;
                            videoMetadataBuffer->crop_h= frame->mEncodeHeight;
                        }
                        else
                        {
                            videoMetadataBuffer->handle = frame->mBuffer;
                            videoMetadataBuffer->off_x = frame->mXOff;
                            videoMetadataBuffer->off_y = frame->mYOff;
                            videoMetadataBuffer->crop_w= frame->mWidth;
                            videoMetadataBuffer->crop_h= frame->mHeight;
                        }
                        /*
                        ALOGD("send frame->mXOff=%d",videoMetadataBuffer->off_x);
                        ALOGD("send frame->mYOff=%d",videoMetadataBuffer->off_y);
                        ALOGD("send frame->mWidth=%d",videoMetadataBuffer->crop_w);
                        ALOGD("send frame->mHeight=%d",videoMetadataBuffer->crop_h);
                        */
                    }

                    CAMHAL_LOGVB("mDataCbTimestamp : frame->mBuffer=%p, videoMetadataBuffer=%p, videoMedatadaBufferMemory=%p",
                                 frame->mBuffer, videoMetadataBuffer, videoMedatadaBufferMemory);

                    if(mRecording)
                    {
                        mDataCbTimestamp(frame->mTimestamp, CAMERA_MSG_VIDEO_FRAME,
                            videoMedatadaBufferMemory, 0, mCallbackCookie);
                    }
                }
               
            }
        }

        break;

    case AppCallbackNotifier::NOTIFIER_CMD_PROCESS_MEMORY_FRAME:
        {
            camera_memory_t * memory = NULL ;
            frame = (CameraFrame *) msg.arg1;
            sp<CameraMemoryAllocater> memoryMgt = NULL;
            bool bFreeMemory = true;
            bool bReturnFrame = true;
            if(!frame)
            {
                break;
            }
            
            if( CameraFrame::VIDEO_FRAME_SYNC == frame->mFrameType || CameraFrame::PREVIEW_RAW_FRAME_SYNC == frame->mFrameType)
            {
                bFreeMemory = false;
            }

            memory = (camera_memory_t *)msg.arg2;
            if(memory == NULL)
            {
                goto return_frame;
            }

            if ( (CameraFrame::RAW_FRAME_SYNC == frame->mFrameType )&&
                ( NULL != mCameraHal ) &&
                ( NULL != mDataCb))
            {
                if ((mNotifierState == AppCallbackNotifier::NOTIFIER_STARTED) && mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_RAW_IMAGE) )
                {
                    //send raw frame
                    CAMERA_SCOPEDTRACE("CAMERA_MSG_RAW_IMAGE");
                    mDataCb(CAMERA_MSG_RAW_IMAGE,memory,0, NULL, mCallbackCookie);
                }
            }
            else if ( ( CameraFrame::IMAGE_FRAME_SYNC == frame->mFrameType ) &&
                ( NULL != mCameraHal ) &&
                ( NULL != mDataCb) &&
                (mNotifierState == AppCallbackNotifier::NOTIFIER_STARTED) )
            {
                CAMERA_SCOPEDTRACE("CAMERA_MSG_COMPRESSED_IMAGE");
                mDataCb(CAMERA_MSG_COMPRESSED_IMAGE,memory,0, NULL, mCallbackCookie);
                
            }
            else if ( ( CameraFrame::PREVIEW_RAW_FRAME_SYNC== frame->mFrameType ) &&
                ( NULL != mCameraHal ) &&
                ( NULL != mDataCb) &&
                ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_PREVIEW_FRAME)) &&
                (mNotifierState == AppCallbackNotifier::NOTIFIER_STARTED) )
            {
                //ActionsCode(author:liuyiguang, change_code)
                //int index = (int)msg.arg3;
                int index = (long)msg.arg3;
                CAMERA_SCOPEDTRACE("CAMERA_MSG_PREVIEW_FRAME");
                mDataCb(CAMERA_MSG_PREVIEW_FRAME,memory,index, NULL, mCallbackCookie);
            }
            else if ( ( CameraFrame::VIDEO_FRAME_SYNC == frame->mFrameType ) &&
                ( NULL != mCameraHal ) &&
                ( NULL != mDataCbTimestamp) &&
                ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_VIDEO_FRAME)  ) )
            {

                if(mRecording && !mUseMetaDataBufferMode)
                {
                    //ActionsCode(author:liuyiguang, change_code)
                    //int index = (int)msg.arg3;
                    int index = (long)msg.arg3;
                    CAMERA_SCOPEDTRACE("CAMERA_MSG_VIDEO_FRAME");
                    mDataCbTimestamp(frame->mTimestamp, CAMERA_MSG_VIDEO_FRAME, memory, index, mCallbackCookie);
                    //bReturnFrame = false;
                }
            }
return_frame:
            if(memory == NULL && CameraFrame::IMAGE_FRAME_SYNC == frame->mFrameType)
            {
            }
            if(memory && bFreeMemory)
            {
                memory->release(memory);
            }
            if(bReturnFrame)
            {
                mFrameProvider->returnFrame(frame->mBuffer, (CameraFrame::FrameType) frame->mFrameType, frame->mStreamType);
            }
        }
        break;

    default:

        break;

    };

exit:

    if ( bFreeFrame && NULL != frame )
    {
        delete frame;
    }

    LOG_FUNCTION_NAME_EXIT;
}

void AppCallbackNotifier::sendToFrameQ(int cmd, CameraFrame* frame, void *arg2, void * arg3)
{
    LOG_FUNCTION_NAME 

    if ( NULL != frame )
    {
        ActUtils::Message msg;
        msg.command = cmd;
        msg.arg1 = frame;
        msg.arg2 = arg2;
        msg.arg3 = arg3;
        Mutex::Autolock lock(mFrameQLock);
        mFrameQ.put(&msg);
    }  

    LOG_FUNCTION_NAME_EXIT;
} 

void AppCallbackNotifier::sendToResize(CameraFrame* frame)
{
    LOG_FUNCTION_NAME 

    if ( NULL != frame )
    {
        ActUtils::Message msg;
        msg.command = AppCallbackNotifier::RESIZE_CMD_RESIZE;
        msg.arg1 = frame;
        msg.arg2 = 0;
        msg.arg3 = 0;
        Mutex::Autolock lock(mResizeQLock);
        mResizeQ.put(&msg);
    }  

    LOG_FUNCTION_NAME_EXIT;
}
void AppCallbackNotifier::sendToJpegEncode(CameraFrame* frame)
{
    LOG_FUNCTION_NAME 

    if ( NULL != frame )
    {
        ActUtils::Message msg;
        msg.command = AppCallbackNotifier::JPEG_CMD_ENCODE;
        msg.arg1 = frame;
        msg.arg2 = 0;
        msg.arg3 = 0;
        Mutex::Autolock lock(mJpegQLock);
        mJpegQ.put(&msg);
    }  

    LOG_FUNCTION_NAME_EXIT;
}

void AppCallbackNotifier::sendToEventQ(CameraHalEvent* evt)
{
    LOG_FUNCTION_NAME; 
    if ( NULL != evt )
    {
        ActUtils::Message msg;
        msg.command = AppCallbackNotifier::NOTIFIER_CMD_PROCESS_EVENT;
        msg.arg1 = evt;
        {
            Mutex::Autolock lock(mEventQLock);
            mEventQ.put(&msg);
        }
    }
    LOG_FUNCTION_NAME_EXIT;
}

void AppCallbackNotifier::flushJpegQ()
{
    CameraFrame *frame;
    ActUtils::Message msg;
    {
        Mutex::Autolock lock(mJpegQLock);
        while(!mJpegQ.isEmpty())
        {
            mJpegQ.get(&msg);
            frame = (CameraFrame *) msg.arg1;
            if(frame != NULL)
            {
                delete frame;
            }
        }
    }
}

void AppCallbackNotifier::flushResizeQ()
{
    CameraFrame *frame;
    ActUtils::Message msg;
    {
        Mutex::Autolock lock(mResizeQLock);
        while(!mResizeQ.isEmpty())
        {
            mResizeQ.get(&msg);
            frame = (CameraFrame *) msg.arg1;
            if(frame != NULL)
            {
                delete frame;
            }
        }
    }
}

void AppCallbackNotifier::flushResizeQForFrame(CameraFrame::FrameType frametype)
{
    CameraFrame *frame;
    ActUtils::Message msg;
    Vector< CameraFrame* > reservedframe;

    {
        Mutex::Autolock lock(mResizeQLock);
        while(!mResizeQ.isEmpty())
        {
            mResizeQ.get(&msg);
            frame = (CameraFrame *) msg.arg1;
            if(frame != NULL)
            {
                if(frame->mFrameType == frametype)
                {
                    delete frame;
                }
                else
                {
                    reservedframe.add(frame);
                }
            }
        }

        while(!reservedframe.isEmpty())
        {
            frame = reservedframe.itemAt(0);
            reservedframe.removeAt(0);
            ActUtils::Message msg;
            msg.command = AppCallbackNotifier::RESIZE_CMD_RESIZE;
            msg.arg1 = frame;
            msg.arg2 = 0;
            msg.arg3 = 0;
            mResizeQ.put(&msg);
        }
    }
}
void AppCallbackNotifier::flushResizeQForPreview()
{
    flushResizeQForFrame(CameraFrame::PREVIEW_RAW_FRAME_SYNC);
}

void AppCallbackNotifier::flushResizeQForVideo()
{
    flushResizeQForFrame(CameraFrame::VIDEO_FRAME_SYNC);
}
void AppCallbackNotifier::flushResizeQForImage()
{
    flushResizeQForFrame(CameraFrame::IMAGE_FRAME_SYNC);   
    flushResizeQForFrame(CameraFrame::IMAGE_FRAME_SYNC);
}

void AppCallbackNotifier::jpegEncode(JpegThread *thread)
{
    ActUtils::Message msg;
    CameraFrame *frame = NULL;

    LOG_FUNCTION_NAME 
    {
        Mutex::Autolock lock(mJpegQLock);
        if(!mJpegQ.isEmpty())
        {
            mJpegQ.get(&msg);
        }
        else
        {
            return;
        }
    }

    frame = (CameraFrame *) msg.arg1;
    if(frame == NULL)
    {
        return;
    }
    if(thread->mPictureValid)
    {
        encodePictureFrame(frame, !thread->mPictureValid);
    }
    else
    {
        delete frame;
    }


    LOG_FUNCTION_NAME_EXIT;
}
void AppCallbackNotifier::resizeFrame(ResizeThread *thread)
{
    ActUtils::Message msg;
    CameraFrame *frame = NULL;
    bool canceled = false;

    LOG_FUNCTION_NAME 
    {
        Mutex::Autolock lock(mResizeQLock);
        if(!mResizeQ.isEmpty())
        {
            mResizeQ.get(&msg);
        }
        else
        {
            return;
        }
    }
    frame = (CameraFrame *) msg.arg1;
    if(frame == NULL)
    {
        return;
    }
    if(frame->mFrameType == CameraFrame::PREVIEW_RAW_FRAME_SYNC)
    {
        canceled = !thread->mPreviewValid;
    }
    else if(frame->mFrameType == CameraFrame::VIDEO_FRAME_SYNC)
    {
        canceled = !thread->mVideoValid;
    }
    else
    {
        canceled = !thread->mPictureValid;
    }
    if(!canceled)
    {
        resizeRawFrame(frame, canceled);
    }
    else
    {
        delete frame;
    }

    LOG_FUNCTION_NAME_EXIT;
}


void AppCallbackNotifier::frameCallbackRelay(CameraFrame* caFrame)
{
    LOG_FUNCTION_NAME;
    AppCallbackNotifier *appcbn = (AppCallbackNotifier*) (caFrame->mCookie);
    appcbn->frameCallback(caFrame);
    LOG_FUNCTION_NAME_EXIT;
}

/**
 *
 * MERGEFIX:  Relax the solution of encode.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
void AppCallbackNotifier::frameCallback(CameraFrame* caFrame)
{
    ///Post the event to the event queue of AppCallbackNotifier
    ActUtils::Message msg;
    CameraFrame *frame = NULL;
    bool bFreeFrame = false;

    LOG_FUNCTION_NAME;


    if ( NULL != caFrame )
    {

        frame = new CameraFrame(*caFrame);

        if(CameraFrame::VIDEO_FRAME_SYNC == frame->mFrameType)
        {
            frame->mConvFormat = mVideoFormat;
            frame->mEncodeWidth = mVideoWidth;
            frame->mEncodeHeight = mVideoHeight;
            /*
            ALOGD("frame->mEncodeWidth=%d",frame->mEncodeWidth);
            ALOGD("frame->mEncodeHeight=%d",frame->mEncodeHeight);
            ALOGD("frame->mWidth=%d",frame->mWidth);
            ALOGD("frame->mHeight=%d",frame->mHeight);
            ALOGD("frame->mOrigWidth=%d",frame->mOrigWidth);
            ALOGD("frame->mOrigHeight=%d",frame->mOrigHeight);
            */
        }
        else if(CameraFrame::IMAGE_FRAME_SYNC == frame->mFrameType)
        {
            frame->mConvFormat = frame->mFormat;
        }
        else
        {
            frame->mConvFormat = mPreviewFormat;
        }
		
        if(CameraFrame::VIDEO_FRAME_SYNC == frame->mFrameType
            || CameraFrame::PREVIEW_RAW_FRAME_SYNC == frame->mFrameType
            || CameraFrame::RAW_FRAME_SYNC == frame->mFrameType)
        {
            CAMHAL_LOGDB(">>frame->mWidth %d frame->mEncodeWidth %d",frame->mWidth,frame->mEncodeWidth);
	    CAMHAL_LOGDB(">>frame->mHeight %d frame->mEncodeHeight %d",frame->mHeight,frame->mEncodeHeight);
	    CAMHAL_LOGDB(">>frame->mFormat %d frame->mConvFormat %d",frame->mWidth,frame->mEncodeWidth);
         
            /**
    	    * BUGFIX: consider omxvce use frame->mOrigWidth for stride, and the frame->mStride value is for 
	    * hal internal use, so check the align of mOrigWidth instead.
    	    *ActionsCode(author:liyuan, change_code)
    	    */
	    if(!isHwZoomSupport(frame->mWidth, frame->mHeight, frame->mFormat,
                frame->mEncodeWidth, frame->mEncodeHeight, frame->mConvFormat, frame->mOrigWidth))
            {
                frame->mQuirks |= CameraFrame::ENCODE_WITH_SW;
		CAMHAL_LOGDA(">>CameraFrame::VIDEO_FRAME_SYNC S");
            }
        
	}else if(CameraFrame::IMAGE_FRAME_SYNC == frame->mFrameType){
	    /**
            * BUGFIX: consider omxvce use frame->mOrigWidth for stride, and the frame->mStride value is for
            * hal internal use, so check the align of mOrigWidth instead.
            *ActionsCode(author:liyuan, change_code)
            */		
	    if(!isHwJpegEncodeSupport(frame->mWidth, frame->mHeight, frame->mFormat,
                frame->mEncodeWidth, frame->mEncodeHeight, frame->mConvFormat, frame->mOrigWidth))
            {
                frame->mQuirks |= CameraFrame::ENCODE_WITH_SW;
		CAMHAL_LOGDA(">>CameraFrame::IMAGE_FRAME_SYNC S");
            }

        }
	CAMHAL_LOGDB(">>frame->mQuirks = %s",frame->mQuirks&CameraFrame::ENCODE_WITH_SW?"ENCODE_WITH_SW":"ENCODE_WITH_HW");

        if(!frame)
        {
            goto exit;
        }
        CAMHAL_LOGDB("frame->mFrameType = %d",frame->mFrameType);

        if ( (CameraFrame::RAW_FRAME_SYNC == frame->mFrameType )&&
                  ( NULL != mCameraHal ) &&
                  ( NULL != mDataCb) &&
                  ( NULL != mNotifyCb ) )
        {

            if ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_RAW_IMAGE) )
            {
                sendToResize(frame);
            }
            else
            {
                if ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_RAW_IMAGE_NOTIFY) )
                {
                    CameraHalEvent *event;
                    event = new CameraHalEvent();
                    
                    event->mEventType = CameraHalEvent::EVENT_RAW_IMAGE_NOTIFY;

                    sendToEventQ(event);//--
	            CAMHAL_LOGDA(">>CameraFrame::RAW_FRAME_SYNC=>sendToEventQ");
                }
                bFreeFrame = true;
                mFrameProvider->returnFrame(frame->mBuffer,
                                            (CameraFrame::FrameType) frame->mFrameType, frame->mStreamType);
            }
        }
        else if ( (CameraFrame::IMAGE_FRAME_SYNC == frame->mFrameType) &&
                (NULL != mCameraHal) &&
                (NULL != mDataCb) &&
                (CameraFrame::ENCODE_RAW_TO_JPEG & frame->mQuirks) )
        {
	    CAMHAL_LOGDA(">>CameraFrame::IMAGE_FRAME_SYNC=>sendToJpegEncode");
            sendToJpegEncode(frame);//--
        }
        else if ( ( CameraFrame::IMAGE_FRAME_SYNC == frame->mFrameType ) &&
                  ( NULL != mCameraHal ) &&
                  ( NULL != mDataCb) )
        {
            if(frame->mFormat != CameraFrame::CAMERA_FRAME_FORMAT_JPEG && frame->mFormat != CameraFrame::CAMERA_FRAME_FORMAT_PNG)
            {
                sendToResize(frame);
            }
            else
            {
                sendToJpegEncode(frame);
            }
        }
        else if ( ( CameraFrame::VIDEO_FRAME_SYNC == frame->mFrameType ) &&
                  ( NULL != mCameraHal ) &&
                  ( NULL != mDataCbTimestamp) &&
                  ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_VIDEO_FRAME)  ) )
        {
            
            if(mRecording)
            {
                nsecs_t timestamp; 

                if ( mFirstRecordingFrame)
                {
                    mTimeStampDelta = frame->mTimestamp  - systemTime(SYSTEM_TIME_MONOTONIC)/1000;
                    mFirstRecordingFrame = false;

                }

                timestamp  = frame->mTimestamp - mTimeStampDelta;
                
                frame->mTimestamp = timestamp*1000;

		/*
		* Condition2: MetaDataBufferMode used and no extra ion videobuf is needed
		* ActionsCode(author:liyuan, add comments)
		*/
                if(mUseMetaDataBufferMode && !mUseCameraHalVideoBuffers)
                {
                    sendToFrameQ(AppCallbackNotifier::NOTIFIER_CMD_PROCESS_FRAME, frame,NULL,NULL);
                }
                else
                {
                /*
		* Condition1: MetaDataBufferMode used and extra ion videobuf is needed
		* Condition3: Non MetaDataBufferMode used 
		* ActionsCode(author:liyuan, add comments)
		*/
                    sendToResize(frame);
                }
            }

        }
        else if ( ( CameraFrame::PREVIEW_RAW_FRAME_SYNC== frame->mFrameType ) &&
                  ( NULL != mCameraHal ) &&
                  ( NULL != mDataCb) &&
                  ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_PREVIEW_FRAME)) )
        {
	    /*
            * SYNC_FROM: revert to 702x's buffer operation way, which is reasonable.
            *ActionsCode(author:liyuan, change_code)
            */
            sendToResize(frame);

        }
        else
        {
            mFrameProvider->returnFrame(frame->mBuffer,
                                        (CameraFrame::FrameType) frame->mFrameType, frame->mStreamType);            
            bFreeFrame = true;

        }

    }

exit:
    if(frame && bFreeFrame)
    {
        delete frame;
    }
    LOG_FUNCTION_NAME_EXIT;
}

void AppCallbackNotifier::flushFrames()
{
    ActUtils::Message msg;
    CameraFrame *frame;

    Mutex::Autolock lock(mFrameQLock);
    while (!mFrameQ.isEmpty())
    {
        mFrameQ.get(&msg);
        frame = (CameraFrame*) msg.arg1;
        if (frame)
        {
            delete frame;
        }
    }

    LOG_FUNCTION_NAME_EXIT;
}

void AppCallbackNotifier::flushAndReturnFrames()
{
    ActUtils::Message msg;
    CameraFrame *frame;

    Mutex::Autolock lock(mFrameQLock);
    while (!mFrameQ.isEmpty())
    {
        mFrameQ.get(&msg);
        frame = (CameraFrame*) msg.arg1;
        if (frame)
        {
            mFrameProvider->returnFrame(frame->mBuffer,
                                        (CameraFrame::FrameType) frame->mFrameType, frame->mStreamType);
            delete frame;
        }
    }

    LOG_FUNCTION_NAME_EXIT;
}

void AppCallbackNotifier::eventCallbackRelay(CameraHalEvent* chEvt)
{
    LOG_FUNCTION_NAME;
    AppCallbackNotifier *appcbn = (AppCallbackNotifier*) (chEvt->mCookie);
    appcbn->eventCallback(chEvt);
    LOG_FUNCTION_NAME_EXIT;
}

void AppCallbackNotifier::eventCallback(CameraHalEvent* chEvt)
{

    ///Post the event to the event queue of AppCallbackNotifier
    ActUtils::Message msg;
    CameraHalEvent *event;


    LOG_FUNCTION_NAME;

    if ( NULL != chEvt )
    {

        event = new CameraHalEvent(*chEvt);
        if ( NULL != event )
        {
            msg.command = AppCallbackNotifier::NOTIFIER_CMD_PROCESS_EVENT;
            msg.arg1 = event;
            {
                Mutex::Autolock lock(mEventQLock);
                mEventQ.put(&msg);
            }
        }
        else
        {
            CAMHAL_LOGEA("Not enough resources to allocate CameraHalEvent");
        }

    }

    LOG_FUNCTION_NAME_EXIT;
}


void AppCallbackNotifier::flushEventQueue()
{

    {
        Mutex::Autolock lock(mEventQLock);
        mEventQ.clear();
    }
}


bool AppCallbackNotifier::processNotificationMessage()
{
    ///Retrieve the command from the command queue and process it
    ActUtils::Message msg;

    LOG_FUNCTION_NAME;

    CAMHAL_LOGDA("+Msg get...");
    mNotificationThread->msgQ().get(&msg);
    CAMHAL_LOGDA("-Msg get...");
    bool ret = true;

    switch(msg.command)
    {
    case NotificationThread::NOTIFIER_EXIT:
        {
            ret = false;
            break;
        }
    case NotificationThread::NOTIFIER_START:
        {
            break;
        }
    case NotificationThread::NOTIFIER_STOP:
        {
            break;
        }
   
    default:
        {
            CAMHAL_LOGEA("Error: ProcessMsg() command from Camera HAL");
            break;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;


}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
bool AppCallbackNotifier::processJpegMessage(JpegThread * thread)
{
    ///Retrieve the command from the command queue and process it
    ActUtils::Message msg;

    LOG_FUNCTION_NAME;

    CAMHAL_LOGDA("+Msg get...");
    mJpegThread->msgQ().get(&msg);
    CAMHAL_LOGDA("-Msg get...");
    bool ret = true;

    switch(msg.command)
    {
    case JpegThread::JPEG_EXIT:
        {
            ret = false;
            break;
        }
    case JpegThread::JPEG_START:
        {
            break;
        }
    case JpegThread::JPEG_STOP:
        {
            break;
        }
   
    case JpegThread::JPEG_FLUSH:
        {
            unsigned int flush;

            //ActionsCode(author:liuyiguang, change_code)
            //flush = (unsigned int)msg.arg1;
            flush = (long)msg.arg1;
            CAMHAL_LOGDB("RESIZE_FLUSH flush =0x%x",flush);

            if(flush&CAPTURE_START_FLUSH)
            {
                if(!thread->mPictureValid)
                {
                    flushJpegQ();
                }
                thread->mPictureValid = true;
            }
            if(flush&CAPTURE_STOP_FLUSH)
            {            
                flushJpegQ();
                thread->mPictureValid = false;
            }
            if(flush&PREVIEW_START_FLUSH)
            {
                thread->mPreviewValid = true;
            }
            if(flush&PREVIEW_STOP_FLUSH)
            {
                thread->mPreviewValid = false;
            }
            if(flush&VIDEO_START_FLUSH)
            {
                thread->mVideoValid = true;
            }
            if(flush&VIDEO_STOP_FLUSH)
            {
                thread->mVideoValid = false;
            }
            mJpegThreadSem.Signal();

            
            break;
        }
    default:
        {
            CAMHAL_LOGEA("Error: ProcessMsg() command from Camera HAL");
            break;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;


}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
bool AppCallbackNotifier::processResizeMessage(ResizeThread * thread)
{
    ///Retrieve the command from the command queue and process it
    ActUtils::Message msg;

    LOG_FUNCTION_NAME;

    CAMHAL_LOGDA("+Msg get...");
    mResizeThread->msgQ().get(&msg);
    CAMHAL_LOGDA("-Msg get...");
    bool ret = true;

    switch(msg.command)
    {
    case ResizeThread::RESIZE_EXIT:
        {
            ret = false;
            break;
        }
    case ResizeThread::RESIZE_START:
        {
            break;
        }
    case ResizeThread::RESIZE_STOP:
        {
            break;
        }
    case ResizeThread::RESIZE_FLUSH:
        {
            unsigned int flush;
            bool flushForImage = false;
            bool flushForPreview = false;
            bool flushForVideo = false;

            //ActionsCode(author:liuyiguang, change_code)
            //flush = (unsigned int)msg.arg1;
            flush = (long)msg.arg1;
            CAMHAL_LOGDB("RESIZE_FLUSH flush =0x%x",flush);

            if(flush&CAPTURE_START_FLUSH)
            {
                if(!thread->mPictureValid)
                {
                    flushForImage = true;
                }
                thread->mPictureValid = true;
            }
            if(flush&CAPTURE_STOP_FLUSH)
            {
                flushForImage = true;
                thread->mPictureValid = false;
            }
            if(flush&PREVIEW_START_FLUSH)
            {
                if(!thread->mPreviewValid)
                {
                    flushForPreview = true;
                }
                thread->mPreviewValid = true;
            }
            if(flush&PREVIEW_STOP_FLUSH)
            {
                flushForPreview = true;
                thread->mPreviewValid = false;
            }
            if(flush&VIDEO_START_FLUSH)
            {
                if(!thread->mVideoValid)
                {
                    flushForVideo = true;
                }
                thread->mVideoValid = true;
            }
            if(flush&VIDEO_STOP_FLUSH)
            {
                flushForVideo = true;
                thread->mVideoValid = false;
            }


            if(flushForImage&&flushForPreview&&flushForVideo)
            {
                flushResizeQ();
            }
            else
            {
                if(flushForImage)
                {
                    flushResizeQForImage();                    
                }
                if(flushForPreview)
                {
                    flushResizeQForPreview();                    
                }
                if(flushForVideo)
                {
                    flushResizeQForVideo();                    
                }
                else
                {
                }
            }
            mResizeThreadSem.Signal();
            
            break;
        }
   
    default:
        {
            CAMHAL_LOGEA("Error: ProcessMsg() command from Camera HAL");
            break;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;


}

AppCallbackNotifier::~AppCallbackNotifier()
{
    LOG_FUNCTION_NAME;

    ///Stop app callback notifier if not already stopped
    stop();

    ///Unregister with the frame provider
    if ( NULL != mFrameProvider )
    {
        mFrameProvider->disableFrameNotification(CameraFrame::ALL_FRAMES);
    }

    //unregister with the event provider
    if ( NULL != mEventProvider )
    {
        mEventProvider->disableEventNotification(CameraHalEvent::ALL_EVENTS);
    }

    ActUtils::Message msg = {0,0,0,0,0,0};
    msg.command = JpegThread::JPEG_EXIT;
    
    ///Post the message to display thread
    mJpegThread->msgQ().put(&msg);

    //Exit and cleanup the thread
    mJpegThread->requestExit();
    mJpegThread->join();

    //Delete the display thread
    mJpegThread.clear();

    msg.command = ResizeThread::RESIZE_EXIT;
    ///Post the message to display thread
    mResizeThread->msgQ().put(&msg);

    //Exit and cleanup the thread
    mResizeThread->requestExit();
    mResizeThread->join();

    //Delete the display thread
    mResizeThread.clear();


    msg.command = NotificationThread::NOTIFIER_EXIT;

    ///Post the message to display thread
    mNotificationThread->msgQ().put(&msg);

    //Exit and cleanup the thread
    mNotificationThread->requestExit();
    mNotificationThread->join();

    //Delete the display thread
    mNotificationThread.clear();

    ///Free the event and frame providers
    if ( NULL != mEventProvider )
    {
        ///Deleting the event provider
        CAMHAL_LOGDA("Stopping Event Provider");
        delete mEventProvider;
        mEventProvider = NULL;
    }

    if ( NULL != mFrameProvider )
    {
        ///Deleting the frame provider
        CAMHAL_LOGDA("Stopping Frame Provider");
        delete mFrameProvider;
        mFrameProvider = NULL;
    }

    releaseSharedVideoBuffers();

    

#ifdef CAMERA_VCE_OMX_JPEG
    mVceJpegEncoder->stopEncode();
    delete mVceJpegEncoder;
#endif

#ifdef CAMERA_VCE_OMX_RESIZE
    mVceImageResize->stopEncode();
    mResizeType = UNUSED_RESIZE;
    delete mVceImageResize;
#endif

    LOG_FUNCTION_NAME_EXIT;
}

//Free all video heaps and buffers
/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
void AppCallbackNotifier::releaseSharedVideoBuffers()
{
    LOG_FUNCTION_NAME;

    if(mUseMetaDataBufferMode)
    {
        camera_memory_t* videoMedatadaBufferMemory;
        for (unsigned int i = 0; i < mVideoMetadataBufferMemoryMap.size();  i++)
        {
            //ActionsCode(author:liuyiguang, change_code)
            //videoMedatadaBufferMemory = (camera_memory_t*) mVideoMetadataBufferMemoryMap.valueAt(i);
            videoMedatadaBufferMemory = (camera_memory_t*) (long)mVideoMetadataBufferMemoryMap.valueAt(i);
            if(NULL != videoMedatadaBufferMemory)
            {
                videoMedatadaBufferMemory->release(videoMedatadaBufferMemory);
                CAMHAL_LOGDB("Released  videoMedatadaBufferMemory=0x%p", videoMedatadaBufferMemory);
            }
        }

        mVideoMetadataBufferMemoryMap.clear();
        mVideoMetadataBufferReverseMap.clear();
        if (mUseCameraHalVideoBuffers)
        {
            mVideoMap.clear();
        }
    }
    else
    {

        if(mVideoMemory)
        {
            mVideoMemory->release(mVideoMemory);
            mVideoMemory = NULL;
        }
        mVideoMemoryMap.clear();
        mVideoMemoryReverseMap.clear();

    }

    LOG_FUNCTION_NAME_EXIT;
}

void AppCallbackNotifier::setEventProvider(int32_t eventMask, MessageNotifier * eventNotifier)
{

    LOG_FUNCTION_NAME;
    ///@remarks There is no NULL check here. We will check
    ///for NULL when we get start command from CameraHal
    ///@Remarks Currently only one event provider (CameraAdapter) is supported
    ///@todo Have an array of event providers for each event bitmask
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

void AppCallbackNotifier::setFrameProvider(FrameNotifier *frameNotifier)
{
    LOG_FUNCTION_NAME;
    ///@remarks There is no NULL check here. We will check
    ///for NULL when we get the start command from CameraAdapter
    mFrameProvider = new FrameProvider(frameNotifier, this, frameCallbackRelay);
    if ( NULL == mFrameProvider )
    {
        CAMHAL_LOGEA("Error in creating FrameProvider");
    }
    else
    {
        //Register only for captured images and RAW for now
        //TODO: Register for and handle all types of frames
        mFrameProvider->enableFrameNotification(CameraFrame::IMAGE_FRAME_SYNC);
        mFrameProvider->enableFrameNotification(CameraFrame::RAW_FRAME_SYNC);
    }

    LOG_FUNCTION_NAME_EXIT;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t AppCallbackNotifier::startImageCallbacks(CameraParameters &params, void *buffers, uint32_t *offsets, int fd, size_t length, size_t count)
{
    unsigned int i;

    if(count > (unsigned int)AppCallbackNotifier::MAX_BUFFERS )
    {
        CAMHAL_LOGEB("buffer num is too large,%d, max=%d", count, AppCallbackNotifier::MAX_BUFFERS);
        errorNotify(CAMERA_ERROR_FATAL);
    }
    
    ActUtils::Message msg = {0,0,0,0,0,0};

    {
        Mutex::Autolock lock(mResizeThreadLock);    

        msg.command = ResizeThread::RESIZE_FLUSH;
        msg.arg1 = (void *)CAPTURE_START_FLUSH;

        ///Post the message to display thread
        mResizeThread->msgQ().put(&msg);

        mResizeThreadSem.Wait();
    }

    {
        Mutex::Autolock lock(mJpegThreadLock);    
        msg.command = JpegThread::JPEG_FLUSH;
        msg.arg1 = (void *)CAPTURE_START_FLUSH;

        ///Post the message to display thread
        mJpegThread->msgQ().put(&msg);

        mJpegThreadSem.Wait();
    }

    mImageBufSize= length;
    mImageBufCount = count;
    CAMHAL_LOGDB("startImageCallbacks-mImageBufCount=%d",mImageBufCount);
    CAMHAL_LOGDB("startImageCallbacks-mImageBufSize=%d",mImageBufSize);
    for(i=0; i< mImageBufCount; i++)
    {
        //ActionsCode(author:liuyiguang, change_code)
        //CAMHAL_LOGDB("startImageCallbacks(%d)=%p", i,(unsigned char *)(((uint32_t *)buffers)[i]));
        //mImageBufs[i] = (unsigned char *)(((uint32_t *)buffers)[i]);
        CAMHAL_LOGDB("startImageCallbacks(%d)=%p", i,(unsigned char *)((long)((uint32_t *)buffers)[i]));
        mImageBufs[i] = (unsigned char *)((long)((uint32_t *)buffers)[i]);
    }


    return NO_ERROR;

}
status_t AppCallbackNotifier::stopImageCallbacks()
{
    LOG_FUNCTION_NAME;

    ActUtils::Message msg = {0,0,0,0,0,0};
    {
        Mutex::Autolock lock(mResizeThreadLock);    
        msg.command = ResizeThread::RESIZE_FLUSH;
        msg.arg1 = (void *)CAPTURE_STOP_FLUSH ;

        ///Post the message to display thread
        mResizeThread->msgQ().put(&msg);
        mResizeThreadSem.Wait();
    }
    {
        Mutex::Autolock lock(mJpegThreadLock);    
        msg.command = JpegThread::JPEG_FLUSH;
        msg.arg1 = (void *)CAPTURE_STOP_FLUSH;

        ///Post the message to display thread
        mJpegThread->msgQ().put(&msg);
        mJpegThreadSem.Wait();
    }

    mImageBufCount = 0;
    LOG_FUNCTION_NAME_EXIT;
    return NO_ERROR;
}
status_t AppCallbackNotifier::waitforImageBufs()
{
    LOG_FUNCTION_NAME;

    ActUtils::Message msg = {0,0,0,0,0,0};
    {
        Mutex::Autolock lock(mResizeThreadLock);    
        msg.command = ResizeThread::RESIZE_FLUSH;
        msg.arg1 = (void*)CAPTURE_STOP_FLUSH ;

        ///Post the message to display thread
        mResizeThread->msgQ().put(&msg);
        mResizeThreadSem.Wait();
    }

    {
        Mutex::Autolock lock(mJpegThreadLock);    
        msg.command = JpegThread::JPEG_FLUSH;
        msg.arg1 = (void*)CAPTURE_STOP_FLUSH;

        ///Post the message to display thread
        mJpegThread->msgQ().put(&msg);

        mJpegThreadSem.Wait();
    }
   
    mImageBufCount = 0;
    LOG_FUNCTION_NAME_EXIT;
    return NO_ERROR;

}

/*
* OPTIMIZE: it will make the continuous preview datacallback to app usr to be more fluent,
* by allocating previewcallback databuf before using.
* NOTE: Only when the CAMERA_MSG_PREVIEW_FRAME msg type is requested, 
* these bufs can be needed.
* ActionsCode(author:liyuan, Add_code) 
*/
status_t AppCallbackNotifier::PreAllocPreviewCBbufs(int w, int h, CameraFrame::FrameFormat format, size_t count)
{
    size_t size = 0;
    int copyBufCount;

    size = CameraFrame::getFrameLength(format, w, h);
		
    if(count < (int)AppCallbackNotifier::PREVIEW_COPY_BUFFERS)
    {
        copyBufCount = (int)AppCallbackNotifier::PREVIEW_COPY_BUFFERS;
			
    }else{
        copyBufCount = count;
    }

    if(mPreviewMemory)
    {
        mPreviewMemory->release(mPreviewMemory);
    }
    mPreviewMemory = mRequestMemory(-1, size, copyBufCount, NULL);
    mPreviewMemoryCount = copyBufCount;
    if(!mPreviewMemory)
    {
        ALOGE("Alloc Memory for PreviewCallback failed");
        return NO_MEMORY;
		
    }else{
        ALOGD("Alloc Memory for PreviewCallback done");
    }
		
    int32_t i;
    for ( i=0; i < copyBufCount; i++)	
    {	
        mPreviewMemoryBufs[i] = reinterpret_cast<unsigned char*>( mPreviewMemory->data) + (i*size);
    }  
    mPreviewMemoryIndex = 0;
	
    mPreAllocPreviewCBbufsdone = true;
	
    return NO_ERROR;

}

/**
 *
 * BUGFIX:  Fix the buffer mismatch problem in the resolution 176x144, which was found in the cts verifier test step.
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 * BUGFIX:  Add support for PIXEL_FORMAT_YUV422SP
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */    
status_t AppCallbackNotifier::startPreviewCallbacks(CameraParameters &params, void *buffers, uint32_t *offsets, int fd, size_t length, size_t count)
{
    sp<MemoryHeapBase> heap;
    sp<MemoryBase> buffer;
    uint32_t *bufArr;


    LOG_FUNCTION_NAME;
    bufArr = reinterpret_cast<uint32_t *>(buffers);

    if(count > (unsigned int)AppCallbackNotifier::MAX_BUFFERS )
    {
        CAMHAL_LOGEB("buffer num is too large,%d, max=%d", count, AppCallbackNotifier::MAX_BUFFERS);
        errorNotify(CAMERA_ERROR_FATAL);
    }


    if ( NULL == mFrameProvider )
    {
        CAMHAL_LOGEA("Trying to start video recording without FrameProvider");
        return -EINVAL;
    }

    if ( mPreviewing )
    {
        CAMHAL_LOGDA("+Already previewing");
        return NO_INIT;
    }
    

    if ( mCameraHal->msgTypeEnabledNoLock(CAMERA_MSG_PREVIEW_FRAME ) )
    {
	/*
	* OPTIMIZE: previously alloc the bufs for preview datacallback use. 
        *ActionsCode(author:liyuan, Add_code)
	*/
    	if(mPreAllocPreviewCBbufsdone == false)
	{
            status_t ret;
	    int w,h;
	    params.getPreviewSize(&w, &h);
	    ret = PreAllocPreviewCBbufs(w,h,mPreviewFormat,count);
	    if(ret != NO_ERROR){
		return ret;
	    }
    	}
        mFrameProvider->enableFrameNotification(CameraFrame::PREVIEW_RAW_FRAME_SYNC);

    }
	

    ActUtils::Message msg = {0,0,0,0,0,0};
    {
        Mutex::Autolock lock(mResizeThreadLock);    
        msg.command = ResizeThread::RESIZE_FLUSH;
        msg.arg1 = (void *)(((unsigned int)PREVIEW_START_FLUSH));

        ///Post the message to display thread
        mResizeThread->msgQ().put(&msg);

        mResizeThreadSem.Wait();
    }
    {

        Mutex::Autolock lock(mJpegThreadLock);    
        msg.command = JpegThread::JPEG_FLUSH;
        msg.arg1 = (void*)(((unsigned int)PREVIEW_START_FLUSH));

        ///Post the message to display thread
        mJpegThread->msgQ().put(&msg);


        mJpegThreadSem.Wait();
    }

    mPreviewBufSize = length;
    mPreviewBufCount = count;

    for(unsigned int i = 0; i < count; i++)
    {
        //ActionsCode(author:liuyiguang, change_code)
        //mPreviewBufs[i] = (unsigned char *)(((uint32_t *)buffers)[i]);
        mPreviewBufs[i] = (unsigned char *)((long)((uint32_t *)buffers)[i]);
    }
	
    //ActionsCode(author:liuyiguang, add_code)
    if (mCameraHal->mVceResize)
    {
        //open vce resize module
        VR_Input_t input;
        input.dstw_align = ALIGN_16PIXELS;
        mVceResizeHandle = VceReSize_Open(&input);
        if (!mVceResizeHandle)
        {
            CAMHAL_LOGDA("Open vce resize module failed!");
            return NO_INIT; 
        }
    }
    mPreviewing = true;

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

void AppCallbackNotifier::useVideoBuffers(bool useVideoBuffers)
{
    LOG_FUNCTION_NAME;

    mUseCameraHalVideoBuffers = useVideoBuffers;

    LOG_FUNCTION_NAME_EXIT;
}

bool AppCallbackNotifier::getUseVideoBuffers()
{
    return mUseCameraHalVideoBuffers;
}

void AppCallbackNotifier::setVideoRes(int width, int height)
{
    LOG_FUNCTION_NAME;

    mVideoWidth = width;
    mVideoHeight = height;

    LOG_FUNCTION_NAME_EXIT;
}

status_t AppCallbackNotifier::stopPreviewCallbacks()
{
    sp<MemoryHeapBase> heap;
    sp<MemoryBase> buffer;

    LOG_FUNCTION_NAME;

    if ( NULL == mFrameProvider )
    {
        CAMHAL_LOGEA("Trying to stop preview callbacks without FrameProvider");
        return -EINVAL;
    }

    if ( !mPreviewing )
    {
        return NO_INIT;
    }

    mFrameProvider->disableFrameNotification(CameraFrame::PREVIEW_RAW_FRAME_SYNC);

    ActUtils::Message msg = {0,0,0,0,0,0};
    {
        Mutex::Autolock lock(mResizeThreadLock);    
        msg.command = ResizeThread::RESIZE_FLUSH;
        msg.arg1 = (void *)((unsigned int)PREVIEW_STOP_FLUSH|(unsigned int)CAPTURE_STOP_FLUSH|(unsigned int)VIDEO_STOP_FLUSH);

        ///Post the message to display thread
        mResizeThread->msgQ().put(&msg);

        mResizeThreadSem.Wait();
    }
    {

        Mutex::Autolock lock(mJpegThreadLock);    
        msg.command = JpegThread::JPEG_FLUSH;
        msg.arg1 = (void*)((unsigned int)PREVIEW_STOP_FLUSH|(unsigned int)CAPTURE_STOP_FLUSH|(unsigned int)VIDEO_STOP_FLUSH); 

        ///Post the message to display thread
        mJpegThread->msgQ().put(&msg);

        mJpegThreadSem.Wait();
    }


    flushEventQueue();
    flushFrames();

    for(unsigned int i = 0; i < mPreviewBufCount; i++)
    {
        mPreviewBufs[i] = 0;
    }
    mPreviewBufCount = 0;
    mPreviewBufSize = 0;
    if(mPreviewMemory)
    {
        mPreviewMemory->release(mPreviewMemory);
        mPreviewMemory = NULL;
    }
    mPreviewMemoryCount = 0;
    mPreviewMemoryIndex = 0;
    mPreAllocPreviewCBbufsdone = false;

    mVideoMemoryMap.clear();
    mVideoMemoryReverseMap.clear();


#ifdef CAMERA_VCE_OMX_JPEG
    mVceJpegEncoder->stopEncode();
#endif

#ifdef CAMERA_VCE_OMX_RESIZE
    mVceImageResize->stopEncode();
    mResizeType = UNUSED_RESIZE;
#endif
        
    mPreviewing = false;

    mImageBufCount = 0;

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;

}

status_t AppCallbackNotifier::useMetaDataBufferMode(bool enable)
{

#ifdef CAMERA_RUN_IN_EMULATOR
    mUseMetaDataBufferMode = false;
    return INVALID_OPERATION;
#else
    mUseMetaDataBufferMode = enable;
    return NO_ERROR;
#endif
}

bool AppCallbackNotifier::getMetaDataBufferMode()
{
    return mUseMetaDataBufferMode;

}


status_t AppCallbackNotifier::startRecording()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mRecordingLock);

    if ( NULL == mFrameProvider )
    {
        CAMHAL_LOGEA("Trying to start video recording without FrameProvider");
        ret = -1;
    }

    if(mRecording)
    {
        return NO_INIT;
    }

    if ( NO_ERROR == ret )
    {
        mFrameProvider->enableFrameNotification(CameraFrame::VIDEO_FRAME_SYNC);
    }
    ActUtils::Message msg = {0,0,0,0,0,0};
    {
        Mutex::Autolock lock(mResizeThreadLock);    
        msg.command = ResizeThread::RESIZE_FLUSH;
        msg.arg1 = (void *)VIDEO_START_FLUSH;

        ///Post the message to display thread
        mResizeThread->msgQ().put(&msg);

        mResizeThreadSem.Wait();
    }

    {
        Mutex::Autolock lock(mJpegThreadLock);    
        msg.command = JpegThread::JPEG_FLUSH;
        msg.arg1 = (void *)VIDEO_START_FLUSH;

        ///Post the message to display thread
        mJpegThread->msgQ().put(&msg);

        mJpegThreadSem.Wait();
    }


    mRecording = true;

    mFirstRecordingFrame = true;
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

//Allocate metadata buffers for video recording
/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t AppCallbackNotifier::initSharedVideoBuffers(CameraParameters &params,void *buffers, uint32_t *offsets, int fd, size_t length, size_t count, void *vidBufs)
{
    status_t ret = NO_ERROR;
    size_t size = 0;

    uint32_t *bufArr = NULL;

    LOG_FUNCTION_NAME;

    if(NULL == buffers)
    {
        CAMHAL_LOGEA("Error! Video buffers are NULL");
        return BAD_VALUE;
    }
    bufArr = (uint32_t *) buffers;

    /*
    * Condition1: MetaDataBufferMode used and extra ion videobuf is needed
    * Condition2: MetaDataBufferMode used and no extra ion videobuf is needed
    * ActionsCode(author:liyuan, add comments)
    */
    if(mUseMetaDataBufferMode)
    {

        camera_memory_t* videoMedatadaBufferMemory = NULL;

        for (uint32_t i = 0; i < count; i++)
        {
            videoMedatadaBufferMemory = mRequestMemory(-1, sizeof(video_metadata_t), 1, NULL);
            if((NULL == videoMedatadaBufferMemory) || (NULL == videoMedatadaBufferMemory->data))
            {
                CAMHAL_LOGEA("Error! Could not allocate memory for Video Metadata Buffers");
                return NO_MEMORY;
            }

            //ActionsCode(author:liuyiguang, change_code)
            //mVideoMetadataBufferMemoryMap.add(bufArr[i], (uint32_t)(videoMedatadaBufferMemory));
            //mVideoMetadataBufferReverseMap.add((uint32_t)(videoMedatadaBufferMemory->data), bufArr[i]);
            mVideoMetadataBufferMemoryMap.add(bufArr[i], (long)(videoMedatadaBufferMemory));
            mVideoMetadataBufferReverseMap.add((long)(videoMedatadaBufferMemory->data), bufArr[i]);
            CAMHAL_LOGDB("bufArr[%d]=0x%x, videoMedatadaBufferMemory=0x%p, videoMedatadaBufferMemory->data=0x%p",
                         i, bufArr[i], videoMedatadaBufferMemory, videoMedatadaBufferMemory->data);

            /*
	    * Condition1: MetaDataBufferMode used and extra ion videobuf is needed
	    * ActionsCode(author:liyuan, add comments)
	    */
            if (vidBufs != NULL)
            {
                uint32_t *vBufArr = (uint32_t *) vidBufs;
                mVideoMap.add(bufArr[i], vBufArr[i]);
                CAMHAL_LOGVB("bufArr[%d]=0x%x, vBuffArr[%d]=0x%x", i, bufArr[i], i, vBufArr[i]);
            }
        }
    }
    else
    {
	/*
	* Condition3: Non MetaDataBufferMode used 
	* ActionsCode(author:liyuan, add comments)
	*/

        size = CameraFrame::getFrameLength(mPreviewFormat, mVideoWidth, mVideoHeight);
        if(mVideoMemory)
        {
            mVideoMemory->release(mVideoMemory);
            mVideoMemory = NULL;
        }
        mVideoMemoryMap.clear();
        mVideoMemoryReverseMap.clear();

        mVideoMemory = mRequestMemory(-1, size, count, NULL);
        mVideoMemoryCount = count;
        if (!mVideoMemory)
        {
            return NO_MEMORY;
        }
        uint32_t i;
        for ( i=0; i < count; i++)
        {
            mVideoBufs[i] = reinterpret_cast<unsigned char*>( mVideoMemory->data) + (i*size);

            mVideoMemoryMap.add(static_cast<uint32_t>(bufArr[i]), static_cast<uint32_t>(i));
            mVideoMemoryReverseMap.add(reinterpret_cast<uint32_t>(mVideoBufs[i]), static_cast<uint32_t>(bufArr[i]));
        }


    }

exit:
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t AppCallbackNotifier::stopRecording()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mRecordingLock);

    if ( NULL == mFrameProvider )
    {
        CAMHAL_LOGEA("Trying to stop video recording without FrameProvider");
        ret = -1;
    }

    if(!mRecording)
    {
        return NO_INIT;
    }

    if ( NO_ERROR == ret )
    {
        mFrameProvider->disableFrameNotification(CameraFrame::VIDEO_FRAME_SYNC);
    }

    mRecording = false;
    mFirstRecordingFrame = false;

    ActUtils::Message msg = {0,0,0,0,0,0};

    {
        Mutex::Autolock lock(mResizeThreadLock);    

        msg.command = ResizeThread::RESIZE_FLUSH;
        msg.arg1 = (void *)VIDEO_STOP_FLUSH;

        ///Post the message to display thread
        mResizeThread->msgQ().put(&msg);

        mResizeThreadSem.Wait();
    }

    {
        Mutex::Autolock lock(mJpegThreadLock);    
        msg.command = JpegThread::JPEG_FLUSH;
        msg.arg1 = (void *)VIDEO_STOP_FLUSH;

        ///Post the message to display thread
        mJpegThread->msgQ().put(&msg);

        mJpegThreadSem.Wait();
    }
    

    ///Release the shared video buffers
    releaseSharedVideoBuffers();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t AppCallbackNotifier::releaseRecordingFrame(const void* mem)
{
    status_t ret = NO_ERROR;
    void *frame = NULL;
    CAMHAL_LOGDB("releaseRecordingFrame=%p", mem);
    LOG_FUNCTION_NAME;
    if ( NULL == mFrameProvider )
    {
        CAMHAL_LOGEA("Trying to stop video recording without FrameProvider");
        ret = -1;
    }

    if ( NULL == mem )
    {
        CAMHAL_LOGEA("Video Frame released is invalid");
        ret = -1;
    }

    if( NO_ERROR != ret )
    {
        return ret;
    }

    if(mUseMetaDataBufferMode)
    {
        video_metadata_t *videoMetadataBuffer = (video_metadata_t *) mem ;
        //ActionsCode(author:liuyiguang, change_code)
        //frame = (void*) mVideoMetadataBufferReverseMap.valueFor((uint32_t) videoMetadataBuffer);
        frame = (void*) (long)mVideoMetadataBufferReverseMap.valueFor((long) videoMetadataBuffer);
        CAMHAL_LOGVB("Releasing frame with videoMetadataBuffer=0x%p, videoMetadataBuffer->handle=0x%p & frame handle=0x%p\n",
                     videoMetadataBuffer, videoMetadataBuffer->handle, frame);
    }
    else
    {
        //ActionsCode(author:liuyiguang, change_code)
        //frame = reinterpret_cast<void *>(mVideoMemoryReverseMap.valueFor(reinterpret_cast<uint32_t>(mem)));
        frame = reinterpret_cast<void *>(mVideoMemoryReverseMap.valueFor(reinterpret_cast<long>(mem)));

        CAMHAL_LOGDB("releaseRecordingFrame frame=%p", frame);
    }

    if ( NO_ERROR == ret )
    {
        ret = mFrameProvider->returnFrame(frame, CameraFrame::VIDEO_FRAME_SYNC,  CameraFrame::PREVIEW_STREAM_TYPE);
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t AppCallbackNotifier::enableMsgType(int32_t msgType)
{
    if( msgType & ( CAMERA_MSG_PREVIEW_FRAME) )
    {
        {
            ActUtils::Message msg = {0,0,0,0,0,0};

            Mutex::Autolock lock(mResizeThreadLock);    
            msg.command = ResizeThread::RESIZE_FLUSH;
            msg.arg1 = (void *)((unsigned int)PREVIEW_START_FLUSH);

            mResizeThread->msgQ().put(&msg);

            mResizeThreadSem.Wait();
        }
	
	/*
        * OPTIMIZE: previously alloc the bufs for preview datacallback use.
        *ActionsCode(author:liyuan, Add_code)
        */	
	if(mPreAllocPreviewCBbufsdone == false)
	{
	    status_t ret;
	    CameraParameters parameters;
	    char *params = mCameraHal->getParameters();
	    const String8 strParams(params);
	    parameters.unflatten(strParams);
	    int w,h;
	    parameters.getPreviewSize(&w, &h);
	    ret = PreAllocPreviewCBbufs(w, h, mPreviewFormat, mPreviewBufCount);
	    if(ret != NO_ERROR){
		return ret;
	    }
	}

        mFrameProvider->enableFrameNotification(CameraFrame::PREVIEW_RAW_FRAME_SYNC);
    }

    return NO_ERROR;
}


status_t AppCallbackNotifier::disableMsgType(int32_t msgType)
{
    if( msgType & (CAMERA_MSG_PREVIEW_FRAME) )
    {
        mFrameProvider->disableFrameNotification(CameraFrame::PREVIEW_RAW_FRAME_SYNC);

        {
            ActUtils::Message msg = {0,0,0,0,0,0};

            Mutex::Autolock lock(mResizeThreadLock);    
            msg.command = ResizeThread::RESIZE_FLUSH;
            msg.arg1 = (void *)((unsigned int)PREVIEW_STOP_FLUSH);

            mResizeThread->msgQ().put(&msg);

            mResizeThreadSem.Wait();
        }
    }

    return NO_ERROR;

}

status_t AppCallbackNotifier::start()
{
    LOG_FUNCTION_NAME;
    if(mNotifierState==AppCallbackNotifier::NOTIFIER_STARTED)
    {
        CAMHAL_LOGDA("AppCallbackNotifier already running");
        LOG_FUNCTION_NAME_EXIT;
        return ALREADY_EXISTS;
    }

    ///Check whether initial conditions are met for us to start
    ///A frame provider should be available, if not return error
    if(!mFrameProvider)
    {
        ///AppCallbackNotifier not properly initialized
        CAMHAL_LOGEA("AppCallbackNotifier not properly initialized - Frame provider is NULL");
        LOG_FUNCTION_NAME_EXIT;
        return NO_INIT;
    }

    ///At least one event notifier should be available, if not return error
    ///@todo Modify here when there is an array of event providers
    if(!mEventProvider)
    {
        CAMHAL_LOGEA("AppCallbackNotifier not properly initialized - Event provider is NULL");
        LOG_FUNCTION_NAME_EXIT;
        ///AppCallbackNotifier not properly initialized
        return NO_INIT;
    }

    mNotifierState = AppCallbackNotifier::NOTIFIER_STARTED;//
    

	
    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;

}

status_t AppCallbackNotifier::stop()
{
    LOG_FUNCTION_NAME;

    if(mNotifierState!=AppCallbackNotifier::NOTIFIER_STARTED)
    {
        CAMHAL_LOGDA("AppCallbackNotifier already in stopped state");
        LOG_FUNCTION_NAME_EXIT;
        return ALREADY_EXISTS;
    }

    mNotifierState = AppCallbackNotifier::NOTIFIER_STOPPED;

    //ActionsCode(author:liuyiguang, add_code)
    if (mVceResizeHandle)
    {
        VceReSize_Close(mVceResizeHandle);
        mVceResizeHandle = NULL;
    }

    LOG_FUNCTION_NAME_EXIT;
    return NO_ERROR;
}



void AppCallbackNotifier::ImageJpegObserver::sendOutputBuffer(void *outbuf, unsigned int offset, unsigned int size)
{
}

void AppCallbackNotifier::ImageJpegObserver::returnInputBuffer(void * buffer)
{
}

void AppCallbackNotifier::ImageJpegObserver::onOmxVceError(int error)
{
    mAppCallbackNotifier->errorNotify(error);
}

void AppCallbackNotifier::ImageResizeObserver::sendOutputBuffer(void *outbuf, unsigned int offset, unsigned int size)
{
}

void AppCallbackNotifier::ImageResizeObserver::returnInputBuffer(void * buffer)
{
}

void AppCallbackNotifier::ImageResizeObserver::onOmxVceError(int error)
{
    mAppCallbackNotifier->errorNotify(error);
}


/*--------------------NotificationHandler Class ENDS here-----------------------------*/



};
