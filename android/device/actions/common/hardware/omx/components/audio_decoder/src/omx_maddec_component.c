/**
  @file src/components/mad/omx_maddec_component.c

  This component implements an Audio decoder based on Actions Audio Codecs lib
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
#include <dlfcn.h>
#include <omxcore.h>
#include <omx_maddec_audio_port.h>
#include <omx_maddec_component.h>
#include <OMX_IndexExt.h>

#define MIN(X,Y)    ((X) < (Y) ?  (X) : (Y))

/** Maximum Number of Audio Mad Decoder Component Instance*/
#define MAX_COMPONENT_MADDEC 4

/** Number of Mad Component Instance*/
static OMX_U32 noMadDecInstance=0;



/** this function initializates the mad framework, and opens an mad decoder of type specified by IL client */
OMX_ERRORTYPE omx_maddec_component_madLibInit(omx_maddec_component_PrivateType* omx_maddec_component_Private) {

	/** 
	xieyihao 2012-6-19 10:23:54
	load the codec module accordingly and get the codec handle
	*/
	typedef void*(*LoadFunction)(void);
  OMX_PTR pCodecModule = NULL;
  LoadFunction codec_loader = NULL;
  OMX_ERRORTYPE err = OMX_ErrorNone;
  char buf[32];
  memset(buf, 0, 32);
  strcpy(buf, "ad");
  switch(omx_maddec_component_Private->audio_coding_type)
	{
	case OMX_AUDIO_CodingMP3:
		strcat(buf, "MP3.so");
		break;
	case OMX_AUDIO_CodingAAC:
		strcat(buf, "AAC.so");
		break;
	case OMX_AUDIO_CodingAC3:
		strcat(buf, "AC3.so");
		break;
	case OMX_AUDIO_CodingPCM:
	case OMX_AUDIO_CodingADPCM:
		strcat(buf, "PCM.so");
		break;
	case OMX_AUDIO_CodingVORBIS:
		strcat(buf, "OGG.so");
		break;
	case OMX_AUDIO_CodingAPE:
		strcat(buf, "APE.so");
		break;
	case OMX_AUDIO_CodingRA:
		strcat(buf, "COOK.so");
		break;
	case OMX_AUDIO_CodingFLAC:
		strcat(buf, "FLAC.so");
		break;
	case OMX_AUDIO_CodingAMR:
		strcat(buf, "AMR.so");
		break;
	case OMX_AUDIO_CodingWMA:
		strcat(buf, "WMASTD.so");
		break;
	case OMX_AUDIO_CodingWMALSL:
		strcat(buf, "WMALSL.so");
		break;
	case OMX_AUDIO_CodingWMAPRO:
		strcat(buf, "WMAPRO.so");
		break;
	case OMX_AUDIO_CodingDTS:
		strcat(buf, "DTS.so");
		break;
	case OMX_AUDIO_CodingACELP:
	    strcat(buf,"ACELP.so");
	    break;
	case OMX_AUDIO_CodingMPC:
	    strcat(buf,"MPC.so");
	    break;
	case OMX_AUDIO_CodingAIFF:
	    strcat(buf,"AIFF.so");
	    break;
	case OMX_AUDIO_CodingAWB:
	    strcat(buf,"AWB.so");
	    break;
	            
	default:
		DEBUG(DEB_LEV_ERR, "In %s encounted unrecognized format %d\n",__func__,omx_maddec_component_Private->audio_coding_type);	
		goto LOADCODEC_EXIT;			
	}

  pCodecModule = dlopen(buf, RTLD_LAZY | RTLD_GLOBAL);
  if( pCodecModule == NULL )
  {
      DEBUG(DEB_LEV_ERR, "In %s open module failed\n",__func__);	
      err = OMX_ErrorComponentNotFound;
      goto LOADCODEC_EXIT;
  }
  codec_loader = (LoadFunction) dlsym(pCodecModule, "get_plugin_info");
  if (codec_loader == NULL)
  {
      DEBUG(DEB_LEV_ERR, "In %s get loader failed\n",__func__);	
      dlclose( pCodecModule);
      pCodecModule = NULL;
      err = OMX_ErrorDynamicResourcesUnavailable;
      goto LOADCODEC_EXIT;
  }
  omx_maddec_component_Private->pPluginInfo = (audiodec_plugin_t *) (codec_loader());
  if (omx_maddec_component_Private->pPluginInfo == NULL)
  {
  	DEBUG(DEB_LEV_ERR, "In %s load codec plugin failed\n",__func__);	
    dlclose( pCodecModule);
    pCodecModule = NULL;
    err = OMX_ErrorDynamicResourcesUnavailable;
    goto LOADCODEC_EXIT;
  }
  
 
  omx_maddec_component_Private->pCodecHandle = omx_maddec_component_Private->pPluginInfo->open(omx_maddec_component_Private->MusicInfo.buf);
  
  if (omx_maddec_component_Private->pCodecHandle == NULL)
  {
  	DEBUG(DEB_LEV_ERR, "In %s open codec handler failed\n",__func__);
    dlclose( pCodecModule);
    pCodecModule = NULL;
    err = OMX_ErrorDynamicResourcesUnavailable;
    goto LOADCODEC_EXIT;
  }
  omx_maddec_component_Private->pPluginInfo->ex_ops(omx_maddec_component_Private->pCodecHandle, EX_OPS_CHUNK_RESET, 0);
  DEBUG(DEB_LEV_SIMPLE_SEQ, " %s GetCodecLibAndHandle: %s finished, ", __func__, buf);
	omx_maddec_component_Private->pCodecModule = pCodecModule;
	tsem_up (omx_maddec_component_Private->madDecSyncSem);
	
	/* xieyihao 2012-7-11 11:38:36
	add downmixer handle
	*/
	omx_maddec_component_Private->downmixer = downmix_open();
	if(omx_maddec_component_Private->downmixer == NULL)
	{
		omx_maddec_component_Private->pPluginInfo->close(omx_maddec_component_Private->pCodecHandle);
		omx_maddec_component_Private->pCodecHandle = NULL;
		DEBUG(DEB_LEV_ERR, "In %s open downmixer handler failed\n",__func__);
    dlclose( pCodecModule);
    pCodecModule = NULL;
    err = OMX_ErrorDynamicResourcesUnavailable;
    goto LOADCODEC_EXIT;		
	}
	
LOADCODEC_EXIT:
  return err;
}


/** this function Deinitializates the mad framework, and close the mad decoder */
void omx_maddec_component_madLibDeInit(omx_maddec_component_PrivateType* omx_maddec_component_Private) {


	/*
  xieyihao 2012-6-19 11:14:38
  unload the codec plugin and close the module
  */
	omx_maddec_component_Private->pPluginInfo->close(omx_maddec_component_Private->pCodecHandle);
	omx_maddec_component_Private->pCodecHandle = NULL;
	omx_maddec_component_Private->pPluginInfo = NULL;
	dlclose(omx_maddec_component_Private->pCodecModule);
	omx_maddec_component_Private->pCodecModule = NULL;	
	downmix_close(omx_maddec_component_Private->downmixer);	

}

/** The Constructor
  *
  * @param openmaxStandComp the component handle to be constructed
  * @param cComponentName name of the component to be constructed
  */
OMX_ERRORTYPE OMX_ComponentInit (OMX_COMPONENTTYPE *openmaxStandComp, OMX_STRING cComponentName)
{

  OMX_ERRORTYPE err = OMX_ErrorNone;
  omx_maddec_component_PrivateType* omx_maddec_component_Private;
  omx_maddec_audio_PortType *inPort,*outPort;
  OMX_U32 i;
  
  /*
  xieyiao 2012-6-26 17:43:50
  the omx core havn't init this field with a valid value, so we just calloc it
  */
  
   if (!openmaxStandComp->pComponentPrivate) {
    openmaxStandComp->pComponentPrivate = calloc(1, sizeof(omx_maddec_component_PrivateType));
    if(openmaxStandComp->pComponentPrivate==NULL)  {
      return OMX_ErrorInsufficientResources;
    }
  }  else {
    DEBUG(DEB_LEV_FUNCTION_NAME, "In %s, Error Component %x Already Allocated\n",
              __func__, (long)openmaxStandComp->pComponentPrivate);
  }
  

  omx_maddec_component_Private = openmaxStandComp->pComponentPrivate;
  omx_maddec_component_Private->ports = NULL;

  /** we could create our own port structures here
    * fixme maybe the base class could use a "port factory" function pointer?
    */
  err = omx_base_filter_Constructor(openmaxStandComp, cComponentName);

  DEBUG(DEB_LEV_SIMPLE_SEQ, "constructor of mad decoder component is called\n");

  /** Domain specific section for the ports. */
  /** first we set the parameter common to both formats
    * parameters related to input port which does not depend upon input audio format
    */
  omx_maddec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nStartPortNumber = 0;
  omx_maddec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts = 2;

  /** Allocate Ports and call port constructor. */
  if (omx_maddec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts && !omx_maddec_component_Private->ports) {
    omx_maddec_component_Private->ports = calloc(omx_maddec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts, sizeof(omx_base_PortType *));
    if (!omx_maddec_component_Private->ports) {
      return OMX_ErrorInsufficientResources;
    }
    for (i=0; i < omx_maddec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
      omx_maddec_component_Private->ports[i] = calloc(1, sizeof(omx_maddec_audio_PortType));
      if (!omx_maddec_component_Private->ports[i]) {
        return OMX_ErrorInsufficientResources;
      }
    }
  }

  omx_audio_port_Constructor(openmaxStandComp, &omx_maddec_component_Private->ports[0], 0, OMX_TRUE);
  omx_audio_port_Constructor(openmaxStandComp, &omx_maddec_component_Private->ports[1], 1, OMX_FALSE);


  /** parameters related to input port */
  inPort = (omx_maddec_audio_PortType *) omx_maddec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];

  inPort->sPortParam.nBufferSize = DEFAULT_AUDIO_IN_BUFFER_SIZE;
  strcpy(inPort->sPortParam.format.audio.cMIMEType, "audio/mpeg");
  inPort->sPortParam.format.audio.eEncoding = OMX_AUDIO_CodingMP3;

  inPort->sAudioParam.eEncoding = OMX_AUDIO_CodingMP3;

  setHeader(&omx_maddec_component_Private->pAudioMp3, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
  omx_maddec_component_Private->pAudioMp3.nPortIndex = 0;
  omx_maddec_component_Private->pAudioMp3.nChannels = 2;
  omx_maddec_component_Private->pAudioMp3.nBitRate = 28000;
  omx_maddec_component_Private->pAudioMp3.nSampleRate = 44100;
  omx_maddec_component_Private->pAudioMp3.nAudioBandWidth = 0;
  omx_maddec_component_Private->pAudioMp3.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  omx_maddec_component_Private->pAudioMp3.eFormat=OMX_AUDIO_MP3StreamFormatMP1Layer3;

  /** parameters related to output port */
  outPort = (omx_maddec_audio_PortType *) omx_maddec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
  outPort->sPortParam.format.audio.eEncoding = OMX_AUDIO_CodingPCM;
  outPort->sPortParam.nBufferSize = DEFAULT_AUDIO_OUT_BUFFER_SIZE;

  outPort->sAudioParam.eEncoding = OMX_AUDIO_CodingPCM;

  /** settings of output port audio format - pcm */
  setHeader(&omx_maddec_component_Private->pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
  omx_maddec_component_Private->pAudioPcmMode.nPortIndex = 1;
  omx_maddec_component_Private->pAudioPcmMode.nChannels = 2;
  omx_maddec_component_Private->pAudioPcmMode.eNumData = OMX_NumericalDataSigned;
  omx_maddec_component_Private->pAudioPcmMode.eEndian = OMX_EndianLittle;
  omx_maddec_component_Private->pAudioPcmMode.bInterleaved = OMX_TRUE;
  omx_maddec_component_Private->pAudioPcmMode.nBitPerSample = 16;
  omx_maddec_component_Private->pAudioPcmMode.nSamplingRate = 44100;
  omx_maddec_component_Private->pAudioPcmMode.ePCMMode = OMX_AUDIO_PCMModeLinear;
  omx_maddec_component_Private->pAudioPcmMode.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  omx_maddec_component_Private->pAudioPcmMode.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
  omx_maddec_component_Private->BufferMgmtFunction = omx_maddec_component_BufferMgmtFunction;
  pthread_mutex_init(&omx_maddec_component_Private->access_mutex, NULL);

  /** now it's time to know the audio coding type of the component */
  if(!strcmp(cComponentName, OMX_ACTIONS_AUDIODEC_STRING))  {
    omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingMP3;
  }  else if (!strcmp(cComponentName, AUDIO_DEC_BASE_NAME)) {
    omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingUnused;
  }  else  {
    // IL client specified an invalid component name
    return OMX_ErrorInvalidComponentName;
  }
  /** initialise the semaphore to be used for mad decoder access synchronization */
  if(!omx_maddec_component_Private->madDecSyncSem) {
    omx_maddec_component_Private->madDecSyncSem = calloc(1,sizeof(tsem_t));
    if(omx_maddec_component_Private->madDecSyncSem == NULL) {
      return OMX_ErrorInsufficientResources;
    }
    tsem_init(omx_maddec_component_Private->madDecSyncSem, 0);
  }
  
  /** initialise the semaphore to be used for flush and emptybuffer access synchronization */
  if(!omx_maddec_component_Private->flushSyncSem) {
    omx_maddec_component_Private->flushSyncSem = calloc(1,sizeof(tsem_t));
    if(omx_maddec_component_Private->flushSyncSem == NULL) {
      return OMX_ErrorInsufficientResources;
    }
    tsem_init(omx_maddec_component_Private->flushSyncSem, 0);
  }

  /** general configuration irrespective of any audio formats
    *  setting values of other fields of omx_maddec_component_Private structure
    */
  memset(&omx_maddec_component_Private->MusicInfo, 0, sizeof(music_info_t));
  omx_maddec_component_Private->MusicInfo.sample_rate = omx_maddec_component_Private->pAudioPcmMode.nSamplingRate;
  omx_maddec_component_Private->MusicInfo.channels = omx_maddec_component_Private->pAudioPcmMode.nChannels;
  omx_maddec_component_Private->maddecReady = OMX_FALSE;
  omx_maddec_component_Private->error_framecount = 0;
  omx_maddec_component_Private->isFirstBuffer = OMX_TRUE;
  omx_maddec_component_Private->bFlush = OMX_FALSE;
  omx_maddec_component_Private->bFlushSync = OMX_FALSE;
  omx_maddec_component_Private->pCodecHandle = NULL;
	omx_maddec_component_Private->pPluginInfo = NULL;
	omx_maddec_component_Private->pCodecModule = NULL;
  omx_maddec_component_Private->BufferMgmtCallback = omx_maddec_component_BufferMgmtCallback;
  omx_maddec_component_Private->messageHandler = omx_mad_decoder_MessageHandler;
  omx_maddec_component_Private->destructor = omx_maddec_component_Destructor;
  openmaxStandComp->SetParameter = omx_maddec_component_SetParameter;
  openmaxStandComp->GetParameter = omx_maddec_component_GetParameter;
  openmaxStandComp->SendCommand = omx_maddec_component_SendCommand;
  openmaxStandComp->EmptyThisBuffer = omx_maddec_component_EmptyThisBuffer;

  noMadDecInstance++;

  if(noMadDecInstance>MAX_COMPONENT_MADDEC)
    return OMX_ErrorInsufficientResources;


  return err;
}

/** this function sets some inetrnal parameters to the input port depending upon the input audio format */
void omx_maddec_component_SetInternalParameters(OMX_COMPONENTTYPE *openmaxStandComp) {

  omx_maddec_component_PrivateType* omx_maddec_component_Private;
  omx_maddec_audio_PortType *pPort;;

  omx_maddec_component_Private = openmaxStandComp->pComponentPrivate;
  /*
  xieyihao 2012-6-18 16:34:23
  add support AAC & AC3
  */
	pPort = (omx_maddec_audio_PortType *) omx_maddec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
  switch (omx_maddec_component_Private->audio_coding_type)
  {
  case OMX_AUDIO_CodingMP3:
  /** setting port & private fields according to MP3 audio format values */
  strcpy(omx_maddec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX]->sPortParam.format.audio.cMIMEType, "audio/mpeg");
  omx_maddec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX]->sPortParam.format.audio.eEncoding = OMX_AUDIO_CodingMP3;

  setHeader(&omx_maddec_component_Private->pAudioMp3, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
  omx_maddec_component_Private->pAudioMp3.nPortIndex = 0;
  omx_maddec_component_Private->pAudioMp3.nChannels = 2;
  omx_maddec_component_Private->pAudioMp3.nBitRate = 28000;
  omx_maddec_component_Private->pAudioMp3.nSampleRate = 44100;
  omx_maddec_component_Private->pAudioMp3.nAudioBandWidth = 0;
  omx_maddec_component_Private->pAudioMp3.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  omx_maddec_component_Private->pAudioMp3.eFormat=OMX_AUDIO_MP3StreamFormatMP1Layer3;
 
  pPort->sAudioParam.eEncoding = OMX_AUDIO_CodingMP3;
  break;
  case OMX_AUDIO_CodingAAC:
  setHeader(&omx_maddec_component_Private->pAudioAac, sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
  omx_maddec_component_Private->pAudioAac.nPortIndex = 0;
  omx_maddec_component_Private->pAudioAac.nChannels = 2;
  omx_maddec_component_Private->pAudioAac.nSampleRate = 44100;
  omx_maddec_component_Private->pAudioAac.nBitRate = 192000;
  omx_maddec_component_Private->pAudioAac.nFrameLength = 1024;
  omx_maddec_component_Private->pAudioAac.eChannelMode = OMX_AUDIO_ChannelModeStereo;
  pPort->sAudioParam.eEncoding = OMX_AUDIO_CodingAAC;
  break;
  case OMX_AUDIO_CodingAC3:
  pPort->sAudioParam.eEncoding = OMX_AUDIO_CodingAC3;
  /*
  xieyihao 2012-6-18 16:35:45
  TODO: add ac3 format default values
  */
  break;
  default:
  DEBUG(DEB_LEV_ERR, "In %s unrecognized format = %d\n",__func__,omx_maddec_component_Private->audio_coding_type);	
  break;
  }
  setHeader(&pPort->sAudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
  pPort->sAudioParam.nPortIndex = 0;
  pPort->sAudioParam.nIndex = 0;

}


/** The destructor */
OMX_ERRORTYPE omx_maddec_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp) {

  omx_maddec_component_PrivateType* omx_maddec_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_U32 i;

  if(omx_maddec_component_Private->madDecSyncSem) {
    tsem_deinit(omx_maddec_component_Private->madDecSyncSem);
    free(omx_maddec_component_Private->madDecSyncSem);
    omx_maddec_component_Private->madDecSyncSem = NULL;
  }
  
  
  if(omx_maddec_component_Private->flushSyncSem) {
    tsem_deinit(omx_maddec_component_Private->flushSyncSem);
    free(omx_maddec_component_Private->flushSyncSem);
    omx_maddec_component_Private->flushSyncSem = NULL;
  }

  /* frees port/s */
  if (omx_maddec_component_Private->ports) {
    for (i=0; i < omx_maddec_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts; i++) {
      if(omx_maddec_component_Private->ports[i])
        omx_maddec_component_Private->ports[i]->PortDestructor(omx_maddec_component_Private->ports[i]);
    }
    free(omx_maddec_component_Private->ports);
    omx_maddec_component_Private->ports=NULL;
  }
  pthread_mutex_destroy(&omx_maddec_component_Private->access_mutex);

  DEBUG(DEB_LEV_FUNCTION_NAME, "Destructor of mad decoder component is called\n");

  omx_base_filter_Destructor(openmaxStandComp);
  
  if(omx_maddec_component_Private->MusicInfo.buf!=NULL){
  	free(omx_maddec_component_Private->MusicInfo.buf);
  	omx_maddec_component_Private->MusicInfo.buf = NULL;
  }

  noMadDecInstance--;

  return OMX_ErrorNone;

}

/** The Initialization function  */
OMX_ERRORTYPE omx_maddec_component_Init(OMX_COMPONENTTYPE *openmaxStandComp)  {
  omx_maddec_component_PrivateType* omx_maddec_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_ERRORTYPE err = OMX_ErrorNone;


  return err;
}

/** The Deinitialization function  */
OMX_ERRORTYPE omx_maddec_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp) {

  omx_maddec_component_PrivateType* omx_maddec_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  if (omx_maddec_component_Private->maddecReady) {
    omx_maddec_component_madLibDeInit(omx_maddec_component_Private);
    omx_maddec_component_Private->maddecReady = OMX_FALSE;
  }
  


  return err;
}
static int post_sat(int acc, int post_max, int post_min) {
	if (acc > post_max) {
		acc = post_max;
	} else if (acc < post_min) {
		acc = post_min;
	}
	return acc;
}
/*
xieyihao 2012-6-20 9:34:16
implement the data mixer
TODO: other channel modes other than stereo,(etc 5.1, mono...)
*/
void omx_maddec_mixdata(omx_maddec_component_PrivateType* omx_maddec_component_Private, 
																	audiout_pcm_t* out, OMX_BUFFERHEADERTYPE* pBufferHead)
{
	int nChannels = out->channels;
	int nSamples = out->samples;
	int nFracbits = out->frac_bits;
	int** pPtrPcm = (int**)(out->pcm);
	short* pPcmOut = (short*)(pBufferHead->pBuffer);
	int ch, i;
	int sat_min = (int) (~0) << 15;
	int sat_max = ~sat_min;
	/*xieyihao 2012-7-11 11:42:37
		add downmix processing
	*/
	if(out->channels > 2)
	{
		downmix_state* downmixer = omx_maddec_component_Private->downmixer;
		downmix_set(downmixer, (downmix_int32 *) (out->pcm[0]),
													(downmix_int32 *) (out->pcm[1]));
		for (i = 0; i < out->channels; i++) 
		{
			downmix_run(downmixer, (downmix_int32 *) (out->pcm[i]),
					out->samples, i);
		}
	}
	if(out->channels > omx_maddec_component_Private->pAudioPcmMode.nChannels)
 	{
     	out->channels=omx_maddec_component_Private->pAudioPcmMode.nChannels; 
 	}

	
	if (out->channels == 1) {
		nChannels = 1;
		for (i = 0; i < out->samples; i++) {
			*pPcmOut++ = (short) post_sat((pPtrPcm[0][i] >> nFracbits), sat_max,
					sat_min);
			
		}
	} else {
		nChannels = 2;
		for (i = 0; i < out->samples; i++) {
			*pPcmOut++ = (short) (post_sat((pPtrPcm[0][i] >> nFracbits), sat_max,
					sat_min));
			*pPcmOut++ = (short) (post_sat((pPtrPcm[1][i] >> nFracbits), sat_max,
					sat_min));
		}
	}
/*	 
	for(i = 0; i < nSamples; i++)
	{
		for(ch = 0; ch < nChannels; ch++)
		{
			*pPcmOut++ = pPtrPcm[ch][i] >> nFracbits;	
		}
		
	}
*/	
	pBufferHead->nOffset = 0;
	pBufferHead->nFilledLen = nSamples * sizeof(short) * nChannels;

}
/**This function prepare the codec type for function omx_maddec_component_madlibinit
	*supported codecs:
	*MP3 AAC AC3
	*/
OMX_ERRORTYPE InitCodingType(omx_maddec_component_PrivateType* omx_maddec_component_Private)
{
	OMX_ERRORTYPE err = OMX_ErrorNone;
	char* extension = omx_maddec_component_Private->MusicInfo.extension;
	
	if(!strcmp(extension, "MP3")) omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingMP3;
	else if(!strcmp(extension, "AAC")) omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingAAC;
	else if(!strcmp(extension, "AC3")) omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingAC3;
	else	err = OMX_ErrorFormatNotDetected;
		
	return err;
	
	
	
	
}
/** This function is the buffer management callback function for MP3 decoding
  * is used to process the input buffer and provide one output buffer
  *
  * @param openmaxStandComp the component handle
  * @param inputbuffer is the input buffer containing the input MP3 content
  * @param outputbuffer is the output buffer on which the output pcm content will be written
  */
/**
  *	BUGFIX:BUG00224916 
  *	memory leak: Replay a video with OWL, there is a memory leak problem.                                                                              
  *	ActionsCode(author:jinsongxue, change_code) 
 */                                       
void omx_maddec_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* inputbuffer, OMX_BUFFERHEADERTYPE* outputbuffer) {
  omx_maddec_component_PrivateType* omx_maddec_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_U32 nchannels;
  int used_bytes = 0;
  int ret;
  OMX_ERRORTYPE err = OMX_ErrorNone;
  audiout_pcm_t aout;
	audiodec_plugin_t *pPluginInfo;
	OMX_HANDLETYPE pCodecHandle;
    aout.samples = 0;
	/*
	xieyihao 2012-6-20 10:02:46
	to fit with the Actions Audiodecoder music_info_t
	*/
	if(omx_maddec_component_Private->isFirstBuffer == OMX_TRUE)
	{
		omx_maddec_component_Private->error_framecount = 0;
		omx_maddec_component_Private->isFirstBuffer = OMX_FALSE;
		/*if it is a codecconfig frame, then we should use it to init the madlib*/
		if(inputbuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
		{
			
			   music_info_t* pMusioInfo = &omx_maddec_component_Private->MusicInfo;				
				 if(inputbuffer->nFilledLen>0){
						pMusioInfo->buf= (void*)malloc(inputbuffer->nFilledLen);
						if(pMusioInfo->buf==NULL){
							DEBUG(DEB_LEV_ERR,"In %s malloc err\n", __func__);
							inputbuffer->nFilledLen = 0;
							inputbuffer->nFlags = 0;
							(*(omx_maddec_component_Private->callbacks->EventHandler))
							     (openmaxStandComp,
							     omx_maddec_component_Private->callbackData,
							     OMX_EventError, /* The command was completed */
							     OMX_ErrorInsufficientResources, /* The commands was a OMX_CommandStateSet */
							     0, /* The state has been changed in message->messageParam */
							     NULL);
							return ;
						}
						memcpy(pMusioInfo->buf,inputbuffer->pBuffer,inputbuffer->nFilledLen);
					}
					if(omx_maddec_component_Private->maddecReady == OMX_FALSE)
					{
							DEBUG(DEB_LEV_SIMPLE_SEQ, "in %s, line %d\n", __func__, __LINE__);
							err = omx_maddec_component_madLibInit(omx_maddec_component_Private);
							if(err != OMX_ErrorNone)
							{
								DEBUG(DEB_LEV_ERR,"In %s Init codec lib failed\n", __func__);	
								inputbuffer->nFilledLen = 0;
								inputbuffer->nFlags = 0;
								(*(omx_maddec_component_Private->callbacks->EventHandler))
							      (openmaxStandComp,
							      omx_maddec_component_Private->callbackData,
							      OMX_EventError, /* The command was completed */
							      err, /* The commands was a OMX_CommandStateSet */
							      0, /* The state has been changed in message->messageParam */
							      NULL);
								return;		
							}
							omx_maddec_component_Private->maddecReady = OMX_TRUE;
					}
					
					inputbuffer->nFilledLen = 0;
					inputbuffer->nFlags = 0;
					return;		 

		}
		if(omx_maddec_component_Private->maddecReady == OMX_FALSE)
		{
				DEBUG(DEB_LEV_SIMPLE_SEQ, "in %s, line %d\n", __func__, __LINE__);
				err = omx_maddec_component_madLibInit(omx_maddec_component_Private);
				if(err != OMX_ErrorNone)
				{
					DEBUG(DEB_LEV_ERR,"In %s Init codec lib failed\n", __func__);	
					inputbuffer->nFilledLen = 0;
					(*(omx_maddec_component_Private->callbacks->EventHandler))
				      (openmaxStandComp,
				      omx_maddec_component_Private->callbackData,
				      OMX_EventError, /* The command was completed */
				      err, /* The commands was a OMX_CommandStateSet */
				      0, /* The state has been changed in message->messageParam */
				      NULL);	
					return;		
				}
				DEBUG(DEB_LEV_SIMPLE_SEQ, "in %s, line %d\n", __func__, __LINE__);
				omx_maddec_component_Private->maddecReady = OMX_TRUE;
		}
			
	}

	if(omx_maddec_component_Private->maddecReady == OMX_FALSE)
	{
		DEBUG(DEB_LEV_ERR, "In %s codec lib Not Ready \n",__func__);
		inputbuffer->nFilledLen = 0;
		err = OMX_ErrorDynamicResourcesUnavailable;
		(*(omx_maddec_component_Private->callbacks->EventHandler))
		  (openmaxStandComp,
		  omx_maddec_component_Private->callbackData,
		  OMX_EventError, /* The command was completed */
		  err, /* The commands was a OMX_CommandStateSet */
		    0, /* The state has been changed in message->messageParam */
		  NULL);
    	return;	
	}
	/*for the video/audio sync player*/
      if( inputbuffer->nTimeStamp != 0 || omx_maddec_component_Private->bFlush == OMX_TRUE)
      {
      	omx_maddec_component_Private->nTimeStamp = inputbuffer->nTimeStamp;
      	inputbuffer->nTimeStamp = 0;		
      	omx_maddec_component_Private->bFlush = OMX_FALSE;
      }
	/*
	if reconfig timestamp, then copy it from outputbuffer
	DONOT decide whether this is the starttime by the input flags
	because its nFlags field was cleared out side in the base_filter.c
	*/
	if(outputbuffer->nFlags & OMX_BUFFERFLAG_STARTTIME)
	{
		DEBUG(DEB_LEV_FULL_SEQ, "In %s receive timestamp buffer, time is %d ms\n", __func__,(OMX_U32)(outputbuffer->nTimeStamp/1000));
		omx_maddec_component_Private->nTimeStamp = outputbuffer->nTimeStamp;
		outputbuffer->nFlags = 0;	
	}
  outputbuffer->nFilledLen = 0;
  outputbuffer->nOffset=0;
  pCodecHandle = omx_maddec_component_Private->pCodecHandle;
  pPluginInfo = omx_maddec_component_Private->pPluginInfo;
  
  /*ActionsCode(jinsongxue, bugfix BUG00224916)
   *process package 0. only if inputbuffer->filledlen non-zero, decoder run following, or there is memory leak.
   */	
  if(inputbuffer->nFilledLen == 0)
  {
  	DEBUG(DEB_LEV_SIMPLE_SEQ, "in %s, line %d\n", __func__, __LINE__);
	return;
  }
  
  ret = pPluginInfo->frame_decode(pCodecHandle, inputbuffer->pBuffer+inputbuffer->nOffset,
  													inputbuffer->nFilledLen, &aout, &used_bytes);
  if(ret < 0)
  {
  	DEBUG(DEB_LEV_ERR,"codec module reported an error\n");
  	used_bytes = inputbuffer->nFilledLen;
  	omx_maddec_component_Private->error_framecount++;
  	/*
  	if receive more than OMX_ACTIONS_AUDIODEC_MAXERRORFRAMECOUNT error
  	frames continuously, then report error immediately
  	*/
  	if((omx_maddec_component_Private->error_framecount >= OMX_ACTIONS_AUDIODEC_MAXERRORFRAMECOUNT)
  		&& (inputbuffer->pBuffer != NULL) && (inputbuffer->nFilledLen != 0))
  	{
	  	err = OMX_ErrorStreamCorrupt;
	  	(*(omx_maddec_component_Private->callbacks->EventHandler))
		  (openmaxStandComp,
		  omx_maddec_component_Private->callbackData,
		  OMX_EventError, /* The command was completed */
		  err, /* The commands was a OMX_CommandStateSet */
		  0, /* The state has been changed in message->messageParam */
		  NULL);
	}
  }
  else
  {
  	omx_maddec_component_Private->error_framecount = 0;	
  }
  
  	/*
  	xieyihao 2012-6-19 14:02:37
  	TODO: implement the reconfig port prms in the future; 
  	*/
	/*if downmix with sample count 0, then the codec may not set channels fields
		so may cause Unpredictability  
	*/
	if(aout.samples > 0)
	{
  	omx_maddec_mixdata(omx_maddec_component_Private,&aout, outputbuffer);
  }
  outputbuffer->nTimeStamp = omx_maddec_component_Private->nTimeStamp;
  inputbuffer->nOffset += used_bytes;
  inputbuffer->nFilledLen -= used_bytes;
  omx_maddec_component_Private->nTimeStamp += 
  ((OMX_S64)aout.samples * 1000000) / (omx_maddec_component_Private->MusicInfo.sample_rate); 
  

}

/** this function sets the parameter values regarding audio format & index */
OMX_ERRORTYPE omx_maddec_component_SetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_IN  OMX_PTR ComponentParameterStructure)  {

  OMX_ERRORTYPE err = OMX_ErrorNone;
  OMX_AUDIO_PARAM_PORTFORMATTYPE *pAudioPortFormat;
  OMX_AUDIO_PARAM_PCMMODETYPE* pAudioPcmMode;
  OMX_AUDIO_PARAM_MP3TYPE * pAudioMp3;
  OMX_PARAM_PORTDEFINITIONTYPE* pPortDef;
  /*
  xieyihao 2012-6-18 16:41:37
  add supported codecs: AAc & AC3
  */
  /*
  kingson 2014-7-24 11:08:37
  add supported codecs: AMR
  */
  OMX_AUDIO_PARAM_AMRTYPE* pAudioAmr;
  OMX_AUDIO_PARAM_AACPROFILETYPE* pAudioAac;
  OMX_PARAM_COMPONENTROLETYPE * pComponentRole;
  OMX_U32 portIndex;

  /* Check which structure we are being fed and make control its header */
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_maddec_component_PrivateType* omx_maddec_component_Private = openmaxStandComp->pComponentPrivate;
  omx_base_component_PrivateType* omx_base_component_Private = openmaxStandComp->pComponentPrivate;
  omx_maddec_audio_PortType *port;
  omx_base_PortType *pPort;
  if (ComponentParameterStructure == NULL) {
    return OMX_ErrorBadParameter;
  }

  DEBUG(DEB_LEV_SIMPLE_SEQ, "   Setting parameter %i\n", nParamIndex);
  switch(nParamIndex) {
  case OMX_IndexParamAudioPortFormat:
    pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
    portIndex = pAudioPortFormat->nPortIndex;
    /*Check Structure Header and verify component state*/
    err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
    if(err!=OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "In %s Parameter Check Error=%x\n",__func__,err);
      break;
    }
    if (portIndex <= 1) {
      port = (omx_maddec_audio_PortType *) omx_maddec_component_Private->ports[portIndex];
      memcpy(&port->sAudioParam, pAudioPortFormat, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
    } else {
      return OMX_ErrorBadPortIndex;
    }
    break;

  case OMX_IndexParamAudioPcm:
    pAudioPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*)ComponentParameterStructure;
    portIndex = pAudioPcmMode->nPortIndex;
    /*Check Structure Header and verify component state*/
    err = omx_base_component_ParameterSanityCheck(hComponent, portIndex, pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
    if(err!=OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "In %s Parameter Check Error=%x\n",__func__,err);
      break;
    }
    memcpy(&omx_maddec_component_Private->pAudioPcmMode, pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
    /*
    xieyihao 2012-6-26 9:33:57
    In case that the codec will not be initialized by the first buffer with flag OMX_BUFFERFLAG_CODECCONFIG
    we need this samplerate prms to caculate the time stamp;
    */
    omx_maddec_component_Private->MusicInfo.sample_rate = pAudioPcmMode->nSamplingRate;
    break;

  case OMX_IndexParamStandardComponentRole:
    pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;

    if (omx_maddec_component_Private->state != OMX_StateLoaded && omx_maddec_component_Private->state != OMX_StateWaitForResources) {
      DEBUG(DEB_LEV_ERR, "In %s Incorrect State=%x lineno=%d\n",__func__,omx_maddec_component_Private->state,__LINE__);
      return OMX_ErrorIncorrectStateOperation;
    }

    if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
      break;
    }
	/*
	xieyihao 2012-6-18 16:16:05
	support three codecs: MP3 AAC AC3
	xieyihao 2012-7-11 11:00:55
	support all the codecs of Actions 
	*/
    if (!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_MP1_ROLE) ||  !strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_MP2_ROLE) || !strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_MP3_ROLE)) 
    {
      omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingMP3;
    }else if(!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_AAC_ROLE))
    {
      omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingAAC;	
    }else if(!strcmp( (char*) pComponentRole->cRole, AUDIO_DEC_AC3_ROLE))
    {
      omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingAC3;
    }else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_AMR_NB_ROLE))
    {
    	omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingAMR;	
    }else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_OGG_ROLE))
    {
    	omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingVORBIS;	
    }else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_WMASTD_ROLE))
    {
    	omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingWMA;	
    }else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_WMAPRO_ROLE))
    {
    	omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingWMAPRO;	
    }else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_WMALSL_ROLE))
    {
    	omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingWMALSL;	
    }else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_APE_ROLE))
    {
    	omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingAPE;	
    }else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_FLAC_ROLE))
    {
    	omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingFLAC;	
    }else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_RA_ROLE))
    {
    	omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingRA;	
    }else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_PCM_ROLE))
    {
    	omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingPCM;	
    }else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_ADPCM_ROLE))
    {
    	omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingADPCM;	
    }else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_DTS_ROLE))
    {
    	omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingDTS;	
    }                 
    else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_ACELP_ROLE))
    {
        omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingACELP;	  
    }
     else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_MPC_ROLE))
    {
        omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingMPC;	  
    }  
    else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_AIFF_ROLE))
    {
        omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingAIFF;	  
    }  
    else if(!strcmp((char*)pComponentRole->cRole, AUDIO_DEC_AMR_WB_ROLE))
    {
        omx_maddec_component_Private->audio_coding_type = OMX_AUDIO_CodingAWB;	  
    }               
   	else {
      return OMX_ErrorBadParameter;
    }
    omx_maddec_component_SetInternalParameters(openmaxStandComp);
    break;

  case OMX_IndexParamAudioMp3:
    pAudioMp3 = (OMX_AUDIO_PARAM_MP3TYPE*) ComponentParameterStructure;
    portIndex = pAudioMp3->nPortIndex;
    err = omx_base_component_ParameterSanityCheck(hComponent,portIndex,pAudioMp3,sizeof(OMX_AUDIO_PARAM_MP3TYPE));
    if(err!=OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "In %s Parameter Check Error=%x\n",__func__,err);
      break;
    }
    if (pAudioMp3->nPortIndex == 0) {
      memcpy(&omx_maddec_component_Private->pAudioMp3, pAudioMp3, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
    }  else {
      return OMX_ErrorBadPortIndex;
    }
    break;
    /*
    xieyihao 2012-6-18 16:42:09
    Add supported codecs: AAC & AC3
    */
  case OMX_IndexParamAudioAac:
  	pAudioAac = (OMX_AUDIO_PARAM_AACPROFILETYPE*) ComponentParameterStructure;
  	portIndex = pAudioAac->nPortIndex;
  	err = omx_base_component_ParameterSanityCheck(hComponent,portIndex,pAudioAac,sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
    if(err!=OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "In %s Parameter Check Error=%x\n",__func__,err);
      break;
    }
    if (pAudioAac->nPortIndex == 0) {
      memcpy(&omx_maddec_component_Private->pAudioAac, pAudioAac, sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
    }  else {
      return OMX_ErrorBadPortIndex;
    }
    break;
  /*
  kingson 2014-11-18 16:18:51
  add AC3 supported
 	*/  
  case OMX_IndexParamAudioAndroidAc3:
  	err = OMX_ErrorNone;
  	break;    
    /*
    kingson 2014-7-24 11:06:31
    Add supported codecs: AMR    
    */
	case OMX_IndexParamAudioAmr:
  	pAudioAmr = (OMX_AUDIO_PARAM_AMRTYPE*) ComponentParameterStructure;
  
    if (pAudioAmr->nPortIndex != 0) {
      return OMX_ErrorBadPortIndex;
    }
    return OMX_ErrorNone; 
  case OMX_IndexParamPortDefinition:
  	pPortDef  = (OMX_PARAM_PORTDEFINITIONTYPE*) ComponentParameterStructure;
    err = omx_base_component_ParameterSanityCheck(hComponent, pPortDef->nPortIndex, pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    if(err!=OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "In %s Parameter Check Error=%x\n",__func__,err);
      break;
    }
    {
      OMX_PARAM_PORTDEFINITIONTYPE *pPortParam;
      OMX_U32 j,old_nBufferCountActual=0;
      pPortParam = &omx_base_component_Private->ports[pPortDef->nPortIndex]->sPortParam;
      if(pPortDef->nBufferCountActual < pPortParam->nBufferCountMin) {
        DEBUG(DEB_LEV_ERR, "In %s nBufferCountActual of param (%i) is < of nBufferCountMin of port(%i)\n",__func__, (int)pPortDef->nBufferCountActual, (int)pPortParam->nBufferCountMin);
        err = OMX_ErrorBadParameter;
        break;
      }
      old_nBufferCountActual         = pPortParam->nBufferCountActual;
      pPortParam->nBufferCountActual = pPortDef->nBufferCountActual;
      /*
		xieyihao 2012-7-12 12:36:28
		fixme:
		in the omx spec, some fields of the OMX_PARAM_PORTDEFINITIONTYPE is read only
		but according to the Android omx client design flow, All of them should be writable
		how to solve the problem? would this cause any compatibility problem?
			
	  */
	  DEBUG(DEFAULT_MESSAGES, "set minimu size : %d\n", pPortDef->nBufferSize);
	  memcpy(pPortParam, pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	        /*If component state Idle/Pause/Executing and re-alloc the following private variables */
      if ((omx_base_component_Private->state == OMX_StateIdle ||
        omx_base_component_Private->state == OMX_StatePause  ||
        omx_base_component_Private->state == OMX_StateExecuting) &&
        (pPortParam->nBufferCountActual > old_nBufferCountActual)) {

        pPort = omx_base_component_Private->ports[pPortDef->nPortIndex];
        if(pPort->pInternalBufferStorage) {
          pPort->pInternalBufferStorage = realloc(pPort->pInternalBufferStorage,pPort->sPortParam.nBufferCountActual*sizeof(OMX_BUFFERHEADERTYPE *));
        }

        if(pPort->bBufferStateAllocated) {
          pPort->bBufferStateAllocated = realloc(pPort->bBufferStateAllocated,pPort->sPortParam.nBufferCountActual*sizeof(BUFFER_STATUS_FLAG));
          for(j=0; j < pPort->sPortParam.nBufferCountActual; j++) {
            pPort->bBufferStateAllocated[j] = BUFFER_FREE;
          }
        }
      }
    }
    break;
  default: /*Call the base component function*/
    return omx_base_component_SetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return err;

}

/** this function gets the parameters regarding audio formats and index */
OMX_ERRORTYPE omx_maddec_component_GetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_INOUT OMX_PTR ComponentParameterStructure)  {

  OMX_AUDIO_PARAM_PORTFORMATTYPE *pAudioPortFormat;
  OMX_AUDIO_PARAM_PCMMODETYPE *pAudioPcmMode;
  OMX_PARAM_COMPONENTROLETYPE *pComponentRole;
  OMX_AUDIO_PARAM_MP3TYPE *pAudioMp3;
  /*
  xieyihao 2012-6-18 16:51:31
  added supported AAC
  kingson  2014-7-24 10:30:32
  added supported AMR  
  */
  OMX_AUDIO_PARAM_AMRTYPE* pAudioAmr; 
  OMX_AUDIO_PARAM_AACPROFILETYPE* pAudioAac;
  omx_maddec_audio_PortType *port;
  OMX_ERRORTYPE err = OMX_ErrorNone;

  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_maddec_component_PrivateType* omx_maddec_component_Private = openmaxStandComp->pComponentPrivate;
  if (ComponentParameterStructure == NULL) {
    return OMX_ErrorBadParameter;
  }
  DEBUG(DEB_LEV_SIMPLE_SEQ, "   Getting parameter %i\n", nParamIndex);
  /* Check which structure we are being fed and fill its header */
  switch(nParamIndex) {
  case OMX_IndexParamAudioInit:
    if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE))) != OMX_ErrorNone) {
      break;
    }
    memcpy(ComponentParameterStructure, &omx_maddec_component_Private->sPortTypesParam[OMX_PortDomainAudio], sizeof(OMX_PORT_PARAM_TYPE));
    break;

  case OMX_IndexParamAudioPortFormat:
    pAudioPortFormat = (OMX_AUDIO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
    if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE))) != OMX_ErrorNone) {
      break;
    }
    if (pAudioPortFormat->nPortIndex <= 1) {
      port = (omx_maddec_audio_PortType *)omx_maddec_component_Private->ports[pAudioPortFormat->nPortIndex];
      memcpy(pAudioPortFormat, &port->sAudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
    } else {
      return OMX_ErrorBadPortIndex;
    }
    break;

  case OMX_IndexParamAudioPcm:
    pAudioPcmMode = (OMX_AUDIO_PARAM_PCMMODETYPE*)ComponentParameterStructure;
    if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE))) != OMX_ErrorNone) {
      break;
    }
    if (pAudioPcmMode->nPortIndex > 1) {
      return OMX_ErrorBadPortIndex;
    }
    memcpy(pAudioPcmMode, &omx_maddec_component_Private->pAudioPcmMode, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
    break;

  case OMX_IndexParamAudioMp3:
    pAudioMp3 = (OMX_AUDIO_PARAM_MP3TYPE*)ComponentParameterStructure;
    if (pAudioMp3->nPortIndex != 0) {
      return OMX_ErrorBadPortIndex;
    }
    if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_MP3TYPE))) != OMX_ErrorNone) {
      break;
    }
    memcpy(pAudioMp3, &omx_maddec_component_Private->pAudioMp3, sizeof(OMX_AUDIO_PARAM_MP3TYPE));
    break;
  /*
  xieyihao 2012-6-18 16:46:46
  add AAC & AC3 supported
 	*/
  case OMX_IndexParamAudioAac:
  	pAudioAac = (OMX_AUDIO_PARAM_AACPROFILETYPE*) ComponentParameterStructure;
  
    if (pAudioAac->nPortIndex != 0) {
      return OMX_ErrorBadPortIndex;
    }
    if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE))) != OMX_ErrorNone) {
      break;
    }
    memcpy(pAudioAac, &omx_maddec_component_Private->pAudioAac, sizeof(OMX_AUDIO_PARAM_AACPROFILETYPE));
    break;
  /*
  kingson 2014-11-18 16:15:51
  add AC3 supported
 	*/  
  case OMX_IndexParamAudioAndroidAc3:
  	break;
  /*
  kingson 2014-7-24 10:32:51
  add AMR supported
 	*/
  case OMX_IndexParamAudioAmr:
  	pAudioAmr = (OMX_AUDIO_PARAM_AMRTYPE*) ComponentParameterStructure;
  	
  	if (pAudioAmr->nPortIndex != 0) {
      return OMX_ErrorBadPortIndex;
    }
    if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_AUDIO_PARAM_AMRTYPE))) != OMX_ErrorNone) {
      break;
    }
    memcpy(pAudioAmr, &omx_maddec_component_Private->pAudioAmr, sizeof(OMX_AUDIO_PARAM_AMRTYPE));
    break;
    	
  case OMX_IndexParamStandardComponentRole:
    pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)ComponentParameterStructure;
    if ((err = checkHeader(ComponentParameterStructure, sizeof(OMX_PARAM_COMPONENTROLETYPE))) != OMX_ErrorNone) {
      break;
    }
    /*
    xieyihao 2012-6-18 16:43:21
    added supported codecs: AAC & AC3
    */
    if (omx_maddec_component_Private->audio_coding_type == OMX_AUDIO_CodingMP3) 
    {
      strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_MP3_ROLE);
    }
    else if(omx_maddec_component_Private->audio_coding_type == OMX_AUDIO_CodingAAC)
    {
    	strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_AAC_ROLE);
    	
    }else if(omx_maddec_component_Private->audio_coding_type == OMX_AUDIO_CodingAC3)
    {
    	strcpy( (char*) pComponentRole->cRole, AUDIO_DEC_AC3_ROLE);		
    }
    else 
    {
      strcpy( (char*) pComponentRole->cRole,"\0");;
    }
    break;
  default: /*Call the base component function*/
    return omx_base_component_GetParameter(hComponent, nParamIndex, ComponentParameterStructure);
  }
  return OMX_ErrorNone;

}

/** message handling of mad decoder */
OMX_ERRORTYPE omx_mad_decoder_MessageHandler(OMX_COMPONENTTYPE* openmaxStandComp, internalRequestMessageType *message)  {

  omx_maddec_component_PrivateType* omx_maddec_component_Private = (omx_maddec_component_PrivateType*)openmaxStandComp->pComponentPrivate;
  OMX_ERRORTYPE err;
  OMX_STATETYPE eCurrentState = omx_maddec_component_Private->state;
  DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s\n", __func__);

  if (message->messageType == OMX_CommandStateSet){
    if ((message->messageParam == OMX_StateIdle) && (omx_maddec_component_Private->state == OMX_StateLoaded)) {
      err = omx_maddec_component_Init(openmaxStandComp);
      if(err!=OMX_ErrorNone) {
        DEBUG(DEB_LEV_ERR, "In %s MAD Decoder Init Failed Error=%x\n",__func__,err);
        return err;
      }
    } else if ((message->messageParam == OMX_StateExecuting) && (omx_maddec_component_Private->state == OMX_StateIdle)) {
      DEBUG(DEB_LEV_FULL_SEQ, "State Changing from Idle to Exec\n");
			
			DEBUG(DEB_LEV_SIMPLE_SEQ, "in %s, line %d\n", __func__, __LINE__);
    }
  }
  /** Execute the base message handling */
  err = omx_base_component_MessageHandler(openmaxStandComp, message);

  if (message->messageType == OMX_CommandStateSet){
    if ((message->messageParam == OMX_StateLoaded) && (eCurrentState == OMX_StateIdle)) {
      err = omx_maddec_component_Deinit(openmaxStandComp);
      if(err!=OMX_ErrorNone) {
        DEBUG(DEB_LEV_ERR, "In %s MAD Decoder Deinit Failed Error=%x\n",__func__,err);
        return err;
      }
    }else if ((message->messageParam == OMX_StateIdle) && (eCurrentState == OMX_StateExecuting)) {
    	if(omx_maddec_component_Private->maddecReady == OMX_TRUE)
    	{
      		omx_maddec_component_madLibDeInit(omx_maddec_component_Private);
      		omx_maddec_component_Private->maddecReady = OMX_FALSE;
      	}
    }
  }else if(message->messageType == OMX_CommandFlush)
  {
  	/*
  	xieyihao 2012-6-21 16:11:55
  	if flush command, then reset the decoder lib
  	*/
  	DEBUG(DEB_LEV_FULL_SEQ,"in %s, reset decoder\n",__func__);
  	if(omx_maddec_component_Private->pPluginInfo){
  		omx_maddec_component_Private->pPluginInfo->ex_ops(omx_maddec_component_Private->pCodecHandle,EX_OPS_CHUNK_RESET,0); 
  	}
  	
  	DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s check if to signal flush done\n", __func__);
  	
  	if(message->messageParam == OMX_ALL)
  	{
  		DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s to signal flush done for first ports\n", __func__);
		tsem_up(omx_maddec_component_Private->flushSyncSem);
		DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s to signal flush done for second ports\n", __func__);
		tsem_up(omx_maddec_component_Private->flushSyncSem);	
	}
	else
	{
		DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s to signal flush done for single port\n", __func__);
		tsem_up(omx_maddec_component_Private->flushSyncSem);		
	}
	
	
	DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s check if signal flush out\n", __func__);
	
  }

  return err;
}

/** This is the central function for component processing. It
  * is executed in a separate thread, is synchronized with
  * semaphores at each port, those are released each time a new buffer
  * is available on the given port.
  */
void* omx_maddec_component_BufferMgmtFunction (void* param) {
  OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;
  omx_base_component_PrivateType* omx_base_component_Private=(omx_base_component_PrivateType*)openmaxStandComp->pComponentPrivate;
  omx_base_filter_PrivateType* omx_base_filter_Private = (omx_base_filter_PrivateType*)omx_base_component_Private;
  omx_base_PortType *pInPort=(omx_base_PortType *)omx_base_filter_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
  omx_base_PortType *pOutPort=(omx_base_PortType *)omx_base_filter_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
  tsem_t* pInputSem = pInPort->pBufferSem;
  tsem_t* pOutputSem = pOutPort->pBufferSem;
  queue_t* pInputQueue = pInPort->pBufferQueue;
  queue_t* pOutputQueue = pOutPort->pBufferQueue;
  OMX_BUFFERHEADERTYPE* pOutputBuffer=NULL;
  OMX_BUFFERHEADERTYPE* pInputBuffer=NULL;
  OMX_BOOL isInputBufferNeeded=OMX_TRUE,isOutputBufferNeeded=OMX_TRUE;
  int inBufExchanged=0,outBufExchanged=0;

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
  while(omx_base_filter_Private->state == OMX_StateIdle || omx_base_filter_Private->state == OMX_StateExecuting ||  omx_base_filter_Private->state == OMX_StatePause ||
    omx_base_filter_Private->transientState == OMX_TransStateLoadedToIdle){

    /*Wait till the ports are being flushed*/
    pthread_mutex_lock(&omx_base_filter_Private->flush_mutex);
    while( PORT_IS_BEING_FLUSHED(pInPort) ||
           PORT_IS_BEING_FLUSHED(pOutPort)) {
      pthread_mutex_unlock(&omx_base_filter_Private->flush_mutex);

      DEBUG(DEB_LEV_FULL_SEQ, "In %s 1 signalling flush all cond iE=%d,iF=%d,oE=%d,oF=%d iSemVal=%d,oSemval=%d\n",
        __func__,inBufExchanged,isInputBufferNeeded,outBufExchanged,isOutputBufferNeeded,pInputSem->semval,pOutputSem->semval);

      if(isOutputBufferNeeded==OMX_FALSE && PORT_IS_BEING_FLUSHED(pOutPort)) {
        pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);
        outBufExchanged--;
        pOutputBuffer=NULL;
        isOutputBufferNeeded=OMX_TRUE;
        DEBUG(DEB_LEV_FULL_SEQ, "Ports are flushing,so returning output buffer\n");
      }

      if(isInputBufferNeeded==OMX_FALSE && PORT_IS_BEING_FLUSHED(pInPort)) {
        pInPort->ReturnBufferFunction(pInPort,pInputBuffer);
        inBufExchanged--;
        pInputBuffer=NULL;
        isInputBufferNeeded=OMX_TRUE;
        DEBUG(DEB_LEV_FULL_SEQ, "Ports are flushing,so returning input buffer\n");
      }

      DEBUG(DEB_LEV_FULL_SEQ, "In %s 2 signalling flush all cond iE=%d,iF=%d,oE=%d,oF=%d iSemVal=%d,oSemval=%d\n",
        __func__,inBufExchanged,isInputBufferNeeded,outBufExchanged,isOutputBufferNeeded,pInputSem->semval,pOutputSem->semval);

      tsem_up(omx_base_filter_Private->flush_all_condition);
      tsem_down(omx_base_filter_Private->flush_condition);
      pthread_mutex_lock(&omx_base_filter_Private->flush_mutex);
    }
    pthread_mutex_unlock(&omx_base_filter_Private->flush_mutex);

    /*No buffer to process. So wait here*/
    if((isInputBufferNeeded==OMX_TRUE && pInputSem->semval==0) &&
      (omx_base_filter_Private->state != OMX_StateLoaded && omx_base_filter_Private->state != OMX_StateInvalid)) {
      //Signalled from EmptyThisBuffer or FillThisBuffer or some thing else
      DEBUG(DEB_LEV_FULL_SEQ, "Waiting for next input/output buffer\n");
      tsem_down(omx_base_filter_Private->bMgmtSem);

    }
    if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid) {
      DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Buffer Management Thread is exiting\n",__func__);
      break;
    }
    if((isOutputBufferNeeded==OMX_TRUE && pOutputSem->semval==0) &&
      (omx_base_filter_Private->state != OMX_StateLoaded && omx_base_filter_Private->state != OMX_StateInvalid) &&
       !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
      //Signalled from EmptyThisBuffer or FillThisBuffer or some thing else
      DEBUG(DEB_LEV_FULL_SEQ, "Waiting for next input/output buffer\n");
      tsem_down(omx_base_filter_Private->bMgmtSem);

    }
    if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid) {
      DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Buffer Management Thread is exiting\n",__func__);
      break;
    }

    DEBUG(DEB_LEV_SIMPLE_SEQ, "Waiting for input buffer semval=%d \n",pInputSem->semval);
    if(pInputSem->semval>0 && isInputBufferNeeded==OMX_TRUE ) {
      tsem_down(pInputSem);
      if(pInputQueue->nelem>0){
        inBufExchanged++;
        isInputBufferNeeded=OMX_FALSE;
        pInputBuffer = dequeue(pInputQueue);
        if(pInputBuffer == NULL){
          DEBUG(DEB_LEV_ERR, "Had NULL input buffer!!\n");
          break;
        }
      }
    }
    /*When we have input buffer to process then get one output buffer*/
    if(pOutputSem->semval>0 && isOutputBufferNeeded==OMX_TRUE) {
      tsem_down(pOutputSem);
      if(pOutputQueue->nelem>0){
        outBufExchanged++;
        isOutputBufferNeeded=OMX_FALSE;
        pOutputBuffer = dequeue(pOutputQueue);
        if(pOutputBuffer == NULL){
          DEBUG(DEB_LEV_ERR, "Had NULL output buffer!! op is=%d,iq=%d\n",pOutputSem->semval,pOutputQueue->nelem);
          break;
        }
      }
    }

    if(isInputBufferNeeded==OMX_FALSE) {
      if(pInputBuffer->hMarkTargetComponent != NULL){
        if((OMX_COMPONENTTYPE*)pInputBuffer->hMarkTargetComponent ==(OMX_COMPONENTTYPE *)openmaxStandComp) {
          /*Clear the mark and generate an event*/
          (*(omx_base_filter_Private->callbacks->EventHandler))
            (openmaxStandComp,
            omx_base_filter_Private->callbackData,
            OMX_EventMark, /* The command was completed */
            1, /* The commands was a OMX_CommandStateSet */
            0, /* The state has been changed in message->messageParam2 */
            pInputBuffer->pMarkData);
        } else {
          /*If this is not the target component then pass the mark*/
          omx_base_filter_Private->pMark.hMarkTargetComponent = pInputBuffer->hMarkTargetComponent;
          omx_base_filter_Private->pMark.pMarkData            = pInputBuffer->pMarkData;
        }
        pInputBuffer->hMarkTargetComponent = NULL;
      }
    }

    if(isInputBufferNeeded==OMX_FALSE && isOutputBufferNeeded==OMX_FALSE) {

      if(omx_base_filter_Private->pMark.hMarkTargetComponent != NULL){
        pOutputBuffer->hMarkTargetComponent = omx_base_filter_Private->pMark.hMarkTargetComponent;
        pOutputBuffer->pMarkData            = omx_base_filter_Private->pMark.pMarkData;
        omx_base_filter_Private->pMark.hMarkTargetComponent = NULL;
        omx_base_filter_Private->pMark.pMarkData            = NULL;
      }

      pOutputBuffer->nTimeStamp = pInputBuffer->nTimeStamp;
	/* that nFlags fields may not only set the STARTTIME bit, so we need noly check the bit accordingly */
      if(pInputBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) {
         DEBUG(DEB_LEV_FULL_SEQ, "Detected  START TIME flag in the input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
         pOutputBuffer->nFlags = pInputBuffer->nFlags;
         pInputBuffer->nFlags = 0;
      }

      if(omx_base_filter_Private->state == OMX_StateExecuting)  {
        if (omx_base_filter_Private->BufferMgmtCallback /*&& pInputBuffer->nFilledLen > 0(the last buffer may be NULL)*/) {
          (*(omx_base_filter_Private->BufferMgmtCallback))(openmaxStandComp, pInputBuffer, pOutputBuffer);
        } else {
          /*It no buffer management call back the explicitly consume input buffer*/
          pInputBuffer->nFilledLen = 0;
        }
      } else if(!(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
        DEBUG(DEB_LEV_ERR, "In %s Received Buffer in non-Executing State(%x)\n", __func__, (int)omx_base_filter_Private->state);
      } else {
          pInputBuffer->nFilledLen = 0;
      }
			/*xieyihao 2012-7-11 11:02:09, the client may set this flag with operator "|", not "=" */
      if((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) && pInputBuffer->nFilledLen==0) {
        DEBUG(DEB_LEV_FULL_SEQ, "Detected EOS flags in input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
        pOutputBuffer->nFlags=pInputBuffer->nFlags;
        pInputBuffer->nFlags=0;
        (*(omx_base_filter_Private->callbacks->EventHandler))
          (openmaxStandComp,
          omx_base_filter_Private->callbackData,
          OMX_EventBufferFlag, /* The command was completed */
          1, /* The commands was a OMX_CommandStateSet */
          pOutputBuffer->nFlags, /* The state has been changed in message->messageParam2 */
          NULL);
        omx_base_filter_Private->bIsEOSReached = OMX_TRUE;
      }
      if(omx_base_filter_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
        /*Waiting at paused state*/
        tsem_wait(omx_base_component_Private->bStateSem);
      }

      /*If EOS and Input buffer Filled Len Zero then Return output buffer immediately*/
      /*xieyihao 2012-7-11 11:02:09, the client may set this flag with operator "|", not "="*/
      if((pOutputBuffer->nFilledLen != 0) || (pOutputBuffer->nFlags & OMX_BUFFERFLAG_EOS) || (omx_base_filter_Private->bIsEOSReached == OMX_TRUE)) {
        if(omx_base_filter_Private->bIsEOSReached == OMX_TRUE){
        	omx_base_filter_Private->bIsEOSReached = OMX_FALSE;
        }
        pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);
        outBufExchanged--;
        pOutputBuffer=NULL;
        isOutputBufferNeeded=OMX_TRUE;
      }
    }

    if(omx_base_filter_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
      /*Waiting at paused state*/
      tsem_wait(omx_base_component_Private->bStateSem);
    }

    /*Input Buffer has been completely consumed. So, return input buffer*/
    /*xieyihao 2012-7-11 11:01:30 for the last NULL input to decoder ,specificated for Actions Audio codecs*/
    if((isInputBufferNeeded == OMX_FALSE) && (pInputBuffer->nFilledLen==0) && !(pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS)) {
      pInPort->ReturnBufferFunction(pInPort,pInputBuffer);
      inBufExchanged--;
      pInputBuffer=NULL;
      isInputBufferNeeded=OMX_TRUE;
    }
  }
  DEBUG(DEB_LEV_SIMPLE_SEQ,"Exiting Buffer Management Thread\n");
  return NULL;
}

OMX_ERRORTYPE omx_maddec_component_SendCommand(
  OMX_HANDLETYPE hComponent,
  OMX_COMMANDTYPE Cmd,
  OMX_U32 nParam,
  OMX_PTR pCmdData) {
   OMX_COMPONENTTYPE* omxComponent = (OMX_COMPONENTTYPE*)hComponent;
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
  omx_maddec_component_PrivateType* omx_maddec_component_Private = (omx_maddec_component_PrivateType*)omxComponent->pComponentPrivate;
  internalRequestMessageType *message;
  queue_t* messageQueue;
  tsem_t* messageSem;
  OMX_U32 i,j,k;
  omx_base_PortType *pPort;
  OMX_ERRORTYPE err = OMX_ErrorNone;
  int errQue;
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for component %p\n", __func__, hComponent);

  messageQueue = omx_base_component_Private->messageQueue;
  messageSem = omx_base_component_Private->messageSem;
  if (omx_base_component_Private->state == OMX_StateInvalid) {
    return OMX_ErrorInvalidState;
  }

  message = calloc(1,sizeof(internalRequestMessageType));
  message->messageParam = nParam;
  message->pCmdData=pCmdData;
  if(Cmd ==  OMX_CommandFlush)
  {
    if (nParam >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                   omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                   omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                   omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts) && nParam != OMX_ALL) {
      return OMX_ErrorBadPortIndex;
    }
    pthread_mutex_lock(&omx_maddec_component_Private->access_mutex);
    omx_maddec_component_Private->bFlushSync = OMX_TRUE;
    pthread_mutex_unlock(&omx_maddec_component_Private->access_mutex);
    message->messageType = OMX_CommandFlush;
    errQue = queue(messageQueue, message);
    if (errQue) {
      /* /TODO the queue is full. This can be handled in a fine way with
       * some retrials, or other checking. For the moment this is a critical error
       * and simply causes the failure of this call
       */
      return OMX_ErrorInsufficientResources;
    }
    tsem_up(messageSem);
  }else 
  {
  	err =  omx_base_component_SendCommand(hComponent, Cmd, nParam, pCmdData);	
  }
  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for component %p\n", __func__, hComponent);
  return err;


}

OMX_ERRORTYPE omx_maddec_component_EmptyThisBuffer(
		OMX_HANDLETYPE hComponent,
		OMX_BUFFERHEADERTYPE* pBuffer) {
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
   omx_maddec_component_PrivateType* omx_maddec_component_Private = (omx_maddec_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
  omx_base_PortType *pPort;
  OMX_ERRORTYPE err;
  
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for component %p\n", __func__, hComponent);

  if (pBuffer->nInputPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                                   omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                                   omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                                   omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
    DEBUG(DEB_LEV_ERR, "In %s: wrong port index\n", __func__);
    return OMX_ErrorBadPortIndex;
  }
  pPort = omx_base_component_Private->ports[pBuffer->nInputPortIndex];
  if (pPort->sPortParam.eDir != OMX_DirInput) {
    DEBUG(DEB_LEV_ERR, "In %s: wrong port direction in Component %s\n", __func__,omx_base_component_Private->name);
    return OMX_ErrorBadPortIndex;
  }
  DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s check if need wait flush \n", __func__);
  pthread_mutex_lock(&omx_maddec_component_Private->access_mutex);
  if(omx_maddec_component_Private->bFlushSync == OMX_TRUE)
  {
  	DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s wait for first port flush done\n", __func__);
  	tsem_down(omx_maddec_component_Private->flushSyncSem);
  	DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s wait for second portflush done\n", __func__);
  	tsem_down(omx_maddec_component_Private->flushSyncSem);
  	omx_maddec_component_Private->bFlushSync = OMX_FALSE;
  	omx_maddec_component_Private->bFlush = OMX_TRUE;
  	
  }
  pthread_mutex_unlock(&omx_maddec_component_Private->access_mutex);
  DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s check wait flush out\n", __func__);
  
  err = pPort->Port_SendBufferFunction(pPort, pBuffer);
  if (err != OMX_ErrorNone) {
	  DEBUG(DEB_LEV_ERR, "Out of %s for component %p with err %d\n", __func__, hComponent, err);
	  return err;
  }
  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for component %p\n", __func__, hComponent);
  return OMX_ErrorNone;
}

