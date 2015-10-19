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



#ifndef BASE_CAMERA_ADAPTER_H
#define BASE_CAMERA_ADAPTER_H

#include "CameraHal.h"
#include "CameraHalDebug.h"

namespace android
{

class BaseCameraAdapter : public CameraAdapter
{

public:


    class CameraStream;
    class CameraStreamHub;
    class BufferRefCount;
    class CameraStreamBuffer;

    class BufferRefCount
    {
    public:
        BufferRefCount():mRefCountSum(0){
            for(int i=0; i < CameraFrame::MAX_FRAME_TYPE; i++)
            {
                mRefCount[i] = 0;
            }
        };
        ~BufferRefCount(){};

        uint32_t mRefCountSum;
        uint32_t mRefCount[CameraFrame::MAX_FRAME_TYPE];
    };

    class CameraStreamBuffer:public BufferRefCount, public RefBase
    {
    public:
        CameraStreamBuffer(CameraStream *s,void *buf):mBuffer(buf),mVaddr(NULL),mStream(s),
            mTransaction(false),mFrameTypes(0),mReturnInTransaction(false),mFrameTypesInTransaction(0){};
        ~CameraStreamBuffer(){};

        void setVaddr(void *vaddr);
        void * getVaddr();

        void dumpBuffer();

        void incRefCount(CameraFrame::FrameType frameType);
        void decRefCount(CameraFrame::FrameType frameType);
        uint32_t getRefCountSum() ;
        uint32_t getRefCount(CameraFrame::FrameType frameType);
        void initRefCount(CameraFrame::FrameType frameType, uint32_t refcnt);

        void clearRefCount(CameraFrame::FrameType frameType);

        void returnFrame(unsigned int frameTypes);

        void startTransaction();
        void endTransaction();

    private:
        void *mBuffer;
        void *mVaddr;
        CameraStream *mStream;
        mutable Mutex mLock;
        bool mTransaction;
        unsigned int mFrameTypes;
        bool mReturnInTransaction;
        unsigned int mFrameTypesInTransaction;
    };

    class CameraStream
    {
    public:
        CameraStream():mHub(NULL),mStreamType(CameraFrame::UNKNOWN_STREAM_TYPE){};
        CameraStream(CameraStreamHub *hub):mHub(hub){};
        ~CameraStream(){
            mStreamBuffers.clear();
        };


        void setHub(CameraStreamHub *hub);
        void setStreamType(CameraFrame::StreamType streamType);
        CameraFrame::StreamType getStreamType();
        /**
        * NEW_FEATURE: Add buffer_state_dump function,when watchdog timeout happens.
        *ActionsCode(author:liyuan, change_code)
        */
	int getStreamBuffersSize();

        sp<CameraStreamBuffer> addFrameBuffer(void *frameBuf) ;
        void removeFrameBuffer(void *frameBuf);
        void removeAllFrameBuffer();

        void clearRefCount(CameraFrame::FrameType frameType);
        
        uint32_t getRefCountByFrameTypes(uint32_t types);

        sp<CameraStreamBuffer> getStreamBuffer(void *frameBuf);

        void returnFrameCb(void *frameBuf, unsigned int frameTypes);
        void onReturnFrame(void *frameBuf, unsigned int returnFrameType, unsigned int frameTypes);

        bool validBuffer(void* frameBuf) ;

        status_t sendFrame(CameraFrame* frame,
            KeyedVector<void *, frame_callback> *subscribers,
            CameraFrame::FrameType frameType) ;
        
    private:
        KeyedVector<void *, sp<CameraStreamBuffer> > mStreamBuffers;
        CameraStreamHub *mHub;
        CameraFrame::StreamType mStreamType;
        mutable Mutex mLock;
    };

    class CameraStreamHub
    {
    public:
        CameraStreamHub(BaseCameraAdapter *bca);
        
        ~CameraStreamHub();

        status_t subscribe(CameraFrame::FrameType frameType, frame_callback callback, void* cookie) ;
        status_t unsubscribe(CameraFrame::FrameType frameType,  void* cookie);


        uint32_t getRefCountByFrameTypes(uint32_t types);

        CameraStream *getStream(CameraFrame::StreamType streamType);

        CameraStream *getStream(void * frameBuf);
        sp<CameraStreamBuffer> getStreamBuffer(void *frameBuf);

        status_t sendFrame(CameraFrame *frame, uint32_t types) ;
        status_t sendFrameToSubscribers(CameraFrame* frame,
            KeyedVector<void *, frame_callback> *subscribers,
            CameraFrame::FrameType frameType);
        
       
        
        void returnFrame(void* frameBuf, CameraFrame::FrameType frameType, CameraFrame::StreamType streamType);

        void returnFrameCb(void *frameBuf, unsigned int frameTypes, CameraFrame::StreamType streamType);
        void onReturnFrame(void *frameBuf, unsigned int returnFrameType, unsigned int frameTypes, CameraFrame::StreamType streamType);


    public:
        KeyedVector<void*, frame_callback> mSubscribers[CameraFrame::MAX_FRAME_TYPE];
        CameraStream mStreams[CameraFrame::MAX_STREAM_TYPE];
        mutable Mutex mLocks[CameraFrame::MAX_FRAME_TYPE];
        BaseCameraAdapter *mBca;
    };


    BaseCameraAdapter();
    virtual ~BaseCameraAdapter();

    ///Initialzes the camera adapter creates any resources required
    virtual status_t initialize(CameraProperties::Properties*) = 0;

    virtual int setErrorHandler(ErrorNotifier *errorNotifier);

    //Message/Frame notification APIs
    virtual void enableMsgType(int32_t msgs, event_callback eventCb=NULL, void* cookie=NULL);
    virtual void disableMsgType(int32_t msgs, void* cookie);
    virtual void enableFrameMsgType(int32_t msgs, frame_callback callback=NULL, void* cookie=NULL);
    virtual void disableFrameMsgType(int32_t msgs, void* cookie);
    virtual void returnFrame(void * frameBuf, CameraFrame::FrameType frameType, CameraFrame::StreamType streamType);
    virtual void onFillFrame(void * frameBuf, unsigned int frameTypes, CameraFrame::StreamType streamType);
    virtual void onReturnFrame(void * frameBuf, unsigned int returnFrameType, unsigned int frameTypes, CameraFrame::StreamType streamType);

    //APIs to configure Camera adapter and get the current parameter set
    virtual status_t setParameters(const CameraParameters& params) = 0;
    virtual void getParameters(CameraParameters& params)  = 0;

    //API to send a command to the camera
    //ActionsCode(author:liuyiguang, change_code)
    //virtual status_t sendCommand(CameraCommands operation, int value1 = 0, int value2 = 0, int value3 = 0 );
    virtual status_t sendCommand(CameraCommands operation, long value1 = 0, int value2 = 0, int value3 = 0 );

    virtual status_t registerImageReleaseCallback(release_image_buffers_callback callback, void *user_data);

    virtual status_t registerEndCaptureCallback(end_image_capture_callback callback, void *user_data);

    //Retrieves the current Adapter state
    virtual AdapterState getState();
    //Retrieves the next Adapter state
    virtual AdapterState getNextState();

    // Rolls the state machine back to INTIALIZED_STATE from the current state
    virtual status_t rollbackToInitializedState();
    virtual status_t rollbackToPreviewState();

protected:
    //The first two methods will try to switch the adapter state.
    //Every call to setState() should be followed by a corresponding
    //call to commitState(). If the state switch fails, then it will
    //get reset to the previous state via rollbackState().
    virtual status_t setState(CameraCommands operation);
    virtual status_t commitState();
    virtual status_t rollbackState();

    // Retrieves the current Adapter state - for internal use (not locked)
    virtual status_t getState(AdapterState &state);
    // Retrieves the next Adapter state - for internal use (not locked)
    virtual status_t getNextState(AdapterState &state);

    //-----------Interface that needs to be implemented by deriving classes --------------------

    //Should be implmented by deriving classes in order to start image capture
    virtual status_t takePicture();

    //Should be implmented by deriving classes in order to start image capture
    virtual status_t stopImageCapture();

    //Should be implemented by deriving classes in oder to initiate autoFocus
    virtual status_t autoFocus();

    //Should be implemented by deriving classes in oder to initiate autoFocus
    virtual status_t cancelAutoFocus();

    //Should be called by deriving classes in order to do some bookkeeping
    virtual status_t startVideoCapture();

    //Should be called by deriving classes in order to do some bookkeeping
    virtual status_t stopVideoCapture();

    //Should be implemented by deriving classes in order to start camera preview
    virtual status_t startPreview();

    //Should be implemented by deriving classes in order to stop camera preview
    virtual status_t stopPreview();

    //Should be implemented by deriving classes in order to start smooth zoom
    virtual status_t startSmoothZoom(int targetIdx);

    //Should be implemented by deriving classes in order to stop smooth zoom
    virtual status_t stopSmoothZoom();

    //Should be implemented by deriving classes in order to stop smooth zoom
    virtual status_t useBuffers(CameraMode mode, void* bufArr, int num, size_t length, unsigned int queueable);

    //Should be implemented by deriving classes in order queue a released buffer in CameraAdapter
    virtual status_t fillThisBuffer(void* frameBuf, unsigned int frameTypes,  CameraFrame::StreamType streamType);

    //API to get the frame size required to be allocated. This size is used to override the size passed
    //by camera service when VSTAB/VNF is turned ON for example
    virtual status_t getFrameSize(size_t &width, size_t &height, int num);

    //API to get required data frame size
    virtual status_t getFrameDataSize(size_t &dataFrameSize, size_t bufferCount);

    //API to get required picture buffers size with the current configuration in CameraParameters
    virtual status_t getPictureBufferSize(size_t &length, size_t bufferCount);

    // Should be implemented by deriving classes in order to start face detection
    // ( if supported )
    virtual status_t startFaceDetection();

    // Should be implemented by deriving classes in order to stop face detection
    // ( if supported )
    virtual status_t stopFaceDetection();

    virtual status_t switchToExecuting();

    // Receive orientation events from CameraHal
    virtual void onOrientationEvent(uint32_t orientation, uint32_t tilt);

    virtual status_t sendFrameToSubscribers(CameraFrame *frame, uint32_t types);
    virtual status_t startTransaction(void *frame);
    virtual status_t endTransaction(void *frame);
    // ---------------------Interface ends-----------------------------------

    status_t notifyFocusSubscribers(CameraHalEvent::FocusStatus status);
    status_t notifyShutterSubscribers();
    status_t notifyZoomSubscribers(int zoomIdx, bool targetReached);
    status_t notifyFaceSubscribers(sp<CameraFDResult> &faces);

// private member functions
private:
    status_t rollbackToPreviousState();

// protected data types and variables
protected:

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    struct timeval mStartFocus;
    struct timeval mStartCapture;

#endif

    mutable Mutex mReturnFrameLock;
    mutable Mutex mSendCommandLock; 
    mutable Mutex mSubscriberLock;

    //Lock protecting the Adapter state
    mutable Mutex mLock;
    AdapterState mAdapterState;
    AdapterState mNextState;

    //Different frame subscribers get stored using these
    KeyedVector<int, event_callback> mFocusSubscribers;
    KeyedVector<int, event_callback> mZoomSubscribers;
    KeyedVector<int, event_callback> mShutterSubscribers;
    KeyedVector<int, event_callback> mFaceSubscribers;

    ErrorNotifier *mErrorNotifier;
    release_image_buffers_callback mReleaseImageBuffersCallback;
    end_image_capture_callback mEndImageCaptureCallback;
    void *mReleaseData;
    void *mEndCaptureData;
    bool mRecording;

    uint32_t mFramesWithOMX;
    uint32_t mFramesWithDisplay;
    uint32_t mFramesWithEncoder;

#ifdef DEBUG_LOG
    KeyedVector<int, bool> mBuffersWithOMX;
#endif
    KeyedVector<void *, CameraFrame *> mFrameQueue;
    CameraStreamHub *mStreamHub;
};

};

#endif //BASE_CAMERA_ADAPTER_H


