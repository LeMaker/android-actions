/**
  src/base/omx_base_video_port.h

  Base Video Port class for OpenMAX ports to be used in derived components.

  Copyright (C) 2007-2009 STMicroelectronics
  Copyright (C) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

*/

#ifndef __OMX_BASE_CAMERA_VIDEO_PORT_H__
#define __OMX_BASE_CAMERA_VIDEO_PORT_H__

#ifndef _OPENMAX_V1_2_
#include "ACT_OMX_Common_V1_2__V1_1.h"
#else
#include <OMX_Types.h>
#include "OMX_IVCommon.h"
#include "OMX_Image.h"
#endif


#include "ACT_OMX_Index.h"
#include "ACT_OMX_IVCommon.h"

#include "omx_base_video_port.h"
#include "omx_camera.h"
#include "omx_classmagic.h"

#define DEFAULT_WIDTH   640
#define DEFAULT_HEIGHT  480
#define DEFAULT_FPS     25
#define DEFAULT_BITRATE 0

typedef struct
{
    int index;//is any param changed?
    int buffersize;//current width and height
    unsigned long phyAddr;
    unsigned char *VirAddr;
    unsigned long Stat_phyAddr;
    unsigned char *Stat_VirAddr;
    OMX_BOOL bAllocByComp;
    OMX_BOOL bUseBufFlag;
    //  OMX_BOOL bAllocPhyFlag;
    OMX_BOOL bAllocStatFlag;
    unsigned long phyAddr_of_resize;
    unsigned char *VirAddr_of_resize;
    unsigned int bytes_of_resize;
    int nVirSize;
    cam_stat_addr_t mStatInfo;
    void *handle;
} OMX_CONFIG_PARAM_ACTEXT;

typedef struct
{
    OMX_BUFFERHEADERTYPE *pBuffHead;
    OMX_CONFIG_PARAM_ACTEXT pConfigParam;
} OMX_BUFFERHEADERTYPE_ACTEXT;

DERIVEDCLASS(omx_base_camera_video_PortType , omx_base_video_PortType)
#define omx_base_camera_video_PortType_FIELDS omx_base_video_PortType_FIELDS \
  /** @param sVideoParam Domain specific (video) OpenMAX port parameter */ \
    OMX_BUFFERHEADERTYPE_ACTEXT *pBufferHeadAct; \
    OMX_IMAGE_PARAM_PORTFORMATTYPE sImageParam; \
    OMX_BOOL bStoreMediadata;\
    OMX_S32  nSensorSelect;\
    OMX_PARAM_SENSORMODETYPE *pSensorMode;\
    /** @此类参数 均为实时操作  */\
    OMX_CONFIG_FLICKERREJECTIONTYPE  *pFlicktype;\
    OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *pNs_level;\
    OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE *pSharp_level;\
    OMX_ACT_CONFIG_FOCUSDISTANCETYPE *pAF_Dis;\
    OMX_ACT_CONFIG_BLITCOMPENSATIONTYPE   *pBlitComp;\
    OMX_IMAGE_PARAM_FLASHCONTROLTYPE *pFlashType;\
    OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE *pFocusType;\
    OMX_CONFIG_COLORCONVERSIONTYPE *pColorFix;\
    OMX_CONFIG_COLORENHANCEMENTTYPE *pColorEft;\
    OMX_CONFIG_IMAGEFILTERTYPE  *pImageFilter;\
    OMX_CONFIG_MIRRORTYPE *pImageMirror;\
    OMX_CONFIG_SCALEFACTORTYPE *pOpticZoomType;\
    OMX_CONFIG_WHITEBALCONTROLTYPE *pWBType;\
    OMX_CONFIG_EXPOSURECONTROLTYPE *pExpType;\
    OMX_CONFIG_CONTRASTTYPE *pContrast;\
    OMX_CONFIG_BRIGHTNESSTYPE *pBright;\
    OMX_CONFIG_GAMMATYPE *pGamma;\
    OMX_CONFIG_SATURATIONTYPE *pSat;\
    OMX_CONFIG_EXPOSUREVALUETYPE *pExpVal;\
    OMX_CONFIG_FOCUSREGIONTYPE *pAFRegionL;\
    OMX_PARAM_FOCUSSTATUSTYPE *pAFStatusL;\
    OMX_CONFIG_FOCUSREGIONCONTROLTYPE *pAFRegion;\
    OMX_CONFIG_FOCUSREGIONSTATUSTYPE *pAFStatus;\
    OMX_BOOL isCapture;\
    OMX_BOOL bCapturePause;\
    OMX_CONFIG_CAPTUREMODETYPE *pCapMode;\
    OMX_CONFIG_EXTCAPTUREMODETYPE *pCapExtMode;\
    OMX_ACT_CAPTYPE *pCapInfo;\
    OMX_CAMERATYPE *omx_camera;\
    OMX_BOOL bCopy;\
    int bStreamOn;\
    int nDropFrames;\
    /* current sensor type */ \
    int nSensorType;\
    int bAWB_Lock;\
    int bAE_Lock;\
    int bAF_Lock;\
    /* Device ISP Process Handle */ \
    void *device_handle;\
    /* Device ISP Module Lib Handle */ \
    void *dMoudleIsp;\
    /* Camera Parameters save */ \
    unsigned int config_idx;\
    queue_t *queue_dq;\
    OMX_ACT_CONFIG_AGCVALUE *act_agc;\
    OMX_ACT_CONFIG_FlashStrobeParams *act_flashstrobe;\
    int bHdr_Enable;\
    int bResizeEnable;\
    int nInputW;\
    int nInputH;\
    int bMJPEG_Enable;\
    OMX_CONFIG_PARAM_ACTEXT imx_buf;\
    OMX_CONFIG_PARAM_ACTEXT pHDR_Buf[4];\
    int width_hdr;\
    int height_hdr;\
    void* (*openHDRI)(void);\
    int (*camera_direct_base_Destructor)(OMX_CAMERATYPE *omx_camera);\
    int (*camera_isp_base_Destructor)(OMX_CAMERATYPE *omx_camera);\
    OMX_ACT_CONFIG_HDR_EVParams *pHdrParam;
ENDCLASS(omx_base_camera_video_PortType)

/**
  * @brief The base contructor for the generic OpenMAX ST Video port
  *
  * This function is executed by the component that uses a port.
  * The parameter contains the info about the component.
  * It takes care of constructing the instance of the port and
  * every object needed by the base port.
  *
  * @param openmaxStandComp pointer to the Handle of the component
  * @param openmaxStandPort the ST port to be initialized
  * @param nPortIndex Index of the port to be constructed
  * @param isInput specifices if the port is an input or an output
  *
  * @return OMX_ErrorInsufficientResources if a memory allocation fails
  */
OMX_ERRORTYPE base_camera_video_port_Constructor(
    OMX_COMPONENTTYPE *openmaxStandComp,
    omx_base_PortType **openmaxStandPort,
    OMX_U32 nPortIndex,
    OMX_BOOL isInput);

OMX_ERRORTYPE camera_video_port_UseBuffer(
    omx_base_PortType *openmaxStandPort,
    OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate,
    OMX_U32 nSizeBytes,
    OMX_U8 *pBuffer);

OMX_ERRORTYPE camera_video_port_AllocateBuffer(
    omx_base_PortType *openmaxStandPort,
    OMX_BUFFERHEADERTYPE **pBuffer,
    OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate,
    OMX_U32 nSizeBytes);

OMX_ERRORTYPE camera_video_port_FreeBuffer(
    omx_base_PortType *openmaxStandPort,
    OMX_U32 nPortIndex,
    OMX_BUFFERHEADERTYPE *pBuffer);


/**
  * @brief The base video port destructor for the generic OpenMAX ST Video port
  *
  * This function is executed by the component that uses a port.
  * The parameter contains the info about the port.
  * It takes care of destructing the instance of the port
  *
  * @param openmaxStandPort the ST port to be destructed
  *
  * @return OMX_ErrorNone
  */


OMX_ERRORTYPE camera_video_port_Destructor(
    omx_base_PortType *openmaxStandPort);

int get_base_bufferidx(OMX_BUFFERHEADERTYPE_ACTEXT *pBufferHeadAct, OMX_BUFFERHEADERTYPE *pBufferTarget, int bufferNum);

#endif
