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



#ifndef OMX_CAMERA_ADAPTER_H
#define OMX_CAMERA_ADAPTER_H

#include "CameraHal.h"
#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_IVCommon.h"
#include "OMX_Component.h"
#include "OMX_Index.h"
#include "ACT_OMX_Index.h"
#include "ACT_OMX_IVCommon.h"
#include "General3A_Settings.h"
#include "Encoder_libjpeg.h"

#include "BaseCameraAdapter.h"

#include "CameraCommon.h"

#include "video_mediadata.h"

#include "OMXVce.h"

#include "CameraWatchDog.h"

//ActionsCode(author:liuyiguang, add_code)
#include "ACT_OMX_Common_V1_2__V1_1.h"

namespace android
{


/* Default portstartnumber of Camera component */
#define OMX_CAMERA_DEFAULT_START_PORT_NUM 0

/* Define number of ports for differt domains */
#define OMX_CAMERA_PORT_OTHER_NUM 0
#define OMX_CAMERA_PORT_VIDEO_NUM 1
#define OMX_CAMERA_PORT_IMAGE_NUM 1
#define OMX_CAMERA_PORT_AUDIO_NUM 0
#define OMX_CAMERA_NUM_PORTS (OMX_CAMERA_PORT_OTHER_NUM + OMX_CAMERA_PORT_VIDEO_NUM + OMX_CAMERA_PORT_IMAGE_NUM + OMX_CAMERA_PORT_AUDIO_NUM)

/* Define start port number for differt domains */
#define OMX_CAMERA_PORT_OTHER_START OMX_CAMERA_DEFAULT_START_PORT_NUM
#define OMX_CAMERA_PORT_VIDEO_START (OMX_CAMERA_PORT_OTHER_START + OMX_CAMERA_PORT_OTHER_NUM)
#define OMX_CAMERA_PORT_IMAGE_START (OMX_CAMERA_PORT_VIDEO_START + OMX_CAMERA_PORT_VIDEO_NUM)
#define OMX_CAMERA_PORT_AUDIO_START (OMX_CAMERA_PORT_IMAGE_START + OMX_CAMERA_PORT_IMAGE_NUM)

/* Port index for camera component */
#define OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW (OMX_CAMERA_PORT_VIDEO_START + 0)
#define OMX_CAMERA_PORT_IMAGE_OUT_IMAGE (OMX_CAMERA_PORT_IMAGE_START + 0)

#define MAX_CAMERA_ZOOM (4)

#define MAX_CAMERA_SCALE (MAX_OMX_VCE_SCALE/MAX_CAMERA_ZOOM)


const int64_t kCameraBufferLatencyNs = 250000000LL; // 250 ms

///OMX Specific Functions
static OMX_ERRORTYPE OMXCameraAdapterEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData);

static OMX_ERRORTYPE OMXCameraAdapterEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE OMXCameraAdapterFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader);

//for capability
static OMX_ERRORTYPE OMXCameraAdapterCapEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_EVENTTYPE eEvent,
        OMX_IN OMX_U32 nData1,
        OMX_IN OMX_U32 nData2,
        OMX_IN OMX_PTR pEventData);

static OMX_ERRORTYPE OMXCameraAdapterCapEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE OMXCameraAdapterCapFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
        OMX_IN OMX_PTR pAppData,
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader);

struct CapResolution
{
    size_t width, height;
    const char *param;
};

struct CapPixelformat
{
    OMX_COLOR_FORMATTYPE pixelformat;
    const char *param;
};

struct CapU32
{
    OMX_U32 num;
    const char *param;
};

struct CapU32Pair
{
    OMX_U32 num1, num2;
    const char *param;
};
struct CapS32
{
    OMX_S32 num;
    const char *param;
};

typedef CapU32 CapFramerate;
typedef CapU32 CapISO;
typedef CapU32 CapSensorName;
typedef CapS32 CapZoom;
typedef CapS32 CapEVComp;


/**
  * Class which completely abstracts the camera hardware interaction from camera hal
  * TODO: Need to list down here, all the message types that will be supported by this class
                Need to implement BufferProvider interface to use AllocateBuffer of OMX if needed
  */

#ifdef CAMERA_VCE_OMX_FD
class OMXCameraAdapter : public BaseCameraAdapter,public OMXVceObserver
#else
class OMXCameraAdapter : public BaseCameraAdapter
#endif
{
public:

    /*--------------------Constant declarations----------------------------------------*/
    static const int32_t MAX_NO_BUFFERS = 20;

    ///@remarks OMX Camera has six ports - buffer input, time input, preview, image, video, and meta data
    static const int MAX_NO_PORTS = 2;

    ///Five second timeout
    static const int CAMERA_ADAPTER_TIMEOUT = 5000*1000;

    static const char CAMERA_OMX_NAME[];// ="OMX.st.v4l.camera_source";


    enum OMXCameraEvents
    {
        CAMERA_PORT_ENABLE  = 0x1,
        CAMERA_PORT_FLUSH   = 0x2,
        CAMERA_PORT_DISABLE = 0x4,
    };

    enum CaptureMode
    {
        HIGH_SPEED = 1,
        HIGH_QUALITY = 2,
        VIDEO_MODE = 3,
        HIGH_QUALITY_ZSL = 4,
    };


    enum Algorithm3A
    {
        WHITE_BALANCE_ALGO = 0x1,
        EXPOSURE_ALGO = 0x2,
        FOCUS_ALGO = 0x4,
    };

    enum AlgoPriority
    {
        FACE_PRIORITY = 0,
        REGION_PRIORITY,
    };

    enum BrightnessMode
    {
        BRIGHTNESS_OFF = 0,
        BRIGHTNESS_ON,
        BRIGHTNESS_AUTO,
    };

    enum CaptureSettingsFlags
    {
        SetFormat               = 1 << 0,
        SetThumb                = 1 << 1,
        SetExpBracket           = 1 << 2,
        SetQuality              = 1 << 3,
        SetRotation             = 1 << 4,
        ECaptureSettingMax,
        ECapturesettingsAll = ( ((ECaptureSettingMax -1 ) << 1) -1 ) /// all possible flags raised
    };

    class GPSData
    {
    public:
        int mLongDeg, mLongMin, mLongSec, mLongSecDiv;
        char mLongRef[GPS_REF_SIZE];
        bool mLongValid;
        int mLatDeg, mLatMin, mLatSec, mLatSecDiv;
        char mLatRef[GPS_REF_SIZE];
        bool mLatValid;
        int mAltitude;
        unsigned char mAltitudeRef;
        bool mAltitudeValid;
        char mMapDatum[GPS_MAPDATUM_SIZE];
        bool mMapDatumValid;
        char mVersionId[GPS_VERSION_SIZE];
        bool mVersionIdValid;
        char mProcMethod[GPS_PROCESSING_SIZE];
        bool mProcMethodValid;
        char mDatestamp[GPS_DATESTAMP_SIZE];
        bool mDatestampValid;
        uint32_t mTimeStampHour;
        uint32_t mTimeStampMin;
        uint32_t mTimeStampSec;
        bool mTimeStampValid;
    };

    class EXIFData
    {
    public:
        GPSData mGPSData;
        bool mMakeValid;
        bool mModelValid;
    };

    ///Parameters specific to any port of the OMX Camera component
    //ActionsCode(author:liuyiguang, change_code)
    class OMXCameraPortParameters
    {
    public:
        //OMX_U32                         mHostBufaddr[MAX_NO_BUFFERS];
        long                            mHostBufaddr[MAX_NO_BUFFERS];
        OMX_BUFFERHEADERTYPE           *mBufferHeader[MAX_NO_BUFFERS];
        //OMX_U32                         mMetadataBufs[MAX_NO_BUFFERS];
        long                            mMetadataBufs[MAX_NO_BUFFERS];
        OMX_U32                         mMetadataNum;
        OMX_U32                         mWidth;
        OMX_U32                         mHeight;
        OMX_U32                         mStride;
        OMX_U8                          mNumBufs;

        // defines maximum number of buffers our of mNumBufs
        // queueable at given moment
        OMX_U8                          mMaxQueueable;

        OMX_U32                         mBufSize;
        OMX_COLOR_FORMATTYPE            mColorFormat;
        OMX_IMAGE_CODINGTYPE            mCompressionFormat;
        //OMX_PARAM_VIDEONOISEFILTERTYPE  mVNFMode;
        //OMX_PARAM_VIDEOYUVRANGETYPE     mYUVRange;
        OMX_CONFIG_BOOLEANTYPE          mVidStabParam;
        OMX_CONFIG_FRAMESTABTYPE        mVidStabConfig;
        OMX_U32                         mCapFrame;
        OMX_U32                         mFrameRate;
        OMX_S32                         mMinFrameRate;
        OMX_S32                         mMaxFrameRate;
        OMX_U32                         mZoomXOff;
        OMX_U32                         mZoomYOff;
        OMX_U32                         mZoomWidth;
        OMX_U32                         mZoomHeight;
        OMX_U32                         mEncodeWidth;
        OMX_U32                         mEncodeHeight;
        OMX_U32                         mCropWidth;
        OMX_U32                         mCropHeight;

        OMX_U32                         mPendingWidth;
        OMX_U32                         mPendingHeight;
        OMX_U32                         mPendingEncodeWidth;
        OMX_U32                         mPendingEncodeHeight;
        OMX_U32                         mPendingCropWidth;
        OMX_U32                         mPendingCropHeight;
        OMX_U32                         mPendingStride;
        OMX_U32                         mPendingBufSize;
        bool                            mPendingSize;

        int                             mCurIndex;
        unsigned int                    mBufferBitmap;

        bool                            mPortEnable;
        Mutex                           mLock;
    };

    ///Context of the OMX Camera component
    class OMXCameraAdapterComponentContext
    {
    public:
        OMX_HANDLETYPE              mHandleComp;
        OMX_U32                     mNumPorts;
        OMX_STATETYPE               mState ;
        OMX_U32                     mPrevPortIndex;
        OMX_U32                     mImagePortIndex;
        OMXCameraPortParameters     mCameraPortParams[MAX_NO_PORTS];
    };

public:

    OMXCameraAdapter(size_t sensor_index);
    ~OMXCameraAdapter();

    ///Initialzes the camera adapter creates any resources required
    virtual status_t initialize(CameraProperties::Properties*);

    //APIs to configure Camera adapter and get the current parameter set
    virtual status_t setParameters(const CameraParameters& params);
    virtual void getParameters(CameraParameters& params);

    // API
    virtual status_t UseBuffersPreview(void* bufArr, int num);

    //API to flush the buffers for preview
    status_t flushBuffers();

    // API
    virtual status_t setFormat(OMX_U32 port, OMXCameraPortParameters &cap);

    virtual status_t setFramerate(OMX_U32 framerate);
        
    // Function to get and populate caps from handle
    static status_t getCaps(CameraProperties::Properties* props, OMX_HANDLETYPE handle, int id);
    static void dumpCaps(OMX_ACT_CAPTYPE &caps);
    static const char* getLUTvalue_OMXtoHAL(int OMXValue, LUTtype LUT);
    static int getLUTvalue_HALtoOMX(const char * HalValue, LUTtype LUT);
    
    /**
    * NEW_FEATURE: Add UVC module support .
    *ActionsCode(author:liyuan, change_code)
    */
    static bool query_UVC_ReplaceMode();	
    static bool get_UVC_ReplaceMode(OMX_UVCMODE *mode, int SensorIndex);

    OMX_ERRORTYPE OMXCameraAdapterEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
            OMX_IN OMX_EVENTTYPE eEvent,
            OMX_IN OMX_U32 nData1,
            OMX_IN OMX_U32 nData2,
            OMX_IN OMX_PTR pEventData);

    OMX_ERRORTYPE OMXCameraAdapterEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
            OMX_IN OMX_BUFFERHEADERTYPE* pBuffer, void *handle);

    OMX_ERRORTYPE OMXCameraAdapterFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
            OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader, void *extra);

    static OMX_ERRORTYPE OMXCameraGetHandle(OMX_HANDLETYPE *handle, OMX_PTR pAppData=NULL, OMX_CALLBACKTYPE *pCallbacks=NULL);
    
    void onWatchDogMsg(int msg);
    /**
    * NEW_FEATURE: Add buffer_state_dump function,when watchdog timeout happens.
    *ActionsCode(author:liyuan, change_code)
    */
    void Hal_Dump_Bufs_Occupied();

protected:

    //Parent class method implementation
    virtual status_t takePicture();
    virtual status_t endImageCallback();
    
    virtual status_t stopImageCapture();
    virtual status_t autoFocus();
    virtual status_t cancelAutoFocus();
    virtual status_t startSmoothZoom(int targetIdx);
    virtual status_t stopSmoothZoom();
    virtual status_t startVideoCapture();
    virtual status_t stopVideoCapture();
    virtual status_t startPreview();
    virtual status_t stopPreview();
    virtual status_t useBuffers(CameraMode mode, void* bufArr, int num, size_t length, unsigned int queueable);
    virtual status_t fillThisBuffer(void* frameBuf, unsigned int frameTypes, CameraFrame::StreamType streamType);
    virtual status_t getFrameSize(size_t &width, size_t &height, int num);
    virtual status_t getPictureBufferSize(size_t &length, size_t bufferCount);
    //virtual status_t startFaceDetection();
    //virtual status_t stopFaceDetection();
    virtual status_t switchToExecuting();
    virtual void onOrientationEvent(uint32_t orientation, uint32_t tilt);

    virtual void onFillFrame(void * frameBuf, unsigned int frameTypes, CameraFrame::StreamType streamType);
    virtual void onReturnFrame(void * frameBuf, unsigned int returnFrameType, unsigned int frameTypes, CameraFrame::StreamType streamType);


#ifdef CAMERA_VCE_OMX_FD
    static void FDFrameCallbackRelay(CameraFrame *cameraFrame);
    virtual void FDFrameCallback(CameraFrame *cameraFrame);
#endif

private:


    status_t disablePreviewPort();
    status_t doSwitchToExecuting();

    void performCleanupAfterError();

    status_t switchToLoaded();

    OMXCameraPortParameters *getPortParams(CameraFrame::StreamType frameType, OMX_U32 &portIndex);

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

    //status_t setPictureRotation(unsigned int degree);
    //status_t setSensorOrientation(unsigned int degree);
    //status_t setImageQuality(unsigned int quality);
    //status_t setThumbnailParams(unsigned int width, unsigned int height, unsigned int quality);

    //EXIF
    status_t setParametersEXIF(const CameraParameters &params,
                               BaseCameraAdapter::AdapterState state);
    status_t convertGPSCoord(double coord, int &deg, int &min, int &sec, int &secDivisor);
    status_t setupEXIF_libjpeg(ExifElementsTable*);
    status_t setupEXIF_vce(CameraHalExif *);

    status_t freeExifObjects();
    status_t allocExifObject(void *key, void **exif);
    status_t getExif(void *key,void **exif, int *type);
    status_t freeExifObject(void *key);
    

    //Focus functionality
    status_t doAutoFocus();
    status_t stopAutoFocus();
    status_t getFocusStatus(OMX_PARAM_FOCUSSTATUSTYPE &eFocusStatus);
    status_t getFocusStatus(CameraHalEvent::FocusStatus &status);
    status_t getFocusRatio(OMX_U32 &focus_ratio);
    status_t checkFocusStatus();
    status_t getMaxFAreas(OMX_U32 *maxfareas);
    status_t returnFocusStatus(CameraHalEvent::FocusStatus focusStatus);
    status_t getFocusMode(OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE &focusMode);
    status_t setFocusMode(OMX_IMAGE_FOCUSCONTROLTYPE focusMode);


    //Focus distances
    status_t setParametersFocus(const CameraParameters &params,
                                BaseCameraAdapter::AdapterState state);
    status_t addFocusDistances(OMX_U32 &near,
                               OMX_U32 &optimal,
                               OMX_U32 &far,
                               CameraParameters& params);
    status_t encodeFocusDistance(OMX_U32 dist, char *buffer, size_t length);
    status_t getFocusDistances(OMX_U32 &near,OMX_U32 &optimal, OMX_U32 &far, OMX_U32 &len);

    //VSTAB and VNF Functionality
    status_t enableVideoNoiseFilter(bool enable);
    status_t enableVideoStabilization(bool enable);

    //Digital zoom
    status_t setParametersZoom(const CameraParameters &params,
                               BaseCameraAdapter::AdapterState state);
    status_t initZoom(unsigned int portIndex);
    status_t setPortZoom(unsigned int portIndex, int index);
    status_t doZoom(int index);
    status_t advanceZoom();

    //3A related parameters
    status_t setParameters3A(const CameraParameters &params,
                             BaseCameraAdapter::AdapterState state);

    // scene modes
    status_t setScene(Gen3A_settings& Gen3A);
    // returns pointer to SceneModesEntry from the LUT for camera given 'name' and 'scene'
    //static const SceneModesEntry* getSceneModeEntry(const char* name, OMX_SCENEMODETYPE scene);


    //Flash modes
    status_t setFlashMode(Gen3A_settings& Gen3A);
    status_t getFlashMode(Gen3A_settings& Gen3A);

    bool needFlashStrobe(Gen3A_settings& Gen3A);
    status_t startFlashStrobe();
    status_t stopFlashStrobe();

    // Focus modes
    status_t setFocusMode(Gen3A_settings& Gen3A);
    status_t getFocusMode(Gen3A_settings& Gen3A);

    //Exposure Modes
    status_t setExposureMode(Gen3A_settings& Gen3A);
    status_t setEVCompensation(Gen3A_settings& Gen3A);
    status_t setWBMode(Gen3A_settings& Gen3A);
    status_t setFlicker(Gen3A_settings& Gen3A);
    status_t setBrightness(Gen3A_settings& Gen3A);
    status_t setContrast(Gen3A_settings& Gen3A);
    status_t setSharpness(Gen3A_settings& Gen3A);
    status_t setSaturation(Gen3A_settings& Gen3A);
    status_t setISO(Gen3A_settings& Gen3A);
    status_t setDenoise(Gen3A_settings& Gen3A);
    status_t setEffect(Gen3A_settings& Gen3A);
    status_t setMeteringAreas(Gen3A_settings& Gen3A);

    status_t getEVCompensation(Gen3A_settings& Gen3A);
    status_t getWBMode(Gen3A_settings& Gen3A);
    status_t getSharpness(Gen3A_settings& Gen3A);
    status_t getSaturation(Gen3A_settings& Gen3A);
    status_t getISO(Gen3A_settings& Gen3A);
    status_t getDenoise(Gen3A_settings& Gen3A);

    // 3A locks
    status_t setExposureLock(Gen3A_settings& Gen3A);
    status_t setFocusLock(Gen3A_settings& Gen3A);
    status_t setWhiteBalanceLock(Gen3A_settings& Gen3A);
    status_t set3ALock(OMX_BOOL toggleExp, OMX_BOOL toggleWb, OMX_BOOL toggleFocus);

    //API to set FrameRate using VFR interface
    status_t setVFramerate(OMX_U32 minFrameRate,OMX_U32 maxFrameRate);

    status_t setParametersAlgo(const CameraParameters &params,
                               BaseCameraAdapter::AdapterState state);
    status_t printComponentVersion(OMX_HANDLETYPE handle);

    //Touch AF
    status_t setTouchFocus();

    status_t updateFocusDistances(CameraParameters &params);
    status_t updateFocusLength(CameraParameters &params);
    //Face detection
    status_t setParametersFD(const CameraParameters &params,
                             BaseCameraAdapter::AdapterState state);
    status_t initFaceDetection();
    status_t destroyFaceDetection();
    status_t startFaceDetection();
    status_t stopFaceDetection();
    status_t setFaceDetectionOrientation(int orientation);
#ifdef CAMERA_VCE_OMX_FD
    status_t detectFaces(void *data, VceCropRect *crop);
    void sendFaces(void *data,unsigned int offset, unsigned int size);
#endif
    void pauseFaceDetection(bool pause);

    //matadata maps
    void clearMetadataBufs(OMXCameraPortParameters &portparam);
    // Utility methods for OMX Capabilities
    static status_t insertCapabilities(CameraProperties::Properties*, OMX_ACT_CAPTYPE&,int);
    static status_t encodeSizeCap(OMX_ACT_VARRESTYPE&, char *, size_t);
    static size_t encodeZoomCap(OMX_S32, const CapZoom*, size_t, char*, size_t);
    static status_t encodeISOCap(OMX_U32 maxISO,
                      const CapISO *cap,
                      size_t capCount,
                      char * buffer,
                      size_t bufferSize);
    static status_t encodePrvVFramerateCap(OMX_ACT_CAPTYPE&,  char*, char *,size_t);
    static status_t encodeCapVFramerateCap(OMX_ACT_CAPTYPE&,  char*, char *,size_t);
    static status_t encodePixelformatCap(OMX_COLOR_FORMATTYPE,
                                         const CapPixelformat*,
                                         size_t,
                                         char*,
                                         size_t);
    static status_t encodeFramerateCap(OMX_U32 framerateMax,
                                       OMX_U32 framerateMin,
                                       const CapFramerate *cap,
                                       size_t capCount,
                                       char * buffer,
                                       size_t bufferSize);
    static status_t insertImageSizes(CameraProperties::Properties*, OMX_ACT_CAPTYPE&, int);
    static status_t insertPreviewSizes(CameraProperties::Properties*, OMX_ACT_CAPTYPE&,int);
    static status_t insertThumbSizes(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t insertZoom(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t insertImageFormats(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t insertPreviewFormats(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t insertFramerates(CameraProperties::Properties* params, OMX_ACT_CAPTYPE &caps);
    static status_t insertPrvVFramerates(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t insertCapVFramerates(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t insertEVs(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t insertIPPModes(CameraProperties::Properties*, OMX_ACT_CAPTYPE &);
    static status_t insertWBModes(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t insertEffects(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t insertSceneModes(CameraProperties::Properties*, OMX_ACT_CAPTYPE &, int);
    static status_t insertFocusModes(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t insertFlickerModes(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t insertISOModes(CameraProperties::Properties* params, OMX_ACT_CAPTYPE &caps) ;
    static status_t insertFlashModes(CameraProperties::Properties*, OMX_ACT_CAPTYPE&, int);
    static status_t insertSenMount(CameraProperties::Properties*, OMX_ACT_CAPTYPE&,int);
    static status_t insertDefaults(CameraProperties::Properties*, OMX_ACT_CAPTYPE&, int);
    static status_t insertLocks(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t insertAreas(CameraProperties::Properties*, OMX_ACT_CAPTYPE&);
    static status_t setDefaultValue(CameraProperties::Properties* params, const char *supportedKey, const char *key, const char *preferedValue);
    static const char * getDefaultValue(CameraProperties::Properties* params, const char *key);

    static status_t buildPreviewFormatMaps(CameraProperties::Properties* params, OMX_ACT_CAPTYPE &caps);

    static bool getCropArea(int sw, int sh, int dw, int dh, int *cw, int *ch, bool isImage);
    static int sizeNearWeight(int cw,int ch, int dw, int dh);
    static status_t findBestSize(OMX_ACT_VARRESTYPE hwSizes[], OMX_U16 hwSizeNum, int width, int height, int *bestWidth, int *bestHeight, bool isImage);
    static status_t findBestExSize(OMX_ACT_VARRESTYPE hwSizes[], OMX_U16 hwSizeNum, int width, int height, ExtendedResolution *exres, bool isImage);
    static status_t findBestFps(OMX_ACT_VARFPSTYPE hwFps[], OMX_U16 hwFpsNum, int fps,  int *bestmin, int *bestmax, int *bestfps);
    

    static bool isResolutionValid(unsigned int width, unsigned int height, const char *supportedResolutions);
    static bool isParameterValid(const char *param, const char *supportedParams);
    static bool isParameterValid(int param, const char *supportedParams);

    
    status_t setParametersCapture(const CameraParameters &params,
                                  BaseCameraAdapter::AdapterState state);
    // Image Capture Service
    status_t startImageCapture();
    status_t disableImagePort();

    //Sets eithter HQ or HS mode and the frame count
    status_t setCaptureMode(OMXCameraAdapter::CaptureMode mode);
    status_t UseBuffersCapture(void* bufArr, int num);
    status_t canSetCapture3A(bool &state);



    //Used for calculation of the average frame rate during preview
    status_t recalculateFPS();

    //Helper method for initializing a CameFrame object
    status_t initCameraFrame(CameraFrame &frame, OMX_IN OMX_BUFFERHEADERTYPE *pBuffHeader, void * handle, OMXCameraPortParameters *port);

    status_t sendCallBacks(CameraFrame frame,unsigned int mask);

    status_t apply3Asettings( Gen3A_settings& Gen3A );
    status_t apply3ADefaults(Gen3A_settings &Gen3A);

    // AutoConvergence
    //status_t setAutoConvergence(OMX_TI_AUTOCONVERGENCEMODETYPE pACMode, OMX_S32 pManualConverence);
    //status_t getAutoConvergence(OMX_TI_AUTOCONVERGENCEMODETYPE *pACMode, OMX_S32 *pManualConverence);

    OMX_OTHER_EXTRADATATYPE *getExtradata(OMX_OTHER_EXTRADATATYPE *extraData, OMX_EXTRADATATYPE type);

#ifdef CAMERA_VCE_OMX_FD
    void sendOutputBuffer(void *outbuf, unsigned int offset, unsigned int size);
    void returnInputBuffer(void * buffer);
    void onOmxVceError(int error);
#endif
    
    class CommandHandler : public Thread
    {
    public:
        CommandHandler(OMXCameraAdapter* ca)
            : Thread(false), mCameraAdapter(ca) { }

        virtual bool threadLoop()
        {
            bool ret;
            ret = Handler();
            return ret;
        }

        status_t put(ActUtils::Message* msg)
        {
            Mutex::Autolock lock(mLock);
            return mCommandMsgQ.put(msg);
        }

        void clearCommandQ()
        {
            Mutex::Autolock lock(mLock);
            mCommandMsgQ.clear();
        }

        enum
        {
            COMMAND_EXIT = -1,
            CAMERA_START_IMAGE_CAPTURE = 0,
            CAMERA_SWITCH_TO_EXECUTING =1,
            CAMERA_END_IMAGE_CALLBACK = 2,
        };

    private:
        bool Handler();
        ActUtils::MessageQueue mCommandMsgQ;
        OMXCameraAdapter* mCameraAdapter;
        Mutex mLock;
    };
    sp<CommandHandler> mCommandHandler;


    class AFHandler : public Thread
    {
    public:
        AFHandler(OMXCameraAdapter* ca)
            : Thread(false), mCameraAdapter(ca) { }

        virtual bool threadLoop()
        {
            bool ret;
            ret = Handler();
            return ret;
        }

        status_t put(ActUtils::Message* msg)
        {
            Mutex::Autolock lock(mLock);
            return mCommandMsgQ.put(msg);
        }

        void clearCommandQ()
        {
            Mutex::Autolock lock(mLock);
            mCommandMsgQ.clear();
        }

        enum
        {
            COMMAND_EXIT = -1,
            CAMERA_PERFORM_AUTOFOCUS = 0,
        };

    private:
        bool Handler();
        ActUtils::MessageQueue mCommandMsgQ;
        OMXCameraAdapter* mCameraAdapter;
        Mutex mLock;
    };
    sp<AFHandler> mAFHandler;

public:

    class OMXCallbackHandler : public Thread
    {
    public:
        OMXCallbackHandler(OMXCameraAdapter* ca)
            : Thread(false), mCameraAdapter(ca) { }

        virtual bool threadLoop()
        {
            bool ret;
            ret = Handler();
            return ret;
        }

        status_t put(ActUtils::Message* msg)
        {
            Mutex::Autolock lock(mLock);
            return mCommandMsgQ.put(msg);
        }

        void clearCommandQ()
        {
            Mutex::Autolock lock(mLock);
            mCommandMsgQ.clear();
        }

        enum
        {
            COMMAND_EXIT = -1,
            CAMERA_FILL_BUFFER_DONE,
        };

    private:
        bool Handler();
        ActUtils::MessageQueue mCommandMsgQ;
        OMXCameraAdapter* mCameraAdapter;
        Mutex mLock;
    };

    sp<OMXCallbackHandler> mOMXCallbackHandler;

private:


    //OMX Capabilities data
    static const CapPixelformat mPixelformats [];
    static const CapU32 mSensorNames[] ;
    static const CapZoom mZoomStages [];
    static const CapEVComp mEVCompRanges [];
    static const CapISO mISOStages [];
    static const CapU32Pair mVarFrameratesDefault;
    static const CapFramerate mFramerates[];
    static const CapResolution mThumbRes[];

    static const CapResolution mExtendedPictureRes[];
    static const CapResolution mExtendedVideoRes[];

    // OMX Camera defaults
    static const char DEFAULT_ANTIBANDING[];
    static const char DEFAULT_BRIGHTNESS[];
    static const char DEFAULT_CONTRAST[];
    static const char DEFAULT_DENOISE[];
    static const char DEFAULT_EFFECT[];
    static const char DEFAULT_EV_COMPENSATION[];
    static const char DEFAULT_EV_STEP[];
    static const char DEFAULT_EXPOSURE_MODE[];
    static const char DEFAULT_FLASH_MODE[];
    static const char DEFAULT_FOCUS_MODE_PREFERRED[];
    static const char DEFAULT_FOCUS_MODE[];
    static const char DEFAULT_PREVIEW_FRAMERATE_RANGE[];
    static const char DEFAULT_IMAGE_FRAMERATE_RANGE[];
    static const char DEFAULT_ISO_MODE[];
    static const char DEFAULT_JPEG_QUALITY[];
    static const char DEFAULT_THUMBNAIL_QUALITY[];
    static const char DEFAULT_THUMBNAIL_SIZE[];
    static const char DEFAULT_PICTURE_FORMAT[];
    static const char DEFAULT_PICTURE_SIZE[];
    static const char DEFAULT_PREVIEW_FORMAT[];
    static const char DEFAULT_FRAMERATE[];
    static const char DEFAULT_PREVIEW_SIZE[];
    static const char DEFAULT_NUM_PREV_BUFS[];
    static const char DEFAULT_NUM_PIC_BUFS[];
    static const char DEFAULT_MAX_FOCUS_AREAS[];
    static const char DEFAULT_FOCUS_AREAS[];
    static const char DEFAULT_SATURATION[];
    static const char DEFAULT_SCENE_MODE[];
    static const char DEFAULT_SHARPNESS[];
    static const char DEFAULT_VSTAB[];
    static const char DEFAULT_VSTAB_SUPPORTED[];
    static const char DEFAULT_WB[];
    static const char DEFAULT_ZOOM[];
    static const char DEFAULT_MAX_FD_HW_FACES[];
    static const char DEFAULT_MAX_FD_SW_FACES[];
    static const char DEFAULT_AE_LOCK[];
    static const char DEFAULT_AWB_LOCK[];
    static const char DEFAULT_MAX_NUM_METERING_AREAS[];
    static const char DEFAULT_LOCK_SUPPORTED[];
    static const char DEFAULT_LOCK_UNSUPPORTED[];
    static const char DEFAULT_FOCAL_LENGTH_PRIMARY[];
    static const char DEFAULT_FOCAL_LENGTH_SECONDARY[];
    static const char DEFAULT_FOCUS_DISTANCES[];
    static const char DEFAULT_HOR_ANGLE[];
    static const char DEFAULT_VER_ANGLE[];
    static const char DEFAULT_VIDEO_SNAPSHOT_SUPPORTED[];
    static const char DEFAULT_VIDEO_SIZE[];
    static const char DEFAULT_PREFERRED_PREVIEW_SIZE_FOR_VIDEO[];

    static const char DEFAULT_PREVIEW_SIZE_SEC[];
    static const char DEFAULT_PICTURE_SIZE_SEC[];
    static const char DEFAULT_FRAMERATE_SEC[];
    static const char DEFAULT_PREVIEW_FRAMERATE_RANGE_SEC[];
    static const char DEFAULT_IMAGE_FRAMERATE_RANGE_SEC[];
    static const char DEFAULT_VIDEO_SIZE_SEC[];
    static const char DEFAULT_PREFERRED_PREVIEW_SIZE_FOR_VIDEO_SEC[];


    static const char DEFAULT_SENSOR_ROTATION[];
    static const char DEFAULT_SENSOR_ROTATION_SEC[];

    OMX_VERSIONTYPE mCompRevision;

    //OMX Component UUID
    OMX_UUIDTYPE mCompUUID;

    //Current Focus distances
    char mFocusDistNear[FOCUS_DIST_SIZE];
    char mFocusDistOptimal[FOCUS_DIST_SIZE];
    char mFocusDistFar[FOCUS_DIST_SIZE];
    char mFocusDistBuffer[FOCUS_DIST_BUFFER_SIZE];

    // Current Focus areas
    Vector< sp<CameraArea> > mFocusAreas;
    mutable Mutex mFocusAreasLock;
	bool mFocusAreasset;

    // Current Metering areas
    Vector< sp<CameraArea> > mMeteringAreas;
    mutable Mutex mMeteringAreasLock;

    size_t mBurstFrames;
    size_t mCapturedFrames;

    mutable Mutex mExifLock;
    KeyedVector<void*, void * > mExifQueue;

    mutable Mutex mFaceDetectionLock;
    //Face detection status
    bool mFaceDetectionRunning;
    bool mFaceDetectionPaused;

    camera_face_t  faceDetectionLastOutput [MAX_NUM_FACES_SUPPORTED];
    int faceDetectionNumFacesLastOutput;
#ifdef CAMERA_VCE_OMX_FD
    OMXVceFaceDetect *mVceFaceDetect;
    FrameProvider *mFDFrameProvider;
#endif

    //Geo-tagging
    EXIFData mEXIFData;

    //jpeg Picture Quality
    unsigned int mPictureQuality;

    //thumbnail resolution
    unsigned int mThumbWidth, mThumbHeight;

    //thumbnail quality
    unsigned int mThumbQuality;

    //variables holding the estimated framerate
    float mFPS, mLastFPS;

    //automatically disable AF after a given amount of frames
    unsigned int mFocusThreshold;

    //This is needed for the CTS tests. They falsely assume, that during
    //smooth zoom the current zoom stage will not change within the
    //zoom callback scope, which in a real world situation is not always the
    //case. This variable will "simulate" the expected behavior
    int mZoomParameterIdx;

    //current zoom
    Mutex mZoomLock;
    int mCurrentZoomIdx, mTargetZoomIdx, mPreviousZoomIndx;
    bool mZoomUpdating, mZoomUpdate;
    int mZoomInc;
    bool mReturnZoomStatus;//
    static const int32_t ZOOM_STEPS [];

    unsigned int mPending3Asettings;
    Mutex m3ASettingsUpdateLock;
    Gen3A_settings mParameters3A;//
    const char *mPictureFormatFromClient;

    //OMX_TI_CONFIG_3A_FACE_PRIORITY mFacePriority;
    //OMX_TI_CONFIG_3A_REGION_PRIORITY mRegionPriority;

    CameraParameters mParams;
    CameraProperties::Properties* mCapabilities;
    unsigned int mPictureRotation;
    bool mCaptureConfigured;
    unsigned int mPendingCaptureSettings;

    bool mCaptureUsePreviewFrame;
    bool mCaptureUsePreviewFrameStarted;

    bool mFlashStrobed;
    int mFlashConvergenceFrameConfig;
    int mFlashConvergenceFrame;

    bool mIternalRecordingHint;

    CameraParameters mParameters;
    OMXCameraAdapterComponentContext mCameraAdapterParameters;
    bool mFirstTimeInit;

    ///Semaphores used internally
    Semaphore mDoAFSem;
    Semaphore mInitSem;
    Semaphore mFlushSem;
    Semaphore mUsePreviewDataSem;
    Semaphore mUsePreviewSem;
    Semaphore mUseCaptureSem;
    Semaphore mStartPreviewSem;
    Semaphore mStopPreviewSem;
    Semaphore mStartCaptureSem;
    Semaphore mStopCaptureSem;
    Semaphore mSwitchToLoadedSem;
    Semaphore mSwitchToExecSem;

    mutable Mutex mStateSwitchLock;

    Vector<struct ActUtils::Message *> mEventSignalQ;
    Mutex mEventLock;

    OMX_STATETYPE mComponentState;

    bool mVnfEnabled;
    bool mVstabEnabled;

    int mSensorOrientation;
    int mDeviceOrientation;
    int mDeviceType;
    bool mSensorOverclock;

    //Indicates if we should leave
    //OMX_Executing state during
    //stop-/startPreview
    bool mOMXStateSwitch;

    int mFrameCount;
    int mLastFrameCount;
    unsigned int mIter;
    nsecs_t mLastFPSTime;
    Mutex mFrameCountMutex;
    Condition mFirstFrameCondition;

    Mutex mDoAFMutex;
    Condition mDoAFCond;
    bool mCheckAF;
    
    OMX_BOOL mFocusPending;
      
    size_t mSensorIndex;
    /**
    * NEW_FEATURE: Add UVC module support .
    *ActionsCode(author:liyuan, change_code)
    */
    static OMX_UVCMODE mUVCReplaceMode;

    Semaphore mCaptureSem;
    bool mCaptureSignalled;

    OMX_BOOL mUserSetExpLock;
    OMX_BOOL mUserSetWbLock;

    bool mSnapshot;
    bool mHdrSupport;
    bool mFlashSupport;

    bool mHflip;
    bool mVflip;

    sp<CameraWatchDog> mWatchDog;

};
}; //// namespace
#endif //OMX_CAMERA_ADAPTER_H

