/**
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
#ifndef __OMX_VIDEOENC_PORT_H__
#define __OMX_VIDEOENC_PORT_H__

#include "omx_base_component.h"
#include "omx_base_filter.h"
#include "omx_classmagic.h"
#include "omx_base_video_port.h"
#include "vce_act_ext.h"
#include "ACT_OMX_IVCommon.h"
#include "Actbuffer.h"
#include "buffer_mng.h"
#include "vce_cfg.h"
#include "log.h"

#define DEFAULT_FRAMERATE_AVC 15
#define MAX_FRAMERATE_AVC 60
#define MIN_FRAMERATE_AVC 1

#ifdef  IC_TYPE_GL5207
#define MAX_IDRPERIOD_AVC 63
#define WFD_IDRPERIOD_AVC 30
#define DEFAULT_IDRPERIOD_AVC 30
#else
#define MAX_IDRPERIOD_AVC 15
#define WFD_IDRPERIOD_AVC 15
#define DEFAULT_IDRPERIOD_AVC 13
#endif
#define MIN_IDRPERIOD_AVC 1

#define  CMIMEType_Video_Raw      "video/raw"
#define  CMIMEType_Video_Mjpep  "video/mjpeg"
#define  CMIMEType_Video_Avc       "video/avc"
#define  CMIMEType_Video_PreView    "video/preview"
#define  CMIMEType_Video_FaceDet    "video/facedet"

#define VIDEO_ENC_COMP_NAME        "OMX.Action.Video.Encoder"
#define VIDEO_ENC_COMP_LIBNAME  "libOMX.Action.Video.Encoder.so"
#define VIDEO_ENC_COMP_ROLE  "video_encoder.avc"
#define JPEG_ENC_COMP_ROLE     "video_encoder.mjpg"
#define IPP_ENC_COMP_ROLE        "video_encoder.ipp"
#ifndef kMetadataBufferTypeGrallocSource_act
#define kMetadataBufferTypeGrallocSource_act 0x1
#endif

typedef enum OMX_ACT_VCE_INDEXTYPE {
	OMX_IndexParamRingBuff = ((OMX_INDEXTYPE)OMX_IndexVendorStartUnused + 1000),   /**< reference: OMX_PARAM_SENSORSELECTTYPE */
	OMX_IndexParamIDRBefore = ((OMX_INDEXTYPE)OMX_IndexVendorStartUnused + 1001),   /**< reference: OMX_PARAM_SENSORSELECTTYPE */
	OMX_IndexParamNative = ((OMX_INDEXTYPE)OMX_IndexVendorStartUnused + 1002),   /**< reference: OMX_PARAM_SENSORSELECTTYPE */
} OMX_ACT_VCE_INDEXTYPE;

/**
 * @brief the base video domain structure that describes each port.
 *
 * The data structure is derived from base port class and contain video
 * domain specific parameters.
 * Other elements can be added in the derived components structures.
 */

DERIVEDCLASS(omx_videoenc_PortType, omx_base_video_PortType)
#define omx_videoenc_PortType_FIELDS omx_base_video_PortType_FIELDS \
OMX_BOOL bIsStoreMediaData; /**< It indicates if the port support mediadata stored in buffers */ \
OMX_CONFIG_COLORBLENDTYPE       pconfig_colorblend;\
OMX_CONFIG_RECTTYPE             pconfig_crop;\
OMX_VIDEO_CONFIG_AVCINTRAPERIOD pconfig_avcperiod;\
OMX_CONFIG_INTRAREFRESHVOPTYPE  pconfig_vopfresh;\
OMX_CONFIG_FRAMERATETYPE        pconfig_framerate;\
OMX_VIDEO_CONFIG_BITRATETYPE    pconfig_bitrate; \
OMX_VIDEO_PARAM_AVCTYPE         pavctype;\
OMX_VIDEO_PARAM_BITRATETYPE     pbitrate;\
OMX_VIDEO_PARAM_QUANTIZATIONTYPE pquanty;\
OMX_VIDEO_PARAM_PROFILELEVELTYPE pprofile;\
OMX_ACT_PARAM_THUMBPARAM pConfigTumb;\
OMX_ACT_PARAM_EXIFPARAM pExifInfo;\
OMX_BOOL                         bconfig_changed; \
OMX_BOOL                         pMVC;  \
OMX_IMAGE_PARAM_QFACTORTYPE pIquanty;  \
OMX_ACT_BlendImageType pBlendType; \
OMX_ACT_PARAM_TsPacketType pActTsPacket; \
OMX_ACT_CONFIG_CommonParams pMediaFormatConvert; \
OMX_VCE_Buffers_List BuffersMng_List; \
OMX_BOOL ringbuffer; \
OMX_S32 ringbuf_framesize; \
void *bufferpool;/**<  test for bufpool */
ENDCLASS(omx_videoenc_PortType)

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
OSCL_IMPORT_REF OMX_ERRORTYPE videoenc_port_Constructor(
  OMX_COMPONENTTYPE *openmaxStandComp,
  omx_base_PortType **openmaxStandPort,
  OMX_U32 nPortIndex,
  OMX_BOOL isInput);

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
OSCL_IMPORT_REF OMX_ERRORTYPE videoenc_port_Destructor(
  omx_base_PortType *openmaxStandPort);


OMX_ERRORTYPE videoenc_port_FlushProcessingBuffers(
	omx_base_PortType *openmaxStandPort);


/** @brief Called by the standard allocate buffer, it implements a base functionality.
 *
 * This function can be overriden if the allocation of the buffer is not a simply malloc call.
 * The parameters are the same as the standard function, except for the handle of the port
 * instead of the handler of the component
 * When the buffers needed by this port are all assigned or allocated, the variable
 * bIsFullOfBuffers becomes equal to OMX_TRUE
 */
OMX_ERRORTYPE videoenc_port_AllocateBuffer(
  omx_base_PortType *openmaxStandPort,
  OMX_BUFFERHEADERTYPE** pBuffer,
  OMX_U32 nPortIndex,
  OMX_PTR pAppPrivate,
  OMX_U32 nSizeBytes);

/** @brief Called by the standard use buffer, it implements a base functionality.
 *
 * This function can be overriden if the use buffer implicate more complicated operations.
 * The parameters are the same as the standard function, except for the handle of the port
 * instead of the handler of the component
 * When the buffers needed by this port are all assigned or allocated, the variable
 * bIsFullOfBuffers becomes equal to OMX_TRUE
 */
OMX_ERRORTYPE videoenc_port_UseBuffer(
  omx_base_PortType *openmaxStandPort,
  OMX_BUFFERHEADERTYPE** ppBufferHdr,
  OMX_U32 nPortIndex,
  OMX_PTR pAppPrivate,
  OMX_U32 nSizeBytes,
  OMX_U8* pBuffer);

/** @brief Called by the standard function.
 *
 * It frees the buffer header and in case also the buffer itself, if needed.
 * When all the bufers are done, the variable bIsEmptyOfBuffers is set to OMX_TRUE
 */
OMX_ERRORTYPE videoenc_port_FreeBuffer(
  omx_base_PortType *openmaxStandPort,
  OMX_U32 nPortIndex,
  OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE videoenc_port_SendBufferFunction(
	omx_base_PortType *openmaxStandPort,
	OMX_BUFFERHEADERTYPE* pBuffer);

OMX_ERRORTYPE videoenc_port_ReturnBufferFunction(
	omx_base_PortType* openmaxStandPort,
	OMX_BUFFERHEADERTYPE* pBuffer);

#endif
