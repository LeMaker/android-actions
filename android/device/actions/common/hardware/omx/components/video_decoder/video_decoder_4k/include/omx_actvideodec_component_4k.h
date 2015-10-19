/**
  @file src/components/ffmpeg/omx_videodec_component.h
  
  This component implements an H.264 / MPEG-4 AVC video decoder. 
  The H.264 / MPEG-4 AVC Video decoder is based on the FFmpeg software library.

  Copyright (C) 2007-2008 STMicroelectronics
  Copyright (C) 2007-2008 Nokia Corporation and/or its subsidiary(-ies).

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

  $Date: 2008-06-27 12:00:23 +0200 (Fri, 27 Jun 2008) $
  Revision $Rev: 554 $
  Author $Author: pankaj_sen $
*/

#ifndef _OMX_VIDEODEC_COMPONENT_H_
#define _OMX_VIDEODEC_COMPONENT_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <omx_base_filter.h>
#include <string.h>
#include <MetadataBufferType.h>



#include "OMX_ActVideoFifo_4k.h"
#include "omx_comp_debug_levels.h"
#include "OMX_Action_Common_4k.h"


#define ACTION_OMX_VIDDEC_DEBUG 0

//sk
#define ACTION_OMX_VIDDEC_KEY_INFOR 1

#define ACTION_OMX_VIDDEC_PRINTF_ERROR 		1 //should always be 1
#define ACTION_OMX_VIDDEC_PRINTF_WARNING 	1
#define ACTION_OMX_VIDDEC_PERFORMANCE 			0
#define ACTION_OMX_VIDDEC_PITNTF_PIC_W_H ACTION_OMX_VIDDEC_KEY_INFOR

#define ACTION_OMX_VIDDEC_TEST_LINEBUFFER	0
#define ACTION_OMX_VIDDEC_PTHREAD 					ACTION_OMX_VIDDEC_KEY_INFOR

#define ACTION_OMX_VIDDEC_PTHREAD_PIPE  		0// (ACTION_OMX_VIDDEC_PTHREAD)


#define ACTION_OMX_VIDDEC_PORTSETTINGSCHANGED_DEBUG 0

#define ACTION_OMX_VIDDEC_CMD 			0
#define ACTION_OMX_VIDDEC_CODA 			0

#define ACTION_OMX_VIDDEC_FILLBUFFERDONE_DEBUG 0

#define ACTION_OMX_VIDDEC_TSEM_DOWN 0

#define VIDDEC_MAX_INPUT_HEAD_NUM 256 //action inside


#define VIDDEC_MIN_INPUT_BUFFER_NUM				2
#define VIDDEC_MIN_OUTPUT_BUFFER_NUM				4
#define MAX_PRIVATE_BUFFERS						6
#define NUM_OF_PORTS								2
#define VIDDEC_MAX_NAMESIZE						128
#define VIDDEC_NOPORT								0xfffffffe

#define VERSION_MAJOR								1
#define VERSION_MINOR        							0
#define VERSION_REVISION       						0
#define VERSION_STEP                     					0

#define VIDDEC_PIPE_WRITE        						1
#define VIDDEC_PIPE_READ          						0

#define VIDDEC_CLEARFLAGS 							0


#define VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE       	537600  //480*640*(1.5+0.25)
#define VIDDEC_DEFAULT_WIDTH                    			640
#define VIDDEC_DEFAULT_HEIGHT                   			480

#define VIDDEC_FACTORFORMAT444                  		3
#define VIDDEC_FACTORFORMAT422                  		2
#define VIDDEC_FACTORFORMAT420                  		(1.5)
#define VIDDEC_FACTORFORMATSEMI420                  		(1.5)
#define VIDDEC_FACTORFORMAT400					1

#define VIDDEC_COLORFORMAT444               			OMX_COLOR_FormatYUV444Planar
#define VIDDEC_COLORFORMAT422               			OMX_COLOR_FormatYUV422Planar
#define VIDDEC_COLORFORMAT420               			OMX_COLOR_FormatYUV420Planar       //coda output format
#define VIDDEC_COLORFORMATSEMI420                 OMX_COLOR_FormatYUV420SemiPlanar  //vde output format
#define VIDDEC_COLORFORMAT400               			OMX_COLOR_FormatL8
#define VIDDEC_COLORFORMATUNUSED           	 		OMX_COLOR_FormatUnused


#define VIDDEC_COMPONENTROLES_H263				"video_decoder.h263"
#define VIDDEC_COMPONENTROLES_AVC				"video_decoder.avc"
#define VIDDEC_COMPONENTROLES_MPEG2				"video_decoder.mpeg2"
#define VIDDEC_COMPONENTROLES_HEVC				"video_decoder.hevc"
#define VIDDEC_COMPONENTROLES_HEVC_91				"video_decoder.hevc_91"
#define VIDDEC_COMPONENTROLES_MPEG4				"video_decoder.mpeg4"
#define VIDDEC_COMPONENTROLES_VC1				"video_decoder.vc1"
#define VIDDEC_COMPONENTROLES_AVS				"video_decoder.avs"
#define VIDDEC_COMPONENTROLES_RV					"video_decoder.rv"
#define VIDDEC_COMPONENTROLES_MJPEG				"video_decoder.mjpeg"
#define VIDDEC_COMPONENTROLES_DIV3				"video_decoder.div3"
#define VIDDEC_COMPONENTROLES_RVG2				"video_decoder.rvg2"
#define VIDDEC_COMPONENTROLES_FLV1				"video_decoder.flv1"
#define VIDDEC_COMPONENTROLES_VP6				"video_decoder.vp6"
#define VIDDEC_COMPONENTROLES_VP8				"video_decoder.vp8"

#define VIDDEC_MIMETYPEH263						"H263"
#define VIDDEC_MIMETYPEAVC						"AVC"
#define VIDDEC_MIMETYPEMPEG4                 			"MPEG4"
#define VIDDEC_MIMETYPEDIV3                     			"DIV3"
#define VIDDEC_MIMETYPEVP8                     			"VP8"
#define VIDDEC_MIMETYPEVP6                     			"VP6"
#define VIDDEC_MIMETYPEFLV1                     			"FLV1"
#define VIDDEC_MIMETYPERVG2                     			"RVG2"
#define VIDDEC_MIMETYPEVC1                      			"VC1"
#define VIDDEC_MIMETYPEMPEG2                    			"MPEG2"
#define VIDDEC_MIMETYPEAVS							"AVS"
#define VIDDEC_MIMETYPERV							"RV"
#define VIDDEC_MIMETYPEMJPEG                   			"MJPEG"
#define VIDDEC_MIMETYPEYUV                      			"YUV"
#define VIDDEC_MIMETYPEHEVC                     		"HEVC"
#define VIDDEC_MIMETYPEHEVC_91                     		"HEVC_91"


#define QUEUE_ELE_NUM (32)
typedef struct tagQUEUE {
    OMX_S64 e[QUEUE_ELE_NUM]; /* elements */
    OMX_U32 s; /* store index */
    OMX_U32 g; /* get index */
}QUEUE;


/*****************enum definitions*********************/
typedef struct {
    MetadataBufferType metadataBufferType;
    void* handle;
}vde_video_metadata_t;

typedef enum VIDDEC_VPU_STATES
{
	VidDec_VPU_State_Unload = 0,
	VidDec_VPU_State_Loaded,
	VidDec_VPU_State_Start,
	VidDec_VPU_State_Running,
	VidDec_VPU_State_Invalid
}VIDDEC_VPU_STATES;

typedef enum VIDDEC_TYPE_ALLOCATE
{
    	VIDDEC_TALLOC_USEBUFFER = 0,
    	VIDDEC_TALLOC_ALLOCBUFFER
}VIDDEC_TYPE_ALLOCATE;

typedef enum VIDDEC_PORT_INDEX
{
    	VIDDEC_INPUT_PORT,
	VIDDEC_OUTPUT_PORT
}VIDDEC_PORT_INDEX;

typedef enum VIDDEC_BUFFER_OWNER
{
    	VIDDEC_BUFFER_WITH_CLIENT = 0x0,
    	VIDDEC_BUFFER_WITH_COMPONENT,
    	VIDDEC_BUFFER_WITH_TUNNELEDCOMP
} VIDDEC_BUFFER_OWNER;

#if 0
typedef struct VIDEO_PROFILE_LEVEL
{
    	OMX_S32  nProfile;
    	OMX_S32  nLevel;
} VIDEO_PROFILE_LEVEL_TYPE;
#endif

typedef enum VIDDEC_INIT_VALUE
{
    	VIDDEC_INIT_ALL,
    	VIDDEC_INIT_AVC,
    	VIDDEC_INIT_MPEG4,
    	VIDDEC_INIT_H263,
    	VIDDEC_INIT_VP6,
    	VIDDEC_INIT_VP8,
    	VIDDEC_INIT_VC1,
    	VIDDEC_INIT_MPEG2,
    	VIDDEC_INIT_DIV3,
    	VIDDEC_INIT_FLV1,
    	VIDDEC_INIT_RVG2,
    	VIDDEC_INIT_AVS,
    	VIDDEC_INIT_RV,
    	VIDDEC_INIT_MJPEG,
    	VIDDEC_INIT_HEVC,
    	VIDDEC_INIT_HEVC_91,  
    	VIDDEC_INIT_PLANAR420,
    	VIDDEC_INIT_SEMIPLANAR420,
    	VIDDEC_INIT_INTERLEAVED422,
    	VIDDEC_INIT_IDLEEXECUTING,
    	VIDDEC_INIT_LOOP,
    	VIIDE_INIT_MAX = 0x7ffffff
}VIDDEC_INIT_VALUE;

typedef enum VIDDEC_DEFAULT_INPUT_INDEX
{
	VIDDEC_DEFAULT_INPUT_INDEX_AVC,
	VIDDEC_DEFAULT_INPUT_INDEX_H263,
	VIDDEC_DEFAULT_INPUT_INDEX_VP6,
	VIDDEC_DEFAULT_INPUT_INDEX_VP8,
	VIDDEC_DEFAULT_INPUT_INDEX_VC1,
	VIDDEC_DEFAULT_INPUT_INDEX_MPEG2,
	VIDDEC_DEFAULT_INPUT_INDEX_DIV3,
	VIDDEC_DEFAULT_INPUT_INDEX_FLV1,
	VIDDEC_DEFAULT_INPUT_INDEX_MPEG4,
	VIDDEC_DEFAULT_INPUT_INDEX_AVS,
	VIDDEC_DEFAULT_INPUT_INDEX_RV,
	VIDDEC_DEFAULT_INPUT_INDEX_MJPEG,
	VIDDEC_DEFAULT_INPUT_INDEX_RVG2,
	VIDDEC_DEFAULT_INPUT_INDEX_HEVC,
	VIDDEC_DEFAULT_INPUT_INDEX_HEVC_91,
	VIDDEC_DEFAULT_INPUT_INDEX_MAX = 0x7ffffff
}VIDDEC_DEFAULT_INPUT_INDEX;

typedef enum VIDDEC_DEFAULT_OUTPUT_INDEX
{	
	VIDDEC_DEFAULT_OUTPUT_INDEX_PLANAR420,
	VIDDEC_DEFAULT_OUTPUT_INDEX_SEMIPLANAR420,
	VIDDEC_DEFAULT_OUTPUT_INDEX_INTERLEAVED422,
	VIDDEC_DEFAULT_OUTPUT_INDEX_MAX = 0x7ffffff
}VIDDEC_DEFAULT_OUTPUT_INDEX;




/** Video Decoder component private structure.
  */
DERIVEDCLASS(VIDDEC_COMPONENT_PRIVATE, omx_base_filter_PrivateType)
#define VIDDEC_COMPONENT_PRIVATE_FIELDS omx_base_filter_PrivateType_FIELDS \
  /** @param PortReSettingReady boolean flag that is true when the video output Port resetting ready */ \
  OMX_BOOL PortReSettingReady; \
  /** @param avcodecReady boolean flag that is true when the video coded has been initialized */ \
  OMX_BOOL avcodecReady;  \
  /** @param IsProbed boolean flag that is true when h264 has been probed */ \
  OMX_BOOL IsProbed;  \
  /** @param IsPortSettingChanged boolean flag that is true when h264 need to reallocate buffer */ \
  OMX_BOOL IsPortSettingChanged;  \
  /** @param minBufferLength Field that stores the minimun allowed size for FFmpeg decoder */ \
  OMX_U16 minBufferLength; \
  /** @param inputCurrBuffer Field that stores pointer of the current input buffer position */ \
  OMX_U8* inputCurrBuffer;\
  /** @param inputCurrLength Field that stores current input buffer length in bytes */ \
  OMX_U32 inputCurrLength;\
  /** @param isFirstBuffer Field that the buffer is the first buffer */ \
  OMX_S32 isFirstBuffer;\
  /** @param isNewBuffer Field that indicate a new buffer has arrived*/ \
  OMX_S32 isNewBuffer;  \
  /** @param video_coding_type Field that indicate the supported video format of video decoder */ \
  OMX_U32 video_coding_type;   \
  /** @param eOutFramePixFmt Field that indicate output frame pixel format */ \
  videodec_plugin_t *p_interface; \
  void *p_so_handle; \
  void *p_handle; \
  void *vce_handle;\
  pthread_mutex_t PortFlushMutex;\
  tsem_t *decodedSem;\
  tsem_t *mDoneSem;\
  port_t *vout; \
  fb_port_t *fb_port; \
  void *CodecConfigPkt; \
  OMX_U32 CodecConfigPktLen;\
  OMX_BUFFERHEADERTYPE *BufferHeadStorage[VIDDEC_MAX_INPUT_HEAD_NUM]; /**< This array contains the bufferhead point*/\
  OMX_PORT_PARAM_TYPE* pPortParamType; \
  OMX_PORT_PARAM_TYPE* pPortParamTypeAudio;\
  OMX_PORT_PARAM_TYPE* pPortParamTypeImage; \
  OMX_PORT_PARAM_TYPE* pPortParamTypeOthers;\
  OMX_BOOL IsGraphicBuffer   ;\
  OMX_BOOL is_outputbuffer_flushing_pending ;\
  OMX_BOOL is_inputbuffer_flushing_pending ;\
  OMX_BOOL is_fifo_disposing ;\
  OMX_BOOL is_error_status ;\
  OMX_BOOL is_videodec_fifo_init  ;\
  OMX_BOOL is_Thumbnail ;\
  OMX_BOOL is_avc_Thumbnail_outputed ;\
  OMX_BOOL is_cur_stream_buf; \
  OMX_BOOL is_end_of_stream; \
  OMX_U32 nNewBufferCountActual; \
  OMX_U32 nNumAssignedBufferHeads;\
  OMX_VERSIONTYPE pComponentVersion; \
  OMX_VERSIONTYPE pSpecVersion; \
  OMX_COMPONENTTYPE* pHandle; \
  OMX_PARAM_COMPONENTROLETYPE componentRole; \
  OMX_PRIORITYMGMTTYPE* pPriorityMgmt; \
  OMX_U32 frame_num; \
  OMX_U32 err_counter; \
  OMX_U32 actualframewidth; \
  OMX_U32 actualframeheight; \
  OMX_U32 actualdisplaywidth; \
  OMX_U32 actualdisplayheight; \
  OMX_S32 suspend_flag;   \
  OMX_BOOL bConfigPktStored;\
  OMX_BOOL bthirdparty;\
  OMX_BOOL bStoreMediadata[NUM_OF_PORTS];\
  OMX_BOOL bComponentFlush;\
  OMX_BOOL bLowRam;\
  OMX_U8*  pstrbuf;\
  BUFFER_STATUS_FLAG bOutPutBufferAllocated ;\
  OMX_CONFIG_RECTTYPE croprect;\
  int videobuffersize;\
  int framebuffernum;\
  pthread_t OutPutThread; /** @param  OutPutThread This field contains the reference to the thread that output buffers for the components */ \
	int OutPutThreadID; /** @param  OutPutThreadID The ID of the pthread that output buffers */ \
  OMX_BOOL output_portformat_inited;/* flash player compatible*/
  
ENDCLASS(VIDDEC_COMPONENT_PRIVATE)

/* Component private entry points declaration */
OMX_ERRORTYPE OMX_ComponentInit(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName);
OMX_ERRORTYPE omx_videodec_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_videodec_component_Init(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_videodec_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_videodec_component_MessageHandler(OMX_COMPONENTTYPE*,internalRequestMessageType*);
OMX_ERRORTYPE omx_videodec_component_UseBuffer(
            OMX_HANDLETYPE hComponent,
            OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_U32 nPortIndex,
            OMX_PTR pAppPrivate,
            OMX_U32 nSizeBytes,
            OMX_U8* pBuffer);
OMX_ERRORTYPE omx_videodec_component_AllocateBuffer(
            OMX_HANDLETYPE hComponent,
            OMX_BUFFERHEADERTYPE** ppBuffer,
            OMX_U32 nPortIndex,
            OMX_PTR pAppPrivate,
            OMX_U32 nSizeBytes);
OMX_ERRORTYPE omx_videodec_component_FillThisBuffer(
  OMX_HANDLETYPE hComponent,
  OMX_BUFFERHEADERTYPE* pBufferHead) ;
  
OMX_ERRORTYPE omx_videodec_component_FreeBuffer(
            OMX_HANDLETYPE hComponent,
            OMX_U32 nPortIndex,
            OMX_BUFFERHEADERTYPE* pBuffer);
void* omx_videodec_component_BufferMgmtFunction (void* param);
void omx_videodec_component_BufferMgmtCallback(
  OMX_COMPONENTTYPE *openmaxStandComp,
  OMX_BUFFERHEADERTYPE* inputbuffer,
  OMX_BUFFERHEADERTYPE* outputbuffer);

OMX_ERRORTYPE omx_videodec_component_GetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_INOUT OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_videodec_component_SetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_IN  OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_videodec_component_ComponentRoleEnum(
  OMX_IN OMX_HANDLETYPE hComponent,
  OMX_OUT OMX_U8 *cRole,
  OMX_IN OMX_U32 nIndex);

void SetInternalVideoParameters(OMX_COMPONENTTYPE *openmaxStandComp);

OMX_ERRORTYPE omx_videodec_component_SetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure);
  
  OMX_ERRORTYPE omx_videodec_component_GetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure);

OMX_ERRORTYPE omx_videodec_component_GetExtensionIndex(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_STRING cParameterName,
  OMX_OUT OMX_INDEXTYPE* pIndexType);
OMX_ERRORTYPE omx_videodec_component_ComponentDeInit(OMX_HANDLETYPE hComponent);
#endif
