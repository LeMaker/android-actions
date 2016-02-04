/**
  @file src/components/ffmpeg/omx_videodec_component.c
  
  This component implements H.264 / MPEG-4 AVC video decoder. 
  The H.264 / MPEG-4 AVC Video decoder is based on the FFmpeg software library.

  Copyright (C) 2007-2008 STMicroelectronics
  Copyright (C) 2007-2008 Nokia Corporation and/or its subsidiary(-ies)

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

  $Date: 2008-08-29 06:10:33 +0200 (Fri, 29 Aug 2008) $
  Revision $Rev: 584 $
  Author $Author: pankaj_sen $
*/

#include <omxcore.h>
#include <omx_base_video_port.h>
#include <omx_actvideodec_component_4k.h>
#include <OMX_Video.h>
#include <ACT_OMX_Index.h>
#include <sys/prctl.h>
#include <cutils/properties.h>
#include "Actions_OSAL_Android.h"
#include "vce_resize.h"
#include <ACT_OMX_IVCommon.h>
#include "Igralloc.h"

/** Maximum Number of Video Component Instance*/
#define MAX_COMPONENT_VIDEODEC 4

/** Counter of Video Component Instance*/
static OMX_U32 noVideoDecInstance = 0;
static OMX_U32 VIDDEC_DEFAULT_FRAME_BUFFER_NUM = 6;
static OMX_U32 VIDDEC_DEFAULT_INPUT_BUFFER_SIZE = 2*1024*1024;
static OMX_U32 VIDDEC_DEFAULT_INPUT_BUFFER_NUM = 4;
static OMX_U32 VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM = 6;

/** The output decoded color format */
#define OUTPUT_DECODED_COLOR_FMT OMX_COLOR_FormatYUV420SemiPlanar 

#define DEFAULT_WIDTH 1920   
#define DEFAULT_HEIGHT 1080   
#define VDE_DECODER_ALIGN 31
/** define the max input buffer size */   
#define DEFAULT_VIDEO_OUTPUT_BUF_SIZE DEFAULT_WIDTH*DEFAULT_HEIGHT*3/2   // YUV 420P 
#define ACTIONS_SYS_MEM


/* H.263 Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE SupportedH263ProfileLevels[] = {
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level40},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level50},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level60},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70},
  {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level10},
  {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level20},
  {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level30},
  {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level40},
  {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level50},
  {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level60},
  {OMX_VIDEO_H263ProfileISWV2, OMX_VIDEO_H263Level70},
  {-1, -1}};


/* MPEG4 Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE SupportedMPEG4ProfileLevels[] = {
	{OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0},
	  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0b},
	  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level1},
	  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level2},
	  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level3},
	  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4},
	  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4a},
	  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level5},
	  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0},
	  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0b},
	  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level1},
	  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level2},
	  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level3},
	  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4},
	  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5},
	  {-1,-1}};


/* AVC Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE SupportedAVCProfileLevels[] ={
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel5},
	{OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel41},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel5},
	{OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel41},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel5},
	{OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51},
	{-1,-1}};





/** The Constructor of the video decoder component
  * @param openmaxStandComp the component handle to be constructed
  * @param cComponentName is the name of the constructed component
  */
OMX_ERRORTYPE OMX_ComponentInit(OMX_COMPONENTTYPE *openmaxStandComp,OMX_STRING cComponentName) 
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_COMPONENTTYPE *pHandle = NULL;
	VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
	//OMX_STRING cComponentName = "OMX.Action.Video.Decoder";
	omx_base_video_PortType *inPort,*outPort;
	OMX_U32 i;
	VR_Input_t vr_input_param;
	
	char memory_property[PROPERTY_VALUE_MAX] = "false";
  property_get("ro.config.low_ram", memory_property, "false");
  DEBUG(DEB_LEV_ERR,"low_ram:%s\n",memory_property);

 

  pHandle = openmaxStandComp;  	
  pHandle->pComponentPrivate = calloc(1, sizeof(VIDDEC_COMPONENT_PRIVATE));
  	
	if(pHandle->pComponentPrivate==NULL)  {
		return OMX_ErrorInsufficientResources;
	}
	pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;
	pComponentPrivate->ports = NULL;

  	eError = omx_base_filter_Constructor(pHandle, cComponentName);
  
  	pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber = 0;
 	pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nPorts = 2;
 	
	/** Allocate Ports and call port constructor. */
	if (pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nPorts && !pComponentPrivate->ports) 
	{
		pComponentPrivate->ports = calloc(pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nPorts, sizeof(omx_base_video_PortType *));
		if (!pComponentPrivate->ports) 
		{
		  return OMX_ErrorInsufficientResources;
		}
		for (i=0; i < pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++) {
		  pComponentPrivate->ports[i] = calloc(1, sizeof(omx_base_video_PortType));
		  if (!pComponentPrivate->ports[i]) {
		    return OMX_ErrorInsufficientResources;
		  }
		}
	}
	
	base_video_port_Constructor(pHandle, &pComponentPrivate->ports[0], 0, OMX_TRUE);
	base_video_port_Constructor(pHandle, &pComponentPrivate->ports[1], 1, OMX_FALSE);

    //common parameters related to input port
	inPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	inPort->sPortParam.nBufferSize = 800*1024;
	inPort->sPortParam.format.video.xFramerate = 30;
	inPort->sPortParam.nBufferCountActual = VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
	inPort->sPortParam.nBufferCountMin = VIDDEC_MIN_INPUT_BUFFER_NUM;
	
	

  //common parameters related to output port
	outPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	outPort->sPortParam.format.video.eColorFormat = OUTPUT_DECODED_COLOR_FMT;
	outPort->sPortParam.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUF_SIZE;
	outPort->sPortParam.format.video.xFramerate = 30;

  /** settings of output port parameter definition */
	outPort->sVideoParam.eColorFormat = OUTPUT_DECODED_COLOR_FMT;
	outPort->sVideoParam.xFramerate = 30;	
	
	outPort->sPortParam.nBufferCountActual = VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
  outPort->sPortParam.nBufferCountMin = VIDDEC_MIN_OUTPUT_BUFFER_NUM;
	

	

  /** general configuration irrespective of any video formats
    * setting other parameters of omx_videodec_component_private  
    */
  
  memset(&vr_input_param,0,sizeof(VR_Input_t));
  vr_input_param.dstw_align = ALIGN_32PIXELS;
  pComponentPrivate->vce_handle = VceReSize_Open(&vr_input_param);
  pthread_mutex_init(&pComponentPrivate->PortFlushMutex,NULL);
  
  pComponentPrivate->decodedSem=(tsem_t*)calloc(1,sizeof(tsem_t));
  pComponentPrivate->mDoneSem=(tsem_t*)calloc(1,sizeof(tsem_t));
  tsem_init(pComponentPrivate->decodedSem, 0);
	tsem_init(pComponentPrivate->mDoneSem, 0);
 

    pComponentPrivate->bComponentFlush = OMX_FALSE;
	pComponentPrivate->avcodecReady = OMX_FALSE;
	pComponentPrivate->isFirstBuffer = 1;
	pComponentPrivate->p_so_handle = NULL;
	pComponentPrivate->p_interface = NULL;
    pComponentPrivate->p_handle = NULL;	
	pComponentPrivate->frame_num = 0;
	pComponentPrivate->is_error_status=OMX_FALSE;
	pComponentPrivate->is_fifo_disposing=OMX_FALSE;
	pComponentPrivate->is_videodec_fifo_init=OMX_FALSE;
	
	pComponentPrivate->bStoreMediadata[OMX_BASE_FILTER_INPUTPORT_INDEX]=OMX_FALSE;
	pComponentPrivate->bStoreMediadata[OMX_BASE_FILTER_OUTPUTPORT_INDEX]=OMX_FALSE;
	pComponentPrivate->bOutPutBufferAllocated = BUFFER_FREE;
	pComponentPrivate->IsGraphicBuffer=OMX_FALSE;
	
	pComponentPrivate->actualframewidth = 0;
	pComponentPrivate->actualframeheight = 0;
	pComponentPrivate->actualdisplaywidth = 0;
	pComponentPrivate->actualdisplayheight = 0;

	pComponentPrivate->is_cur_stream_buf=OMX_FALSE;
	pComponentPrivate->is_end_of_stream=OMX_FALSE;
	pComponentPrivate->is_Thumbnail=OMX_FALSE;
	pComponentPrivate->err_counter=0;
	pComponentPrivate->is_avc_Thumbnail_outputed=OMX_FALSE;
	pComponentPrivate->is_outputbuffer_flushing_pending=OMX_FALSE;
	pComponentPrivate->is_inputbuffer_flushing_pending=OMX_FALSE;
	pComponentPrivate->output_portformat_inited = OMX_FALSE;
	pComponentPrivate->IsProbed = OMX_FALSE;
	pComponentPrivate->IsPortSettingChanged =OMX_FALSE;
	pComponentPrivate->bConfigPktStored = OMX_FALSE;	
	pComponentPrivate->bthirdparty = OMX_FALSE;
	pComponentPrivate->suspend_flag=0;
	pComponentPrivate->nNewBufferCountActual=0;
	pComponentPrivate->PortReSettingReady =OMX_FALSE;
	pComponentPrivate->CodecConfigPktLen=0;	
	pComponentPrivate->OutPutThreadID=-1;
	pComponentPrivate->bLowRam = OMX_FALSE;
	if(!strcmp(memory_property,"true")){
  	 pComponentPrivate->bLowRam = OMX_TRUE;
  	 VIDDEC_DEFAULT_INPUT_BUFFER_NUM = 2;
  	 VIDDEC_DEFAULT_FRAME_BUFFER_NUM = 4;
  	 VIDDEC_DEFAULT_INPUT_BUFFER_SIZE = 1024*1024; 
  }
  
	pComponentPrivate->CodecConfigPkt = (OMX_U8*)actal_malloc(16*1024);
	if(pComponentPrivate->CodecConfigPkt==NULL){
	  return OMX_ErrorInsufficientResources;
    }
	pComponentPrivate->pstrbuf=(OMX_U8*)actal_malloc(2*1024*1024);
	if(pComponentPrivate->pstrbuf==NULL){
	  return OMX_ErrorInsufficientResources;
	}
  	pComponentPrivate->pPortParamType=(OMX_PORT_PARAM_TYPE*)actal_malloc(sizeof(OMX_PORT_PARAM_TYPE));
	if(pComponentPrivate->pPortParamType==NULL){
	return OMX_ErrorInsufficientResources;
	}
  	actal_memset(pComponentPrivate->pPortParamType,0,sizeof(OMX_PORT_PARAM_TYPE));
  	pComponentPrivate->pPortParamTypeAudio=(OMX_PORT_PARAM_TYPE*)actal_malloc(sizeof(OMX_PORT_PARAM_TYPE));
	if(pComponentPrivate->pPortParamTypeAudio==NULL){
	return OMX_ErrorInsufficientResources;
	}
  	actal_memset(pComponentPrivate->pPortParamTypeAudio,0,sizeof(OMX_PORT_PARAM_TYPE));
  	pComponentPrivate->pPortParamTypeImage=(OMX_PORT_PARAM_TYPE*)actal_malloc(sizeof(OMX_PORT_PARAM_TYPE));
	if(pComponentPrivate->pPortParamTypeImage==NULL){
	return OMX_ErrorInsufficientResources;
	}
  	actal_memset(pComponentPrivate->pPortParamTypeImage,0,sizeof(OMX_PORT_PARAM_TYPE));
  	pComponentPrivate->pPortParamTypeOthers=(OMX_PORT_PARAM_TYPE*)actal_malloc(sizeof(OMX_PORT_PARAM_TYPE));
	if(pComponentPrivate->pPortParamTypeOthers==NULL){
	return OMX_ErrorInsufficientResources;
	}
  	memset(pComponentPrivate->pPortParamTypeOthers,0,sizeof(OMX_PORT_PARAM_TYPE));


  /** initializing the codec context etc that was done earlier by ffmpeglibinit function */
 	pComponentPrivate->messageHandler = omx_videodec_component_MessageHandler;
	pComponentPrivate->BufferMgmtFunction =omx_videodec_component_BufferMgmtFunction;
	pComponentPrivate->BufferMgmtCallback = omx_videodec_component_BufferMgmtCallback;


	pComponentPrivate->destructor = omx_videodec_component_Destructor;
	pHandle->SetParameter = omx_videodec_component_SetParameter;
	pHandle->GetParameter = omx_videodec_component_GetParameter;
	pHandle->FillThisBuffer = omx_videodec_component_FillThisBuffer;
	pHandle->SetConfig    = omx_videodec_component_SetConfig;
	pHandle->GetConfig    = omx_videodec_component_GetConfig;
	pHandle->ComponentRoleEnum = omx_videodec_component_ComponentRoleEnum;
	pHandle->GetExtensionIndex = omx_videodec_component_GetExtensionIndex;
	pHandle->AllocateBuffer = omx_videodec_component_AllocateBuffer;
	pHandle->UseBuffer  = omx_videodec_component_UseBuffer;	
	pHandle->FillThisBuffer = omx_videodec_component_FillThisBuffer;
	pHandle->FreeBuffer = omx_videodec_component_FreeBuffer;


	noVideoDecInstance++;

	if(noVideoDecInstance > MAX_COMPONENT_VIDEODEC) {
		return OMX_ErrorInsufficientResources;
	}
	
	return eError;
}


/** The destructor of the video decoder component
  */
OMX_ERRORTYPE omx_videodec_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp) 
{
	OMX_COMPONENTTYPE *pHandle = NULL;
	VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
	pHandle= openmaxStandComp;
	pComponentPrivate = pHandle->pComponentPrivate;
	
	OMX_U32 i;

	DEBUG(DEB_LEV_FULL_SEQ,"omx_videodec_component_ComponentDeInit 0\n");	  	
	if(pComponentPrivate->pPortParamType) {
        free(pComponentPrivate->pPortParamType);
        pComponentPrivate->pPortParamType = NULL;
  }
  if(pComponentPrivate->pPortParamTypeAudio) {
        free(pComponentPrivate->pPortParamTypeAudio);
        pComponentPrivate->pPortParamTypeAudio = NULL;
  }
  if(pComponentPrivate->pPortParamTypeImage) {
        free(pComponentPrivate->pPortParamTypeImage);
        pComponentPrivate->pPortParamTypeImage = NULL;
  }
  if(pComponentPrivate->pPortParamTypeOthers) {
        free(pComponentPrivate->pPortParamTypeOthers);
        pComponentPrivate->pPortParamTypeOthers = NULL;
  }

  if (pComponentPrivate->ports) {
	for (i=0; i < pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++) {   
		if(pComponentPrivate->ports[i])   
		{
            pComponentPrivate->ports[i]->bIsDestroying=OMX_TRUE;
            tsem_up(pComponentPrivate->ports[i]->pAllocSem);				   
		}
	}      
	} 
	
	omx_base_filter_Destructor(pHandle);
	pthread_mutex_destroy(&pComponentPrivate->PortFlushMutex);
	if(pComponentPrivate->pstrbuf){
		actal_free(pComponentPrivate->pstrbuf);
		pComponentPrivate->pstrbuf=NULL;
	}
	if(pComponentPrivate->CodecConfigPkt!=NULL){
		actal_free(pComponentPrivate->CodecConfigPkt);
		pComponentPrivate->CodecConfigPkt=NULL;
	}
	omx_videodec_component_Deinit(openmaxStandComp);
	if(pComponentPrivate->fb_port != NULL) {
		fb_fifo_dispose(pComponentPrivate->fb_port);
		pComponentPrivate->fb_port=NULL;
	} 
	if(pComponentPrivate->vce_handle!=NULL){
		VceReSize_Close(pComponentPrivate->vce_handle);
		pComponentPrivate->vce_handle=NULL;
	}
	
	tsem_deinit(pComponentPrivate->decodedSem);
	free(pComponentPrivate->decodedSem);
	pComponentPrivate->decodedSem = NULL;
	
	tsem_deinit(pComponentPrivate->mDoneSem);
	free(pComponentPrivate->mDoneSem);
	pComponentPrivate->mDoneSem = NULL;
	
DEBUG(DEB_LEV_FULL_SEQ,"omx_videodec_component_ComponentDeInit 2\n");	
	/* frees port/s */   
	if (pComponentPrivate->ports) {
DEBUG(DEB_LEV_FULL_SEQ,"----------a nPorts %d\n",pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nPorts);		   
	for (i=0; i < pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++) {   
DEBUG(DEB_LEV_FULL_SEQ,"----------b pComponentPrivate->ports[i] %x \n",pComponentPrivate->ports[i]);		   
		if(pComponentPrivate->ports[i])   
		{
DEBUG(DEB_LEV_FULL_SEQ,"----------c\n");		   
            tsem_up(pComponentPrivate->ports[i]->pAllocSem);				   
	    	pComponentPrivate->ports[i]->PortDestructor(pComponentPrivate->ports[i]);   
DEBUG(DEB_LEV_FULL_SEQ,"----------d\n");		   	    
		}
	}   
DEBUG(DEB_LEV_FULL_SEQ,"----------e\n");	
	free(pComponentPrivate->ports);   
DEBUG(DEB_LEV_FULL_SEQ,"----------f\n");		   
	pComponentPrivate->ports=NULL;   
	} 
DEBUG(DEB_LEV_FULL_SEQ,"omx_videodec_component_ComponentDeInit 3\n");	
	
DEBUG(DEB_LEV_FULL_SEQ,"Destructor of video decoder component is called\n");
	
//	omx_base_filter_Destructor(pHandle);
	noVideoDecInstance--;
DEBUG(DEB_LEV_FULL_SEQ,"omx_videodec_component_ComponentDeInit 4\n");	
	return OMX_ErrorNone;
}


typedef void*(*FuncPtr)(void);
/** It initializates the FFmpeg framework, and opens an FFmpeg videodecoder of type specified by IL client 
  */ 
OMX_ERRORTYPE omx_videodec_component_actvideoInit(VIDDEC_COMPONENT_PRIVATE *pComponentPrivate) 
{
  FuncPtr func_handle;
	char libname[12] = "vd_";
	int i;
	omx_base_video_PortType *inPort,*outPort;
	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
	OMX_VIDEO_PARAM_PORTFORMATTYPE* pPortFormat;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	inPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	outPort= (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	pPortDef = &(outPort->sPortParam);
	pPortFormat = &(outPort->sVideoParam);
  pComponentPrivate->is_videodec_fifo_init=OMX_FALSE;
	pComponentPrivate->is_fifo_disposing=OMX_FALSE;
	if(pComponentPrivate->is_Thumbnail==OMX_TRUE){
		pComponentPrivate->framebuffernum = outPort->sPortParam.nBufferCountActual;
	}else{
		pComponentPrivate->framebuffernum = VIDDEC_DEFAULT_FRAME_BUFFER_NUM;
	}
  
  
  if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC) || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC_91)){
  	pComponentPrivate->videobuffersize = (((pPortDef->format.video.nStride+31)&(~31))+64) * 	(((pPortDef->format.video.nFrameHeight+31)&(~31))+64) * VIDDEC_FACTORFORMAT420;
  }else{
  	pComponentPrivate->videobuffersize = ((pPortDef->format.video.nStride+31)&(~31)) * ((pPortDef->format.video.nFrameHeight+15)&(~15)) * VIDDEC_FACTORFORMAT420;
  }
 
	switch(inPort->sVideoParam.eCompressionFormat)
	{
		case OMX_VIDEO_CodingRVG2:
			DEBUG(DEB_LEV_FULL_SEQ,"------------is rvg2 here\n");
			strcat(libname, "rvg2.so");
			break;
		case OMX_VIDEO_CodingHEVC:
			DEBUG(DEB_LEV_ERR,"------------hevc here\n");
			strcat(libname, "hevc.so");
			break;
	  case OMX_VIDEO_CodingHEVC_91:
			DEBUG(DEB_LEV_ERR,"------------hevc_91 here\n");
			strcat(libname, "hevc_91.so");
			break;
	 case OMX_VIDEO_CodingMPEG2:
	 		DEBUG(DEB_LEV_FULL_SEQ,"------------mpeg2 here\n");
	 		pComponentPrivate->videobuffersize =  (pComponentPrivate->actualframewidth * pComponentPrivate->actualframeheight*3)/2;
			strcat(libname, "mpeg.so");
			break;
	 case OMX_VIDEO_CodingMPEG4:
	 		DEBUG(DEB_LEV_FULL_SEQ,"------------mpeg4 here\n");
			strcat(libname, "xvid.so");
			break;
	
	 case OMX_VIDEO_CodingAVC:
	 		DEBUG(DEB_LEV_FULL_SEQ,"------------h264 here\n");
	 		pComponentPrivate->videobuffersize += ((pPortDef->format.video.nStride+31)&(~31)) * ((pPortDef->format.video.nFrameHeight+15)&(~15))/4;
			strcat(libname, "h264.so");
			break;
	 case OMX_VIDEO_CodingH263:
	 		DEBUG(DEB_LEV_FULL_SEQ,"------------h263 here\n");
			strcat(libname, "h263.so");
			break;
	 case OMX_VIDEO_CodingFLV1:
	 		DEBUG(DEB_LEV_FULL_SEQ,"------------flv1 here\n");
			strcat(libname, "flv1.so");
			break;
	 case OMX_VIDEO_CodingDIV3:
	 		DEBUG(DEB_LEV_FULL_SEQ,"------------msm4 here\n");
			strcat(libname, "msm4.so");
			break;
	 case OMX_VIDEO_CodingVP6:
	 		DEBUG(DEB_LEV_FULL_SEQ,"------------vp6 here\n");
			strcat(libname, "vp6.so");
			break;
	 case OMX_VIDEO_CodingVP8:
	 		DEBUG(DEB_LEV_FULL_SEQ,"------------vp8 here\n");
			strcat(libname, "vp8.so");
			break;
	  case OMX_VIDEO_CodingRV:
	 		DEBUG(DEB_LEV_FULL_SEQ,"------------rv34 here\n");
			strcat(libname, "rv34.so");
			break;
		case OMX_VIDEO_CodingVC1:
	 		DEBUG(DEB_LEV_FULL_SEQ,"------------vc1 here\n");
			strcat(libname, "vc1.so");
			break;
	case OMX_VIDEO_CodingMJPEG:
			DEBUG(DEB_LEV_FULL_SEQ,"------------mjpg here\n");
			strcat(libname, "mjpg.so");
			break;
	default:
			DEBUG(DEB_LEV_FULL_SEQ,"In %s encounted unrecognized format %d\n",__func__,inPort->sVideoParam.eCompressionFormat);	
			err = OMX_ErrorComponentNotFound;
			goto LOADCODEC_EXIT;	
	}

	pComponentPrivate->p_so_handle = dlopen(libname, RTLD_NOW);
  if (pComponentPrivate->p_so_handle == NULL)
  {
    	DEBUG(DEB_LEV_FULL_SEQ,"In %s dlopen Error\n",__func__);
        err = OMX_ErrorComponentNotFound;
			goto LOADCODEC_EXIT;
  }

   func_handle = (FuncPtr)dlsym( pComponentPrivate->p_so_handle , "get_plugin_info");
   if(func_handle == NULL) 
    {
    	DEBUG(DEB_LEV_FULL_SEQ,"In %s dlsym Error\n",__func__);
        dlclose( pComponentPrivate->p_so_handle);
		pComponentPrivate->p_so_handle = NULL;
		err = OMX_ErrorDynamicResourcesUnavailable;
		goto LOADCODEC_EXIT;
    }

    pComponentPrivate->p_interface = (videodec_plugin_t*)func_handle();
    if(pComponentPrivate->p_interface == NULL) {
    	DEBUG(DEB_LEV_FULL_SEQ,"In %s get func_handle Error\n",__func__);
        dlclose(pComponentPrivate->p_so_handle);
        pComponentPrivate->p_so_handle = NULL;
        err = OMX_ErrorDynamicResourcesUnavailable;
				goto LOADCODEC_EXIT;
    }
    if(pComponentPrivate->fb_port==NULL){
    	DEBUG(DEB_LEV_ERR,"fb fifo open \n");
    	pComponentPrivate->fb_port = fb_fifo_open((void **)(&(pComponentPrivate->vout)));
    	
    }
    
    DEBUG(DEB_LEV_FULL_SEQ,"video lib init2 InitBuf %x\n",pComponentPrivate->CodecConfigPkt);
    if(inPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingHEVC || inPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingHEVC_91){
			int ap_param;
			ap_param = pComponentPrivate->is_Thumbnail;   
			pComponentPrivate->p_handle = pComponentPrivate->p_interface->open(&ap_param, pComponentPrivate->CodecConfigPktLen>0 ? pComponentPrivate->CodecConfigPkt:NULL , pComponentPrivate->fb_port);
		}else{
			pComponentPrivate->p_handle = pComponentPrivate->p_interface->open(NULL, pComponentPrivate->CodecConfigPktLen>0 ? pComponentPrivate->CodecConfigPkt:NULL , pComponentPrivate->fb_port);
		}
		
   	if((pPortFormat->xFramerate>>16)>30 && pComponentPrivate->is_Thumbnail==OMX_FALSE && pPortDef->format.video.nStride* ((pPortDef->format.video.nFrameHeight+15)&(~15))>=640*360 ){
		DEBUG(DEB_LEV_ERR,"xFramerate is %d \n",(pPortFormat->xFramerate>>16));
		//pComponentPrivate->p_interface->ex_ops(pComponentPrivate->p_handle,DISCARD_FRAMES,1);
		pComponentPrivate->p_interface->ex_ops(pComponentPrivate->p_handle,EX_RESERVED2,1);
	}
	
		if(pComponentPrivate->p_handle == NULL) {
			dlclose(pComponentPrivate->p_so_handle);
	    pComponentPrivate->p_so_handle = NULL;
	    pComponentPrivate->is_error_status=OMX_TRUE;
	    err = OMX_ErrorDynamicResourcesUnavailable;		
	    goto LOADCODEC_EXIT; 
		}	



	return OMX_ErrorNone;

LOADCODEC_EXIT:
	return err;	
}


/** It Deinitializates the ffmpeg framework, and close the ffmpeg video decoder of selected coding type
  */
void omx_videodec_component_actvideoDeInit(VIDDEC_COMPONENT_PRIVATE *pComponentPrivate) 
{	
	pComponentPrivate->is_fifo_disposing=OMX_TRUE;
	
	if (pComponentPrivate->p_interface) 
	{
		if(pComponentPrivate->p_handle != NULL)
		{
	
			DEBUG(DEB_LEV_ERR,"==decoder dispose==\n");
			pComponentPrivate->p_interface->dispose(pComponentPrivate->p_handle);
			pComponentPrivate->p_handle = NULL;
		}
		if(pComponentPrivate->p_so_handle!=NULL){
			dlclose(pComponentPrivate->p_so_handle);
			pComponentPrivate->p_so_handle = NULL;
		}
		pComponentPrivate->p_interface = NULL;
	}
	if(pComponentPrivate->fb_port != NULL) {
		DEBUG(DEB_LEV_ERR,"===fb disposeing===\n");
		fb_fifo_dispose(pComponentPrivate->fb_port);
		pComponentPrivate->fb_port=NULL;
	} 
	if(pComponentPrivate->vce_handle!=NULL){
		VceReSize_Close(pComponentPrivate->vce_handle);
		pComponentPrivate->vce_handle=NULL;
	}
	
}
void omx_videodec_getoutput(OMX_COMPONENTTYPE* openmaxStandComp,frame_buf_handle *raw_buf_lcd,OMX_BUFFERHEADERTYPE* pOutputBuffer)
{
	    VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE *)openmaxStandComp->pComponentPrivate;
	    omx_base_filter_PrivateType* omx_base_filter_Private = (omx_base_filter_PrivateType*)openmaxStandComp->pComponentPrivate;
      	omx_base_video_PortType *pOutPort=(omx_base_video_PortType *)omx_base_filter_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
      	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = &(pOutPort->sPortParam);
		OMX_S64 timestamp;
		OMX_S64 pts;
		OMX_CONFIG_RECTTYPE *PortOutCrop= &(pComponentPrivate->croprect);
		int i;				
	    vde_video_metadata_t*	MetaData = NULL;
	    void *buff_handle = NULL;
        unsigned char* vir_addr = NULL;
      		
			if(raw_buf_lcd != NULL) 
			{
				dec_buf_t *frame_header =raw_buf_lcd->vo_frame_info;
				timestamp=frame_header->time_stamp;
				pOutputBuffer->nTimeStamp = timestamp*1000;	
				pOutputBuffer->nFilledLen = pOutputBuffer->nAllocLen;
				pOutputBuffer->nOffset = 0; 
				OMX_PTR phy_addr = 0;
				VR_Parm_t vr_parm;	
				if(pComponentPrivate->bOutPutBufferAllocated==BUFFER_ASSIGNED){
					if(pComponentPrivate->bStoreMediadata[OMX_BASE_FILTER_OUTPUTPORT_INDEX]==OMX_TRUE){
						MetaData = (vde_video_metadata_t*)pOutputBuffer->pBuffer;
						buff_handle =MetaData->handle;
					}else if(pComponentPrivate->IsGraphicBuffer == OMX_TRUE){
						buff_handle = pOutputBuffer->pBuffer;
					}
					if(buff_handle!=NULL){
#ifdef ENABLE_ACTIONS_OSAL
							Actions_OSAL_GetPhyAddr(buff_handle, &phy_addr);
#else
							if(IGralloc_getPhys(buff_handle, &phy_addr)<0){	
									DEBUG(DEB_LEV_ERR,"omx_videodec_getoutput: calling IGralloc_getPhys() failed! \n");						
							}			
#endif
					} 
				}else if(pComponentPrivate->bOutPutBufferAllocated==BUFFER_ALLOCATED){
						phy_addr = actal_get_phyaddr(pOutputBuffer->pBuffer);
						actal_cache_flush(raw_buf_lcd->vir_addr);	
				}
				vr_parm.src_addr = raw_buf_lcd->phy_addr;
				if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC) || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC_91)){
					vr_parm.srcstride = pPortDef->format.video.nStride + 64;
					vr_parm.srcheight = ((pPortDef->format.video.nFrameHeight+31)&(~31))+64;
					vr_parm.cropw = PortOutCrop->nWidth+64;
				  vr_parm.croph = PortOutCrop->nHeight+64;
				}else{
					vr_parm.srcstride = (pComponentPrivate->actualframewidth+31)&(~31);
					vr_parm.srcheight = (pComponentPrivate->actualframeheight+15)&(~15);
					vr_parm.cropw = pComponentPrivate->actualdisplaywidth;
				  vr_parm.croph = pComponentPrivate->actualdisplayheight;
				}

				
				vr_parm.dst_addr = phy_addr;
				vr_parm.wscale = 2;
				vr_parm.hscale = 2;	
					
				VceReSize_Run(pComponentPrivate->vce_handle,&vr_parm);		
				pComponentPrivate->vout->put_rbuf(pComponentPrivate->vout);	
      }
}
unsigned int tsem_ctor(tsem_t* tsem) {
  pthread_mutex_lock(&tsem->mutex);
  unsigned int ctor= tsem->semval;
  pthread_mutex_unlock(&tsem->mutex);
  return ctor;
}
void* omx_videodec_component_OutPutFunction(void* param) {
	OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;
	VIDDEC_COMPONENT_PRIVATE* pComponentPrivate=(VIDDEC_COMPONENT_PRIVATE*)openmaxStandComp->pComponentPrivate;
	omx_base_video_PortType *pInPort=(omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	omx_base_video_PortType *pOutPort=(omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	tsem_t* pOutputSem = pOutPort->pBufferSem;
	queue_t* pOutputQueue = pOutPort->pBufferQueue;
	OMX_BUFFERHEADERTYPE* pOutputBuffer=NULL;
	OMX_BOOL isOutputBufferNeeded=OMX_TRUE;
	frame_buf_handle *raw_buf_lcd = NULL;
	while(pComponentPrivate->state== OMX_StateIdle || pComponentPrivate->state==OMX_StateExecuting || pComponentPrivate->state==OMX_StatePause || pComponentPrivate->transientState == OMX_TransStateLoadedToIdle){
	
		pthread_mutex_lock(&pComponentPrivate->flush_mutex);
		while(PORT_IS_BEING_FLUSHED(pOutPort) && !PORT_IS_BEING_FLUSHED(pInPort)){
			 pthread_mutex_unlock(&pComponentPrivate->flush_mutex);
			 if(isOutputBufferNeeded==OMX_FALSE) {
        	pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);
        	pOutputBuffer=NULL;
        	isOutputBufferNeeded=OMX_TRUE;
        	DEBUG(DEB_LEV_FULL_SEQ, "Ports are flushing,so returning output buffer\n");
       }
        DEBUG(DEB_LEV_ERR,"output flush wait start\n");
     		pthread_mutex_lock(&pComponentPrivate->PortFlushMutex);
       	tsem_up(pComponentPrivate->flush_all_condition);      
       	tsem_down(pComponentPrivate->flush_condition);	
     		pthread_mutex_unlock(&pComponentPrivate->PortFlushMutex);
        DEBUG(DEB_LEV_ERR,"output flush wait end\n");
       pthread_mutex_lock(&pComponentPrivate->flush_mutex);
		}
		pthread_mutex_unlock(&pComponentPrivate->flush_mutex);
		
		if(pComponentPrivate->state == OMX_StateLoaded || pComponentPrivate->state == OMX_StateInvalid) {
			tsem_down(pComponentPrivate->mDoneSem);
      DEBUG(DEB_LEV_ERR, "In %s Buffer Management Thread is exiting\n",__func__);
      break;
    }
    /*No buffer to process. So wait here*/
    if((isOutputBufferNeeded==OMX_TRUE && pOutputSem->semval==0) &&
      (pComponentPrivate->state != OMX_StateLoaded && pComponentPrivate->state != OMX_StateInvalid)
      && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
      //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
  
      tsem_down(pComponentPrivate->bMgmtSem);
      if(PORT_IS_BEING_FLUSHED(pInPort)){
      	if(pComponentPrivate->bMgmtSem->semval==0){
      		tsem_up(pComponentPrivate->bMgmtSem);
      	}
      }
  
    }
		if(pComponentPrivate->state == OMX_StateLoaded || pComponentPrivate->state == OMX_StateInvalid) {
			tsem_up(pComponentPrivate->bMgmtSem);
			tsem_down(pComponentPrivate->mDoneSem);
      DEBUG(DEB_LEV_ERR, "In %s Buffer Management Thread is exiting\n",__func__);
      break;
    }
    if(isOutputBufferNeeded==OMX_TRUE && pOutputSem->semval>0 && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))){
      	tsem_down(pOutputSem);
      	if(pOutputQueue->nelem>0){
        	isOutputBufferNeeded=OMX_FALSE;
        	pOutputBuffer = dequeue(pOutputQueue);
        	pOutputBuffer->nFilledLen=0;
        	if(pOutputBuffer == NULL){
          	DEBUG(DEB_LEV_ERR, "Had NULL output buffer!! op is=%d,iq=%d\n",pOutputSem->semval,pOutputQueue->nelem);
          	break;
        	}
     	  }
		}
    if(isOutputBufferNeeded==OMX_FALSE && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort)) && pComponentPrivate->is_videodec_fifo_init==OMX_TRUE){
    	raw_buf_lcd = pComponentPrivate->vout->get_rbuf(pComponentPrivate->vout);
    	if(raw_buf_lcd!=NULL){
    		tsem_down(pComponentPrivate->decodedSem);
    		pOutputBuffer->nFlags=0;
    		omx_videodec_getoutput(openmaxStandComp,raw_buf_lcd,pOutputBuffer);
    		if(pComponentPrivate->bIsEOSReached == OMX_TRUE && pComponentPrivate->vout->get_rbuf_num(pComponentPrivate->vout)==0){
    			DEBUG(DEB_LEV_ERR,"signal EOS ,line %d \n",__LINE__);
    			pOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
    			pComponentPrivate->bIsEOSReached = OMX_FALSE;
    		}
    	}    	
    }    
    if(isOutputBufferNeeded==OMX_FALSE && pOutputBuffer->nFilledLen != 0){
    		pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);         
        pOutputBuffer=NULL;    
        isOutputBufferNeeded=OMX_TRUE;
    }
    
	}
	return NULL;
}

/** internal function to set codec related parameters in the private type structure 
  */
void SetInternalVideoParameters(OMX_COMPONENTTYPE *openmaxStandComp) {

  VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE *)openmaxStandComp->pComponentPrivate;

}



/** The Initialization function of the video decoder
  */
OMX_ERRORTYPE omx_videodec_component_Init(OMX_COMPONENTTYPE *openmaxStandComp) {

	VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE *)openmaxStandComp->pComponentPrivate;
	OMX_ERRORTYPE eError = OMX_ErrorNone; 

	
  
	

  return eError;
}

/** The Deinitialization function of the video decoder  
  */
OMX_ERRORTYPE omx_videodec_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp) {

  VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = openmaxStandComp->pComponentPrivate;
  OMX_ERRORTYPE eError = OMX_ErrorNone;  

  if (pComponentPrivate->avcodecReady) {
    omx_videodec_component_actvideoDeInit(pComponentPrivate);
    pComponentPrivate->avcodecReady = OMX_FALSE;
  }

  return eError;
} 

void* omx_videodec_component_BufferMgmtFunction (void* param) {
  OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;
  omx_base_filter_PrivateType* omx_base_filter_Private = (omx_base_filter_PrivateType*)openmaxStandComp->pComponentPrivate;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate=(VIDDEC_COMPONENT_PRIVATE*)openmaxStandComp->pComponentPrivate;
  omx_base_video_PortType *pInPort=(omx_base_video_PortType *)omx_base_filter_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
  omx_base_video_PortType *pOutPort=(omx_base_video_PortType *)omx_base_filter_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
  tsem_t* pInputSem = pInPort->pBufferSem;
  queue_t* pInputQueue = pInPort->pBufferQueue;
  OMX_BUFFERHEADERTYPE* pInputBuffer=NULL;
  OMX_BOOL isInputBufferNeeded=OMX_TRUE;
  int inBufExchanged=0;
  int nFilledBuffDone=0;
  
  omx_base_filter_Private->bellagioThreads->nThreadBufferMngtID = (long int)syscall(__NR_gettid);
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s of component %p\n", __func__, openmaxStandComp);
  DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s the thread ID is %i\n", __func__, (int)omx_base_filter_Private->bellagioThreads->nThreadBufferMngtID);
  prctl(PR_SET_NAME, (unsigned long)"BufferMgmt", 0, 0, 0);

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s\n", __func__);
  /* checks if the component is in a state able to receive buffers */
  while(omx_base_filter_Private->state == OMX_StateIdle || omx_base_filter_Private->state == OMX_StateExecuting ||  omx_base_filter_Private->state == OMX_StatePause ||
    omx_base_filter_Private->transientState == OMX_TransStateLoadedToIdle){

  
    /*Wait till the ports are being flushed*/
    pthread_mutex_lock(&omx_base_filter_Private->flush_mutex);
    while( PORT_IS_BEING_FLUSHED(pInPort) && !PORT_IS_BEING_FLUSHED(pOutPort)) {
      pthread_mutex_unlock(&omx_base_filter_Private->flush_mutex);

      DEBUG(DEB_LEV_FULL_SEQ, "In %s 1 signaling flush all cond iE=%d,iF=%d,iSemVal=%d\n",
        __func__,inBufExchanged,isInputBufferNeeded,pInputSem->semval);


      if(isInputBufferNeeded==OMX_FALSE) {
        pInPort->ReturnBufferFunction(pInPort,pInputBuffer);
        inBufExchanged--;
        pInputBuffer=NULL;
        isInputBufferNeeded=OMX_TRUE;
        DEBUG(DEB_LEV_FULL_SEQ, "Ports are flushing,so returning input buffer\n");
      }

     DEBUG(DEB_LEV_FULL_SEQ, "In %s 2 signaling flush all cond iE=%d,iF=%d,iSemVal=%d\n",
        __func__,inBufExchanged,isInputBufferNeeded,pInputSem->semval);

      DEBUG(DEB_LEV_ERR,"input flush wait start \n");
      pthread_mutex_lock(&pComponentPrivate->PortFlushMutex);
      pComponentPrivate->bComponentFlush= OMX_TRUE;
      tsem_up(omx_base_filter_Private->flush_all_condition);      
      tsem_down(omx_base_filter_Private->flush_condition);
      pthread_mutex_unlock(&pComponentPrivate->PortFlushMutex);
      DEBUG(DEB_LEV_ERR,"input flush wait end \n");
      pthread_mutex_lock(&omx_base_filter_Private->flush_mutex);
    }
    pthread_mutex_unlock(&omx_base_filter_Private->flush_mutex);

    
    if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid) {
      DEBUG(DEB_LEV_ERR, "In %s Buffer Management Thread is exiting\n",__func__);
      break;
    }
    /*No buffer to process. So wait here*/
    if((isInputBufferNeeded==OMX_TRUE && pInputSem->semval==0) &&
      (omx_base_filter_Private->state != OMX_StateLoaded && omx_base_filter_Private->state != OMX_StateInvalid)
      &&  !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
      //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
  
       tsem_down(omx_base_filter_Private->bMgmtSem);
       if(PORT_IS_BEING_FLUSHED(pOutPort)){
      	if(pComponentPrivate->bMgmtSem->semval==0){
      		tsem_up(pComponentPrivate->bMgmtSem);
      	}
      }
  
    }

    if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid) {
      DEBUG(DEB_LEV_ERR, "In %s Buffer Management Thread is exiting\n",__func__);
      tsem_up(omx_base_filter_Private->bMgmtSem);
      break;
    }

    DEBUG(DEB_LEV_FULL_SEQ, "inPutNeed:%d,Waiting for input buffer semval=%d in %s\n",isInputBufferNeeded,pInputSem->semval, __func__);
    if(pInputSem->semval>0 && isInputBufferNeeded==OMX_TRUE && pComponentPrivate->is_error_status==OMX_FALSE && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
    
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

    
    
    

    if(isInputBufferNeeded==OMX_FALSE && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort)) ) {
    
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
   
      if((pInputBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME) {
         DEBUG(DEB_LEV_FULL_SEQ, "Detected  START TIME flag in the input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
         pInputBuffer->nFlags = 0;
      }

      if(omx_base_filter_Private->state == OMX_StateExecuting && pComponentPrivate->is_error_status==OMX_FALSE)  {

        if (omx_base_filter_Private->BufferMgmtCallback && pInputBuffer->nFilledLen > 0) {
        	
          (*(omx_base_filter_Private->BufferMgmtCallback))(openmaxStandComp, pInputBuffer, NULL);
         
        } else{
          /*It no buffer management call back the explicitly consume input buffer*/
          pInputBuffer->nFilledLen = 0;
        }
      } else if(!PORT_IS_BEING_FLUSHED(pInPort)) {
        DEBUG(DEB_LEV_ERR, "In %s Received Buffer in non-Executing State(%x)\n", __func__, (int)omx_base_filter_Private->state);
      } else {
          pInputBuffer->nFilledLen = 0;
        
      }
  
      if((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS && pInputBuffer->nFilledLen==0) {
        DEBUG(DEB_LEV_ERR,"Detected EOS flags in input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
        
        
        if((((omx_base_video_PortType *)pInPort)->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC || ((omx_base_video_PortType *)pInPort)->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingMPEG4 ) && pComponentPrivate->p_handle!=NULL)
        {
        	
        	 pComponentPrivate->p_interface->ex_ops(pComponentPrivate->p_handle,FLUSH_BUFFERS_REMAINING,0);	
        
        }
     
        (*(omx_base_filter_Private->callbacks->EventHandler))
          (openmaxStandComp,
          omx_base_filter_Private->callbackData,
          OMX_EventBufferFlag, /* The command was completed */
          1, /* The commands was a OMX_CommandStateSet */
          pInputBuffer->nFlags, /* The state has been changed in message->messageParam2 */
          NULL);
        	pInputBuffer->nFlags=0;
      
        omx_base_filter_Private->bIsEOSReached = OMX_TRUE;
      }
      if(omx_base_filter_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
        /*Waiting at paused state*/
        tsem_wait(omx_base_filter_Private->bStateSem);
      }			
    }

    if(omx_base_filter_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort))) {
      /*Waiting at paused state*/
  
      tsem_wait(omx_base_filter_Private->bStateSem);
     
    }

    /*Input Buffer has been completely consumed. So, return input buffer*/
    if((isInputBufferNeeded == OMX_FALSE) && pInputBuffer->nFilledLen==0 ) {
  
      pInPort->ReturnBufferFunction(pInPort,pInputBuffer);
    
      inBufExchanged--;
      pInputBuffer=NULL;
      isInputBufferNeeded=OMX_TRUE;
    }
  }
  DEBUG(DEB_LEV_ERR, "Out of %s of component %p\n", __func__, openmaxStandComp);
  return NULL;
}

void omx_videdec_component_probe(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pInputBuffer,video_codec_info_t* video_codec_info ){
	
	  VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE *)openmaxStandComp->pComponentPrivate;
	  omx_base_video_PortType *pOutPort=(omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	  OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = &(pOutPort->sPortParam);
    stream_buf_handle bitstream_buf;
    OMX_CONFIG_RECTTYPE *PortOutCrop= &(pComponentPrivate->croprect);
		bitstream_buf.data_len = pInputBuffer->nFilledLen;
		if (pComponentPrivate->bthirdparty==OMX_TRUE){
			bitstream_buf.vir_addr = pInputBuffer->pBuffer - sizeof(packet_header_t);				
		}else{
			bitstream_buf.vir_addr = pInputBuffer->pBuffer;
		}
  	actal_cache_flush(bitstream_buf.vir_addr);
		bitstream_buf.phy_addr=(unsigned char*)actal_get_phyaddr(bitstream_buf.vir_addr);
	
		pComponentPrivate->p_interface->probe(pComponentPrivate->CodecConfigPktLen>0 ? pComponentPrivate->CodecConfigPkt:NULL,&bitstream_buf,video_codec_info);
  	
    pComponentPrivate->actualdisplaywidth =  video_codec_info->width;
    pComponentPrivate->actualdisplayheight = video_codec_info->height;
    pComponentPrivate->actualframewidth =  (video_codec_info->src_width+31)&(~31);
    pComponentPrivate->actualframeheight  = (video_codec_info->src_height+15)&(~15);
    
 	 	PortOutCrop->nSize = sizeof(OMX_CONFIG_RECTTYPE);
  	PortOutCrop->nLeft = 0;
  	PortOutCrop->nTop =0;
  	PortOutCrop->nWidth =  video_codec_info->width/2;
  	PortOutCrop->nHeight = video_codec_info->height/2;
  	DEBUG(DEB_LEV_ERR,"display width:%d,display height:%d \n",PortOutCrop->nWidth,PortOutCrop->nHeight);
  		(*(pComponentPrivate->callbacks->EventHandler))
      	(openmaxStandComp,
      	pComponentPrivate->callbackData,
      	OMX_EventPortSettingsChanged, /* The command was completed */
      	OMX_BASE_FILTER_OUTPUTPORT_INDEX, /* The commands was a OMX_CommandStateSet */
      	OMX_IndexConfigCommonOutputCrop, /* The state has been changed in message->messageParam */
      	NULL);
  

}
void omx_videdec_component_addpackheader(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pInputBuffer){

	VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE *)openmaxStandComp->pComponentPrivate; 
	omx_base_video_PortType *pInPort=(omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX]; 
	packet_header_t *pheader;
  pComponentPrivate->bthirdparty=OMX_TRUE;
  if(pInputBuffer->nFilledLen >= sizeof(packet_header_t)){
  	pheader=(packet_header_t *)pInputBuffer->pBuffer;
  	if(pheader->header_type ==VIDEO_PACKET){
  		pComponentPrivate->bthirdparty=OMX_FALSE;
  	}
  }
  if(pComponentPrivate->is_cur_stream_buf==OMX_FALSE && pComponentPrivate->bthirdparty==OMX_TRUE){
  	pheader=pInputBuffer->pBuffer - sizeof(packet_header_t);
		memset(pheader,0,sizeof(packet_header_t));
	  pheader->header_type = VIDEO_PACKET;
	  pheader->packet_ts = pInputBuffer->nTimeStamp/1000;
	  if(pComponentPrivate->bComponentFlush==OMX_TRUE){
	  	pheader->seek_reset_flag = 1;
	  	pComponentPrivate->bComponentFlush = OMX_FALSE;
	  }
	  if(pComponentPrivate->bConfigPktStored==OMX_FALSE && pComponentPrivate->CodecConfigPkt!=NULL && pComponentPrivate->CodecConfigPktLen!=0 ){
	  	memcpy(pComponentPrivate->pstrbuf,pInputBuffer->pBuffer, pInputBuffer->nFilledLen);
			memcpy(pInputBuffer->pBuffer,pComponentPrivate->CodecConfigPkt,pComponentPrivate->CodecConfigPktLen);
			pheader->block_len = pInputBuffer->nFilledLen+pComponentPrivate->CodecConfigPktLen;
			memcpy(pInputBuffer->pBuffer+pComponentPrivate->CodecConfigPktLen, pComponentPrivate->pstrbuf, pInputBuffer->nFilledLen);
			pInputBuffer->nFilledLen = pInputBuffer->nFilledLen+sizeof(packet_header_t)+pComponentPrivate->CodecConfigPktLen;
			pComponentPrivate->bConfigPktStored=OMX_TRUE;
		}else{
	  	pheader->block_len = pInputBuffer->nFilledLen;
			pInputBuffer->nFilledLen += sizeof(packet_header_t);
	  }

	} 
}
/** This function is used to process the input buffer and provide one output buffer
  */
void omx_videodec_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pInputBuffer, OMX_BUFFERHEADERTYPE* pOutputBuffer) 
{
	VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE *)openmaxStandComp->pComponentPrivate; 
	omx_base_video_PortType *pInPort=(omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX]; 
	omx_base_video_PortType *pOutPort=(omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = &(pOutPort->sPortParam);
	OMX_ERRORTYPE err = OMX_ErrorNone;
  	dec_buf_t *vo_frame;
	int i;
	int ret=0;

	
//第一个包为init_buf
	if(pComponentPrivate->isFirstBuffer == 1)
	{

		pComponentPrivate->isFirstBuffer = 0;

		if(pInputBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)//OMXCodec.cpp
		{			

			
				DEBUG(DEB_LEV_FULL_SEQ,"pInputBuffer->nFilledLen %d pBuffer %x\n",pInputBuffer->nFilledLen,pInputBuffer->pBuffer);

				memcpy(pComponentPrivate->CodecConfigPkt,pInputBuffer->pBuffer,pInputBuffer->nFilledLen);
				pComponentPrivate->CodecConfigPktLen = pInputBuffer->nFilledLen;
				if(pComponentPrivate->avcodecReady == OMX_FALSE)
				{
					DEBUG(DEB_LEV_FULL_SEQ,"in %s, line %d\n", __func__, __LINE__);
					err = omx_videodec_component_actvideoInit(pComponentPrivate);
					if(err != OMX_ErrorNone)
					{
						DEBUG(DEB_LEV_FULL_SEQ,"In %s Init codec lib failed\n", __func__);	
						pInputBuffer->nFilledLen = 0;
				    pComponentPrivate->is_error_status=OMX_TRUE;
				    pInPort->ReturnBufferFunction(pInPort,pInputBuffer);
				    (*(pComponentPrivate->callbacks->EventHandler))
      			(openmaxStandComp,
      			 pComponentPrivate->callbackData,
      			OMX_EventError, /* The command was completed */
      			err, /* The commands was a OMX_CommandStateSet */
      			0, /* The state has been changed in message->messageParam */
      			NULL);
						return;		
					}
					pComponentPrivate->avcodecReady = OMX_TRUE;
				}
				
				pInputBuffer->nFilledLen = 0;
				pInputBuffer->nFlags = 0;
				return;		 
			
		}

		if(pComponentPrivate->avcodecReady == OMX_FALSE)
		{
			DEBUG(DEB_LEV_FULL_SEQ,"in %s, line %d\n", __func__, __LINE__);
			err = omx_videodec_component_actvideoInit(pComponentPrivate);
			if(err != OMX_ErrorNone)
			{
				DEBUG(DEB_LEV_FULL_SEQ,"In %s Init codec lib failed\n", __func__);	
				pInputBuffer->nFilledLen = 0;
				pComponentPrivate->is_error_status=OMX_TRUE;
				pInPort->ReturnBufferFunction(pInPort,pInputBuffer);
				(*(pComponentPrivate->callbacks->EventHandler))
      		(openmaxStandComp,
      		 pComponentPrivate->callbackData,
      		OMX_EventError, /* The command was completed */
      		err, /* The commands was a OMX_CommandStateSet */
      		0, /* The state has been changed in message->messageParam */
      		NULL);
				return;		
			}
	
			pComponentPrivate->avcodecReady = OMX_TRUE;
		}
	}
	//add 28 bytes header and h264 sps header 
  omx_videdec_component_addpackheader(openmaxStandComp,pInputBuffer);
	

//===============h264 probe=========================	
  if( pComponentPrivate->is_fifo_disposing==OMX_FALSE && pComponentPrivate->IsProbed==OMX_FALSE \
  	 && pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC){
  	video_codec_info_t video_codec_info;  
  	omx_videdec_component_probe(openmaxStandComp,pInputBuffer,&video_codec_info);
    pComponentPrivate->IsProbed=OMX_TRUE;
  	pComponentPrivate->framebuffernum =video_codec_info.ref_num+2;
  	DEBUG(DEB_LEV_ERR,"===probed,src_width:%d,src_height:%d,ref_num:%d=== \n",video_codec_info.src_width,video_codec_info.src_height,video_codec_info.ref_num); 
  }
  if(pComponentPrivate->is_videodec_fifo_init==OMX_FALSE){
  	 DEBUG(DEB_LEV_ERR,"framebuffernum is %d \n",pComponentPrivate->framebuffernum);
		 for(i = 0;i<pComponentPrivate->framebuffernum;i++) {
		 	 
				ret=raw_fifo_init(pComponentPrivate->vout,pComponentPrivate->framebuffernum,pComponentPrivate->videobuffersize);//note pBuffHead->nAllocLen
				if(ret<0){
					pComponentPrivate->is_error_status = OMX_TRUE ;						
					pInputBuffer->nFilledLen = 0;				
				  	err = OMX_ErrorUndefined;
				 	(*(pComponentPrivate->callbacks->EventHandler))
      				(openmaxStandComp,
      		 		pComponentPrivate->callbackData,
      				OMX_EventError, /* The command was completed */
      				err, /* The commands was a OMX_CommandStateSet */
      				0, /* The state has been changed in message->messageParam */
      				NULL);
          return;
			  }		
		 }
		 pComponentPrivate->vout->fifo_reset(pComponentPrivate->vout);
		 pComponentPrivate->is_videodec_fifo_init = OMX_TRUE;
	}

//解码,后续再考虑怎样加入解码线程
  if(pComponentPrivate->is_fifo_disposing==OMX_FALSE && pComponentPrivate->is_error_status==OMX_FALSE &&pComponentPrivate->PortReSettingReady==OMX_FALSE){
		frame_buf_handle *raw_buf_dec = NULL;	


		try_get_wbuf_timeout(&raw_buf_dec, pComponentPrivate->vout, &(pComponentPrivate->suspend_flag),100);
		if(raw_buf_dec==NULL){
			if(pComponentPrivate->is_cur_stream_buf==OMX_FALSE && pComponentPrivate->bthirdparty==OMX_TRUE){
				pInputBuffer->nFilledLen -= sizeof(packet_header_t);				
			}
			return ;
		}
	
	  // ========== start to decode ==============
		stream_buf_handle bitstream_buf;		
		bitstream_buf.data_len = pInputBuffer->nFilledLen;	

	  if(pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingDIV3 || pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingFLV1 \
	  	|| pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingHEVC || pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingHEVC_91){
	  	if (pComponentPrivate->bthirdparty==OMX_TRUE){
				bitstream_buf.vir_addr = pInputBuffer->pBuffer - sizeof(packet_header_t);		
			}else{
    		bitstream_buf.vir_addr = pInputBuffer->pBuffer;
	  	}
	  }else{
			if (pComponentPrivate->bthirdparty==OMX_TRUE){
				bitstream_buf.vir_addr = pInputBuffer->pBuffer - sizeof(packet_header_t);		
			}else{
    		bitstream_buf.vir_addr = pInputBuffer->pBuffer;
	  	}
			actal_cache_flush(bitstream_buf.vir_addr);
		  	bitstream_buf.phy_addr=(unsigned char*)actal_get_phyaddr(bitstream_buf.vir_addr);
	  }
	

		   int rt = pComponentPrivate->p_interface->decode_data(pComponentPrivate->p_handle, &bitstream_buf);

		
			// h264解缩略图， 第一个包可能存在错误，这时候解码器报错，继续给数据进来。
		 if (pComponentPrivate->is_Thumbnail==OMX_TRUE && rt == PLUGIN_RETURN_NOT_SUPPORT && pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC) {
		    DEBUG(DEB_LEV_FULL_SEQ,"*************** Thumbnail first packet not support***************\n");
            pComponentPrivate->is_cur_stream_buf = OMX_FALSE;
            pInputBuffer->nFilledLen = 0; 
            return;            
     }
     if (pComponentPrivate->is_Thumbnail==OMX_FALSE && rt == PLUGIN_RETURN_NOT_SUPPORT && pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC) {
            DEBUG(DEB_LEV_FULL_SEQ,"*************** PLUGIN_RETURN_NOT_SUPPORT***************\n");
            rt = PLUGIN_RETURN_NORMAL;
     }
 
		if (rt == PLUGIN_RETURN_NORMAL) {
			pComponentPrivate->is_cur_stream_buf = OMX_FALSE;
			pComponentPrivate->err_counter=0;
		} else if(rt == PLUGIN_RETURN_PKT_NOTEND) {
			pComponentPrivate->is_cur_stream_buf = OMX_TRUE;
			pComponentPrivate->err_counter=0;
		} else {
			pComponentPrivate->err_counter++;
			pComponentPrivate->is_cur_stream_buf = OMX_FALSE;	
			pInputBuffer->nFilledLen = 0;
			if(pComponentPrivate->err_counter>30 || pComponentPrivate->is_Thumbnail==OMX_TRUE || rt==PLUGIN_RETURN_NOT_SUPPORT){	 
				DEBUG(DEB_LEV_ERR,"decode err In %s,packet len :%d \n",__func__,bitstream_buf.data_len);
				pComponentPrivate->is_error_status = OMX_TRUE ;						
				pInPort->ReturnBufferFunction(pInPort,pInputBuffer);
				err = OMX_ErrorUndefined;
				(*(pComponentPrivate->callbacks->EventHandler))
      		(openmaxStandComp,
      		 pComponentPrivate->callbackData,
      		OMX_EventError, /* The command was completed */
      		err, /* The commands was a OMX_CommandStateSet */
      		0, /* The state has been changed in message->messageParam */
      		NULL);
    return;
    	}
		}
		if(pComponentPrivate->is_cur_stream_buf == OMX_FALSE) {
		//一包多帧的码流包也解完了
		//omx_base_filter.c中会将该包emptybufferdone
			pInputBuffer->nFilledLen = 0; 
		}	
		if(pComponentPrivate->is_Thumbnail==OMX_TRUE && pComponentPrivate->is_avc_Thumbnail_outputed==OMX_FALSE && pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC) {
			pComponentPrivate->is_avc_Thumbnail_outputed=OMX_TRUE;
			raw_buf_dec->vo_frame_info->display_flag = 1;//set the display flag,so avc  can get a thumnail		
		}
		tsem_up(pComponentPrivate->decodedSem);
	}
}


OMX_ERRORTYPE VIDDEC_CheckSetParameter(VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, OMX_PTR pCompParam, OMX_INDEXTYPE nParamIndex) 
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	omx_base_video_PortType *inPort,*outPort;
	inPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	outPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];


	if (pComponentPrivate->state == OMX_StateInvalid) {
		eError = OMX_ErrorInvalidState;
		DEBUG(DEB_LEV_FULL_SEQ,"invalid state \n",__FILE__,__LINE__);
		goto EXIT;
    }

	if (pComponentPrivate->state != OMX_StateLoaded && pComponentPrivate->state != OMX_StateWaitForResources) {
		/*using OMX_CONFIG_ROTATIONTYPE because it is smallest structure that contains nPortIndex;*/
		OMX_CONFIG_ROTATIONTYPE* pTempFormat = (OMX_CONFIG_ROTATIONTYPE*)pCompParam;

		switch (nParamIndex) {
			/*the indices corresponding to the parameter structures containing the field "nPortIndex"*/
			case OMX_IndexParameterStoreMediaData:
				break;
			case OMX_IndexThumbnail:
				break;
			case OMX_IndexParamVideoPortFormat:
			case OMX_IndexParamPortDefinition:

				if (pTempFormat->nPortIndex ==  inPort->sPortParam.nPortIndex) {
					if (inPort->sPortParam.bEnabled){
						eError = OMX_ErrorIncorrectStateOperation;
					}
				}
				else if (pTempFormat->nPortIndex == outPort->sPortParam.nPortIndex) {
					if (outPort->sPortParam.bEnabled){
						eError = OMX_ErrorIncorrectStateOperation;
					}
				}/*it cannot be -1 because structure assignment will happen on one port*/
				else {
					eError = OMX_ErrorBadPortIndex;
				}
				break;
			default:
				eError = OMX_ErrorIncorrectStateOperation;
				break;
		}
	}

EXIT:
	
	if(eError != OMX_ErrorNone)
	{
		DEBUG(DEB_LEV_FULL_SEQ,"In %s VIDDEC_CheckSetParameter error\n", __func__);	
	}

    return eError;
}


OMX_ERRORTYPE VIDDEC_Load_Defaults (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, OMX_S32 nPassing)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	omx_base_video_PortType *inPort,*outPort;

	inPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	outPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

	DEBUG(DEB_LEV_FULL_SEQ,"in VIDDEC_Load_Defaults nPassing %d\n",nPassing);

	switch(nPassing){
		case VIDDEC_INIT_ALL:
//看似没什么用，添加变量
			/* Set component version */
			pComponentPrivate->pComponentVersion.s.nVersionMajor 			   	= VERSION_MAJOR;
			pComponentPrivate->pComponentVersion.s.nVersionMinor 			   	= VERSION_MINOR;
			pComponentPrivate->pComponentVersion.s.nRevision 				   	= VERSION_REVISION;
			pComponentPrivate->pComponentVersion.s.nStep 					= VERSION_STEP;

			/* Set spec version */
			pComponentPrivate->pSpecVersion.s.nVersionMajor					= VERSION_MAJOR;
			pComponentPrivate->pSpecVersion.s.nVersionMinor					= VERSION_MINOR;
			pComponentPrivate->pSpecVersion.s.nRevision						= VERSION_REVISION;
			pComponentPrivate->pSpecVersion.s.nStep							= VERSION_STEP;

			pComponentPrivate->pHandle->pApplicationPrivate 					= NULL;

//对应pPortParamType已有数据结构可用
			pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nPorts                           		= NUM_OF_PORTS;
			pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber                 	= VIDDEC_INPUT_PORT;

//input和output port都已用bellagio模板construct

			OMX_CONF_INIT_STRUCT( &(pComponentPrivate->componentRole), OMX_PARAM_COMPONENTROLETYPE);
			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_AVC);

			/* Set pPriorityMgmt defaults */
			OMX_CONF_INIT_STRUCT(pComponentPrivate->pPriorityMgmt, OMX_PRIORITYMGMTTYPE);
			pComponentPrivate->pPriorityMgmt->nGroupPriority                    		= -1;
			pComponentPrivate->pPriorityMgmt->nGroupID                          		= -1;
			pComponentPrivate->state 							 			= OMX_StateInvalid;


			break;

		case VIDDEC_INIT_AVC:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    	inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_AVC;
    	inPort->sVideoParam.eCompressionFormat            			= OMX_VIDEO_CodingAVC;
    	inPort->sVideoParam.eColorFormat                  			= VIDDEC_COLORFORMATUNUSED;
			
			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEAVC);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingAVC;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_AVC);

			break;
			      
		
		
			case VIDDEC_INIT_HEVC:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    	inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_HEVC;
    	inPort->sVideoParam.eCompressionFormat            			= OMX_VIDEO_CodingHEVC;
    	inPort->sVideoParam.eColorFormat                  			= VIDDEC_COLORFORMATUNUSED;
			
			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
		
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEHEVC);
    	inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    	inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    	inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingHEVC;
    	inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC);

			break; 
		case VIDDEC_INIT_HEVC_91:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    	inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_HEVC_91;
    	inPort->sVideoParam.eCompressionFormat            			= OMX_VIDEO_CodingHEVC_91;
    	inPort->sVideoParam.eColorFormat                  			= VIDDEC_COLORFORMATUNUSED;
			
			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
		
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEHEVC_91);
    	inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    	inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    	inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingHEVC_91;
    	inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC_91);

			break;  
		case VIDDEC_INIT_MPEG4:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_MPEG4;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingMPEG4;
    		inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEMPEG4;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEMPEG4);
    		inPort->sPortParam.format.video.nFrameWidth        	 	= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingMPEG4;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_MPEG4);
			
			break;

		case VIDDEC_INIT_H263:			
			inPort->sVideoParam.nPortIndex						= VIDDEC_INPUT_PORT;
			inPort->sVideoParam.nIndex							= VIDDEC_DEFAULT_INPUT_INDEX_H263;
			inPort->sVideoParam.eCompressionFormat				= OMX_VIDEO_CodingH263;
			inPort->sVideoParam.eColorFormat					= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEH263;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEH263);
    	inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    	inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat 	 	= OMX_VIDEO_CodingH263;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_H263);
			
			break;		

		case VIDDEC_INIT_VC1:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_VC1;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingVC1;
    		inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEVC1;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEVC1);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingVC1;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

//			pComponentPrivate->IsCodaSeqInitBuf  								= OMX_TRUE;
//			pComponentPrivate->IsCodaSeqInitWMV9 								= OMX_FALSE;
//			pComponentPrivate->CodaSeqInitBufLen								= 0;


			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_VC1);
			
			break;

		case VIDDEC_INIT_MPEG2:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_MPEG2;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingMPEG2;
    		inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEMPEG2;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEMPEG2);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingMPEG2;
    		inPort->sPortParam.format.video.eColorFormat       	 	= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_MPEG2);

			break;
			
		case VIDDEC_INIT_DIV3:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_DIV3;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingDIV3;
    		inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEDIV3;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEDIV3);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingDIV3;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_DIV3);
			
			break;
			
		case VIDDEC_INIT_AVS:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_AVS;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingAVS;
    		inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEAVS;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEAVS);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingAVS;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_AVS);
			
			break;
		case VIDDEC_INIT_VP6:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_VP6;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingVP6;
    		inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEAVS;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEVP6);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingVP6;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_VP6);
			
			break;
			case VIDDEC_INIT_VP8:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_VP8;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingVP8;
    		inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEAVS;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEVP8);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingVP8;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_VP8);
			
			break;	
		case VIDDEC_INIT_FLV1:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_FLV1;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingFLV1;
    		inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEAVS;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEFLV1);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingFLV1;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_FLV1);
			
			break;	
		case VIDDEC_INIT_RV:
			inPort->sVideoParam.nPortIndex                   			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_RV;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingRV;
       	 	inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPERV;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPERV);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingRV;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

//			pComponentPrivate->IsCodaSeqInitBuf  								= OMX_TRUE;
//			pComponentPrivate->CodaSeqInitBufLen								= 0;
//			pComponentPrivate->IsNeedSwapBack									= OMX_FALSE;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_RV);
			break;
			
		case VIDDEC_INIT_RVG2:
			inPort->sVideoParam.nPortIndex                   			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_RVG2;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingRVG2;
       	 	inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPERV;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPERV);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingRVG2;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

//			pComponentPrivate->IsCodaSeqInitBuf  								= OMX_TRUE;
//			pComponentPrivate->CodaSeqInitBufLen								= 0;
//			pComponentPrivate->IsNeedSwapBack									= OMX_FALSE;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_RVG2);
			break;
			
		case VIDDEC_INIT_MJPEG:
			DEBUG(DEB_LEV_FULL_SEQ,"in VIDDEC_INIT_MJPEG\n");
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_MJPEG;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingMJPEG;
   	 		inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEMJPEG;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEMJPEG);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingMJPEG;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_WIDTH * VIDDEC_DEFAULT_HEIGHT *
                                                                              								VIDDEC_FACTORFORMATSEMI420;
			outPort->sPortParam.nBufferCountActual             		= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_MJPEG);
DEBUG(DEB_LEV_FULL_SEQ,"out VIDDEC_INIT_MJPEG\n");			
			break;
    
    case VIDDEC_INIT_SEMIPLANAR420:
			outPort->sVideoParam.nPortIndex                    		= VIDDEC_OUTPUT_PORT;
    		outPort->sVideoParam.nIndex                        		= VIDDEC_DEFAULT_OUTPUT_INDEX_SEMIPLANAR420;
    		outPort->sVideoParam.eCompressionFormat            	= OMX_VIDEO_CodingUnused;
    		outPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATSEMI420;
			
			outPort->sPortParam.nBufferSize                    			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual             		= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			//outPort->sPortParam.format.video.cMIMEType			= VIDDEC_MIMETYPEYUV;
			strcpy(outPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEYUV);
    		outPort->sPortParam.format.video.nFrameWidth        		= VIDDEC_DEFAULT_WIDTH;
    		outPort->sPortParam.format.video.nFrameHeight       		= VIDDEC_DEFAULT_HEIGHT;
			outPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			outPort->sPortParam.format.video.nSliceHeight			= 0;
    		outPort->sPortParam.format.video.eCompressionFormat 	= OMX_VIDEO_CodingUnused;
    		outPort->sPortParam.format.video.eColorFormat       		= VIDDEC_COLORFORMATSEMI420;
			
            break;
            
		case VIDDEC_INIT_PLANAR420:
			outPort->sVideoParam.nPortIndex                    		= VIDDEC_OUTPUT_PORT;
    		outPort->sVideoParam.nIndex                        		= VIDDEC_DEFAULT_OUTPUT_INDEX_PLANAR420;
    		outPort->sVideoParam.eCompressionFormat            	= OMX_VIDEO_CodingUnused;
    		outPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMAT420;
			
			outPort->sPortParam.nBufferSize                    			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual             		= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			//outPort->sPortParam.format.video.cMIMEType			= VIDDEC_MIMETYPEYUV;
			strcpy(outPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEYUV);
    		outPort->sPortParam.format.video.nFrameWidth        		= VIDDEC_DEFAULT_WIDTH;
    		outPort->sPortParam.format.video.nFrameHeight       		= VIDDEC_DEFAULT_HEIGHT;
			outPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			outPort->sPortParam.format.video.nSliceHeight			= 0;
    		outPort->sPortParam.format.video.eCompressionFormat 	= OMX_VIDEO_CodingUnused;
    		outPort->sPortParam.format.video.eColorFormat       		= VIDDEC_COLORFORMAT420;
			
            break;
			
		case VIDDEC_INIT_IDLEEXECUTING:
//			pComponentPrivate->bIsPaused                        						= OMX_FALSE;
//			pComponentPrivate->nCountInputBFromApp 							= 0;
//			pComponentPrivate->nCountOutputBFromApp						= 0;
//			
//			pComponentPrivate->CodaSeqInited  								= OMX_FALSE;
//			pComponentPrivate->CodaFrameBRegistered							= OMX_FALSE;
//
//			pComponentPrivate->bIsInputFlushPending							= OMX_FALSE;
//			pComponentPrivate->bIsOutputFlushPending						 	= OMX_FALSE;
//
//			pComponentPrivate->bInPortSettingsChanged						= OMX_FALSE;
//			pComponentPrivate->bOutPortSettingsChanged						= OMX_FALSE;
//			pComponentPrivate->bDynamicConfigurationInProgress				 	= OMX_FALSE;
//			
//			pComponentPrivate->NeedFreeAllBuffers 							= OMX_TRUE;

			break;
			
		case VIDDEC_INIT_LOOP:
//			pComponentPrivate->bIsPaused                        						= OMX_FALSE;
//			pComponentPrivate->nCountInputBFromApp 							= 0;
//			pComponentPrivate->nCountOutputBFromApp						= 0;
//			
//			pComponentPrivate->CodaSeqInited  								= OMX_FALSE;
//			pComponentPrivate->CodaFrameBRegistered							= OMX_FALSE;
//
//
//			pComponentPrivate->bIsInputFlushPending							= OMX_FALSE;
//			pComponentPrivate->bIsOutputFlushPending						 	= OMX_FALSE;
//
//			pComponentPrivate->bInPortSettingsChanged						= OMX_FALSE;
//			pComponentPrivate->bOutPortSettingsChanged						= OMX_FALSE;
//			pComponentPrivate->bDynamicConfigurationInProgress				 	= OMX_FALSE;
//			
//			pComponentPrivate->NeedFreeAllBuffers 							= OMX_TRUE;
//
//			pComponentPrivate->SeqErrorTest=0;
			break;
			
		}
EXIT:

	if(eError != OMX_ErrorNone)
	{
		DEBUG(DEB_LEV_FULL_SEQ,"In %s VIDDEC_Load_Defaults error\n", __func__);	
	}
	


	return eError;

}


OMX_ERRORTYPE omx_videodec_component_FillThisBuffer(
  OMX_HANDLETYPE hComponent,
  OMX_BUFFERHEADERTYPE* pBufferHead) {

  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
  omx_base_video_PortType *pPort;
  OMX_ERRORTYPE err;

    
  
  
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for component %p\n", __func__, hComponent);

  if (pBufferHead->nOutputPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                                    omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                                    omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                                    omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
    DEBUG(DEB_LEV_ERR, "In %s: wrong port index\n", __func__);
    return OMX_ErrorBadPortIndex;
  }
  pPort = omx_base_component_Private->ports[pBufferHead->nOutputPortIndex];
  if (pPort->sPortParam.eDir != OMX_DirOutput) {
	  DEBUG(DEB_LEV_ERR, "In %s: wrong port(%d) direction(%x) pBufferHead=%p in Component %s\n", __func__,
			  (int)pBufferHead->nOutputPortIndex, (int)pPort->sPortParam.eDir, pBufferHead, omx_base_component_Private->name);
    return OMX_ErrorBadPortIndex;
  }
  err = pPort->Port_SendBufferFunction(pPort,  pBufferHead);
  if (err != OMX_ErrorNone) {
	  DEBUG(DEB_LEV_ERR, "Out of %s for component %p with err %s\n", __func__, hComponent, errorName(err));
	  return err;
  }

  
  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for component %p\n", __func__, hComponent);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_videodec_component_SetParameter(
OMX_IN  OMX_HANDLETYPE hComponent,
OMX_IN  OMX_INDEXTYPE nParamIndex,
OMX_IN  OMX_PTR ComponentParameterStructure) 
{
	OMX_COMPONENTTYPE* pHandle= NULL;
	VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_PARAM_COMPONENTROLETYPE *pRole = NULL;
	OMX_PARAM_COMPONENTROLETYPE compRole;
	omx_base_video_PortType *inPort,*outPort;	
	StoreMetaDataInBuffersParams *pMediaType;
	DEBUG(DEB_LEV_FULL_SEQ,"   Setting parameter %x,%s\n", nParamIndex,__func__);
	
	OMX_CONF_CHECK_CMD(hComponent, ComponentParameterStructure, OMX_TRUE);
	pHandle= (OMX_COMPONENTTYPE*)hComponent;
	pComponentPrivate = pHandle->pComponentPrivate;
	
	inPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	outPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

	eError = VIDDEC_CheckSetParameter(pComponentPrivate, ComponentParameterStructure, nParamIndex);

	if (eError != OMX_ErrorNone)
	{
		eError = OMX_ErrorIncorrectStateOperation;
		DEBUG(DEB_LEV_FULL_SEQ,"CheckSetParameter error ,%s\n",__func__);
		goto EXIT;
	}
	
	switch (nParamIndex) 
	{
		 case OMX_IndexParameterStoreMediaData:
			pMediaType = (StoreMetaDataInBuffersParams*)ComponentParameterStructure;		
			if(pMediaType->nPortIndex==OMX_ALL){
				pComponentPrivate->bStoreMediadata[VIDDEC_INPUT_PORT] = pMediaType->bStoreMetaData;
				pComponentPrivate->bStoreMediadata[VIDDEC_OUTPUT_PORT]= pMediaType->bStoreMetaData;
			}else if(pMediaType->nPortIndex==VIDDEC_INPUT_PORT){
				pComponentPrivate->bStoreMediadata[VIDDEC_INPUT_PORT] = pMediaType->bStoreMetaData;
			}else if(pMediaType->nPortIndex==VIDDEC_OUTPUT_PORT){
				pComponentPrivate->bStoreMediadata[VIDDEC_OUTPUT_PORT] = pMediaType->bStoreMetaData;
			}else{
				eError = OMX_ErrorBadPortIndex;
				goto EXIT;
			}
			break;
		case OMX_IndexThumbnail:
			pComponentPrivate->is_Thumbnail=OMX_TRUE;
			DEBUG(DEB_LEV_FULL_SEQ,"SetParameter OMX_IndexThumbnail\n");
			break;
		case OMX_IndexParamStreamBufferMode://sk added
			DEBUG(DEB_LEV_FULL_SEQ,"SetParameter OMX_IndexParamStreamBufferMode\n");			
			break;
		case OMX_GoogleAndroidIndexEnableAndroidNativeBuffers:
			DEBUG(DEB_LEV_FULL_SEQ,"SetParameter OMX_GoogleAndroidIndexEnableAndroidNativeBuffers\n");			
			break;
    case OMX_IndexParamStandardComponentRole:
    		DEBUG(DEB_LEV_FULL_SEQ,"SetParameter OMX_IndexParamStandardComponentRole 0\n");
    		if (ComponentParameterStructure != NULL) 
    		{
    			char std_string[10];
        		pRole = (OMX_PARAM_COMPONENTROLETYPE *)ComponentParameterStructure;
DEBUG(DEB_LEV_FULL_SEQ,"SetParameter OMX_IndexParamStandardComponentRole pRole->cRole %s\n",pRole->cRole);        		
        		/*OMX_CONF_CHK_VERSION( pRole, OMX_PARAM_COMPONENTROLETYPE, eError, pComponentPrivate->dbg);*/
        		if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_H263) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_H263);
                    		strcpy(std_string, "h263");
                		}
                		else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_AVC) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_AVC);
                    		strcpy(std_string, "h264");
                		}else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_HEVC) == 0){
                			  eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_HEVC);
                    		strcpy(std_string, "hevc");
                		}else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_HEVC_91) == 0){
                			  eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_HEVC_91);
                    		strcpy(std_string, "hevc_91");
                		}else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_VP8) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_VP8);
                    		strcpy(std_string, "vp8");
                		}
                		else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_VP6) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_VP6);
                    		strcpy(std_string, "vp6");
                		}
                		else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_MPEG2) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_MPEG2);
                    		strcpy(std_string, "mpeg");
                		}
                		else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_MPEG4) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_MPEG4);
                    		strcpy(std_string, "xvid");
                		}
                		else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_VC1) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_VC1);
                    		strcpy(std_string, "vc1");
                		}
										else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_AVS) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_AVS);
                    		strcpy(std_string, "avs");
                		}
										else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_RV) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_RV);
                    		strcpy(std_string, "rv34");
                		}
										else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_MJPEG) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_MJPEG);
                    		strcpy(std_string, "mjpg");
                		}
										else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_DIV3) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_DIV3);
                    		strcpy(std_string, "msm4");
                		}
										else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_FLV1) == 0) {
											eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_FLV1);
											strcpy(std_string, "flv1");
										}
										else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_RVG2) == 0){
											eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_RVG2);
											strcpy(std_string, "rvg2");
										}
            				else {
									DEBUG(DEB_LEV_FULL_SEQ,"%s %d,badparameter\n",__FUNCTION__,__LINE__);
                    		eError = OMX_ErrorBadParameter;
                		}
						
                		if(eError != OMX_ErrorNone) {
                   	 		goto EXIT;
                		}
        if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_RVG2) == 0 || strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_HEVC) == 0 || strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_HEVC_91) == 0){
        	if( outPort->sVideoParam.eColorFormat != VIDDEC_COLORFORMAT420) {
								eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_PLANAR420);
						if(eError != OMX_ErrorNone) {
									goto EXIT;
								}
							}
				} else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_MJPEG) != 0){
					if( outPort->sVideoParam.eColorFormat != VIDDEC_COLORFORMATSEMI420) {
								eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_SEMIPLANAR420);
						if(eError != OMX_ErrorNone) {
									goto EXIT;
								}
							}
				}
        		memcpy( (void *)&(pComponentPrivate->componentRole), (void *)pRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
 
        		//dlopen act plugin here by Reis
//        		eError = omx_videodec_component_actvideoInit(pComponentPrivate);
//        		if(eError != OMX_ErrorNone) {
//        			DEBUG(DEB_LEV_FULL_SEQ,"In %s actvideoInit Error=%x\n",__func__,eError); 
//        			goto EXIT;
//        		}
    		} 
    		else {
    			DEBUG(DEB_LEV_FULL_SEQ,"%s %d,badparameter\n",__FUNCTION__,__LINE__);
        		eError = OMX_ErrorBadParameter;
        		goto EXIT;
    		}
    		break;

		case OMX_IndexParamVideoPortFormat:
		{
			OMX_VIDEO_PARAM_PORTFORMATTYPE* pPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
	    	if (pPortFormat->nPortIndex == VIDDEC_INPUT_PORT) 
	    	{
	        	if(pPortFormat->eColorFormat == OMX_COLOR_FormatUnused) 
	        	{
						
	            	switch (pPortFormat->eCompressionFormat) 
	            	{
						case OMX_VIDEO_CodingH263:
							//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPEH263;
							strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEH263);
		            		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingH263;
		            		break;
		        case OMX_VIDEO_CodingFLV1:
	                		//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPEDIV3;
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEFLV1);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingFLV1;
	                		break;
						case OMX_VIDEO_CodingAVC:			
							strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEAVC);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingAVC;
	                		break;
	                		
	          
	                		
	          case OMX_VIDEO_CodingMPEG2:
							
							strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEMPEG2);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingMPEG2;
	               	 		break;
	          case OMX_VIDEO_CodingHEVC:
							
							strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEHEVC);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingHEVC;
	               	 		break;
	           case OMX_VIDEO_CodingHEVC_91:
							
							strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEHEVC_91);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingHEVC_91;
	               	 		break;
	               	 		
						case OMX_VIDEO_CodingMPEG4:
	                		
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEMPEG4);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingMPEG4;
	                		break;
						case OMX_VIDEO_CodingRV:
	                		
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPERV);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingRV;
	                		break;
						case OMX_VIDEO_CodingVC1:
	                		
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEVC1);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingVC1;
	                		break;
						case OMX_VIDEO_CodingMJPEG:
	                		
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEMJPEG);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingMJPEG;
	                		break;
						case OMX_VIDEO_CodingAVS:
	                		
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEAVS);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingAVS;
	                		break;
						case OMX_VIDEO_CodingDIV3:
	                		
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEDIV3);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingDIV3;
	                		break;
	          case OMX_VIDEO_CodingVP6:
	                	
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEVP6);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingVP6;
	                		break;
	          case OMX_VIDEO_CodingVP8:
	                		
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEVP8);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingVP8;
	                		break;
						case OMX_VIDEO_CodingRVG2:
							
							strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPERVG2);
							inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingRVG2;
							break;
						default:
	                        eError = OMX_ErrorNoMore;	
							goto EXIT;
	                        break;
		            }
		        }
		        else 
		        {
		    		eError = OMX_ErrorBadParameter;
					goto EXIT;
				}
					
		 		if(eError == OMX_ErrorNone) 
		 		{
					memcpy(&inPort->sVideoParam, pPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
				}	
	    	}
	    	else if (pPortFormat->nPortIndex == VIDDEC_OUTPUT_PORT) 
	    	{
	        	if(pPortFormat->eCompressionFormat == OMX_VIDEO_CodingUnused) 
	        	{
	    			if(inPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingMJPEG) 
	    			{
	            		switch (pPortFormat->eColorFormat) 
	            		{
	                		case OMX_COLOR_FormatYUV420Planar:
	                			outPort->sPortParam.format.video.eColorFormat = VIDDEC_COLORFORMAT420;
	                			break;
	                		case OMX_COLOR_FormatYUV420SemiPlanar:
	                			outPort->sPortParam.format.video.eColorFormat = VIDDEC_COLORFORMATSEMI420;
	                			break;
	                		case OMX_COLOR_FormatYUV422Planar:
	                			outPort->sPortParam.format.video.eColorFormat = VIDDEC_COLORFORMAT422;
	                			break;
							case OMX_COLOR_FormatL8:
								outPort->sPortParam.format.video.eColorFormat = VIDDEC_COLORFORMAT400;
				                break;
							case OMX_COLOR_FormatYUV444Planar:
								outPort->sPortParam.format.video.eColorFormat = VIDDEC_COLORFORMAT444;
								break;
							default:
								eError = OMX_ErrorNoMore;
								goto EXIT;
			                    break;
			            }
	            	}
	            	else
	            	{
	            		switch (pPortFormat->eColorFormat) 
	            		{
	                		case OMX_COLOR_FormatYUV420Planar:
	                    		outPort->sPortParam.format.video.eColorFormat = VIDDEC_COLORFORMAT420;
	                    		break;
	                    case OMX_COLOR_FormatYUV420SemiPlanar:
	                			outPort->sPortParam.format.video.eColorFormat = VIDDEC_COLORFORMATSEMI420;
	                			break;
	                		default:
	                    		eError = OMX_ErrorNoMore;
								goto EXIT;
	                    		break;
	            		}
	        		}
	        	}
	        	else 
	        	{
	            	eError = OMX_ErrorBadParameter;
					goto EXIT;
	        	}
				
	        	if(eError == OMX_ErrorNone) 
	        	{
	            		memcpy(&(outPort->sVideoParam), pPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	        	}
	    	}
	    	else 
	    	{
	        	eError = OMX_ErrorBadPortIndex;
				goto EXIT;
	    	}
			break;
		} 
		case OMX_IndexParamPortDefinition:
		{
	    	OMX_PARAM_PORTDEFINITIONTYPE* pPortDefParam = (OMX_PARAM_PORTDEFINITIONTYPE*)ComponentParameterStructure;
	    	if (pPortDefParam->nPortIndex == VIDDEC_INPUT_PORT) 
	    	{
	        	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = &(inPort->sPortParam);		
	        	memcpy(pPortDef, pPortDefParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
				    pPortDef->format.video.nStride = \
				    pPortDef->format.video.nFrameWidth=(((pPortDef->format.video.nFrameWidth + 31)&(~31))/2+31)&(~31);
				    pPortDef->format.video.nFrameHeight = (((pPortDef->format.video.nFrameHeight + 15)&(~15))/2+15)&(~15);
	
			}
			else if (pPortDefParam->nPortIndex == VIDDEC_OUTPUT_PORT) 
			{
	        	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = &(outPort->sPortParam);
	        	OMX_CONFIG_RECTTYPE *PortOutCrop= &(pComponentPrivate->croprect);
	        	memcpy(pPortDef, pPortDefParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	        	pComponentPrivate->actualframewidth =(pPortDef->format.video.nFrameWidth + 31)&(~31);
	        	pComponentPrivate->actualframeheight = (pPortDef->format.video.nFrameHeight + 15)&(~15);
	        	pComponentPrivate->actualdisplaywidth =  pPortDef->format.video.nFrameWidth;
            pComponentPrivate->actualdisplayheight = pPortDef->format.video.nFrameHeight;
	        	PortOutCrop->nSize = sizeof(OMX_CONFIG_RECTTYPE);
  			 		PortOutCrop->nLeft = 0;
  		   		PortOutCrop->nTop =  0;
  		   		PortOutCrop->nWidth =  pComponentPrivate->actualframewidth/2;
  		   		PortOutCrop->nHeight = pComponentPrivate->actualframeheight/2;
	        	pPortDef->format.video.nStride = \
				    pPortDef->format.video.nFrameWidth=(((pPortDef->format.video.nFrameWidth + 31)&(~31))/2+31)&(~31);
				    pPortDef->format.video.nFrameHeight = (((pPortDef->format.video.nFrameHeight + 15)&(~15))/2+15)&(~15);
	        	if(pPortDef->format.video.nFrameWidth == 0)
	        	{
	        		pPortDef->format.video.nFrameWidth = VIDDEC_DEFAULT_WIDTH;
	        	}
	        	if(pPortDef->format.video.nFrameHeight == 0)
	        	{
	        		pPortDef->format.video.nFrameHeight = VIDDEC_DEFAULT_HEIGHT;
	        	}
	        	
				DEBUG(DEB_LEV_FULL_SEQ,"----------------pPortDef->format.video.nStride %d,nBufferCounter is %d \n",pPortDef->format.video.nStride,pPortDef->nBufferCountActual);
				if(inPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingMJPEG)
				{
					switch(pPortDef->format.video.eColorFormat)
					{
						case VIDDEC_COLORFORMAT420:
							if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC) || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC_91)){
								pPortDef->nBufferSize = (((pPortDef->format.video.nStride+31)&(~31))+64) * 	(((pPortDef->format.video.nFrameHeight+31)&(~31))+64) * VIDDEC_FACTORFORMAT420;
						  }else{
						  	pPortDef->nBufferSize = pPortDef->format.video.nStride * 	pPortDef->format.video.nFrameHeight * VIDDEC_FACTORFORMAT420;
								pPortDef->nBufferSize += (pPortDef->format.video.nStride *	pPortDef->format.video.nFrameHeight)/4;
						  }
							break;
						case VIDDEC_COLORFORMATSEMI420:
							pPortDef->nBufferSize = pPortDef->format.video.nStride *	pPortDef->format.video.nFrameHeight * VIDDEC_FACTORFORMATSEMI420;
							break;
						case VIDDEC_COLORFORMAT422:
							pPortDef->nBufferSize = pPortDef->format.video.nStride * 	pPortDef->format.video.nFrameHeight * VIDDEC_FACTORFORMAT422;
							break;
						case VIDDEC_COLORFORMAT400:
							pPortDef->nBufferSize = pPortDef->format.video.nStride *	pPortDef->format.video.nFrameHeight * VIDDEC_FACTORFORMAT400;
							break;
						case VIDDEC_COLORFORMAT444:
							pPortDef->nBufferSize = pPortDef->format.video.nStride *	pPortDef->format.video.nFrameHeight * VIDDEC_FACTORFORMAT444;
							break;
						default:
							eError = OMX_ErrorNoMore;
							goto EXIT;
							break;
							
					}
				}
				else
				{
					switch(pPortDef->format.video.eColorFormat)
					{
						case VIDDEC_COLORFORMAT420:
							pPortDef->nBufferSize = pPortDef->format.video.nStride * 	pPortDef->format.video.nFrameHeight * VIDDEC_FACTORFORMAT420;
							pPortDef->nBufferSize += (pPortDef->format.video.nStride *	pPortDef->format.video.nFrameHeight)/4;
							break;
						case VIDDEC_COLORFORMATSEMI420:
							pPortDef->nBufferSize = pPortDef->format.video.nStride * 	pPortDef->format.video.nFrameHeight * VIDDEC_FACTORFORMATSEMI420;
							pPortDef->nBufferSize += 1*(pPortDef->format.video.nStride *	pPortDef->format.video.nFrameHeight)/4;
							break;
						default:
							eError = OMX_ErrorNoMore;
							goto EXIT;
							break;
					}
DEBUG(DEB_LEV_FULL_SEQ,"----------------pPortDef->nBufferSize %d\n",pPortDef->nBufferSize);
				}
	        }
	        else 
	        {
				eError = OMX_ErrorBadPortIndex;
				goto EXIT;
	        }
			break;
		}
		case OMX_IndexParamVideoInit:
          memcpy(pComponentPrivate->pPortParamType, (OMX_PORT_PARAM_TYPE*)ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE));
          break;
        
    	case OMX_IndexParamAudioInit:
    	  memcpy(pComponentPrivate->pPortParamTypeAudio, (OMX_PORT_PARAM_TYPE*)ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE));
          break;
    	case OMX_IndexParamImageInit:
    	  memcpy(pComponentPrivate->pPortParamTypeImage, (OMX_PORT_PARAM_TYPE*)ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE));
          break;
    	case OMX_IndexParamOtherInit:
    	 memcpy(pComponentPrivate->pPortParamTypeOthers, (OMX_PORT_PARAM_TYPE*)ComponentParameterStructure, sizeof(OMX_PORT_PARAM_TYPE));
          break;
      case OMX_IndexParamNumAvailableStreams:
      	  break;
    	case OMX_IndexParamVideoWmv:
    	case OMX_IndexParamVideoMpeg4:
    	case OMX_IndexParamVideoMpeg2: 
    	case OMX_IndexParamVideoAvc:
    	case OMX_IndexParamVideoH263:
    	case OMX_IndexParamPriorityMgmt:
    	case OMX_IndexParamCompBufferSupplier:
    	case OMX_IndexConfigVideoMBErrorReporting:/**< reference: OMX_CONFIG_MBERRORREPORTINGTYPE */
    	case OMX_IndexParamCommonDeblocking: /**< reference: OMX_PARAM_DEBLOCKINGTYPE */
    	case OMX_IndexParamVideoMacroblocksPerFrame:
    	case OMX_IndexParamActiveStream:
    	case OMX_IndexParamSuspensionPolicy:
    	case OMX_IndexParamComponentSuspended:
    	case OMX_IndexAutoPauseAfterCapture:		
    	case OMX_IndexParamCustomContentPipe:
    	case OMX_IndexParamDisableResourceConcealment:
 		case OMX_IndexConfigMetadataItemCount:
   	case OMX_IndexConfigContainerNodeCount:
   	case OMX_IndexConfigMetadataItem:
  		case OMX_IndexConfigCounterNodeID:
   	case OMX_IndexParamMetadataFilterType:
    	case OMX_IndexConfigCommonTransitionEffect:
    	case OMX_IndexKhronosExtensions:
    	case OMX_IndexParamMetadataKeyFilter:
    	case OMX_IndexConfigPriorityMgmt:
    	case OMX_IndexConfigAudioChannelVolume:
    	case OMX_IndexConfigFlashControl:
    	case OMX_IndexParamVideoProfileLevelQuerySupported:
    	case OMX_IndexParamVideoProfileLevelCurrent:			
    	case OMX_IndexConfigVideoBitrate:
    	case OMX_IndexConfigVideoFramerate:
	case OMX_IndexConfigVideoIntraVOPRefresh:
    	case OMX_IndexConfigVideoIntraMBRefresh:
    	case OMX_IndexConfigVideoMacroBlockErrorMap:
    	case OMX_IndexParamVideoSliceFMO:
    	case OMX_IndexConfigVideoAVCIntraPeriod:
    	case OMX_IndexConfigVideoNalSize:
    	case OMX_IndexConfigCommonExposureValue:
    	case OMX_IndexConfigCommonOutputSize:
    	case OMX_IndexParamCommonExtraQuantData:
    	case OMX_IndexConfigCommonFocusRegion:
    	case OMX_IndexConfigCommonFocusStatus:
    	case OMX_IndexParamContentURI:
    	case OMX_IndexConfigCaptureMode:
    	case OMX_IndexConfigCapturing:
		
    	default:

    		DEBUG(DEB_LEV_FULL_SEQ,"In %s index %x not spt\n",__func__,nParamIndex);

        	eError = OMX_ErrorUnsupportedIndex;
        	break;
	}
EXIT:

if(eError != OMX_ErrorNone)
{
	DEBUG(DEB_LEV_FULL_SEQ,"In %s Error=%x\n",__func__,eError);
}
return eError;
}

OMX_ERRORTYPE omx_videodec_component_GetParameter(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_INDEXTYPE nParamIndex,
  OMX_INOUT OMX_PTR ComponentParameterStructure) 
{
	OMX_COMPONENTTYPE* pComp = NULL;
	VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = NULL;
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	OMX_PARAM_COMPONENTROLETYPE *pRole = NULL;	
	
	omx_base_video_PortType *inPort,*outPort;
	DEBUG(DEB_LEV_FULL_SEQ,"in omx_videodec_component_GetParameter,nParamIndex:%x\n",nParamIndex);

	OMX_CONF_CHECK_CMD(hComponent, ComponentParameterStructure, OMX_TRUE);
	pComp = (OMX_COMPONENTTYPE*)hComponent;
	pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)pComp->pComponentPrivate;

	if (pComponentPrivate->state == OMX_StateInvalid) 
	{
		DEBUG(DEB_LEV_FULL_SEQ,"In %s State Error=%x\n",__func__,eError);
		eError = OMX_ErrorInvalidState;
		goto EXIT;
	}
	inPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	outPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

	switch (nParamIndex) {
	case OMX_IndexParamPortDefinition:
	{
		OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
		if (pPortDef->nPortIndex == VIDDEC_INPUT_PORT)
		 {		
			OMX_PARAM_PORTDEFINITIONTYPE *pInPortDef = &(inPort->sPortParam);
			memcpy(pPortDef, pInPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
		}
		else if (pPortDef->nPortIndex == VIDDEC_OUTPUT_PORT) 
		{
			OMX_PARAM_PORTDEFINITIONTYPE *pOutPortDef = &(outPort->sPortParam);
      memcpy(pPortDef, pOutPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));;
     }
     else 
     {
        eError = OMX_ErrorBadPortIndex;
				goto EXIT;
     }
		break;
	}
	case OMX_IndexParamVideoPortFormat:
	{
		OMX_VIDEO_PARAM_PORTFORMATTYPE* pPortFormat = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)ComponentParameterStructure;
		OMX_U32 nNumberOfSupported = 0;
		OMX_U32 index =0;
		if (pPortFormat->nPortIndex == VIDDEC_INPUT_PORT) 
		{
			switch (pPortFormat->nIndex) 
			{
				case VIDDEC_DEFAULT_INPUT_INDEX_AVC:
	       			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_AVC;
	       			inPort->sVideoParam.eCompressionFormat     	= OMX_VIDEO_CodingAVC;
	        		break;
	      case VIDDEC_DEFAULT_INPUT_INDEX_HEVC:
	       			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_HEVC;
	       			inPort->sVideoParam.eCompressionFormat     	= OMX_VIDEO_CodingHEVC;
	        		break;
	      case VIDDEC_DEFAULT_INPUT_INDEX_HEVC_91:
	       			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_HEVC_91;
	       			inPort->sVideoParam.eCompressionFormat     	= OMX_VIDEO_CodingHEVC_91;
	        		break;
				case VIDDEC_DEFAULT_INPUT_INDEX_H263:
 					inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_H263;
    				inPort->sVideoParam.eCompressionFormat        	= OMX_VIDEO_CodingH263;
            		break;
				case VIDDEC_DEFAULT_INPUT_INDEX_VC1:
    				inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_VC1;
      				inPort->sVideoParam.eCompressionFormat       	= OMX_VIDEO_CodingVC1;
       				break;
      			case VIDDEC_DEFAULT_INPUT_INDEX_MPEG2:
    				inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_MPEG2;
      				inPort->sVideoParam.eCompressionFormat        	= OMX_VIDEO_CodingMPEG2;
       				break;
    			case VIDDEC_DEFAULT_INPUT_INDEX_DIV3:
        			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_DIV3;
      				inPort->sVideoParam.eCompressionFormat        	= OMX_VIDEO_CodingDIV3;
      				break;
    			case VIDDEC_DEFAULT_INPUT_INDEX_RVG2:

    				inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_RVG2;
    				inPort->sVideoParam.eCompressionFormat        	= OMX_VIDEO_CodingRVG2;
    				break;
				case VIDDEC_DEFAULT_INPUT_INDEX_MPEG4:
          			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_MPEG4;
           			inPort->sVideoParam.eCompressionFormat       	= OMX_VIDEO_CodingMPEG4;
         			break;
				case VIDDEC_DEFAULT_INPUT_INDEX_AVS:
        			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_AVS;
      				inPort->sVideoParam.eCompressionFormat      	= OMX_VIDEO_CodingAVS;
      				break;
				case VIDDEC_DEFAULT_INPUT_INDEX_RV:
        			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_RV;
      				inPort->sVideoParam.eCompressionFormat        	= OMX_VIDEO_CodingRV;
      				break;
      	case VIDDEC_DEFAULT_INPUT_INDEX_FLV1:
        			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_FLV1;
      				inPort->sVideoParam.eCompressionFormat        	= OMX_VIDEO_CodingFLV1;
      				break;
      	case VIDDEC_DEFAULT_INPUT_INDEX_VP6:
        			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_VP6;
      				inPort->sVideoParam.eCompressionFormat        	= OMX_VIDEO_CodingVP6;
      				break;
      	case VIDDEC_DEFAULT_INPUT_INDEX_VP8:
        			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_VP8;
      				inPort->sVideoParam.eCompressionFormat        	= OMX_VIDEO_CodingVP8;
      				break;
				case VIDDEC_DEFAULT_INPUT_INDEX_MJPEG:
        			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_MJPEG;
      				inPort->sVideoParam.eCompressionFormat      	= OMX_VIDEO_CodingMJPEG;
      				break;
      			default:
         			eError = OMX_ErrorNoMore;		
         			break;
			}
	  		if(eError == OMX_ErrorNone) {
	    			memcpy(ComponentParameterStructure, &(inPort->sVideoParam), 
	                    		sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	  		}
  		}
  		else if (pPortFormat->nPortIndex == VIDDEC_OUTPUT_PORT) 
  		{
		   	 nNumberOfSupported = 2;
		   	 index = pPortFormat->nIndex;
		   	 if(pPortFormat->nIndex >= nNumberOfSupported)
			 	 {
					DEBUG(DEB_LEV_ERR, "Warning!VideoPortFormat is no more!%s,%d\n",__FILE__,__LINE__);
					return OMX_ErrorNoMore;
				 }
				 memcpy(ComponentParameterStructure, &(outPort->sVideoParam),
              			sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
      	 pComponentPrivate->output_portformat_inited = OMX_TRUE;
      	 pPortFormat->nIndex = index;

    	}
   		else 
   		{
   			DEBUG(DEB_LEV_FULL_SEQ,"In %s port index Error=%x\n",__func__,eError);
          		eError = OMX_ErrorBadPortIndex;
			goto EXIT;
     	}
		break;
	}
	case OMX_IndexParamStandardComponentRole:

 		if (ComponentParameterStructure != NULL) 
 		{
        	pRole = (OMX_PARAM_COMPONENTROLETYPE *)ComponentParameterStructure;
        	
        	memcpy( pRole, &(pComponentPrivate->componentRole), sizeof(OMX_PARAM_COMPONENTROLETYPE));
        	
    	}
    	else
    	{
        	eError = OMX_ErrorBadParameter;
			goto EXIT;
		}
		break;
		
	case OMX_IndexParamVideoProfileLevelQuerySupported:
	{

		VIDEO_PROFILE_LEVEL_TYPE* pProfileLevel = NULL;
    		OMX_U32 nNumberOfProfiles = 0;
   		OMX_VIDEO_PARAM_PROFILELEVELTYPE *pParamProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)ComponentParameterStructure;
    	pParamProfileLevel->nPortIndex = inPort->sPortParam.nPortIndex;

       		/* Choose table based on compression format */
   		switch(inPort->sPortParam.format.video.eCompressionFormat)
   		{
 			case OMX_VIDEO_CodingH263:
				pProfileLevel = SupportedH263ProfileLevels;
        		nNumberOfProfiles = sizeof(SupportedH263ProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
       			break;
      		case OMX_VIDEO_CodingMPEG4:
				pProfileLevel = SupportedMPEG4ProfileLevels;
         		nNumberOfProfiles = sizeof(SupportedMPEG4ProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
       			break;
       		case OMX_VIDEO_CodingAVC:
				pProfileLevel = SupportedAVCProfileLevels;
         		nNumberOfProfiles = sizeof(SupportedAVCProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
           		break;
        	default:
          	 	return OMX_ErrorBadParameter;
       	}

  		if((pParamProfileLevel->nProfileIndex >= (nNumberOfProfiles - 1)))
  			return OMX_ErrorBadParameter;

		/* Point to table entry based on index */
  		pProfileLevel += pParamProfileLevel->nProfileIndex;

  		/* -1 indicates end of table */
  		if(pProfileLevel->nProfile != -1) 
  		{
     		pParamProfileLevel->eProfile = pProfileLevel->nProfile;
       		pParamProfileLevel->eLevel = pProfileLevel->nLevel;
    			eError = OMX_ErrorNone;
    	}
  		else 
  		{
   			eError = OMX_ErrorNoMore;
   		}
	
  		break;
	}  
	case OMX_GoogleAndroidIndexGetAndroidNativeBufferUsage:
	{
		break;
	}    
    
    case OMX_IndexParamVideoInit:
	{
	
#if 0    
	    OMX_PORT_PARAM_TYPE *portParamType =
                              (OMX_PORT_PARAM_TYPE *) ComponentParameterStructure;
          DEBUG(DEB_LEV_FULL_SEQ,"get_parameter: OMX_IndexParamVideoInit %d %d\n", portParamType->nPorts, portParamType->nStartPortNumber);
 
    	portParamType->nVersion.nVersion=1;
    	portParamType->nVersion.s.nVersionMajor=1;
    	portParamType->nVersion.s.nVersionMinor=0;
    	portParamType->nVersion.s.nRevision=0;
    	portParamType->nVersion.s.nStep=0;
        portParamType->nSize = sizeof(portParamType);
		portParamType->nPorts           = 2;
		portParamType->nStartPortNumber = 0;
#else
		memcpy(ComponentParameterStructure, pComponentPrivate->pPortParamType, sizeof(OMX_PORT_PARAM_TYPE));
   
#endif
	    break;
	}   
	case OMX_IndexParamNumAvailableStreams:
      	  break;
	     case OMX_IndexParamAudioInit:
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortParamTypeAudio, sizeof(OMX_PORT_PARAM_TYPE));
            break;
        case OMX_IndexParamImageInit:
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortParamTypeImage, sizeof(OMX_PORT_PARAM_TYPE));
            break;
        case OMX_IndexParamOtherInit:
            memcpy(ComponentParameterStructure, pComponentPrivate->pPortParamTypeOthers, sizeof(OMX_PORT_PARAM_TYPE));
            break;
    case OMX_IndexConfigVideoMBErrorReporting: /**< reference: OMX_CONFIG_MBERRORREPORTINGTYPE */
    case OMX_IndexParamVideoMacroblocksPerFrame:/**< reference: OMX_PARAM_MACROBLOCKSTYPE */
    case OMX_IndexParamVideoProfileLevelCurrent:              
    case OMX_IndexParamPriorityMgmt:       
    case OMX_IndexParamVideoWmv:
    case OMX_IndexParamVideoMpeg4:
    case OMX_IndexParamVideoMpeg2:
    case OMX_IndexParamVideoAvc:
    case OMX_IndexParamVideoH263:
    case OMX_IndexParamCompBufferSupplier:        
    case OMX_IndexParamCommonDeblocking: /**< reference: OMX_PARAM_DEBLOCKINGTYPE */		
    default:
        eError = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:

	if(eError != OMX_ErrorNone)
	{
		DEBUG(DEB_LEV_FULL_SEQ,"In %s EXIT Error=%x\n",__func__,eError);
	}
	
	return eError;
}
OMX_ERRORTYPE VIDEC_Port_AllocateBuffer(
  omx_base_video_PortType *openmaxStandPort,
  OMX_BUFFERHEADERTYPE** pBuffer,
  OMX_U32 nPortIndex,
  OMX_PTR pAppPrivate,
  OMX_U32 nSizeBytes) {

  unsigned int i;
  long phy_addr;
  OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)(omxComponent->pComponentPrivate);
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for port %p\n", __func__, openmaxStandPort);

  if (nPortIndex != openmaxStandPort->sPortParam.nPortIndex) {
    return OMX_ErrorBadPortIndex;
  }
  if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
    return OMX_ErrorBadPortIndex;
  }

  if (omx_base_component_Private->transientState != OMX_TransStateLoadedToIdle) {
    if (!openmaxStandPort->bIsTransientToEnabled) {
      DEBUG(DEB_LEV_ERR, "In %s: The port is not allowed to receive buffers\n", __func__);
      return OMX_ErrorIncorrectStateTransition;
    }
  }

  if(nSizeBytes < openmaxStandPort->sPortParam.nBufferSize) {
    DEBUG(DEB_LEV_ERR, "In %s: Requested Buffer Size %lu is less than Minimum Buffer Size %lu\n", __func__, nSizeBytes, openmaxStandPort->sPortParam.nBufferSize);
    return OMX_ErrorIncorrectStateTransition;
  }
 
  for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++){
    if (openmaxStandPort->bBufferStateAllocated[i] == BUFFER_FREE) {
      openmaxStandPort->pInternalBufferStorage[i] = calloc(1,sizeof(OMX_BUFFERHEADERTYPE));
      if (!openmaxStandPort->pInternalBufferStorage[i]) {
        return OMX_ErrorInsufficientResources;
      }
      setHeader(openmaxStandPort->pInternalBufferStorage[i], sizeof(OMX_BUFFERHEADERTYPE));
      /* allocate the buffer */
#ifdef ACTIONS_SYS_MEM
			if(pComponentPrivate->is_Thumbnail==OMX_TRUE && nPortIndex==OMX_BASE_FILTER_OUTPUTPORT_INDEX){
				 openmaxStandPort->pInternalBufferStorage[i]->pBuffer = (OMX_U8* )actal_malloc_cached_manual(nSizeBytes,&phy_addr);
			}else{
				if (nPortIndex== OMX_BASE_FILTER_INPUTPORT_INDEX){
					openmaxStandPort->pInternalBufferStorage[i]->pBuffer = (OMX_U8* )actal_malloc_cached_manual(nSizeBytes + sizeof(packet_header_t),&phy_addr);
					openmaxStandPort->pInternalBufferStorage[i]->pBuffer += sizeof(packet_header_t);
				}else{
					openmaxStandPort->pInternalBufferStorage[i]->pBuffer = (OMX_U8* )actal_malloc_cached_manual(nSizeBytes,&phy_addr);
				}
				
			}
     
      DEBUG(DEB_LEV_FULL_SEQ,"===AllocateBuffer:%x,nSizeBytes:%x===\n",phy_addr,nSizeBytes);
#else      
      openmaxStandPort->pInternalBufferStorage[i]->pBuffer = calloc(1,nSizeBytes);
#endif      
      if(openmaxStandPort->pInternalBufferStorage[i]->pBuffer==NULL) {
        return OMX_ErrorInsufficientResources;
      }
      openmaxStandPort->pInternalBufferStorage[i]->nAllocLen = nSizeBytes;
      openmaxStandPort->pInternalBufferStorage[i]->pPlatformPrivate = openmaxStandPort;
      openmaxStandPort->pInternalBufferStorage[i]->pAppPrivate = pAppPrivate;
      *pBuffer = openmaxStandPort->pInternalBufferStorage[i];
  	  (*pBuffer)->pBuffer =openmaxStandPort->pInternalBufferStorage[i]->pBuffer;
      openmaxStandPort->bBufferStateAllocated[i] = BUFFER_ALLOCATED;
      openmaxStandPort->bBufferStateAllocated[i] |= HEADER_ALLOCATED;
      if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
        openmaxStandPort->pInternalBufferStorage[i]->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
      } else {
        openmaxStandPort->pInternalBufferStorage[i]->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
      }
      openmaxStandPort->nNumAssignedBuffers++;
      DEBUG(DEB_LEV_PARAMS, "openmaxStandPort->nNumAssignedBuffers %i\n", (int)openmaxStandPort->nNumAssignedBuffers);

      if (openmaxStandPort->sPortParam.nBufferCountActual == openmaxStandPort->nNumAssignedBuffers) {
        openmaxStandPort->sPortParam.bPopulated = OMX_TRUE;
        openmaxStandPort->bIsFullOfBuffers = OMX_TRUE;
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s nPortIndex=%d\n",__func__,(int)nPortIndex);
        tsem_up(openmaxStandPort->pAllocSem);
      }
      DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for port %p\n", __func__, openmaxStandPort);
      return OMX_ErrorNone;
    }
  }
 
  DEBUG(DEB_LEV_ERR, "Out of %s for port %p. Error: no available buffers\n",__func__, openmaxStandPort);
  return OMX_ErrorInsufficientResources;
}
OMX_ERRORTYPE omx_videodec_component_AllocateBuffer(
            OMX_HANDLETYPE hComponent,
            OMX_BUFFERHEADERTYPE** ppBuffer,
            OMX_U32 nPortIndex,
            OMX_PTR pAppPrivate,
            OMX_U32 nSizeBytes) {
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
  omx_base_video_PortType *pPort;
  OMX_ERRORTYPE err;

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for component %p\n", __func__, hComponent);

  if (nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
    DEBUG(DEB_LEV_ERR, "In %s: wrong port index\n", __func__);
    return OMX_ErrorBadPortIndex;
  }
  pPort = omx_base_component_Private->ports[nPortIndex];
  err = VIDEC_Port_AllocateBuffer(pPort, ppBuffer, nPortIndex, pAppPrivate, nSizeBytes);
  if (err != OMX_ErrorNone) {
	  DEBUG(DEB_LEV_ERR, "Out of %s for component %p with err %i\n", __func__, hComponent, (int)err);
	  return err;
  }
  if(nPortIndex==OMX_BASE_FILTER_OUTPUTPORT_INDEX && pComponentPrivate->nNumAssignedBufferHeads<=pPort->sPortParam.nBufferCountActual)
  {
  	pComponentPrivate->BufferHeadStorage[pComponentPrivate->nNumAssignedBufferHeads++]=*ppBuffer;
  	pComponentPrivate->bOutPutBufferAllocated = BUFFER_ALLOCATED;
  }

  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for component %p buffer %p\n", __func__, hComponent, ppBuffer);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE VIDEC_Port_UseBuffer(
  omx_base_video_PortType *openmaxStandPort,
  OMX_BUFFERHEADERTYPE** ppBufferHdr,
  OMX_U32 nPortIndex,
  OMX_PTR pAppPrivate,
  OMX_U32 nSizeBytes,
  OMX_U8* pBuffer) {

  unsigned int i;
  OMX_BUFFERHEADERTYPE* returnBufferHeader;
  OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for port %p\n", __func__, openmaxStandPort);
  if (nPortIndex != openmaxStandPort->sPortParam.nPortIndex) {
    return OMX_ErrorBadPortIndex;
  }
  if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
    return OMX_ErrorBadPortIndex;
  }

  if (omx_base_component_Private->transientState != OMX_TransStateLoadedToIdle) {
    if (!openmaxStandPort->bIsTransientToEnabled) {
      DEBUG(DEB_LEV_ERR, "In %s: The port of Comp %s is not allowed to receive buffers\n", __func__,omx_base_component_Private->name);
      return OMX_ErrorIncorrectStateTransition;
    }
  }

  for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++){
    if (openmaxStandPort->bBufferStateAllocated[i] == BUFFER_FREE) {
      openmaxStandPort->pInternalBufferStorage[i] = calloc(1,sizeof(OMX_BUFFERHEADERTYPE));
      if (!openmaxStandPort->pInternalBufferStorage[i]) {
        return OMX_ErrorInsufficientResources;
      }
      openmaxStandPort->bIsEmptyOfBuffers = OMX_FALSE;
      setHeader(openmaxStandPort->pInternalBufferStorage[i], sizeof(OMX_BUFFERHEADERTYPE));

      openmaxStandPort->pInternalBufferStorage[i]->pBuffer = pBuffer;
      openmaxStandPort->pInternalBufferStorage[i]->nAllocLen = nSizeBytes;
      openmaxStandPort->pInternalBufferStorage[i]->pPlatformPrivate = openmaxStandPort;
      openmaxStandPort->pInternalBufferStorage[i]->pAppPrivate = pAppPrivate;
      openmaxStandPort->bBufferStateAllocated[i] = BUFFER_ASSIGNED;
      openmaxStandPort->bBufferStateAllocated[i] |= HEADER_ALLOCATED;
      returnBufferHeader = calloc(1,sizeof(OMX_BUFFERHEADERTYPE));
      if (!returnBufferHeader) {
        return OMX_ErrorInsufficientResources;
      }
      setHeader(returnBufferHeader, sizeof(OMX_BUFFERHEADERTYPE));
      returnBufferHeader->pBuffer = pBuffer;
      returnBufferHeader->nAllocLen = nSizeBytes;
      returnBufferHeader->pPlatformPrivate = openmaxStandPort;
      returnBufferHeader->pAppPrivate = pAppPrivate;
      if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
        openmaxStandPort->pInternalBufferStorage[i]->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
        returnBufferHeader->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
      } else {
        openmaxStandPort->pInternalBufferStorage[i]->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
        returnBufferHeader->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
      }
      *ppBufferHdr = returnBufferHeader;
      openmaxStandPort->nNumAssignedBuffers++;
      DEBUG(DEB_LEV_PARAMS, "openmaxStandPort->nNumAssignedBuffers %i\n", (int)openmaxStandPort->nNumAssignedBuffers);

      if (openmaxStandPort->sPortParam.nBufferCountActual == openmaxStandPort->nNumAssignedBuffers) {
        openmaxStandPort->sPortParam.bPopulated = OMX_TRUE;
        openmaxStandPort->bIsFullOfBuffers = OMX_TRUE;
        tsem_up(openmaxStandPort->pAllocSem);
      }
      DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for port %p\n", __func__, openmaxStandPort);
      return OMX_ErrorNone;
    }
  }
  DEBUG(DEB_LEV_ERR, "In %s Error: no available buffers CompName=%s\n",__func__,omx_base_component_Private->name);
  return OMX_ErrorInsufficientResources;
}


OMX_ERRORTYPE omx_videodec_component_UseBuffer(
            OMX_HANDLETYPE hComponent,
            OMX_BUFFERHEADERTYPE** ppBufferHdr,
            OMX_U32 nPortIndex,
            OMX_PTR pAppPrivate,
            OMX_U32 nSizeBytes,
            OMX_U8* pBuffer) {
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
  omx_base_video_PortType *pPort;
  OMX_ERRORTYPE err;

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for component %p\n", __func__, hComponent);
  if (nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
    DEBUG(DEB_LEV_ERR, "In %s: wrong port index\n", __func__);
    return OMX_ErrorBadPortIndex;
  }
  if(pComponentPrivate->bStoreMediadata[nPortIndex]){
		if(nSizeBytes < sizeof(vde_video_metadata_t))
		{
			DEBUG(DEB_LEV_ERR,"Bad buffer size on storemediadata type %d\n",nSizeBytes);
			return OMX_ErrorBadParameter;
		}
	}
	
  pPort = omx_base_component_Private->ports[nPortIndex];
  err = VIDEC_Port_UseBuffer(pPort, ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);
  if (err != OMX_ErrorNone) {
	  DEBUG(DEB_LEV_ERR, "Out of %s for component %p with err %i\n", __func__, hComponent, (int)err);
	  return err;
  }
  if(nPortIndex==OMX_BASE_FILTER_OUTPUTPORT_INDEX && pComponentPrivate->nNumAssignedBufferHeads<=pPort->sPortParam.nBufferCountActual)
  {
  	pComponentPrivate->BufferHeadStorage[pComponentPrivate->nNumAssignedBufferHeads++]=*ppBufferHdr;
  	pComponentPrivate->bOutPutBufferAllocated = BUFFER_ASSIGNED;
  }


  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for component %p\n", __func__, hComponent);
  return OMX_ErrorNone;
}




OMX_ERRORTYPE omx_videodec_component_MessageHandler(OMX_COMPONENTTYPE* openmaxStandComp,internalRequestMessageType *message) {
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)openmaxStandComp->pComponentPrivate;
  omx_base_video_PortType *pInPort=(omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
  omx_base_video_PortType *pOutPort=(omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX]; 
  tsem_t* pOutputSem = pOutPort->pBufferSem;
	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = &(pOutPort->sPortParam);
  OMX_ERRORTYPE err;
  OMX_U32 j;
  OMX_STATETYPE eCurrentState = pComponentPrivate->state;

  DEBUG(DEB_LEV_FULL_SEQ,"In %s,messageType is %d,messageParam is %d\n", __func__,message->messageType,message->messageParam);
  switch(message->messageType){
  	case OMX_CommandStateSet:{

     if ((message->messageParam == OMX_StateIdle ) && (pComponentPrivate->state == OMX_StateLoaded)) {
      		err = omx_videodec_component_Init(openmaxStandComp);
      		if(err!=OMX_ErrorNone) { 
        		DEBUG(DEB_LEV_FULL_SEQ,"In %s Video Decoder Init Failed Error=%x\n",__func__,err); 
        		return err;
      		} 
 
      		omx_base_component_MessageHandler(openmaxStandComp,message);
      		
      
   	 } else if(message->messageParam == OMX_StateExecuting && pComponentPrivate->state == OMX_StateIdle){
   	 		pComponentPrivate->OutPutThreadID=pthread_create(&pComponentPrivate->OutPutThread,
                                                  NULL,
                                                  omx_videodec_component_OutPutFunction,
                                                  openmaxStandComp
  																								);
  		 DEBUG(DEB_LEV_ERR,"create output thread \n"); 
  			omx_base_component_MessageHandler(openmaxStandComp,message);
   	 }else if ((message->messageParam == OMX_StateLoaded) && (pComponentPrivate->state == OMX_StateIdle)) {
      		omx_base_component_MessageHandler(openmaxStandComp,message);
      		if(pComponentPrivate->OutPutThreadID == 0 ){
      			/*Signal Buffer Management thread to exit*/
      			if(tsem_ctor(pComponentPrivate->decodedSem)==0){
      				tsem_up(pComponentPrivate->decodedSem);
      			}
      			if(tsem_ctor(pComponentPrivate->mDoneSem)==0){
      				tsem_up(pComponentPrivate->mDoneSem);
      			}
      			
      			err=pthread_join(pComponentPrivate->OutPutThread, NULL);
      			pComponentPrivate->OutPutThreadID=-1;
      			if(err != 0){
      				DEBUG(DEB_LEV_ERR,"In %s pthread_join returned err=%d\n",__func__,err);
      			}
      		}
      		err = omx_videodec_component_Deinit(openmaxStandComp);
      		if(err!=OMX_ErrorNone) { 
        		DEBUG(DEB_LEV_FULL_SEQ,"In %s Video Decoder Deinit Failed Error=%x\n",__func__,err); 
       	 		return err;
      		} 
      		if(pComponentPrivate->fb_port != NULL) {
							fb_fifo_dispose(pComponentPrivate->fb_port);
							pComponentPrivate->fb_port=NULL;
					} 
					if(pComponentPrivate->vce_handle!=NULL){
							VceReSize_Close(pComponentPrivate->vce_handle);
							pComponentPrivate->vce_handle=NULL;
					}
   	 }else if(message->messageParam == OMX_StateInvalid){
   	 	      
   	 				omx_base_component_MessageHandler(openmaxStandComp,message);
   	 				if(pComponentPrivate->OutPutThreadID == 0 ){
      			/*Signal Buffer Management thread to exit*/
      				if(tsem_ctor(pComponentPrivate->decodedSem)==0){
      					tsem_up(pComponentPrivate->decodedSem);
      				}
      				if(tsem_ctor(pComponentPrivate->mDoneSem)==0){
      					tsem_up(pComponentPrivate->mDoneSem);
      				}
      				err=pthread_join(pComponentPrivate->OutPutThread, NULL);
      				pComponentPrivate->OutPutThreadID=-1;
      				if(err != 0){
      					DEBUG(DEB_LEV_ERR,"In %s pthread_join returned err=%d\n",__func__,err);
      				}
      		  }
      		  err = omx_videodec_component_Deinit(openmaxStandComp);
   	 	      if(err!=OMX_ErrorNone) { 
        			DEBUG(DEB_LEV_FULL_SEQ,"In %s Video Decoder Deinit Failed Error=%x\n",__func__,err); 
       	 			return err;
      			}
      			if(pComponentPrivate->fb_port != NULL) {
							fb_fifo_dispose(pComponentPrivate->fb_port);
							pComponentPrivate->fb_port=NULL;
      			} 
      			if(pComponentPrivate->vce_handle){
							VceReSize_Close(pComponentPrivate->vce_handle);
							pComponentPrivate->vce_handle=NULL;
						}
   	 }else if ((message->messageParam == OMX_StateIdle) && (eCurrentState == OMX_StateExecuting)){  	 	    
   		 	 	
   		 	 	omx_base_video_PortType *pOutPort=pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
   		 	 	raw_fifo_timeout_wakeup(pComponentPrivate->vout,&(pComponentPrivate->suspend_flag));
   		 	 	if(PORT_IS_ENABLED(pInPort)) {
   		 	 		pComponentPrivate->is_inputbuffer_flushing_pending = OMX_TRUE;
   		 	 		DEBUG(DEB_LEV_ERR,"========input flushinging start at stop==========\n");
   		 	 		pInPort->FlushProcessingBuffers(pInPort);
   		 	 		DEBUG(DEB_LEV_ERR,"========input flushinging end at stop==========\n");
   		 	 		pComponentPrivate->is_inputbuffer_flushing_pending = OMX_FALSE;
          }   		 	 	
   		 	 	if(PORT_IS_ENABLED(pOutPort)) {
   		 	 		pComponentPrivate->is_outputbuffer_flushing_pending = OMX_TRUE;
   		 	 		DEBUG(DEB_LEV_ERR,"========output flushinging start at stop==========\n");
   		 	 		pOutPort->FlushProcessingBuffers(pOutPort);

            DEBUG(DEB_LEV_ERR,"========output flushinging end at stop==========\n");
            pComponentPrivate->is_outputbuffer_flushing_pending = OMX_FALSE ;
          }
          
   		 		pComponentPrivate->state = OMX_StateIdle;
      		if (pComponentPrivate->callbacks) {
    				DEBUG(DEB_LEV_SIMPLE_SEQ, "running callback in %s\n", __func__);
    				(*(pComponentPrivate->callbacks->EventHandler))
    				(openmaxStandComp,
    				pComponentPrivate->callbackData,
    				OMX_EventCmdComplete, /* The command was completed */
    				OMX_CommandStateSet, /* The commands was a OMX_CommandStateSet */
    				message->messageParam, /* The state has been changed in message->messageParam */
    				NULL);
    			}
      		
   	 }else{
   	 	omx_base_component_MessageHandler(openmaxStandComp,message);
   	 }

  	}
  	break;
  	case OMX_CommandFlush :{
  		if(message->messageParam == OMX_ALL) {
         	
       pInPort->bIsPortFlushed = OMX_TRUE;
       pOutPort->bIsPortFlushed = OMX_TRUE;
       /*---flush inputport---*/
       pComponentPrivate->is_inputbuffer_flushing_pending = OMX_TRUE;		
       
       err = pInPort->FlushProcessingBuffers(pInPort);
       if(err == OMX_ErrorNone){
       	(*(pComponentPrivate->callbacks->EventHandler))
        	(openmaxStandComp,
        	pComponentPrivate->callbackData,
        	OMX_EventCmdComplete, /* The command was completed */
        	OMX_CommandFlush, /* The commands was a OMX_CommandStateSet */
        	OMX_BASE_FILTER_INPUTPORT_INDEX, /* The state has been changed in message->messageParam */
        	NULL);
       }else{
       	 (*(pComponentPrivate->callbacks->EventHandler))
      		(openmaxStandComp,
      		pComponentPrivate->callbackData,
      		OMX_EventError, /* The command was completed */
      		err, /* The commands was a OMX_CommandStateSet */
      		0, /* The state has been changed in message->messageParam */
      		NULL);
       }
       
       pComponentPrivate->is_inputbuffer_flushing_pending = OMX_FALSE;      
       
       /*---flush outputport---*/
       pComponentPrivate->is_outputbuffer_flushing_pending = OMX_TRUE;
       raw_fifo_timeout_wakeup(pComponentPrivate->vout,&(pComponentPrivate->suspend_flag));
       pComponentPrivate->vout->fifo_reset(pComponentPrivate->vout);
       pComponentPrivate->is_cur_stream_buf = OMX_FALSE;

       err = pOutPort->FlushProcessingBuffers(pOutPort);
       if(err == OMX_ErrorNone){
       	(*(pComponentPrivate->callbacks->EventHandler))
        	(openmaxStandComp,
        	pComponentPrivate->callbackData,
        	OMX_EventCmdComplete, /* The command was completed */
        	OMX_CommandFlush, /* The commands was a OMX_CommandStateSet */
        	OMX_BASE_FILTER_OUTPUTPORT_INDEX, /* The state has been changed in message->messageParam */
        	NULL);
       }else{
       	 (*(pComponentPrivate->callbacks->EventHandler))
      		(openmaxStandComp,
      		pComponentPrivate->callbackData,
      		OMX_EventError, /* The command was completed */
      		err, /* The commands was a OMX_CommandStateSet */
      		0, /* The state has been changed in message->messageParam */
      		NULL);
       }
         tsem_reset(pComponentPrivate->decodedSem);
  		 pComponentPrivate->is_outputbuffer_flushing_pending =OMX_FALSE;   
  	  } else if ( message->messageParam==OMX_BASE_FILTER_OUTPUTPORT_INDEX ){
  			
  			pComponentPrivate->is_outputbuffer_flushing_pending = OMX_TRUE;
  			DEBUG(DEB_LEV_ERR,"====OutPut Flush start=====\n");
  			raw_fifo_timeout_wakeup(pComponentPrivate->vout,&(pComponentPrivate->suspend_flag));
  			pComponentPrivate->vout->fifo_reset(pComponentPrivate->vout);
			pComponentPrivate->is_cur_stream_buf = OMX_FALSE;	       
     
			err =  omx_base_component_MessageHandler(openmaxStandComp,message);
			tsem_reset(pComponentPrivate->decodedSem);
  			DEBUG(DEB_LEV_ERR,"====OutPut Flush end=====\n");
  			pComponentPrivate->is_outputbuffer_flushing_pending = OMX_FALSE;
  		} else{
  			 DEBUG(DEB_LEV_ERR,"====inPut Flush start=====\n");
  			 pComponentPrivate->is_inputbuffer_flushing_pending =OMX_TRUE;
  			 			 
  			 err =  omx_base_component_MessageHandler(openmaxStandComp,message);
  			 DEBUG(DEB_LEV_ERR,"====inPut Flush end=====\n");
  			 pComponentPrivate->is_inputbuffer_flushing_pending =OMX_FALSE;
  		}
  	}
  	break;
  	default:
  		err =  omx_base_component_MessageHandler(openmaxStandComp,message);

  	break;
  	
  }
  	
  return err;
}


OMX_ERRORTYPE VIDEC_Port_FreeBuffer(
  omx_base_video_PortType *openmaxStandPort,
  OMX_U32 nPortIndex,
  OMX_BUFFERHEADERTYPE* pBuffer) {

  OMX_S32 i;
  OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)(omxComponent->pComponentPrivate);
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for port %p\n", __func__, openmaxStandPort);

  if (nPortIndex != openmaxStandPort->sPortParam.nPortIndex) {
    return OMX_ErrorBadPortIndex;
  }
  if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
    return OMX_ErrorBadPortIndex;
  }

  if (omx_base_component_Private->transientState != OMX_TransStateIdleToLoaded) {
    if (!openmaxStandPort->bIsTransientToDisabled) {
      DEBUG(DEB_LEV_FULL_SEQ, "In %s: The port is not allowed to free the buffers\n", __func__);
      (*(omx_base_component_Private->callbacks->EventHandler))
        (omxComponent,
        omx_base_component_Private->callbackData,
        OMX_EventError, /* The command was completed */
        OMX_ErrorPortUnpopulated, /* The commands was a OMX_CommandStateSet */
        nPortIndex, /* The state has been changed in message->messageParam2 */
        NULL);
    }
  }
 

  for(i=openmaxStandPort->sPortParam.nBufferCountActual - 1; i >= 0; i--)
  {
    if (openmaxStandPort->bBufferStateAllocated[i] & (BUFFER_ASSIGNED | BUFFER_ALLOCATED)) {
			
      openmaxStandPort->bIsFullOfBuffers = OMX_FALSE;
      if(openmaxStandPort->pInternalBufferStorage[i]->pAppPrivate != pBuffer->pAppPrivate){
      	continue;      	
      }
      if (openmaxStandPort->bBufferStateAllocated[i] & BUFFER_ALLOCATED) {
      	if(openmaxStandPort->pInternalBufferStorage[i]->pBuffer){
      		DEBUG(DEB_LEV_FULL_SEQ,"===freebuffer:%x,nBuffsize:%x===\n",openmaxStandPort->pInternalBufferStorage[i]->pBuffer,openmaxStandPort->pInternalBufferStorage[i]->nAllocLen);
      		if(pComponentPrivate->is_Thumbnail==OMX_TRUE  && nPortIndex==OMX_BASE_FILTER_OUTPUTPORT_INDEX){
		  			actal_free_cached_manual(openmaxStandPort->pInternalBufferStorage[i]->pBuffer);
		  		}else{
		  			if (nPortIndex== OMX_BASE_FILTER_INPUTPORT_INDEX){
						actal_free_cached_manual(openmaxStandPort->pInternalBufferStorage[i]->pBuffer - sizeof(packet_header_t));
					}else{
						actal_free_cached_manual(openmaxStandPort->pInternalBufferStorage[i]->pBuffer);
					}
		  		
		  		}
          openmaxStandPort->pInternalBufferStorage[i]->pBuffer=NULL;
          
       }
      } else if (openmaxStandPort->bBufferStateAllocated[i] & BUFFER_ASSIGNED) {
        free(pBuffer);
      }
      if(openmaxStandPort->bBufferStateAllocated[i] & HEADER_ALLOCATED) {
        free(openmaxStandPort->pInternalBufferStorage[i]);
        openmaxStandPort->pInternalBufferStorage[i]=NULL;
      }

      openmaxStandPort->bBufferStateAllocated[i] = BUFFER_FREE;

      openmaxStandPort->nNumAssignedBuffers--;
      DEBUG(DEB_LEV_PARAMS, "openmaxStandPort->nNumAssignedBuffers %i\n", (int)openmaxStandPort->nNumAssignedBuffers);

      if (openmaxStandPort->nNumAssignedBuffers == 0) {
      	DEBUG(DEB_LEV_ERR, "nNumAssignedBuffers free normal \n");
        openmaxStandPort->sPortParam.bPopulated = OMX_FALSE;
        openmaxStandPort->bIsEmptyOfBuffers = OMX_TRUE;
        tsem_up(openmaxStandPort->pAllocSem);
        if(nPortIndex==OMX_BASE_FILTER_OUTPUTPORT_INDEX)
        {
        	pComponentPrivate->bOutPutBufferAllocated = BUFFER_FREE;
        	pComponentPrivate->IsGraphicBuffer = OMX_FALSE;
        	openmaxStandPort->sPortParam.nBufferCountActual=((VIDDEC_COMPONENT_PRIVATE*)omxComponent->pComponentPrivate)->nNewBufferCountActual;
        }
      }
      DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for port %p\n", __func__, openmaxStandPort);
      return OMX_ErrorNone;
    }
  }
  DEBUG(DEB_LEV_ERR, "Out of %s for port %p with OMX_ErrorInsufficientResources\n", __func__, openmaxStandPort);
  return OMX_ErrorInsufficientResources;
}
OMX_ERRORTYPE omx_videodec_component_FreeBuffer(
            OMX_HANDLETYPE hComponent,
            OMX_U32 nPortIndex,
            OMX_BUFFERHEADERTYPE* pBuffer) {
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)omx_base_component_Private;
  omx_base_video_PortType *pPort;
  OMX_ERRORTYPE err;
  pComponentPrivate->is_fifo_disposing=OMX_TRUE;
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for component %p\n", __func__, hComponent);
  if (nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts)) {
    DEBUG(DEB_LEV_ERR, "In %s: wrong port index\n", __func__);
    return OMX_ErrorBadPortIndex;
  }

  pPort = omx_base_component_Private->ports[nPortIndex];
  err = VIDEC_Port_FreeBuffer(pPort, nPortIndex, pBuffer);
  if (err != OMX_ErrorNone) {
	  DEBUG(DEB_LEV_ERR, "Out of %s for component %p with err %i\n", __func__, hComponent, (int)err);
	  return err;
  }
  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for component %p\n", __func__, hComponent);
  return OMX_ErrorNone;
}
OMX_ERRORTYPE omx_videodec_component_ComponentRoleEnum(
  OMX_IN OMX_HANDLETYPE hComponent,
  OMX_OUT OMX_U8 *cRole,
  OMX_IN OMX_U32 nIndex) {

  	VIDDEC_COMPONENT_PRIVATE *pComponentPrivate;
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	DEBUG(DEB_LEV_FULL_SEQ,"Entry ComponentRoleEnum,Getting configuration %i\n", nIndex);
	pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE *)(((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate);
	
	memset(cRole, 0x0, OMX_MAX_STRINGNAME_SIZE);
	switch (nIndex) {
		case VIDDEC_DEFAULT_INPUT_INDEX_H263:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_H263);
			break;
	  case VIDDEC_DEFAULT_INPUT_INDEX_FLV1:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_FLV1);
			break;
		case VIDDEC_DEFAULT_INPUT_INDEX_VP6:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_VP6);
			break;
		case VIDDEC_DEFAULT_INPUT_INDEX_VP8:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_VP8);
			break;
		case VIDDEC_DEFAULT_INPUT_INDEX_AVC:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_AVC);
			break;
		case VIDDEC_DEFAULT_INPUT_INDEX_HEVC:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_HEVC);
			break;
		case VIDDEC_DEFAULT_INPUT_INDEX_HEVC_91:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_HEVC_91);
			break;
		case VIDDEC_DEFAULT_INPUT_INDEX_MPEG2:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_MPEG2);
			break;
		case VIDDEC_DEFAULT_INPUT_INDEX_MPEG4:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_MPEG4);
			break;
		case VIDDEC_DEFAULT_INPUT_INDEX_DIV3:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_DIV3);
			break;
		case VIDDEC_DEFAULT_INPUT_INDEX_VC1:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_VC1);
			break;
		case VIDDEC_DEFAULT_INPUT_INDEX_AVS:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_AVS);
			break;
		case VIDDEC_DEFAULT_INPUT_INDEX_RV:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_RV);
			break;
		case VIDDEC_DEFAULT_INPUT_INDEX_MJPEG:
			strcpy((char*)cRole, VIDDEC_COMPONENTROLES_MJPEG);
			break;
		/*todo add spark, it was not added because it is not in khronos spec, yet*/
		default:
			eError = OMX_ErrorNoMore;
			break;
	}
	
EXIT:

	if(eError != OMX_ErrorNone)
	{
		DEBUG(DEB_LEV_FULL_SEQ,"In %s EXIT Error=%x\n",__func__,eError);
	}

    return eError;
}

OMX_ERRORTYPE omx_videodec_component_SetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure) {

  OMX_ERRORTYPE err = OMX_ErrorNone;
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)openmaxStandComp->pComponentPrivate;
  if (pComponentConfigStructure == NULL) {
    return OMX_ErrorBadParameter;
  }
  DEBUG(DEB_LEV_FULL_SEQ,"   Getting configuration %i\n", nIndex);
  
  return omx_base_component_SetConfig(hComponent, nIndex, pComponentConfigStructure);
  
  return err;
}
OMX_ERRORTYPE omx_videodec_component_GetConfig(
  OMX_HANDLETYPE hComponent,
  OMX_INDEXTYPE nIndex,
  OMX_PTR pComponentConfigStructure) {

  OMX_ERRORTYPE err = OMX_ErrorNone;
  omx_base_video_PortType *outPort;

  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)openmaxStandComp->pComponentPrivate;
  if (pComponentConfigStructure == NULL) {
    return OMX_ErrorBadParameter;
  }
  switch (nIndex){
  	case OMX_IndexConfigCommonOutputCrop:
  	{
  			OMX_CONFIG_RECTTYPE *PortOutCrop=(OMX_CONFIG_RECTTYPE *)pComponentConfigStructure;
  			if(PortOutCrop->nPortIndex==VIDDEC_OUTPUT_PORT){
  				memcpy(PortOutCrop,&(pComponentPrivate->croprect),sizeof(OMX_CONFIG_RECTTYPE));  				
  			}
  			break;
  	}
  	default:
        err = omx_base_component_GetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }
   
  return err;
}

OMX_ERRORTYPE omx_videodec_component_GetExtensionIndex(
  OMX_IN  OMX_HANDLETYPE hComponent,
  OMX_IN  OMX_STRING cParameterName,
  OMX_OUT OMX_INDEXTYPE* pIndexType) {
  	

  OMX_COMPONENTTYPE* pHandle = NULL;
	VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = NULL;

	pHandle = (OMX_COMPONENTTYPE*)hComponent;
	pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)pHandle->pComponentPrivate;


	if(pComponentPrivate->state == OMX_StateInvalid){
		DEBUG(DEB_LEV_FULL_SEQ,"Get Extension Index in Invalid State\n");
		return OMX_ErrorInvalidState;
	}
	else if(!strncmp(cParameterName,"OMX.google.android.index.enableAndroidNativeBuffers", sizeof("OMX.google.android.index.enableAndroidNativeBuffers")-1)) {
		*pIndexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexEnableAndroidNativeBuffers;
		 return OMX_ErrorNone;
	}
	else if(!strncmp(cParameterName,"OMX.google.android.index.useAndroidNativeBuffer2", sizeof("OMX.google.android.index.useAndroidNativeBuffer2")-1)) {
	
    pComponentPrivate->IsGraphicBuffer = OMX_TRUE;
		*pIndexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexUseAndroidNativeBuffer2;
		return OMX_ErrorNone;
	//	return OMX_ErrorNotImplemented;
	}
	else if(!strncmp(cParameterName,"OMX.google.android.index.useAndroidNativeBuffer", sizeof("OMX.google.android.index.useAndroidNativeBuffer")-1)) {

		return OMX_ErrorNotImplemented;
//		*pIndexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexUseAndroidNativeBuffer;
//		 return OMX_ErrorNone;
	}
	else if(!strncmp(cParameterName,"OMX.google.android.index.getAndroidNativeBufferUsage", sizeof("OMX.google.android.index.getAndroidNativeBufferUsage")-1)) {
		*pIndexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexGetAndroidNativeBufferUsage;
		 return OMX_ErrorNone;
	}else if(!strncmp(cParameterName,"OMX.google.android.index.storeMetaDataInBuffers", 
		sizeof("OMX.google.android.index.storeMetaDataInBuffers")-1)){
		*pIndexType = (OMX_INDEXTYPE)OMX_IndexParameterStoreMediaData;
		return OMX_ErrorNone;
	}
	else {
		DEBUG(DEB_LEV_FULL_SEQ,"Extension: %s not implemented\n", cParameterName);
		return OMX_ErrorNotImplemented;
	}
  
  return OMX_ErrorNoMore;
}

