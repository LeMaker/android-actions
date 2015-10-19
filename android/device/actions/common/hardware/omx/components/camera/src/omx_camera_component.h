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
#ifndef _OPENMAX_V1_2_
#include "ACT_OMX_Common_V1_2__V1_1.h"
#endif
#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include "omx_base_source.h"
#include "omx_base_camera_video_port.h"
#include "ALdec_plugin.h"

/** OMX_BASE_FILTER_INPUTPORT_INDEX is the index of any input port for the derived components
 */
#define OMX_BASE_FILTER_VIDEO_INDEX 0

/** OMX_BASE_FILTER_OUTPUTPORT_INDEX is the index of any output port for the derived components
 */
#define OMX_BASE_FILTER_IMAGE_INDEX 1

#define OMX_BASE_FILTER_SYNCPORT_INDEX 2

/** OMX_BASE_FILTER_ALLPORT_INDEX as the standard specifies, the -1 value for port index is used to point to all the ports
 */
#define OMX_BASE_FILTER_ALLPORT_INDEX -1

#define OMX_MAX_SENSORS 0x3

#define ISP_COMP_ROLE "camera_source"

#define _MAX_QUEUEBUF_ 32

typedef struct
{
    unsigned int nPortIndex;
    int nBufferQueue;
    unsigned long pBufferPhy[_MAX_QUEUEBUF_];
    unsigned long pBufferStat[_MAX_QUEUEBUF_];
    int bQb[_MAX_QUEUEBUF_];
} v4l2_buf_prep_t;

DERIVEDCLASS(omx_camera_PrivateType, omx_base_source_PrivateType)
#define omx_camera_PrivateType_FIELDS omx_base_source_PrivateType_FIELDS \
  pthread_mutex_t cmd_mutex;\
  pthread_mutex_t dq_mutex;\
  pthread_mutex_t cmd_mutex_resize;\
  int nSensorSelect[2];\
  /* Preview and capture switch by self*/\
  int bTransFrom_C2V;\
  /* Raw or Yuv sensor */ \
  int nModuleType[3];\
  /*V4l2Hal Module Lib */\
  void  *ModuleLib;\
  /* Capability Information */ \
  OMX_ACT_CAPTYPE *pCapInfo[OMX_MAX_SENSORS];\
  int Dgain_th[OMX_MAX_SENSORS];\
  /** @param BufferMgmtCallback function pointer for algorithm callback */ \
  int (*camera_direct_base_Constructor)(OMX_CAMERATYPE *omx_camera);\
  int (*camera_direct_base_Destructor)(OMX_CAMERATYPE *omx_camera);\
  int (*camera_isp_base_Constructor)(OMX_CAMERATYPE *omx_camera);\
  int (*camera_isp_base_Destructor)(OMX_CAMERATYPE *omx_camera);\
  int (*camera_module_query)(camera_module_info_t *mode_info,int id,int uvcmode);\
  int (*camera_module_release)();\
  /* V4l2 buf's phy address save*/\
  v4l2_buf_prep_t pBufQ[2];\
  /*common ISP lib , handle and functions */ \
  void *ispctl_handle;\
  void *imxctl_handle; \
  /*mjpeg*/\
  videodec_plugin_t *vdec_plugn;\
  void *vdec_handle;\
  void* vdec_lib;\
  /* HDR lib & handle */ \
  void* HDRILib; \
  /* watch dog handle */ \
  void *wdog_handle;
ENDCLASS(omx_camera_PrivateType)


/* Component private entry points declaration */
OMX_ERRORTYPE OMX_ComponentInit(OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName);
OMX_ERRORTYPE omx_camera_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);

OMX_ERRORTYPE omx_camera_component_GetParameter(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_camera_component_SetParameter(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_camera_component_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_camera_component_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nParamIndex,
    OMX_PTR ComponentParameterStructure);

#endif
