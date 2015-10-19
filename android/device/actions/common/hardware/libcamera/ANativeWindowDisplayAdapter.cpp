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

#include "ANativeWindowDisplayAdapter.h"
#include <OMX_IVCommon.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include "Converters.h"

namespace android
{

///Constant declarations
///@todo Check the time units
const int ANativeWindowDisplayAdapter::DISPLAY_TIMEOUT = 1000;  // seconds

//Suspends buffers after given amount of failed dq's
const int ANativeWindowDisplayAdapter::FAILED_DQS_TO_SUSPEND = 3;


/*--------------------ANativeWindowDisplayAdapter Class STARTS here-----------------------------*/


/**
 * Display Adapter class STARTS here..
 */
ANativeWindowDisplayAdapter::ANativeWindowDisplayAdapter():mDisplayThread(NULL),
    mDisplayState(ANativeWindowDisplayAdapter::DISPLAY_INIT),
    mDisplayEnabled(false),
    mBufferCount(0)



{
    LOG_FUNCTION_NAME;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    mShotToShot = false;
    mStartCapture.tv_sec = 0;
    mStartCapture.tv_usec = 0;
    mStandbyToShot.tv_sec = 0;
    mStandbyToShot.tv_usec = 0;
    mMeasureStandby = false;
#endif

    mPixelFormat = NULL;
    mBufferHandleMap = NULL;
    mGrallocHandleMap = NULL;
    mOffsetsMap = NULL;
    mFrameProvider = NULL;
    mANativeWindow = NULL;
    mVaddrMap = NULL;

    mFrameWidth = 0;
    mFrameHeight = 0;
    mPreviewWidth = 0;
    mPreviewHeight = 0;

    mSuspend = false;
    mFailedDQs = 0;

    mPaused = false;
    mXOff = -1;
    mYOff = -1;
    mFirstInit = false;

    mFD = -1;
#ifdef CAMERA_DISPLAY_RGB565
    mConvBuf = NULL;
#endif

#ifdef CAMERA_FRAME_STAT
    statInit();
#endif    
    LOG_FUNCTION_NAME_EXIT;
}

ANativeWindowDisplayAdapter::~ANativeWindowDisplayAdapter()
{
    Semaphore sem;
    ActUtils::Message msg;

    LOG_FUNCTION_NAME;

    ///If Frame provider exists
    if (mFrameProvider)
    {
        // Unregister with the frame provider
        mFrameProvider->disableFrameNotification(CameraFrame::ALL_FRAMES);
        delete mFrameProvider;
        mFrameProvider = NULL;
    }

    ///The ANativeWindow object will get destroyed here
    destroy();

    ///If Display thread exists
    if(mDisplayThread.get())
    {
        ///Kill the display thread
        sem.Create();
        msg.command = DisplayThread::DISPLAY_EXIT;

        // Send the semaphore to signal once the command is completed
        msg.arg1 = &sem;

        ///Post the message to display thread
        mDisplayThread->msgQ().put(&msg);

        ///Wait for the ACK - implies that the thread is now started and waiting for frames
        sem.Wait();

        // Exit and cleanup the thread
        mDisplayThread->requestExitAndWait();

        // Delete the display thread
        mDisplayThread.clear();
    }

#ifdef CAMERA_DISPLAY_RGB565
    if(mConvBuf != NULL)
    {
        free(mConvBuf);
        mConvBuf = NULL;
    }
#endif

    LOG_FUNCTION_NAME_EXIT;

}

status_t ANativeWindowDisplayAdapter::initialize()
{
    LOG_FUNCTION_NAME;

    ///Create the display thread
    mDisplayThread = new DisplayThread(this);
    if ( !mDisplayThread.get() )
    {
        CAMHAL_LOGEA("Couldn't create display thread");
        LOG_FUNCTION_NAME_EXIT;
        return NO_MEMORY;
    }

    ///Start the display thread
    status_t ret = mDisplayThread->run("DisplayThread", ANDROID_PRIORITY_HIGHEST);
    //status_t ret = mDisplayThread->run("DisplayThread", PRIORITY_URGENT_DISPLAY);
    if ( ret != NO_ERROR )
    {
        CAMHAL_LOGEA("Couldn't run display thread");
        LOG_FUNCTION_NAME_EXIT;
        return ret;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

int ANativeWindowDisplayAdapter::setPreviewWindow(preview_stream_ops_t* window)
{
    LOG_FUNCTION_NAME;
    ///Note that Display Adapter cannot work without a valid window object
    if ( !window)
    {
        CAMHAL_LOGEA("NULL window object passed to DisplayAdapter");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }
    /*
        window is a series of nativewindow operation interface, not atually nativewindow
        when window is updated, window ops is still the same,
        it 's not correct to compare window ops to decide when window is updated.
    */

#if 0
    if ( window == mANativeWindow )
    {
        return ALREADY_EXISTS;
    }
#endif

    ///Destroy the existing window object, if it exists
    destroy();

    ///Move to new window obj
    mANativeWindow = window;

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

int ANativeWindowDisplayAdapter::setFrameProvider(FrameNotifier *frameProvider)
{
    LOG_FUNCTION_NAME;

    // Check for NULL pointer
    if ( !frameProvider )
    {
        CAMHAL_LOGEA("NULL passed for frame provider");
        LOG_FUNCTION_NAME_EXIT;
        return BAD_VALUE;
    }

    //Release any previous frame providers
    if ( NULL != mFrameProvider )
    {
        delete mFrameProvider;
    }

    /** Dont do anything here, Just save the pointer for use when display is
         actually enabled or disabled
    */
    mFrameProvider = new FrameProvider(frameProvider, this, frameCallbackRelay);

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

int ANativeWindowDisplayAdapter::setErrorHandler(ErrorNotifier *errorNotifier)
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

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

status_t ANativeWindowDisplayAdapter::setSnapshotTimeRef(struct timeval *refTime)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    if ( NULL != refTime )
    {
        Mutex::Autolock lock(mLock);
        memcpy(&mStartCapture, refTime, sizeof(struct timeval));
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}

#endif


int ANativeWindowDisplayAdapter::enableDisplay(int width, int height, struct timeval *refTime, S3DParameters *s3dParams)
{
    Semaphore sem;
    ActUtils::Message msg;

    LOG_FUNCTION_NAME;

    if ( mDisplayEnabled )
    {
        CAMHAL_LOGDA("Display is already enabled");
        LOG_FUNCTION_NAME_EXIT;

        return NO_ERROR;
    }

#if 0 //TODO: s3d is not part of bringup...will reenable
    if (s3dParams)
        mOverlay->set_s3d_params(s3dParams->mode, s3dParams->framePacking,
                                 s3dParams->order, s3dParams->subSampling);
#endif

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    if ( NULL != refTime )
    {
        Mutex::Autolock lock(mLock);
        memcpy(&mStandbyToShot, refTime, sizeof(struct timeval));
        mMeasureStandby = true;
    }

#endif

    //Send START_DISPLAY COMMAND to display thread. Display thread will start and then wait for a message
    sem.Create();
    msg.command = DisplayThread::DISPLAY_START;

    // Send the semaphore to signal once the command is completed
    msg.arg1 = &sem;

    ///Post the message to display thread
    mDisplayThread->msgQ().put(&msg);

    ///Wait for the ACK - implies that the thread is now started and waiting for frames
    sem.Wait();

    // Register with the frame provider for frames
    if(mFrameProvider != NULL)
    {
        mFrameProvider->enableFrameNotification(CameraFrame::PREVIEW_FRAME_SYNC);
    }

    mDisplayEnabled = true;
    mPreviewWidth = width;
    mPreviewHeight = height;

    CAMHAL_LOGVB("mPreviewWidth = %d mPreviewHeight = %d", mPreviewWidth, mPreviewHeight);

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

int ANativeWindowDisplayAdapter::disableDisplay(bool cancel_buffer)
{
    status_t ret = NO_ERROR;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();

    LOG_FUNCTION_NAME;

    if(!mDisplayEnabled)
    {
        CAMHAL_LOGDA("Display is already disabled");
        LOG_FUNCTION_NAME_EXIT;
        return ALREADY_EXISTS;
    }

    // Unregister with the frame provider here
    if(mFrameProvider != NULL)
    {
        mFrameProvider->disableFrameNotification(CameraFrame::PREVIEW_FRAME_SYNC);
    }

    if ( NULL != mDisplayThread.get() )
    {
        //Send STOP_DISPLAY COMMAND to display thread. Display thread will stop and dequeue all messages
        // and then wait for message
        Semaphore sem;
        sem.Create();
        ActUtils::Message msg;
        msg.command = DisplayThread::DISPLAY_STOP;

        // Send the semaphore to signal once the command is completed
        msg.arg1 = &sem;

        ///Post the message to display thread
        mDisplayThread->msgQ().put(&msg);

        ///Wait for the ACK for display to be disabled

        sem.Wait();

    }

    Mutex::Autolock lock(mLock);
    {
        ///Reset the display enabled flag
        mDisplayEnabled = false;

        ///Reset the offset values
        mXOff = -1;
        mYOff = -1;

        ///Reset the frame width and height values
        mFrameWidth =0;
        mFrameHeight = 0;
        mPreviewWidth = 0;
        mPreviewHeight = 0;

        if(cancel_buffer)
        {
            // Return the buffers to ANativeWindow here, the mFramesWithCameraAdapterMap is also cleared inside
            returnBuffersToWindow();
        }
        else
        {
            mANativeWindow = NULL;
            // Clear the frames with camera adapter map
            mFramesWithCameraAdapterMap.clear();
        }


    }

#ifdef CAMERA_FRAME_STAT
    statSaveInfo();
#endif

    LOG_FUNCTION_NAME_EXIT;

    return NO_ERROR;
}

status_t ANativeWindowDisplayAdapter::pauseDisplay(bool pause)
{
    status_t ret = NO_ERROR;

    LOG_FUNCTION_NAME;

    {
        Mutex::Autolock lock(mLock);
        mPaused = pause;
    }

    LOG_FUNCTION_NAME_EXIT;

    return ret;
}


void ANativeWindowDisplayAdapter::destroy()
{
    LOG_FUNCTION_NAME;

    ///Check if the display is disabled, if not disable it
    if ( mDisplayEnabled )
    {
        CAMHAL_LOGDA("WARNING: Calling destroy of Display adapter when display enabled. Disabling display..");
        disableDisplay(false);
    }

    mBufferCount = 0;

    LOG_FUNCTION_NAME_EXIT;
}

/**
 *
 * BUGFIX:  Resolve the blurred screen problem in the resolution 176x144 formats YV12 and NV21, which was found in the cts verifier test step.
 * MERGEFIX:  Fix for 32/64 bits enviroment.
 *
 ************************************
 *      
 * ActionsCode(author:liuyiguang, change_code)
 */
// Implementation of inherited interfaces
void* ANativeWindowDisplayAdapter::allocateBuffer(int width, int height, const char* format, int &bytes, int numBufs)
{
    LOG_FUNCTION_NAME;
    status_t err;
    int i = -1;
    const int lnumBufs = numBufs;
    int undequeued = 0;

    if ( NULL != mGrallocHandleMap )
    {
        delete [] mGrallocHandleMap;
        mGrallocHandleMap = NULL;
    }

    if( mBufferHandleMap != NULL)
    {
        delete [] mBufferHandleMap;
        mBufferHandleMap = NULL;
    }
    if(NULL != mVaddrMap)
    {
        delete[]  mVaddrMap;
        mVaddrMap = NULL;
    }


    mBufferHandleMap = new buffer_handle_t*[lnumBufs];
    mGrallocHandleMap = new buffer_handle_t[lnumBufs];
    mVaddrMap = new void *[lnumBufs];
    
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    Rect bounds;
    CameraFrame::FrameFormat cameraformat;

    cameraformat = (CameraFrame::FrameFormat)CameraFrame::getFrameFormat(format);

    if ( NULL == mANativeWindow )
    {
        return NULL;
    }
    CAMHAL_LOGDB("allocateBuffer size=%dx%d", width, height);

    // Set gralloc usage bits for window.
    err = mANativeWindow->set_usage(mANativeWindow, CAMHAL_GRALLOC_USAGE);
    if (err != 0)
    {
        CAMHAL_LOGEB("native_window_set_usage failed: %s (%d)", strerror(-err), -err);

        if ( ENODEV == err )
        {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mANativeWindow = NULL;
        }

        return NULL;
    }

    CAMHAL_LOGDB("Number of buffers set to ANativeWindow %d", numBufs);
    ///Set the number of buffers needed for camera preview
    err = mANativeWindow->set_buffer_count(mANativeWindow, numBufs);
    if (err != 0)
    {
        CAMHAL_LOGEB("native_window_set_buffer_count failed: %s (%d)", strerror(-err), -err);

        if ( ENODEV == err )
        {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mANativeWindow = NULL;
        }

        return NULL;
    }
    CAMHAL_LOGDB("Configuring %d buffers for ANativeWindow", numBufs);
    mBufferCount = numBufs;//


    // Set window geometry
    //ActionsCode(author:liuyiguang, change_code)
    err = mANativeWindow->set_buffers_geometry(
              mANativeWindow,
              width,
              height,
#ifdef CAMERA_DISPLAY_RGB565
              HAL_PIXEL_FORMAT_RGB_565
#else
              CameraFrame::getHalFormat(format)
#endif
              );
//    err = mANativeWindow->set_buffers_geometry(
//              mANativeWindow,
//              SIZE_ALIGN_UP_32(width),
//              height,
//#ifdef CAMERA_DISPLAY_RGB565
//              HAL_PIXEL_FORMAT_RGB_565
//#else
//              CameraFrame::getHalFormat(format)
//#endif
//              );
    if (err != 0)
    {
        CAMHAL_LOGEB("native_window_set_buffers_geometry failed: %s (%d)", strerror(-err), -err);

        if ( ENODEV == err )
        {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mANativeWindow = NULL;
        }

        return NULL;
    }

    ///We just return the buffers from ANativeWindow, if the width and height are same, else (vstab, vnf case)
    ///re-allocate buffers using ANativeWindow and then get them
    ///@todo - Re-allocate buffers for vnf and vstab using the width, height, format, numBufs etc
    if ( mBufferHandleMap == NULL )
    {
        CAMHAL_LOGEA("Couldn't create array for ANativeWindow buffers");
        LOG_FUNCTION_NAME_EXIT;
        return NULL;
    }

    mANativeWindow->get_min_undequeued_buffer_count(mANativeWindow, &undequeued);

    for ( i=0; i < mBufferCount; i++ )
    {
        buffer_handle_t* handle;
        int stride;  // dummy variable to get stride
        // TODO(XXX): Do we need to keep stride information in camera hal?

        err = mANativeWindow->dequeue_buffer(mANativeWindow, (buffer_handle_t**) &handle, &stride);

        if (err != 0)
        {
            CAMHAL_LOGEB("dequeueBuffer failed: %s (%d)", strerror(-err), -err);

            if ( ENODEV == err )
            {
                CAMHAL_LOGEA("Preview surface abandoned!");
                mANativeWindow = NULL;
            }

            goto fail;
        }
        mBufferHandleMap[i] =  handle;
        mGrallocHandleMap[i] = *handle;
        //ActionsCode(author:liuyiguang, change_code)
        //mFramesWithCameraAdapterMap.add((int) mGrallocHandleMap[i], i);
        mFramesWithCameraAdapterMap.add((long) mGrallocHandleMap[i], i);

        bytes =  CameraFrame::getFrameLength(cameraformat, width, height);

    }

    // lock the initial queueable buffers
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = width;
    bounds.bottom = height;

    for( i = 0;  i < mBufferCount-undequeued; i++ )
    {
        void *vaddr;

        mANativeWindow->lock_buffer(mANativeWindow, mBufferHandleMap[i]);

        mapper.lock((buffer_handle_t) mGrallocHandleMap[i], CAMHAL_GRALLOC_LOCK_USAGE, bounds, &vaddr);
        mVaddrMap[i] = vaddr;
        clearUnalignedBuf((unsigned char *)vaddr, width, height, bytes,cameraformat);
        mapper.unlock((buffer_handle_t) mGrallocHandleMap[i]);
    }

    // return the rest of the buffers back to ANativeWindow
    for(i = (mBufferCount-undequeued); i >= 0 && i < mBufferCount; i++)
    {
        err = mANativeWindow->cancel_buffer(mANativeWindow, mBufferHandleMap[i]);
        if (err != 0)
        {
            CAMHAL_LOGEB("cancel_buffer failed: %s (%d)", strerror(-err), -err);

            if ( ENODEV == err )
            {
                CAMHAL_LOGEA("Preview surface abandoned!");
                mANativeWindow = NULL;
            }

            goto fail;
        }
        //ActionsCode(author:liuyiguang, change_code)
        //mFramesWithCameraAdapterMap.removeItem((int) mGrallocHandleMap[i]);
        mFramesWithCameraAdapterMap.removeItem((long) mGrallocHandleMap[i]);
        //LOCK UNLOCK TO GET YUV POINTERS
        void *vaddr;
        mapper.lock((buffer_handle_t) mGrallocHandleMap[i], CAMHAL_GRALLOC_LOCK_USAGE, bounds, &vaddr);
        mVaddrMap[i] = vaddr;
        clearUnalignedBuf((unsigned char *)vaddr,width,height,bytes,cameraformat);
        mapper.unlock((buffer_handle_t) mGrallocHandleMap[i]);
    }

    mFirstInit = true;
    mPixelFormat = getPixFormatConstant(format);
    mFrameWidth = width;
    mFrameHeight = height;

    return mGrallocHandleMap;

fail:
    // need to cancel buffers if any were dequeued
    for (int start = 0; start < i && i > 0; start++)
    {
        int err = mANativeWindow->cancel_buffer(mANativeWindow, mBufferHandleMap[start]);
        if (err != 0)
        {
            CAMHAL_LOGEB("cancelBuffer failed w/ error 0x%08x", err);
            break;
        }
        //ActionsCode(author:liuyiguang, change_code)
        //mFramesWithCameraAdapterMap.removeItem((int) mGrallocHandleMap[start]);
        mFramesWithCameraAdapterMap.removeItem((long) mGrallocHandleMap[start]);
    }

    freeBuffer(mGrallocHandleMap);

    CAMHAL_LOGEA("Error occurred, performing cleanup");

    if ( NULL != mErrorNotifier.get() )
    {
        mErrorNotifier->errorNotify(CAMERA_ERROR_FATAL);
    }

    LOG_FUNCTION_NAME_EXIT;
    return NULL;

}

uint32_t * ANativeWindowDisplayAdapter::getOffsets()
{
    const int lnumBufs = mBufferCount;

    LOG_FUNCTION_NAME;

    // TODO(XXX): Need to remove getOffsets from the API. No longer needed

    if ( NULL == mANativeWindow )
    {
        CAMHAL_LOGEA("mANativeWindow reference is missing");
        goto fail;
    }

    if( mBufferHandleMap == NULL)
    {
        CAMHAL_LOGEA("Buffers not allocated yet!!");
        goto fail;
    }

    if(mOffsetsMap == NULL)
    {
        mOffsetsMap = new uint32_t[lnumBufs];
        for(int i = 0; i < mBufferCount; i++)
        {
            mOffsetsMap[i] = 0;
        }
    }

    LOG_FUNCTION_NAME_EXIT;

    return mOffsetsMap;

fail:

    if ( NULL != mOffsetsMap )
    {
        delete [] mOffsetsMap;
        mOffsetsMap = NULL;
    }

    if ( NULL != mErrorNotifier.get() )
    {
        mErrorNotifier->errorNotify(-ENOSYS);
    }

    LOG_FUNCTION_NAME_EXIT;

    return NULL;
}

int ANativeWindowDisplayAdapter::maxQueueableBuffers(unsigned int& queueable)
{
    LOG_FUNCTION_NAME;
    int ret = NO_ERROR;
    int undequeued = 0;

    if(mBufferCount == 0)
    {
        ret = -ENOSYS;
        goto end;
    }

    if(!mANativeWindow)
    {
        ret = -ENOSYS;
        goto end;
    }

    ret = mANativeWindow->get_min_undequeued_buffer_count(mANativeWindow, &undequeued);
    if ( NO_ERROR != ret )
    {
        CAMHAL_LOGEB("get_min_undequeued_buffer_count failed: %s (%d)", strerror(-ret), -ret);

        if ( ENODEV == ret )
        {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mANativeWindow = NULL;
        }

        return -ret;
    }

    queueable = mBufferCount - undequeued;

end:
    return ret;
    LOG_FUNCTION_NAME_EXIT;
}

int ANativeWindowDisplayAdapter::getFd()
{
    LOG_FUNCTION_NAME;

    //remove get handle fd code, fixed to be -1
#if 0
    if(mFD == -1)
    {
        private_handle_t* handle =  (private_handle_t*) *(mBufferHandleMap[0]);
        // TODO: should we dup the fd? not really necessary and another thing for ANativeWindow
        // to manage and close...
        mFD = dup(handle->fd);
    }
#endif
    mFD = -1;


    LOG_FUNCTION_NAME_EXIT;

    return mFD;

}
void* ANativeWindowDisplayAdapter::getVaddrs()
{
    LOG_FUNCTION_NAME;
    return mVaddrMap;
    LOG_FUNCTION_NAME_EXIT;
}

status_t ANativeWindowDisplayAdapter::returnBuffersToWindow()
{
    status_t ret = NO_ERROR;

    //Give the buffers back to display here -  sort of free it
    if (mANativeWindow)
        for(unsigned int i = 0; i < mFramesWithCameraAdapterMap.size(); i++)
        {
            int value = mFramesWithCameraAdapterMap.valueAt(i);

            ret = mANativeWindow->cancel_buffer(mANativeWindow, mBufferHandleMap[value]);
            if ( ENODEV == ret )
            {
                CAMHAL_LOGEA("Preview surface abandoned!");
                mANativeWindow = NULL;
                return -ret;
            }
            else if ( NO_ERROR != ret )
            {
                CAMHAL_LOGEB("cancel_buffer() failed: %s (%d)",
                             strerror(-ret),
                             -ret);
                return -ret;
            }
        }
    else
    {
        CAMHAL_LOGEA("mANativeWindow is NULL");
    }

    ///Clear the frames with camera adapter map
    mFramesWithCameraAdapterMap.clear();

    return ret;

}

int ANativeWindowDisplayAdapter::freeBuffer(void* buf)
{
    LOG_FUNCTION_NAME;

    int *buffers = (int *) buf;
    status_t ret = NO_ERROR;

    Mutex::Autolock lock(mLock);

    if((int *)mGrallocHandleMap != buffers)
    {
        CAMHAL_LOGEA("CameraHal passed wrong set of buffers to free!!!");
        if (mGrallocHandleMap != NULL)
            delete []mGrallocHandleMap;
        mGrallocHandleMap = NULL;
    }


    returnBuffersToWindow();

    if ( NULL != buf )
    {
        delete [] buffers;
        mGrallocHandleMap = NULL;
    }

    if( mBufferHandleMap != NULL)
    {
        delete [] mBufferHandleMap;
        mBufferHandleMap = NULL;
    }

    if ( NULL != mOffsetsMap )
    {
        delete [] mOffsetsMap;
        mOffsetsMap = NULL;
    }

    if( mFD != -1)
    {
        close(mFD);  // close duped handle
        mFD = -1;
    }
    if(NULL != mVaddrMap)
    {
        delete[] mVaddrMap;
        mVaddrMap = NULL;
    }

    return NO_ERROR;
}


bool ANativeWindowDisplayAdapter::supportsExternalBuffering()
{
    return false;
}

int ANativeWindowDisplayAdapter::useBuffers(void *bufArr, int num)
{
    return NO_ERROR;
}

void ANativeWindowDisplayAdapter::displayThread()
{
    bool shouldLive = true;
    int timeout = 0;
    status_t ret;

    LOG_FUNCTION_NAME;

    while(shouldLive)
    {
        ret = ActUtils::MessageQueue::waitForMsg(&mDisplayThread->msgQ()
                ,  &mDisplayQ
                , NULL
                , ANativeWindowDisplayAdapter::DISPLAY_TIMEOUT);

        if ( !mDisplayThread->msgQ().isEmpty() )
        {
            ///Received a message from CameraHal, process it
            shouldLive = processHalMsg();

        }
        else  if( !mDisplayQ.isEmpty())
        {
            if ( mDisplayState== ANativeWindowDisplayAdapter::DISPLAY_INIT )
            {

                ///If display adapter is not started, continue
                continue;

            }
            else
            {
                ActUtils::Message msg;
                ///Get the dummy msg from the displayQ
                if(mDisplayQ.get(&msg)!=NO_ERROR)
                {
                    CAMHAL_LOGEA("Error in getting message from display Q");
                    continue;
                }

                // There is a frame from ANativeWindow for us to dequeue
                // We dequeue and return the frame back to Camera adapter
                if(mDisplayState == ANativeWindowDisplayAdapter::DISPLAY_STARTED)
                {
                    handleFrameReturn();
                }

                if (mDisplayState == ANativeWindowDisplayAdapter::DISPLAY_EXITED)
                {
                    ///we exit the thread even though there are frames still to dequeue. They will be dequeued
                    ///in disableDisplay
                    shouldLive = false;
                }
            }
        }
    }

    LOG_FUNCTION_NAME_EXIT;
}


bool ANativeWindowDisplayAdapter::processHalMsg()
{
    ActUtils::Message msg;

    LOG_FUNCTION_NAME;


    mDisplayThread->msgQ().get(&msg);
    bool ret = true, invalidCommand = false;

    switch ( msg.command )
    {

    case DisplayThread::DISPLAY_START:

        CAMHAL_LOGDA("Display thread received DISPLAY_START command from Camera HAL");
        mDisplayState = ANativeWindowDisplayAdapter::DISPLAY_STARTED;

        break;

    case DisplayThread::DISPLAY_STOP:

        ///@bug There is no API to disable SF without destroying it
        ///@bug Buffers might still be w/ display and will get displayed
        ///@remarks Ideal seqyence should be something like this
        ///mOverlay->setParameter("enabled", false);
        CAMHAL_LOGDA("Display thread received DISPLAY_STOP command from Camera HAL");
        mDisplayState = ANativeWindowDisplayAdapter::DISPLAY_STOPPED;

        break;

    case DisplayThread::DISPLAY_EXIT:

        CAMHAL_LOGDA("Display thread received DISPLAY_EXIT command from Camera HAL.");
        CAMHAL_LOGDA("Stopping display thread...");
        mDisplayState = ANativeWindowDisplayAdapter::DISPLAY_EXITED;
        ///Note that the SF can have pending buffers when we disable the display
        ///This is normal and the expectation is that they may not be displayed.
        ///This is to ensure that the user experience is not impacted
        ret = false;
        break;

    default:

        CAMHAL_LOGEB("Invalid Display Thread Command 0x%x.", msg.command);
        invalidCommand = true;

        break;
    }

    ///Signal the semaphore if it is sent as part of the message
    if ( ( msg.arg1 ) && ( !invalidCommand ) )
    {

        CAMHAL_LOGDA("+Signalling display semaphore");
        Semaphore &sem = *((Semaphore*)msg.arg1);

        sem.Signal();

        CAMHAL_LOGDA("-Signalling display semaphore");
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
status_t ANativeWindowDisplayAdapter::PostFrame(ANativeWindowDisplayAdapter::DisplayFrame &dispFrame)
{
    status_t ret = NO_ERROR;
    uint32_t actualFramesWithDisplay = 0;
    android_native_buffer_t *buffer = NULL;
    int i;

    ///@todo Do cropping based on the stabilized frame coordinates
    ///@todo Insert logic to drop frames here based on refresh rate of
    ///display or rendering rate whichever is lower
    ///Queue the buffer to overlay

    if (!mGrallocHandleMap || !dispFrame.mBuffer)
    {
        CAMHAL_LOGEA("NULL sent to PostFrame");
        return -EINVAL;
    }

    for ( i = 0; i < mBufferCount; i++ )
    {
        //ActionsCode(author:liuyiguang, change_code)
        //if ( ((int) dispFrame.mBuffer ) == (int)mGrallocHandleMap[i] )
        if ( (long)(dispFrame.mBuffer) == (long)mGrallocHandleMap[i] )
        {
            break;
        }
    }
    CAMHAL_LOGVB("ANativeWindowDisplayAdapter::PostFrame index = %d", i);

    if ( mDisplayState == ANativeWindowDisplayAdapter::DISPLAY_STARTED &&
            (!mPaused ) && !mSuspend)
    {
        Mutex::Autolock lock(mLock);
        uint32_t xOff =  dispFrame.mXOff;
        uint32_t yOff = dispFrame.mYOff;


        CAMHAL_LOGDB(" xOff = %d, yOff = %d,w = %d h = %d ",
            xOff, yOff , dispFrame.mWidth,dispFrame.mHeight);
        // Set crop only if current x and y offsets do not match with frame offsets
        if((mXOff!=xOff) || (mYOff!=yOff))
        {
            // We'll ignore any errors here, if the surface is
            // already invalid, we'll know soon enough.
            mANativeWindow->set_crop(mANativeWindow, xOff, yOff,
                                     (xOff)+dispFrame.mWidth, yOff+dispFrame.mHeight);

            ///Update the current x and y offsets
            mXOff = xOff;
            mYOff = yOff;
        }

        ret = mANativeWindow->enqueue_buffer(mANativeWindow, mBufferHandleMap[i]);
        if (ret != 0)
        {
            CAMHAL_LOGEB("Surface::queueBuffer returned error %d", ret);
        }

        //ActionsCode(author:liuyiguang, change_code)
        //mFramesWithCameraAdapterMap.removeItem((int) dispFrame.mBuffer);
        mFramesWithCameraAdapterMap.removeItem((long) dispFrame.mBuffer);


        // HWComposer has not minimum buffer requirement. We should be able to dequeue
        // the buffer immediately
        ActUtils::Message msg;
        mDisplayQ.put(&msg);


#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

        if ( mMeasureStandby )
        {
            CameraHal::PPM("Standby to first shot: Sensor Change completed - ", &mStandbyToShot);
            mMeasureStandby = false;
        }
#endif

    }
    else
    {
        Mutex::Autolock lock(mLock);

        // cancel buffer and dequeue another one
        ret = mANativeWindow->cancel_buffer(mANativeWindow, mBufferHandleMap[i]);
        if (ret != 0)
        {
            CAMHAL_LOGEB("Surface::queueBuffer returned error %d", ret);
        }

        //ActionsCode(author:liuyiguang, change_code)
        //mFramesWithCameraAdapterMap.removeItem((int) dispFrame.mBuffer);
        mFramesWithCameraAdapterMap.removeItem((long) dispFrame.mBuffer);

        ActUtils::Message msg;
        mDisplayQ.put(&msg);
        ret = NO_ERROR;
    }

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
bool ANativeWindowDisplayAdapter::handleFrameReturn()
{
    status_t err;
    buffer_handle_t* buf;
    int i = 0;
    int stride;  // dummy variable to get stride
    Rect bounds;
    void *vaddr;

    // TODO(XXX): Do we need to keep stride information in camera hal?

    if ( NULL == mANativeWindow )
    {
        return false;
    }

    err = mANativeWindow->dequeue_buffer(mANativeWindow, &buf, &stride);
    if (err != 0)
    {
        CAMHAL_LOGEB("dequeueBuffer failed: %s (%d)", strerror(-err), -err);

        if ( ENODEV == err )
        {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mANativeWindow = NULL;
        }

        return false;
    }

    err = mANativeWindow->lock_buffer(mANativeWindow, buf);
    if (err != 0)
    {
        CAMHAL_LOGEB("lockbuffer failed: %s (%d)", strerror(-err), -err);

        if ( ENODEV == err )
        {
            CAMHAL_LOGEA("Preview surface abandoned!");
            mANativeWindow = NULL;
        }

        return false;
    }

    for(i = 0; i < mBufferCount; i++)
    {
        if (mBufferHandleMap[i] == buf)
            break;
    }

    // lock buffer before sending to FrameProvider for filling
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = mFrameWidth;
    bounds.bottom = mFrameHeight;

    //ActionsCode(author:liuyiguang, change_code)
    //mFramesWithCameraAdapterMap.add((int) mGrallocHandleMap[i], i);
    mFramesWithCameraAdapterMap.add((long) mGrallocHandleMap[i], i);

    CAMHAL_LOGVB("handleFrameReturn: found graphic buffer %d of %d", i, mBufferCount-1);
    if(mFrameProvider != NULL)
    {
        mFrameProvider->returnFrame( (void*)mGrallocHandleMap[i], CameraFrame::PREVIEW_FRAME_SYNC, CameraFrame::PREVIEW_STREAM_TYPE);
    }
    usleep(20000);
    return true;
}

void ANativeWindowDisplayAdapter::frameCallbackRelay(CameraFrame* caFrame)
{

    if ( NULL != caFrame )
    {
        if ( NULL != caFrame->mCookie )
        {
            ANativeWindowDisplayAdapter *da = (ANativeWindowDisplayAdapter*) caFrame->mCookie;
            da->frameCallback(caFrame);
        }
        else
        {
            CAMHAL_LOGEB("Invalid Cookie in Camera Frame = %p, Cookie = %p", caFrame, caFrame->mCookie);
        }
    }
    else
    {
        CAMHAL_LOGEB("Invalid Camera Frame = %p", caFrame);
    }

}

void ANativeWindowDisplayAdapter::frameCallback(CameraFrame* caFrame)
{
    ///Call queueBuffer of overlay in the context of the callback thread
    DisplayFrame df;
    df.mBuffer = caFrame->mBuffer;
    df.mType = (CameraFrame::FrameType) caFrame->mFrameType;
    df.mXOff = caFrame->mXOff;
    df.mYOff = caFrame->mYOff;
    df.mLength = caFrame->mLength;
    df.mWidth = caFrame->mWidth;
    df.mHeight = caFrame->mHeight;

    //SAVE_FILE((unsigned char *)caFrame->mVaddr + caFrame->mOffset, caFrame->mOrigWidth, caFrame->mOrigHeight,12);
#ifdef CAMERA_FRAME_STAT
    statFrame(caFrame);
#endif

#ifdef CAMERA_DISPLAY_ENABLE

#ifdef CAMERA_DISPLAY_RGB565
    if(mConvBuf == NULL)
    {
        mConvBuf = (unsigned char *)malloc(1024*1024*2);
    }
    CAMHAL_LOGEB("YV12ToRGB565 mformat = %d,  %s , %d",caFrame->mFormat, __func__, __LINE__);

    if(caFrame->mFormat == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP)
    {
        //CAMHAL_LOGEB("YV12ToRGB565 %s , %d", __func__, __LINE__);
        NV12ToRGB565((unsigned char *)caFrame->mVaddr + caFrame->mOffset, mConvBuf, caFrame->mOrigWidth, caFrame->mOrigHeight);
        //CAMHAL_LOGEB("YV12ToRGB565 %s , %d", __func__, __LINE__);
        memcpy((unsigned char *)caFrame->mVaddr + caFrame->mOffset, mConvBuf, caFrame->mOrigWidth*caFrame->mOrigHeight*2);
        //CAMHAL_LOGEB("YV12ToRGB565 %s , %d", __func__, __LINE__);
    }
    else if(caFrame->mFormat == CameraFrame::CAMERA_FRAME_FORMAT_YUV420P)
    {
        //CAMHAL_LOGEB("YV12ToRGB565 %s , %d", __func__, __LINE__);
        YU12ToRGB565((unsigned char *)caFrame->mVaddr + caFrame->mOffset, mConvBuf, caFrame->mOrigWidth, caFrame->mOrigHeight);
        //CAMHAL_LOGEB("YV12ToRGB565 %s , %d", __func__, __LINE__);
        memcpy((unsigned char *)caFrame->mVaddr + caFrame->mOffset, mConvBuf, caFrame->mOrigWidth*caFrame->mOrigHeight*2);
        //CAMHAL_LOGEB("YV12ToRGB565 %s , %d", __func__, __LINE__);
    }


#endif

    PostFrame(df);
#else

    mFrameProvider->returnFrame( (void*)caFrame->mBuffer, CameraFrame::PREVIEW_FRAME_SYNC, CameraFrame::PREVIEW_STREAM_TYPE);
#endif
}



#ifdef CAMERA_FRAME_STAT

#define MAX_STAT_COUNT (20*60*60)
#define LAG_THRESHOLD  (-1)

void ANativeWindowDisplayAdapter::statInit()
{
    mStatStarted = false;
    mStatLagTime = NULL;
    mStatLagTimeStamp = NULL;
    mStatFlyTime = NULL;
    mStatCaptureIntervalTime  =NULL;
    mStatCaptureScheTime = NULL;
    mStatLagCount = 0;
    mStatFps = 0.0;
    mStatFrameCount = 0;

    mStatStartTime = 0;
    mStatLastTime = 0;

}

void ANativeWindowDisplayAdapter::statSaveInfo()
{
    static int      counter = 0;
    int             fd = -1;
    char            fn[256];
    char            buffer[1024];

    if(!mStatStarted)
    {
        return;
    }

    fn[0] = 0;
    buffer[0] = 0;
    snprintf(fn, sizeof(fn)-1, "/data/camera_data/framestat%d.txt", counter);
    fd = open(fn, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, 0777);
    if(fd < 0)
    {
        CAMHAL_LOGEB("Unable to open file %s: %s", fn, strerror(fd));
        return;
    }

    snprintf(buffer, sizeof(buffer)-1, "StartTime:%llu\nLastTime:%llu\nDuration:%llu\nFps:%f\nFrameCount:%d\n\n\nmStatLagCount:%d\n\n", 
        mStatStartTime,mStatLastTime, mStatLastTime-mStatStartTime, mStatFps, mStatFrameCount,mStatLagCount);

    write(fd, buffer, strlen(buffer));

    for(int i=0; i<mStatLagCount; i++)
    {
        snprintf(buffer, sizeof(buffer)-1, "%6d\t\t%10llu\t%8d\t%8d\t%8d\t%8d\n", 
            i, mStatLagTimeStamp[i], mStatLagTime[i], mStatFlyTime[i], mStatCaptureIntervalTime[i],mStatCaptureScheTime[i]);
        write(fd, buffer, strlen(buffer));
    }

    close(fd);


    counter++;


    statFreeBuffer();
    statInit();

}

void ANativeWindowDisplayAdapter::statFrame(CameraFrame* caFrame)
{
    if(!mStatStarted)
    {
        statFreeBuffer();
        statInit();
        statAllocBuffer();
        mStatStartTime = mStatLastTime = cameraGetMs();

        mStatStarted  = true;
    }
    else
    {
        unsigned long long cur;
        int delta;

        mStatFrameCount++;

        cur = cameraGetMs();
        delta = cur - mStatLastTime;
        mStatFps = 1000*mStatFrameCount/((float)(cur - mStatStartTime));

        mStatLastTime = cur;

        if(delta>LAG_THRESHOLD && mStatLagCount < MAX_STAT_COUNT)
        {
            mStatLagTime[mStatLagCount] = delta;
            mStatLagTimeStamp[mStatLagCount] = cur;
            mStatFlyTime[mStatLagCount] = cur-caFrame->mCaptureTime;
            mStatCaptureIntervalTime[mStatLagCount] = caFrame->mCaptureIntervalTime;
            mStatCaptureScheTime[mStatLagCount] = caFrame->mCaptureScheTime;
            mStatLagCount++;
        }


    }


}

void ANativeWindowDisplayAdapter::statAllocBuffer()
{
    if(!mStatLagTime)
    {
        //log max time is one hour
        mStatLagTime = (int *)malloc(sizeof(int)*MAX_STAT_COUNT);
    }
    if(!mStatLagTimeStamp)
    {
        mStatLagTimeStamp = (unsigned long long *)malloc(sizeof(unsigned long long)*MAX_STAT_COUNT);
    }
    if(!mStatFlyTime)
    {
        mStatFlyTime = (int *)malloc(sizeof(int)*MAX_STAT_COUNT);
    }
    if(!mStatCaptureIntervalTime)
    {
        mStatCaptureIntervalTime = (int *)malloc(sizeof(int)*MAX_STAT_COUNT);
    }
    if(!mStatCaptureScheTime)
    {
        mStatCaptureScheTime = (int *)malloc(sizeof(int)*MAX_STAT_COUNT);
    }

}

void ANativeWindowDisplayAdapter::statFreeBuffer()
{
    if(mStatLagTime)
    {
        free(mStatLagTime);
        mStatLagTime = NULL;
    }
    if(mStatLagTimeStamp)
    {
        free(mStatLagTimeStamp);
        mStatLagTimeStamp = NULL;
    }
    if(mStatFlyTime)
    {
        free(mStatFlyTime);
        mStatFlyTime = NULL;
    }
    if(mStatCaptureIntervalTime)
    {
        free(mStatCaptureIntervalTime);
        mStatCaptureIntervalTime = NULL;
    }
    if(mStatCaptureScheTime)
    {
        free(mStatCaptureScheTime);
        mStatCaptureScheTime = NULL;
    }
}


#endif    

/*--------------------ANativeWindowDisplayAdapter Class ENDS here-----------------------------*/

};

