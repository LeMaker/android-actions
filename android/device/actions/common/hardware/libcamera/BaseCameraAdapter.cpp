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

#include "BaseCameraAdapter.h"

namespace android
{

/*--------------------Camera Adapter Class STARTS here-----------------------------*/

void BaseCameraAdapter::CameraStreamBuffer::setVaddr(void *vaddr){
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);
    mVaddr = vaddr;
    LOG_FUNCTION_NAME_EXIT
};
void * BaseCameraAdapter::CameraStreamBuffer::getVaddr(){
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);
    LOG_FUNCTION_NAME_EXIT
    return mVaddr;
};

void BaseCameraAdapter::CameraStreamBuffer::startTransaction()
{
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);
    if(mTransaction || mRefCountSum != 0)
    {
        ALOGE("warning, nested startTransaction!!!");
    }
    mTransaction = true;
    mReturnInTransaction = false;
    mFrameTypesInTransaction = 0;

    LOG_FUNCTION_NAME_EXIT
    return ;
}

void BaseCameraAdapter::CameraStreamBuffer::endTransaction()
{
    LOG_FUNCTION_NAME
    bool needReturn = false;
    bool needCB = false;
    unsigned int frameTypes = 0;
    dumpBuffer();
    {
        Mutex::Autolock lock(mLock);
        frameTypes = mFrameTypes;
        if(mTransaction && mFrameTypes)
        {
            mTransaction =false;

            if(mRefCountSum == 0)
            {
                needReturn = true;
                mFrameTypes = 0;
            }

            if(mReturnInTransaction)
            {
                needCB = true;
            }
        }
    }
    if(needCB)
    {    
        mStream->onReturnFrame(mBuffer, mFrameTypesInTransaction, frameTypes);
    }

    if(needReturn)
    {
        returnFrame(frameTypes);                
    }
    dumpBuffer();

    LOG_FUNCTION_NAME_EXIT
    return ;
}
void BaseCameraAdapter::CameraStreamBuffer::dumpBuffer()
{
#ifdef DEBUG_LOG
    LOG_FUNCTION_NAME
    int i;
    Mutex::Autolock lock(mLock);
    CAMHAL_LOGIB("======dumpBuffer=%p\n", mBuffer);
    CAMHAL_LOGIB("mRefCountSum=%d\n", mRefCountSum);
    for(i = 0; i < CameraFrame::MAX_FRAME_TYPE; i++)
    {
        CAMHAL_LOGIB("mRefCount[%d]=%d\n", i,mRefCount[i]);
    }
    CAMHAL_LOGIA("=======end dumpBuffer\n");

    LOG_FUNCTION_NAME_EXIT
#endif
}

void BaseCameraAdapter::CameraStreamBuffer::incRefCount(CameraFrame::FrameType frameType){
    LOG_FUNCTION_NAME
    int frameIndex =CameraFrame::toFrameTypeIndex(frameType);
    Mutex::Autolock lock(mLock);
    mRefCountSum++;
    mRefCount[frameIndex]++;
    mFrameTypes |= frameType;

    LOG_FUNCTION_NAME_EXIT
};
void BaseCameraAdapter::CameraStreamBuffer::decRefCount(CameraFrame::FrameType frameType){
    LOG_FUNCTION_NAME
    bool needReturn = false;
    bool needCB = false;
    unsigned int frameTypes = 0;
    int frameIndex =CameraFrame::toFrameTypeIndex(frameType);

    dumpBuffer();
    {
        Mutex::Autolock lock(mLock);
        frameTypes = mFrameTypes;
        if(mRefCount[frameIndex]>0)
        {
            mRefCountSum--;
            mRefCount[frameIndex]--;
            if(mRefCountSum == 0 && !mTransaction)
            {
                needReturn = true;
                mFrameTypes = 0;
            }

            if(!mTransaction)
            {
                needCB = true;
            }
            else
            {
                mReturnInTransaction = true;
                mFrameTypesInTransaction |= frameType;
            }
        }
    }
    if(needCB)
    {
        mStream->onReturnFrame(mBuffer, frameType, frameTypes);
    }

    if(needReturn)
    {
        returnFrame(frameTypes);                
    }

    dumpBuffer();

    LOG_FUNCTION_NAME_EXIT
};
uint32_t BaseCameraAdapter::CameraStreamBuffer::getRefCountSum() {
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);
    LOG_FUNCTION_NAME_EXIT
    return mRefCountSum;
};
uint32_t BaseCameraAdapter::CameraStreamBuffer::getRefCount(CameraFrame::FrameType frameType){
    LOG_FUNCTION_NAME
    int frameIndex =CameraFrame::toFrameTypeIndex(frameType);
    Mutex::Autolock lock(mLock);
    LOG_FUNCTION_NAME_EXIT
    return mRefCount[frameIndex];
};
void BaseCameraAdapter::CameraStreamBuffer::initRefCount(CameraFrame::FrameType frameType, uint32_t refcnt){
    LOG_FUNCTION_NAME
    int frameIndex =CameraFrame::toFrameTypeIndex(frameType);
    Mutex::Autolock lock(mLock);
    mRefCountSum = refcnt;
    mRefCount[frameIndex] = refcnt;
    mFrameTypes |= frameType;
    LOG_FUNCTION_NAME_EXIT
};

void BaseCameraAdapter::CameraStreamBuffer::clearRefCount(CameraFrame::FrameType frameType){
    LOG_FUNCTION_NAME
    uint32_t oldSum = 0;
    bool needReturn = false;
    bool needCB = false;
    unsigned int frameTypes = 0;
    int frameIndex =CameraFrame::toFrameTypeIndex(frameType);
    dumpBuffer();
    {
        Mutex::Autolock lock(mLock);
        oldSum = mRefCountSum;
        frameTypes = mFrameTypes;
        if(mRefCount[frameIndex] > 0)
        {
            uint32_t cnt = mRefCount[frameIndex];
            mRefCountSum -= cnt;
            mRefCount[frameIndex] = 0;
            if(!mTransaction)
            {
                needCB = true;
            }
            else
            {
                mReturnInTransaction = true;
                mFrameTypesInTransaction |= frameType;
            }

            if(mRefCountSum==0 && oldSum > 0 && !mTransaction)
            {
                needReturn = true;                
                mFrameTypes = 0;
            }
        }

    }
    if(needCB)
    {    
        mStream->onReturnFrame(mBuffer, frameType, frameTypes);
    }
    if(needReturn)
    {
        returnFrame(frameTypes);                
    }
    dumpBuffer();
    LOG_FUNCTION_NAME_EXIT
};

void BaseCameraAdapter::CameraStreamBuffer::returnFrame(unsigned int frameTypes)
{
    LOG_FUNCTION_NAME

    mStream->returnFrameCb(mBuffer, frameTypes);
    LOG_FUNCTION_NAME_EXIT
};


void BaseCameraAdapter::CameraStream::setHub(CameraStreamHub *hub)
{
    LOG_FUNCTION_NAME
    mHub = hub;
    LOG_FUNCTION_NAME_EXIT
}
void BaseCameraAdapter::CameraStream::setStreamType(CameraFrame::StreamType streamType)
{
    LOG_FUNCTION_NAME
    mStreamType = streamType;
    LOG_FUNCTION_NAME_EXIT
}
CameraFrame::StreamType BaseCameraAdapter::CameraStream::getStreamType()
{
    return mStreamType;
}

sp<BaseCameraAdapter::CameraStreamBuffer> BaseCameraAdapter::CameraStream::addFrameBuffer(void *frameBuf) {
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);
    sp<CameraStreamBuffer> streamBuf = new  CameraStreamBuffer(this,frameBuf);
    mStreamBuffers.add((void *)frameBuf, streamBuf);
    LOG_FUNCTION_NAME_EXIT
    return streamBuf;

};
void BaseCameraAdapter::CameraStream::removeFrameBuffer(void *frameBuf){
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);
    ssize_t index = mStreamBuffers.indexOfKey(frameBuf);
    if(index < 0)
    {
        return ;
    }
    mStreamBuffers.removeItemsAt(index,1);
    LOG_FUNCTION_NAME_EXIT
    
};
void BaseCameraAdapter::CameraStream::removeAllFrameBuffer(){
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);
    mStreamBuffers.clear();
    LOG_FUNCTION_NAME_EXIT
};

void BaseCameraAdapter::CameraStream::clearRefCount(CameraFrame::FrameType frameType){
    LOG_FUNCTION_NAME
    size_t i = 0 ;
    Mutex::Autolock lock(mLock);

    for(i=0; i<mStreamBuffers.size(); i++)
    {
        mStreamBuffers.valueAt(i)->clearRefCount(frameType);
    }
    LOG_FUNCTION_NAME_EXIT
}

 /**
 * NEW_FEATURE: Add buffer_state_dump function,when watchdog timeout happens.
 *ActionsCode(author:liyuan, change_code)
 */
int BaseCameraAdapter::CameraStream::getStreamBuffersSize(){
    return mStreamBuffers.size();
}

uint32_t BaseCameraAdapter::CameraStream::getRefCountByFrameTypes(uint32_t types)
{
    uint32_t sum = 0;
    uint32_t mask = 1;
    size_t index= 0;
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);
    for(mask = 1; types != 0; types&=(~mask),mask<<=1)
    {
        if(types&mask)
        {
            for(index=0; index<mStreamBuffers.size(); index++)
            {
                sum += mStreamBuffers.valueAt(index)->getRefCount((CameraFrame::FrameType)mask);
            }
        }
    }

    LOG_FUNCTION_NAME_EXIT
    return sum;
}

sp<BaseCameraAdapter::CameraStreamBuffer> BaseCameraAdapter::CameraStream::getStreamBuffer(void *frameBuf){
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);
    ssize_t index = mStreamBuffers.indexOfKey(frameBuf);
    if(index < 0)
    {
        return NULL;
    }
    LOG_FUNCTION_NAME_EXIT
    return mStreamBuffers.valueAt(index);
};

void BaseCameraAdapter::CameraStream::returnFrameCb(void *frameBuf, unsigned int frameTypes)
{
    LOG_FUNCTION_NAME

    mHub->returnFrameCb(frameBuf, frameTypes, mStreamType);
    LOG_FUNCTION_NAME_EXIT
}
void BaseCameraAdapter::CameraStream::onReturnFrame(void *frameBuf, unsigned int returnFrameType, unsigned int frameTypes)
{
    LOG_FUNCTION_NAME
    mHub->onReturnFrame(frameBuf, returnFrameType, frameTypes, mStreamType);
    LOG_FUNCTION_NAME_EXIT
}

bool BaseCameraAdapter::CameraStream::validBuffer(void* frameBuf) {
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);
    ssize_t index = mStreamBuffers.indexOfKey(frameBuf);
    LOG_FUNCTION_NAME_EXIT
    return (index >= 0);

};

status_t BaseCameraAdapter::CameraStream::sendFrame(CameraFrame* frame,
    KeyedVector<void *, frame_callback> *subscribers,
    CameraFrame::FrameType frameType)
{
    size_t refCount = 0;
    status_t ret = NO_ERROR;
    frame_callback callback = NULL;
    
    sp<CameraStreamBuffer> streamBuffer ;
    LOG_FUNCTION_NAME
    Mutex::Autolock lock(mLock);
    ssize_t index = mStreamBuffers.indexOfKey(frame->mBuffer);
    if(index < 0)
    {
        return BAD_VALUE;
    }
    streamBuffer =  mStreamBuffers.valueAt(index);


    frame->mVaddr = streamBuffer->getVaddr();
    for ( size_t i = 0 ; i < subscribers->size(); i++ )
    {
        //need to inc refcount
        streamBuffer->incRefCount(frameType);

        frame->mCookie = ( void * ) subscribers->keyAt(i);
        callback = (frame_callback) subscribers->valueAt(i);

        if (!callback)
        {
            streamBuffer->decRefCount(frameType);
            CAMHAL_LOGEB("callback not set for frame type: 0x%x", frameType);
            return -EINVAL;
        }
        mLock.unlock();
        callback(frame);
        mLock.lock();
    }
    LOG_FUNCTION_NAME_EXIT

    return ret;
}


BaseCameraAdapter::CameraStreamHub::CameraStreamHub(BaseCameraAdapter *bca):mBca(bca){
    int i;
    LOG_FUNCTION_NAME
    for(i=0; i < CameraFrame::MAX_STREAM_TYPE; i++)
    {
        mStreams[i].setHub(this);
        mStreams[i].setStreamType((CameraFrame::StreamType)i);
    }
    LOG_FUNCTION_NAME_EXIT
};

BaseCameraAdapter::CameraStreamHub::~CameraStreamHub(){
    int i = 0;
    LOG_FUNCTION_NAME
    for(i=0; i < CameraFrame::MAX_FRAME_TYPE; i++)
    {
        mSubscribers[i].clear();
    }
    LOG_FUNCTION_NAME_EXIT
};

status_t BaseCameraAdapter::CameraStreamHub::subscribe(CameraFrame::FrameType frameType, frame_callback callback, void* cookie) {
    LOG_FUNCTION_NAME
    uint32_t frameTypeIndex = CameraFrame::toFrameTypeIndex(frameType); 
    if(frameTypeIndex >= CameraFrame::MAX_FRAME_TYPE)
    {
        return BAD_INDEX;
    }
    Mutex::Autolock lock(mLocks[frameTypeIndex]);
    mSubscribers[frameTypeIndex].add(cookie, callback);

    LOG_FUNCTION_NAME_EXIT
    return NO_ERROR;

};
status_t BaseCameraAdapter::CameraStreamHub::unsubscribe(CameraFrame::FrameType frameType,  void* cookie){
    LOG_FUNCTION_NAME
    uint32_t frameTypeIndex = CameraFrame::toFrameTypeIndex(frameType); 
    if(frameTypeIndex >= CameraFrame::MAX_FRAME_TYPE)
    {
        return BAD_INDEX;
    }
    Mutex::Autolock lock(mLocks[frameTypeIndex]);
    if (mSubscribers[frameTypeIndex].size() == 0)
    {
        return NO_ERROR;
    }

    mSubscribers[frameTypeIndex].removeItem(cookie);

    if(mSubscribers[frameTypeIndex].size() > 0)
    {
        return NO_ERROR;
    }
    //TODO:remove refcount
    int i =0;
    for(i=0; i < CameraFrame::MAX_STREAM_TYPE; i++)
    {
    
        mStreams[i].clearRefCount(frameType);

    }
    LOG_FUNCTION_NAME_EXIT
    return NO_ERROR;
};


uint32_t BaseCameraAdapter::CameraStreamHub::getRefCountByFrameTypes(uint32_t types)
{
    int i = 0;
    uint32_t sum = 0;
    LOG_FUNCTION_NAME
    for(i=0; i < CameraFrame::MAX_STREAM_TYPE; i++)
    {
        sum += mStreams[i].getRefCountByFrameTypes(types);
    } 
    LOG_FUNCTION_NAME_EXIT
    return sum;

}

BaseCameraAdapter::CameraStream *BaseCameraAdapter::CameraStreamHub::getStream(CameraFrame::StreamType streamType){
    LOG_FUNCTION_NAME
    if(streamType < CameraFrame::MAX_STREAM_TYPE)
    {
        return &mStreams[streamType];
    }
    else
    {
        return NULL;
    }
    LOG_FUNCTION_NAME_EXIT
};

BaseCameraAdapter::CameraStream *BaseCameraAdapter::CameraStreamHub::getStream(void * frameBuf){
    LOG_FUNCTION_NAME
    sp<CameraStreamBuffer> streamBuffer;
    streamBuffer = mStreams[CameraFrame::PREVIEW_STREAM_TYPE].getStreamBuffer(frameBuf);
    if(streamBuffer.get())
    {
        return &mStreams[CameraFrame::PREVIEW_STREAM_TYPE];
    }
    streamBuffer = mStreams[CameraFrame::IMAGE_STREAM_TYPE].getStreamBuffer(frameBuf);
    if(streamBuffer.get())
    {
        return &mStreams[CameraFrame::IMAGE_STREAM_TYPE];
    }
    LOG_FUNCTION_NAME_EXIT
    return NULL;
    
    
};
sp<BaseCameraAdapter::CameraStreamBuffer> BaseCameraAdapter::CameraStreamHub::getStreamBuffer(void * frameBuf){
    LOG_FUNCTION_NAME
    sp<CameraStreamBuffer> streamBuffer;
    streamBuffer = mStreams[CameraFrame::PREVIEW_STREAM_TYPE].getStreamBuffer(frameBuf);
    if(streamBuffer.get())
    {
        return streamBuffer;
    }
    streamBuffer = mStreams[CameraFrame::IMAGE_STREAM_TYPE].getStreamBuffer(frameBuf);
    if(streamBuffer.get())
    {
        return streamBuffer;
    }
    LOG_FUNCTION_NAME_EXIT
    return NULL;
    
    
};


status_t BaseCameraAdapter::CameraStreamHub::sendFrame(CameraFrame *frame, uint32_t types) {
    status_t ret = NO_ERROR;
    uint32_t mask;
    int index = 0;
    LOG_FUNCTION_NAME

    CameraStream *cameraStream = &mStreams[frame->mStreamType];

    if ( NULL == frame )
    {
        CAMHAL_LOGEA("Invalid CameraFrame");
        return -EINVAL;
    }
    

    for( index = 0, mask=1; index < CameraFrame::MAX_FRAME_TYPE; index++,mask <<=1)
    {
        if(mask & types )
        {
            switch( mask )
            {

            case CameraFrame::IMAGE_FRAME_SYNC:
                {
#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
                    CameraHal::PPM("Shot to Jpeg: ", &mBca->mStartCapture);
#endif
                }
                break;
            default:
                break;
            }
            ret = sendFrameToSubscribers(frame, &mSubscribers[index], (CameraFrame::FrameType)mask);

            
            types &= ~mask;
            if(!types)
            {
                break;
            }
        }//IF
    }//FOR
    LOG_FUNCTION_NAME_EXIT

EXIT:
    return ret;
};

status_t BaseCameraAdapter::CameraStreamHub::sendFrameToSubscribers(CameraFrame* frame,
    KeyedVector<void *, frame_callback> *subscribers,
    CameraFrame::FrameType frameType)
{
    size_t refCount = 0;
    status_t ret = NO_ERROR;
    frame_callback callback = NULL;
    LOG_FUNCTION_NAME

    uint32_t frameTypeIndex = CameraFrame::toFrameTypeIndex(frameType); 
    CameraStream *cameraStream = &mStreams[frame->mStreamType];

    frame->mFrameType = frameType;
    Mutex::Autolock lock(mLocks[frameTypeIndex]);
    if(subscribers->size()>0)
    {
        ret = cameraStream->sendFrame(frame, subscribers, frameType);
    }
    LOG_FUNCTION_NAME_EXIT
    

    return ret;
}

void BaseCameraAdapter::CameraStreamHub::returnFrame(void* frameBuf, CameraFrame::FrameType frameType, CameraFrame::StreamType streamType)
{
    LOG_FUNCTION_NAME
    //do not lock for return frame
    sp<CameraStreamBuffer> streamBuffer;
    if(streamType == CameraFrame::UNKNOWN_STREAM_TYPE)
    {
        streamBuffer = mStreams[CameraFrame::PREVIEW_STREAM_TYPE].getStreamBuffer(frameBuf);
        if(!streamBuffer.get())
        {
            streamBuffer = mStreams[CameraFrame::IMAGE_STREAM_TYPE].getStreamBuffer(frameBuf);
        }
    }
    else
    {
        streamBuffer = mStreams[streamType].getStreamBuffer(frameBuf);
    }
    if(!streamBuffer.get())
    {
        return;
    }
    streamBuffer->decRefCount(frameType);
    LOG_FUNCTION_NAME_EXIT

};

void BaseCameraAdapter::CameraStreamHub::returnFrameCb(void *frameBuf, unsigned int frameTypes, CameraFrame::StreamType streamType)
{
    LOG_FUNCTION_NAME
    mBca->onFillFrame(frameBuf, frameTypes, streamType);
    mBca->fillThisBuffer(frameBuf, frameTypes, streamType);
    LOG_FUNCTION_NAME_EXIT
};

void BaseCameraAdapter::CameraStreamHub::onReturnFrame(void *frameBuf,unsigned int returnFrameType, unsigned int frameTypes, CameraFrame::StreamType streamType)
{
    LOG_FUNCTION_NAME
    mBca->onReturnFrame(frameBuf, returnFrameType, frameTypes, streamType);
    LOG_FUNCTION_NAME_EXIT
};

   

BaseCameraAdapter::BaseCameraAdapter()
{
    mReleaseImageBuffersCallback = NULL;
    mEndImageCaptureCallback = NULL;
    mErrorNotifier = NULL;
    mEndCaptureData = NULL;
    mReleaseData = NULL;
    mRecording = false;

    mAdapterState = INTIALIZED_STATE;
    mNextState = INTIALIZED_STATE;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
    mStartFocus.tv_sec = 0;
    mStartFocus.tv_usec = 0;
    mStartCapture.tv_sec = 0;
    mStartCapture.tv_usec = 0;
#endif
    mStreamHub = new CameraStreamHub(this);

}

BaseCameraAdapter::~BaseCameraAdapter()
{
    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mSubscriberLock);
    mFocusSubscribers.clear();
    mShutterSubscribers.clear();
    mZoomSubscribers.clear();
    mFaceSubscribers.clear();

    delete mStreamHub;

    LOG_FUNCTION_NAME_EXIT;
}

status_t BaseCameraAdapter::registerImageReleaseCallback(release_image_buffers_callback callback, void *user_data)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    mReleaseImageBuffersCallback = callback;
    mReleaseData = user_data;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::registerEndCaptureCallback(end_image_capture_callback callback, void *user_data)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    mEndImageCaptureCallback= callback;
    mEndCaptureData = user_data;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::setErrorHandler(ErrorNotifier *errorNotifier)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NULL == errorNotifier )
    {
        CAMHAL_LOGEA("Invalid Error Notifier reference");
        ret = -EINVAL;
    }

    if ( NO_ERROR == ret )
    {
        mErrorNotifier = errorNotifier;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}
void BaseCameraAdapter::enableFrameMsgType(int32_t msgs, frame_callback callback, void* cookie)
{

    LOG_FUNCTION_NAME;
    uint32_t mask;
    for(mask = 1; msgs != 0; msgs&=~mask, mask<<=1)
    {
        if(mask&msgs)
        {
            mStreamHub->subscribe((CameraFrame::FrameType)mask, callback, cookie);
        }
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
void BaseCameraAdapter::enableMsgType(int32_t msgs, event_callback eventCb, void* cookie)
{

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mSubscriberLock);
    
    if ( CameraHalEvent::ALL_EVENTS == msgs)
    {
        //ActionsCode(author:liuyiguang, change_code)
        //mFocusSubscribers.add((int) cookie, eventCb);
        //mShutterSubscribers.add((int) cookie, eventCb);
        //mZoomSubscribers.add((int) cookie, eventCb);
        //mFaceSubscribers.add((int) cookie, eventCb);
        mFocusSubscribers.add((long) cookie, eventCb);
        mShutterSubscribers.add((long) cookie, eventCb);
        mZoomSubscribers.add((long) cookie, eventCb);
        mFaceSubscribers.add((long) cookie, eventCb);
    }
    else
    {
        CAMHAL_LOGEA("Message type subscription no supported yet!");
    }

    LOG_FUNCTION_NAME_EXIT;
}
void BaseCameraAdapter::disableFrameMsgType(int32_t msgs, void* cookie)
{

    LOG_FUNCTION_NAME;

    uint32_t mask;
    for(mask = 1; msgs != 0; msgs&=~mask, mask<<=1)
    {
        if(mask&msgs)
        {
            mStreamHub->unsubscribe((CameraFrame::FrameType)mask, cookie);
        }
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
void BaseCameraAdapter::disableMsgType(int32_t msgs, void* cookie)
{

    LOG_FUNCTION_NAME;
    
    Mutex::Autolock lock(mSubscriberLock);
    if ( CameraHalEvent::ALL_EVENTS == msgs)
    {
        //Subscribe only for focus
        //TODO: Process case by case
        //ActionsCode(author:liuyiguang, change_code)
        //mFocusSubscribers.removeItem((int) cookie);
        //mShutterSubscribers.removeItem((int) cookie);
        //mZoomSubscribers.removeItem((int) cookie);
        //mFaceSubscribers.removeItem((int) cookie);
        mFocusSubscribers.removeItem((long) cookie);
        mShutterSubscribers.removeItem((long) cookie);
        mZoomSubscribers.removeItem((long) cookie);
        mFaceSubscribers.removeItem((long) cookie);
    }
    else
    {
        CAMHAL_LOGEB("Message type 0x%x subscription no supported yet!", msgs);
    }

    LOG_FUNCTION_NAME_EXIT;
}


void BaseCameraAdapter::returnFrame(void* frameBuf, CameraFrame::FrameType frameType, CameraFrame::StreamType streamType)
{
    status_t res = NO_ERROR;
    size_t subscriberCount = 0;
    int refCount = -1;

    if ( NULL == frameBuf )
    {
        CAMHAL_LOGEA("Invalid frameBuf");
        return;
    }

    if ( NO_ERROR == res)
    {
        Mutex::Autolock lock(mReturnFrameLock);
        mStreamHub->returnFrame(frameBuf, frameType, streamType);
    }

}

void BaseCameraAdapter::onFillFrame(void* frameBuf, unsigned int frameTypes, CameraFrame::StreamType streamType)
{
}
void BaseCameraAdapter::onReturnFrame(void* frameBuf,  unsigned int returnFrameType, unsigned int frameTypes, CameraFrame::StreamType streamType)
{
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
//status_t BaseCameraAdapter::sendCommand(CameraCommands operation, int value1, int value2, int value3)
status_t BaseCameraAdapter::sendCommand(CameraCommands operation, long value1, int value2, int value3)
{
    status_t ret = NO_ERROR;
    struct timeval *refTimestamp;
    BuffersDescriptor *desc = NULL;
    CameraFrame *frame = NULL;

    LOG_FUNCTION_NAME;

    //Mutex::Autolock lock(mSendCommandLock);
    switch ( operation )
    {
    case CameraAdapter::CAMERA_USE_BUFFERS_PREVIEW:
        CAMHAL_LOGDA("Use buffers for preview");
        desc = ( BuffersDescriptor * ) value1;

        if ( NULL == desc )
        {
            CAMHAL_LOGEA("Invalid preview buffers!");
            return -EINVAL;
        }

        if ( ret == NO_ERROR )
        {
            ret = setState(operation);
        }

        if ( ret == NO_ERROR )
        {
            CameraStream *stream = mStreamHub->getStream(CameraFrame::PREVIEW_STREAM_TYPE); 
            stream->removeAllFrameBuffer();
            for ( uint32_t i = 0 ; i < desc->mMaxQueueable ; i++ )
            {
                //ActionsCode(author:liuyiguang, change_code)
                //sp<CameraStreamBuffer> streamBuf = stream->addFrameBuffer((void *)((uint32_t*)desc->mBuffers)[i]);
                sp<CameraStreamBuffer> streamBuf = stream->addFrameBuffer((void *)(long)((uint32_t*)desc->mBuffers)[i]);
                if(NULL !=  desc->mVaddrs)
                {
                    //ActionsCode(author:liuyiguang, change_code)
                    //streamBuf->setVaddr((void *)((uint32_t*)desc->mVaddrs)[i]);
                    streamBuf->setVaddr((void *)(long)((uint32_t*)desc->mVaddrs)[i]);
                }
                else
                {
                    streamBuf->setVaddr(NULL);
                }
            }
            for ( uint32_t i = desc->mMaxQueueable ; i < desc->mCount ; i++ )
            {
                //ActionsCode(author:liuyiguang, change_code)
                //sp<CameraStreamBuffer> streamBuf = stream->addFrameBuffer((void *)((uint32_t*)desc->mBuffers)[i]);
                sp<CameraStreamBuffer> streamBuf = stream->addFrameBuffer((void *)(long)((uint32_t*)desc->mBuffers)[i]);
                streamBuf->initRefCount(CameraFrame::PREVIEW_FRAME_SYNC, 1);
                if(NULL !=  desc->mVaddrs)
                {
                    //ActionsCode(author:liuyiguang, change_code)
                    //streamBuf->setVaddr((void *)((uint32_t*)desc->mVaddrs)[i]);
                    streamBuf->setVaddr((void *)(long)((uint32_t*)desc->mVaddrs)[i]);
                }
                else
                {
                    streamBuf->setVaddr(NULL);
                }
            }
        }

        if ( NULL != desc )
        {
            ret = useBuffers(CameraAdapter::CAMERA_PREVIEW,
                             desc->mBuffers,
                             desc->mCount,
                             desc->mLength,
                             desc->mMaxQueueable);
        }

        if ( ret == NO_ERROR )
        {
            ret = commitState();
        }
        else
        {
            ret |= rollbackState();
        }

        break;

    case CameraAdapter::CAMERA_USE_BUFFERS_IMAGE_CAPTURE:
        CAMHAL_LOGDA("Use buffers for image capture");
        desc = ( BuffersDescriptor * ) value1;

        if ( NULL == desc )
        {
            CAMHAL_LOGEA("Invalid capture buffers!");
            return -EINVAL;
        }

        if ( ret == NO_ERROR )
        {
            ret = setState(operation);
        }

        if ( ret == NO_ERROR )
        {
            CameraStream *stream = mStreamHub->getStream(CameraFrame::IMAGE_STREAM_TYPE); 
            stream->removeAllFrameBuffer();
            for ( uint32_t i = 0 ; i < desc->mMaxQueueable ; i++ )
            {
                //ActionsCode(author:liuyiguang, change_code)
                //sp<CameraStreamBuffer> streamBuf = stream->addFrameBuffer((void *)((uint32_t*)desc->mBuffers)[i]);
                sp<CameraStreamBuffer> streamBuf = stream->addFrameBuffer((void *)(long)((uint32_t*)desc->mBuffers)[i]);
                if(NULL !=  desc->mVaddrs)
                {
                    //ActionsCode(author:liuyiguang, change_code)
                    streamBuf->setVaddr((void *)(long)((uint32_t*)desc->mVaddrs)[i]);
                }
                else
                {
                    streamBuf->setVaddr(NULL);
                }
            }
            for ( uint32_t i = desc->mMaxQueueable ; i < desc->mCount ; i++ )
            {
                //ActionsCode(author:liuyiguang, change_code)
                //sp<CameraStreamBuffer> streamBuf = stream->addFrameBuffer((void *)((uint32_t*)desc->mBuffers)[i]);
                sp<CameraStreamBuffer> streamBuf = stream->addFrameBuffer((void *)(long)((uint32_t*)desc->mBuffers)[i]);
                streamBuf->initRefCount(CameraFrame::IMAGE_FRAME_SYNC, 1);
                if(NULL !=  desc->mVaddrs)
                {
                    //ActionsCode(author:liuyiguang, change_code)
                    //streamBuf->setVaddr((void *)((uint32_t*)desc->mVaddrs)[i]);
                    streamBuf->setVaddr((void *)(long)((uint32_t*)desc->mVaddrs)[i]);
                }
                else
                {
                    streamBuf->setVaddr(NULL);
                }
            }
            
        }

        if ( NULL != desc )
        {
            ret = useBuffers(CameraAdapter::CAMERA_IMAGE_CAPTURE,
                             desc->mBuffers,
                             desc->mCount,
                             desc->mLength,
                             desc->mMaxQueueable);
        }

        if ( ret == NO_ERROR )
        {
            ret = commitState();
        }
        else
        {
            ret |= rollbackState();
        }

        break;

    case CameraAdapter::CAMERA_START_SMOOTH_ZOOM:
        {

            if ( ret == NO_ERROR )
            {
                ret = setState(operation);
            }

            if ( ret == NO_ERROR )
            {
                ret = startSmoothZoom(value1);
            }

            if ( ret == NO_ERROR )
            {
                ret = commitState();
            }
            else
            {
                ret |= rollbackState();
            }

            break;

        }

    case CameraAdapter::CAMERA_STOP_SMOOTH_ZOOM:
        {

            if ( ret == NO_ERROR )
            {
                ret = setState(operation);
            }

            if ( ret == NO_ERROR )
            {
                ret = stopSmoothZoom();
            }

            if ( ret == NO_ERROR )
            {
                ret = commitState();
            }
            else
            {
                ret |= rollbackState();
            }

            break;

        }

    case CameraAdapter::CAMERA_START_PREVIEW:
        {

            CAMHAL_LOGDA("Start Preview");

            if ( ret == NO_ERROR )
            {
                ret = setState(operation);
            }

            if ( ret == NO_ERROR )
            {
                ret = startPreview();
            }

            if ( ret == NO_ERROR )
            {
                ret = commitState();
            }
            else
            {
                ret |= rollbackState();
            }

            break;

        }

    case CameraAdapter::CAMERA_STOP_PREVIEW:
        {

            CAMHAL_LOGDA("Stop Preview");

            if ( ret == NO_ERROR )
            {
                ret = setState(operation);
            }

            if ( ret == NO_ERROR )
            {
                ret = stopPreview();
            } 
            if ( ret == NO_ERROR )
            { 
                CameraStream *stream = mStreamHub->getStream(CameraFrame::PREVIEW_STREAM_TYPE); 
                stream->removeAllFrameBuffer(); 
            }

            if ( ret == NO_ERROR )
            {
                ret = commitState();
            }
            else
            {
                ret |= rollbackState();
            }

            break;

        }

    case CameraAdapter::CAMERA_START_VIDEO:
        {

            CAMHAL_LOGDA("Start video recording");

            if ( ret == NO_ERROR )
            {
                ret = setState(operation);
            }

            if ( ret == NO_ERROR )
            {
                ret = startVideoCapture();
            }

            if ( ret == NO_ERROR )
            {
                ret = commitState();
            }
            else
            {
                ret |= rollbackState();
            }

            break;

        }

    case CameraAdapter::CAMERA_STOP_VIDEO:
        {

            CAMHAL_LOGDA("Stop video recording");

            if ( ret == NO_ERROR )
            {
                ret = setState(operation);
            }

            if ( ret == NO_ERROR )
            {
                ret = stopVideoCapture();
            }

            if ( ret == NO_ERROR )
            {
                ret = commitState();
            }
            else
            {
                ret |= rollbackState();
            }

            break;

        }

    case CameraAdapter::CAMERA_PREVIEW_FLUSH_BUFFERS:
        {

            if ( ret == NO_ERROR )
            {
                ret = setState(operation);
            }

            if ( ret == NO_ERROR )
            {
                ret = flushBuffers();
            }

            if ( ret == NO_ERROR )
            {
                ret = commitState();
            }
            else
            {
                ret |= rollbackState();
            }

            break;

        }

    case CameraAdapter::CAMERA_START_IMAGE_CAPTURE:
        {

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

            refTimestamp = ( struct timeval * ) value1;
            if ( NULL != refTimestamp )
            {
                memcpy( &mStartCapture, refTimestamp, sizeof( struct timeval ));
            }

#endif

            if ( ret == NO_ERROR )
            {
                ret = setState(operation);
            }

            if ( ret == NO_ERROR )
            {
                ret = takePicture();
            }

            if ( ret == NO_ERROR )
            {
                ret = commitState();
            }
            else
            {
                ret |= rollbackState();
            }

            break;

        }

    case CameraAdapter::CAMERA_STOP_IMAGE_CAPTURE:
        {

            if ( ret == NO_ERROR )
            {
                ret = setState(operation);
            }

            if ( ret == NO_ERROR )
            {
                ret = stopImageCapture();
            } 
            if ( ret == NO_ERROR )
            { 
                CameraStream *stream = mStreamHub->getStream(CameraFrame::IMAGE_STREAM_TYPE); 
                stream->removeAllFrameBuffer(); 
            }

            if ( ret == NO_ERROR )
            {
                ret = commitState();
            }
            else
            {
                ret |= rollbackState();
            }

            break;

        }


    case CameraAdapter::CAMERA_PERFORM_AUTOFOCUS:

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

        refTimestamp = ( struct timeval * ) value1;
        if ( NULL != refTimestamp )
        {
            memcpy( &mStartFocus, refTimestamp, sizeof( struct timeval ));
        }

#endif

        if ( ret == NO_ERROR )
        {
            ret = setState(operation);
        }

        if ( ret == NO_ERROR )
        {
            ret = autoFocus();
        }

        if ( ret == NO_ERROR )
        {
            ret = commitState();
        }
        else
        {
            ret |= rollbackState();
        }

        break;

    case CameraAdapter::CAMERA_CANCEL_AUTOFOCUS:

        if ( ret == NO_ERROR )
        {
            ret = setState(operation);
        }

        if ( ret == NO_ERROR )
        {
            ret = cancelAutoFocus();
        }

        if ( ret == NO_ERROR )
        {
            ret = commitState();
        }
        else
        {
            ret |= rollbackState();
        }

        break;

    case CameraAdapter::CAMERA_QUERY_RESOLUTION_PREVIEW:

        if ( ret == NO_ERROR )
        {
            ret = setState(operation);
        }

        if ( ret == NO_ERROR )
        {
            int n;
            frame = ( CameraFrame * ) value1;
            n = ( int) value2;

            if ( NULL != frame )
            {
                //ActionsCode(author:liuyiguang, change_code)
                //ret = getFrameSize(frame->mWidth, frame->mHeight, n);
                ret = getFrameSize((size_t&)frame->mWidth, (size_t&)frame->mHeight, n);
            }
            else
            {
                ret = -EINVAL;
            }
        }

        if ( ret == NO_ERROR )
        {
            ret = commitState();
        }
        else
        {
            ret |= rollbackState();
        }

        break;

    case CameraAdapter::CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:

        if ( ret == NO_ERROR )
        {
            ret = setState(operation);
        }

        if ( ret == NO_ERROR )
        {
            frame = ( CameraFrame * ) value1;

            if ( NULL != frame )
            {
                ret = getPictureBufferSize(frame->mLength, value2);
            }
            else
            {
                ret = -EINVAL;
            }
        }

        if ( ret == NO_ERROR )
        {
            ret = commitState();
        }
        else
        {
            ret |= rollbackState();
        }

        break;


    case CameraAdapter::CAMERA_START_FD:

        ret = startFaceDetection();

        break;

    case CameraAdapter::CAMERA_STOP_FD:

        ret = stopFaceDetection();

        break;

    case CameraAdapter::CAMERA_SWITCH_TO_EXECUTING:
        ret = switchToExecuting();
        break;

    default:
        CAMHAL_LOGEB("Command 0x%x unsupported!", operation);
        break;
    };

    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

/**
 *
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 * BUGFIX:    Remove CAMERA_CANCEL_AUTOFOCUS.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
status_t BaseCameraAdapter::notifyFocusSubscribers(CameraHalEvent::FocusStatus status)
{
    event_callback eventCb;
    CameraHalEvent focusEvent;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    //ActionsCode(Do not chage AF_ACTIVE state, otherwise it will rewrite the state while autoFocus in use)
    //CameraAdapter::AdapterState currentState = getState();
    //if(currentState&AF_ACTIVE)
    //{
    //    sendCommand(CAMERA_CANCEL_AUTOFOCUS);
    //}   


    if ( mFocusSubscribers.size() == 0 )
    {
        CAMHAL_LOGDA("No Focus Subscribers!");
        return NO_INIT;
    }

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
    if (status == CameraHalEvent::FOCUS_STATUS_PENDING)
    {
        gettimeofday(&mStartFocus, NULL);
    }
    else
    {
        //dump the AF latency
        CameraHal::PPM("Focus finished in: ", &mStartFocus);
    }
#endif

    focusEvent.mEventData = new CameraHalEvent::CameraHalEventData();
    if ( NULL == focusEvent.mEventData.get() )
    {
        return -ENOMEM;
    }

    focusEvent.mEventType = CameraHalEvent::EVENT_FOCUS_LOCKED;
    focusEvent.mEventData->focusEvent.focusStatus = status;

    for (unsigned int i = 0 ; i < mFocusSubscribers.size(); i++ )
    {
        //ActionsCode(author:liuyiguang, change_code)
        //focusEvent.mCookie = (void *) mFocusSubscribers.keyAt(i);
        focusEvent.mCookie = (void *) (long)mFocusSubscribers.keyAt(i);
        eventCb = (event_callback) mFocusSubscribers.valueAt(i);
        eventCb ( &focusEvent );
    }

    focusEvent.mEventData.clear();

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
status_t BaseCameraAdapter::notifyShutterSubscribers()
{
    CameraHalEvent shutterEvent;
    event_callback eventCb;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( mShutterSubscribers.size() == 0 )
    {
        CAMHAL_LOGEA("No shutter Subscribers!");
        return NO_INIT;
    }

    shutterEvent.mEventData = new CameraHalEvent::CameraHalEventData();
    if ( NULL == shutterEvent.mEventData.get() )
    {
        return -ENOMEM;
    }

    shutterEvent.mEventType = CameraHalEvent::EVENT_SHUTTER;
    shutterEvent.mEventData->shutterEvent.shutterClosed = true;

    for (unsigned int i = 0 ; i < mShutterSubscribers.size() ; i++ )
    {
        //ActionsCode(author:liuyiguang, change_code)
        //shutterEvent.mCookie = ( void * ) mShutterSubscribers.keyAt(i);
        shutterEvent.mCookie = ( void *)(long) mShutterSubscribers.keyAt(i);
        eventCb = ( event_callback ) mShutterSubscribers.valueAt(i);

        CAMHAL_LOGDA("Sending shutter callback");

        eventCb ( &shutterEvent );
    }

    shutterEvent.mEventData.clear();

    LOG_FUNCTION_NAME;

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
status_t BaseCameraAdapter::notifyZoomSubscribers(int zoomIdx, bool targetReached)
{
    event_callback eventCb;
    CameraHalEvent zoomEvent;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( mZoomSubscribers.size() == 0 )
    {
        CAMHAL_LOGDA("No zoom Subscribers!");
        return NO_INIT;
    }

    zoomEvent.mEventData = new CameraHalEvent::CameraHalEventData();
    if ( NULL == zoomEvent.mEventData.get() )
    {
        return -ENOMEM;
    }

    zoomEvent.mEventType = CameraHalEvent::EVENT_ZOOM_INDEX_REACHED;
    zoomEvent.mEventData->zoomEvent.currentZoomIndex = zoomIdx;
    zoomEvent.mEventData->zoomEvent.targetZoomIndexReached = targetReached;

    for (unsigned int i = 0 ; i < mZoomSubscribers.size(); i++ )
    {
        //ActionsCode(author:liuyiguang, change_code)
        //zoomEvent.mCookie = (void *) mZoomSubscribers.keyAt(i);
        zoomEvent.mCookie = (void *)(long) mZoomSubscribers.keyAt(i);
        eventCb = (event_callback) mZoomSubscribers.valueAt(i);

        eventCb ( &zoomEvent );
    }

    zoomEvent.mEventData.clear();

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
status_t BaseCameraAdapter::notifyFaceSubscribers(sp<CameraFDResult> &faces)
{
    event_callback eventCb;
    CameraHalEvent faceEvent;
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( mFaceSubscribers.size() == 0 )
    {
        CAMHAL_LOGDA("No face detection subscribers!");
        return NO_INIT;
    }

    faceEvent.mEventData = new CameraHalEvent::CameraHalEventData();
    if ( NULL == faceEvent.mEventData.get() )
    {
        return -ENOMEM;
    }

    faceEvent.mEventType = CameraHalEvent::EVENT_FACE;
    faceEvent.mEventData->faceEvent = faces;

    for (unsigned int i = 0 ; i < mFaceSubscribers.size(); i++ )
    {
        //ActionsCode(author:liuyiguang, change_code)
        //faceEvent.mCookie = (void *) mFaceSubscribers.keyAt(i);
        faceEvent.mCookie = (void *) (long)mFaceSubscribers.keyAt(i);
        eventCb = (event_callback) mFaceSubscribers.valueAt(i);

        eventCb ( &faceEvent );
    }

    faceEvent.mEventData.clear();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}
status_t BaseCameraAdapter::startTransaction(void *frame)
{
    status_t ret = NO_ERROR;
    if ( NULL == frame )
    {
        CAMHAL_LOGEA("Invalid CameraFrame");
        return -EINVAL;
    }

    sp<CameraStreamBuffer> streamBuf = mStreamHub->getStreamBuffer(frame);
    if(streamBuf != NULL)
    {
        streamBuf->startTransaction();
    }

EXIT:
    return ret;
}
status_t BaseCameraAdapter::endTransaction(void *frame)
{
    status_t ret = NO_ERROR;
    if ( NULL == frame )
    {
        CAMHAL_LOGEA("Invalid CameraFrame");
        return -EINVAL;
    }

    sp<CameraStreamBuffer> streamBuf = mStreamHub->getStreamBuffer(frame);
    if(streamBuf != NULL)
    {
        streamBuf->endTransaction();
    }

EXIT:
    return ret;
}

status_t BaseCameraAdapter::sendFrameToSubscribers(CameraFrame *frame, uint32_t types)
{
    status_t ret = NO_ERROR;
    unsigned int mask;

    if ( NULL == frame )
    {
        CAMHAL_LOGEA("Invalid CameraFrame");
        return -EINVAL;
    }

    mStreamHub->sendFrame(frame, types);

EXIT:
    return ret;
}


status_t BaseCameraAdapter::startVideoCapture()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    //If the capture is already ongoing, return from here.
    if ( mRecording )
    {
        ret = NO_INIT;
    }


    if ( NO_ERROR == ret )
    {

        mRecording = true;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::stopVideoCapture()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( !mRecording )
    {
        ret = NO_INIT;
    }

    if ( NO_ERROR == ret )
    {
        mRecording = false;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

//-----------------Stub implementation of the interface ------------------------------

status_t BaseCameraAdapter::takePicture()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::stopImageCapture()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}



status_t BaseCameraAdapter::autoFocus()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    notifyFocusSubscribers(CameraHalEvent::FOCUS_STATUS_FAIL);

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::cancelAutoFocus()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::startSmoothZoom(int targetIdx)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::stopSmoothZoom()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::startPreview()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::stopPreview()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::useBuffers(CameraMode mode, void* bufArr, int num, size_t length, unsigned int queueable)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::fillThisBuffer(void* frameBuf, unsigned int frameTypes, CameraFrame::StreamType streamType)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::getFrameSize(size_t &width, size_t &height, int num)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::getFrameDataSize(size_t &dataFrameSize, size_t bufferCount)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::getPictureBufferSize(size_t &length,  size_t bufferCount)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::startFaceDetection()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::stopFaceDetection()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::switchToExecuting()
{
    status_t ret = NO_ERROR;
    LOG_FUNCTION_NAME;
    LOG_FUNCTION_NAME_EXIT;
    return ret;
}

status_t BaseCameraAdapter::setState(CameraCommands operation)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    mLock.lock();

    switch ( mAdapterState&(~(AF_ACTIVE|ZOOM_ACTIVE)) )
    {

    case INTIALIZED_STATE:

        switch ( operation )
        {

        case CAMERA_USE_BUFFERS_PREVIEW:
            CAMHAL_LOGDB("Adapter state switch INTIALIZED_STATE->LOADED_PREVIEW_STATE event = 0x%x",
                         operation);
            mNextState = LOADED_PREVIEW_STATE;
            break;

            //These events don't change the current state
        case CAMERA_QUERY_RESOLUTION_PREVIEW:
        case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
            CAMHAL_LOGDB("Adapter state switch INTIALIZED_STATE->INTIALIZED_STATE event = 0x%x",
                         operation);
            mNextState = INTIALIZED_STATE;
            break;

        default:
            ret = INVALID_OPERATION;
            break;

        }

        break;

    case LOADED_PREVIEW_STATE:

        switch ( operation )
        {

        case CAMERA_START_PREVIEW:
            CAMHAL_LOGDB("Adapter state switch LOADED_PREVIEW_STATE->PREVIEW_STATE event = 0x%x",
                         operation);
            mNextState = PREVIEW_STATE ;
            break;

        case CAMERA_STOP_PREVIEW:
            CAMHAL_LOGDB("Adapter state switch LOADED_PREVIEW_STATE->INTIALIZED_STATE event = 0x%x",
                         operation);
            mNextState = INTIALIZED_STATE;
            break;

            //These events don't change the current state
        case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
            CAMHAL_LOGDB("Adapter state switch LOADED_PREVIEW_STATE->LOADED_PREVIEW_STATE event = 0x%x",
                         operation);
            mNextState = LOADED_PREVIEW_STATE;
            break;

        default:
            ret = INVALID_OPERATION;
            break;

        }

        break;

    case PREVIEW_STATE:

        switch ( operation )
        {

        case CAMERA_STOP_PREVIEW:
            if(mAdapterState&(AF_ACTIVE|ZOOM_ACTIVE))
            {
                CAMHAL_LOGEB("Adapter state switch PREVIEW_ACTIVE Invalid Op! event = 0x%x",
                    operation);
                ret = INVALID_OPERATION;
            }
            else
            {
                CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->INTIALIZED_STATE event = 0x%x",
                    operation);
                mNextState = INTIALIZED_STATE;
            }
            break;

        case CAMERA_USE_BUFFERS_IMAGE_CAPTURE:
            CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->LOADED_CAPTURE_STATE event = 0x%x",
                         operation);
            mNextState = (CameraAdapter::AdapterState)(LOADED_CAPTURE_STATE|(mAdapterState&(AF_ACTIVE|ZOOM_ACTIVE)));
            break;

        case CAMERA_START_VIDEO:
            CAMHAL_LOGDB("Adapter state switch PREVIEW_STATE->VIDEO_STATE event = 0x%x",
                         operation);
            mNextState = (CameraAdapter::AdapterState)(VIDEO_STATE|(mAdapterState&(AF_ACTIVE|ZOOM_ACTIVE)));
            break;

        case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
            CAMHAL_LOGDB("Adapter state switch PREVIEW_ACTIVE->PREVIEW_ACTIVE event = 0x%x",
                         operation);
            mNextState = (CameraAdapter::AdapterState)(PREVIEW_STATE|(mAdapterState&(AF_ACTIVE|ZOOM_ACTIVE)));
            break;

        default:
            ret = INVALID_OPERATION;
            break;

        }

        break;

    case LOADED_CAPTURE_STATE:

        switch ( operation )
        {

        case CAMERA_START_IMAGE_CAPTURE:
            CAMHAL_LOGDB("Adapter state switch LOADED_CAPTURE_STATE->CAPTURE_STATE event = 0x%x",
                         operation);
            mNextState = (CameraAdapter::AdapterState)(CAPTURE_STATE|(mAdapterState&(AF_ACTIVE|ZOOM_ACTIVE)));
            break;


        default:
            ret = INVALID_OPERATION;
            break;

        }

        break;

    case CAPTURE_STATE:

        switch ( operation )
        {
        case CAMERA_STOP_IMAGE_CAPTURE:
            CAMHAL_LOGDB("Adapter state switch CAPTURE_STATE->PREVIEW_STATE event = 0x%x",
                         operation);
            mNextState = (CameraAdapter::AdapterState)(PREVIEW_STATE|(mAdapterState&(AF_ACTIVE|ZOOM_ACTIVE)));
            break;

        default:
            ret = INVALID_OPERATION;
            break;

        }

        break;


    case VIDEO_STATE:

        switch ( operation )
        {

        case CAMERA_STOP_VIDEO:
            CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->PREVIEW_STATE event = 0x%x",
                         operation);
            mNextState = (CameraAdapter::AdapterState)(PREVIEW_STATE|(mAdapterState&(AF_ACTIVE|ZOOM_ACTIVE)));
            break;

        case CAMERA_USE_BUFFERS_IMAGE_CAPTURE:
            CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->VIDEO_LOADED_CAPTURE_STATE event = 0x%x",
                         operation);
            mNextState = (CameraAdapter::AdapterState)(VIDEO_LOADED_CAPTURE_STATE|(mAdapterState&(AF_ACTIVE|ZOOM_ACTIVE)));
            break;

        case CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE:
            CAMHAL_LOGDB("Adapter state switch VIDEO_STATE->VIDEO_STATE event = 0x%x",
                         operation);
            mNextState = (CameraAdapter::AdapterState)(VIDEO_STATE|(mAdapterState&(AF_ACTIVE|ZOOM_ACTIVE)));
            break;

        default:
            ret = INVALID_OPERATION;
            break;

        }

        break;


    case VIDEO_LOADED_CAPTURE_STATE:

        switch ( operation )
        {

        case CAMERA_START_IMAGE_CAPTURE:
            CAMHAL_LOGDB("Adapter state switch LOADED_CAPTURE_STATE->CAPTURE_STATE event = 0x%x",
                         operation);
            mNextState = (CameraAdapter::AdapterState)(VIDEO_CAPTURE_STATE|(mAdapterState&(AF_ACTIVE|ZOOM_ACTIVE)));
            break;

        default:
            ret = INVALID_OPERATION;
            break;

        }

        break;

    case VIDEO_CAPTURE_STATE:

        switch ( operation )
        {
        case CAMERA_STOP_IMAGE_CAPTURE:
            CAMHAL_LOGDB("Adapter state switch CAPTURE_STATE->PREVIEW_STATE event = 0x%x",
                         operation);
            mNextState = (CameraAdapter::AdapterState)(VIDEO_STATE|(mAdapterState&(AF_ACTIVE|ZOOM_ACTIVE)));
            break;

        default:
            ret = INVALID_OPERATION;
            break;

        }

        break;



    default:
        CAMHAL_LOGEA("Invalid Adapter state!");
        ret = INVALID_OPERATION;
    }

    if(ret != INVALID_OPERATION)
    {
        goto exit;
    }


    if(mAdapterState&PREVIEW_ACTIVE)
    {
        if(mAdapterState&AF_ACTIVE)
        {
            //AF state
            switch ( operation )
            {

            case CAMERA_CANCEL_AUTOFOCUS:
                
                mNextState = (CameraAdapter::AdapterState)(mAdapterState&(~AF_ACTIVE));
                CAMHAL_LOGDB("Adapter state switch %d->%d event = 0x%x",
                    mAdapterState,mNextState, operation); 
                ret = NO_ERROR;
                break;

            default:
                ret = INVALID_OPERATION;
                break;

            }
        }
        else
        {
            //not AF state
            switch ( operation )
            {
            case CAMERA_PERFORM_AUTOFOCUS:
                mNextState = (CameraAdapter::AdapterState)(mAdapterState|AF_ACTIVE);
                CAMHAL_LOGDB("Adapter state switch %d->%d event = 0x%x",
                    mAdapterState,mNextState, operation); 
                ret = NO_ERROR;
                break;


            default:
                ret = INVALID_OPERATION;
                break;

            }
        }

        if(ret != INVALID_OPERATION)
        {
            goto exit;
        }


        if( mAdapterState&ZOOM_ACTIVE)
        {
            // zoom state
            switch ( operation )
            {

            case CAMERA_STOP_SMOOTH_ZOOM:
                mNextState = (CameraAdapter::AdapterState)(mAdapterState&(~ZOOM_ACTIVE));
                CAMHAL_LOGDB("Adapter state switch %d->%d event = 0x%x",
                    mAdapterState,mNextState, operation); 
                ret = NO_ERROR;
                break;

            default:
                ret = INVALID_OPERATION;
                break;

            }         
        }
        else
        {
            //not zoom state
            switch ( operation )
            {

            case CAMERA_START_SMOOTH_ZOOM:
                mNextState = (CameraAdapter::AdapterState)(mAdapterState|ZOOM_ACTIVE);
                CAMHAL_LOGDB("Adapter state switch %d->%d event = 0x%x",
                    mAdapterState,mNextState, operation); 
                ret = NO_ERROR;
                break;

            default:
                ret = INVALID_OPERATION;
                break;

            }         
        }
    }

exit:
    if(ret != NO_ERROR)
    {
        CAMHAL_LOGDB("Adapter state switch %d Invalid Op! event = 0x%x",
           mAdapterState, operation);
    }
    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::rollbackToInitializedState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    while ((getState() != INTIALIZED_STATE) && (ret == NO_ERROR))
    {
        ret = rollbackToPreviousState();
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}
status_t BaseCameraAdapter::rollbackToPreviewState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    while ((getState() > PREVIEW_STATE) && (ret == NO_ERROR))
    {
        CAMHAL_LOGDB("rollbackToPreviewState 0x%x",getState());
        ret = rollbackToPreviousState();
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::rollbackToPreviousState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    CameraAdapter::AdapterState currentState = getState();

    if(currentState&ZOOM_ACTIVE)
    {
        ret = sendCommand(CAMERA_STOP_SMOOTH_ZOOM);
    }    
    else if(currentState&AF_ACTIVE)
    {
        ret = sendCommand(CAMERA_CANCEL_AUTOFOCUS);
    }
    else
    {
        switch (currentState)
        {
        case INTIALIZED_STATE:
            return NO_ERROR;

        case PREVIEW_STATE:
            ret = sendCommand(CAMERA_STOP_PREVIEW);
            break;

        case CAPTURE_STATE:
            ret = sendCommand(CAMERA_STOP_IMAGE_CAPTURE);
            break;

        case VIDEO_STATE:
            ret = sendCommand(CAMERA_STOP_VIDEO);
            break;

        case VIDEO_CAPTURE_STATE:
            ret = sendCommand(CAMERA_STOP_IMAGE_CAPTURE);
            break;

        default:
            CAMHAL_LOGEA("Invalid Adapter state!");
            ret = INVALID_OPERATION;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

//State transition finished successfully.
//Commit the state and unlock the adapter state.
status_t BaseCameraAdapter::commitState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    mAdapterState = mNextState;

    mLock.unlock();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::rollbackState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    mNextState = mAdapterState;

    mLock.unlock();

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

// getNextState() and getState()
// publicly exposed functions to retrieve the adapter states
// please notice that these functions are locked
CameraAdapter::AdapterState BaseCameraAdapter::getState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mLock);

    LOG_FUNCTION_NAME_EXIT;

    return mAdapterState;
}

CameraAdapter::AdapterState BaseCameraAdapter::getNextState()
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    Mutex::Autolock lock(mLock);

    LOG_FUNCTION_NAME_EXIT;

    return mNextState;
}

// getNextState() and getState()
// internal protected functions to retrieve the adapter states
// please notice that these functions are NOT locked to help
// internal functions query state in the middle of state
// transition
status_t BaseCameraAdapter::getState(AdapterState &state)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    state = mAdapterState;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

status_t BaseCameraAdapter::getNextState(AdapterState &state)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    state = mNextState;

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

void BaseCameraAdapter::onOrientationEvent(uint32_t orientation, uint32_t tilt)
{
    LOG_FUNCTION_NAME;
    LOG_FUNCTION_NAME_EXIT;
}
//-----------------------------------------------------------------------------



};

/*--------------------Camera Adapter Class ENDS here-----------------------------*/

