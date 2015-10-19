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
* @file OMXFD.cpp
*
* This file contains functionality for handling face detection.
*
*/
#include "CameraHalDebug.h"

#include "CameraHal.h"
#include "OMXCameraAdapter.h"

#include "OMXVce.h"

namespace android
{

status_t OMXCameraAdapter::setParametersFD(const CameraParameters &params,
        BaseCameraAdapter::AdapterState state)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t OMXCameraAdapter::initFaceDetection()
{
    status_t ret = NO_ERROR;
#ifdef CAMERA_VCE_OMX_FD
    mVceFaceDetect = new OMXVceFaceDetect();
    mVceFaceDetect->setVceObserver(this);
    ret = mVceFaceDetect->init();
    if ( NO_ERROR != ret )
    {
        CAMHAL_LOGEA("Couldn't mVceFaceDetect.init!");
    }

    mFDFrameProvider = new FrameProvider(this, this, FDFrameCallbackRelay);

#endif
    return ret;

}

status_t OMXCameraAdapter::destroyFaceDetection()
{
    status_t ret = NO_ERROR;
#ifdef CAMERA_VCE_OMX_FD
    delete mVceFaceDetect;
    delete mFDFrameProvider;
#endif
    return ret;

}
status_t OMXCameraAdapter::startFaceDetection()
{
    status_t ret = NO_ERROR;
    Mutex::Autolock lock(mFaceDetectionLock);
    if(mFaceDetectionRunning)
    {
        return ret;
    }

    //add this for cts, if not support fd, just return error
    if(atoi(mCapabilities->get(CameraProperties::MAX_FD_HW_FACES))==0)
    {
        ret = BAD_VALUE;
        return ret;
    }

#ifdef CAMERA_VCE_OMX_FD
    OMXCameraPortParameters *cap;
    cap = &mCameraAdapterParameters.mCameraPortParams[mCameraAdapterParameters.mPrevPortIndex];

    ret = mVceFaceDetect->setFaceDetectInfo(mDeviceType, mDeviceOrientation);
    if(ret < 0)
    {
        CAMHAL_LOGEB("setFaceDetectInfo error, %d,%d\n",mDeviceType, mDeviceOrientation);
    }

    ret = mVceFaceDetect->setImageSize(cap->mWidth, cap->mHeight);
    ret |= mVceFaceDetect->setOutputImageSize(cap->mWidth, cap->mHeight);
    ret |= mVceFaceDetect->setImageFormat(cap->mColorFormat);
    ret |= mVceFaceDetect->setImageInputCnt((OMX_U32)cap->mNumBufs);
    ret |= mVceFaceDetect->useBuffers(OMXVceImageInputPort, cap->mBufSize,reinterpret_cast<void **>(&(cap->mHostBufaddr)),cap->mNumBufs);
    ret |= mVceFaceDetect->startFaceDetect();

    if ( NULL != mFDFrameProvider )
    {
        mFDFrameProvider->enableFrameNotification(CameraFrame::FD_FRAME_SYNC);
    }

#endif
    mFaceDetectionRunning = true;
out:
    return ret;
}

status_t OMXCameraAdapter::stopFaceDetection()
{
    status_t ret = NO_ERROR;
    const char *str = NULL;
    BaseCameraAdapter::AdapterState state;
    BaseCameraAdapter::getState(state);

    Mutex::Autolock lock(mFaceDetectionLock);
    if(!mFaceDetectionRunning)
    {
        return ret;
    }
    mFaceDetectionRunning =false;
#ifdef CAMERA_VCE_OMX_FD
    if ( NULL != mFDFrameProvider )
    {
        mFDFrameProvider->disableFrameNotification(CameraFrame::FD_FRAME_SYNC);
    }

    ret = mVceFaceDetect->stopFaceDetect();
#endif
    

out:
    return ret;
}

void OMXCameraAdapter::pauseFaceDetection(bool pause)
{
    status_t ret = NO_ERROR;
    Mutex::Autolock lock(mFaceDetectionLock);
    // pausing will only take affect if fd is already running
    if (mFaceDetectionRunning)
    {
        mFaceDetectionPaused = pause;
#ifdef CAMERA_VCE_OMX_FD
        if(!mFaceDetectionPaused)
        {
            ret = mVceFaceDetect->setFaceDetectInfo(mDeviceType, mDeviceOrientation);
            if(ret < 0)
            {
                CAMHAL_LOGEB("setFaceDetectInfo error, %d,%d\n",mDeviceType, mDeviceOrientation);
            }
            
        }
#endif
    }
}
status_t OMXCameraAdapter::setFaceDetectionOrientation(int orientation)
{
    status_t ret = NO_ERROR;
#ifdef CAMERA_VCE_OMX_FD
    Mutex::Autolock lock(mFaceDetectionLock);
    if(!mFaceDetectionRunning || mFaceDetectionPaused)
    {
        return ret;
    }
    ret = mVceFaceDetect->setFaceDetectInfo(mDeviceType, orientation);
    if(ret < 0)
    {
        CAMHAL_LOGEB("setFaceDetectInfo error, %d,%d\n",mDeviceType, mDeviceOrientation);
    }
#endif
    return ret;
}



#ifdef CAMERA_VCE_OMX_FD
status_t OMXCameraAdapter::detectFaces(void *data,VceCropRect *crop)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    ret = mVceFaceDetect->faceDetect(data, crop, NULL, NULL, NULL);

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
void OMXCameraAdapter::sendFaces(void *data,unsigned int offset, unsigned int size)
{
    status_t ret = NO_ERROR;
    camera_frame_metadata_t *faces;
    omx_camera_frame_metadata *frame_metadata = NULL;
    sp<CameraFDResult> fdResult = NULL;
    int facenum = 0;

    LOG_FUNCTION_NAME;
    if(!mFaceDetectionRunning || mFaceDetectionPaused)
    {
        goto EXIT;
    }
    //ActionsCode(author:liuyiguang, change_code)
    //frame_metadata = (omx_camera_frame_metadata *)((unsigned int)data+offset);
    frame_metadata = (omx_camera_frame_metadata *)((long)data+offset);
    faces = ( camera_frame_metadata_t * ) malloc(sizeof(camera_frame_metadata_t));
    if ( NULL == faces )
    {
        ret =  -ENOMEM;
        goto EXIT;
    }
    facenum = frame_metadata->number_of_faces > MAX_FD_HW_NUM ?MAX_FD_HW_NUM:frame_metadata->number_of_faces;

    faces->faces = ( camera_face_t * ) malloc(sizeof(camera_face_t)*facenum);
    if ( NULL == faces->faces )
    {
        ret =  -ENOMEM;
        goto EXIT;
    }
    memcpy(faces->faces, frame_metadata->faces, sizeof(camera_face_t)*facenum);
    faces->number_of_faces =facenum;
    /*
    ALOGE("faces->number_of_faces=%d\n",faces->number_of_faces);
    if(faces->number_of_faces>=1)
    {
        ALOGE("faces->faces[0].x=%d\n",faces->faces[0].rect[0]);
        ALOGE("faces->faces[0].y=%d\n",faces->faces[0].rect[1]);
        ALOGE("faces->faces[0].w=%d\n",faces->faces[0].rect[2]);
        ALOGE("faces->faces[0].h=%d\n",faces->faces[0].rect[3]);
    }
    */
    if ( NO_ERROR == ret )
    {
        fdResult = new CameraFDResult(faces);
    }
    
    if(frame_metadata != NULL)
    {
        mVceFaceDetect->freeFaceDetectData(frame_metadata);
    }

    if ( NULL != fdResult.get() )
    {
        notifyFaceSubscribers(fdResult);
        fdResult.clear();
    }

    LOG_FUNCTION_NAME_EXIT;

    return ;
EXIT:
    if(frame_metadata != NULL)
    {
        mVceFaceDetect->freeFaceDetectData(frame_metadata);
    }
    return ;
}

void OMXCameraAdapter::FDFrameCallbackRelay(CameraFrame *cameraFrame)
{
    LOG_FUNCTION_NAME;
    OMXCameraAdapter *appcbn = (OMXCameraAdapter*) (cameraFrame->mCookie);
    appcbn->FDFrameCallback(cameraFrame);
    LOG_FUNCTION_NAME_EXIT;
}

void OMXCameraAdapter::FDFrameCallback(CameraFrame *cameraFrame)
{
    LOG_FUNCTION_NAME;
    if(!mFaceDetectionRunning || mFaceDetectionPaused)
    {
        goto EXIT;
    }
    VceCropRect crop;
    crop.cropw =  cameraFrame->mWidth;
    crop.croph =  cameraFrame->mHeight;
    crop.cropx =  cameraFrame->mXOff;
    crop.cropy =  cameraFrame->mYOff;
    detectFaces(cameraFrame->mBuffer, &crop);
EXIT:
    LOG_FUNCTION_NAME_EXIT;

    return;
}

void OMXCameraAdapter::sendOutputBuffer(void *outbuf, unsigned int offset, unsigned int size)
{
    LOG_FUNCTION_NAME;
    sendFaces(outbuf,offset,size);
    LOG_FUNCTION_NAME_EXIT;
}
void OMXCameraAdapter::returnInputBuffer(void * buffer)
{
    LOG_FUNCTION_NAME;
    mFDFrameProvider->returnFrame(buffer,  CameraFrame::FD_FRAME_SYNC, CameraFrame::PREVIEW_STREAM_TYPE);
    LOG_FUNCTION_NAME_EXIT;
}
void OMXCameraAdapter::onOmxVceError(int error)
{
    LOG_FUNCTION_NAME;
    mErrorNotifier->errorNotify(error);
    LOG_FUNCTION_NAME_EXIT;
}
#endif


};
