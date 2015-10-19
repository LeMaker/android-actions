/**
  @file src/components/camera/omx_camera_source_component.c

  OpenMAX camera source component.
  The OpenMAX camera component is a V4L2-based video source whose functionalities
  include preview, video capture, image capture, video thumbnail and image
  thumbnail. It has 3 (output) ports: Port 0 is used for preview; Port 1 is used
  for video and image capture; Port 2 is used for video and image thumbnail.

  Copyright (C) 2007-2008  Motorola and STMicroelectronics

  This code is licensed under LGPL see README for full LGPL notice.

  Date                             Author                Comment
  Mon, 09 Jul 2007                 Motorola              File created
  Tue, 06 Apr 2008                 STM                   Modified to support Video for Linux Two(V4L2)

  This Program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  $Date$
  Revision $Rev$
  Author $Author$

*/

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#include <OMX_Core.h>
#include <omx_comp_debug_levels.h>

#include "omx_camera_source_component.h"
#include "buffer_handle_mapper.h"
#include "omx_camera_source_capabilities.h"
#include "video_mediadata.h"



#include <ion/ion.h>


#define ION_ALLOC_FLAG (1<<ASOC_ION_HEAP_CARVEOUT)

#define DEFAULT_FRAME_RATE 15

#define DEFAULT_FRAME_WIDTH 320
#define DEFAULT_FRAME_HEIGHT 240

#define DEFAULT_COLOR_FORMAT OMX_COLOR_FormatYUV420SemiPlanar

#define GET_SYSERR_STRING() strerror(errno)

/* Thumbnail (snapshot) index from video captured frame */
#define OMX_CAM_VC_SNAPSHOT_INDEX    5

#define CLEAR(x) memset (&(x), 0, sizeof (x))

/* V4L2 Mapping Queue Interface */
#define OMX_MAPBUFQUEUE_ISEMPTY(_queue_) (0 == (_queue_).nBufCountTotal)
#define OMX_MAPBUFQUEUE_ISFULL(_queue_) ((_queue_).nBufCountTotal > 0 && (_queue_).nNextCaptureIndex == (_queue_).nLastBufIndex)
#define OMX_MAPBUFQUEUE_NOBUFCAPTURED(_queue_) (0 == (_queue_).nBufCountCaptured)
#define OMX_MAPBUFQUEUE_HASBUFCAPTURED(_queue_) (!(OMX_MAPBUFQUEUE_NOBUFCAPTURED(_queue_)))
#define OMX_MAPBUFQUEUE_NOBUFWAITTOCAPTURE(_queue_) (OMX_MAPBUFQUEUE_HASBUFCAPTURED(_queue_) && ((_queue_).nNextWaitIndex == (_queue_).nNextCaptureIndex))
#define OMX_MAPBUFQUEUE_HASBUFWAITTOCAPTURE(_queue_) ((!(OMX_MAPBUFQUEUE_NOBUFWAITTOCAPTURE(_queue_)))&&(!(OMX_MAPBUFQUEUE_ISEMPTY(_queue_))))

#define OMX_MAPBUFQUEUE_GETMAXLEN(_queue_) ((_queue_).nFrame)

#define OMX_MAPBUFQUEUE_GETNEXTCAPTURE(_queue_) ((_queue_).nNextCaptureIndex)
#define OMX_MAPBUFQUEUE_GETNEXTWAIT(_queue_) ((_queue_).nNextWaitIndex)
#define OMX_MAPBUFQUEUE_GETLASTBUFFER(_queue_) ((_queue_).nLastBufIndex)
#define OMX_MAPBUFQUEUE_GETNEXTINDEX(_queue_, _curindex_) ((_curindex_ + 1) % (_queue_).nFrame)

#define OMX_MAPBUFQUEUE_GETBUFCOUNTCAPTURED(_queue_) ((_queue_).nBufCountCaptured)

#define OMX_MAPBUFQUEUE_GETBUFADDR(_queue_, _bufindex_) ((OMX_PTR) (_queue_).bufferQueue[(_bufindex_)])
#define OMX_MAPBUFQUEUE_GETBUF(_queue_, _bufindex_) ((OMX_PTR) &(_queue_).buffers[(_bufindex_)])
#define OMX_MAPBUFQUEUE_GETBUFVADDR(_queue_, _bufindex_) ((OMX_PTR) (_queue_).buffers[(_bufindex_)].pCapAddrStart)

#define OMX_MAPBUFQUEUE_MAKEEMPTY(_queue_) do \
{ \
    (_queue_).nNextCaptureIndex = 0; \
    (_queue_).nNextWaitIndex = 0; \
    (_queue_).nLastBufIndex = 0; \
    (_queue_).nBufCountTotal = 0; \
    (_queue_).nBufCountCaptured = 0; \
} while (0)
#define OMX_MAPBUFQUEUE_ENQUEUE(_queue_, _handle_) do \
{ \
    (_queue_).bufferQueue[(_queue_).nNextCaptureIndex]=(OMX_U32*)_handle_; \
    (_queue_).nNextCaptureIndex = ((_queue_).nNextCaptureIndex + 1) % (_queue_).nFrame; \
    (_queue_).nBufCountTotal ++; \
} while (0)
#define OMX_MAPBUFQUEUE_DEQUEUE(_queue_) do \
{ \
    (_queue_).nLastBufIndex = ((_queue_).nLastBufIndex + 1) % (_queue_).nFrame; \
    (_queue_).nBufCountTotal --; \
    (_queue_).nBufCountCaptured --; \
} while (0)


#define OMX_MAPBUFQUEUE_ADDCAPTUREDBUF(_queue_) do \
{ \
    (_queue_).nNextWaitIndex = ((_queue_).nNextWaitIndex + 1) % (_queue_).nFrame; \
    (_queue_).nBufCountCaptured ++; \
} while (0)

#define OMX_MAPBUFQUEUE_GETTIMESTAMP(_queue_, _bufindex_) ((_queue_).qTimeStampQueue[(_bufindex_)])
#define OMX_MAPBUFQUEUE_SETTIMESTAMP(_queue_, _bufindex_, _timestamp_) do \
{ \
    (_queue_).qTimeStampQueue[(_bufindex_)] = (OMX_TICKS)(_timestamp_); \
} while (0)

#define OMX_MAPBUFQUEUE_PORTINDEX(_queue_) ((_queue_).nPortIndex)


#define OMX_MAPBUFQUEUE_GETBUF_BYHEADER(_queue_,_handle_,_buf_,_index_)    \
do{ \
    unsigned int k = 0;\
    _buf_=NULL;\
    for(k = 0; k<(_queue_).nFrame;k++)\
    {\
        if((_queue_).buffers[k].handle == _handle_) \
        {\
            _buf_=&((_queue_).buffers[k]);\
            _index_ = k;\
            break;\
        }\
    }\
} while (0)


#define OMX_MAPBUFQUEUE_GETBUF_BYVADDR(_queue_,_vaddr_,_buf_,_index_)    \
do{ \
    int k = 0;\
    _buf_=NULL;\
    for(k = 0; k<(_queue_).nFrame;k++)\
    {\
        if((_queue_).buffers[k].pCapAddrStart == _vaddr_) \
        {\
            _buf_=&((_queue_).buffers[k]);\
            _index_ = k;\
            break;\
        }\
    }\
} while (0)

#define OMX_MAPBUFQUEUE_GETBUF_BYPADDR(_queue_,_paddr_,_buf_,_index_)    \
do{ \
    int k = 0;\
    _buf_=NULL;\
    for(k = 0; k<(_queue_).nFrame;k++)\
    {\
        if((_queue_).buffers[k].phys_addr == _paddr_) \
        {\
            _buf_=&((_queue_).buffers[k]);\
            _index_ = k;\
            break;\
        }\
    }\
} while (0)


#define PORTTYPE_IS_VIDEO(nPortIndex)  \
    ((nPortIndex >= omx_camera_source_component_Private->sPortTypesParam [OMX_PortDomainVideo].nStartPortNumber)                      \
    && (nPortIndex < (omx_camera_source_component_Private->sPortTypesParam [OMX_PortDomainVideo].nPorts \
    +omx_camera_source_component_Private->sPortTypesParam [OMX_PortDomainVideo].nStartPortNumber)))


#define PORTTYPE_IS_IMAGE(nPortIndex)  \
    ((nPortIndex >= omx_camera_source_component_Private->sPortTypesParam [OMX_PortDomainImage].nStartPortNumber)                      \
    && (nPortIndex < (omx_camera_source_component_Private->sPortTypesParam [OMX_PortDomainImage].nPorts \
            +omx_camera_source_component_Private->sPortTypesParam [OMX_PortDomainImage].nStartPortNumber))) 

typedef struct CAM_SENSOR_OMXV4LCOLORTYPE
{
    OMX_COLOR_FORMATTYPE eOmxColorFormat;
    V4L2_COLOR_FORMATTYPE sV4lColorFormat;
} CAM_SENSOR_OMXV4LCOLORTYPE;

typedef struct CAM_CAPTURE_FRAMESIZETYPE
{
    OMX_U32 nWidth;
    OMX_U32 nHeight;
} CAM_CAPTURE_FRAMESIZETYPE;

static const CAM_SENSOR_OMXV4LCOLORTYPE g_SupportedColorTable[] =
{
    { OMX_COLOR_FormatL8,
        { V4L2_PIX_FMT_GREY, 8 } },
    { OMX_COLOR_Format16bitRGB565,
        { V4L2_PIX_FMT_RGB565, 16 } },
    { OMX_COLOR_Format24bitRGB888,
        { V4L2_PIX_FMT_RGB24, 24 } },
    { OMX_COLOR_FormatYCbYCr,
        { V4L2_PIX_FMT_YUYV, 16 } },
    { OMX_COLOR_FormatYUV420Planar,
        { V4L2_PIX_FMT_YUV420, 12 } } ,
    { OMX_COLOR_FormatYUV420SemiPlanar,
        { V4L2_PIX_FMT_NV12, 12 } }
};

/* Table for supported capture framsizes (based on the WebEye V2000 camera) */
static const CAM_CAPTURE_FRAMESIZETYPE g_SupportedFramesizeTable[] =
{
    { 64, 48 },
    { 72, 64 },
    { 88, 72 },
    { 96, 128 },
    { 128, 96 },
    { 144, 176 },
    { 160, 120 },
    { 176, 144 },
    { 200, 80 },
    { 224, 96 },
    { 256, 144 },
    { 320, 240 },
    { 352, 288 },
    { 432, 256 },
    { 512, 376 },
    { 568, 400 },
    { 640, 480 },
    { 800, 600 },
    { 1280, 720}
};

static int camera_init_userptr(
    omx_camera_source_component_PrivateType * omx_camera_source_component_Private, OMX_U32 portIndex, OMX_U32 bufCount);

static int xioctl(int fd, int request, void *arg)
{
    int r;

    do
        r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);

    //DEBUG (DEB_LEV_SIMPLE_SEQ, "v4l2 command = 0x%x result = %d!!!!\n", request,r);
    return r;
}

static OMX_ERRORTYPE omx_camera_source_component_GetParameter (OMX_IN
    OMX_HANDLETYPE
    hComponent,
    OMX_IN
    OMX_INDEXTYPE
    nParamIndex,
    OMX_INOUT
    OMX_PTR
    ComponentParameterStructure);

static OMX_ERRORTYPE omx_camera_source_component_SetParameter (OMX_IN
    OMX_HANDLETYPE
    hComponent,
    OMX_IN
    OMX_INDEXTYPE
    nParamIndex,
    OMX_IN OMX_PTR
    ComponentParameterStructure);

static OMX_ERRORTYPE omx_camera_source_component_GetConfig (OMX_IN
    OMX_HANDLETYPE
    hComponent,
    OMX_IN
    OMX_INDEXTYPE
    nConfigIndex,
    OMX_INOUT OMX_PTR
    pComponentConfigStructure);

static OMX_ERRORTYPE omx_camera_source_component_SetConfig (OMX_IN
    OMX_HANDLETYPE
    hComponent,
    OMX_IN
    OMX_INDEXTYPE
    nConfigIndex,
    OMX_IN OMX_PTR
    pComponentConfigStructure);

/** This is the central function for buffer processing.
 * It is executed in a separate thread.
 * @param param input parameter, a pointer to the OMX standard structure
 */
static void *omx_camera_source_component_BufferMgmtFunction(void *param);

/** Specific operations for camera component when state changes
 * @param openmaxStandComp the openmax component which state is to be changed
 * @param destinationState the requested target state
 */
static OMX_ERRORTYPE omx_camera_source_component_DoStateSet(
    OMX_COMPONENTTYPE * openmaxStandComp, OMX_U32 destinationState);

static OMX_ERRORTYPE camera_CheckSupportedColorFormat (OMX_IN
    OMX_COLOR_FORMATTYPE
    eColorFormat);
static OMX_ERRORTYPE camera_CheckSupportedFramesize (OMX_IN OMX_U32
    nFrameWidth,
    OMX_IN OMX_U32
    nFrameHeight);
static OMX_ERRORTYPE camera_MapColorFormatOmxToV4l (OMX_IN
    OMX_COLOR_FORMATTYPE
    eOmxColorFormat,
    OMX_INOUT
    V4L2_COLOR_FORMATTYPE *
    pV4lColorFormat);

static OMX_ERRORTYPE camera_MapColorFormatV4lToOmx (OMX_IN
    V4L2_COLOR_FORMATTYPE *
    pV4lColorFormat,
    OMX_INOUT
    OMX_COLOR_FORMATTYPE *
    pOmxColorFormat);

static OMX_U32 camera_CalculateBufferSize (OMX_IN OMX_U32 nWidth,
    OMX_IN OMX_U32 nHeight,
    OMX_IN OMX_COLOR_FORMATTYPE
    eOmxColorFormat);

static OMX_ERRORTYPE camera_SetConfigCapturing (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private,
    OMX_IN OMX_BOOL enable);

static OMX_ERRORTYPE camera_OpenCameraDevice (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private);
static OMX_ERRORTYPE camera_CloseCameraDevice (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private);
static OMX_ERRORTYPE camera_InitCameraDevice (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private, OMX_U32 portIndex);
static OMX_ERRORTYPE camera_InitBufQueue (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private, OMX_U32 portIndex);
static OMX_ERRORTYPE camera_FreeBufQueue (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private);
static OMX_ERRORTYPE camera_StartCameraDevice (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private, OMX_U32 portIndex);
static OMX_ERRORTYPE camera_StopCameraDevice (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private, 
    OMX_BOOL  restore);
static OMX_ERRORTYPE camera_HandleThreadBufferCapture (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private);
static OMX_ERRORTYPE camera_GenerateTimeStamp (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private);
static OMX_ERRORTYPE camera_SendCapturedBuffers (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private);

static OMX_ERRORTYPE camera_UpdateCapturedBufferQueue (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private);
static OMX_ERRORTYPE camera_ProcessPortOneBuffer (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private,
    OMX_IN OMX_U32 nPortIndex, int bufIndex);


static OMX_ERRORTYPE camera_AddTimeStamp (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private,
    OMX_IN OMX_BUFFERHEADERTYPE *
    pBufHeader, int bufIndex);
static OMX_ERRORTYPE camera_HandleStillImageCapture (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private);

/* Check whether eColorFormat is supported */
static OMX_ERRORTYPE
camera_CheckSupportedColorFormat (OMX_IN OMX_COLOR_FORMATTYPE eColorFormat)
{
    OMX_U32 i;

    for (i = 0;
        i < sizeof (g_SupportedColorTable) / sizeof (g_SupportedColorTable[0]);
        i++)
    {
        if (eColorFormat == g_SupportedColorTable[i].eOmxColorFormat)
        {
            return OMX_ErrorNone;
        }
    }

    /* Not found supported color format */
    return OMX_ErrorUnsupportedSetting;
}

/* Check whether the frame size (nFrameWidth, nFrameHeight) is supported */
static OMX_ERRORTYPE
camera_CheckSupportedFramesize (OMX_IN OMX_U32 nFrameWidth,
    OMX_IN OMX_U32 nFrameHeight)
{
    OMX_U32 i;

    for (i = 0;
        i <
        sizeof (g_SupportedFramesizeTable) /
        sizeof (g_SupportedFramesizeTable[0]); i++)
    {
        if (nFrameWidth == g_SupportedFramesizeTable[i].nWidth &&
            nFrameHeight == g_SupportedFramesizeTable[i].nHeight)
        {
            return OMX_ErrorNone;
        }
    }

    /* Not found supported frame size */
    return OMX_ErrorUnsupportedSetting;
}

/* Map OMX color format to V4L2 color format */
static OMX_ERRORTYPE
camera_MapColorFormatOmxToV4l (OMX_IN OMX_COLOR_FORMATTYPE eOmxColorFormat,
    OMX_INOUT V4L2_COLOR_FORMATTYPE *
    pV4lColorFormat)
{
    OMX_U32 i;

    for (i = 0;
        i < sizeof (g_SupportedColorTable) / sizeof (g_SupportedColorTable[0]);
        i++)
    {
        if (eOmxColorFormat == g_SupportedColorTable[i].eOmxColorFormat)
        {
            (*pV4lColorFormat) = g_SupportedColorTable[i].sV4lColorFormat;
            return OMX_ErrorNone;
        }
    }

    /* Not found supported color format */
    return OMX_ErrorUnsupportedSetting;
}

/* Map V4L2 color format to OMX color format */
static OMX_ERRORTYPE
camera_MapColorFormatV4lToOmx (OMX_IN V4L2_COLOR_FORMATTYPE * pV4lColorFormat,
    OMX_INOUT OMX_COLOR_FORMATTYPE *
    pOmxColorFormat)
{
    OMX_U32 i;

    for (i = 0;
        i < sizeof (g_SupportedColorTable) / sizeof (g_SupportedColorTable[0]);
        i++)
    {
        if (pV4lColorFormat->v4l2Pixfmt ==
            g_SupportedColorTable[i].sV4lColorFormat.v4l2Pixfmt
            && pV4lColorFormat->v4l2Depth ==
            g_SupportedColorTable[i].sV4lColorFormat.v4l2Depth)
        {
            (*pOmxColorFormat) = g_SupportedColorTable[i].eOmxColorFormat;
            return OMX_ErrorNone;
        }
    }

    /* Not found supported color format */
    return OMX_ErrorUnsupportedSetting;
}

/* Calculate buffer size according to (width,height,color format) */
static OMX_U32
camera_CalculateBufferSize (OMX_IN OMX_U32 nWidth,
    OMX_IN OMX_U32 nHeight,
    OMX_IN OMX_COLOR_FORMATTYPE eOmxColorFormat)
{
    OMX_U32 i;

    for (i = 0;
        i < sizeof (g_SupportedColorTable) / sizeof (g_SupportedColorTable[0]);
        i++)
    {
        if (eOmxColorFormat == g_SupportedColorTable[i].eOmxColorFormat)
        {
            return (nWidth * nHeight *
                g_SupportedColorTable[i].sV4lColorFormat.v4l2Depth + 7) / 8;
        }
    }

    /* Not found supported color format, return 0 */
    return 0;
}

/* Set capturing configuration in OMX_SetConfig */
static OMX_ERRORTYPE
camera_SetConfigCapturing (OMX_IN omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private,
    OMX_IN OMX_BOOL enable)
{
    omx_base_PortType *pCapturePort;
    struct timeval now;
    OMX_ERRORTYPE err = OMX_ErrorNone;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    pthread_mutex_lock (&omx_camera_source_component_Private->setconfig_mutex);
    if (enable !=
        omx_camera_source_component_Private->bCapturingNext)
    {
        omx_camera_source_component_Private->bCapturingNext =
            enable;
#if 1
        pCapturePort =
            (omx_base_PortType *)
            omx_camera_source_component_Private->ports[OMX_CAMPORT_INDEX_CP];

        if (PORT_IS_ENABLED (pCapturePort) && enable == OMX_FALSE
            && omx_camera_source_component_Private->bAutoPause == OMX_TRUE)
        {
            /* In autopause mode, command camera component to pause state */
            if ((err =
                    omx_camera_source_component_DoStateSet
                    (omx_camera_source_component_Private->openmaxStandComp,
                     (OMX_U32) OMX_StatePause)) != OMX_ErrorNone)
            {
                goto EXIT;
            }
        }
#endif
    }

EXIT:
    pthread_mutex_unlock
        (&omx_camera_source_component_Private->setconfig_mutex);
    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}
/* Initialize the camera device */
static OMX_ERRORTYPE
camera_OpenCameraDevice (OMX_IN omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    /* Open camera device file */
    char devName[PATH_MAX];
    char dev_minor = 0;
    memset(devName,0,PATH_MAX);
    switch(omx_camera_source_component_Private->sSensorType.eSensor)
    {
        case 0:
            dev_minor = V4L2_CAMERA_DEV_MINOR_0;
            break;
        case 1:
            dev_minor = V4L2_CAMERA_DEV_MINOR_1;
            break;
        case 2:
            dev_minor = V4L2_CAMERA_DEV_MINOR_2;
            break;
        default:
            dev_minor = V4L2_CAMERA_DEV_MINOR_0;
            break;
    }
    snprintf(devName,PATH_MAX,"%s%d",V4L2DEV_FILENAME,dev_minor);
    omx_camera_source_component_Private->fdCam =
        open (devName, O_RDWR | O_NONBLOCK, 0);
    DEBUG (DEB_LEV_ERR, "open device,name=%s,   !!!\n", devName);

    if (omx_camera_source_component_Private->fdCam < 0)
    {
        DEBUG (DEB_LEV_ERR, "%s: <ERROR> -- Open camera failed: %s\n", __func__,
            GET_SYSERR_STRING ());
        err = OMX_ErrorHardware;
        goto ERR_HANDLE;
    }

    /* Query camera capability */
    if (-1 ==
        xioctl (omx_camera_source_component_Private->fdCam, VIDIOC_QUERYCAP,
            &omx_camera_source_component_Private->cap))
    {
        if (EINVAL == errno)
        {
            DEBUG (DEB_LEV_ERR, "%s is no V4L2 device\n", V4L2DEV_FILENAME);
            err = OMX_ErrorHardware;
            goto ERR_HANDLE;
        }
        else
        {
            DEBUG (DEB_LEV_ERR, "%s error %d, %s\n", "VIDIOC_QUERYCAP", errno,
                strerror (errno));
            err = OMX_ErrorHardware;
            goto ERR_HANDLE;
        }
    }

    if (!
        (omx_camera_source_component_Private->cap.
         capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        DEBUG (DEB_LEV_ERR, "%s is no video capture device\n",
            V4L2DEV_FILENAME);
        return OMX_ErrorHardware;
    }

    if (!
        (omx_camera_source_component_Private->cap.
         capabilities & V4L2_CAP_STREAMING))
    {
        DEBUG (DEB_LEV_ERR, "%s does not support streaming i/o\n",
            V4L2DEV_FILENAME);
        return OMX_ErrorHardware;
    }

    
    return err;

ERR_HANDLE:
    DEBUG (DEB_LEV_ERR, "close device, %s, %d  !!!\n",  __func__, __LINE__);
    camera_CloseCameraDevice(omx_camera_source_component_Private);
    return err;
}
static OMX_ERRORTYPE
camera_CloseCameraDevice (OMX_IN omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;


    DEBUG (DEB_LEV_ERR, "close device,  %s, %d !!!\n",  __func__,__LINE__);
    if (omx_camera_source_component_Private->fdCam >= 0)
    {
        DEBUG (DEB_LEV_ERR, "close device, %s, %d  !!!\n",  __func__, __LINE__);
        close (omx_camera_source_component_Private->fdCam);
        omx_camera_source_component_Private->fdCam = -1;
    }

    return err;

}

/* Initialize the camera device */
static OMX_ERRORTYPE
camera_InitCameraDevice (OMX_IN omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private, OMX_U32 portIndex)
{
    omx_base_PortType *pPreviewPort =
        (omx_base_PortType *)
        omx_camera_source_component_Private->ports[OMX_CAMPORT_INDEX_VF];
    omx_base_PortType *pCapturePort =
        (omx_base_PortType *)
        omx_camera_source_component_Private->ports[OMX_CAMPORT_INDEX_CP];
    omx_base_PortType *pPort;
    OMX_ERRORTYPE err = OMX_ErrorNone;

    OMX_U32 width, height, format;

    

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    if(omx_camera_source_component_Private->fdCam < 0){
        return OMX_ErrorHardware;
    }

    
    /* Select video input, video standard and tune here. */
    omx_camera_source_component_Private->cropcap.type =
        V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 ==
        xioctl (omx_camera_source_component_Private->fdCam, VIDIOC_CROPCAP,
            &omx_camera_source_component_Private->cropcap))
    {
        /* Errors ignored. */
    }

    omx_camera_source_component_Private->crop.type =
        V4L2_BUF_TYPE_VIDEO_CAPTURE;
    omx_camera_source_component_Private->crop.c = omx_camera_source_component_Private->cropcap.defrect; /* reset to default */

    if (-1 ==
        xioctl (omx_camera_source_component_Private->fdCam, VIDIOC_S_CROP,
            &omx_camera_source_component_Private->crop))
    {
        switch (errno)
        {
        case EINVAL:
            /* Cropping not supported. */
            break;
        default:
            /* Errors ignored. */
            break;
        }
    }

    CLEAR (omx_camera_source_component_Private->fmt);

    /* About camera color format settings.... */
    /* Get the camera sensor color format from an enabled port */
    if (portIndex == OMX_CAMPORT_INDEX_CP)
    {
        pPort = pCapturePort;

        width = pPort->sPortParam.format.image.nFrameWidth;
        height = pPort->sPortParam.format.image.nFrameHeight;
        format = pPort->sPortParam.format.image.eColorFormat;
    }
    else
    {
        
        pPort = pPreviewPort;

        width = pPort->sPortParam.format.video.nFrameWidth;
        height = pPort->sPortParam.format.video.nFrameHeight;
        format = pPort->sPortParam.format.video.eColorFormat;
    }
    omx_camera_source_component_Private->eOmxColorFormat =format;

    DEBUG (DEB_LEV_PARAMS,
            "%s: width=%d\n", __func__,(int)width);
    DEBUG (DEB_LEV_PARAMS,
            "%s: height=%d\n", __func__,(int)height);
    DEBUG (DEB_LEV_PARAMS,
            "%s: format=%d\n", __func__,(int)format);
    
    if ((err =
            camera_MapColorFormatOmxToV4l
            (format,
             &omx_camera_source_component_Private->sV4lColorFormat)) !=
        OMX_ErrorNone)
    {
        DEBUG (DEB_LEV_ERR,
            "%s: <ERROR> -- map color from omx to v4l failed!\n", __func__);
        goto ERR_HANDLE;
    }

    /** Initialize video capture pixel format */
    /* First get original color format from camera device */
    omx_camera_source_component_Private->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 ==
        xioctl (omx_camera_source_component_Private->fdCam, VIDIOC_G_FMT,
            &omx_camera_source_component_Private->fmt))
    {
        DEBUG (DEB_LEV_ERR, "%s error %d, %s\n", "VIDIOC_G_FMT", errno,
            strerror (errno));
        err = OMX_ErrorHardware;
        goto ERR_HANDLE;
    }

    DEBUG (DEB_LEV_SIMPLE_SEQ,
        "%s: v4l2_format.fmt.pix.pixelformat (Before set) = %c%c%c%c\n",
        __func__,
        (char) (omx_camera_source_component_Private->fmt.fmt.pix.
            pixelformat),
        (char) (omx_camera_source_component_Private->fmt.fmt.pix.
            pixelformat >> 8),
        (char) (omx_camera_source_component_Private->fmt.fmt.pix.
            pixelformat >> 16),
        (char) (omx_camera_source_component_Private->fmt.fmt.pix.
            pixelformat >> 24));
    DEBUG (DEB_LEV_SIMPLE_SEQ,
        "%s: v4l2_format.fmt.pix.field (Before set) = %d\n", __func__,
        omx_camera_source_component_Private->fmt.fmt.pix.field);

    /* Set color format and frame size to camera device */
    omx_camera_source_component_Private->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   
    omx_camera_source_component_Private->fmt.fmt.pix.width = width;
    omx_camera_source_component_Private->fmt.fmt.pix.height = height;
    
    omx_camera_source_component_Private->fmt.fmt.pix.pixelformat = omx_camera_source_component_Private->sV4lColorFormat.v4l2Pixfmt;
    omx_camera_source_component_Private->fmt.fmt.pix.field =
        V4L2_FIELD_INTERLACED;

    DEBUG (DEB_LEV_SIMPLE_SEQ,
        "%s: v4l2_format.fmt.pix.pixelformat (Before set) = %c%c%c%c\n",
        __func__,
        (char) (omx_camera_source_component_Private->fmt.fmt.pix.
            pixelformat),
        (char) (omx_camera_source_component_Private->fmt.fmt.pix.
            pixelformat >> 8),
        (char) (omx_camera_source_component_Private->fmt.fmt.pix.
            pixelformat >> 16),
        (char) (omx_camera_source_component_Private->fmt.fmt.pix.
            pixelformat >> 24));
    if (-1 ==
        xioctl (omx_camera_source_component_Private->fdCam, VIDIOC_S_FMT,
            &omx_camera_source_component_Private->fmt))
    {
        DEBUG (DEB_LEV_ERR, "%s error %d, %s\n", "VIDIOC_S_FMT", errno,
            strerror (errno));
        err = OMX_ErrorHardware;
        goto ERR_HANDLE;
    }

    DEBUG (DEB_LEV_SIMPLE_SEQ,
        "%s: v4l2_format.fmt.pix.pixelformat (After set) = %c%c%c%c\n",
        __func__,
        (char) (omx_camera_source_component_Private->fmt.fmt.pix.
            pixelformat),
        (char) (omx_camera_source_component_Private->fmt.fmt.pix.
            pixelformat >> 8),
        (char) (omx_camera_source_component_Private->fmt.fmt.pix.
            pixelformat >> 16),
        (char) (omx_camera_source_component_Private->fmt.fmt.pix.
            pixelformat >> 24));
    DEBUG (DEB_LEV_SIMPLE_SEQ,
        "%s: v4l2_format.fmt.pix.field (After set) = %d\n", __func__,
        omx_camera_source_component_Private->fmt.fmt.pix.field);

    /* Note VIDIOC_S_FMT may change width and height. */
    if (portIndex == OMX_CAMPORT_INDEX_CP)
    {
        pPort->sPortParam.format.image.nFrameWidth =
            omx_camera_source_component_Private->fmt.fmt.pix.width;
        pPort->sPortParam.format.image.nFrameHeight =
            omx_camera_source_component_Private->fmt.fmt.pix.height;

         DEBUG (DEB_LEV_SIMPLE_SEQ,
        "Frame Width=%d, Height=%d,  nFrame=%d\n",
        (int) pPort->sPortParam.format.image.nFrameWidth,
        (int) pPort->sPortParam.format.image.nFrameHeight,
        (int) pPort->sPortParam.nBufferCountActual);
    }
    else
    {
        pPort->sPortParam.format.video.nFrameWidth =
            omx_camera_source_component_Private->fmt.fmt.pix.width;
        pPort->sPortParam.format.video.nFrameHeight =
            omx_camera_source_component_Private->fmt.fmt.pix.height;

         DEBUG (DEB_LEV_SIMPLE_SEQ,
        "Frame Width=%d, Height=%d,  nFrame=%d\n",
        (int) pPort->sPortParam.format.video.nFrameWidth,
        (int) pPort->sPortParam.format.video.nFrameHeight,
        (int) pPort->sPortParam.nBufferCountActual);

    }

    

   


    /* Allocate time stamp queue */

    err = camera_InitBufQueue(omx_camera_source_component_Private, portIndex);
    if(err != OMX_ErrorNone){
        goto ERR_HANDLE;
    }

    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;

ERR_HANDLE:
    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}

static OMX_ERRORTYPE
camera_InitBufQueue (OMX_IN omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private, OMX_U32 portIndex){
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_U32 nBufCount = 0;
    OMX_U32 nBufWidth = 0;
    OMX_U32 nBufHeight = 0;
    
    omx_base_PortType *pPreviewPort =
        (omx_base_PortType *)
        omx_camera_source_component_Private->ports[OMX_CAMPORT_INDEX_VF];
    omx_base_PortType *pCapturePort =
        (omx_base_PortType *)
        omx_camera_source_component_Private->ports[OMX_CAMPORT_INDEX_CP];

    omx_base_PortType *pPort;
    if(portIndex == OMX_CAMPORT_INDEX_VF){
        pPort = pPreviewPort;
    }else if(portIndex == OMX_CAMPORT_INDEX_CP){
        pPort = pCapturePort;
    }else{
        err = OMX_ErrorHardware;
        goto ERR_HANDLE;
    }
    
        
    nBufCount = pPort->sPortParam.nBufferCountActual;
    nBufWidth = pPort->sPortParam.format.video.nFrameWidth;
    nBufHeight = pPort->sPortParam.format.video.nFrameHeight;
   
      
    DEBUG(DEB_LEV_SIMPLE_SEQ, " nBufCount = %d\n",(int)nBufCount);

    omx_camera_source_component_Private->sMapbufQueue.buffers
        = calloc(nBufCount,
        sizeof(*omx_camera_source_component_Private->sMapbufQueue.buffers));

    if (!omx_camera_source_component_Private->sMapbufQueue.buffers)
    {
        DEBUG(DEB_LEV_ERR, "Out of memory\n");
        err = OMX_ErrorHardware;
        goto ERR_HANDLE;
    }

    omx_camera_source_component_Private->sMapbufQueue.bufferQueue
        = calloc(nBufCount, sizeof(OMX_U32*));

    if (!omx_camera_source_component_Private->sMapbufQueue.bufferQueue)
    {
        DEBUG(DEB_LEV_ERR, "Out of memory\n");
        err = OMX_ErrorHardware;
        goto ERR_HANDLE;
    }
    omx_camera_source_component_Private->sMapbufQueue.nFrame = nBufCount;
    omx_camera_source_component_Private->sMapbufQueue.nWidth = nBufWidth;
    omx_camera_source_component_Private->sMapbufQueue.nHeight = nBufHeight;
    omx_camera_source_component_Private->sMapbufQueue.nPortIndex = portIndex;
    

    DEBUG(DEB_LEV_SIMPLE_SEQ, " nBufCount = %d\n",(int)nBufCount);
    camera_init_userptr (omx_camera_source_component_Private, portIndex,nBufCount);

    omx_camera_source_component_Private->sMapbufQueue.qTimeStampQueue =
        calloc (omx_camera_source_component_Private->sMapbufQueue.nFrame,
            sizeof (OMX_TICKS));
    if (omx_camera_source_component_Private->sMapbufQueue.qTimeStampQueue ==
        NULL)
    {
        DEBUG (DEB_LEV_ERR,
            "%s: <ERROR> -- Allocate time stamp queue failed!\n", __func__);
        err = OMX_ErrorInsufficientResources;
        goto ERR_HANDLE;
    }      
    omx_camera_source_component_Private->sMapbufQueue.bInit = OMX_TRUE;
    return err;


ERR_HANDLE:
    camera_FreeBufQueue(omx_camera_source_component_Private);
    return err;
}
/* Deinitialize the camera device */
static OMX_ERRORTYPE
camera_FreeBufQueue (OMX_IN omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_U32 i;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);
    omx_camera_source_component_Private->sMapbufQueue.bInit = OMX_FALSE;
    if (omx_camera_source_component_Private->sMapbufQueue.qTimeStampQueue !=
        NULL)
    {
        free (omx_camera_source_component_Private->sMapbufQueue.
            qTimeStampQueue);
        omx_camera_source_component_Private->sMapbufQueue.qTimeStampQueue =
            NULL;
    }

    if (omx_camera_source_component_Private->sMapbufQueue.bufferQueue != NULL)
    {
        free(omx_camera_source_component_Private->sMapbufQueue.bufferQueue);
        omx_camera_source_component_Private->sMapbufQueue.bufferQueue = NULL;
    }

    if (omx_camera_source_component_Private->sMapbufQueue.buffers != NULL)
    {

        for (i = 0;
            i <
            OMX_MAPBUFQUEUE_GETMAXLEN
            (omx_camera_source_component_Private->sMapbufQueue); i++)
        {

        }
        free (omx_camera_source_component_Private->sMapbufQueue.buffers);

        omx_camera_source_component_Private->sMapbufQueue.buffers = NULL;
    }
    
    

    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}

/* Start the camera device */
static OMX_ERRORTYPE
camera_StartCameraDevice (OMX_IN omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private, OMX_U32 portIndex)
{
    struct timeval now;
    struct timespec sleepTime;
    OMX_U32 i = 0;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    enum v4l2_buf_type type;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

        
    err = camera_InitCameraDevice(omx_camera_source_component_Private, portIndex);
    if(err != OMX_ErrorNone){
        err = OMX_ErrorHardware;
        goto EXIT;
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 ==
        xioctl (omx_camera_source_component_Private->fdCam, VIDIOC_STREAMON,
            &type))
    {
        DEBUG (DEB_LEV_ERR, "%s error %d, %s\n", "VIDIOC_STREAMON", errno,
            strerror (errno));
        err = OMX_ErrorHardware;
        goto EXIT;
    }
    omx_camera_source_component_Private->bDeviceStarted = OMX_TRUE;
    omx_camera_source_component_Private->bFirstFrame = OMX_TRUE;

    DEBUG (DEB_LEV_FUNCTION_NAME, "%s, %d", __func__, __LINE__);

EXIT:
    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}

/* Stop the camera device */
static OMX_ERRORTYPE
camera_StopCameraDevice (OMX_IN omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private, OMX_BOOL restore)
{
    OMX_U32 i = 0;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    int ioErr = 0;
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;
    OMX_BUFFERHEADERTYPE *pBufHeader = NULL;

    omx_base_PortType
        * pPreviewPort =
        (omx_base_PortType *) omx_camera_source_component_Private->ports[OMX_CAMPORT_INDEX_VF];
    omx_base_PortType
        * pCapturePort =
        (omx_base_PortType *) omx_camera_source_component_Private->ports[OMX_CAMPORT_INDEX_CP];

    omx_base_PortType
        * pPort;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    if(OMX_MAPBUFQUEUE_PORTINDEX(omx_camera_source_component_Private->sMapbufQueue) == OMX_CAMPORT_INDEX_CP){
        pPort = pCapturePort;
    }else if(OMX_MAPBUFQUEUE_PORTINDEX(omx_camera_source_component_Private->sMapbufQueue) == OMX_CAMPORT_INDEX_VF){
        pPort = pPreviewPort;
    }else{
        pPort = NULL;
    }
    omx_camera_source_component_Private->bDeviceStarted = OMX_FALSE;

    

    if(omx_camera_source_component_Private->fdCam < 0 )
    {
        return err;
    }

    //send capture frame
     /* Try to send buffers */
    if (OMX_MAPBUFQUEUE_HASBUFCAPTURED
        (omx_camera_source_component_Private->sMapbufQueue))
    {
        camera_SendCapturedBuffers (omx_camera_source_component_Private);
    } 
    
    //stream off
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    do
    {
        ioErr =
            xioctl (omx_camera_source_component_Private->fdCam, VIDIOC_STREAMOFF,
                &type);
    }
    while (ioErr < 0 && EINTR == errno);
    if (ioErr < 0)
    {
        DEBUG (DEB_LEV_ERR,
            "%s: <ERROR> -- Wait the camera hardware to finish capturing failed: %s\n",
            __func__, GET_SYSERR_STRING ());
        err = OMX_ErrorHardware;
        goto EXIT;
    }

    //enqueue waiting frame
    if(restore == OMX_TRUE)
    {

        while (OMX_MAPBUFQUEUE_HASBUFWAITTOCAPTURE
            (omx_camera_source_component_Private->sMapbufQueue))
        {
            i =
                OMX_MAPBUFQUEUE_GETLASTBUFFER
                (omx_camera_source_component_Private->sMapbufQueue);

            pBufHeader =  (OMX_BUFFERHEADERTYPE*)OMX_MAPBUFQUEUE_GETBUFADDR (             
                omx_camera_source_component_Private->sMapbufQueue, i);
            if(pBufHeader){
                queue (pPort->pBufferQueue, pBufHeader); 
                tsem_up(pPort->pBufferSem);
            }

            OMX_MAPBUFQUEUE_DEQUEUE(omx_camera_source_component_Private->sMapbufQueue);

        }
    }
    
    /* Reset Mapping Buffer Queue */
    OMX_MAPBUFQUEUE_MAKEEMPTY
        (omx_camera_source_component_Private->sMapbufQueue);

    camera_FreeBufQueue(omx_camera_source_component_Private);

    /* Reset port mapbuf queue index */
    for (i = 0;i <OMX_CAMPORT_INDEX_MAX; i++)
    {
        if(PORTTYPE_IS_VIDEO(i))
        {
            omx_camera_source_component_video_PortType *port;
            port =
                (omx_camera_source_component_video_PortType *)
                omx_camera_source_component_Private->ports[i];
            
        }else if(PORTTYPE_IS_IMAGE(i)){
            omx_camera_source_component_image_PortType *port;
            port =
                (omx_camera_source_component_image_PortType *)
                omx_camera_source_component_Private->ports[i];
            
        }
    }

EXIT:
    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}

/** The Constructor for camera source component
 * @param openmaxStandComp is the pointer to the OMX component
 * @param cComponentName is the name of the constructed component
 */
OMX_ERRORTYPE omx_camera_source_component_Constructor(
    OMX_COMPONENTTYPE * openmaxStandComp, OMX_STRING cComponentName)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_U32 i, index;
    omx_camera_source_component_video_PortType *previewPort;
    omx_camera_source_component_image_PortType *imagePort;
    omx_camera_source_component_PrivateType
        * omx_camera_source_component_Private;
    OMX_U32 nTotalPorts = 0;

    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    //RM_RegisterComponent(CAMERA_COMP_NAME, MAX_CAMERA_COMPONENTS);

    /** Allocate camera component private structure */
    if (!openmaxStandComp->pComponentPrivate)
    {
        DEBUG(DEB_LEV_FUNCTION_NAME, "In %s, allocating component\n", __func__);
        openmaxStandComp->pComponentPrivate = calloc(1,
            sizeof(omx_camera_source_component_PrivateType));
        if (openmaxStandComp->pComponentPrivate == NULL)
        {
            return OMX_ErrorInsufficientResources;
        }
    }
    else
    {
        DEBUG(DEB_LEV_FUNCTION_NAME,
            "In %s, Error Component %x Already Allocated\n", __func__,
            (int) openmaxStandComp->pComponentPrivate);
    }

    /* Call base source constructor */
    omx_camera_source_component_Private
        = (omx_camera_source_component_PrivateType *) openmaxStandComp-> pComponentPrivate;
    err = omx_base_source_Constructor(openmaxStandComp, cComponentName);

    /* Overwrite default settings by base source */
    omx_camera_source_component_Private
        = (omx_camera_source_component_PrivateType *) openmaxStandComp-> pComponentPrivate;
    omx_camera_source_component_Private->sPortTypesParam[OMX_PortDomainVideo]. nStartPortNumber
        = CAMERA_VIDEO_PORT_START;
    omx_camera_source_component_Private->sPortTypesParam[OMX_PortDomainVideo]. nPorts
        = NUM_CAMERA_VIDEO_PORTS;

    omx_camera_source_component_Private->sPortTypesParam[OMX_PortDomainImage]. nStartPortNumber
        = CAMERA_IMAGE_PORT_START;
    omx_camera_source_component_Private->sPortTypesParam[OMX_PortDomainImage]. nPorts
        = NUM_CAMERA_IMAGE_PORTS;

    omx_camera_source_component_Private->BufferMgmtFunction
        = omx_camera_source_component_BufferMgmtFunction;

    /** Init camera private parameters by default values */
    pthread_mutex_init(&omx_camera_source_component_Private->idle_state_mutex,
        NULL);
    pthread_cond_init(
        &omx_camera_source_component_Private-> idle_wait_condition, NULL);
    pthread_cond_init(
        &omx_camera_source_component_Private-> idle_process_condition, NULL);
    omx_camera_source_component_Private->bWaitingOnIdle = OMX_FALSE;

    pthread_mutex_init(&omx_camera_source_component_Private->setconfig_mutex,
        NULL);

    omx_camera_source_component_Private->eOmxColorFormat = DEFAULT_COLOR_FORMAT;
    omx_camera_source_component_Private->sV4lColorFormat.v4l2Pixfmt
        = V4L2_PIX_FMT_YUV420;
    omx_camera_source_component_Private->sV4lColorFormat.v4l2Depth = 12;

    omx_camera_source_component_Private->fdCam = -1;
    memset(&omx_camera_source_component_Private->sMapbufQueue, 0,
        sizeof(OMX_V4L2_MAPBUFFER_QUEUETYPE));
    omx_camera_source_component_Private->sMapbufQueue.nFrame = 0;
    omx_camera_source_component_Private->sMapbufQueue.bInit = OMX_FALSE;
    omx_camera_source_component_Private->bCapturing = OMX_FALSE;
    omx_camera_source_component_Private->bCapturingNext = OMX_FALSE;
    omx_camera_source_component_Private->bIsFirstFrame = OMX_FALSE;
    omx_camera_source_component_Private->bAutoPause = OMX_FALSE;
    omx_camera_source_component_Private->bThumbnailStart = OMX_FALSE;
    omx_camera_source_component_Private->nCapturedCount = 0;
    omx_camera_source_component_Private->bDeviceStarted = OMX_FALSE;
    omx_camera_source_component_Private->bFirstFrame = OMX_TRUE;
    /**open ion driver*/
    omx_camera_source_component_Private->ion_fd = ion_open ();
    if (omx_camera_source_component_Private->ion_fd < 0)
    {
        return OMX_ErrorInsufficientResources;
    }

    setHeader(&omx_camera_source_component_Private->sSensorType, sizeof(OMX_PARAM_SENSORSELECTTYPE)); 
    omx_camera_source_component_Private->sSensorType.nPortIndex = OMX_ALL;
    omx_camera_source_component_Private->sSensorType.eSensor = OMX_PrimarySensor;

    /** Allocate Ports. */
    nTotalPorts
        = (omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainVideo].nPorts
            + omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainImage].nPorts);

    if (nTotalPorts && !omx_camera_source_component_Private->ports)
    {
        omx_camera_source_component_Private->ports = calloc(nTotalPorts,
            sizeof(omx_base_PortType *));
        if (!omx_camera_source_component_Private->ports)
        {
            return OMX_ErrorInsufficientResources;
        }
        for (i
            = omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber; i
            < omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainVideo].nPorts+omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber; i++)
        {
            /** this is the important thing separating this from the base class; size of the struct is for derived class port type
             * this could be refactored as a smarter factory function instead?
             */
            omx_camera_source_component_Private->ports[i] = calloc(1,
                sizeof(omx_camera_source_component_video_PortType));
            if (!omx_camera_source_component_Private->ports[i])
            {
                return OMX_ErrorInsufficientResources;
            }
        }
        for (i
            = omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainImage].nStartPortNumber; i
            < omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainImage].nPorts+omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainImage].nStartPortNumber; i++)
        {
            /** this is the important thing separating this from the base class; size of the struct is for derived class port type
             * this could be refactored as a smarter factory function instead?
             */
            omx_camera_source_component_Private->ports[i] = calloc(1,
                sizeof(omx_camera_source_component_image_PortType));
            if (!omx_camera_source_component_Private->ports[i])
            {
                return OMX_ErrorInsufficientResources;
            }
        }
    }

    for (i
        = omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber; i
        < omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainVideo].nPorts+omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber; i++)
    {
        /** Call base port constructor */
        base_video_port_Constructor(openmaxStandComp,
            &omx_camera_source_component_Private-> ports[i], i, OMX_FALSE);
        previewPort
            = (omx_camera_source_component_video_PortType *) omx_camera_source_component_Private->ports[i];
        /** Init port parameters by default values */
        previewPort->sPortParam.nBufferSize = DEFAULT_FRAME_WIDTH
            * DEFAULT_FRAME_HEIGHT * 3 / 2;
        previewPort->sPortParam.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
        previewPort->sPortParam.format.video.nFrameHeight
            = DEFAULT_FRAME_HEIGHT;
        previewPort->sPortParam.format.video.nStride = DEFAULT_FRAME_WIDTH;
        previewPort->sPortParam.format.video.nSliceHeight
            = DEFAULT_FRAME_HEIGHT;
        previewPort->sPortParam.format.video.xFramerate = DEFAULT_FRAME_RATE;
        previewPort->sPortParam.format.video.eColorFormat
            = DEFAULT_COLOR_FORMAT;
#ifdef PORT_DISABLE
        previewPort->sPortParam.bEnabled = OMX_FALSE;
#endif
        previewPort->sPortParam.nBufferCountActual = 4;
        previewPort->bIsStoreMetaData = OMX_FALSE;
    }
    for (i
        = omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainImage].nStartPortNumber; i
        < omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainImage].nPorts+omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainImage].nStartPortNumber; i++)
    {
        /** Call base port constructor */
        base_image_port_Constructor(openmaxStandComp,
            &omx_camera_source_component_Private-> ports[i], i, OMX_FALSE);
        imagePort
            = (omx_camera_source_component_image_PortType *) omx_camera_source_component_Private->ports[i];
        /** Init port parameters by default values */
        imagePort->sPortParam.nBufferSize = DEFAULT_FRAME_WIDTH
            * DEFAULT_FRAME_HEIGHT * 3 / 2;
        imagePort->sPortParam.format.image.nFrameWidth = DEFAULT_FRAME_WIDTH;
        imagePort->sPortParam.format.image.nFrameHeight = DEFAULT_FRAME_HEIGHT;
        imagePort->sPortParam.format.image.nStride = DEFAULT_FRAME_WIDTH;
        imagePort->sPortParam.format.image.nSliceHeight = DEFAULT_FRAME_HEIGHT;
        imagePort->sPortParam.format.image.eColorFormat = DEFAULT_COLOR_FORMAT;
#ifdef PORT_DISABLE
        imagePort->sPortParam.bEnabled = OMX_FALSE;
#endif
        imagePort->sPortParam.nBufferCountActual = 4;
        imagePort->bIsStoreMetaData = OMX_FALSE;
    }

    /** set the function pointers */
    omx_camera_source_component_Private->DoStateSet
        = &omx_camera_source_component_DoStateSet;
    omx_camera_source_component_Private->destructor
        = omx_camera_source_component_Destructor;
    openmaxStandComp->SetParameter = omx_camera_source_component_SetParameter;
    openmaxStandComp->GetParameter = omx_camera_source_component_GetParameter;
    openmaxStandComp->SetConfig = omx_camera_source_component_SetConfig;
    openmaxStandComp->GetConfig = omx_camera_source_component_GetConfig;

    DEBUG(DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}

/** The Destructor for camera source component
 * @param openmaxStandComp is the pointer to the OMX component
 */
OMX_ERRORTYPE omx_camera_source_component_Destructor(
    OMX_COMPONENTTYPE * openmaxStandComp)
{
    omx_camera_source_component_PrivateType
        * omx_camera_source_component_Private =
        (omx_camera_source_component_PrivateType *) openmaxStandComp-> pComponentPrivate;
    OMX_U32 i, index;
    OMX_U32 nTotalPorts = 0;

    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    /** frees the port structures*/
    nTotalPorts
        = (omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainVideo].nPorts
            + omx_camera_source_component_Private-> sPortTypesParam[OMX_PortDomainImage].nPorts);
    if (nTotalPorts && omx_camera_source_component_Private->ports)
    {
        for (i = 0; i < nTotalPorts; i++)
        {
            if (omx_camera_source_component_Private->ports[i])
            {
                base_port_Destructor(
                    omx_camera_source_component_Private-> ports[i]);
            }
        }
        free(omx_camera_source_component_Private->ports);
        omx_camera_source_component_Private->ports = NULL;
    }

    pthread_mutex_destroy(
        &omx_camera_source_component_Private-> idle_state_mutex);
    pthread_cond_destroy(
        &omx_camera_source_component_Private-> idle_wait_condition);
    pthread_cond_destroy(
        &omx_camera_source_component_Private-> idle_process_condition);

    pthread_mutex_destroy(
        &omx_camera_source_component_Private-> setconfig_mutex);

    camera_FreeBufQueue(omx_camera_source_component_Private);
    DEBUG (DEB_LEV_ERR, "close device, %s, %d  !!!\n",  __func__, __LINE__);
    camera_CloseCameraDevice(omx_camera_source_component_Private);

    if (omx_camera_source_component_Private->ion_fd >= 0)
    {
        ion_close (omx_camera_source_component_Private->ion_fd);
        omx_camera_source_component_Private->ion_fd = -1;
    }
    DEBUG(DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        OMX_ErrorNone);
    return omx_base_source_Destructor(openmaxStandComp);;
}

/** Specific operations for camera component when state changes
 * @param openmaxStandComp the OpenMAX component which state is to be changed
 * @param destinationState the requested target state
 */
static OMX_ERRORTYPE omx_camera_source_component_DoStateSet(
    OMX_COMPONENTTYPE * openmaxStandComp, OMX_U32 destinationState)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    omx_camera_source_component_PrivateType
        * omx_camera_source_component_Private =
        (omx_camera_source_component_PrivateType *) openmaxStandComp-> pComponentPrivate;

    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    DEBUG(
        DEB_LEV_SIMPLE_SEQ,
        "%s: Before base DoStateSet: destinationState=%ld,omx_camera_source_component_Private->state=%d,omx_camera_source_component_Private->transientState=%d\n",
        __func__, destinationState,
        omx_camera_source_component_Private->state,
        omx_camera_source_component_Private->transientState);

    if (omx_camera_source_component_Private->state == OMX_StateLoaded
        && destinationState == OMX_StateIdle)
    {
        /* Loaded --> Idle */
        if ((err = camera_OpenCameraDevice(omx_camera_source_component_Private))
            != OMX_ErrorNone)
        {
            goto EXIT;
        }
    }
    else if (omx_camera_source_component_Private->state == OMX_StateIdle
        && destinationState == OMX_StateExecuting)
    {
        /* Idle --> Exec */
        pthread_mutex_lock(
            &omx_camera_source_component_Private-> idle_state_mutex);
        if (!omx_camera_source_component_Private->bWaitingOnIdle)
        {
            pthread_cond_wait(
                &omx_camera_source_component_Private-> idle_process_condition,
                &omx_camera_source_component_Private-> idle_state_mutex);
        }
        
        pthread_mutex_unlock(
            &omx_camera_source_component_Private-> idle_state_mutex);
    }
    else if (omx_camera_source_component_Private->state == OMX_StateIdle
        && destinationState == OMX_StateLoaded)
    {
        /* Idle --> Loaded */
        pthread_mutex_lock(
            &omx_camera_source_component_Private-> idle_state_mutex);
        if (!omx_camera_source_component_Private->bWaitingOnIdle)
        {
            pthread_cond_wait(
                &omx_camera_source_component_Private-> idle_process_condition,
                &omx_camera_source_component_Private-> idle_state_mutex);
        }
        camera_FreeBufQueue(omx_camera_source_component_Private);
        DEBUG (DEB_LEV_ERR, "close device, %s, %d  !!!\n",  __func__, __LINE__);
        camera_CloseCameraDevice(omx_camera_source_component_Private);
        if (omx_camera_source_component_Private->bWaitingOnIdle)
        {
            pthread_cond_signal(
                &omx_camera_source_component_Private-> idle_wait_condition);
        }
        pthread_mutex_unlock(
            &omx_camera_source_component_Private-> idle_state_mutex);
    }

    omx_camera_source_component_Private->eLastState
        = omx_camera_source_component_Private->state;
    err = omx_base_component_DoStateSet(openmaxStandComp, destinationState);
    DEBUG(
        DEB_LEV_SIMPLE_SEQ,
        "%s: After base DoStateSet: destinationState=%ld,omx_camera_source_component_Private->state=%d,omx_camera_source_component_Private->transientState=%d\n",
        __func__, destinationState,
        omx_camera_source_component_Private->state,
        omx_camera_source_component_Private->transientState);

    if (omx_camera_source_component_Private->eLastState == OMX_StateIdle
        && omx_camera_source_component_Private->state == OMX_StateExecuting)
    {
        /* Idle --> Exec */
        pthread_mutex_lock(
            &omx_camera_source_component_Private-> idle_state_mutex);
        if (omx_camera_source_component_Private->bWaitingOnIdle)
        {
            /*
            if ((err = camera_StartCameraDevice(
                        omx_camera_source_component_Private)) != OMX_ErrorNone)
            {
                pthread_mutex_unlock(
                    &omx_camera_source_component_Private-> idle_state_mutex);
                goto EXIT;
            }
            */
            pthread_cond_signal(
                &omx_camera_source_component_Private-> idle_wait_condition);
        }
        pthread_mutex_unlock(
            &omx_camera_source_component_Private-> idle_state_mutex);
    }
    else if (omx_camera_source_component_Private->eLastState
        == OMX_StateExecuting && omx_camera_source_component_Private->state
        == OMX_StateIdle)
    {
        /* Exec --> Idle */
        pthread_mutex_lock(
            &omx_camera_source_component_Private-> idle_state_mutex);
        if (!omx_camera_source_component_Private->bWaitingOnIdle)
        {
            pthread_cond_wait(
                &omx_camera_source_component_Private-> idle_process_condition,
                &omx_camera_source_component_Private-> idle_state_mutex);
        }
        camera_StopCameraDevice(omx_camera_source_component_Private, OMX_FALSE);
        pthread_mutex_unlock(
            &omx_camera_source_component_Private-> idle_state_mutex);
    }

EXIT: DEBUG(DEB_LEV_FUNCTION_NAME,
          "Out of %s for camera component, return code: 0x%X\n", __func__,
          err);
      return err;
}

/** The GetParameter method for camera source component
 * @param hComponent input parameter, the handle of V4L2 camera component
 * @param nParamIndex input parameter, the index of the structure to be filled
 * @param ComponentParameterStructure inout parameter, a pointer to the structure that receives parameters
 */
static OMX_ERRORTYPE
omx_camera_source_component_GetParameter (OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR
    ComponentParameterStructure)
{   

    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE *openmaxStandComp;
    omx_camera_source_component_PrivateType
        * omx_camera_source_component_Private;
    
    OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoPortFormat;
    OMX_PARAM_SENSORMODETYPE *pSensorMode;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    if (ComponentParameterStructure == NULL)
    {
        return OMX_ErrorBadParameter;
    }

    openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
    omx_camera_source_component_Private =
        (omx_camera_source_component_PrivateType *)
        openmaxStandComp->pComponentPrivate;

    DEBUG (DEB_LEV_SIMPLE_SEQ, "%s: Getting parameter %i\n", __func__,
        nParamIndex);

    switch (nParamIndex)
    {
    case OMX_IndexParamVideoInit:
        if ((err =
                checkHeader (ComponentParameterStructure,
                    sizeof (OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone)
        {
            DEBUG (DEB_LEV_ERR, "%s (line %d): Check header failed!\n",
                __func__, __LINE__);
            break;
        }
        memcpy (ComponentParameterStructure,
            &omx_camera_source_component_Private->sPortTypesParam
            [OMX_PortDomainVideo], sizeof (OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamVideoPortFormat:
        {
            omx_base_PortType *pPreviewPort;
            pVideoPortFormat =
                (OMX_VIDEO_PARAM_PORTFORMATTYPE *) ComponentParameterStructure;
            if ((err =
                    checkHeader (ComponentParameterStructure,
                        sizeof (OMX_VIDEO_PARAM_PORTFORMATTYPE))) !=
                OMX_ErrorNone)
            {
                DEBUG (DEB_LEV_ERR, "%s (line %d): Check header failed!\n",
                    __func__, __LINE__);
                break;
            }
            if (PORTTYPE_IS_VIDEO(pVideoPortFormat->nPortIndex))
            {
                pPreviewPort =
                    (omx_base_PortType *)
                    omx_camera_source_component_Private->ports[pVideoPortFormat->
                    nPortIndex];
                pVideoPortFormat->eCompressionFormat =
                    pPreviewPort->sPortParam.format.video.eCompressionFormat;
                pVideoPortFormat->eColorFormat =
                    pPreviewPort->sPortParam.format.video.eColorFormat;
            }
            else
            {
                err = OMX_ErrorBadPortIndex;
            }
        }
        break;


    default: /*Call the base component function */
        err =
            omx_base_component_GetParameter (hComponent, nParamIndex,
                ComponentParameterStructure);
        break;
    }

    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}

/** The SetParameter method for camera source component
 * @param hComponent input parameter, the handle of V4L2 camera component
 * @param nParamIndex input parameter, the index of the structure to be set
 * @param ComponentParameterStructure input parameter, a pointer to the parameter structure
 */
static OMX_ERRORTYPE
omx_camera_source_component_SetParameter (OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_IN OMX_PTR
    ComponentParameterStructure)
{   

    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE *openmaxStandComp;
    omx_camera_source_component_PrivateType
        * omx_camera_source_component_Private;
    OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
    OMX_VIDEO_PARAM_PORTFORMATTYPE *pVideoPortFormat;
    OMX_PARAM_SENSORMODETYPE *pSensorMode;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    if (ComponentParameterStructure == NULL)
    {
        return OMX_ErrorBadParameter;
    }

    openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
    omx_camera_source_component_Private =
        (omx_camera_source_component_PrivateType *)
        openmaxStandComp->pComponentPrivate;

    DEBUG (DEB_LEV_SIMPLE_SEQ, "%s: Setting parameter %i\n", __func__,
        nParamIndex);

    switch (nParamIndex)
    {
    case OMX_IndexParamVideoInit:
        if ((err =
                checkHeader (ComponentParameterStructure,
                    sizeof (OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone)
        {
            DEBUG (DEB_LEV_ERR, "%s (line %d): Check header failed!\n",
                __func__, __LINE__);
            break;
        }
        memcpy (&omx_camera_source_component_Private->sPortTypesParam
            [OMX_PortDomainVideo], ComponentParameterStructure,
            sizeof (OMX_PORT_PARAM_TYPE));
        break;

    case OMX_IndexParamPortDefinition:
        {
            omx_base_PortType *pPort;
            pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *) ComponentParameterStructure;
            if (PORTTYPE_IS_VIDEO(pPortDef->nPortIndex))
            {
                err =
                    camera_CheckSupportedColorFormat (pPortDef->format.video.
                        eColorFormat);
                if (err != OMX_ErrorNone)
                {
                    DEBUG (DEB_LEV_ERR,
                        "%s (line %d): Supported Color Format Check failed!\n",
                        __func__, __LINE__);
                    break;
                }
                err =
                    camera_CheckSupportedFramesize (pPortDef->format.video.
                        nFrameWidth,
                        pPortDef->format.video.
                        nFrameHeight);
                if (err != OMX_ErrorNone)
                {
                    DEBUG (DEB_LEV_ERR,
                        "%s (line %d): Supported Frame Size Check failed!\n",
                        __func__, __LINE__);
                    break;
                }
                /*Call the base component function */
                err =
                    omx_base_component_SetParameter (hComponent, nParamIndex,
                        ComponentParameterStructure);
                if (err != OMX_ErrorNone)
                {
                    DEBUG (DEB_LEV_ERR,
                        "%s (line %d): Call base SetParameter failed!\n",
                        __func__, __LINE__);
                    break;
                }
                pPort =
                    (omx_base_PortType *)
                    omx_camera_source_component_Private->ports[pPortDef->nPortIndex];
                memcpy (&pPort->sPortParam, pPortDef,
                    sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
                pPort->sPortParam.nBufferSize =
                    camera_CalculateBufferSize (pPort->sPortParam.format.video.
                        nFrameWidth,
                        pPort->sPortParam.format.video.
                        nFrameHeight,
                        pPort->sPortParam.format.video.
                        eColorFormat);
            }
            else if (PORTTYPE_IS_IMAGE(pPortDef->nPortIndex))
            {
                err =
                    camera_CheckSupportedColorFormat (pPortDef->format.image.
                        eColorFormat);
                if (err != OMX_ErrorNone)
                {
                    DEBUG (DEB_LEV_ERR,
                        "%s (line %d): Supported Color Format Check failed!\n",
                        __func__, __LINE__);
                    break;
                }
                err =
                    camera_CheckSupportedFramesize (pPortDef->format.image.
                        nFrameWidth,
                        pPortDef->format.image.
                        nFrameHeight);
                if (err != OMX_ErrorNone)
                {
                    DEBUG (DEB_LEV_ERR,
                        "%s (line %d): Supported Frame Size Check failed!\n",
                        __func__, __LINE__);
                    break;
                }
                /*Call the base component function */
                err =
                    omx_base_component_SetParameter (hComponent, nParamIndex,
                        ComponentParameterStructure);
                if (err != OMX_ErrorNone)
                {
                    DEBUG (DEB_LEV_ERR,
                        "%s (line %d): Call base SetParameter failed!\n",
                        __func__, __LINE__);
                    break;
                }
                pPort =
                    (omx_base_PortType *)
                    omx_camera_source_component_Private->ports[pPortDef->nPortIndex];
                memcpy (&pPort->sPortParam, pPortDef,
                    sizeof (OMX_PARAM_PORTDEFINITIONTYPE));
                pPort->sPortParam.nBufferSize =
                    camera_CalculateBufferSize (pPort->sPortParam.format.image.
                        nFrameWidth,
                        pPort->sPortParam.format.image.
                        nFrameHeight,
                        pPort->sPortParam.format.image.
                        eColorFormat);

            }
            else
            {
            }
        }
        break;

    case OMX_IndexParamVideoPortFormat:
        {
            omx_base_PortType *pPort;
            pVideoPortFormat =
                (OMX_VIDEO_PARAM_PORTFORMATTYPE *) ComponentParameterStructure;
            err =
                omx_base_component_ParameterSanityCheck (hComponent,
                    pVideoPortFormat->nPortIndex,
                    pVideoPortFormat,
                    sizeof
                    (OMX_VIDEO_PARAM_PORTFORMATTYPE));
            if (err != OMX_ErrorNone)
            {
                DEBUG (DEB_LEV_ERR,
                    "%s (line %d): Parameter Sanity Check failed!\n", __func__,
                    __LINE__);
                break;
            }
            err = camera_CheckSupportedColorFormat (pVideoPortFormat->eColorFormat);
            if (err != OMX_ErrorNone)
            {
                DEBUG (DEB_LEV_ERR,
                    "%s (line %d): Supported Color Format Check failed!\n",
                    __func__, __LINE__);
                break;
            }
            if (PORTTYPE_IS_VIDEO(pVideoPortFormat->nPortIndex))
            {
                pPort =
                    (omx_base_PortType *)
                    omx_camera_source_component_Private->ports[pVideoPortFormat->
                    nPortIndex];
                pPort->sPortParam.format.video.eCompressionFormat =
                    pVideoPortFormat->eCompressionFormat;
                pPort->sPortParam.format.video.eColorFormat =
                    pVideoPortFormat->eColorFormat;
            }
            else
            {
                err = OMX_ErrorBadPortIndex;
                break;
            }
        }
        break;

    case OMX_IndexParameterStoreMediaData:
        {
            StoreMetaDataInBuffersParams *storeMetaData = (StoreMetaDataInBuffersParams *)ComponentParameterStructure;

            err =
                omx_base_component_ParameterSanityCheck (hComponent,
                    storeMetaData->nPortIndex,
                    storeMetaData,
                    sizeof
                    (StoreMetaDataInBuffersParams));
            if (err != OMX_ErrorNone)
            {
                DEBUG (DEB_LEV_ERR,
                    "%s (line %d): Parameter Sanity Check failed!\n", __func__,
                    __LINE__);
                break;
            }   
           
            omx_base_PortType *pPort = (omx_base_PortType *)omx_camera_source_component_Private->ports[storeMetaData->nPortIndex];
            if(PORTTYPE_IS_IMAGE(storeMetaData->nPortIndex)){
                ((omx_camera_source_component_image_PortType *)pPort)->bIsStoreMetaData = storeMetaData->bStoreMetaData;
            }else if(PORTTYPE_IS_VIDEO(storeMetaData->nPortIndex)){
                ((omx_camera_source_component_video_PortType *)pPort)->bIsStoreMetaData = storeMetaData->bStoreMetaData;
            }else{
            }
        }
        break;
    case OMX_ACT_IndexParamSensorSelect:
        {
            OMX_PARAM_SENSORSELECTTYPE *sensorType = (OMX_PARAM_SENSORSELECTTYPE *)ComponentParameterStructure;

            if ((err =
                    checkHeader (ComponentParameterStructure,
                        sizeof (OMX_PARAM_SENSORSELECTTYPE))) != OMX_ErrorNone)
            {
                DEBUG (DEB_LEV_ERR, "%s (line %d): Check header failed!\n",
                    __func__, __LINE__);
                break;
            }      
            DEBUG (DEB_LEV_SIMPLE_SEQ, "omx_camera_source_component_Private->state =%d!\n",omx_camera_source_component_Private->state);
            DEBUG (DEB_LEV_SIMPLE_SEQ, "sensorType->eSensor =%d!\n",sensorType->eSensor);
            if(omx_camera_source_component_Private->state == OMX_StateLoaded){
                if(sensorType->eSensor >= MAX_CAMERA_NUMS){
                    err = OMX_ErrorNoMore;
                    DEBUG (DEB_LEV_ERR, "OMX_ACT_IndexParamSensorSelect OMX_ErrorNoMore");
                    break;
                }
                omx_camera_source_component_Private->sSensorType.eSensor = sensorType->eSensor;
            }else{
                err = OMX_ErrorIncorrectStateOperation;
            }
        }
        break;

    case OMX_IndexParamCommonSensorMode:
        err = OMX_ErrorNone;
        break;

    default: /*Call the base component function */
        err =
            omx_base_component_SetParameter (hComponent, nParamIndex,
                ComponentParameterStructure);
        break;
    }

    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}

/** The GetConfig method for camera source component
 * @param hComponent input parameter, the handle of V4L2 camera component
 * @param nConfigIndex input parameter, the index of the structure to be filled
 * @param pComponentConfigStructure inout parameter, a pointer to the structure that receives configurations
 */
static OMX_ERRORTYPE
omx_camera_source_component_GetConfig (OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nConfigIndex,
    OMX_INOUT OMX_PTR
    pComponentConfigStructure)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE *openmaxStandComp;
    omx_camera_source_component_PrivateType
        * omx_camera_source_component_Private;
    OMX_CONFIG_BOOLEANTYPE *pCapturing;
    OMX_CONFIG_BOOLEANTYPE *pAutoPause;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    if (pComponentConfigStructure == NULL)
    {
        return OMX_ErrorBadParameter;
    }

    openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
    omx_camera_source_component_Private =
        (omx_camera_source_component_PrivateType *)
        openmaxStandComp->pComponentPrivate;

    DEBUG (DEB_LEV_SIMPLE_SEQ, "%s: Getting configuration %i\n", __func__,
        nConfigIndex);

    switch (nConfigIndex)
    {
    case OMX_IndexConfigCapturing:
        pCapturing = (OMX_CONFIG_BOOLEANTYPE *) pComponentConfigStructure;
        if ((err =
                checkHeader (pComponentConfigStructure,
                    sizeof (OMX_CONFIG_BOOLEANTYPE))) != OMX_ErrorNone)
        {
            DEBUG (DEB_LEV_ERR, "%s (line %d): Check header failed!\n",
                __func__, __LINE__);
            break;
        }
        pCapturing->bEnabled =
            omx_camera_source_component_Private->bCapturingNext;
        break;
    case OMX_IndexAutoPauseAfterCapture:
        pAutoPause = (OMX_CONFIG_BOOLEANTYPE *) pComponentConfigStructure;
        if ((err =
                checkHeader (pComponentConfigStructure,
                    sizeof (OMX_CONFIG_BOOLEANTYPE))) != OMX_ErrorNone)
        {
            DEBUG (DEB_LEV_ERR, "%s (line %d): Check header failed!\n",
                __func__, __LINE__);
            break;
        }
        pAutoPause->bEnabled = omx_camera_source_component_Private->bAutoPause;
        break;

    case OMX_ACT_IndexConfigCamCapabilities:
        {
            OMX_ACT_CAPTYPE *capType = ( OMX_ACT_CAPTYPE *)pComponentConfigStructure;

            if(capType->nSensorIndex>=MAX_CAMERA_NUMS)
            {
                err = OMX_ErrorNoMore;
                break;
            }
            err = getCamCapabilities((OMX_ACT_CAPTYPE *)capType, capType->nSensorIndex);
        }
        break;


    default:
        err =
            omx_base_component_GetConfig (hComponent, nConfigIndex,
                pComponentConfigStructure);
        break;
    }

    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}

/** The SetConfig method for camera source component
 * @param hComponent input parameter, the handle of V4L2 camera component
 * @param nConfigIndex input parameter, the index of the structure to be set
 * @param pComponentConfigStructure input parameter, a pointer to the configuration structure
 */
static OMX_ERRORTYPE
omx_camera_source_component_SetConfig (OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE nConfigIndex,
    OMX_IN OMX_PTR
    pComponentConfigStructure)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE *openmaxStandComp;
    omx_camera_source_component_PrivateType
        * omx_camera_source_component_Private;
    OMX_CONFIG_BOOLEANTYPE *pCapturing;
    OMX_CONFIG_PORTBOOLEANTYPE *pPortCapturing;
    OMX_CONFIG_BOOLEANTYPE *pAutoPause;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    if (pComponentConfigStructure == NULL)
    {
        return OMX_ErrorBadParameter;
    }

    openmaxStandComp = (OMX_COMPONENTTYPE *) hComponent;
    omx_camera_source_component_Private =
        (omx_camera_source_component_PrivateType *)
        openmaxStandComp->pComponentPrivate;

    DEBUG (DEB_LEV_SIMPLE_SEQ, "%s: Setting configuration %i\n", __func__,
        nConfigIndex);

    switch (nConfigIndex)
    {
    case OMX_IndexConfigCapturing:
        pCapturing = (OMX_CONFIG_BOOLEANTYPE *) pComponentConfigStructure;
        if ((err =
                checkHeader (pComponentConfigStructure,
                    sizeof (OMX_CONFIG_BOOLEANTYPE))) != OMX_ErrorNone)
        {
            DEBUG (DEB_LEV_ERR, "%s (line %d): Check header failed!\n",
                __func__, __LINE__);
            break;
        }
        err =
            camera_SetConfigCapturing (omx_camera_source_component_Private,
                pCapturing->bEnabled);
        break;
    case OMX_IndexConfigCommonPortCapturing:
        pPortCapturing = (OMX_CONFIG_PORTBOOLEANTYPE *) pComponentConfigStructure;
        if ((err =
                checkHeader (pComponentConfigStructure,
                    sizeof (OMX_CONFIG_PORTBOOLEANTYPE))) != OMX_ErrorNone)
        {
            DEBUG (DEB_LEV_ERR, "%s (line %d): Check header failed!\n",
                __func__, __LINE__);
            break;
        }
        err =
            camera_SetConfigCapturing (omx_camera_source_component_Private,
                pPortCapturing->bEnabled);
        break;
    case OMX_IndexAutoPauseAfterCapture:
        pAutoPause = (OMX_CONFIG_BOOLEANTYPE *) pComponentConfigStructure;
        if ((err =
                checkHeader (pComponentConfigStructure,
                    sizeof (OMX_CONFIG_BOOLEANTYPE))) != OMX_ErrorNone)
        {
            DEBUG (DEB_LEV_ERR, "%s (line %d): Check header failed!\n",
                __func__, __LINE__);
            break;
        }
        pthread_mutex_lock
            (&omx_camera_source_component_Private->setconfig_mutex);
        omx_camera_source_component_Private->bAutoPause = pAutoPause->bEnabled;
        pthread_mutex_unlock
            (&omx_camera_source_component_Private->setconfig_mutex);
        break;

    case OMX_IndexConfigCaptureMode:
        err = OMX_ErrorNone;
        break;
    default:
        err =
            omx_base_component_SetConfig (hComponent, nConfigIndex,
                pComponentConfigStructure);
        break;
    }

    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}

/** This is the central function for buffer processing.
 * It is executed in a separate thread.
 * @param param input parameter, a pointer to the OMX standard structure
 */
static void *
omx_camera_source_component_BufferMgmtFunction(void *param)
{
    OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *) param;
    omx_camera_source_component_PrivateType
        * omx_camera_source_component_Private =
        (omx_camera_source_component_PrivateType *) openmaxStandComp-> pComponentPrivate;
    omx_camera_source_component_video_PortType
        * pPreviewPort =
        (omx_camera_source_component_video_PortType *) omx_camera_source_component_Private->ports[OMX_CAMPORT_INDEX_VF];
    omx_camera_source_component_image_PortType
        * pCapturePort =
        (omx_camera_source_component_image_PortType *) omx_camera_source_component_Private->ports[OMX_CAMPORT_INDEX_CP];
    tsem_t *pPreviewBufSem = pPreviewPort->pBufferSem;
    tsem_t *pCaptureBufSem = pCapturePort->pBufferSem;
    OMX_BUFFERHEADERTYPE *pPreviewBuffer = NULL;
    OMX_BUFFERHEADERTYPE *pCaptureBuffer = NULL;
    OMX_BUFFERHEADERTYPE *pThumbnailBuffer = NULL;

    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    while (omx_camera_source_component_Private->state == OMX_StateIdle
        || omx_camera_source_component_Private->state == OMX_StateExecuting
        || omx_camera_source_component_Private->state == OMX_StatePause
        || omx_camera_source_component_Private->transientState
        == OMX_TransStateLoadedToIdle)
    {

        /*Wait till the ports are being flushed */
        pthread_mutex_lock(&omx_camera_source_component_Private->flush_mutex);
        while (PORT_IS_BEING_FLUSHED(pPreviewPort) || PORT_IS_BEING_FLUSHED(
                pCapturePort))
        {
            pthread_mutex_unlock(
                &omx_camera_source_component_Private-> flush_mutex);

            DEBUG(
                DEB_LEV_FULL_SEQ,
                "In %s 1 signalling flush all cond PrevewSemVal=%d,CaptureSemval=%d\n",
                __func__, pPreviewBufSem->semval, pCaptureBufSem->semval);
            DEBUG(
                DEB_LEV_FULL_SEQ,
                "In %s 1 signalling flush all cond pPreviewBuffer=0x%lX,pCaptureBuffer=0x%lX, \n",
                __func__, (OMX_U32) pPreviewBuffer,
                (OMX_U32) pCaptureBuffer);

            if (pPreviewBuffer != NULL && PORT_IS_BEING_FLUSHED(pPreviewPort))
            {
                pPreviewPort->ReturnBufferFunction(
                    (omx_base_PortType *) pPreviewPort, pPreviewBuffer);
                pPreviewBuffer = NULL;
                DEBUG(DEB_LEV_FULL_SEQ,
                    "Ports are flushing,so returning Preview buffer\n");
            }

            if (pCaptureBuffer != NULL && PORT_IS_BEING_FLUSHED(pCapturePort))
            {
                pCapturePort->ReturnBufferFunction(
                    (omx_base_PortType *) pCapturePort, pCaptureBuffer);
                pCaptureBuffer = NULL;
                DEBUG(DEB_LEV_FULL_SEQ,
                    "Ports are flushing,so returning Capture buffer\n");
            }

            DEBUG(
                DEB_LEV_FULL_SEQ,
                "In %s 2 signalling flush all cond PrevewSemVal=%d,CaptureSemval=%d\n",
                __func__, pPreviewBufSem->semval, pCaptureBufSem->semval);
            DEBUG(
                DEB_LEV_FULL_SEQ,
                "In %s 2 signalling flush all cond pPreviewBuffer=0x%lX,pCaptureBuffer=0x%lX, \n",
                __func__, (OMX_U32) pPreviewBuffer,
                (OMX_U32) pCaptureBuffer);

            tsem_up(omx_camera_source_component_Private->flush_all_condition);
            tsem_down(omx_camera_source_component_Private->flush_condition);
            pthread_mutex_lock(
                &omx_camera_source_component_Private-> flush_mutex);
        }
        pthread_mutex_unlock(&omx_camera_source_component_Private-> flush_mutex);

        pthread_mutex_lock(
            &omx_camera_source_component_Private-> setconfig_mutex);
        if (!omx_camera_source_component_Private->bCapturing
            && omx_camera_source_component_Private->bCapturingNext)
        {
        }
        else if (omx_camera_source_component_Private->bCapturing
            && !omx_camera_source_component_Private->bCapturingNext)
        {
            omx_camera_source_component_Private->nCapturedCount = 0;
        }
        omx_camera_source_component_Private->bCapturing
            = omx_camera_source_component_Private->bCapturingNext;
        pthread_mutex_unlock(
            &omx_camera_source_component_Private-> setconfig_mutex);

        if (omx_camera_source_component_Private->state == OMX_StatePause
            && !(PORT_IS_BEING_FLUSHED(pPreviewPort)
                || PORT_IS_BEING_FLUSHED(pCapturePort)))
        {
            /*Waiting at paused state */
            DEBUG(DEB_LEV_FULL_SEQ, "In %s: wait at State %d\n", __func__,
                omx_camera_source_component_Private->state);
            tsem_wait(omx_camera_source_component_Private->bStateSem);
        }

        pthread_mutex_lock(
            &omx_camera_source_component_Private-> idle_state_mutex);
        if (omx_camera_source_component_Private->state == OMX_StateIdle
            && !(PORT_IS_BEING_FLUSHED(pPreviewPort)
                || PORT_IS_BEING_FLUSHED(pCapturePort)))
        {
            /*Waiting at idle state */
            DEBUG(DEB_LEV_FULL_SEQ, "In %s: wait at State %d\n", __func__,
                omx_camera_source_component_Private->state);
            omx_camera_source_component_Private->bWaitingOnIdle = OMX_TRUE;
            pthread_cond_signal(
                &omx_camera_source_component_Private-> idle_process_condition);
            pthread_cond_wait(
                &omx_camera_source_component_Private-> idle_wait_condition,
                &omx_camera_source_component_Private-> idle_state_mutex);
            if (omx_camera_source_component_Private->transientState
                == OMX_TransStateIdleToLoaded)
            {
                DEBUG(DEB_LEV_SIMPLE_SEQ,
                    "In %s Buffer Management Thread is exiting\n", __func__);
                pthread_mutex_unlock(
                    &omx_camera_source_component_Private-> idle_state_mutex);
                break;
            }
            if(omx_camera_source_component_Private->state == OMX_StateExecuting){
                struct timeval now;

                omx_camera_source_component_Private->bIsFirstFrame = OMX_TRUE;

                gettimeofday (&now, NULL);
                omx_camera_source_component_Private->nRefWallTime =
                    (OMX_TICKS) (now.tv_sec * 1000000 + now.tv_usec);
            }
        }
        omx_camera_source_component_Private->bWaitingOnIdle = OMX_FALSE;
        pthread_mutex_unlock(
            &omx_camera_source_component_Private-> idle_state_mutex);

        /* After cemera does start, capture video data from camera */
        camera_HandleThreadBufferCapture(omx_camera_source_component_Private);

    }

    DEBUG(DEB_LEV_FUNCTION_NAME, "Exiting Buffer Management Thread: %s\n",
        __func__);
    return NULL;
}

/* Buffer capture routine for the buffer management thread */
static OMX_ERRORTYPE
camera_HandleThreadBufferCapture (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private)
{
    OMX_U32 nCurTimeInMilliSec;
    OMX_S32 nTimeToWaitInMilliSec;
    struct timeval now;
    struct timespec sleepTime;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    struct v4l2_buffer buf;

    omx_base_PortType
        * pPreviewPort =
        (omx_base_PortType *) omx_camera_source_component_Private->ports[OMX_CAMPORT_INDEX_VF];
    omx_base_PortType
        * pCapturePort =
        (omx_base_PortType *) omx_camera_source_component_Private->ports[OMX_CAMPORT_INDEX_CP];

    omx_base_PortType
        *pPort = NULL;
    CLEAR (buf);

    /*
    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    DEBUG (DEB_LEV_FULL_SEQ,
        "%s: mapbuf queue index [last, wait, capture] = [%ld, %ld, %ld]\n",
        __func__,
        omx_camera_source_component_Private->sMapbufQueue.nLastBufIndex,
        omx_camera_source_component_Private->sMapbufQueue.nNextWaitIndex,
        omx_camera_source_component_Private->sMapbufQueue.nNextCaptureIndex);

    DEBUG (DEB_LEV_FULL_SEQ,
        "%s: mapbuf queue count [captured, total] = [%ld, %ld]\n", __func__,
        omx_camera_source_component_Private->sMapbufQueue.nBufCountCaptured,
        omx_camera_source_component_Private->sMapbufQueue.nBufCountTotal);
        */
    if(omx_camera_source_component_Private->state != OMX_StateExecuting){
        

        err = OMX_ErrorNone;
        goto EXIT;
    }
    //image state
    if(PORT_IS_ENABLED(pCapturePort)
        &&(omx_camera_source_component_Private->bCapturing == OMX_TRUE)){
        if(omx_camera_source_component_Private->bDeviceStarted == OMX_FALSE){
            if ((err = camera_StartCameraDevice(
                        omx_camera_source_component_Private,OMX_CAMPORT_INDEX_CP)) != OMX_ErrorNone)
            {
                err = OMX_ErrorNone;
                goto EXIT;
            }
        }else if(OMX_MAPBUFQUEUE_PORTINDEX(omx_camera_source_component_Private->sMapbufQueue) != OMX_CAMPORT_INDEX_CP){
            //switch device
            if ((err = camera_StopCameraDevice(
                        omx_camera_source_component_Private, OMX_TRUE)) != OMX_ErrorNone)
            {
                err = OMX_ErrorNone;
                goto EXIT;
            }

            if ((err = camera_StartCameraDevice(
                        omx_camera_source_component_Private,OMX_CAMPORT_INDEX_CP)) != OMX_ErrorNone)
            {
                err = OMX_ErrorNone;
                goto EXIT;
            }
        }
    }else if(PORT_IS_ENABLED(pPreviewPort)){
        if(omx_camera_source_component_Private->bDeviceStarted == OMX_FALSE){
            if ((err = camera_StartCameraDevice(
                        omx_camera_source_component_Private,OMX_CAMPORT_INDEX_VF)) != OMX_ErrorNone)
            {
                err = OMX_ErrorNone;
                goto EXIT;
            }
        }else if(OMX_MAPBUFQUEUE_PORTINDEX(omx_camera_source_component_Private->sMapbufQueue) != OMX_CAMPORT_INDEX_VF){

            //switch device
            if ((err = camera_StopCameraDevice(
                        omx_camera_source_component_Private, OMX_TRUE)) != OMX_ErrorNone)
            {
                err = OMX_ErrorNone;
                goto EXIT;
            }

            if ((err = camera_StartCameraDevice(
                        omx_camera_source_component_Private,OMX_CAMPORT_INDEX_VF)) != OMX_ErrorNone)
            {
                err = OMX_ErrorNone;
                goto EXIT;
            }
        }
    }else{
        err = OMX_ErrorNone;
        goto EXIT;
    }
    /* Wait to sync buffer */
    if (OMX_MAPBUFQUEUE_HASBUFWAITTOCAPTURE
        (omx_camera_source_component_Private->sMapbufQueue))

    {   

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        buf.memory = V4L2_MEMORY_USERPTR;

        if (-1 ==
            xioctl (omx_camera_source_component_Private->fdCam, VIDIOC_DQBUF,
                &buf))
        {
            switch (errno)
            {
            case EAGAIN:
                err =  OMX_ErrorHardware;;

                goto NEXT_QBUF;
            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */

            default:
                DEBUG (DEB_LEV_SIMPLE_SEQ, " %s error VIDIOC_DQBUF\n", __func__);
                err = OMX_ErrorHardware;;
                goto EXIT;
            }
        }

        /* Generate time stamp for the new captured buffer */
        if ((err =
                camera_GenerateTimeStamp (omx_camera_source_component_Private)) !=
            OMX_ErrorNone)
        {
            DEBUG (DEB_LEV_ERR, "%s: <ERROR> -- Generate time stamp failed!\n",
                __func__);
            goto EXIT;
        }

        OMX_MAPBUFQUEUE_ADDCAPTUREDBUF
            (omx_camera_source_component_Private->sMapbufQueue);
    }



    /* Try to send buffers */
    if (OMX_MAPBUFQUEUE_HASBUFCAPTURED
        (omx_camera_source_component_Private->sMapbufQueue))
    {
        if(omx_camera_source_component_Private->bFirstFrame)
        {
            omx_camera_source_component_Private->bFirstFrame = OMX_FALSE;
        }
        camera_SendCapturedBuffers (omx_camera_source_component_Private);
    }
    if(omx_camera_source_component_Private->bFirstFrame)
    {
        nTimeToWaitInMilliSec = 0;
        omx_camera_source_component_Private->bFirstFrame = OMX_FALSE;
    }
    else
    {
        /* Calculate waiting time */
        gettimeofday (&now, NULL);
        nCurTimeInMilliSec =
            ((OMX_U32) now.tv_sec) * 1000 + ((OMX_U32) now.tv_usec) / 1000;
        nTimeToWaitInMilliSec =
            (OMX_S32) (omx_camera_source_component_Private->nFrameIntervalInMilliSec -
                (nCurTimeInMilliSec -
                 omx_camera_source_component_Private->
                 nLastCaptureTimeInMilliSec));
        if(nTimeToWaitInMilliSec <=0)
        {
            nTimeToWaitInMilliSec = 20;
        }

        DEBUG (DEB_LEV_FULL_SEQ,
            "%s: [current time, last capture time, time to wait]=[%lu, %lu, %ld]\n",
            __func__, nCurTimeInMilliSec,
            omx_camera_source_component_Private->nLastCaptureTimeInMilliSec,
            nTimeToWaitInMilliSec);
    }
    /* Wait some time according to frame rate */
    if (nTimeToWaitInMilliSec > 0)
    {
        sleepTime.tv_sec = nTimeToWaitInMilliSec / 1000;
        sleepTime.tv_nsec = (nTimeToWaitInMilliSec % 1000) * 1000000;
        DEBUG (DEB_LEV_FULL_SEQ, "%s: Actually wait for %ld msec\n", __func__,
            nTimeToWaitInMilliSec);
        nanosleep (&sleepTime, NULL);
    }

    /* record last capture time */
    gettimeofday (&now, NULL);
    omx_camera_source_component_Private->nLastCaptureTimeInMilliSec =
        ((OMX_U32) now.tv_sec) * 1000 + ((OMX_U32) now.tv_usec) / 1000;

    /* Start to capture the next buffer */

    OMX_BUFFERHEADERTYPE *pBufHeader = NULL;
NEXT_QBUF:
    if(OMX_MAPBUFQUEUE_PORTINDEX(omx_camera_source_component_Private->sMapbufQueue) == OMX_CAMPORT_INDEX_CP){
        pPort = pCapturePort;
    }else if(OMX_MAPBUFQUEUE_PORTINDEX(omx_camera_source_component_Private->sMapbufQueue) == OMX_CAMPORT_INDEX_VF){
        pPort = pPreviewPort;
    }else{
        goto EXIT;
    }
    
    while (pPort->pBufferSem->semval > 0)                                                         
    {                                                                                         
        /* Dequeue a buffer from buffer queue */                                              
        tsem_down (pPort->pBufferSem);                                                        
        pBufHeader = dequeue (pPort->pBufferQueue);                                           
        if (pBufHeader == NULL)                                                              
        {                                                                                    
            break;                                                                      
        } 

        struct buffer *buffer = NULL;
        int index = 0;
        DEBUG (DEB_LEV_ERR, " pBufHeader = %x\n", (unsigned int)pBufHeader);
        DEBUG (DEB_LEV_ERR, " pBufHeader->pbuffer = %x\n", (unsigned int)pBufHeader->pBuffer);
        DEBUG (DEB_LEV_ERR, " pBufHeader->pbuffer->handle = %x\n", (unsigned int)((video_metadata_t *)pBufHeader->pBuffer)->handle);
        DEBUG (DEB_LEV_ERR, " index =  %d error VIDIOC_QBUF\n", (int)index);
        OMX_MAPBUFQUEUE_GETBUF_BYHEADER(omx_camera_source_component_Private->sMapbufQueue,((video_metadata_t *)pBufHeader->pBuffer)->handle, buffer,index);
        if(buffer == NULL){
            continue;
        }
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = index;
        buf.length = omx_camera_source_component_Private->sMapbufQueue.buffers[index].length;
        buf.m.userptr =
            (unsigned long) buffer->phys_addr;

        if (-1 == xioctl (omx_camera_source_component_Private->fdCam, VIDIOC_QBUF,
            &buf))
        {
            DEBUG (DEB_LEV_ERR, " %s %d error VIDIOC_QBUF\n", __func__, __LINE__);
            err = OMX_ErrorHardware;
        }
        
        OMX_MAPBUFQUEUE_ENQUEUE(omx_camera_source_component_Private->sMapbufQueue, pBufHeader);
    }
   

EXIT:
    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}

/* Generate time stamp for the new captured buffer */
static OMX_ERRORTYPE
camera_GenerateTimeStamp (OMX_IN omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_U32 nBufferIndex;
    struct timeval now;
    OMX_TICKS nCurrentWallTime;
    OMX_TICKS nTimeStamp;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    nBufferIndex =
        OMX_MAPBUFQUEUE_GETNEXTWAIT
        (omx_camera_source_component_Private->sMapbufQueue);

    gettimeofday (&now, NULL);
    nCurrentWallTime = (OMX_TICKS) (now.tv_sec * 1000000 + now.tv_usec);

    /* To protect nRefWallTime */
    nTimeStamp =
        nCurrentWallTime - omx_camera_source_component_Private->nRefWallTime;

    OMX_MAPBUFQUEUE_SETTIMESTAMP
        (omx_camera_source_component_Private->sMapbufQueue, nBufferIndex,
         nTimeStamp);

    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}                                                                                                     

/* Try to send captured buffers in mapbuf queue to each port.                                                    
 * Note: In this function, multiple buffers may be sent.
 */
static OMX_ERRORTYPE
camera_SendCapturedBuffers (OMX_IN omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private)
{
    OMX_U32 nBufferCountCur = 0;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_S32 nPortIndex;
    int bufIndex;

    nPortIndex = OMX_MAPBUFQUEUE_PORTINDEX(omx_camera_source_component_Private->sMapbufQueue);
    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    omx_base_PortType *port;                                                 
    port = (omx_base_PortType *) omx_camera_source_component_Private->ports[nPortIndex];                     
    if (PORT_IS_ENABLED (port) && (omx_camera_source_component_Private->bCapturing                  
            || OMX_CAMPORT_INDEX_CP != nPortIndex))                                                 
    {                                                                                               
        nBufferCountCur = OMX_MAPBUFQUEUE_GETBUFCOUNTCAPTURED(omx_camera_source_component_Private->sMapbufQueue);  
        bufIndex =                                        
            OMX_MAPBUFQUEUE_GETLASTBUFFER (omx_camera_source_component_Private->sMapbufQueue); 
        while ( (nBufferCountCur > 0))    
        {                                                                                                        
            camera_ProcessPortOneBuffer (omx_camera_source_component_Private, (OMX_U32) nPortIndex, bufIndex);   
            bufIndex = OMX_MAPBUFQUEUE_GETNEXTINDEX                                              
                (omx_camera_source_component_Private->sMapbufQueue, bufIndex);                   
            nBufferCountCur--;               
        }                                                                                                       
    }           

    err =
        camera_UpdateCapturedBufferQueue (omx_camera_source_component_Private);

    DEBUG (DEB_LEV_FULL_SEQ,
        "%s: After returning buffers, mapbuf queue index [last, wait, capture] = [%ld, %ld, %ld]\n",
        __func__,
        omx_camera_source_component_Private->sMapbufQueue.nLastBufIndex,
        omx_camera_source_component_Private->sMapbufQueue.nNextWaitIndex,
        omx_camera_source_component_Private->sMapbufQueue.nNextCaptureIndex);

    DEBUG (DEB_LEV_FULL_SEQ,
        "%s: After returning buffers,mapbuf queue count [captured, total] = [%ld, %ld]\n", __func__,
        omx_camera_source_component_Private->sMapbufQueue.nBufCountCaptured,
        omx_camera_source_component_Private->sMapbufQueue.nBufCountTotal);

    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}


/* Update captured buffer queue in mapbuf queue */
static OMX_ERRORTYPE
camera_UpdateCapturedBufferQueue (OMX_IN
    omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);
    OMX_U32 nPortIndex = OMX_MAPBUFQUEUE_PORTINDEX(omx_camera_source_component_Private->sMapbufQueue);

    while (OMX_MAPBUFQUEUE_HASBUFCAPTURED
        (omx_camera_source_component_Private->sMapbufQueue))
    {

        OMX_MAPBUFQUEUE_DEQUEUE
            (omx_camera_source_component_Private->sMapbufQueue);
    }

    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}

/* Process one buffer on the specified port */
static OMX_ERRORTYPE
camera_ProcessPortOneBuffer (OMX_IN omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private,
    OMX_IN OMX_U32 nPortIndex, int bufIndex)
{
    OMX_BUFFERHEADERTYPE *pBufHeader = NULL;
    OMX_BOOL bStrideAlign = OMX_FALSE;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    
    omx_base_PortType *port = (omx_base_PortType *) omx_camera_source_component_Private->ports[nPortIndex];     
    unsigned char *vaddr;                                                                     
    pBufHeader =  (OMX_BUFFERHEADERTYPE*)OMX_MAPBUFQUEUE_GETBUFADDR (               
                    omx_camera_source_component_Private->sMapbufQueue, bufIndex);                                                                                         \
    if (OMX_CAMPORT_INDEX_CP == nPortIndex)                                               
    {                                                                                     
        if ((err = camera_HandleStillImageCapture (omx_camera_source_component_Private)) != OMX_ErrorNone)       
        {                                                                                 
            goto EXIT;                                                                    
        }                                                                                 
    }                                                                                     
    else if (OMX_CAMPORT_INDEX_VF == nPortIndex)                                          
    {                                                                                     
        if ((err = camera_AddTimeStamp (omx_camera_source_component_Private,              
                    pBufHeader, bufIndex)) != OMX_ErrorNone)                                        
        {                                                                                 
            goto EXIT;                                                                    
        }                                                                                 
    }                                                                                     
    /* Return buffer */                                                                   
    pBufHeader->nFilledLen =  port->sPortParam.nBufferSize;                                  
    port->ReturnBufferFunction ((omx_base_PortType *) port, pBufHeader); 
    

EXIT:
    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}
                                                                                




/* Add time stamp to a buffer header */
static OMX_ERRORTYPE
camera_AddTimeStamp (OMX_IN omx_camera_source_component_PrivateType *
    omx_camera_source_component_Private,
    OMX_IN OMX_BUFFERHEADERTYPE * pBufHeader,  int bufIndex)
{
    omx_camera_source_component_video_PortType *pPreviewPort =
        (omx_camera_source_component_video_PortType *)
        omx_camera_source_component_Private->ports[OMX_CAMPORT_INDEX_VF];
    OMX_ERRORTYPE err = OMX_ErrorNone;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    if (omx_camera_source_component_Private->bIsFirstFrame)
    {
        pBufHeader->nFlags = OMX_BUFFERFLAG_STARTTIME;
        omx_camera_source_component_Private->bIsFirstFrame = OMX_FALSE;
    }
    else
    {
        pBufHeader->nFlags = 0;
    }

    pBufHeader->nTimeStamp =
        OMX_MAPBUFQUEUE_GETTIMESTAMP
        (omx_camera_source_component_Private->sMapbufQueue,
         bufIndex);

    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);
    return err;
}

/* Handle still image capture use case */
static OMX_ERRORTYPE
camera_HandleStillImageCapture (OMX_IN omx_camera_source_component_PrivateType
    * omx_camera_source_component_Private)
{
    OMX_ERRORTYPE err = OMX_ErrorNone;

    DEBUG (DEB_LEV_FUNCTION_NAME, "In %s for camera component\n", __func__);

    pthread_mutex_lock (&omx_camera_source_component_Private->setconfig_mutex);
    

    if (omx_camera_source_component_Private->bAutoPause)
    {
        omx_camera_source_component_Private->bCapturingNext = OMX_FALSE;
        /* In autopause mode, command camera component to pause state */
        if ((err =
                omx_camera_source_component_DoStateSet
                (omx_camera_source_component_Private->openmaxStandComp,
                 (OMX_U32) OMX_StatePause)) != OMX_ErrorNone)
        {
            goto EXIT;
        }
    }

EXIT:
    pthread_mutex_unlock
        (&omx_camera_source_component_Private->setconfig_mutex);
    DEBUG (DEB_LEV_FUNCTION_NAME,
        "Out of %s for camera component, return code: 0x%X\n", __func__,
        err);

    return err;
}

static int camera_init_userptr(
    omx_camera_source_component_PrivateType * omx_camera_source_component_Private, OMX_U32 portIndex, OMX_U32 bufCount)
{
    struct v4l2_requestbuffers req;
    OMX_U32 i;
    unsigned int page_size;

    page_size = getpagesize();

    CLEAR (req);

    req.count = bufCount;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(omx_camera_source_component_Private->fdCam,
            VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            DEBUG(DEB_LEV_ERR, "%s does not support "
                "memory mapping\n", V4L2DEV_FILENAME);
            return OMX_ErrorHardware;
        }
        else
        {
            DEBUG(DEB_LEV_ERR, "%s error %d, %s\n", "VIDIOC_REQBUFS", errno,
                strerror(errno));
            return OMX_ErrorHardware;
        }
    }

    if (req.count < 2)
    {
        DEBUG(DEB_LEV_ERR, "Insufficient buffer memory on %s\n",
            V4L2DEV_FILENAME);
        return OMX_ErrorHardware;
    }

    omx_camera_source_component_Private->sMapbufQueue.nFrame = req.count;

    DEBUG(DEB_LEV_SIMPLE_SEQ, "%d, bufcount=%d\n",__LINE__,req.count);

    for (i = 0; i < req.count; ++i)
    {
        struct v4l2_buffer buf;

        CLEAR (buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_USERPTR;
        buf.index = i;


        if (-1 == xioctl(omx_camera_source_component_Private->fdCam,
                VIDIOC_QUERYBUF, &buf))
        {
            DEBUG(DEB_LEV_ERR, "%s error %d, %s\n", "VIDIOC_QUERYBUF", errno,
                strerror(errno));
            return OMX_ErrorHardware;
        }

        omx_camera_source_component_Private->sMapbufQueue.buffers[i].length
            = buf.length;
        
            omx_base_PortType *pPort = (omx_base_PortType *)omx_camera_source_component_Private->ports[portIndex];
            buffer_handle_t handle;
            OMX_U8 *phys_addr;
            OMX_U8 *virt_addr;
            while( !(pPort->bBufferStateAllocated[i]&(BUFFER_ASSIGNED|HEADER_ALLOCATED))){
                struct timespec sleepTime;
                sleepTime.tv_sec = 100 / 1000;
                sleepTime.tv_nsec = (100 % 1000) * 1000000;
                DEBUG (DEB_LEV_ERR, "%s: Actually wait for %ld msec\n", __func__,
                    100L);
                nanosleep (&sleepTime, NULL);
            }


            handle = (buffer_handle_t)((video_metadata_t *)pPort->pInternalBufferStorage[i]->pBuffer)->handle;
            virt_addr = bufferHandleLock(handle,0,0);
            phys_addr = getPhys(handle);

            omx_camera_source_component_Private->sMapbufQueue.buffers[i].
                pCapAddrStart = virt_addr;
            omx_camera_source_component_Private->sMapbufQueue.buffers[i].fd = -1;
            omx_camera_source_component_Private->sMapbufQueue.buffers[i].handle =
                (void *)handle;
            omx_camera_source_component_Private->sMapbufQueue.buffers[i].
                phys_addr = phys_addr;



        if (!omx_camera_source_component_Private->sMapbufQueue.buffers[i]. pCapAddrStart)
        {
            DEBUG(DEB_LEV_ERR, "%s error %d, %s\n", "userptr memalign", errno,
                strerror(errno));
            return OMX_ErrorHardware;
        }
        DEBUG(
            DEB_LEV_SIMPLE_SEQ,
            "i=%d,handle = %x, addr=%x,paddr=%x length=%d\n",
            (int) i,
            (int)omx_camera_source_component_Private->sMapbufQueue.buffers[i].handle,
            (int) omx_camera_source_component_Private->sMapbufQueue. buffers[i].pCapAddrStart,
            (int) omx_camera_source_component_Private->sMapbufQueue.buffers[i].phys_addr,
            (int) omx_camera_source_component_Private->sMapbufQueue. buffers[i].length);

    }


    return OMX_ErrorNone;
}
