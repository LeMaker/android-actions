#ifndef CAMERA_OMX_VCE_H
#define CAMERA_OMX_VCE_H

#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/Vector.h>
#include <utils/threads.h>
#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_IVCommon.h"
#include "OMX_Component.h"
#include "OMX_Index.h"
#include "ACT_OMX_Index.h"
#include "ACT_OMX_IVCommon.h"

#include "Semaphore.h"
#include "MessageQueue.h"
#include "CameraCommon.h"


#define MAX_OMX_VCE_SCALE (32)

#define MAX_OMX_VCE_BUFFERS 20


#define VCE_ENCODE_SYNC_TIMEOUT 10000000

//#define OMX_VCE_OUTPUT_USEBUFFER

namespace android
{

class MemoryManager;
class CameraHalExif;

static OMX_ERRORTYPE OnEvent(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_EVENTTYPE eEvent,
    OMX_IN OMX_U32 nData1,
    OMX_IN OMX_U32 nData2,
    OMX_IN OMX_PTR pEventData);

// static
static OMX_ERRORTYPE OnEmptyBufferDone(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer) ;

// static
static OMX_ERRORTYPE OnFillBufferDone(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);


class OMXVceObserver
{
public:
    virtual void sendOutputBuffer(void *outbuf,unsigned int offset, unsigned int size)=0;
    virtual void returnInputBuffer(void * buffer)=0;
    virtual void onOmxVceError(int error)=0;
    virtual  ~OMXVceObserver(){};
};


class OMXEventSyncList
{
public:
    OMXEventSyncList() {};
    ~OMXEventSyncList();
    OMX_ERRORTYPE SignalEvent(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_EVENTTYPE eEvent,
                              OMX_IN OMX_U32 nData1,
                              OMX_IN OMX_U32 nData2,
                              OMX_IN OMX_PTR pEventData);

    OMX_ERRORTYPE RemoveEvent(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_EVENTTYPE eEvent,
                              OMX_IN OMX_U32 nData1,
                              OMX_IN OMX_U32 nData2,
                              OMX_IN OMX_PTR pEventData);


    status_t RegisterForEvent(OMX_IN OMX_HANDLETYPE hComponent,
                              OMX_IN OMX_EVENTTYPE eEvent,
                              OMX_IN OMX_U32 nData1,
                              OMX_IN OMX_U32 nData2,
                              OMX_IN Semaphore &semaphore);

    status_t FlushEvent();
private:
    Vector<struct ActUtils::Message *> mEventSignalQ;
    Mutex mEventLock;
};

enum
{
    OMXVceEncoderType  = 0,
    OMXVceFaceDetectType   = 1,
};

enum
{
    OMXVceImageInputPort = 0,
    OMXVceEncoderOutputPort = 1,
    OMXVceFDOutputPort   = 2,
};

typedef struct VceCropRect_t
{
    int cropx;
    int cropy;
    int cropw;
    int croph;
}VceCropRect;


class OMXVce
{
public:
    static char OMX_VCE_NAME[];// "OMX.st.vce"

    OMXVce();
    virtual ~OMXVce();

    status_t init(int type, bool sync);
    status_t getOmxHandle();
    status_t freeOmxHandle();
    status_t switchToExec();
    status_t execToIdle();
    status_t switchToLoaded();

    status_t getPortDefinition(OMX_U32 portIndex,OMX_PARAM_PORTDEFINITIONTYPE &portDefinition);
    status_t setPortDefinition(OMX_U32 portIndex,OMX_PARAM_PORTDEFINITIONTYPE &portDefinition);

    status_t setImageCropSize(OMX_U32 x, OMX_U32 y, OMX_U32 w, OMX_U32 h);

    status_t loadedToIdle( );

    status_t useBuffers(OMX_U32 portIndex, OMX_U32 bufSize,void* bufs[],OMX_U32 bufCount);

    status_t disableAllPorts();

    status_t disablePort(OMX_U32 portIndex);
    status_t fillAllOutputBuffers();

    status_t processImageAsync(void * buf,VceCropRect *crop);
    status_t processImageSync(void * buf, VceCropRect *crop, void **outbuf,unsigned int *offset, unsigned int *size);
    status_t freeImageData(void * buf);


    status_t setImageSize(OMX_U32 w, OMX_U32 h);
    status_t setOutputImageSize(OMX_U32 w, OMX_U32 h);
    status_t setImageFormat(OMX_U32 f);
    status_t setImageInputCnt(OMX_U32 n);
    

    status_t onEvent(OMX_EVENTTYPE eEvent,
                     OMX_U32 nData1,
                     OMX_U32 nData2,
                     OMX_PTR pEventData);

    status_t onEmptyBufferDone(OMX_BUFFERHEADERTYPE *pBuffer);
    status_t onFillBufferDone(OMX_BUFFERHEADERTYPE *pBuffer);

    status_t setStoreMetaData(OMX_U32 portIndex, OMX_BOOL enable);
    status_t cycleOutputBuffer(void *outbuf);
    virtual void returnInputBuffer(void * buffer);
    virtual void sendOutputBuffer(void *outbuf, unsigned int offset, unsigned int size);
    virtual status_t setVceObserver(OMXVceObserver *observer);

    

public:
    OMX_HANDLETYPE mOmxHandle;
    OMX_STATETYPE mComponentState;

    //key: buffer_handle_t,  value:OMX_BUFFERHEADERTYPE
    KeyedVector<unsigned int, unsigned int> mInputBufferMap;
    //key: void *,  value:OMX_BUFFERHEADERTYPE
    KeyedVector<unsigned int, unsigned int> mOutputBufferMap;



    OMX_U32 mInputPortIndex;
    OMX_U32 mOutputPortIndex;

    OMXVceObserver *mObserver;

    unsigned int mInputBufferSize;
    unsigned int mOutputBufferSize;
    unsigned int mInputBufferCount;
    unsigned int mOutputBufferCount;
    void *mInputBuffers[MAX_OMX_VCE_BUFFERS];
    void *mOutputBuffers[MAX_OMX_VCE_BUFFERS];

    void *mOutputBufferHandles[MAX_OMX_VCE_BUFFERS];
    unsigned int mOutputBufferHandleCount;
    
    //current crop area
    VceCropRect mImageCrop;
    
    //current input image prop
    OMX_U32 mImageWidth;
    OMX_U32 mImageHeight;
    OMX_U32 mImageFormat;
    OMX_U32 mImageInputCnt;

    OMX_U32 mOutputImageWidth;
    OMX_U32 mOutputImageHeight;

    //current output image prop
    OMX_U32 mImageOutFormat;
    OMX_U32 mImageCoding;

    OMX_PARAM_PORTDEFINITIONTYPE mInputPortParam;
    OMX_PARAM_PORTDEFINITIONTYPE mOutputPortParam;

    bool mSync;
    int mType;

private:
    static OMX_CALLBACKTYPE mCallbacks;

    Semaphore mSwitchToExecSem;
    Semaphore mSwitchToIdleSem;
    Semaphore mPortEnableSem;
    Semaphore mPortDisableSem;
    Semaphore mSwitchToLoadedSem;


    Mutex mImageLock;
    Condition mImageCond;

    Mutex mFillBufferSyncLock;
    Condition mFillBufferSyncCond; 
    bool mFillBufferSyncAvilable;

    //output buf to other
    unsigned char *mOutputBuf;
    unsigned int mOutputBufSize;
    unsigned int mOutputBufOffset;



    bool mInputPortEnable;
    bool mOutputPortEnable;

    bool mImageProcessing;

    OMXEventSyncList mEventSyncList;

    sp<MemoryManager> mMemoryManager;


};


enum VceError
{
    VCE_ERROR_FATAL = 0x1, //Fatal errors can only be recovered by restarting media server
    VCE_ERROR_HARD = 0x2,  // Hard errors are hardware hangs that may be recoverable by resetting the hardware internally within the adapter
    VCE_ERROR_SOFT = 0x4, // Soft errors are non fatal errors that can be recovered from without needing to stop use-case
};


typedef struct  ImageJpegEncoderParam
{
    void *   inData;
    void *   outData;
    uint32_t  outDataOffset;
    uint32_t outDataSize;
    int32_t  format;
    int32_t  coding;
    uint32_t bufferSize;
    uint32_t buffersCount;
    void *   buffers[MAX_OMX_VCE_BUFFERS];
    uint32_t xoff;
    uint32_t yoff;
    uint32_t width;
    uint32_t height;
    uint32_t origWidth;
    uint32_t origHeight;
    uint32_t outputWidth;
    uint32_t outputHeight;
    uint32_t quality;
    uint32_t thumbWidth;
    uint32_t thumbHeight;
} ImageJpegEncoderParam;



class OMXVceJpegEncoder:public OMXVce
{
public:
    status_t init();

    virtual status_t encode(ImageJpegEncoderParam *param, CameraHalExif *exifParam);
    virtual status_t encode(ImageJpegEncoderParam *param);
    status_t freeEncodeDataLocked(void *buf);
    status_t freeEncodeData(void *buf);
    status_t  stopEncode();
private:
    status_t setImageQuality(OMX_U32 quality);
    status_t setThumbnailSize(OMX_U32 w, OMX_U32 h);
    status_t setExif(OMX_ACT_PARAM_EXIFPARAM *exif);
    status_t setExif(CameraHalExif *exifParam);

    bool isNeedSetParam(ImageJpegEncoderParam *newp, ImageJpegEncoderParam *oldp);

    
private:
    Mutex mJpegLock;
    ImageJpegEncoderParam mLastParam;
};


typedef struct  ImageResizeParam
{
    void *   inData;
    void *   outData;
    uint32_t  outDataOffset;
    uint32_t  outDataSize;
    uint32_t xoff;
    uint32_t yoff;
    uint32_t width;
    uint32_t height;
} ImageResizeParam;


class OMXVceImageResize:public OMXVce
{
public:
    status_t init();

    status_t setPortParameters();
    status_t  prepareEncode();
    status_t  stopEncode();

    virtual status_t encode(ImageResizeParam *param);
    status_t freeEncodeData(void *buf);

private:
    Mutex mResizeLock;
    
};

class OMXVceFaceDetect: public OMXVce
{
public:
    status_t init();
    status_t setFaceDetect(OMX_BOOL enable);
    status_t setFaceDetectInfo(OMX_U32  sensor,OMX_U32  orientation);
    status_t startFaceDetect();

    status_t stopFaceDetect();

    status_t faceDetect(void *data, VceCropRect *crop,void **outData,unsigned int *offset, unsigned int *outSize);

    status_t freeFaceDetectData(void *outData);

private:
    Mutex mFDLock;
    

};


};




#endif
