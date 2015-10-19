/**
  @file src/components/mad/omx_maddec_component.h

  This component implements an MP3 decoder based on mad
  software library.

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

  $Date$
  Revision $Rev$
  Author $Author$

*/

#ifndef _OMX_MADDEC_COMPONENT_H_
#define _OMX_MADDEC_COMPONENT_H_

#include <OMX_Types.h>
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <omx_base_filter.h>

//specific include files
//#include <mad.h>
#include "audio_decoder_lib.h"
#include "music_parser_lib.h"
#include "downmix.h"


/*
xieyihao 2012-6-19 14:34:59
extension codecs to the OpenMax spec 
*/
#define NUM_ACTIONS_AUDIO_CODECS	18

/*
xieyihao 2012-6-19 14:36:28
define the constant strings of Actions OMX component
*/
#define AUDIO_DEC_BASE_NAME 	"OMX.Action.Audio.Decoder"
#define AUDIO_DEC_MP1_ROLE 		"audio_decoder.mp1"
#define AUDIO_DEC_MP2_ROLE 		"audio_decoder.mp2"
#define AUDIO_DEC_MP3_ROLE 		"audio_decoder.mp3"
#define AUDIO_DEC_AC3_ROLE 		"audio_decoder.ac3"	
#define AUDIO_DEC_AAC_ROLE 		"audio_decoder.aac"
#define AUDIO_DEC_AMR_NB_ROLE 	"audio_decoder.amrnb"
#define AUDIO_DEC_OGG_ROLE 		"audio_decoder.vorbis"
#define AUDIO_DEC_WMASTD_ROLE 	"audio_decoder.wmastd"
#define AUDIO_DEC_WMALSL_ROLE	"audio_decoder.wmalsl"
#define AUDIO_DEC_WMAPRO_ROLE	"audio_decoder.wmapro"
#define AUDIO_DEC_APE_ROLE 		"audio_decoder.ape"
#define AUDIO_DEC_FLAC_ROLE 	"audio_decoder.flac"
#define AUDIO_DEC_RA_ROLE		"audio_decoder.cook"
#define AUDIO_DEC_PCM_ROLE		"audio_decoder.pcm"
#define AUDIO_DEC_ADPCM_ROLE 	"audio_decoder.adpcm"
#define AUDIO_DEC_DTS_ROLE		"audio_decoder.dts"
#define AUDIO_DEC_ACELP_ROLE	"audio_decoder.acelp"
#define AUDIO_DEC_MPC_ROLE    	"audio_decoder.mpc"
#define AUDIO_DEC_AIFF_ROLE   	"audio_decoder.aiff"
#define AUDIO_DEC_AMR_WB_ROLE 	"audio_decoder.amrwb"

/*not yet supported*/
#define AUDIO_DEC_SAMPLE_ROLE 	"audio_decoder.sample"
#define AUDIO_DEC_ALAC_ROLE   	"audio_decoder.alac"


#define OMX_ACTIONS_AUDIODEC_STRING "OMX.Action.Audio.Decoder"

/*
xieyihao 2012-6-19 14:34:51 
define the extension codecs coding type constants 

*/
#define OMX_AUDIO_CodingAC3 		(OMX_AUDIO_CodingVendorStartUnused + 0)
#define OMX_AUDIO_CodingAPE 		(OMX_AUDIO_CodingVendorStartUnused + 1)
#define OMX_AUDIO_CodingFLAC		(OMX_AUDIO_CodingVendorStartUnused + 2)
#define OMX_AUDIO_CodingDTS			(OMX_AUDIO_CodingVendorStartUnused + 3)
#define OMX_AUDIO_CodingWMALSL		(OMX_AUDIO_CodingVendorStartUnused + 4)
#define OMX_AUDIO_CodingWMAPRO		(OMX_AUDIO_CodingVendorStartUnused + 5)
#define OMX_AUDIO_CodingMPC			(OMX_AUDIO_CodingVendorStartUnused + 6)
#define OMX_AUDIO_CodingACELP		(OMX_AUDIO_CodingVendorStartUnused + 7)
#define OMX_AUDIO_CodingAIFF		(OMX_AUDIO_CodingVendorStartUnused + 8)
#define OMX_AUDIO_CodingAWB			(OMX_AUDIO_CodingVendorStartUnused + 9)


#define OMX_ACTIONS_AUDIODEC_MAXERRORFRAMECOUNT (100)

/** Mp3Dec mad component private structure.
 */
DERIVEDCLASS(omx_maddec_component_PrivateType, omx_base_filter_PrivateType)
#define omx_maddec_component_PrivateType_FIELDS omx_base_filter_PrivateType_FIELDS \
  /** @param semaphore for mad decoder access syncrhonization */\
  tsem_t* madDecSyncSem; \
  /** @param pAudioAmr Referece to OMX_AUDIO_PARAM_AMRTYPE structure*/  \
  OMX_AUDIO_PARAM_AMRTYPE pAudioAmr; \
  /** @param pAudioMp3 Referece to OMX_AUDIO_PARAM_MP3TYPE structure*/  \
  OMX_AUDIO_PARAM_MP3TYPE pAudioMp3;  \
  /** @param pAudioAac referece to OMX_AUDIO_PARAM_AACPROFILETYPE structure */ \
  OMX_AUDIO_PARAM_AACPROFILETYPE pAudioAac;\
  /** @param pAudioPcmMode Referece to OMX_AUDIO_PARAM_PCMMODETYPE structure*/  \
  OMX_AUDIO_PARAM_PCMMODETYPE pAudioPcmMode;  \
  /** @param maddecReady boolean flag that is true when the audio coded has been initialized */ \
  OMX_BOOL maddecReady;  \
  /** @param audio_coding_type Field that indicate the supported audio format of audio decoder */ \
  OMX_U32 audio_coding_type;  \
  /** @param time_stamp, us */ \
  OMX_TICKS nTimeStamp; \
  /** @param musicinfo, information from demuxer */ \
  music_info_t MusicInfo; \
  /** @param isFirstBuffer Field that the buffer is the first buffer */ \
  OMX_S32 isFirstBuffer; \
  /** @param codec plugin infomation */ \
  audiodec_plugin_t* pPluginInfo; \
  /** @param module handler, for operations on the .so */ \
	OMX_PTR pCodecModule; \
	/** @param codec handle */ \
	OMX_HANDLETYPE pCodecHandle; \
	/** @param mixer handler */ \
	downmix_state* downmixer; \
	/** @param flush flag */ \
	OMX_BOOL bFlush;		\
	/** @param flush sem */ \
	tsem_t* flushSyncSem;   \
	/** @param flush sync flag */ \
	OMX_BOOL bFlushSync; \
	/** @param mutex for bFlushSync access */ \
	pthread_mutex_t access_mutex; \
	/** @error frame count to report error */ \
	OMX_S32 error_framecount;
ENDCLASS(omx_maddec_component_PrivateType)

//-------------------------------------------------------------------------------------------------------------------


/* Component private entry points declaration */
OMX_ERRORTYPE omx_maddec_component_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName);
OMX_ERRORTYPE omx_maddec_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_maddec_component_Init(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_maddec_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp);
OMX_ERRORTYPE omx_mad_decoder_MessageHandler(OMX_COMPONENTTYPE*,internalRequestMessageType*);

void omx_maddec_component_BufferMgmtCallback(
  OMX_COMPONENTTYPE *openmaxStandComp,
  OMX_BUFFERHEADERTYPE* inputbuffer,
  OMX_BUFFERHEADERTYPE* outputbuffer);

OMX_ERRORTYPE omx_maddec_component_GetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_INOUT OMX_PTR ComponentParameterStructure);

OMX_ERRORTYPE omx_maddec_component_SetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_IN  OMX_PTR ComponentParameterStructure);
OMX_ERRORTYPE omx_maddec_component_SendCommand(
  OMX_HANDLETYPE hComponent,
  OMX_COMMANDTYPE Cmd,
  OMX_U32 nParam,
  OMX_PTR pCmdData);
OMX_ERRORTYPE omx_maddec_component_EmptyThisBuffer(
		OMX_HANDLETYPE hComponent,
		OMX_BUFFERHEADERTYPE* pBuffer);
void omx_maddec_component_SetInternalParameters(OMX_COMPONENTTYPE *openmaxStandComp);

void* omx_maddec_component_BufferMgmtFunction (void* param);
OMX_ERRORTYPE OMX_ComponentInit (OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName);
#endif
