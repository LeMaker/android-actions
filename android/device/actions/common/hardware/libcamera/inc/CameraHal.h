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


#ifndef ANDROID_HARDWARE_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_CAMERA_HARDWARE_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <utils/threads.h>
#include <linux/videodev2.h>
#include "binder/MemoryBase.h"
#include "binder/MemoryHeapBase.h"
#include <utils/threads.h>
#include <camera/CameraParameters.h>
#include <hardware/camera.h>
#include "MessageQueue.h"
#include "Semaphore.h"
#include "CameraProperties.h"
#include "ActCameraParameters.h"
#include "SensorListener.h"
#include "CameraUtils.h"
#include "OMXVce.h"
#include "CameraHWCaps.h"

#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBuffer.h>

/*
*1.2.0 porting to android-4.4
*1.1.1 add checksum for media_profiles.xml
*1.1.0 auto genarate media_profiles.xml
*
*/
 #define CAMERAHAL_VERSION "CameraHal-1.2.0"

#define MAX_FD_HW_NUM   32
#define MAX_FD_HW_NUM_STR "32"

#define DEFAULT_PREVIEW_WIDTH           640
#define DEFAULT_PREVIEW_HEIGHT          480
#define DEFAULT_PICTURE_WIDTH   640 
#define DEFAULT_PICTURE_HEIGHT  480 


#define DEFAULT_FRAME_RATE 15

//5mp
//#define MAX_PICTURE_WIDTH   2600
//#define MAX_PICTURE_HEIGHT  2000 
//ActionsCode(author:liuyiguang, change_code)
//8mp
#define MAX_PICTURE_WIDTH   3300
#define MAX_PICTURE_HEIGHT  2500 
#define MAX_PICTURE_BPP  12 

#define MAX_CAMERA_BUFFERS    8 //NUM_OVERLAY_BUFFERS_REQUESTED
#define MAX_CAMERA_CAPTURE_BUFFERS 2

#define DEFAULT_THUMB_WIDTH     128
#define DEFAULT_THUMB_HEIGHT    96

#define SATURATION_OFFSET 0
#define SHARPNESS_OFFSET 0
#define CONTRAST_OFFSET 0

#define CAMHAL_GRALLOC_USAGE (GRALLOC_USAGE_HW_CAMERA | GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN)
#define CAMHAL_GRALLOC_LOCK_USAGE  (GRALLOC_USAGE_SW_READ_OFTEN)
#define CAMHAL_GRALLOC_VIDEO_USAGE  (GRALLOC_USAGE_HW_CAMERA|GRALLOC_USAGE_SW_WRITE_OFTEN)

#define CAMHAL_GRALLOC_VIDEO_LOCK_USAGE (GRALLOC_USAGE_SW_WRITE_OFTEN) 

//Enables Absolute PPM measurements in logcat
#define PPM_INSTRUMENTATION_ABS 1

#define LOCK_BUFFER_TRIES 5


#define NONNEG_ASSIGN(x,y) \
    if(x > -1) \
        y = x

#define ALIGN16(n) (((n)+0xf)&(~0xf))

//exif
#define EXIF_MODEL_SIZE             100
#define EXIF_MAKE_SIZE              100
#define EXIF_DATE_TIME_SIZE         20

#define GPS_MIN_DIV                 60
#define GPS_SEC_DIV                 60
#define GPS_SEC_ACCURACY            1000
#define GPS_TIMESTAMP_SIZE          6
#define GPS_DATESTAMP_SIZE          11
#define GPS_REF_SIZE                2
#define GPS_MAPDATUM_SIZE           100
#define GPS_PROCESSING_SIZE         100
#define GPS_VERSION_SIZE            4
#define GPS_NORTH_REF               "N"
#define GPS_SOUTH_REF               "S"
#define GPS_EAST_REF                "E"
#define GPS_WEST_REF                "W"


#define MAX_CAMERA_ZOOM (4)

#define MAX_CAMERA_SCALE (MAX_OMX_VCE_SCALE/MAX_CAMERA_ZOOM)

//ActionsCode(author:liuyiguang, add_code)
#define INT_ALIGN   10 

namespace android
{


#define PARAM_BUFFER            256

///Forward declarations
class CameraHal;
class CameraFrame;
class CameraHalEvent;
class DisplayFrame;
class OMXVceJpegEncoder;
class MemoryManager;
class ExifElementsTable;


class  CameraHalExif
{
public:
    uint32_t    width;
    uint32_t    height;

    bool        bExifEnable;
    uint32_t    imageOri;

    char        dateTime[EXIF_DATE_TIME_SIZE];
    char        exifmake[EXIF_MAKE_SIZE];
    char        exifmodel[EXIF_MODEL_SIZE];
    uint32_t    focalLengthL;
    uint32_t    focalLengthH;
    bool        bGPS;
    uint32_t    gpsLATL[3];//latitude
    uint32_t    gpsLATH[3];
    uint32_t    gpsLATREF; //N:0,S:1

    uint32_t    gpsLONGL[3];//longitude
    uint32_t    gpsLONGH[3];
    uint32_t    gpsLONGREF; //E:0,W:1

    uint32_t    gpsALTL;
    uint32_t    gpsALTH;
    uint32_t    gpsALTREF; //Sea level:0, under sea level:1

    char        gpsProcessMethod[GPS_PROCESSING_SIZE];
    uint32_t    gpsTimeL[3];
    uint32_t    gpsTimeH[3];
    char        gpsDatestamp[GPS_DATESTAMP_SIZE];
};


class CameraHalAsyncCaller : public Thread {
public:
    CameraHalAsyncCaller(void *func,void *param)
        :  Thread(false),mCallee(func),mParam(param) { }
    CameraHalAsyncCaller()
        : Thread(false),mCallee(NULL),mParam(NULL) { }

    void setCallee(void *func, void *param)
    {
        this->join();
        mCallee = func;
        mParam = param;
    }
    virtual bool threadLoop() {
        if(mCallee)
        {
            ((void (*)(void *))mCallee)(mParam);
        }
        return false;
    }

private:
    void* mCallee;
    void* mParam;
};


class CameraFormatConverter
{
public:
    static int halFormatForCameraFormat(const char *cameraformat);
};

class CameraArea : public RefBase
{
public:

    CameraArea(ssize_t top,
               ssize_t left,
               ssize_t bottom,
               ssize_t right,
               size_t weight) : mTop(top),
        mLeft(left),
        mBottom(bottom),
        mRight(right),
        mWeight(weight) {}

    status_t transfrom(size_t width,
                       size_t height,
                       size_t &top,
                       size_t &left,
                       size_t &areaWidth,
                       size_t &areaHeight);

    bool isValid()
    {
        return ( ( 0 != mTop ) || ( 0 != mLeft ) || ( 0 != mBottom ) || ( 0 != mRight) );
    }

    bool isZeroArea()
    {
        return  ( (0 == mTop ) && ( 0 == mLeft ) && ( 0 == mBottom )
                  && ( 0 == mRight ) && ( 0 == mWeight ));
    }

    size_t getWeight()
    {
        return mWeight;
    }

    bool compare(const sp<CameraArea> &area);

    static status_t parseAreas(const char *area,
                               size_t areaLength,
                               Vector< sp<CameraArea> > &areas);

    static status_t checkArea(ssize_t top,
                              ssize_t left,
                              ssize_t bottom,
                              ssize_t right,
                              ssize_t weight);

    static bool areAreasDifferent(Vector< sp<CameraArea> > &, Vector< sp<CameraArea> > &);

protected:
    static const ssize_t TOP = -1000;
    static const ssize_t LEFT = -1000;
    static const ssize_t BOTTOM = 1000;
    static const ssize_t RIGHT = 1000;
    static const ssize_t WEIGHT_MIN = 1;
    static const ssize_t WEIGHT_MAX = 1000;

    ssize_t mTop;
    ssize_t mLeft;
    ssize_t mBottom;
    ssize_t mRight;
    size_t mWeight;
};

class CameraFDResult : public RefBase
{
public:

    CameraFDResult() : mFaceData(NULL) {};
    CameraFDResult(camera_frame_metadata_t *faces) : mFaceData(faces) {};

    virtual ~CameraFDResult()
    {
        if ( ( NULL != mFaceData ) && ( NULL != mFaceData->faces ) )
        {
            free(mFaceData->faces);
            free(mFaceData);
            mFaceData=NULL;
        }

        if(( NULL != mFaceData ))
        {
            free(mFaceData);
            mFaceData = NULL;
        }
    }

    camera_frame_metadata_t *getFaceResult()
    {
        return mFaceData;
    };

    static const ssize_t TOP = -1000;
    static const ssize_t LEFT = -1000;
    static const ssize_t BOTTOM = 1000;
    static const ssize_t RIGHT = 1000;
    static const ssize_t INVALID_DATA = -2000;

private:

    camera_frame_metadata_t *mFaceData;
};

class CameraFrame
{
public:

    //NOTE:IMAGE_FRAME_SYNC should be in the tail
    enum FrameType
    {
        PREVIEW_FRAME_SYNC = (1<<0), ///SYNC implies that the frame needs to be explicitly returned after consuming in order to be filled by camera again
        PREVIEW_RAW_FRAME_SYNC = (1<<1),
        VIDEO_FRAME_SYNC = (1<<2), ///Timestamp will be updated for these frames
        FD_FRAME_SYNC = (1<<3),
        RAW_FRAME_SYNC = (1<<4),
        IMAGE_FRAME_SYNC = (1<<5), ///Image Frame is the image capture output frame
        MAX_FRAME_TYPE = 6, 
        ALL_FRAMES = 0xFFFF   ///Maximum of 16 frame types supported
    };

    static uint32_t toFrameTypeIndex(FrameType type)
    {
        return __builtin_ctz(type);
    }

    enum StreamType{
        UNKNOWN_STREAM_TYPE = -1,
        PREVIEW_STREAM_TYPE = 0,
        IMAGE_STREAM_TYPE = 1,
        MAX_STREAM_TYPE,
    };

    enum FrameFormat
    {
        CAMERA_FRAME_FORMAT_YUV420SP = 0,
        CAMERA_FRAME_FORMAT_YUV420P,
        CAMERA_FRAME_FORMAT_YUV420SP_NV12,//NV12
        CAMERA_FRAME_FORMAT_YUV420P_YU12, //YU12
        CAMERA_FRAME_FORMAT_RGB565,
        CAMERA_FRAME_FORMAT_YUV422I,
        CAMERA_FRAME_FORMAT_JPEG,
        CAMERA_FRAME_FORMAT_PNG,

    };

    static const char *getCameraFormat(FrameFormat frameformat)
    {
        switch(frameformat)
        {
        case CAMERA_FRAME_FORMAT_YUV420SP:
            return CameraParameters::PIXEL_FORMAT_YUV420SP;
        case CAMERA_FRAME_FORMAT_YUV420P:
            return CameraParameters::PIXEL_FORMAT_YUV420P;
        case CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            return ActCameraParameters::PIXEL_FORMAT_YUV420SP_NV12;
        case CAMERA_FRAME_FORMAT_YUV420P_YU12:
            return ActCameraParameters::PIXEL_FORMAT_YUV420P_YU12;
        case CAMERA_FRAME_FORMAT_RGB565:
            return CameraParameters::PIXEL_FORMAT_RGB565;
        case CAMERA_FRAME_FORMAT_YUV422I:
            return CameraParameters::PIXEL_FORMAT_YUV422I;

        default:
            return NULL;

        }
        return NULL;
    }
    static FrameFormat getFrameFormat(const char *valstr)
    {
        if(!strcmp(valstr,CameraParameters::PIXEL_FORMAT_YUV420P) )
        {
            return CAMERA_FRAME_FORMAT_YUV420P;
        }
        else if(!strcmp(valstr,ActCameraParameters::PIXEL_FORMAT_YUV420P_YU12 ))
        {
            return CAMERA_FRAME_FORMAT_YUV420P_YU12;
        }
        else if(!strcmp(valstr,CameraParameters::PIXEL_FORMAT_YUV420SP))
        {
            return CAMERA_FRAME_FORMAT_YUV420SP;
        }
        else if(!strcmp(valstr,ActCameraParameters::PIXEL_FORMAT_YUV420SP_NV12 ))
        {
            return CAMERA_FRAME_FORMAT_YUV420SP_NV12;
        }
        else if(!strcmp(valstr,CameraParameters::PIXEL_FORMAT_YUV422I))
        {
            return CAMERA_FRAME_FORMAT_YUV422I;
        }
        else if(!strcmp(valstr,CameraParameters::PIXEL_FORMAT_RGB565))
        {
            return CAMERA_FRAME_FORMAT_RGB565;
        }
        else
        {
        }     
        return (FrameFormat)-1;
    }

    static int getFrameLength(FrameFormat format, int w, int h)
    {
        int size = 0;
        int bpp = 12;
        if(format == CameraFrame::CAMERA_FRAME_FORMAT_YUV422I)
        {
            bpp = 16;
            size = (w*h*bpp)>>3;
        }
        else if(format == CameraFrame::CAMERA_FRAME_FORMAT_YUV420P)
        {
            bpp = 12;
            size = (ALIGN16(w)*h) + (ALIGN16(w>>1)*h);
        }
        else if(format == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP)
        {
            bpp = 12;
            size = (ALIGN16(w)*h) + (ALIGN16(w)*(h>>1));
        }
        else if(format == CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12)
        {
            bpp = 12;
            size = (ALIGN16(w)*h) + (ALIGN16(w>>1)*h);
        }
        else if(format == CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12)
        {
            bpp = 12;
            size = (ALIGN16(w)*h) + (ALIGN16(w)*(h>>1));
        }
        else if(format == CameraFrame::CAMERA_FRAME_FORMAT_RGB565)
        {
            bpp = 16;
            size = (w*h*bpp)>>3;
        }
        else
        {
            bpp = 16;
            size = (w*h*bpp)>>3;
        }
        return size;
    }
    int getEncodeFrameLength()
    {
        return getFrameLength((FrameFormat)mConvFormat, mEncodeWidth, mEncodeHeight);
    }

    static int getHalFormat(const char *cameraformat)
    {
        if(cameraformat == NULL)
        {
            return -1;
        }
        if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_RGB565) == 0)
        {
            return HAL_PIXEL_FORMAT_RGB_565;
        }
        else if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_RGBA8888) == 0)
        {
            return HAL_PIXEL_FORMAT_RGBA_8888;
        }
        else if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_YUV422SP) == 0)
        {
            return HAL_PIXEL_FORMAT_YCbCr_422_SP;
        }
        else if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_YUV420SP) == 0)
        {
            return HAL_PIXEL_FORMAT_YCrCb_420_SP;
        }
        else if(strcmp(cameraformat, ActCameraParameters::PIXEL_FORMAT_YUV420SP_NV12) == 0)
        {
            return HAL_PIXEL_FORMAT_ACT_NV12;
        }
        else if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_YUV422I) == 0)
        {
            return HAL_PIXEL_FORMAT_YCbCr_422_I;
        }
        else if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_YUV420P) == 0)
        {
            return HAL_PIXEL_FORMAT_YV12;
        }
        else if(strcmp(cameraformat, ActCameraParameters::PIXEL_FORMAT_YUV420P_YU12) == 0)
        {
            return HAL_PIXEL_FORMAT_ACT_YU12;
        }
        else if(strcmp(cameraformat, CameraParameters::PIXEL_FORMAT_JPEG) == 0)
        {
            return -1;
        }
        else
        {
            return -1;
        }
        return -1;

    }

    static FrameFormat getFrameFormatFromOmx(OMX_COLOR_FORMATTYPE omxFormat)
    {
        FrameFormat format = (FrameFormat)-1;
        switch((int)omxFormat)
        {
            case OMX_COLOR_FormatYUV420SemiPlanar:
            format =  CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12;
            break;

            case (OMX_COLOR_FORMATTYPE)OMX_COLOR_FormatYVU420SemiPlanar:
            format =  CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP;
            break;

            case OMX_COLOR_FormatYUV420Planar:
            format =  CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12;
            break;

            case (OMX_COLOR_FORMATTYPE)OMX_COLOR_FormatYVU420Planar:
            format =  CameraFrame::CAMERA_FRAME_FORMAT_YUV420P;
            break;

            case OMX_COLOR_Format16bitRGB565:
            format =  CameraFrame::CAMERA_FRAME_FORMAT_RGB565;
            break;

            case OMX_COLOR_FormatYCbYCr:
            format =  CameraFrame::CAMERA_FRAME_FORMAT_YUV422I;
            break;

            default:
            break;
        }
        return format;
    }

    static OMX_COLOR_FORMATTYPE getOmxFormat(FrameFormat format)
    {
        OMX_COLOR_FORMATTYPE omxFormat = (OMX_COLOR_FORMATTYPE)-1;
        switch(format)
        {
            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP_NV12:
            omxFormat =  OMX_COLOR_FormatYUV420SemiPlanar;
            break;

            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420SP:
            omxFormat =  (OMX_COLOR_FORMATTYPE)OMX_COLOR_FormatYVU420SemiPlanar;
            break;

            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P_YU12:
            omxFormat =  OMX_COLOR_FormatYUV420Planar;
            break;

            case CameraFrame::CAMERA_FRAME_FORMAT_YUV420P:
            omxFormat =  (OMX_COLOR_FORMATTYPE)OMX_COLOR_FormatYVU420Planar;
            break;

            case CameraFrame::CAMERA_FRAME_FORMAT_RGB565:
            omxFormat =  OMX_COLOR_Format16bitRGB565;
            break;

            case CameraFrame::CAMERA_FRAME_FORMAT_YUV422I:
            omxFormat =  OMX_COLOR_FormatYCbYCr;
            break;

            default:
            break;
        }
        return omxFormat;
    }



    enum FrameQuirks
    {
        ENCODE_RAW_TO_JPEG = 0x1 << 0,
        HAS_EXIF_DATA = 0x1 << 1,
        HAS_EXIF_DATA_LIBJPEG = 0x1 << 2,
        HAS_EXIF_DATA_VCE = 0x1 << 3,
        ENCODE_WITH_SW = 0x1 << 4,
    };

    //default contrustor
    CameraFrame():
        mCookie(NULL),
        mCookie2(NULL),
        mBuffer(NULL),
        mFrameType(0),
        mStreamType(CameraFrame::PREVIEW_STREAM_TYPE),
        mTimestamp(0),
        mWidth(0),
        mHeight(0),
        mOffset(0),
        mXOff(0),
        mYOff(0),
        mOrigWidth(0),
        mOrigHeight(0),
        mEncodeWidth(0),
        mEncodeHeight(0),
        mStride(0),
        mFd(0),
        mLength(0),
        mQuirks(0),
        mFormat(CAMERA_FRAME_FORMAT_YUV420SP),
        mConvFormat(CAMERA_FRAME_FORMAT_YUV420SP)
    {

        mVaddr = NULL;
#ifdef CAMERA_FRAME_STAT
        mCaptureTime = 0;
        mCaptureIntervalTime = 0;
        mCaptureScheTime = 0;
#endif
    }

    //copy constructor
    CameraFrame(const CameraFrame &frame) :
        mCookie(frame.mCookie),
        mCookie2(frame.mCookie2),
        mBuffer(frame.mBuffer),
        mFrameType(frame.mFrameType),
        mStreamType(frame.mStreamType),
        mTimestamp(frame.mTimestamp),
        mWidth(frame.mWidth),
        mHeight(frame.mHeight),
        mOffset(frame.mOffset),
        mXOff(frame.mXOff),
        mYOff(frame.mYOff),
        mOrigWidth(frame.mOrigWidth),
        mOrigHeight(frame.mOrigHeight),
        mEncodeWidth(frame.mEncodeWidth),
        mEncodeHeight(frame.mEncodeHeight),
        mStride(frame.mStride),
        mFd(frame.mFd),
        mLength(frame.mLength),
        mQuirks(frame.mQuirks),
        mFormat(frame.mFormat),
        mConvFormat(frame.mConvFormat)
    {
        mVaddr = frame.mVaddr;
#ifdef CAMERA_FRAME_STAT
        mCaptureTime = frame.mCaptureTime;
        mCaptureIntervalTime = frame.mCaptureIntervalTime;
        mCaptureScheTime = frame.mCaptureScheTime;
#endif
    }

    void *mCookie;
    void *mCookie2;
    void *mBuffer;
    int mFrameType;
    StreamType mStreamType;
    nsecs_t mTimestamp;
    unsigned int mWidth, mHeight;
    uint32_t mOffset;
    uint32_t mXOff;
    uint32_t mYOff;
    unsigned int mOrigWidth, mOrigHeight;
    unsigned int mEncodeWidth, mEncodeHeight;
    unsigned int mStride;
    int mFd;
    size_t mLength;
    unsigned int mQuirks;
    unsigned int mFormat;
    unsigned int mConvFormat;
    void * mVaddr;

#ifdef CAMERA_FRAME_STAT
    unsigned long long mCaptureTime;
    int mCaptureIntervalTime;
    int mCaptureScheTime;
#endif
    ///@todo add other member vars like  stride etc
};

enum CameraHalError
{
    CAMERA_ERROR_FATAL = 0x1, //Fatal errors can only be recovered by restarting media server
    CAMERA_ERROR_HARD = 0x2,  // Hard errors are hardware hangs that may be recoverable by resetting the hardware internally within the adapter
    CAMERA_ERROR_SOFT = 0x4, // Soft errors are non fatal errors that can be recovered from without needing to stop use-case
};

///Common Camera Hal Event class which is visible to CameraAdapter,DisplayAdapter and AppCallbackNotifier
///@todo Rename this class to CameraEvent
class CameraHalEvent
{
public:
    //Enums
    enum CameraHalEventType
    {
        NO_EVENTS = 0x0,
        EVENT_FOCUS_LOCKED = 0x1,
        EVENT_FOCUS_ERROR = 0x2,
        EVENT_ZOOM_INDEX_REACHED = 0x4,
        EVENT_SHUTTER = 0x8,
        EVENT_FACE = 0x10,
        EVENT_RAW_IMAGE_NOTIFY = 0x20,
        EVENT_ERROR_NOTIFY = 0x40,
        ///@remarks Future enum related to display, like frame displayed event, could be added here
        ALL_EVENTS = 0xFFFF ///Maximum of 16 event types supported
    };

    enum FocusStatus
    {
        FOCUS_STATUS_INVALID = -1,
        FOCUS_STATUS_SUCCESS = 0x1,
        FOCUS_STATUS_FAIL = 0x2,
        FOCUS_STATUS_PENDING = 0x4,
        FOCUS_STATUS_DONE = 0x8,
    };
    ///Class declarations
    ///@remarks Add a new class for a new event type added above

    //Shutter event specific data
    typedef struct ShutterEventData_t
    {
        bool shutterClosed;
    } ShutterEventData;

    ///Focus event specific data
    typedef struct FocusEventData_t
    {
        FocusStatus focusStatus;
        int currentFocusValue;
    } FocusEventData;

    ///Zoom specific event data
    typedef struct ZoomEventData_t
    {
        int currentZoomIndex;
        bool targetZoomIndexReached;
    } ZoomEventData;

    typedef struct FaceData_t
    {
        ssize_t top;
        ssize_t left;
        ssize_t bottom;
        ssize_t right;
        size_t score;
    } FaceData;

    typedef sp<CameraFDResult> FaceEventData;

    class CameraHalEventData : public RefBase
    {

    public:

        CameraHalEvent::FocusEventData focusEvent;
        CameraHalEvent::ZoomEventData zoomEvent;
        CameraHalEvent::ShutterEventData shutterEvent;
        CameraHalEvent::FaceEventData faceEvent;
    };

    //default contrustor
    CameraHalEvent():
        mCookie(NULL),
        mEventType(NO_EVENTS) {}

    //copy constructor
    CameraHalEvent(const CameraHalEvent &event) :
        mCookie(event.mCookie),
        mEventType(event.mEventType),
        mEventData(event.mEventData) {};

    void* mCookie;
    CameraHalEventType mEventType;
    sp<CameraHalEventData> mEventData;

};

///      Have a generic callback class based on template - to adapt CameraFrame and Event
typedef void (*frame_callback) (CameraFrame *cameraFrame);
typedef void (*event_callback) (CameraHalEvent *event);

//signals CameraHAL to relase image buffers
typedef void (*release_image_buffers_callback) (void *userData);
typedef void (*end_image_capture_callback) (void *userData);

/**
  * Interface class implemented by classes that have some events to communicate to dependendent classes
  * Dependent classes use this interface for registering for events
  */
class MessageNotifier
{
public:
    static const uint32_t EVENT_BIT_FIELD_POSITION;

    ///@remarks Msg type comes from CameraFrame and CameraHalEvent classes
    ///           MSB 16 bits is for events and LSB 16 bits is for frame notifications
    ///         FrameProvider and EventProvider classes act as helpers to event/frame
    ///         consumers to call this api
    virtual void enableMsgType(int32_t msgs, event_callback eventCb=NULL, void* cookie=NULL) = 0;
    virtual void disableMsgType(int32_t msgs, void* cookie) = 0;

    virtual ~MessageNotifier() {};
};

class ErrorNotifier : public virtual RefBase
{
public:
    virtual void errorNotify(int error) = 0;

    virtual ~ErrorNotifier() {};
};


/**
  * Interace class abstraction for Camera Adapter to act as a frame provider
  * This interface is fully implemented by Camera Adapter
  */
class FrameNotifier  
{
public:
    static const uint32_t FRAME_BIT_FIELD_POSITION;
    virtual void enableFrameMsgType(int32_t msgs, frame_callback frameCb=NULL, void* cookie=NULL) = 0;
    virtual void disableFrameMsgType(int32_t msgs, void* cookie) = 0;

    virtual void returnFrame(void* frameBuf, CameraFrame::FrameType frameType, CameraFrame::StreamType streamType) = 0;
    virtual ~FrameNotifier() {};
};

/**   * Wrapper class around Frame Notifier, which is used by display and notification classes for interacting with Camera Adapter
  */
class FrameProvider
{
    FrameNotifier* mFrameNotifier;
    void* mCookie;
    frame_callback mFrameCallback;

public:
    FrameProvider(FrameNotifier *fn, void* cookie, frame_callback frameCallback)
        :mFrameNotifier(fn), mCookie(cookie),mFrameCallback(frameCallback) { }

    int enableFrameNotification(int32_t frameTypes);
    int disableFrameNotification(int32_t frameTypes);
    int returnFrame(void *frameBuf, CameraFrame::FrameType frameType, CameraFrame::StreamType streamType);
};

/** Wrapper class around MessageNotifier, which is used by display and notification classes for interacting with
   *  Camera Adapter
  */
class EventProvider
{
public:
    MessageNotifier* mEventNotifier;
    void* mCookie;
    event_callback mEventCallback;

public:
    EventProvider(MessageNotifier *mn, void* cookie, event_callback eventCallback)
        :mEventNotifier(mn), mCookie(cookie), mEventCallback(eventCallback) {}

    int enableEventNotification(int32_t eventTypes);
    int disableEventNotification(int32_t eventTypes);
};

/*
  * Interface for providing buffers
  */
class BufferProvider
{
public:
    virtual void* allocateBuffer(int width, int height, const char* format, int &bytes, int numBufs) = 0;

    //additional methods used for memory mapping
    virtual uint32_t * getOffsets() = 0;
    virtual void* getVaddrs() = 0;

    virtual int getFd() = 0 ;
    virtual int freeBuffer(void* buf)= 0;

    virtual ~BufferProvider() {}
};

/**
  * Class for handling data and notify callbacks to application
  */
class   AppCallbackNotifier: public ErrorNotifier , public virtual RefBase
{

public:

    ///Constants
    static const int NOTIFIER_TIMEOUT;
    static const int32_t MAX_BUFFERS = 20;
    static const int32_t PREVIEW_COPY_BUFFERS = 8;

    enum NotifierCommands
    {
        NOTIFIER_CMD_PROCESS_EVENT,
        NOTIFIER_CMD_PROCESS_FRAME,
        NOTIFIER_CMD_PROCESS_MEMORY_FRAME,
        NOTIFIER_CMD_PROCESS_ERROR,
    };

    enum ResizeCommands
    {
        RESIZE_CMD_RESIZE,
    };
    enum JpegCommands
    {
        JPEG_CMD_ENCODE,
    };
    enum NotifierState
    {
        NOTIFIER_STOPPED,
        NOTIFIER_STARTED,
        NOTIFIER_EXITED,
    };
    enum FlushCommands
    {
        PREVIEW_START_FLUSH = 1<<0,
        PREVIEW_STOP_FLUSH  = 1<<1,
        CAPTURE_START_FLUSH = 1<<2,
        CAPTURE_STOP_FLUSH  = 1<<3,
        VIDEO_START_FLUSH   = 1<<4,
        VIDEO_STOP_FLUSH    = 1<<5,
    };

private:
    enum
    {
        USE_PHY_ADDRESS     = 0,
        USE_VIRTUAL_ADDRESS = 1,
    };
public:
    AppCallbackNotifier() 
#ifdef CAMERA_VCE_OMX_JPEG
        :mImageJpegObserver(this)
#endif
#ifdef CAMERA_VCE_OMX_RESIZE
#ifdef CAMERA_VCE_OMX_JPEG
        ,mImageResizeObserver(this)
#else
        :mImageResizeObserver(this)
#endif
#endif
    {
    };

    ~AppCallbackNotifier();

    ///Initialzes the callback notifier, creates any resources required
    status_t initialize();

    ///Starts the callbacks to application
    status_t start();

    ///Stops the callbacks from going to application
    status_t stop();

    void setEventProvider(int32_t eventMask, MessageNotifier * eventProvider);
    void setFrameProvider(FrameNotifier *frameProvider);

    //All sub-components of Camera HAL call this whenever any error happens
    virtual void errorNotify(int error);

    status_t startPreviewCallbacks(CameraParameters &params, void *buffers, uint32_t *offsets, int fd, size_t length, size_t count);
    status_t stopPreviewCallbacks();
    status_t PreAllocPreviewCBbufs(int w, int h, CameraFrame::FrameFormat format, size_t count);

    status_t startImageCallbacks(CameraParameters &params, void *buffers, uint32_t *offsets, int fd, size_t length, size_t count);
    status_t stopImageCallbacks();
    status_t waitforImageBufs();
    status_t _waitforImageBufs();
    status_t clearEncoderQueue();

    status_t enableMsgType(int32_t msgType);
    status_t disableMsgType(int32_t msgType);

    //API for enabling/disabling measurement data
    void setMeasurements(bool enable);

    //thread loops
    bool notificationThread();

    ///Notification callback functions
    static void frameCallbackRelay(CameraFrame* caFrame);
    static void eventCallbackRelay(CameraHalEvent* chEvt);
    void frameCallback(CameraFrame* caFrame);
    void eventCallback(CameraHalEvent* chEvt);
    void flushAndReturnFrames();
    void flushFrames();

    void setCallbacks(CameraHal *cameraHal,
                      camera_notify_callback notify_cb,
                      camera_data_callback data_cb,
                      camera_data_timestamp_callback data_cb_timestamp,
                      camera_request_memory get_memory,
                      void *user);


    //Notifications from CameraHal for video recording case
    status_t startRecording();
    status_t stopRecording();
    status_t initSharedVideoBuffers(CameraParameters &params,void *buffers, uint32_t *offsets, int fd, size_t length, size_t count, void *vidBufs);
    status_t releaseRecordingFrame(const void *opaque);

    status_t useMetaDataBufferMode(bool enable);
    bool getMetaDataBufferMode();

    void setPreviewFormat(CameraFrame::FrameFormat format);
    void setVideoFormat(CameraFrame::FrameFormat format);

    void EncoderDoneCb(void*, void*, CameraFrame::FrameType type, void* cookie1, void* cookie2, void* cookie3, bool canceled);
#ifdef CAMERA_VCE_OMX_JPEG
    void EncoderVceDoneCb(void*, void*, CameraFrame::FrameType type, void* cookie1, void* cookie2, void* cookie3, bool canceled);
#endif

    void useVideoBuffers(bool useVideoBuffers);

    bool getUseVideoBuffers();
    void setVideoRes(int width, int height);

    void flushEventQueue();

#ifdef CAMERA_VCE_OMX_RESIZE
    enum{
        UNUSED_RESIZE,
        PRVIEW_RESIZE,
        CAPTURE_RESIZE
    };
    void vceResize(CameraFrame* frame, unsigned char *src, unsigned char *dest, int resizeType, bool align);
    void vceResizeImprove(CameraFrame* frame, unsigned char *src, unsigned char *dest);
#endif

    //Internal class definitions
    class NotificationThread : public Thread
    {
        AppCallbackNotifier* mAppCallbackNotifier;
        ActUtils::MessageQueue mNotificationThreadQ;
    public:
        enum NotificationThreadCommands
        {
            NOTIFIER_START,
            NOTIFIER_STOP,
            NOTIFIER_EXIT,
        };

    public:
        NotificationThread(AppCallbackNotifier* nh)
            : Thread(false), mAppCallbackNotifier(nh) { }
        virtual bool threadLoop()
        {
            return mAppCallbackNotifier->notificationThread();
        }

        ActUtils::MessageQueue &msgQ()
        {
            return mNotificationThreadQ;
        }
    };

    class JpegThread : public Thread
    {
        AppCallbackNotifier* mAppCallbackNotifier;
        ActUtils::MessageQueue mJpegThreadQ;
    public:
        enum JpegThreadCommands
        {
            JPEG_START,
            JPEG_STOP,
            JPEG_EXIT,
            JPEG_FLUSH,
        };
       
    public:
        JpegThread(AppCallbackNotifier* nh)
            : Thread(false), mAppCallbackNotifier(nh) 
        {
            mPreviewValid = false;
            mPictureValid = false;
        }
        virtual bool threadLoop()
        {
            return mAppCallbackNotifier->jpegThread(this);
        }

        ActUtils::MessageQueue &msgQ()
        {
            return mJpegThreadQ;
        }

        bool mPreviewValid;
        bool mPictureValid;
        bool mVideoValid;
    };
    class ResizeThread : public Thread
    {
        AppCallbackNotifier* mAppCallbackNotifier;
        ActUtils::MessageQueue mResizeThreadQ;
    public:
        enum ResizeThreadCommands
        {
            RESIZE_START,
            RESIZE_STOP,
            RESIZE_EXIT,
            RESIZE_FLUSH,
        };
       
    public:
        ResizeThread(AppCallbackNotifier* nh)
            : Thread(false), mAppCallbackNotifier(nh) 
        {
            mPreviewValid = false;
            mPictureValid = false;
        }
        virtual bool threadLoop()
        {
            return mAppCallbackNotifier->resizeThread(this);
        }

        ActUtils::MessageQueue &msgQ()
        {
            return mResizeThreadQ;
        }

        bool mPreviewValid;
        bool mPictureValid;
        bool mVideoValid;
    };

    //Friend declarations
    friend class NotificationThread;
    friend class JpegThread;
    friend class ResizeThread;

    class ImageJpegObserver : public OMXVceObserver
    {
    public:
        ImageJpegObserver(AppCallbackNotifier *nh):mAppCallbackNotifier(nh){};
        void sendOutputBuffer(void *outbuf, unsigned int offset, unsigned int size);
        void returnInputBuffer(void * buffer);
        void onOmxVceError(int error);
        
    private:
        AppCallbackNotifier* mAppCallbackNotifier;
        
    };
    friend class ImageJpegObserver;

    class ImageResizeObserver : public OMXVceObserver
    {
    public:
        ImageResizeObserver(AppCallbackNotifier *nh):mAppCallbackNotifier(nh){};
        void sendOutputBuffer(void *outbuf, unsigned int offset, unsigned int size);
        void returnInputBuffer(void * buffer);
        void onOmxVceError(int error);
        
    private:
        AppCallbackNotifier* mAppCallbackNotifier;
        
    };
    friend class ImageResizeObserver;

    class CameraMemoryAllocater: public RefBase
    {
    public:
        CameraMemoryAllocater(AppCallbackNotifier *nh,int size, int count):mSize(size),mCount(count),mAppCallbackNotifier(nh) 
        {
            if(mAppCallbackNotifier->mRequestMemory!= NULL)
            {
                mMemory=mAppCallbackNotifier->mRequestMemory(-1, mSize,mCount,NULL);
            }
            else
            {
                mMemory = NULL;
            }
        }

        ~CameraMemoryAllocater()
        {
            if(mMemory)
            {
               mMemory->release(mMemory);
               mMemory = NULL;
            }
        }

        int getSize()
        {
            return mSize;
        }

        int getCount()
        {
            return mCount;
        }

        camera_memory_t *getMemory()
        {
            return mMemory;
        }

    private:
        int mSize;
        int mCount;
        camera_memory_t *mMemory;
        AppCallbackNotifier* mAppCallbackNotifier;
    };
    friend class CameraMemoryAllocater;


    bool jpegThread(JpegThread *);
    bool resizeThread(ResizeThread *);

private:
    void notifyEvent();
    void notifyFrame();
    void jpegEncode(JpegThread *);
    void resizeFrame(ResizeThread *);
    bool processNotificationMessage();
    bool processJpegMessage(JpegThread *);
    bool processResizeMessage(ResizeThread *);
    void releaseSharedVideoBuffers();
    status_t dummyRaw();
    void resizeRawFrame(CameraFrame* frame, bool canceled);
    void encodePictureFrame(CameraFrame* frame, bool canceled);

    camera_memory_t  *jpegEncodeSW(CameraFrame* frame);
    camera_memory_t *jpegEncodeHW(CameraFrame* frame);

    status_t convertEXIF_libjpeg(CameraHalExif* exif,ExifElementsTable* exifTable,CameraFrame *frame);
    

    void sendToFrameQ(int cmd, CameraFrame* frame,void *arg2, void *arg3);
    void sendToResize(CameraFrame* frame);
    void sendToJpegEncode(CameraFrame* frame);
    void sendToEventQ(CameraHalEvent* evt);

    void flushJpegQ();
    void flushResizeQ();
    void flushResizeQForFrame(CameraFrame::FrameType frametype);
    void flushResizeQForPreview();
    void flushResizeQForVideo();
    void flushResizeQForImage();


private:

    CameraHal* mCameraHal;
    camera_notify_callback mNotifyCb;
    camera_data_callback   mDataCb;
    camera_data_timestamp_callback mDataCbTimestamp;
    camera_request_memory mRequestMemory;//见AppCallbackNotifier::setCallbacks
    void *mCallbackCookie;
    void *mVceResizeHandle;

    //Keeps Video MemoryHeaps and Buffers within
    //these objects
    KeyedVector<unsigned int, unsigned int> mVideoHeaps;
    KeyedVector<unsigned int, unsigned int> mVideoBuffers;
    KeyedVector<unsigned int, unsigned int> mVideoMap;

    //Keeps list of Gralloc handles and associated Video Metadata Buffers
    KeyedVector<uint32_t, uint32_t> mVideoMetadataBufferMemoryMap;
    KeyedVector<uint32_t, uint32_t> mVideoMetadataBufferReverseMap;


    sp< NotificationThread> mNotificationThread;
    sp< JpegThread> mJpegThread;
    sp< ResizeThread> mResizeThread;
    EventProvider *mEventProvider;
    FrameProvider *mFrameProvider;
    ActUtils::MessageQueue mEventQ;
    ActUtils::MessageQueue mFrameQ;//notificationThread  notifyFrame
    ActUtils::MessageQueue mResizeQ;
    ActUtils::MessageQueue mJpegQ;//jpegThread
    NotifierState mNotifierState;

    mutable Mutex mEventQLock;
    mutable Mutex mFrameQLock;
    mutable Mutex mResizeQLock;
    mutable Mutex mJpegQLock;

    mutable Mutex mLock;
    

#ifdef CAMERA_VCE_OMX_JPEG
    ImageJpegObserver mImageJpegObserver;
    OMXVceJpegEncoder *mVceJpegEncoder;//libcamera/vce/??
#endif

#ifdef CAMERA_VCE_OMX_RESIZE
    ImageResizeObserver mImageResizeObserver;
    OMXVceImageResize *mVceImageResize;
    int mResizeType;
#endif

    
    

    unsigned char* mImageBufs[MAX_BUFFERS];
    uint32_t mImageBufCount;
    uint32_t mImageBufSize;

    bool mPreviewing;

    unsigned char* mPreviewBufs[MAX_BUFFERS];
    uint32_t mPreviewBufCount;
    uint32_t mPreviewBufSize;


    camera_memory_t* mPreviewMemory;//
    uint32_t mPreviewMemoryCount;
    uint32_t mPreviewMemoryIndex;
    unsigned char* mPreviewMemoryBufs[MAX_BUFFERS];//
    bool mPreAllocPreviewCBbufsdone;
	

    camera_memory_t* mVideoMemory;
    uint32_t mVideoMemoryCount;
    KeyedVector<uint32_t, uint32_t> mVideoMemoryMap;
    KeyedVector<uint32_t, uint32_t> mVideoMemoryReverseMap;
    unsigned char* mVideoBufs[MAX_BUFFERS];
    const char *mPreviewPixelFormat;

    //Burst mode active
    bool mBurst;
    mutable Mutex mRecordingLock;
    bool mRecording;
    bool mMeasurementEnabled;

    bool mUseMetaDataBufferMode;
    bool mRawAvailable;
    bool mUseCameraHalVideoBuffers;

    int mVideoWidth;
    int mVideoHeight;
    mutable Mutex mImageEncodeLock;

    bool mFirstRecordingFrame;
    nsecs_t mTimeStampDelta;

    mutable Semaphore mNotifierThreadSem;

    mutable Semaphore mJpegThreadSem;
    mutable Mutex mJpegThreadLock;

    mutable Semaphore mResizeThreadSem;
    mutable Mutex mResizeThreadLock;

    CameraFrame::FrameFormat mPreviewFormat;
    CameraFrame::FrameFormat mVideoFormat;
};







/**
  * CameraAdapter interface class
  * Concrete classes derive from this class and provide implementations based on the specific camera h/w interface
  */

class CameraAdapter: public FrameNotifier,public MessageNotifier, public virtual RefBase
{
public:
    enum AdapterActiveStates
    {
        INTIALIZED_ACTIVE =     1 << 0,
        LOADED_PREVIEW_ACTIVE = 1 << 1,
        PREVIEW_ACTIVE =        1 << 2,
        LOADED_CAPTURE_ACTIVE = 1 << 3,
        CAPTURE_ACTIVE =        1 << 4,
        VIDEO_ACTIVE =          1 << 5,
        AF_ACTIVE =             1 << 6,
        ZOOM_ACTIVE =           1 << 7,
    };
public:
    typedef struct
    {
        void *mBuffers;
        void *mVaddrs;
        uint32_t *mOffsets;
        int mFd;
        size_t mLength;
        size_t mCount;
        size_t mMaxQueueable;
    } BuffersDescriptor;

    enum CameraCommands
    {
        CAMERA_START_PREVIEW                        = 0,
        CAMERA_STOP_PREVIEW                         = 1,
        CAMERA_START_VIDEO                          = 2,
        CAMERA_STOP_VIDEO                           = 3,
        CAMERA_START_IMAGE_CAPTURE                  = 4,
        CAMERA_STOP_IMAGE_CAPTURE                   = 5,
        CAMERA_PERFORM_AUTOFOCUS                    = 6,
        CAMERA_CANCEL_AUTOFOCUS                     = 7,
        CAMERA_PREVIEW_FLUSH_BUFFERS                = 8,
        CAMERA_START_SMOOTH_ZOOM                    = 9,
        CAMERA_STOP_SMOOTH_ZOOM                     = 10,
        CAMERA_USE_BUFFERS_PREVIEW                  = 11,
        CAMERA_SET_TIMEOUT                          = 12,
        CAMERA_CANCEL_TIMEOUT                       = 13,
        CAMERA_QUERY_RESOLUTION_PREVIEW             = 16,
        CAMERA_QUERY_BUFFER_SIZE_IMAGE_CAPTURE      = 17,
        CAMERA_USE_BUFFERS_IMAGE_CAPTURE            = 19,
        CAMERA_TIMEOUT_EXPIRED                      = 21,
        CAMERA_START_FD                             = 22,
        CAMERA_STOP_FD                              = 23,
        CAMERA_SWITCH_TO_EXECUTING                  = 24,
    };

    enum CameraMode
    {
        CAMERA_PREVIEW,
        CAMERA_IMAGE_CAPTURE,
        CAMERA_VIDEO
    };

    enum AdapterState
    {
        INTIALIZED_STATE           = INTIALIZED_ACTIVE,
        LOADED_PREVIEW_STATE       = LOADED_PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        PREVIEW_STATE              = PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        LOADED_CAPTURE_STATE       = LOADED_CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        CAPTURE_STATE              = CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_STATE                = VIDEO_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_LOADED_CAPTURE_STATE = VIDEO_ACTIVE | LOADED_CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
        VIDEO_CAPTURE_STATE        = VIDEO_ACTIVE | CAPTURE_ACTIVE | PREVIEW_ACTIVE | INTIALIZED_ACTIVE,
    };

public:

    ///Initialzes the camera adapter creates any resources required
    virtual int initialize(CameraProperties::Properties*) = 0;

    virtual int setErrorHandler(ErrorNotifier *errorNotifier) = 0;

    //Message/Frame notification APIs
    virtual void enableMsgType(int32_t msgs,
                               event_callback eventCb = NULL,
                               void *cookie = NULL) = 0;
    virtual void disableMsgType(int32_t msgs, void* cookie) = 0;
    virtual void enableFrameMsgType(int32_t msgs,
                               frame_callback callback = NULL,
                               void *cookie = NULL) = 0;
    virtual void disableFrameMsgType(int32_t msgs, void* cookie) = 0;
    virtual void returnFrame(void* frameBuf, CameraFrame::FrameType frameType, CameraFrame::StreamType streamType) = 0;

    //APIs to configure Camera adapter and get the current parameter set
    virtual int setParameters(const CameraParameters& params) = 0;
    virtual void getParameters(CameraParameters& params) = 0;

    //API to flush the buffers from Camera
    status_t flushBuffers()
    {
        return sendCommand(CameraAdapter::CAMERA_PREVIEW_FLUSH_BUFFERS);
    }

    //Registers callback for returning image buffers back to CameraHAL
    virtual int registerImageReleaseCallback(release_image_buffers_callback callback, void *user_data) = 0;

    //Registers callback, which signals a completed image capture
    virtual int registerEndCaptureCallback(end_image_capture_callback callback, void *user_data) = 0;

    //API to send a command to the camera
    //ActionsCode(author:liuyiguang, change_code)
    //virtual status_t sendCommand(CameraCommands operation, int value1=0, int value2=0, int value3=0) = 0;
    virtual status_t sendCommand(CameraCommands operation, long value1=0, int value2=0, int value3=0) = 0;

    virtual ~CameraAdapter() {};

    //Retrieves the current Adapter state
    virtual AdapterState getState() = 0;

    //Retrieves the next Adapter state
    virtual AdapterState getNextState() = 0;

    // Receive orientation events from CameraHal
    virtual void onOrientationEvent(uint32_t orientation, uint32_t tilt) = 0;

    // Rolls the state machine back to INTIALIZED_STATE from the current state
    virtual status_t rollbackToInitializedState() = 0;
    virtual status_t rollbackToPreviewState() = 0;

    // Retrieves the current Adapter state - for internal use (not locked)
    virtual status_t getState(AdapterState &state) = 0;
    // Retrieves the next Adapter state - for internal use (not locked)
    virtual status_t getNextState(AdapterState &state) = 0;

protected:
    //The first two methods will try to switch the adapter state.
    //Every call to setState() should be followed by a corresponding
    //call to commitState(). If the state switch fails, then it will
    //get reset to the previous state via rollbackState().
    virtual status_t setState(CameraCommands operation) = 0;
    virtual status_t commitState() = 0;
    virtual status_t rollbackState() = 0;
};

class DisplayAdapter : public BufferProvider, public virtual RefBase
{
public:
    typedef struct S3DParameters_t
    {
        int mode;
        int framePacking;
        int order;
        int subSampling;
    } S3DParameters;

    ///Initializes the display adapter creates any resources required
    virtual int initialize() = 0;

    virtual int setPreviewWindow(struct preview_stream_ops *window) = 0;
    virtual int setFrameProvider(FrameNotifier *frameProvider) = 0;
    virtual int setErrorHandler(ErrorNotifier *errorNotifier) = 0;
    virtual int enableDisplay(int width, int height, struct timeval *refTime = NULL, S3DParameters *s3dParams = NULL) = 0;
    virtual int disableDisplay(bool cancel_buffer = true) = 0;
    //Used for Snapshot review temp. pause
    virtual int pauseDisplay(bool pause) = 0;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS
    //Used for shot to snapshot measurement
    virtual int setSnapshotTimeRef(struct timeval *refTime = NULL) = 0;
#endif

    virtual int useBuffers(void *bufArr, int num) = 0;
    virtual bool supportsExternalBuffering() = 0;

    // Get max queueable buffers display supports
    // This function should only be called after
    // allocateBuffer
    virtual int maxQueueableBuffers(unsigned int& queueable) = 0;
};

static void releaseImageBuffers(void *userData);

static void endImageCapture(void *userData);

/**
   Implementation of the Android Camera hardware abstraction layer

   This class implements the interface methods defined in CameraHardwareInterface
   for the OMAP4 platform

*/
class CameraHal

{

public:
    ///Constants
    static const int NO_BUFFERS_PREVIEW;
    static const int NO_BUFFERS_IMAGE_CAPTURE;
    static const uint32_t VFR_SCALE = 1000;


    /*--------------------Interface Methods---------------------------------*/

    //@{
public:

    /** Set the notification and data callbacks */
    void setCallbacks(camera_notify_callback notify_cb,
                      camera_data_callback data_cb,
                      camera_data_timestamp_callback data_cb_timestamp,
                      camera_request_memory get_memory,
                      void *user);

    /** Receives orientation events from SensorListener **/
    void onOrientationEvent(uint32_t orientation, uint32_t tilt);

    /**
     * The following three functions all take a msgtype,
     * which is a bitmask of the messages defined in
     * include/ui/Camera.h
     */

    /**
     * Enable a message, or set of messages.
     */
    void        enableMsgType(int32_t msgType);

    /**
     * Disable a message, or a set of messages.
     */
    void        disableMsgType(int32_t msgType);

    /**
     * Query whether a message, or a set of messages, is enabled.
     * Note that this is operates as an AND, if any of the messages
     * queried are off, this will return false.
     */
    int        msgTypeEnabled(int32_t msgType);
    int        msgTypeEnabledNoLock(int32_t msgType);

    /**
     * Start preview mode.
     */
    int    startPreview();

    /**
     * Only used if overlays are used for camera preview.
     */
    int setPreviewWindow(struct preview_stream_ops *window);

    /**
     * Stop a previously started preview.
     */
    void        stopPreview();

    /**
     * Returns true if preview is enabled.
     */
    bool        previewEnabledInner();

    bool        previewEnabled();

    /**
     * Start record mode. When a record image is available a CAMERA_MSG_VIDEO_FRAME
     * message is sent with the corresponding frame. Every record frame must be released
     * by calling releaseRecordingFrame().
     */
    int    startRecording();

    /**
     * Stop a previously started recording.
     */
    void        stopRecording();

    /**
     * Returns true if recording is enabled.
     */
    int        recordingEnabled();

    /**
     * Release a record frame previously returned by CAMERA_MSG_VIDEO_FRAME.
     */
    void        releaseRecordingFrame(const void *opaque);

    /**
     * Start auto focus, the notification callback routine is called
     * with CAMERA_MSG_FOCUS once when focusing is complete. autoFocus()
     * will be called again if another auto focus is needed.
     */
    int    autoFocus();

    /**
     * Cancels auto-focus function. If the auto-focus is still in progress,
     * this function will cancel it. Whether the auto-focus is in progress
     * or not, this function will return the focus position to the default.
     * If the camera does not support auto-focus, this is a no-op.
     */
    int    cancelAutoFocus();

    /**
     * Take a picture.
     */
    int    takePicture();

    /**
     * Cancel a picture that was started with takePicture.  Calling this
     * method when no picture is being taken is a no-op.
     */
    int    cancelPicture();

    /** Set the camera parameters. */
    int    setParameters(const char* params);
    int    setParameters(const CameraParameters& params);

    /** Return the camera parameters. */
    char*  getParameters();
    void putParameters(char *);

    /**
     * Send command to camera driver.
     */
    int sendCommand(int32_t cmd, int32_t arg1, int32_t arg2);

    /**
     * Release the hardware resources owned by this object.  Note that this is
     * *not* done in the destructor.
     */
    void release();

    /**
     * Dump state of the camera hardware
     */
    int dump(int fd) const;


    status_t storeMetaDataInBuffers(bool enable);

    //@}

    /*--------------------Internal Member functions - Public---------------------------------*/

public:
    /** @name internalFunctionsPublic */
    //@{

    /** Constructor of CameraHal */
    CameraHal(int cameraId);

    // Destructor of CameraHal
    ~CameraHal();

    /** Initialize CameraHal */
    status_t initialize(CameraProperties::Properties*);

    /** Deinitialize CameraHal */
    void deinitialize();

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    //Uses the constructor timestamp as a reference to calcluate the
    // elapsed time
    static void PPM(const char *);
    //Uses a user provided timestamp as a reference to calcluate the
    // elapsed time
    static void PPM(const char *, struct timeval*, ...);

#endif

    /** Free image bufs */
    status_t freeImageBufs();

    //Signals the end of image capture
    status_t signalEndImageCapture();

    //Events
    static void eventCallbackRelay(CameraHalEvent* event);
    void eventCallback(CameraHalEvent* event);
    void setEventProvider(int32_t eventMask, MessageNotifier * eventProvider);

    /*--------------------Internal Member functions - Private---------------------------------*/
private:

    /** @name internalFunctionsPrivate */
    //@{

    /**  Set the camera parameters specific to Video Recording. */
    bool        setVideoModeParameters(const CameraParameters&);

    /** Reset the camera parameters specific to Video Recording. */
    bool       resetVideoModeParameters();

    /** Restart the preview with setParameter. */
    status_t        restartPreview();

    status_t parseResolution(const char *resStr, int &width, int &height);

    void insertSupportedParams();

    /** Allocate preview buffers */
    status_t allocPreviewBufs(int width, int height, const char* previewFormat, unsigned int bufferCount, unsigned int &max_queueable);

    /** Allocate video buffers */
    status_t allocVideoBufs(uint32_t width, uint32_t height, const char *format, uint32_t bufferCount);
    /** Allocate image capture buffers */
    status_t allocImageBufs(unsigned int width, unsigned int height, size_t length, const char* format, unsigned int bufferCount);

    /** Free preview buffers */
    status_t freePreviewBufs();

    /** Free video bufs */
    status_t freeVideoBufs(void *bufs);

    //Check if a given resolution is supported by the current camera
    //instance
    bool isResolutionValid(unsigned int width, unsigned int height, const char *supportedResolutions);

    //Check if a given parameter is supported by the current camera
    // instance
    bool isParameterValid(const char *param, const char *supportedParams);
    bool isParameterValid(int param, const char *supportedParams);
    status_t doesSetParameterNeedUpdate(const char *new_param, const char *old_params, bool &update);

    /** Initialize default parameters */
    void initDefaultParameters();

    void dumpProperties(CameraProperties::Properties& cameraProps);

    void setShutter(bool enable);

    void forceStopPreview();

    void selectFPSRange(int framerate, int *min_fps, int *max_fps);

    void setPreferredPreviewRes(int width, int height);
    void resetPreviewRes(CameraParameters *mParams, int width, int height);
    int setPreviewFormatParameter(const char *f, int w);

    //@}


    /*----------Member variables - Public ---------------------*/
public:
    int32_t mMsgEnabled;
    bool mRecordEnabled;
    nsecs_t mCurrentTime;
    bool mFalsePreview;
    bool mPreviewEnabled;
    uint32_t mTakePictureQueue;
    bool mBracketingEnabled;
    bool mBracketingRunning;
    //User shutter override
    bool mShutterEnabled;
    bool mMeasurementEnabled;
    //Google's parameter delimiter
    static const char PARAMS_DELIMITER[];

    CameraAdapter *mCameraAdapter;
    sp<AppCallbackNotifier> mAppCallbackNotifier;
    sp<DisplayAdapter> mDisplayAdapter;
    sp<MemoryManager> mMemoryManager;

    sp<IMemoryHeap> mPictureHeap;

    int* mGrallocHandles;
    bool mFpsRangeChangedByApp;

    bool mVceResize;




///static member vars

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    //Timestamp from the CameraHal constructor
    static struct timeval ppm_start;
    //Timestamp of the autoFocus command
    static struct timeval mStartFocus;
    //Timestamp of the startPreview command
    static struct timeval mStartPreview;
    //Timestamp of the takePicture command
    static struct timeval mStartCapture;

#endif

    /*----------Member variables - Private ---------------------*/
private:
    bool mDynamicPreviewSwitch;
    //keeps paused state of display
    bool mDisplayPaused;
    //Index of current camera adapter
    int mCameraIndex;

    mutable Mutex mLock;

    sp<SensorListener> mSensorListener;

    void* mCameraAdapterHandle;

    CameraParameters mParameters;
    bool mPreviewRunning;
    bool mPreviewStateOld;
    bool mRecordingEnabled;
    EventProvider *mEventProvider;

    int32_t *mPreviewDataBufs;
    uint32_t *mPreviewDataOffsets;
    int mPreviewDataFd;
    int mPreviewDataLength;
    int32_t *mImageBufs;
    uint32_t mImageBufCount;
    uint32_t *mImageOffsets;
    int mImageFd;
    int mImageLength;
    int32_t *mPreviewBufs;//预览buf管理结构数组的指针
    uint32_t *mPreviewOffsets;//
    void *mPreviewVaddrs;//
    int mPreviewLength;
    int mPreviewFd;
    int32_t *mVideoBufs;
    uint32_t *mVideoOffsets;
    int mVideoFd;
    int mVideoLength;

    int mBracketRangePositive;
    int mBracketRangeNegative;

    ///@todo Rename this as preview buffer provider
    BufferProvider *mBufProvider;//mDisplayAdapter
    BufferProvider *mVideoBufProvider;


    CameraProperties::Properties* mCameraProperties;

    bool mPreviewStartInProgress;

    bool mSetPreviewWindowCalled;

    uint32_t mPreviewWidth;
    uint32_t mPreviewHeight;
    int32_t mMaxZoomSupported;

    int mVideoWidth;
    int mVideoHeight;
#ifdef CAMERA_IGNORE_STOPPREVIEW_AFTER_CAPTURE
    bool mAfterTakePicture;
#endif

    mutable Condition mTakePictureWait;

    //for cts, after takepicture, should set mPreviewCBEnabled=false
    //avoid testPreviewPictureSizesCombination to fail
    bool mPreviewCBEnabled;
    
    const char *mPreviewFormat;    
    const char *mMappedPreviewFormat;
    bool mNeedfmtrefresh;    
    const char *mVideoFormat;


    mutable Mutex mParametersLock;

    bool mUseVideoBuffers;

};


}; // namespace android

#endif
