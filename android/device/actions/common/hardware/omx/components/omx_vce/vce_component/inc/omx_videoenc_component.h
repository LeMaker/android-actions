/**
  src/components/videoscheduler/omx_video_scheduler_component.h

  This component implements a video scheduler

  Copyright (C) 2008-2009 STMicroelectronics
  Copyright (C) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).

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

#ifndef _OMX_VIDEO_SCHEDULER_H_
#define _OMX_VIDEO_SCHEDULER_H_

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#ifndef _OPENMAX_V1_2_
#include "ACT_OMX_Common_V1_2__V1_1.h"
#endif
#include "ACT_OMX_Index.h"
#include "omx_base_filter.h"
#include "omx_base_video_port.h"
#include "omx_videoenc_port.h"
#include "enc_param.h"
#include "frame_mng.h"

#define OMX_BUFFERFLAG_ACTZEROOFFSET 0x80000000
typedef enum OMX_VIDEO_ENCODING_TYPE {
	OMX_VIDEO_EnCoding_AVC,          /**< H.264/AVC */
	OMX_VIDEO_EnCoding_MJPEG,
	OMX_VIDEO_EnCoding_PREVIEW,
} OMX_VIDEO_ENCODING_TYPE;

DERIVEDCLASS(omx_videoenc_PrivateType, omx_base_filter_PrivateType)
#define omx_videoenc_PrivateType_FIELDS omx_base_filter_PrivateType_FIELDS \
OMX_BOOL FD_Enable; \
OMX_BOOL MSG_EXIT;\
OMX_BOOL MNG_EXIT;\
enc_jpeg_param_t act_jpg_param;\
enc_h264_param_t act_avc_param;\
enc_prp_param_t act_prp_param;\
int enc_mode;\
int enc_codec;\
mng_internal_t mng_info;\
OMX_VIDEO_ENCODING_TYPE video_encoding_type; \
OMX_BOOL isFlushed;\
OMX_BOOL isIDRBefore;
ENDCLASS(omx_videoenc_PrivateType)


/* Component private entry points declaration */
OMX_ERRORTYPE OMX_ComponentInit(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName);

OMX_ERRORTYPE omx_videoenc_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);

OMX_ERRORTYPE omx_videoenc_component_SendCommand(
	OMX_HANDLETYPE hComponent,
	OMX_COMMANDTYPE Cmd,
	OMX_U32 nParam,
	OMX_PTR pCmdData);

OMX_ERRORTYPE omx_videoenc_component_GetExtensionIndex(
	OMX_HANDLETYPE hComponent,
	OMX_STRING cParameterName,
	OMX_INDEXTYPE* pIndexType);

OMX_ERRORTYPE omx_videoenc_component_GetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_videoenc_component_SetParameter(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_videoenc_component_GetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_videoenc_component_SetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nParamIndex,
  OMX_PTR ComponentParameterStructure);

void* omx_videoenc_BufferMgmtFunction (void* param);

#endif
