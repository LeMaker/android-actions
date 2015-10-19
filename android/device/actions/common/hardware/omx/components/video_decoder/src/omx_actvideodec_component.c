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
#include <omx_actvideodec_component.h>
#include <OMX_Video.h>
#include <ACT_OMX_Index.h>
#include <sys/prctl.h>
#include <cutils/properties.h>
#include <ACT_OMX_IVCommon.h>
#include "vce_resize.h"
#include "Igralloc.h"


#include "Actions_OSAL_Android.h"
/** Maximum Number of Video Component Instance*/
#define MAX_COMPONENT_VIDEODEC 4
#define ENABLE_VCERESIZE
/** Counter of Video Component Instance*/
static OMX_U32 noVideoDecInstance = 0;
static OMX_U32 VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM = 8;
static OMX_U32 VIDDEC_DEFAULT_INPUT_BUFFER_SIZE = 3*1024*1024;
static OMX_U32 VIDDEC_DEFAULT_INPUT_BUFFER_NUM = 4;
static OMX_U32 VIDDEC_DEFAULT_FRAME_BUFFER_NUM = 8;


/** The output decoded color format */
#define OUTPUT_DECODED_COLOR_FMT OMX_COLOR_FormatYUV420SemiPlanar 

#define DEFAULT_WIDTH 1920   
#define DEFAULT_HEIGHT 1080   
#define   VIDEO_WIDTH_ALIGN 			31 



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

OMX_BOOL createNodeList(Node **headNode){
	if(*headNode!=NULL){
		DEBUG(DEB_LEV_ERR,"++++NodeList has already created++++\n");
		return OMX_TRUE;
	}
	Node* head = *headNode = (Node*) actal_malloc(sizeof(Node));	
	if(NULL == head){  
		return OMX_FALSE;  
  } 
  head->nTimeStamp = -1;  
  head->pNext = NULL;
  return OMX_TRUE;
}


void destroyNodeList(Node *headNode){
	DEBUG(DEB_LEV_ERR,"destroynode list head:%p\n",headNode);
	Node *head = headNode;
	if(head==NULL){
		DEBUG(DEB_LEV_ERR,"!~~~~~~~~~~~~destroyNodeList head already null~~~~~~~~~~~\n");
		return;
	}
	if(NULL == head->pNext){
			actal_free(head);
		 	return;  
  }  
	Node* p = head->pNext;
	while(NULL != p){  
		Node* tmp = p;  
    p = p->pNext;  
    actal_free(tmp);  
  } 
  actal_free(head); 
}
OMX_BOOL queueNode(Node *head,OMX_S64 nTimeStamp){  
	if(NULL == head){  
		return OMX_FALSE;  
  }  
  Node* p = head->pNext;  
  Node* q = head;  
  while(NULL != p){  
  	q = p;  
    p = p->pNext;  
  }
  Node* node = (Node*) actal_malloc(sizeof(Node));
  if(node==NULL){  	
  	return OMX_FALSE;
  }
  node->nTimeStamp = nTimeStamp;
  q->pNext = node;  
  node->pNext = NULL;  
  return OMX_TRUE;      
}

void printfNodeList(Node *head){
	if(NULL == head){ 
		DEBUG(DEB_LEV_ERR,"+++node null++++\n"); 
		return;  
  }
  DEBUG(DEB_LEV_ERR,"+++printfNodeList head:%p++++\n",head);   
  Node* p = head->pNext;    
  while(NULL != p){
  	DEBUG(DEB_LEV_ERR,"+++node is :%p++++\n",p); 
    p = p->pNext;  
  }
}

OMX_S64 dequeueNode(Node *head,OMX_S64 nTimeStamp){  
	if(NULL == head){  
       return OMX_FALSE;  
  }
  Node*   q = head;
  Node*   p = head->pNext;
  OMX_S64 Timestampfound = -1;
  while(NULL != p){ 
  	if((p->nTimeStamp/1000)==nTimeStamp){
  		q->pNext = p->pNext;
  		Timestampfound = p->nTimeStamp;
  		actal_free(p);
  		return Timestampfound;
  	}
  	q = p;
  	p = p->pNext; 
  }
  
  return Timestampfound;
 }
 
void resetNodeList(Node *head){
	if(head==NULL){
		DEBUG(DEB_LEV_ERR,"++++NodeList has not created yet++++\n");
		return;
	}
	Node* p = head->pNext;
	while(p != NULL){
		Node* tmp = p;  
    p = p->pNext;
    actal_free(tmp);
	}
	head->nTimeStamp = -1;  
  head->pNext = NULL;
}
 



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
	OMX_S32 hde_fd =-1;
	char* d="/dev/hde";

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
		pComponentPrivate->ports = calloc(pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nPorts, sizeof(omx_base_PortType *));
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
  outPort->sPortParam.nBufferCountMin =    VIDDEC_MIN_OUTPUT_BUFFER_NUM;
	
  pComponentPrivate->bComponentFlush = OMX_FALSE;


  /** general configuration irrespective of any video formats
    * setting other parameters of omx_videodec_component_private  
    */
	pComponentPrivate->avcodecReady = OMX_FALSE;
	pComponentPrivate->isFirstBuffer = 1;
	pComponentPrivate->p_so_handle = NULL;
	pComponentPrivate->p_interface = NULL;
  pComponentPrivate->p_handle = NULL;	
	pComponentPrivate->is_error_status=OMX_FALSE;
	pComponentPrivate->is_fifo_disposing=OMX_FALSE;
	pComponentPrivate->is_videodec_fifo_init=OMX_FALSE;
	pComponentPrivate->is_end_of_stream=OMX_FALSE;
	pComponentPrivate->is_cur_stream_buf=OMX_FALSE;
	pComponentPrivate->is_Thumbnail=OMX_FALSE;
	pComponentPrivate->err_counter=0;
	pComponentPrivate->is_h264_Thumbnail_outputed=OMX_FALSE;
	pComponentPrivate->IsGraphicBuffer=OMX_FALSE;
	pComponentPrivate->IsSWcodec=OMX_FALSE;
	pComponentPrivate->is_outputbuffer_flushing_pending=0;
	pComponentPrivate->is_inputbuffer_flushing_pending=0;
	pComponentPrivate->output_portformat_inited = OMX_FALSE;
	pComponentPrivate->IsProbed = OMX_FALSE;
	pComponentPrivate->IsPortBeingDisabled =OMX_FALSE;
	pComponentPrivate->bConfigPktStored = OMX_FALSE;	
	pComponentPrivate->bthirdparty = OMX_FALSE;
	pComponentPrivate->bthirdpartychecked =  OMX_FALSE;
	pComponentPrivate->bStoreMediadata[OMX_BASE_FILTER_INPUTPORT_INDEX]=OMX_FALSE;
	pComponentPrivate->bStoreMediadata[OMX_BASE_FILTER_OUTPUTPORT_INDEX]=OMX_FALSE;
	pComponentPrivate->bResizeEnabled= OMX_FALSE;
	pComponentPrivate->bResizeRealloced = OMX_FALSE;
	pComponentPrivate->bBufferNotEngouh = OMX_FALSE;
	pComponentPrivate->last_buffer_header = NULL;
	pComponentPrivate->vce_handle = NULL;
	pComponentPrivate->suspend_flag=0;
	pComponentPrivate->nNewBufferCountActual=0;
	pComponentPrivate->IsPortBufferRealloc =OMX_FALSE;
	pComponentPrivate->bLowRam = OMX_FALSE;
	pComponentPrivate->bHdeExsit = OMX_FALSE;
	
	hde_fd = open(d, O_RDWR);
	if (hde_fd < 0){            		
		d="/dev/mali1";
    hde_fd = open(d, O_RDWR);     
  }
  
  
  if(hde_fd>=0){
  	DEBUG(DEB_LEV_ERR,"hde exist\n");
  	close(hde_fd);
  	pComponentPrivate->bHdeExsit = OMX_TRUE;
 	}
 	

  if(!strcmp(memory_property,"true")){
  	 pComponentPrivate->bLowRam = OMX_TRUE;
  	 VIDDEC_DEFAULT_INPUT_BUFFER_NUM = 2;
  	 VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM = 6;
  	 VIDDEC_DEFAULT_FRAME_BUFFER_NUM = 4;
  }
	
	pComponentPrivate->headNode = NULL;
	if(createNodeList(&pComponentPrivate->headNode)!=OMX_TRUE){
  	 return OMX_ErrorInsufficientResources;
  }
    
	pComponentPrivate->CodecConfigPktLen=0;
	pComponentPrivate->CodecConfigPkt = (OMX_U8*)actal_malloc(256*1024);
	if(pComponentPrivate->CodecConfigPkt==NULL){
	  return OMX_ErrorInsufficientResources;
  }

	pComponentPrivate->pstrbuf=(OMX_U8*)actal_malloc(VIDDEC_DEFAULT_INPUT_BUFFER_SIZE);
	if(pComponentPrivate->pstrbuf==NULL){
	  return OMX_ErrorInsufficientResources;
	}
	pthread_mutex_init(&pComponentPrivate->port_freebuffer_mutex, NULL);
	

  
  pComponentPrivate->PortDisableSem=(tsem_t*)calloc(1,sizeof(tsem_t));
  if(pComponentPrivate->PortDisableSem==NULL){
  	return OMX_ErrorInsufficientResources;
  }
  tsem_init(pComponentPrivate->PortDisableSem, 0);
  
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
  memset(&(pComponentPrivate->croprect),0,sizeof(OMX_CONFIG_RECTTYPE));

  /** initializing the codec context etc that was done earlier by ffmpeglibinit function */
 	pComponentPrivate->messageHandler = omx_videodec_component_MessageHandler;
	pComponentPrivate->BufferMgmtFunction =omx_videodec_component_BufferMgmtFunction;
	pComponentPrivate->BufferMgmtCallback = omx_videodec_component_BufferMgmtCallback;


	pComponentPrivate->destructor = omx_videodec_component_Destructor;
	pHandle->SetParameter = omx_videodec_component_SetParameter;
	pHandle->GetParameter = omx_videodec_component_GetParameter;
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
DEBUG(DEB_LEV_SIMPLE_SEQ,"omx_videodec_component_ComponentDeInit 0\n");	


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
	if(pComponentPrivate->PortDisableSem){
		tsem_deinit(pComponentPrivate->PortDisableSem);
		free(pComponentPrivate->PortDisableSem);
		pComponentPrivate->PortDisableSem=NULL;
	}

	
	if(pComponentPrivate->vce_handle!=NULL){
		VceReSize_Close(pComponentPrivate->vce_handle);
		pComponentPrivate->vce_handle=NULL;
	}
	if(pComponentPrivate->pstrbuf){
		actal_free(pComponentPrivate->pstrbuf);
		pComponentPrivate->pstrbuf=NULL;
	}
	if(pComponentPrivate->CodecConfigPkt!=NULL){
		actal_free(pComponentPrivate->CodecConfigPkt);
		pComponentPrivate->CodecConfigPkt=NULL;
	}
	destroyNodeList(pComponentPrivate->headNode);
	pComponentPrivate->headNode = NULL;
	omx_videodec_component_Deinit(openmaxStandComp); 
	pthread_mutex_destroy(&pComponentPrivate->port_freebuffer_mutex);
	if(pComponentPrivate->fb_port != NULL) {
		fb_fifo_dispose(pComponentPrivate->fb_port,pComponentPrivate->IsGraphicBuffer);
		pComponentPrivate->fb_port=NULL;
		pComponentPrivate->is_videodec_fifo_init = OMX_FALSE;
	}
DEBUG(DEB_LEV_SIMPLE_SEQ,"omx_videodec_component_ComponentDeInit 2\n");	
	/* frees port/s */   
	if (pComponentPrivate->ports) {
DEBUG(DEB_LEV_SIMPLE_SEQ,"----------a nPorts %d\n",pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nPorts);		   
	for (i=0; i < pComponentPrivate->sPortTypesParam[OMX_PortDomainVideo].nPorts; i++) {   
DEBUG(DEB_LEV_SIMPLE_SEQ,"----------b pComponentPrivate->ports[i] %x \n",pComponentPrivate->ports[i]);		   
		if(pComponentPrivate->ports[i])   
		{
DEBUG(DEB_LEV_SIMPLE_SEQ,"----------c\n");		   
            tsem_up(pComponentPrivate->ports[i]->pAllocSem);				   
	    	pComponentPrivate->ports[i]->PortDestructor(pComponentPrivate->ports[i]);   
DEBUG(DEB_LEV_SIMPLE_SEQ,"----------d\n");		   	    
		}
	}   
DEBUG(DEB_LEV_SIMPLE_SEQ,"----------e\n");	
	free(pComponentPrivate->ports);   
DEBUG(DEB_LEV_SIMPLE_SEQ,"----------f\n");		   
	pComponentPrivate->ports=NULL;   
	} 
DEBUG(DEB_LEV_SIMPLE_SEQ,"omx_videodec_component_ComponentDeInit 3\n");	
	
	DEBUG(DEB_LEV_SIMPLE_SEQ,"Destructor of video decoder component is called\n");
	
//	omx_base_filter_Destructor(pHandle);
	noVideoDecInstance--;
DEBUG(DEB_LEV_SIMPLE_SEQ,"omx_videodec_component_ComponentDeInit 4\n");	
	return OMX_ErrorNone;
}


typedef void*(*FuncPtr)(void);
/** It initializates the FFmpeg framework, and opens an FFmpeg videodecoder of type specified by IL client 
  */ 
OMX_ERRORTYPE omx_videodec_component_actvideoInit(VIDDEC_COMPONENT_PRIVATE *pComponentPrivate) 
{
  	FuncPtr func_handle;
	char libname[24] = "vd_";
	int i;
	OMX_BOOL ish264=OMX_FALSE;
	omx_base_video_PortType *inPort,*outPort;
	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
	OMX_VIDEO_PARAM_PORTFORMATTYPE* pPortFormat;
	vde_video_metadata_t *MetaData=NULL;
	OMX_BOOL IsMpeg4Codec=OMX_FALSE;
	OMX_ERRORTYPE err = OMX_ErrorNone;
	inPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	outPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	pPortFormat = &(outPort->sVideoParam);
	pPortDef = &(outPort->sPortParam);
	pComponentPrivate->framebuffernum = VIDDEC_DEFAULT_FRAME_BUFFER_NUM;
  pComponentPrivate->videobuffersize = ((pPortDef->format.video.nStride+31)&(~31)) * ((pPortDef->format.video.nFrameHeight+15)&(~15)) * VIDDEC_FACTORFORMAT420;


	switch(inPort->sVideoParam.eCompressionFormat)
	{
		case OMX_VIDEO_CodingRVG2:
			DEBUG(DEB_LEV_ERR,"------------is rvg2 here\n");
			pComponentPrivate->IsSWcodec = OMX_TRUE;
			strcat(libname, "rvg2.so");
			break;
		case OMX_VIDEO_CodingWMV2:
			DEBUG(DEB_LEV_ERR,"------------is wmv2 here\n");
			pComponentPrivate->IsSWcodec = OMX_TRUE;
			strcat(libname, "wmv2.so");
			break;
	case OMX_VIDEO_CodingVP9:
	 		DEBUG(DEB_LEV_ERR,"------------vp9 here\n");
			pComponentPrivate->IsSWcodec = OMX_TRUE;
			strcat(libname, "vp9.so");
			break;				
	 case OMX_VIDEO_CodingHEVC:
			
			if(pComponentPrivate->bHdeExsit==OMX_FALSE){
				pComponentPrivate->IsSWcodec = OMX_TRUE;
				strcat(libname, "hevc.so");
				DEBUG(DEB_LEV_ERR,"------------hevc here\n");
			}else{
				strcat(libname, "h265.so");
				DEBUG(DEB_LEV_ERR,"------------h265 here\n");
			}
			
			break;
	 case OMX_VIDEO_CodingHEVC_91:
			DEBUG(DEB_LEV_ERR,"------------hevc_91 here\n");
			pComponentPrivate->IsSWcodec = OMX_TRUE;
			strcat(libname, "hevc_91.so");
			break;
	 case OMX_VIDEO_CodingMPEG2:
	 		DEBUG(DEB_LEV_ERR,"------------mpeg2 here\n");
			strcat(libname, "mpeg.so");
			break;
	 case OMX_VIDEO_CodingMPEG4:
	 		DEBUG(DEB_LEV_ERR,"------------mpeg4 here\n");
			strcat(libname, "xvid.so");
			IsMpeg4Codec = OMX_TRUE;
			break;
	 case OMX_VIDEO_CodingAVC:
	 		DEBUG(DEB_LEV_ERR,"------------h264 here\n");
	 		ish264=OMX_TRUE;
			strcat(libname, "h264.so");
			pComponentPrivate->videobuffersize += ((pPortDef->format.video.nStride+31)&(~31)) * ((pPortDef->format.video.nFrameHeight+15)&(~15))/4;
			break;
	 case OMX_VIDEO_CodingH263:
	 		DEBUG(DEB_LEV_ERR,"------------h263 here\n");
			strcat(libname, "h263.so");
			break;
	case OMX_VIDEO_CodingFLV1:
	 		DEBUG(DEB_LEV_ERR,"------------flv1 here\n");
			strcat(libname, "flv1.so");
			break;
	case OMX_VIDEO_CodingDIV3:
	 		DEBUG(DEB_LEV_ERR,"------------msm4 here\n");
			strcat(libname, "msm4.so");
			break;
	case OMX_VIDEO_CodingVP6:
	 		DEBUG(DEB_LEV_ERR,"------------vp6 here\n");
			strcat(libname, "vp6.so");
			break;
	case OMX_VIDEO_CodingVP8:
	 		DEBUG(DEB_LEV_ERR,"------------vp8 here\n");
			strcat(libname, "vp8.so");
			break;
	case OMX_VIDEO_CodingRV:
	 		DEBUG(DEB_LEV_ERR,"------------rv34 here\n");
			strcat(libname, "rv34.so");
			break;
		case OMX_VIDEO_CodingVC1:
	 		DEBUG(DEB_LEV_ERR,"------------vc1 here\n");
			strcat(libname, "vc1.so");
			break;
	case OMX_VIDEO_CodingMJPEG:
			DEBUG(DEB_LEV_ERR,"------------mjpg here\n");
			strcat(libname, "mjpg.so");
			break;
    case OMX_VIDEO_CodingAVS:
			DEBUG(DEB_LEV_ERR,"------------is avs.. here\n");
			pComponentPrivate->IsSWcodec = OMX_TRUE;
			strcat(libname, "avs.so");
			break;
	default:
			DEBUG(DEB_LEV_ERR,"In %s encounted unrecognized format %d\n",__func__,inPort->sVideoParam.eCompressionFormat);	
			err = OMX_ErrorComponentNotFound;
			goto LOADCODEC_EXIT;	
	}

	pComponentPrivate->p_so_handle = dlopen(libname, RTLD_NOW);
    if (pComponentPrivate->p_so_handle == NULL)
    {
    	DEBUG(DEB_LEV_ERR,"In %s dlopen Error\n",__func__);
        err = OMX_ErrorComponentNotFound;
		goto LOADCODEC_EXIT;
    }

    func_handle = (FuncPtr)dlsym( pComponentPrivate->p_so_handle , "get_plugin_info");
    if(func_handle == NULL) 
    {
    	DEBUG(DEB_LEV_ERR,"In %s dlsym Error\n",__func__);
        dlclose( pComponentPrivate->p_so_handle);
		pComponentPrivate->p_so_handle = NULL;
		err = OMX_ErrorDynamicResourcesUnavailable;
		goto LOADCODEC_EXIT;
    }

    pComponentPrivate->p_interface = (videodec_plugin_t*)func_handle();
    if(pComponentPrivate->p_interface == NULL) {
    	 DEBUG(DEB_LEV_ERR,"In %s get func_handle Error\n",__func__);
        dlclose(pComponentPrivate->p_so_handle);
        pComponentPrivate->p_so_handle = NULL;
        err = OMX_ErrorDynamicResourcesUnavailable;
				goto LOADCODEC_EXIT;
    }
    if(pComponentPrivate->bResizeEnabled==OMX_FALSE){
    	 if(pComponentPrivate->is_videodec_fifo_init==OMX_FALSE){
    		pComponentPrivate->is_videodec_fifo_init=OMX_TRUE;
    		pComponentPrivate->fb_port = fb_fifo_open((void **)(&(pComponentPrivate->vout)));
				for(i = 0;i<outPort->sPortParam.nBufferCountActual;i++) {
					OMX_BUFFERHEADERTYPE *pBuffHead = outPort->pInternalBufferStorage[i];
					if(pComponentPrivate->bStoreMediadata[OMX_BASE_FILTER_OUTPUTPORT_INDEX]==OMX_TRUE){
						MetaData = (vde_video_metadata_t*)pBuffHead->pBuffer;
						raw_fifo_init_useBuffer(pComponentPrivate->vout, MetaData->handle, \
						outPort->sPortParam.nBufferCountActual,pBuffHead->nAllocLen,pBuffHead->pAppPrivate,OMX_TRUE,pComponentPrivate->IsSWcodec);
					}else{
						raw_fifo_init_useBuffer(pComponentPrivate->vout, pBuffHead->pBuffer, \
						outPort->sPortParam.nBufferCountActual,pBuffHead->nAllocLen,pBuffHead->pAppPrivate,pComponentPrivate->IsGraphicBuffer,pComponentPrivate->IsSWcodec);
					}
				}
			
			 }
    }
   
	
    if(inPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingHEVC || inPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingHEVC_91 || inPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingAVS){
    	if(pComponentPrivate->bHdeExsit==OMX_FALSE){
    		int ap_param;
				ap_param = pComponentPrivate->is_Thumbnail ? 1:0;   
				pComponentPrivate->p_handle = pComponentPrivate->p_interface->open(&ap_param, pComponentPrivate->CodecConfigPktLen>0 ? pComponentPrivate->CodecConfigPkt:NULL,pComponentPrivate->fb_port);
    	}else{
    		if(inPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingHEVC){
    			pComponentPrivate->p_handle = pComponentPrivate->p_interface->open(NULL,pComponentPrivate->CodecConfigPktLen>0 ? pComponentPrivate->CodecConfigPkt:NULL, pComponentPrivate->fb_port);
    		}else{
    			int ap_param;
					ap_param = pComponentPrivate->is_Thumbnail ? 1:0;   
					pComponentPrivate->p_handle = pComponentPrivate->p_interface->open(&ap_param, pComponentPrivate->CodecConfigPktLen>0 ? pComponentPrivate->CodecConfigPkt:NULL,pComponentPrivate->fb_port);
    		}
    	}
			
		}else{
			if(IsMpeg4Codec==OMX_TRUE){
				unsigned int init_param[5];
				OMX_CONFIG_RECTTYPE *PortOutCrop= &(pComponentPrivate->croprect);
				memset(init_param,0,20);
				init_param[2]= PortOutCrop->nWidth;
				init_param[3]= PortOutCrop->nHeight;
				DEBUG(DEB_LEV_ERR,"width:%d,height:%d\n",init_param[2],init_param[3]);
				pComponentPrivate->p_handle = pComponentPrivate->p_interface->open(NULL, init_param , pComponentPrivate->fb_port);				
			}else{
				pComponentPrivate->p_handle = pComponentPrivate->p_interface->open(NULL,pComponentPrivate->CodecConfigPktLen>0 ? pComponentPrivate->CodecConfigPkt:NULL, pComponentPrivate->fb_port);
			}
			
		}

	if(pComponentPrivate->p_handle == NULL) {
		dlclose(pComponentPrivate->p_so_handle);
	    pComponentPrivate->p_so_handle = NULL;
	    pComponentPrivate->is_error_status=OMX_TRUE;
	    err = OMX_ErrorDynamicResourcesUnavailable;		
	    goto LOADCODEC_EXIT; 
	}	

	if((pPortFormat->xFramerate>>16)>30 && (ish264==OMX_TRUE || IsMpeg4Codec==OMX_TRUE) && pComponentPrivate->is_Thumbnail==OMX_FALSE && pPortDef->format.video.nStride* ((pPortDef->format.video.nFrameHeight+15)&(~15))>=1280*720 ){
		DEBUG(DEB_LEV_ERR,"xFramerate is %d \n",(pPortFormat->xFramerate>>16));
		pComponentPrivate->p_interface->ex_ops(pComponentPrivate->p_handle,DISCARD_FRAMES,1);
	}
	
	if(pComponentPrivate->bHdeExsit && pComponentPrivate->is_Thumbnail==OMX_FALSE){
		OMX_BOOL pp_enabled = OMX_FALSE;
   	char pp[PROPERTY_VALUE_MAX] = "false";
  	property_get("ro.sf.pp_enable",pp,"false");
  	if(!strcasecmp(pp, "true")){
  			pp_enabled = OMX_TRUE;
  	}
  	if(pp_enabled){
  		if(inPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingAVC && pPortDef->format.video.nFrameWidth * pPortDef->format.video.nFrameHeight>1920*1088){
				DEBUG(DEB_LEV_ERR,"enable avc pp\n");
				pComponentPrivate->p_interface->ex_ops(pComponentPrivate->p_handle,EX_RESERVED1,1);
				
			}

			if(inPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingHEVC && pPortDef->format.video.nFrameWidth * pPortDef->format.video.nFrameHeight>1920*1088){
				DEBUG(DEB_LEV_ERR,"enable hevc pp\n");
				pComponentPrivate->p_interface->ex_ops(pComponentPrivate->p_handle,EX_RESERVED1,1);
				
		  }

  	}
		
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
		DEBUG(DEB_LEV_ERR,"===fb disposeing,line %d===\n",__LINE__);
		fb_fifo_dispose(pComponentPrivate->fb_port,pComponentPrivate->IsGraphicBuffer);
		pComponentPrivate->fb_port=NULL;
		pComponentPrivate->is_videodec_fifo_init = OMX_FALSE;
	}

}
OMX_ERRORTYPE omx_videodec_component_videodecfifoInit(VIDDEC_COMPONENT_PRIVATE *pComponentPrivate)
{
	int i;
	omx_base_video_PortType *outPort;
	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef;
	pComponentPrivate->is_fifo_disposing=OMX_FALSE;
	pComponentPrivate->nNumAssignedBufferHeads=0;
	vde_video_metadata_t *MetaData=NULL;
	if(pComponentPrivate->fb_port==NULL){
		DEBUG(DEB_LEV_ERR,"===fb_port open===\n");
		pComponentPrivate->fb_port = fb_fifo_open((void **)(&(pComponentPrivate->vout)));
		if(pComponentPrivate->fb_port==NULL){
			return OMX_ErrorInsufficientResources;			
		}
	}

	outPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	pPortDef = &(outPort->sPortParam);
#ifdef ENABLE_VCERESIZE
	if(((pPortDef->format.video.nFrameWidth==176 && pPortDef->format.video.nFrameHeight==144) || (pPortDef->format.video.nFrameWidth==720 && pPortDef->format.video.nFrameHeight==480))&& pComponentPrivate->is_Thumbnail==OMX_FALSE && pComponentPrivate->IsSWcodec==OMX_FALSE){
		VR_Input_t vr_input_param;
		pComponentPrivate->bResizeEnabled=OMX_TRUE;
		memset(&vr_input_param,0,sizeof(VR_Input_t));
		vr_input_param.dstw_align = ALIGN_16PIXELS;
        pComponentPrivate->vce_handle = VceReSize_Open(&vr_input_param);		
	}
#endif

	if(pComponentPrivate->bResizeEnabled==OMX_FALSE){
		pComponentPrivate->is_videodec_fifo_init = OMX_TRUE;
		for(i = 0;i<outPort->sPortParam.nBufferCountActual;i++) {		  
			OMX_BUFFERHEADERTYPE *pBuffHead = outPort->pInternalBufferStorage[i];
			if(pComponentPrivate->bStoreMediadata[OMX_BASE_FILTER_OUTPUTPORT_INDEX]==OMX_TRUE){
				MetaData = (vde_video_metadata_t*)pBuffHead->pBuffer;
				raw_fifo_init_useBuffer(pComponentPrivate->vout, MetaData->handle, \
				outPort->sPortParam.nBufferCountActual,pBuffHead->nAllocLen,pBuffHead->pAppPrivate,OMX_TRUE,pComponentPrivate->IsSWcodec);
			}else{
				raw_fifo_init_useBuffer(pComponentPrivate->vout, pBuffHead->pBuffer, \
				outPort->sPortParam.nBufferCountActual,pBuffHead->nAllocLen,pBuffHead->pAppPrivate,pComponentPrivate->IsGraphicBuffer,pComponentPrivate->IsSWcodec);
			}			
		}
		pComponentPrivate->vout->fifo_reset(pComponentPrivate->vout);
	}
	
	
	pComponentPrivate->IsPortBufferRealloc=OMX_FALSE;
	return OMX_ErrorNone;
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
	omx_base_video_PortType *inPort,*outPort;	
	int i;
	pComponentPrivate->isFirstBuffer=1;
	pComponentPrivate->bConfigPktStored = OMX_FALSE;
	pComponentPrivate->IsProbed = OMX_FALSE;
	inPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	outPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	inPort->bIsDestroying = OMX_FALSE;
	outPort->bIsDestroying = OMX_FALSE;
	return eError;
}

/** The Deinitialization function of the video decoder  
  */
OMX_ERRORTYPE omx_videodec_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp) {

  VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = openmaxStandComp->pComponentPrivate;
  OMX_ERRORTYPE eError = OMX_ErrorNone;  
  pthread_mutex_lock(&pComponentPrivate->port_freebuffer_mutex);
  if (pComponentPrivate->avcodecReady) {
    omx_videodec_component_actvideoDeInit(pComponentPrivate);
    pComponentPrivate->avcodecReady = OMX_FALSE;
  }
  pthread_mutex_unlock(&pComponentPrivate->port_freebuffer_mutex);

  return eError;
} 

void* omx_videodec_component_BufferMgmtFunction (void* param) {
  OMX_COMPONENTTYPE* openmaxStandComp = (OMX_COMPONENTTYPE*)param;
  omx_base_filter_PrivateType* omx_base_filter_Private = (omx_base_filter_PrivateType*)openmaxStandComp->pComponentPrivate;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate=(VIDDEC_COMPONENT_PRIVATE*)openmaxStandComp->pComponentPrivate;
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
  int nFilledBuffDone=0;
  int has_output=0;
  frame_buf_handle *raw_buf_lcd = NULL;

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
    while( PORT_IS_BEING_FLUSHED(pInPort) ||
           PORT_IS_BEING_FLUSHED(pOutPort)) {
      pthread_mutex_unlock(&omx_base_filter_Private->flush_mutex);

      DEBUG(DEB_LEV_FULL_SEQ, "In %s 1 signaling flush all cond iE=%d,iF=%d,oE=%d,oF=%d iSemVal=%d,oSemval=%d\n",
        __func__,inBufExchanged,isInputBufferNeeded,outBufExchanged,isOutputBufferNeeded,pInputSem->semval,pOutputSem->semval);

      if(isOutputBufferNeeded==OMX_FALSE && PORT_IS_BEING_FLUSHED(pOutPort) && pOutputBuffer!=NULL) {
      	if(pComponentPrivate->bResizeEnabled==OMX_TRUE){
      		pOutputBuffer->nFlags |=OMX_BUFFERFLAG_DECODEONLY;
      		pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);
      	}else{
      		if(pComponentPrivate->vout->is_fifo_bequeued(pComponentPrivate->vout,pOutputBuffer->pAppPrivate)==1){
      			pOutputBuffer->nFlags |=OMX_BUFFERFLAG_DECODEONLY;
      			pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);
      		}
      	}
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
      if(!strcasecmp((char*)(pComponentPrivate->componentRole.cRole),VIDDEC_COMPONENTROLES_VP6)){
      	if(pComponentPrivate->last_buffer_header!=NULL && PORT_IS_BEING_FLUSHED(pInPort)){
      		pInPort->ReturnBufferFunction(pInPort,pComponentPrivate->last_buffer_header);
      		pComponentPrivate->last_buffer_header = NULL;
      		inBufExchanged--;
      		isInputBufferNeeded=OMX_TRUE;
      		DEBUG(DEB_LEV_FULL_SEQ, "Ports are flushing,so returning input buffer\n");
      	}
      }
     

      DEBUG(DEB_LEV_FULL_SEQ, "In %s 2 signaling flush all cond iE=%d,iF=%d,oE=%d,oF=%d iSemVal=%d,oSemval=%d\n",
        __func__,inBufExchanged,isInputBufferNeeded,outBufExchanged,isOutputBufferNeeded,pInputSem->semval,pOutputSem->semval);

      tsem_up(omx_base_filter_Private->flush_all_condition);
      tsem_down(omx_base_filter_Private->flush_condition);
      pthread_mutex_lock(&omx_base_filter_Private->flush_mutex);
    }
    pthread_mutex_unlock(&omx_base_filter_Private->flush_mutex);

    /*No buffer to process. So wait here*/
    if((isInputBufferNeeded==OMX_TRUE && pInputSem->semval==0) &&
      (omx_base_filter_Private->state != OMX_StateLoaded && omx_base_filter_Private->state != OMX_StateInvalid)) {
      //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
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
      //Signaled from EmptyThisBuffer or FillThisBuffer or some thing else
      DEBUG(DEB_LEV_FULL_SEQ, "Waiting for next input/output buffer\n");
      tsem_down(omx_base_filter_Private->bMgmtSem);

    }
    if(omx_base_filter_Private->state == OMX_StateLoaded || omx_base_filter_Private->state == OMX_StateInvalid) {
      DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Buffer Management Thread is exiting\n",__func__);
      break;
    }

    DEBUG(DEB_LEV_FULL_SEQ, "Waiting for input buffer semval=%d in %s\n",pInputSem->semval, __func__);
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
        if(!(pInputBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)){
        	queueNode(pComponentPrivate->headNode,pInputBuffer->nTimeStamp);
        }        
        if((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS && pInputBuffer->nFilledLen==0 && pOutputSem->semval==0 ){
        	if(pComponentPrivate->bResizeEnabled==OMX_FALSE){
        		isOutputBufferNeeded=OMX_FALSE;
        	}else{
        		(*(omx_base_filter_Private->callbacks->EventHandler))
          		(openmaxStandComp,
          		omx_base_filter_Private->callbackData,
          		OMX_EventBufferFlag, /* The command was completed */
          		1, /* The commands was a OMX_CommandStateSet */
          		pInputBuffer->nFlags, /* The state has been changed in message->messageParam2 */
          		NULL);
        	}
         	
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

 //     pOutputBuffer->nTimeStamp = pInputBuffer->nTimeStamp;
      if((pInputBuffer->nFlags & OMX_BUFFERFLAG_STARTTIME) == OMX_BUFFERFLAG_STARTTIME) {
         DEBUG(DEB_LEV_FULL_SEQ, "Detected  START TIME flag in the input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
         if(pOutputBuffer!=NULL){
      		 pOutputBuffer->nFlags = pInputBuffer->nFlags;
       	 }
         pInputBuffer->nFlags = 0;
      }

      if(omx_base_filter_Private->state == OMX_StateExecuting)  {
        if (omx_base_filter_Private->BufferMgmtCallback && pInputBuffer->nFilledLen > 0) {
          (*(omx_base_filter_Private->BufferMgmtCallback))(openmaxStandComp, pInputBuffer, pOutputBuffer);
        } else{
          /*It no buffer management call back the explicitly consume input buffer*/
          pInputBuffer->nFilledLen = 0;
        }
      } else if(!(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
        DEBUG(DEB_LEV_ERR, "In %s Received Buffer in non-Executing State(%x)\n", __func__, (int)omx_base_filter_Private->state);
      } else {
          pInputBuffer->nFilledLen = 0;
      }
  
      if((pInputBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS && pInputBuffer->nFilledLen==0) {
        DEBUG(DEB_LEV_ERR,"Detected EOS flags in input buffer filled len=%d\n", (int)pInputBuffer->nFilledLen);
        
        omx_base_video_PortType *pInVideoPort=(omx_base_video_PortType *)pInPort;
        if((pInVideoPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC || pInVideoPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC || pInVideoPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC_91 || pInVideoPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingMPEG4 || pInVideoPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingMPEG2) && pComponentPrivate->p_handle!=NULL)
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
        DEBUG(DEB_LEV_ERR,"wait for statepause\n");
        tsem_wait(omx_base_filter_Private->bStateSem);
      }
      
      /*----we relocate the pOutputBuffer before send to client-----*/
      if(omx_base_filter_Private->state == OMX_StateExecuting && pComponentPrivate->is_outputbuffer_flushing_pending==0 && pComponentPrivate->is_inputbuffer_flushing_pending==0 && pComponentPrivate->is_fifo_disposing==OMX_FALSE && pComponentPrivate->IsPortBufferRealloc==OMX_FALSE){
      	if(pComponentPrivate->is_Thumbnail==OMX_TRUE && pComponentPrivate->is_h264_Thumbnail_outputed==OMX_TRUE){
      		has_output=0;
      	}else{
      		if(pComponentPrivate->bResizeEnabled==OMX_FALSE){
      			has_output=omx_videodec_output_relocation(openmaxStandComp,&pOutputBuffer);
      		}else{
      			has_output = 0;
      			if(pComponentPrivate->is_videodec_fifo_init==OMX_TRUE){
      				raw_buf_lcd = pComponentPrivate->vout->get_rbuf_postprocess(pComponentPrivate->vout);
      				if(raw_buf_lcd!=NULL){
      					omx_videodec_getoutput(openmaxStandComp,raw_buf_lcd,pOutputBuffer);
      					has_output = 1;
      				}
      			}
      			
      		}      		
      	}
    	}else{
    		has_output=0;
    	}
      
      if(has_output==1 && pOutputBuffer->nFilledLen != 0)
      {  
      	if(omx_base_filter_Private->bIsEOSReached==OMX_TRUE){
      		if(pComponentPrivate->bResizeEnabled==OMX_FALSE && pComponentPrivate->vout->get_rbuf_num(pComponentPrivate->vout)==0){
      			pOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
      			pComponentPrivate->is_end_of_stream = OMX_TRUE;
      			DEBUG(DEB_LEV_ERR,"signal EOS %d \n",__LINE__);  			    				
      		}
      		if(pComponentPrivate->bResizeEnabled==OMX_TRUE && pComponentPrivate->vout->get_rbuf_num_postprocess(pComponentPrivate->vout)==0){
      			pOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
      			pComponentPrivate->is_end_of_stream = OMX_TRUE;
      			DEBUG(DEB_LEV_ERR,"signal EOS %d \n",__LINE__);
      		}      	
      	}   
      	pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);         
        outBufExchanged--;
        pOutputBuffer=NULL;    
        isOutputBufferNeeded=OMX_TRUE;
      }
      
    } 
    
    /*If EOS ,output all the buffers*/
    if(pComponentPrivate->bResizeEnabled==OMX_FALSE  \
    	&&omx_base_filter_Private->bIsEOSReached == OMX_TRUE \
    	&& pComponentPrivate->is_end_of_stream ==OMX_FALSE){
    		 
      	int num=pComponentPrivate->vout->get_rbuf_num(pComponentPrivate->vout);
      	int loop_counter=0;
      	int counter = 0;
      	pComponentPrivate->is_end_of_stream = OMX_TRUE;       	
      	DEBUG(DEB_LEV_ERR,"======endof stream ,remaining frames is %d===\n",num);       	
      	if(pComponentPrivate->is_fifo_disposing==OMX_FALSE && num==0){
      		  frame_buf_handle *raw_buf_dec = NULL;
      			try_get_wbuf_timeout(&raw_buf_dec, pComponentPrivate->vout, &(pComponentPrivate->suspend_flag),50,0); 
      			if(raw_buf_dec!=NULL){
      				raw_buf_dec->vo_frame_info->display_flag=1;
   						raw_buf_dec->vo_frame_info->time_stamp = 0;
   						num = pComponentPrivate->vout->get_rbuf_num(pComponentPrivate->vout);
   						if(num!=1){
   							DEBUG(DEB_LEV_ERR,"something maybe unnormal\n");
   						}
   						if(omx_videodec_output_relocation(openmaxStandComp,&pOutputBuffer)==1){    				
   							pOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
      	    		pOutputBuffer->nFilledLen = 0;  			
      	    		DEBUG(DEB_LEV_ERR,"signal EOS %d \n",__LINE__); 				
      					pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer); 
      					num = 0;   			  
      				}
      			} 
      	}
      	
      	while(loop_counter<num && counter<pOutPort->sPortParam.nBufferCountActual){
      		counter++;
      		if(omx_videodec_output_relocation(openmaxStandComp,&pOutputBuffer)==1){
      			loop_counter++;
      			if(loop_counter==num){
      				DEBUG(DEB_LEV_ERR,"signal EOS %d \n",__LINE__);
      				pOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
      			}      				
      			pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);
      		}
      	}
      	
      	outBufExchanged--;
      	pOutputBuffer=NULL;
      	isOutputBufferNeeded=OMX_TRUE;
    }
    if(pComponentPrivate->bResizeEnabled==OMX_TRUE \
    	&& omx_base_filter_Private->bIsEOSReached==OMX_TRUE \
    	&& pComponentPrivate->is_end_of_stream==OMX_FALSE\
    	&& isOutputBufferNeeded==OMX_FALSE \
    	&& omx_base_filter_Private->state == OMX_StateExecuting \
    	&& pComponentPrivate->is_videodec_fifo_init==OMX_TRUE){
    	 
    	raw_buf_lcd = pComponentPrivate->vout->get_rbuf_postprocess(pComponentPrivate->vout);
      if(raw_buf_lcd!=NULL){
      	omx_videodec_getoutput(openmaxStandComp,raw_buf_lcd,pOutputBuffer);
      	if(pComponentPrivate->vout->get_rbuf_num_postprocess(pComponentPrivate->vout)==0){
      		pComponentPrivate->is_end_of_stream = OMX_TRUE;
      		pOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
      		DEBUG(DEB_LEV_ERR,"signal EOS %d \n",__LINE__);
      	}
      }else{
      	pOutputBuffer->nFilledLen = 0;
      	pComponentPrivate->is_end_of_stream = OMX_TRUE;
      	pOutputBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
      	DEBUG(DEB_LEV_ERR,"signal EOS %d \n",__LINE__);      	
      }
    
      pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);         
      outBufExchanged--;
      pOutputBuffer=NULL;    
      isOutputBufferNeeded=OMX_TRUE;
      
    }

    if(omx_base_filter_Private->state==OMX_StatePause && !(PORT_IS_BEING_FLUSHED(pInPort) || PORT_IS_BEING_FLUSHED(pOutPort))) {
      /*Waiting at paused state*/
      tsem_wait(omx_base_filter_Private->bStateSem);
    }
    
    if(pComponentPrivate->bResizeEnabled==OMX_TRUE && pComponentPrivate->IsPortBufferRealloc==OMX_TRUE){
    	isOutputBufferNeeded = OMX_TRUE;
    }
    /*Input Buffer has been completely consumed. So, return input buffer*/
    if(isInputBufferNeeded == OMX_FALSE && pInputBuffer->nFilledLen==0 ) {
    	if(!strcasecmp((char*)(pComponentPrivate->componentRole.cRole),VIDDEC_COMPONENTROLES_VP6)){ 
    		if(pComponentPrivate->last_buffer_header!=NULL){
    			pInPort->ReturnBufferFunction(pInPort,pComponentPrivate->last_buffer_header);
    		}
    		pComponentPrivate->last_buffer_header = pInputBuffer;
    	}else{
    		pInPort->ReturnBufferFunction(pInPort,pInputBuffer);
    	} 
		  
      inBufExchanged--;
      pInputBuffer=NULL;
      isInputBufferNeeded=OMX_TRUE;
    }
  }
  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s of component %p\n", __func__, openmaxStandComp);
  return NULL;
}
/*-----for h264 Thumbnail-----*/
void omx_videodec_Thumbnail_h264(OMX_COMPONENTTYPE* openmaxStandComp,frame_buf_handle *decoded_frame)
{
	VIDDEC_COMPONENT_PRIVATE *p = (VIDDEC_COMPONENT_PRIVATE *)openmaxStandComp->pComponentPrivate; 
	omx_base_video_PortType *pOutPort=(omx_base_video_PortType *)p->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];	
	int i;
	OMX_S64 timestamp;
	OMX_BUFFERHEADERTYPE *pBuffHead;
	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = &(pOutPort->sPortParam);
	for(i = 0;i<pOutPort->sPortParam.nBufferCountActual;i++) {
			pBuffHead=p->BufferHeadStorage[i];
			if(pBuffHead->pBuffer==decoded_frame->vir_addr){				
						 dec_buf_t *frame_header = decoded_frame->vo_frame_info;			
				     timestamp=frame_header->time_stamp;
				     pBuffHead->nTimeStamp = timestamp*1000;	
					   pBuffHead->nFilledLen = pBuffHead->nAllocLen;
				     pBuffHead->nOffset = 0;
				     pBuffHead->nFlags=OMX_BUFFERFLAG_ENDOFFRAME;
				     DEBUG(DEB_LEV_FULL_SEQ,"===h264 output Thumbnail,nFilledLen:%d===\n",pBuffHead->nFilledLen);		
				     p->vout->put_rbuf(p->vout);
				     (*(pOutPort->BufferProcessedCallback))(
			        pOutPort->standCompContainer,
			        p->callbackData,
			        pBuffHead);
						 break;
			}
	}
}
int check_spspps(unsigned char *buf, int len)
{
    int i;
    unsigned int zerolen;
    unsigned int nal_unit_type;

    i = 0;
    zerolen = 0;
    nal_unit_type = 0;
    while(i + 3 < len)
    {      
       if (buf[i] == 0){
           zerolen++;
       }else{
           if ((zerolen >= 2)&&(buf[i] == 0x01)) //find NAL
           {
              if ((nal_unit_type == 7))
              {
                  return nal_unit_type;
              }             
           //check spspps
              nal_unit_type = buf[i + 1] & 0x1f;
           }
           zerolen = 0;
       }
       i++;
    }
    return 0;
}
void omx_videodec_component_flush(OMX_COMPONENTTYPE *openmaxStandComp){
	VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE *)openmaxStandComp->pComponentPrivate; 
	omx_base_video_PortType *pInPort=(omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX]; 
	omx_base_video_PortType *pOutPort=(omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	int remainingframes=pComponentPrivate->vout->get_rbuf_num(pComponentPrivate->vout); 
	int counter=0;
	int loop_counter=0;
	OMX_BUFFERHEADERTYPE* pOutputBuffer = NULL;
	DEBUG(DEB_LEV_ERR,"remainingframes is %d\n",remainingframes);
	while(counter<remainingframes && loop_counter<pOutPort->sPortParam.nBufferCountActual){
		loop_counter++;//incase dead loop
		if(omx_videodec_output_relocation(openmaxStandComp,&pOutputBuffer)==1){
			counter++;
			pOutputBuffer->nFlags |= OMX_BUFFERFLAG_DECODEONLY;
			pOutPort->ReturnBufferFunction(pOutPort,pOutputBuffer);
	  }
  } 
	
}

void omx_checkthirdparty(VIDDEC_COMPONENT_PRIVATE *p,OMX_BUFFERHEADERTYPE* pInputBuffer){
	  if(p->bthirdpartychecked==OMX_TRUE){
	  	return;
	  }	  
  	if(pInputBuffer->nFilledLen >= sizeof(packet_header_t)){
  		p->bthirdparty=OMX_TRUE;
  		packet_header_t *pheader=(packet_header_t *)pInputBuffer->pBuffer;
  		if(pheader->header_type ==VIDEO_PACKET){
  			p->bthirdparty=OMX_FALSE;
  		}
  	}
  	p->bthirdpartychecked = OMX_TRUE;
}

/** This function is used to process the input buffer and provide one output buffer
  */
void omx_videodec_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pInputBuffer, OMX_BUFFERHEADERTYPE* pOutputBuffer) 
{
	VIDDEC_COMPONENT_PRIVATE *p = (VIDDEC_COMPONENT_PRIVATE *)openmaxStandComp->pComponentPrivate; 
	omx_base_video_PortType *pInPort=(omx_base_video_PortType *)p->ports[OMX_BASE_FILTER_INPUTPORT_INDEX]; 
	omx_base_video_PortType *pOutPort=(omx_base_video_PortType *)p->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = &(pOutPort->sPortParam);
	OMX_ERRORTYPE err = OMX_ErrorNone;
  dec_buf_t *vo_frame;
	int i;
	OMX_BOOL isneedrealloc=OMX_FALSE;
	OMX_BOOL isbuffernotengouh=OMX_FALSE;
	int ret=0;
	int naltype = 0;
	int alignh = 15;
	int costdown = 0;
	
	


	if(p->is_error_status==OMX_TRUE || p->IsPortBufferRealloc==OMX_TRUE){
		return;
	}
//第一个包为init_buf
	if(p->isFirstBuffer == 1)
	{

		p->isFirstBuffer = 0;

		if(pInputBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG)//OMXCodec.cpp
		{			

			
				DEBUG(DEB_LEV_ERR,"pInputBuffer->nFilledLen %d line:%d\n",pInputBuffer->nFilledLen,__LINE__);
				if(pInputBuffer->nFilledLen>256*1024){
					DEBUG(DEB_LEV_ERR,"config pkt is too big,and buffer is not enough\n");
				}
				memcpy(p->CodecConfigPkt,pInputBuffer->pBuffer,pInputBuffer->nFilledLen);
				p->CodecConfigPktLen = pInputBuffer->nFilledLen;
				if(p->avcodecReady == OMX_FALSE)
				{
					DEBUG(DEB_LEV_ERR,"in %s, line %d\n", __func__, __LINE__);
					err = omx_videodec_component_actvideoInit(p);
					if(err != OMX_ErrorNone)
					{
						DEBUG(DEB_LEV_ERR,"In %s Init codec lib failed\n", __func__);	
						pInputBuffer->nFilledLen = 0;
				        p->is_error_status=OMX_TRUE;
				    (*(p->callbacks->EventHandler))
      			(openmaxStandComp,
      			 p->callbackData,
      			OMX_EventError, /* The command was completed */
      			err, /* The commands was a OMX_CommandStateSet */
      			0, /* The state has been changed in message->messageParam */
      			NULL);
						return;		
					}
					p->avcodecReady = OMX_TRUE;
				}
				
				pInputBuffer->nFilledLen = 0;
				pInputBuffer->nFlags = 0;
				return;		 
			
		}

		if(p->avcodecReady == OMX_FALSE)
		{
			DEBUG(DEB_LEV_ERR,"in %s, line %d\n", __func__, __LINE__);
			err = omx_videodec_component_actvideoInit(p);
			if(err != OMX_ErrorNone)
			{
				DEBUG(DEB_LEV_ERR,"In %s Init codec lib failed\n", __func__);	
				pInputBuffer->nFilledLen = 0;
				p->is_error_status=OMX_TRUE;
				(*(p->callbacks->EventHandler))
      		(openmaxStandComp,
      		 p->callbackData,
      		OMX_EventError, /* The command was completed */
      		err, /* The commands was a OMX_CommandStateSet */
      		0, /* The state has been changed in message->messageParam */
      		NULL);
				return;		
			}
	
			p->avcodecReady = OMX_TRUE;
		}
	}
	
  if(p->isFirstBuffer==0\
		&& (pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC\
		|| pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC_91
		)){
		if(pInputBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG){
			    memcpy(p->CodecConfigPkt,pInputBuffer->pBuffer,pInputBuffer->nFilledLen);
			    p->CodecConfigPktLen = pInputBuffer->nFilledLen;
				pInputBuffer->nFilledLen = 0;
				pInputBuffer->nFlags = 0;
				p->bConfigPktStored = OMX_FALSE;
				p->IsProbed = OMX_FALSE;
				DEBUG(DEB_LEV_ERR,"new cfgpkt,nTimeStamp:%lld\n",p->CodecConfigPktLen,pInputBuffer->nTimeStamp);
				return;
		}
	}
	if(p->isFirstBuffer==0 && pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC){
		if(pInputBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG){
			  naltype=check_spspps(pInputBuffer->pBuffer,pInputBuffer->nFilledLen);
			  if(naltype == 0x7){
			  	memcpy(p->CodecConfigPkt,pInputBuffer->pBuffer,pInputBuffer->nFilledLen);
					p->CodecConfigPktLen = pInputBuffer->nFilledLen;
					pInputBuffer->nFilledLen = 0;
					pInputBuffer->nFlags = 0;
					p->bConfigPktStored = OMX_FALSE;
					p->IsProbed = OMX_FALSE;
					DEBUG(DEB_LEV_ERR,"new sps ,spsLen:%d,nTimeStamp:%lld\n",p->CodecConfigPktLen,pInputBuffer->nTimeStamp);
					return;
			  }
		}
	}
	omx_checkthirdparty(p,pInputBuffer);
	if(p->is_cur_stream_buf==OMX_FALSE && p->bthirdparty==OMX_TRUE){

				packet_header_t *pheader=(packet_header_t *)(pInputBuffer->pBuffer - sizeof(packet_header_t));
			  memset(pheader,0,sizeof(packet_header_t));
	  		pheader->header_type = VIDEO_PACKET;
	  		pheader->packet_ts = pInputBuffer->nTimeStamp/1000;	  			  
	  	  if(p->bComponentFlush==OMX_TRUE){
	  	  	pheader->seek_reset_flag = 1;
	  	  	p->bComponentFlush = OMX_FALSE;
	  	  }
        
	  	  if(p->bConfigPktStored==OMX_FALSE && p->CodecConfigPkt!=NULL && p->CodecConfigPktLen!=0 && (pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC || pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingMPEG4  || pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC || pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC_91)) {
		  	  		memcpy(p->pstrbuf,pInputBuffer->pBuffer, pInputBuffer->nFilledLen);	  	 
			  		memcpy(pInputBuffer->pBuffer,p->CodecConfigPkt,p->CodecConfigPktLen);
			  		pheader->block_len = pInputBuffer->nFilledLen+p->CodecConfigPktLen;
			  		memcpy(pInputBuffer->pBuffer+p->CodecConfigPktLen, p->pstrbuf, pInputBuffer->nFilledLen);
			  		pInputBuffer->nFilledLen = pInputBuffer->nFilledLen+sizeof(packet_header_t)+p->CodecConfigPktLen;
			  		p->bConfigPktStored=OMX_TRUE;
			  		DEBUG(DEB_LEV_ERR,"###memcpy sps/pps pkt###\n");

	  	  	}else{
	  	  	  	  		
	  			  pheader->block_len = pInputBuffer->nFilledLen;
				    pInputBuffer->nFilledLen += sizeof(packet_header_t);
	  	  }
	}

	

DEC_PROBE:
//===============probe=========================	
  if( p->is_fifo_disposing==OMX_FALSE  && p->IsProbed==OMX_FALSE && \
  	 (pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC || \
  	  pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC_91 || \
  		pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingVP9 || \
  		pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVS || \
  	  pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC)){
  	video_codec_info_t video_codec_info;
  	OMX_CONFIG_RECTTYPE *crop = &(p->croprect);
  	OMX_CONFIG_RECTTYPE PortOutCrop;
  	stream_buf_handle bitstream_buf;
  	int ref_num_level=0;
		
		bitstream_buf.data_len = pInputBuffer->nFilledLen;
		if (p->bthirdparty==OMX_TRUE){
			bitstream_buf.vir_addr = pInputBuffer->pBuffer - sizeof(packet_header_t);	
		}else{
			bitstream_buf.vir_addr = pInputBuffer->pBuffer;
	  }	


	  bitstream_buf.phy_addr=(unsigned char*)actal_get_phyaddr(bitstream_buf.vir_addr);


    if(p->bLowRam==OMX_TRUE){
    	costdown =1;
    }

  	p->IsProbed=OMX_TRUE;

  	ret = p->p_interface->probe(p->CodecConfigPktLen>0 ? p->CodecConfigPkt:NULL,&bitstream_buf,&video_codec_info);
  	if(ret!=0){
  		DEBUG(DEB_LEV_ERR,"===probe err===\n");
  		pInputBuffer->nFilledLen = 0;
			p->is_error_status=OMX_TRUE;
			err = OMX_ErrorUndefined;
			(*(p->callbacks->EventHandler))
			(openmaxStandComp,
		 	p->callbackData,
			OMX_EventError, /* The command was completed */
			err, /* The commands was a OMX_CommandStateSet */
			0, /* The state has been changed in message->messageParam */
			NULL);
			return; 
  	}
    if(pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingVP8){
    	video_codec_info.ref_num = 3;
    }
		if (video_codec_info.supported == PLUGIN_NOT_SUPPORTED_OTHER)
		{
			DEBUG(DEB_LEV_ERR,"In %s Init h264 lib failed\n", __func__); 
			pInputBuffer->nFilledLen = 0;
			p->is_error_status=OMX_TRUE;
			err = OMX_ErrorUndefined;
			(*(p->callbacks->EventHandler))
			(openmaxStandComp,
		 	p->callbackData,
			OMX_EventError, /* The command was completed */
			err, /* The commands was a OMX_CommandStateSet */
			0, /* The state has been changed in message->messageParam */
			NULL);
			return; 	
		}

  	DEBUG(DEB_LEV_ERR,"===probed,src_width:%d,src_height:%d,,nWidth:%d,nHeight:%d,ref_num:%d=== \n",video_codec_info.src_width,video_codec_info.src_height,video_codec_info.width,video_codec_info.height,video_codec_info.ref_num); 
  	
  	PortOutCrop.nSize = sizeof(OMX_CONFIG_RECTTYPE);
  	
    if(pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC || pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC_91){
    	int xexpand=0;
    	int yexpand=0;
    	if(p->bHdeExsit==OMX_FALSE || pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC_91){
    		xexpand =32;
    		yexpand =32;
    	}
    	PortOutCrop.nLeft = video_codec_info.xpos + xexpand;
    	PortOutCrop.nTop =  video_codec_info.ypos + yexpand;
    	PortOutCrop.nWidth =  video_codec_info.width;
  	  PortOutCrop.nHeight = video_codec_info.height; 
    }else{
    	//for avs fieldmode
    	if(pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVS && video_codec_info.supported == PLUGIN_NOT_SUPPORTED_FIELDMOD){
    		video_codec_info.width= video_codec_info.width;
    		video_codec_info.height = video_codec_info.height/2;
    	}
    	PortOutCrop.nLeft = video_codec_info.xpos;
    	PortOutCrop.nTop =  video_codec_info.ypos;
    	PortOutCrop.nWidth =  video_codec_info.width;
  	  PortOutCrop.nHeight = video_codec_info.height; 
    }
    
  	
    if(!strcasecmp(p->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC)){
    	if(p->bHdeExsit){
    		alignh = 7;
    	}
    } 
    
 		DEBUG(DEB_LEV_ERR,"alignh:%d\n",alignh);
    if(p->bHdeExsit==OMX_FALSE){
    	if((((video_codec_info.src_width+31)&(~31))!=pPortDef->format.video.nStride || ((video_codec_info.src_height+alignh)&(~alignh))!=((pPortDef->format.video.nFrameHeight+alignh)&(~alignh))) && \
  			pInPort->sVideoParam.eCompressionFormat != OMX_VIDEO_CodingHEVC && pInPort->sVideoParam.eCompressionFormat != OMX_VIDEO_CodingHEVC_91&& \
  		  pInPort->sVideoParam.eCompressionFormat != OMX_VIDEO_CodingVP9 ){
  		  	isneedrealloc= OMX_TRUE;
  		  	if(p->bResizeEnabled==OMX_TRUE){
  		  		if(((video_codec_info.src_width+15)&(~15))!=pPortDef->format.video.nStride \
  		  			|| ((video_codec_info.src_height+alignh)&(~alignh))!=((pPortDef->format.video.nFrameHeight+alignh)&(~alignh))){
  		  			p->bResizeRealloced = OMX_TRUE ;
  		  			DEBUG(DEB_LEV_ERR,"width or height is not correct\n");
  		  		}else{
  		  			isneedrealloc = OMX_FALSE;
  		  		}
  		  	}
  		  	if(isneedrealloc==OMX_TRUE){
  		  		pPortDef->format.video.nStride = \
  					pPortDef->format.video.nFrameWidth =  (video_codec_info.src_width+31)&(~31);
						pPortDef->format.video.nSliceHeight =\
  					pPortDef->format.video.nFrameHeight = (video_codec_info.src_height +alignh)&(~alignh);
  		  	}
					
  		}
    }else{
    	if((((video_codec_info.src_width+31)&(~31))!=pPortDef->format.video.nStride || ((video_codec_info.src_height+alignh)&(~alignh))!=((pPortDef->format.video.nFrameHeight+alignh)&(~alignh))) && \
  			pInPort->sVideoParam.eCompressionFormat != OMX_VIDEO_CodingHEVC_91 && pInPort->sVideoParam.eCompressionFormat != OMX_VIDEO_CodingVP9){
  				isneedrealloc = OMX_TRUE;
  				if(p->bResizeEnabled==OMX_TRUE){
  		  		if(((video_codec_info.src_width+15)&(~15))!=pPortDef->format.video.nStride  \
  		  			|| ((video_codec_info.src_height+alignh)&(~alignh))!=((pPortDef->format.video.nFrameHeight+alignh)&(~alignh))){
  		  			p->bResizeRealloced = OMX_TRUE ;
  		  	
  		  			DEBUG(DEB_LEV_ERR,"width or height is not correct,relocate buffer and set bResizeEnabled to false\n");
  		  		}else{
  		  			isneedrealloc=OMX_FALSE;
  		  		}
  		  	}
  		  	if(isneedrealloc==OMX_TRUE){
  		  		pPortDef->format.video.nStride = \
  					pPortDef->format.video.nFrameWidth =  (video_codec_info.src_width+31)&(~31);
						pPortDef->format.video.nSliceHeight =\
  					pPortDef->format.video.nFrameHeight = (video_codec_info.src_height +alignh)&(~alignh);
  		  	}
  		}
    }
    if(p->bResizeRealloced == OMX_TRUE){
    	 usleep(20*1000);
    }
    if(crop->nLeft != PortOutCrop.nLeft   || crop->nTop != PortOutCrop.nTop || \
    	 crop->nWidth !=PortOutCrop.nWidth  || crop->nHeight !=PortOutCrop.nHeight){
    	 memcpy(crop,&PortOutCrop,sizeof(OMX_CONFIG_RECTTYPE));
    	 DEBUG(DEB_LEV_ERR,"crop region change to,%d,%d,%d,%d\n",crop->nLeft,crop->nTop,crop->nWidth,crop->nHeight);    	
  		(*(p->callbacks->EventHandler))
  		(openmaxStandComp,p->callbackData,
      OMX_EventPortSettingsChanged, /* The command was completed */
      OMX_BASE_FILTER_OUTPUTPORT_INDEX, /* The commands was a OMX_CommandStateSet */
      OMX_IndexConfigCommonOutputCrop, /* The state has been changed in message->messageParam */
      NULL);
    }
  	if(p->bResizeEnabled==OMX_FALSE || p->bResizeRealloced==OMX_TRUE){
  		
  		if((video_codec_info.ref_num+4-costdown)>pPortDef->nBufferCountActual && p->is_Thumbnail==OMX_FALSE){
  			 isbuffernotengouh =OMX_TRUE;
  			 p->bBufferNotEngouh = OMX_TRUE;
  		}
  		p->bResizeRealloced=OMX_FALSE;
  		
  		if(isbuffernotengouh==OMX_TRUE || isneedrealloc==OMX_TRUE){
  			p->IsPortBeingDisabled  =OMX_TRUE;
  			if(p->bthirdparty==OMX_TRUE && isneedrealloc==OMX_TRUE){ 
  				p->nNewBufferCountActual=(video_codec_info.ref_num + 4);
  			}else{
  				p->nNewBufferCountActual=(video_codec_info.ref_num + 4-costdown);
  			}
  			
      DEBUG(DEB_LEV_ERR,"buffers:%d is not enough,need %d \n",pPortDef->nBufferCountActual,p->nNewBufferCountActual);
      p->IsPortBufferRealloc=OMX_TRUE;
      if(p->is_videodec_fifo_init==OMX_TRUE){
      	 raw_fifo_timeout_wakeup(p->vout,&(p->suspend_flag));
      	 if((pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC ||pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC ||pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC_91) && p->p_handle!=NULL)
         {
        	 p->p_interface->ex_ops(p->p_handle,FLUSH_BUFFERS_REMAINING,0);	
         }
         p->vout->fifo_reset(p->vout);
         raw_fifo_internal_dispose(p->vout,p->IsGraphicBuffer);	
      }
     
      if(p->bResizeEnabled==OMX_TRUE){
      	pOutputBuffer->nFlags |= OMX_BUFFERFLAG_DECODEONLY;
    		pOutputBuffer->nFilledLen = 0;
      	(*(pOutPort->BufferProcessedCallback))(
        	pOutPort->standCompContainer,
        	p->callbackData,
        	pOutputBuffer);
      }
      omx_videodec_FlushBeforeDisableOutPort(pOutPort); //return output buffer to omxil
			(*(p->callbacks->EventHandler))
      				(openmaxStandComp,
      				p->callbackData,
      				OMX_EventPortSettingsChanged, /* The command was completed */
      				OMX_BASE_FILTER_OUTPUTPORT_INDEX, /* The commands was a OMX_CommandStateSet */
      				0, /* The state has been changed in message->messageParam */
      				NULL);   	

      if(p->bthirdparty==OMX_TRUE){
      		 pInputBuffer->nFilledLen -= sizeof(packet_header_t);
      	}
     	return;					
  		}
  	}
  	if(p->bResizeEnabled==OMX_TRUE){
  		p->framebuffernum =video_codec_info.ref_num+2;
  	}
  }


  if(p->bResizeEnabled==OMX_TRUE){  		
  		if(p->is_videodec_fifo_init==OMX_FALSE){
  	 		DEBUG(DEB_LEV_ERR,"framebuffernum is %d \n",p->framebuffernum);
		 		for(i = 0;i<p->framebuffernum;i++) {		 	 
					ret=raw_fifo_init(p->vout,p->framebuffernum,p->videobuffersize);//note pBuffHead->nAllocLen
					if(ret<0){
						p->is_error_status = OMX_TRUE ;						
				  	err = OMX_ErrorUndefined;
				 		(*(p->callbacks->EventHandler))
      				(openmaxStandComp,
      		 		p->callbackData,
      				OMX_EventError, /* The command was completed */
      				err, /* The commands was a OMX_CommandStateSet */
      				0, /* The state has been changed in message->messageParam */
      				NULL);
          	return;
			  	}		
		 		}
		 		p->vout->fifo_reset(p->vout);
		 		p->is_videodec_fifo_init = OMX_TRUE;
			}
  }
  if(p->is_fifo_disposing==OMX_FALSE){
		frame_buf_handle *raw_buf_dec = NULL;
		


    if(p->bResizeEnabled==OMX_TRUE){
    	try_get_wbuf_timeout(&raw_buf_dec, p->vout, &(p->suspend_flag),50,1);
    }else{
    	try_get_wbuf_timeout(&raw_buf_dec, p->vout, &(p->suspend_flag),50,0);
    }
		
		if(raw_buf_dec==NULL){	
		//	p->vout->dump_info(p->vout);
			if(p->is_cur_stream_buf==OMX_FALSE && p->bthirdparty==OMX_TRUE){
				pInputBuffer->nFilledLen -= sizeof(packet_header_t);				
			}
			return ;
		}
		
		
	  // ========== start to decode ==============
		stream_buf_handle bitstream_buf;		
		bitstream_buf.data_len = pInputBuffer->nFilledLen;	

    if(p->bHdeExsit==OMX_FALSE){
  
     	  if(pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingDIV3 || pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingFLV1\
     	  	|| pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingHEVC || pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingHEVC_91\
	  	 		|| pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingH263 ||pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingVP9\
	  	 		||pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingWMV2  ||pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingRVG2 \
	  	 		||pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingAVS){
	  			if (p->bthirdparty==OMX_TRUE){
						bitstream_buf.vir_addr = pInputBuffer->pBuffer - sizeof(packet_header_t);		
					}else{
    				bitstream_buf.vir_addr = pInputBuffer->pBuffer;
	  	 		}
	  		}else{
					if (p->bthirdparty==OMX_TRUE){
						bitstream_buf.vir_addr = pInputBuffer->pBuffer - sizeof(packet_header_t);		
					}else{
    				bitstream_buf.vir_addr = pInputBuffer->pBuffer;
	  	 		}
		 			actal_cache_flush(bitstream_buf.vir_addr);
					bitstream_buf.phy_addr=(unsigned char*)actal_get_phyaddr(bitstream_buf.vir_addr);
	  		}
    }else{

     	  if(pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingDIV3 || pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingFLV1\
     	  	||pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingH263|| pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingRVG2 \
     	  	|| pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingHEVC_91 ||pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingWMV2\
     	  	||pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingVP9 ||pInPort->sVideoParam.eCompressionFormat==OMX_VIDEO_CodingAVS){
	  			if (p->bthirdparty==OMX_TRUE){
						bitstream_buf.vir_addr = pInputBuffer->pBuffer - sizeof(packet_header_t);		
					}else{
    				bitstream_buf.vir_addr = pInputBuffer->pBuffer;
	  	 		}
	  		}else{
					if (p->bthirdparty==OMX_TRUE){
						bitstream_buf.vir_addr = pInputBuffer->pBuffer - sizeof(packet_header_t);		
					}else{
    				bitstream_buf.vir_addr = pInputBuffer->pBuffer;
	  	 		}
		 			actal_cache_flush(bitstream_buf.vir_addr);
					bitstream_buf.phy_addr=(unsigned char*)actal_get_phyaddr(bitstream_buf.vir_addr);
	  		}
    }
	

			
		

		int rt = p->p_interface->decode_data(p->p_handle, &bitstream_buf);

	

    
			// h264解缩略图， 第一个包可能存在错误，这时候解码器报错，继续给数据进来。
		if (p->is_Thumbnail==OMX_TRUE && rt == PLUGIN_RETURN_NOT_SUPPORT && pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC) {
		    DEBUG(DEB_LEV_SIMPLE_SEQ,"*************** Thumbnail first packet not support***************\n");
            p->is_cur_stream_buf = OMX_FALSE;
            pInputBuffer->nFilledLen = 0; 
            return;            
        }
    if (p->is_Thumbnail==OMX_FALSE && rt == PLUGIN_RETURN_NOT_SUPPORT && pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC) {
            DEBUG(DEB_LEV_SIMPLE_SEQ,"*************** PLUGIN_RETURN_NOT_SUPPORT***************\n");
            rt = PLUGIN_RETURN_NORMAL;
    }     
    if(rt==PLUGIN_NOT_SUPPORTED_OTHER && pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC) { //for netflix changable resolution
    	p->IsProbed = OMX_FALSE;
    	DEBUG(DEB_LEV_ERR,"codec notify resolution change \n");
    	p->p_interface->ex_ops(p->p_handle,FLUSH_BUFFERS_REMAINING,0);
    	omx_videodec_component_flush(openmaxStandComp);
    	p->vout->fifo_reset(p->vout);
    	goto DEC_PROBE;    	
    }
		if (rt == PLUGIN_RETURN_NORMAL) {
			p->is_cur_stream_buf = OMX_FALSE;
			p->err_counter=0;
		} else if(rt == PLUGIN_RETURN_PKT_NOTEND) {
		p->is_cur_stream_buf = OMX_TRUE;
			p->err_counter=0;
		} else {
			p->err_counter++;
			p->is_cur_stream_buf = OMX_FALSE;	
			pInputBuffer->nFilledLen = 0;
			if(p->err_counter>30 || p->is_Thumbnail==OMX_TRUE || rt==PLUGIN_RETURN_NOT_SUPPORT){	 
				DEBUG(DEB_LEV_ERR,"decode err In %s,packet len :%d \n",__func__,bitstream_buf.data_len);
				p->is_error_status = OMX_TRUE ;						
				err = OMX_ErrorUndefined;
				(*(p->callbacks->EventHandler))
      		(openmaxStandComp,
      		 p->callbackData,
      		OMX_EventError, /* The command was completed */
      		err, /* The commands was a OMX_CommandStateSet */
      		0, /* The state has been changed in message->messageParam */
      		NULL);
    return;
    	}
		}
		if(p->is_cur_stream_buf == OMX_FALSE) {
		//一包多帧的码流包也解完了
		//omx_base_filter.c中会将该包emptybufferdone
			pInputBuffer->nFilledLen = 0; 
		}	
		if(p->is_Thumbnail==OMX_TRUE && p->is_h264_Thumbnail_outputed==OMX_FALSE && (pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVC || pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC || pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingHEVC_91) || pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingAVS) {
			p->is_h264_Thumbnail_outputed=OMX_TRUE;
			omx_videodec_Thumbnail_h264(openmaxStandComp,raw_buf_dec);			
		}
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
		DEBUG(DEB_LEV_SIMPLE_SEQ,"invalid state \n",__FILE__,__LINE__);
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
		DEBUG(DEB_LEV_SIMPLE_SEQ,"In %s VIDDEC_CheckSetParameter error\n", __func__);	
	}

    return eError;
}
void omx_videodec_getoutput(OMX_COMPONENTTYPE* openmaxStandComp,frame_buf_handle *raw_buf_lcd,OMX_BUFFERHEADERTYPE* pOutputBuffer)
{
	    VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE *)openmaxStandComp->pComponentPrivate;
	    omx_base_filter_PrivateType* omx_base_filter_Private = (omx_base_filter_PrivateType*)openmaxStandComp->pComponentPrivate;
	    omx_base_video_PortType *pInPort=(omx_base_video_PortType *)omx_base_filter_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX]; 
      omx_base_video_PortType *pOutPort=(omx_base_video_PortType *)omx_base_filter_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
      OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = &(pOutPort->sPortParam);
      OMX_CONFIG_RECTTYPE *PortOutCrop= &(pComponentPrivate->croprect);
			OMX_S64 timestamp = -1;
			OMX_S64 timestampfound = -1;
			OMX_S64 pts;
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
				pOutputBuffer->nFlags = OMX_BUFFERFLAG_ENDOFFRAME;
				 /*---add time compensation for cts test---*/
				if(pComponentPrivate->bthirdparty==OMX_TRUE){
				 	timestampfound = dequeueNode(pComponentPrivate->headNode,timestamp);
				  if(timestampfound>=0 ){
				  	pOutputBuffer->nTimeStamp = timestampfound;
				  }
				}
				if(pComponentPrivate->is_Thumbnail==OMX_FALSE){
					OMX_PTR phy_addr = 0;
					VR_Parm_t vr_parm;
					if(pComponentPrivate->IsGraphicBuffer==OMX_TRUE ||pComponentPrivate->bStoreMediadata[OMX_BASE_FILTER_OUTPUTPORT_INDEX]==OMX_TRUE){
						if(pComponentPrivate->IsGraphicBuffer==OMX_TRUE){
						  buff_handle=pOutputBuffer->pBuffer;
          	}else{
            	MetaData = (vde_video_metadata_t*)pOutputBuffer->pBuffer;
						  buff_handle =MetaData->handle;
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
					}else{
						phy_addr = actal_get_phyaddr(pOutputBuffer->pBuffer);
						memset(pOutputBuffer->pBuffer,0,pOutputBuffer->nFilledLen);
						actal_cache_flush(pOutputBuffer->pBuffer);
					}
					
					vr_parm.src_addr = raw_buf_lcd->phy_addr;
					vr_parm.srcstride = (pPortDef->format.video.nFrameWidth+VIDEO_WIDTH_ALIGN)&(~VIDEO_WIDTH_ALIGN);
					vr_parm.srcheight = (pPortDef->format.video.nFrameHeight+15)&(~15);
					vr_parm.cropw = PortOutCrop->nWidth;
					vr_parm.croph = PortOutCrop->nHeight;
					vr_parm.dst_addr = phy_addr;
					vr_parm.wscale = 1;
					vr_parm.hscale = 1;
					VceReSize_Run(pComponentPrivate->vce_handle,&vr_parm);
				
				}else{
					memcpy(pOutputBuffer->pBuffer,raw_buf_lcd->vir_addr,pOutputBuffer->nFilledLen);
				}		
				
				pComponentPrivate->vout->put_rbuf(pComponentPrivate->vout);	
      }
}
int omx_videodec_output_relocation(OMX_COMPONENTTYPE* openmaxStandComp,OMX_BUFFERHEADERTYPE** pout)
{
	    VIDDEC_COMPONENT_PRIVATE *p = (VIDDEC_COMPONENT_PRIVATE *)openmaxStandComp->pComponentPrivate;
	    omx_base_filter_PrivateType* omx_base_filter_Private = (omx_base_filter_PrivateType*)openmaxStandComp->pComponentPrivate;
	    omx_base_video_PortType *pInPort=(omx_base_video_PortType *)p->ports[OMX_BASE_FILTER_INPUTPORT_INDEX]; 
      omx_base_PortType *pOutPort=(omx_base_PortType *)omx_base_filter_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
      OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = &(pOutPort->sPortParam);
      OMX_CONFIG_RECTTYPE *PortOutCrop= &(p->croprect);
			OMX_BUFFERHEADERTYPE *pBuffHead;
			OMX_S64 timestamp;
			OMX_S64 timestampfound;
			OMX_S64 pts;
			int i=0;
			int j=0	;		
			int rt=0;	
			vde_video_metadata_t*	MetaData = NULL;
			void *buff_handle = NULL;
	    frame_buf_handle *raw_buf_lcd = NULL;
			raw_buf_lcd = p->vout->get_rbuf(p->vout);

			
			if(raw_buf_lcd != NULL) 
			{
				for(i = 0;i<pOutPort->sPortParam.nBufferCountActual;i++) {			

					pBuffHead=p->BufferHeadStorage[i];
					if(p->IsGraphicBuffer==OMX_TRUE ||p->bStoreMediadata[OMX_BASE_FILTER_OUTPUTPORT_INDEX]==OMX_TRUE){
						OMX_PTR phy_addr = 0;						
            if(p->IsGraphicBuffer==OMX_TRUE){

						  buff_handle=pBuffHead->pBuffer;
            }else{
            	MetaData = (vde_video_metadata_t*)pBuffHead->pBuffer;
						  buff_handle =MetaData->handle;
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
						
					   
						if(phy_addr==raw_buf_lcd->phy_addr){
						 dec_buf_t *frame_header =raw_buf_lcd->vo_frame_info;
						 timestamp = frame_header->time_stamp;
						 pBuffHead->nTimeStamp = timestamp*1000;
				     pBuffHead->nFilledLen = pBuffHead->nAllocLen;		    
				     pBuffHead->nOffset = 0; 
				     pBuffHead->nFlags=OMX_BUFFERFLAG_ENDOFFRAME;
				     /*---add time compensation for cts test---*/
				     if(p->bthirdparty==OMX_TRUE){
				     	timestampfound = dequeueNode(p->headNode,timestamp);
				     	if(timestampfound>=0 ){
				     		pBuffHead->nTimeStamp = timestampfound;
				     	}
				     }
		
				     *pout=pBuffHead; 


				     p->vout->put_rbuf(p->vout);
				     rt=1;		    
						 break;
						}
					}else{
	
						if(pBuffHead->pBuffer==raw_buf_lcd->vir_addr){				
						 dec_buf_t *frame_header = raw_buf_lcd->vo_frame_info;	
						 timestamp = frame_header->time_stamp;		
				   	 pBuffHead->nTimeStamp = timestamp*1000;
				   	  /*---add time compensation for cts test---*/
				     if(p->bthirdparty==OMX_TRUE){
				     	timestampfound = dequeueNode(p->headNode,timestamp);
				     	if(timestampfound>=0 ){
				     		pBuffHead->nTimeStamp = timestampfound;
				     	}
				     }			     				     				  
				     pBuffHead->nFilledLen = pBuffHead->nAllocLen;				     
				     pBuffHead->nOffset = 0;
				     pBuffHead->nFlags=OMX_BUFFERFLAG_ENDOFFRAME;
				     *pout=pBuffHead;
				     p->vout->put_rbuf(p->vout);
				     rt=1;
						 break;
						}
					}
				
					
			  }
			  if(i==pOutPort->sPortParam.nBufferCountActual){
			  	 DEBUG(DEB_LEV_ERR,"===not in queue===\n");
			  }
      }
      return rt;
}
void omx_videodec_raw_fifo_queue(OMX_HANDLETYPE hComponent,OMX_BUFFERHEADERTYPE* pBufferHead)
{
	OMX_COMPONENTTYPE *pHandle = (OMX_COMPONENTTYPE *)hComponent;
	VIDDEC_COMPONENT_PRIVATE *pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE *)pHandle->pComponentPrivate;
	pComponentPrivate->vout->fifo_queue(pComponentPrivate->vout,pBufferHead->pAppPrivate,pBufferHead->pBuffer,pComponentPrivate->IsSWcodec);
}
OMX_ERRORTYPE VIDDEC_Load_Defaults (VIDDEC_COMPONENT_PRIVATE* pComponentPrivate, OMX_S32 nPassing)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;
	omx_base_video_PortType *inPort,*outPort;


	inPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	outPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

	DEBUG(DEB_LEV_SIMPLE_SEQ,"in VIDDEC_Load_Defaults nPassing %d\n",nPassing);

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

//			pComponentPrivate->destinationState 								= OMX_StateInvalid;
			pComponentPrivate->state 							 			= OMX_StateInvalid;

//后续debug
//			pComponentPrivate->CodaSeqInited  								= OMX_FALSE;
//			pComponentPrivate->CodaFrameBRegistered							= OMX_FALSE;

//			pComponentPrivate->bIsInputFlushPending							= OMX_FALSE;
//			pComponentPrivate->bIsOutputFlushPending						 	= OMX_FALSE;

//			pComponentPrivate->bInPortSettingsChanged						= OMX_FALSE;
//			pComponentPrivate->bOutPortSettingsChanged						= OMX_FALSE;
//			pComponentPrivate->bDynamicConfigurationInProgress				 	= OMX_FALSE;
			
//			pComponentPrivate->NeedFreeAllBuffers 							= OMX_TRUE;
//
//			pComponentPrivate->SeqErrorTest=0;
			
			break;

		case VIDDEC_INIT_AVC:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    	inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_AVC;
    	inPort->sVideoParam.eCompressionFormat            			= OMX_VIDEO_CodingAVC;
    	inPort->sVideoParam.eColorFormat                  			= VIDDEC_COLORFORMATUNUSED;
			
			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEAVC;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEAVC);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingAVC;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin           = VIDDEC_MIN_OUTPUT_BUFFER_NUM;

			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_AVC);

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
		case VIDDEC_INIT_HEVC:
			inPort->sVideoParam.nPortIndex                   			= VIDDEC_INPUT_PORT;
    			inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_HEVC;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingHEVC;
       	 	inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

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
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM_HEVC;//VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;
			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC);
			break;
			
	  case VIDDEC_INIT_HEVC_91:
			inPort->sVideoParam.nPortIndex                   			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_HEVC_91;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingHEVC_91;
       	 	inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

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
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM_HEVC;//VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin           = VIDDEC_MIN_OUTPUT_BUFFER_NUM;
			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC_91);
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
			DEBUG(DEB_LEV_SIMPLE_SEQ,"in VIDDEC_INIT_MJPEG\n");
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
DEBUG(DEB_LEV_SIMPLE_SEQ,"out VIDDEC_INIT_MJPEG\n");			
			break;
 case VIDDEC_INIT_WMV2:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_WMV2;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingWMV2;
    		inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEVC1;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPWMV2);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingWMV2;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

//			pComponentPrivate->IsCodaSeqInitBuf  								= OMX_TRUE;
//			pComponentPrivate->IsCodaSeqInitWMV9 								= OMX_FALSE;
//			pComponentPrivate->CodaSeqInitBufLen								= 0;


			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_WMV2);
			
			break;
 case VIDDEC_INIT_VP9:
			inPort->sVideoParam.nPortIndex                    			= VIDDEC_INPUT_PORT;
    		inPort->sVideoParam.nIndex                        			= VIDDEC_DEFAULT_INPUT_INDEX_VP9;
    		inPort->sVideoParam.eCompressionFormat            		= OMX_VIDEO_CodingVP9;
    		inPort->sVideoParam.eColorFormat                  		= VIDDEC_COLORFORMATUNUSED;

			inPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_INPUT_BUFFER_SIZE;
			inPort->sPortParam.nBufferCountActual             			= VIDDEC_DEFAULT_INPUT_BUFFER_NUM;
			inPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_INPUT_BUFFER_NUM;
			//inPort->sPortParam.format.video.cMIMEType				= VIDDEC_MIMETYPEVC1;
			strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPVP9);
    		inPort->sPortParam.format.video.nFrameWidth         		= VIDDEC_DEFAULT_WIDTH;
    		inPort->sPortParam.format.video.nFrameHeight        		= VIDDEC_DEFAULT_HEIGHT;
			inPort->sPortParam.format.video.nStride 				= VIDDEC_DEFAULT_WIDTH;
			inPort->sPortParam.format.video.nSliceHeight			= 0;
    		inPort->sPortParam.format.video.eCompressionFormat  	= OMX_VIDEO_CodingVP9;
    		inPort->sPortParam.format.video.eColorFormat        		= VIDDEC_COLORFORMATUNUSED;

			outPort->sPortParam.nBufferSize                      			= VIDDEC_DEFAULT_OUTPUT_BUFFER_SIZE;
			outPort->sPortParam.nBufferCountActual				= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM_VP9;
			outPort->sPortParam.nBufferCountMin             			= VIDDEC_MIN_OUTPUT_BUFFER_NUM;

//			pComponentPrivate->IsCodaSeqInitBuf  								= OMX_TRUE;
//			pComponentPrivate->IsCodaSeqInitWMV9 								= OMX_FALSE;
//			pComponentPrivate->CodaSeqInitBufLen								= 0;


			strcpy((char*)pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_VP9);
			
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
			outPort->sPortParam.nBufferCountActual             		= VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM_HEVC;//VIDDEC_DEFAULT_OUTPUT_BUFFER_NUM;
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
		DEBUG(DEB_LEV_SIMPLE_SEQ,"In %s VIDDEC_Load_Defaults error\n", __func__);	
	}


	return eError;

}


OMX_ERRORTYPE omx_videodec_component_FillThisBuffer(
  OMX_HANDLETYPE hComponent,
  OMX_BUFFERHEADERTYPE* pBufferHead) {

  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
  omx_base_PortType *pPort;
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
  if(pComponentPrivate->IsPortBufferRealloc==OMX_TRUE){
		   pBufferHead->nFlags |=OMX_BUFFERFLAG_DECODEONLY;
		   pBufferHead->nFilledLen =0;
				(*(pPort->BufferProcessedCallback))(
        pPort->standCompContainer,
        omx_base_component_Private->callbackData,
        pBufferHead);
        return  OMX_ErrorNone;
	}
     /*------here we queue the fifo to internal(actually we just set reserved2)------*/
  if(pComponentPrivate->is_fifo_disposing==OMX_FALSE && pComponentPrivate->bResizeEnabled==OMX_FALSE &&  pComponentPrivate->IsPortBufferRealloc==OMX_FALSE){
		omx_videodec_raw_fifo_queue(hComponent,pBufferHead);
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

 
	DEBUG(DEB_LEV_SIMPLE_SEQ,"   Setting parameter %x,%s\n", nParamIndex,__func__);
	
	OMX_CONF_CHECK_CMD(hComponent, ComponentParameterStructure, OMX_TRUE);
	pHandle= (OMX_COMPONENTTYPE*)hComponent;
	pComponentPrivate = pHandle->pComponentPrivate;
	
	inPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	outPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

	eError = VIDDEC_CheckSetParameter(pComponentPrivate, ComponentParameterStructure, nParamIndex);

	if (eError != OMX_ErrorNone)
	{
		eError = OMX_ErrorIncorrectStateOperation;
		DEBUG(DEB_LEV_SIMPLE_SEQ,"CheckSetParameter error ,%s\n",__func__);
		goto EXIT;
	}
	
	switch (nParamIndex) 
	{
		case OMX_IndexParameterStoreMediaData:

			pMediaType = (StoreMetaDataInBuffersParams*)ComponentParameterStructure;		
			if(pMediaType->nPortIndex==VIDDEC_OUTPUT_PORT){
				DEBUG(DEB_LEV_ERR,"out bStoreMetaData \n");
				pComponentPrivate->bStoreMediadata[VIDDEC_OUTPUT_PORT] = pMediaType->bStoreMetaData;
			}else{
				eError = OMX_ErrorBadPortIndex;
				goto EXIT;
			}
			break;
			
		case OMX_IndexThumbnail:
			pComponentPrivate->is_Thumbnail=OMX_TRUE;
			DEBUG(DEB_LEV_SIMPLE_SEQ,"SetParameter OMX_IndexThumbnail\n");
			break;
		case OMX_IndexParamStreamBufferMode://sk added
			DEBUG(DEB_LEV_SIMPLE_SEQ,"SetParameter OMX_IndexParamStreamBufferMode\n");			
			break;
		case OMX_GoogleAndroidIndexEnableAndroidNativeBuffers:
			DEBUG(DEB_LEV_SIMPLE_SEQ,"SetParameter OMX_GoogleAndroidIndexEnableAndroidNativeBuffers\n");			
			break;
    case OMX_IndexParamStandardComponentRole:
    		DEBUG(DEB_LEV_SIMPLE_SEQ,"SetParameter OMX_IndexParamStandardComponentRole 0\n");
    		if (ComponentParameterStructure != NULL) 
    		{
    			char std_string[10];
        		pRole = (OMX_PARAM_COMPONENTROLETYPE *)ComponentParameterStructure;
DEBUG(DEB_LEV_SIMPLE_SEQ,"SetParameter OMX_IndexParamStandardComponentRole pRole->cRole %s\n",pRole->cRole);        		
        		/*OMX_CONF_CHK_VERSION( pRole, OMX_PARAM_COMPONENTROLETYPE, eError, pComponentPrivate->dbg);*/
        		if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_H263) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_H263);
                    		strcpy(std_string, "h263");
                		}
                		else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_AVC) == 0) {
                    		eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_AVC);
                    		strcpy(std_string, "h264");
                		}
                		else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_VP8) == 0) {
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
										else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_HEVC) == 0){
											eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_HEVC);
											strcpy(std_string, "hevc");
										}
										else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_HEVC_91) == 0){
											eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_HEVC_91);
											strcpy(std_string, "hevc_91");
										}
										else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_WMV2) == 0){
											eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_WMV2);
											strcpy(std_string, "wmv8");
										}
										else if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_VP9) == 0){
											eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_VP9);
											strcpy(std_string, "vp9");
										}
            				else {
									DEBUG(DEB_LEV_SIMPLE_SEQ,"%s %d,badparameter\n",__FUNCTION__,__LINE__);
                    		eError = OMX_ErrorBadParameter;
                		}
						
                		if(eError != OMX_ErrorNone) {
                   	 		goto EXIT;
                		}
        if(strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_RVG2) == 0 || \
        	 strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_HEVC) == 0 ||  \
        	 strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_HEVC_91) == 0 || \
					 strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_WMV2) == 0 || \
					 strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_AVS) == 0  ||  \
					 strcmp( (char*)pRole->cRole, VIDDEC_COMPONENTROLES_VP9) == 0){
						if(pComponentPrivate->bHdeExsit==OMX_FALSE){
							if( outPort->sVideoParam.eColorFormat != VIDDEC_COLORFORMAT420) {
								eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_PLANAR420);
							}
						}else{
							if(!strcmp((char*)pRole->cRole, VIDDEC_COMPONENTROLES_HEVC)){
								eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_SEMIPLANAR420);								
							}else{
								eError = VIDDEC_Load_Defaults(pComponentPrivate, VIDDEC_INIT_PLANAR420);	
							}
						}
						if(eError != OMX_ErrorNone) {
							goto EXIT;
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
 

    		} 
    		else {
    			DEBUG(DEB_LEV_SIMPLE_SEQ,"%s %d,badparameter\n",__FUNCTION__,__LINE__);
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
							//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPEAVC;
							strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEAVC);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingAVC;
	                		break;
	                   	case OMX_VIDEO_CodingMPEG2:
							//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPEMPEG2;
							strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEMPEG2);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingMPEG2;
	               	 		break;
						case OMX_VIDEO_CodingMPEG4:
	                		//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPEMPEG4;
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEMPEG4);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingMPEG4;
	                		break;
						case OMX_VIDEO_CodingRV:
	                		//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPERV;
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPERV);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingRV;
	                		break;
						case OMX_VIDEO_CodingVC1:
	                		//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPEVC1;
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEVC1);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingVC1;
	                		break;
						case OMX_VIDEO_CodingMJPEG:
	                		//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPEMJPEG;
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEMJPEG);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingMJPEG;
	                		break;
						case OMX_VIDEO_CodingAVS:
	                		//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPEAVS;
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEAVS);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingAVS;
	                		break;
						case OMX_VIDEO_CodingDIV3:
	                		//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPEDIV3;
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEDIV3);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingDIV3;
	                		break;
	          case OMX_VIDEO_CodingVP6:
	                		//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPEDIV3;
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEVP6);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingVP6;
	                		break;
	          case OMX_VIDEO_CodingVP8:
	                		//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPEDIV3;
	                		strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEVP8);
	                		inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingVP8;
	                		break;
						case OMX_VIDEO_CodingRVG2:
							//inPort->sPortParam.format.video.cMIMEType = VIDDEC_MIMETYPERVG2;
							strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPERVG2);
							inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingRVG2;
							break;
						case OMX_VIDEO_CodingHEVC:
		          strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEHEVC);
							inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingHEVC;
							break;
						case OMX_VIDEO_CodingHEVC_91:
		          strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPEHEVC_91);
							inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingHEVC_91;
							break;
						case OMX_VIDEO_CodingWMV2:
		          strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPWMV2);
							inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingWMV2;
							break;
						case OMX_VIDEO_CodingVP9:
		          strcpy(inPort->sPortParam.format.video.cMIMEType,VIDDEC_MIMETYPVP9);
							inPort->sPortParam.format.video.eCompressionFormat  = OMX_VIDEO_CodingVP9;
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
				omx_base_video_PortType * pPort = (omx_base_video_PortType *)pComponentPrivate->ports[pPortDefParam->nPortIndex];
        OMX_PARAM_PORTDEFINITIONTYPE *pPortParam = &pPort->sPortParam;
 	      int alignh = 15;
 	      if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC)){
 	      	if(pComponentPrivate->bHdeExsit){
 	      		alignh = 7;
    			}else{
    				pComponentPrivate->IsSWcodec = OMX_TRUE;
    			}
    		} 
    		
    		if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC_91) || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_WMV2) || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_VP9) || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_AVS)){
    			pComponentPrivate->IsSWcodec = OMX_TRUE;
    		} 
	     
  		  
  		  memcpy(pPortParam, pPortDefParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
  		  
	    	if (pPortDefParam->nPortIndex == VIDDEC_INPUT_PORT){

#ifdef ENABLE_VCERESIZE
  		      if(((pPortParam->format.video.nFrameWidth == 176 && pPortParam->format.video.nFrameHeight==144) || ((pPortParam->format.video.nFrameWidth == 720 && pPortParam->format.video.nFrameHeight==480))) && pComponentPrivate->is_Thumbnail==OMX_FALSE && pComponentPrivate->IsSWcodec==OMX_FALSE){
  		      	pPortParam->format.video.nStride = pPortParam->format.video.nFrameWidth;
  		      	pPortParam->format.video.nSliceHeight = pPortParam->format.video.nFrameHeight; 
  		      }else{
  		      	pPortParam->format.video.nStride = \
  		      	pPortParam->format.video.nFrameWidth =(pPortParam->format.video.nFrameWidth+ 31)&(~31);
  		      	pPortParam->format.video.nSliceHeight =\
							pPortParam->format.video.nFrameHeight=(pPortParam->format.video.nFrameHeight+alignh)&(~alignh);
  		      }
#else
						pPortParam->format.video.nStride =    \
						pPortParam->format.video.nFrameWidth =(pPortParam->format.video.nFrameWidth+ 31)&(~31);
						pPortParam->format.video.nSliceHeight = \
						pPortParam->format.video.nFrameHeight=(pPortParam->format.video.nFrameHeight+alignh)&(~alignh);
#endif
		    
	
				}else if (pPortDefParam->nPortIndex == VIDDEC_OUTPUT_PORT){
					   OMX_CONFIG_RECTTYPE *PortOutCrop= &(pComponentPrivate->croprect);	
	      		 PortOutCrop->nSize = sizeof(OMX_CONFIG_RECTTYPE);
  			     PortOutCrop->nLeft = 0;
  		  		 PortOutCrop->nTop =  0;
  		       PortOutCrop->nWidth =  pPortParam->format.video.nFrameWidth;
  		       PortOutCrop->nHeight = pPortParam->format.video.nFrameHeight;
	        	if(pPortParam->format.video.nFrameWidth == 0){
	        		pPortParam->format.video.nFrameWidth = VIDDEC_DEFAULT_WIDTH;
	        	}
	        	if(pPortParam->format.video.nFrameHeight == 0){
	        		pPortParam->format.video.nFrameHeight = VIDDEC_DEFAULT_HEIGHT;
	        	}
#ifdef ENABLE_VCERESIZE
	        	if(((pPortParam->format.video.nFrameWidth == 176 && pPortParam->format.video.nFrameHeight==144) || ((pPortParam->format.video.nFrameWidth == 720 && pPortParam->format.video.nFrameHeight==480))) && pComponentPrivate->is_Thumbnail==OMX_FALSE && pComponentPrivate->IsSWcodec==OMX_FALSE){
  		      	pPortParam->format.video.nStride = pPortParam->format.video.nFrameWidth;
  		      	pPortParam->format.video.nSliceHeight =  pPortParam->format.video.nFrameHeight; 
  		      }else{
  		      	if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC) || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC_91)){
							 
							  if(pComponentPrivate->bHdeExsit==OMX_FALSE || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC_91)){
							  	pPortParam->format.video.nStride = \
							 		pPortParam->format.video.nFrameWidth =   ((pPortParam->format.video.nFrameWidth+31)&(~31))+2*32;
							  	pPortParam->format.video.nSliceHeight = \
							 	  pPortParam->format.video.nFrameHeight =  ((pPortParam->format.video.nFrameHeight+31)&(~31))+2*32;
							  }else{
							  	pPortParam->format.video.nStride = \
							 		pPortParam->format.video.nFrameWidth =   ((pPortParam->format.video.nFrameWidth+31)&(~31));
									pPortParam->format.video.nSliceHeight = \
							 	  pPortParam->format.video.nFrameHeight =  ((pPortParam->format.video.nFrameHeight+alignh)&(~alignh));
							  }
							 
														  
							}else if (!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_WMV2)){
							  pPortParam->format.video.nStride = \
							  pPortParam->format.video.nFrameWidth =   (pPortParam->format.video.nFrameWidth+31)&(~31);
							  pPortParam->format.video.nSliceHeight = \
							  pPortParam->format.video.nFrameHeight =  (pPortParam->format.video.nFrameHeight+15)&(~15);
						  }else if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_VP9)){
						 	  pPortParam->format.video.nStride = \
							  pPortParam->format.video.nFrameWidth =   (pPortParam->format.video.nFrameWidth+15)&(~15);
							  pPortParam->format.video.nSliceHeight = \
							  pPortParam->format.video.nFrameHeight =  (pPortParam->format.video.nFrameHeight+31)&(~31);							 
						  }else if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_AVS)){
						  	 	pPortParam->format.video.nStride = \
							 		pPortParam->format.video.nFrameWidth =   (pPortParam->format.video.nFrameWidth+31)&(~31);
							  	pPortParam->format.video.nSliceHeight = \
							  	pPortParam->format.video.nFrameHeight =  (pPortParam->format.video.nFrameHeight+15)&(~15);
						  }else{
						 		pPortParam->format.video.nStride = \
							  pPortParam->format.video.nFrameWidth =   (pPortParam->format.video.nFrameWidth+31)&(~31);
							  pPortParam->format.video.nSliceHeight = \
							  pPortParam->format.video.nFrameHeight =  (pPortParam->format.video.nFrameHeight+15)&(~15);
						  }
  		      }
#else
						if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC) || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC_91)){
							if(pComponentPrivate->bHdeExsit==OMX_FALSE || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC_91)){
								pPortParam->format.video.nStride = \
							 	pPortParam->format.video.nFrameWidth =   ((pPortParam->format.video.nFrameWidth+31)&(~31))+2*32;
							  pPortParam->format.video.nSliceHeight = \
							 	pPortParam->format.video.nFrameHeight =  ((pPortParam->format.video.nFrameHeight+31)&(~31))+2*32;
							}else{
								pPortParam->format.video.nStride = \
							 	pPortParam->format.video.nFrameWidth =   ((pPortParam->format.video.nFrameWidth+31)&(~31));
								 pPortParam->format.video.nSliceHeight = \
							 	pPortParam->format.video.nFrameHeight =  ((pPortParam->format.video.nFrameHeight+alignh)&(~alignh));
							};							  

						}else if (!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_WMV2)){
							  pPortParam->format.video.nStride = \
							  pPortParam->format.video.nFrameWidth =   (pPortParam->format.video.nFrameWidth+31)&(~31);
							  pPortParam->format.video.nSliceHeight = \
							  pPortParam->format.video.nFrameHeight =  (pPortParam->format.video.nFrameHeight+15)&(~15);
						  
						}else if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_VP9)){
						 	  pPortParam->format.video.nStride = \
							  pPortParam->format.video.nFrameWidth =   (pPortParam->format.video.nFrameWidth+31)&(~31);
							  pPortParam->format.video.nSliceHeight = \
							  pPortParam->format.video.nFrameHeight =  (pPortParam->format.video.nFrameHeight+31)&(~31);
							 
						}else{
						 		pPortParam->format.video.nStride = \
							  pPortParam->format.video.nFrameWidth =   (pPortParam->format.video.nFrameWidth+31)&(~31);
							  pPortParam->format.video.nSliceHeight = \
							  pPortParam->format.video.nFrameHeight =  (pPortParam->format.video.nFrameHeight+15)&(~15);
						}
#endif			
				 		
				if(inPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingMJPEG)
				{
					switch(pPortParam->format.video.eColorFormat)
					{
						case VIDDEC_COLORFORMAT420:
							pPortParam->nBufferSize = pPortParam->format.video.nStride *	pPortParam->format.video.nFrameHeight * VIDDEC_FACTORFORMAT420;
							break;
						case VIDDEC_COLORFORMATSEMI420:
							pPortParam->nBufferSize = pPortParam->format.video.nStride *	pPortParam->format.video.nFrameHeight * VIDDEC_FACTORFORMATSEMI420;
							break;
						case VIDDEC_COLORFORMAT422:
							pPortParam->nBufferSize = pPortParam->format.video.nStride * 	pPortParam->format.video.nFrameHeight * VIDDEC_FACTORFORMAT422;
							break;
						case VIDDEC_COLORFORMAT400:
							pPortParam->nBufferSize = pPortParam->format.video.nStride *	pPortParam->format.video.nFrameHeight * VIDDEC_FACTORFORMAT400;
							break;
						case VIDDEC_COLORFORMAT444:
							pPortParam->nBufferSize = pPortParam->format.video.nStride *	pPortParam->format.video.nFrameHeight * VIDDEC_FACTORFORMAT444;
							break;
						default:
							eError = OMX_ErrorNoMore;
							goto EXIT;
							break;
							
					}
				}
				else
				{
					switch(pPortParam->format.video.eColorFormat)
					{
						case VIDDEC_COLORFORMAT420:
						if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC) || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC_91) \
						  || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_WMV2) || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_VP9)){
						  	pPortParam->nBufferSize =  pPortParam->format.video.nStride * pPortParam->format.video.nFrameHeight* VIDDEC_FACTORFORMAT420;

						}else{
						 		pPortParam->nBufferSize = pPortParam->format.video.nStride * 	pPortParam->format.video.nFrameHeight * VIDDEC_FACTORFORMAT420;
								pPortParam->nBufferSize += (pPortParam->format.video.nStride *	pPortParam->format.video.nFrameHeight)/4;
						}						 
							
						break;
						case VIDDEC_COLORFORMATSEMI420:
							if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC) || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC_91) \
						  || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_WMV2) || !strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_VP9)){
						  	pPortParam->nBufferSize =  pPortParam->format.video.nStride * pPortParam->format.video.nFrameHeight* VIDDEC_FACTORFORMATSEMI420;
						  	if(!strcasecmp(pComponentPrivate->componentRole.cRole, VIDDEC_COMPONENTROLES_HEVC)){
						  		if(pComponentPrivate->bHdeExsit){
						  			pPortParam->nBufferSize += (pPortParam->format.video.nStride *	pPortParam->format.video.nFrameHeight)/4;
						  		}
						  	}
						  
						  }else{
						 		pPortParam->nBufferSize = pPortParam->format.video.nStride * 	pPortParam->format.video.nFrameHeight * VIDDEC_FACTORFORMATSEMI420;
								pPortParam->nBufferSize += (pPortParam->format.video.nStride *	pPortParam->format.video.nFrameHeight)/4;

						  }		
							break;
						default:
							eError = OMX_ErrorNoMore;
							goto EXIT;
							break;
					}
				}
	    }else {
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

    		DEBUG(DEB_LEV_SIMPLE_SEQ,"In %s index %x not spt\n",__func__,nParamIndex);

        	eError = OMX_ErrorUnsupportedIndex;
        	break;
	}
EXIT:

if(eError != OMX_ErrorNone)
{
	DEBUG(DEB_LEV_SIMPLE_SEQ,"In %s Error=%x\n",__func__,eError);
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
	DEBUG(DEB_LEV_SIMPLE_SEQ,"in omx_videodec_component_GetParameter,nParamIndex:%x\n",nParamIndex);

	OMX_CONF_CHECK_CMD(hComponent, ComponentParameterStructure, OMX_TRUE);
	pComp = (OMX_COMPONENTTYPE*)hComponent;
	pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)pComp->pComponentPrivate;

	if (pComponentPrivate->state == OMX_StateInvalid) 
	{
		DEBUG(DEB_LEV_SIMPLE_SEQ,"In %s State Error=%x\n",__func__,eError);
		eError = OMX_ErrorInvalidState;
		goto EXIT;
	}
	inPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
	outPort = (omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

	switch (nParamIndex) {
	case OMX_IndexParamPortDefinition:
	{
		OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = (OMX_PARAM_PORTDEFINITIONTYPE *)ComponentParameterStructure;
		if (pPortDef->nPortIndex == VIDDEC_INPUT_PORT){		
			OMX_PARAM_PORTDEFINITIONTYPE *pInPortDef = &(inPort->sPortParam);
			memcpy(pPortDef, pInPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
		}else if (pPortDef->nPortIndex == VIDDEC_OUTPUT_PORT) {
			  OMX_PARAM_PORTDEFINITIONTYPE *pOutPortDef = &(outPort->sPortParam);
			  memcpy(pPortDef, pOutPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
    }else{
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
				case VIDDEC_DEFAULT_INPUT_INDEX_HEVC:

	        			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_HEVC;
    				inPort->sVideoParam.eCompressionFormat        	= OMX_VIDEO_CodingHEVC;
    				break;
				case VIDDEC_DEFAULT_INPUT_INDEX_HEVC_91:

	        			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_HEVC_91;
    				inPort->sVideoParam.eCompressionFormat        	= OMX_VIDEO_CodingHEVC_91;
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
				case VIDDEC_DEFAULT_INPUT_INDEX_WMV2:
        			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_WMV2;
      				inPort->sVideoParam.eCompressionFormat      	= OMX_VIDEO_CodingWMV2;
      				break;	
				case VIDDEC_DEFAULT_INPUT_INDEX_VP9:
        			inPort->sVideoParam.nIndex                        	= VIDDEC_DEFAULT_INPUT_INDEX_VP9;
      				inPort->sVideoParam.eCompressionFormat      	= OMX_VIDEO_CodingVP9;
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
					DEBUG(DEB_LEV_SIMPLE_SEQ, "Warning!VideoPortFormat is no more!%s,%d\n",__FILE__,__LINE__);
					return OMX_ErrorNoMore;
				 }    		
    		memcpy(ComponentParameterStructure, &(outPort->sVideoParam),
              			sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
      	pComponentPrivate->output_portformat_inited = OMX_TRUE;
      	pPortFormat->nIndex = index;

    	}
   		else 
   		{
   			DEBUG(DEB_LEV_SIMPLE_SEQ,"In %s port index Error=%x\n",__func__,eError);
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
          DEBUG(DEB_LEV_SIMPLE_SEQ,"get_parameter: OMX_IndexParamVideoInit %d %d\n", portParamType->nPorts, portParamType->nStartPortNumber);
 
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
		DEBUG(DEB_LEV_SIMPLE_SEQ,"In %s EXIT Error=%x\n",__func__,eError);
	}
	
	return eError;
}
OMX_ERRORTYPE VIDEC_Port_AllocateBuffer(
  omx_base_PortType *openmaxStandPort,
  OMX_BUFFERHEADERTYPE** pBuffer,
  OMX_U32 nPortIndex,
  OMX_PTR pAppPrivate,
  OMX_U32 nSizeBytes) {

  unsigned int i;
  long phy_addr;
  OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)(omxComponent->pComponentPrivate);
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
  omx_base_video_PortType *pInPort=(omx_base_video_PortType*)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
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
			if (nPortIndex== OMX_BASE_FILTER_INPUTPORT_INDEX){
				openmaxStandPort->pInternalBufferStorage[i]->pBuffer = (OMX_U8* )actal_malloc_cached_manual(nSizeBytes + sizeof(packet_header_t),&phy_addr);
				openmaxStandPort->pInternalBufferStorage[i]->pBuffer += sizeof(packet_header_t);
			}else{
				//for testOtherVP8Image.need uncache buffer
				if(pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingVP8 && pComponentPrivate->is_Thumbnail==OMX_FALSE){
					openmaxStandPort->pInternalBufferStorage[i]->pBuffer = (OMX_U8* )actal_malloc_uncache(nSizeBytes,&phy_addr);
				}else{
					openmaxStandPort->pInternalBufferStorage[i]->pBuffer = (OMX_U8* )actal_malloc_cached_manual(nSizeBytes,&phy_addr);
				}
				
			}			

     
      DEBUG(DEB_LEV_SIMPLE_SEQ,"===AllocateBuffer:%x,nSizeBytes:%x===\n",phy_addr,nSizeBytes);
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
  omx_base_PortType *pPort;
  OMX_ERRORTYPE err;

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for component %p\n", __func__, hComponent);

  if (nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts) || (nPortIndex>=NUM_OF_PORTS)) {
    DEBUG(DEB_LEV_ERR, "In %s: wrong port index\n", __func__);
    return OMX_ErrorBadPortIndex;
  }
 
  if(pComponentPrivate->bStoreMediadata[nPortIndex]){
		DEBUG(DEB_LEV_ERR,"bIsStoreMediaData Not Support here.\n");
		return OMX_ErrorUnsupportedSetting;
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
  }
  if(nPortIndex==OMX_BASE_FILTER_OUTPUTPORT_INDEX && pComponentPrivate->nNumAssignedBufferHeads==pPort->sPortParam.nBufferCountActual)
  {

  	omx_videodec_component_videodecfifoInit(pComponentPrivate);
  }
  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for component %p buffer %p\n", __func__, hComponent, ppBuffer);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE VIDEC_Port_UseBuffer(
  omx_base_PortType *openmaxStandPort,
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
  omx_base_PortType *pPort;
  OMX_ERRORTYPE err;

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for component %p\n", __func__, hComponent);
  if (nPortIndex >= (omx_base_component_Private->sPortTypesParam[OMX_PortDomainAudio].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainVideo].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts +
                     omx_base_component_Private->sPortTypesParam[OMX_PortDomainOther].nPorts) ||(nPortIndex>=NUM_OF_PORTS)) {
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
  }
  if(nPortIndex==OMX_BASE_FILTER_OUTPUTPORT_INDEX && pComponentPrivate->nNumAssignedBufferHeads==pPort->sPortParam.nBufferCountActual)
  {

  	omx_videodec_component_videodecfifoInit(pComponentPrivate);

  }

  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for component %p\n", __func__, hComponent);
  return OMX_ErrorNone;
}
OMX_ERRORTYPE omx_videodec_FlushProcessingBuffersResize(omx_base_video_PortType *openmaxStandPort) {
  omx_base_component_PrivateType* omx_base_component_Private;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate;
  OMX_BUFFERHEADERTYPE* pBuffer;
  int errQue;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for port %p\n", __func__, openmaxStandPort);
  omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandPort->standCompContainer->pComponentPrivate;
  pComponentPrivate= (VIDDEC_COMPONENT_PRIVATE*)openmaxStandPort->standCompContainer->pComponentPrivate;
  if(openmaxStandPort->sPortParam.eDomain!=OMX_PortDomainOther) { /* clock buffers not used in the clients buffer managment function */
    pthread_mutex_lock(&omx_base_component_Private->flush_mutex);
    openmaxStandPort->bIsPortFlushed=OMX_TRUE;
    /*Signal the buffer management thread of port flush,if it is waiting for buffers*/
    if(omx_base_component_Private->bMgmtSem->semval==0) {
      tsem_up(omx_base_component_Private->bMgmtSem);
    }

    if(omx_base_component_Private->state != OMX_StateExecuting ) {
      /*Waiting at paused state*/
      tsem_signal(omx_base_component_Private->bStateSem);
    }
    DEBUG(DEB_LEV_FULL_SEQ, "In %s waiting for flush all condition port index =%d\n", __func__,(int)openmaxStandPort->sPortParam.nPortIndex);
    /* Wait until flush is completed */
    pthread_mutex_unlock(&omx_base_component_Private->flush_mutex);
    tsem_down(omx_base_component_Private->flush_all_condition);
  }
   DEBUG(DEB_LEV_FUNCTION_NAME, "In %s flushed all the buffers under processing\n", __func__);

  tsem_reset(omx_base_component_Private->bMgmtSem);

  /* Flush all the buffers not under processing */
  while (openmaxStandPort->pBufferSem->semval > 0) {
    DEBUG(DEB_LEV_FULL_SEQ, "In %s TFlag=%x Flusing Port=%d,Semval=%d Qelem=%d\n",
    __func__,(int)openmaxStandPort->nTunnelFlags,(int)openmaxStandPort->sPortParam.nPortIndex,
    (int)openmaxStandPort->pBufferSem->semval,(int)openmaxStandPort->pBufferQueue->nelem);

    tsem_down(openmaxStandPort->pBufferSem);
    pBuffer = dequeue(openmaxStandPort->pBufferQueue);
    if (PORT_IS_TUNNELED(openmaxStandPort) && !PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)) {
      DEBUG(DEB_LEV_FULL_SEQ, "In %s: Comp %s is returning io:%d buffer\n",
        __func__,omx_base_component_Private->name,(int)openmaxStandPort->sPortParam.nPortIndex);
      if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
        ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->FillThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
      } else {
        ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->EmptyThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
      }
    } else if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
        errQue = queue(openmaxStandPort->pBufferQueue,pBuffer);
        if (errQue) {
      	  /* /TODO the queue is full. This can be handled in a fine way with
      	   * some retrials, or other checking. For the moment this is a critical error
      	   * and simply causes the failure of this call
      	   */
      	  return OMX_ErrorInsufficientResources;
        }
    } else {
      (*(openmaxStandPort->BufferProcessedCallback))(
        openmaxStandPort->standCompContainer,
        omx_base_component_Private->callbackData,
        pBuffer);
    }
  }
  /*Port is tunneled and supplier and didn't received all it's buffer then wait for the buffers*/
  if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
    while(openmaxStandPort->pBufferQueue->nelem!= openmaxStandPort->nNumAssignedBuffers){
      tsem_down(openmaxStandPort->pBufferSem);
      DEBUG(DEB_LEV_PARAMS, "In %s Got a buffer qelem=%d\n",__func__,openmaxStandPort->pBufferQueue->nelem);
    }
    tsem_reset(openmaxStandPort->pBufferSem);
  }

  pthread_mutex_lock(&omx_base_component_Private->flush_mutex);
  openmaxStandPort->bIsPortFlushed=OMX_FALSE;
  pthread_mutex_unlock(&omx_base_component_Private->flush_mutex);
  if(pComponentPrivate->is_videodec_fifo_init==OMX_TRUE){
  	raw_fifo_timeout_wakeup(pComponentPrivate->vout,&(pComponentPrivate->suspend_flag));
  	pComponentPrivate->vout->fifo_reset(pComponentPrivate->vout);
  }
  if(pComponentPrivate->bBufferNotEngouh!=OMX_TRUE){
  	resetNodeList(pComponentPrivate->headNode);
  }
  
  pComponentPrivate->bComponentFlush =OMX_TRUE;
  tsem_up(omx_base_component_Private->flush_condition);

  DEBUG(DEB_LEV_FULL_SEQ, "Out %s Port Index=%d bIsPortFlushed=%d Component %s\n", __func__,
    (int)openmaxStandPort->sPortParam.nPortIndex,(int)openmaxStandPort->bIsPortFlushed,omx_base_component_Private->name);

  DEBUG(DEB_LEV_PARAMS, "In %s TFlag=%x Qelem=%d BSem=%d bMgmtsem=%d component=%s\n", __func__,
    (int)openmaxStandPort->nTunnelFlags,
    (int)openmaxStandPort->pBufferQueue->nelem,
    (int)openmaxStandPort->pBufferSem->semval,
    (int)omx_base_component_Private->bMgmtSem->semval,
    omx_base_component_Private->name);

  DEBUG(DEB_LEV_FUNCTION_NAME, "Out %s Port %p Index=%d\n", __func__, openmaxStandPort, (int)openmaxStandPort->sPortParam.nPortIndex);
  return OMX_ErrorNone;
}
OMX_ERRORTYPE omx_videodec_FlushProcessingBuffers(omx_base_video_PortType *openmaxStandPort) {
  omx_base_component_PrivateType* omx_base_component_Private;
  OMX_BUFFERHEADERTYPE* pBuffer;
  OMX_BUFFERHEADERTYPE* pBuffHead;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate;
  int errQue;
  int i;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for port %p\n", __func__, openmaxStandPort);
  omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandPort->standCompContainer->pComponentPrivate;
  pComponentPrivate= (VIDDEC_COMPONENT_PRIVATE*)openmaxStandPort->standCompContainer->pComponentPrivate;
  
 if(pComponentPrivate->is_fifo_disposing==OMX_TRUE){
 	  return OMX_ErrorNone;
 }
  if(openmaxStandPort->sPortParam.eDomain!=OMX_PortDomainOther) { /* clock buffers not used in the clients buffer managment function */
    pthread_mutex_lock(&omx_base_component_Private->flush_mutex);
    openmaxStandPort->bIsPortFlushed=OMX_TRUE;
    /*Signal the buffer management thread of port flush,if it is waiting for buffers*/
    if(omx_base_component_Private->bMgmtSem->semval==0) {
      tsem_up(omx_base_component_Private->bMgmtSem);
    }

    if(omx_base_component_Private->state != OMX_StateExecuting ) {
      /*Waiting at paused state*/
      tsem_signal(omx_base_component_Private->bStateSem);
    }
    DEBUG(DEB_LEV_FULL_SEQ, "In %s waiting for flush all condition port index =%d\n", __func__,(int)openmaxStandPort->sPortParam.nPortIndex);
    /* Wait until flush is completed */
    pthread_mutex_unlock(&omx_base_component_Private->flush_mutex);
    tsem_down(omx_base_component_Private->flush_all_condition);
  }
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s flushed all the buffers under processing\n", __func__);
  tsem_reset(omx_base_component_Private->bMgmtSem);

  /* Flush all the buffers not under processing */
  while (openmaxStandPort->pBufferSem->semval > 0) {
    DEBUG(DEB_LEV_FULL_SEQ, "In %s TFlag=%x Flusing Port=%d,Semval=%d Qelem=%d\n",
    __func__,(int)openmaxStandPort->nTunnelFlags,(int)openmaxStandPort->sPortParam.nPortIndex,
    (int)openmaxStandPort->pBufferSem->semval,(int)openmaxStandPort->pBufferQueue->nelem);

    tsem_down(openmaxStandPort->pBufferSem);
    pBuffer = dequeue(openmaxStandPort->pBufferQueue);
    if (PORT_IS_TUNNELED(openmaxStandPort) && !PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)) {
      DEBUG(DEB_LEV_FULL_SEQ, "In %s: Comp %s is returning io:%d buffer\n",
        __func__,omx_base_component_Private->name,(int)openmaxStandPort->sPortParam.nPortIndex);
      if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
        ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->FillThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
      } else {
        ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->EmptyThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
      }
    } else if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
        errQue = queue(openmaxStandPort->pBufferQueue,pBuffer);
        if (errQue) {
      	  /* /TODO the queue is full. This can be handled in a fine way with
      	   * some retrials, or other checking. For the moment this is a critical error
      	   * and simply causes the failure of this call
      	   */
      	  return OMX_ErrorInsufficientResources;
        }
    } else {
    	pBuffer->nFilledLen=0;
    }
  }
  for(i = 0;i<openmaxStandPort->sPortParam.nBufferCountActual;i++) {
  	  if(openmaxStandPort->bBufferStateAllocated[i]==BUFFER_FREE){
  	  	continue;
  	  }  	  
			pBuffHead=pComponentPrivate->BufferHeadStorage[i];
		  pBuffHead->nFilledLen = 0;
			pBuffHead->nOffset = 0;	
			if(pComponentPrivate->vout->is_fifo_bequeued(pComponentPrivate->vout,pBuffHead->pAppPrivate)==1){
			pBuffHead->nFlags |= OMX_BUFFERFLAG_DECODEONLY;
      (*(openmaxStandPort->BufferProcessedCallback))(
        openmaxStandPort->standCompContainer,
        omx_base_component_Private->callbackData,
        pBuffHead);
      }				
	}
  /*Port is tunneled and supplier and didn't received all it's buffer then wait for the buffers*/
  if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
    while(openmaxStandPort->pBufferQueue->nelem!= openmaxStandPort->nNumAssignedBuffers){
      tsem_down(openmaxStandPort->pBufferSem);
      DEBUG(DEB_LEV_PARAMS, "In %s Got a buffer qelem=%d\n",__func__,openmaxStandPort->pBufferQueue->nelem);
    }
    tsem_reset(openmaxStandPort->pBufferSem);
  }

  pthread_mutex_lock(&omx_base_component_Private->flush_mutex);
  openmaxStandPort->bIsPortFlushed=OMX_FALSE;
  pthread_mutex_unlock(&omx_base_component_Private->flush_mutex);
  
  //reset decoder here
  if(pComponentPrivate->is_videodec_fifo_init==OMX_TRUE){
  	raw_fifo_timeout_wakeup(pComponentPrivate->vout,&(pComponentPrivate->suspend_flag));
    pComponentPrivate->vout->fifo_reset(pComponentPrivate->vout);
  }
  if(pComponentPrivate->bBufferNotEngouh!=OMX_TRUE){
  	resetNodeList(pComponentPrivate->headNode);
  }
  pComponentPrivate->bComponentFlush =OMX_TRUE;
  tsem_up(omx_base_component_Private->flush_condition);

  DEBUG(DEB_LEV_FULL_SEQ, "Out %s Port Index=%d bIsPortFlushed=%d Component %s\n", __func__,
    (int)openmaxStandPort->sPortParam.nPortIndex,(int)openmaxStandPort->bIsPortFlushed,omx_base_component_Private->name);

  DEBUG(DEB_LEV_PARAMS, "In %s TFlag=%x Qelem=%d BSem=%d bMgmtsem=%d component=%s\n", __func__,
    (int)openmaxStandPort->nTunnelFlags,
    (int)openmaxStandPort->pBufferQueue->nelem,
    (int)openmaxStandPort->pBufferSem->semval,
    (int)omx_base_component_Private->bMgmtSem->semval,
    omx_base_component_Private->name);

  DEBUG(DEB_LEV_FUNCTION_NAME, "Out %s Port %p Index=%d\n", __func__, openmaxStandPort, (int)openmaxStandPort->sPortParam.nPortIndex);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_videodec_FlushBeforeDisableOutPort(omx_base_video_PortType *openmaxStandPort) {
  omx_base_component_PrivateType* omx_base_component_Private;
  OMX_BUFFERHEADERTYPE* pBuffer;
  OMX_BUFFERHEADERTYPE* pBuffHead;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate;
  int errQue;
  int i;

	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s for port %p\n", __func__, openmaxStandPort);
  omx_base_component_Private = (omx_base_component_PrivateType*)openmaxStandPort->standCompContainer->pComponentPrivate;
  pComponentPrivate= (VIDDEC_COMPONENT_PRIVATE*)openmaxStandPort->standCompContainer->pComponentPrivate;
  
  if(pComponentPrivate->is_fifo_disposing==OMX_TRUE){
 	  return OMX_ErrorNone;
  }
  /* Flush all the buffers not under processing */
  while (openmaxStandPort->pBufferSem->semval > 0) {
    DEBUG(DEB_LEV_FULL_SEQ, "In %s TFlag=%x Flusing Port=%d,Semval=%d Qelem=%d\n",
    __func__,(int)openmaxStandPort->nTunnelFlags,(int)openmaxStandPort->sPortParam.nPortIndex,
    (int)openmaxStandPort->pBufferSem->semval,(int)openmaxStandPort->pBufferQueue->nelem);

    tsem_down(openmaxStandPort->pBufferSem);
    pBuffer = dequeue(openmaxStandPort->pBufferQueue);
    if (PORT_IS_TUNNELED(openmaxStandPort) && !PORT_IS_BUFFER_SUPPLIER(openmaxStandPort)) {
      DEBUG(DEB_LEV_FULL_SEQ, "In %s: Comp %s is returning io:%d buffer\n",
        __func__,omx_base_component_Private->name,(int)openmaxStandPort->sPortParam.nPortIndex);
      if (openmaxStandPort->sPortParam.eDir == OMX_DirInput) {
        ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->FillThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
      } else {
        ((OMX_COMPONENTTYPE*)(openmaxStandPort->hTunneledComponent))->EmptyThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
      }
    } else if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
        errQue = queue(openmaxStandPort->pBufferQueue,pBuffer);
        if (errQue) {
      	  /* /TODO the queue is full. This can be handled in a fine way with
      	   * some retrials, or other checking. For the moment this is a critical error
      	   * and simply causes the failure of this call
      	   */
      	  return OMX_ErrorInsufficientResources;
        }
    } else {
    	if(pComponentPrivate->bResizeEnabled==OMX_FALSE){
    		pBuffer->nFilledLen=0;
    	}else{
    		pBuffer->nFlags |= OMX_BUFFERFLAG_DECODEONLY;
    		pBuffer->nFilledLen=0;
    		(*(openmaxStandPort->BufferProcessedCallback))(
        openmaxStandPort->standCompContainer,
        omx_base_component_Private->callbackData,
        pBuffer);
    	}
    	
    }
  }
  if(pComponentPrivate->bResizeEnabled==OMX_TRUE){
  	return OMX_ErrorNone;
  }
  for(i = 0;i<openmaxStandPort->sPortParam.nBufferCountActual;i++) {
  	 if(openmaxStandPort->bBufferStateAllocated[i]==BUFFER_FREE){
  	  	continue;
  	  } 
			pBuffHead=pComponentPrivate->BufferHeadStorage[i];
		  pBuffHead->nFilledLen = 0;
			pBuffHead->nOffset = 0;	

			if(pComponentPrivate->vout->is_fifo_bequeued(pComponentPrivate->vout,pBuffHead->pAppPrivate)==1){
			pBuffHead->nFlags |= OMX_BUFFERFLAG_DECODEONLY;
      (*(openmaxStandPort->BufferProcessedCallback))(
        openmaxStandPort->standCompContainer,
        omx_base_component_Private->callbackData,
        pBuffHead);
      }				
	}
  /*Port is tunneled and supplier and didn't received all it's buffer then wait for the buffers*/
  if (PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort)) {
    while(openmaxStandPort->pBufferQueue->nelem!= openmaxStandPort->nNumAssignedBuffers){
      tsem_down(openmaxStandPort->pBufferSem);
      DEBUG(DEB_LEV_PARAMS, "In %s Got a buffer qelem=%d\n",__func__,openmaxStandPort->pBufferQueue->nelem);
    }
    tsem_reset(openmaxStandPort->pBufferSem);
  }
   
  DEBUG(DEB_LEV_FUNCTION_NAME, "Out %s Port %p Index=%d\n", __func__, openmaxStandPort, (int)openmaxStandPort->sPortParam.nPortIndex);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE omx_videodec_component_MessageHandler(OMX_COMPONENTTYPE* openmaxStandComp,internalRequestMessageType *message) {
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)openmaxStandComp->pComponentPrivate;
  omx_base_video_PortType *pInPort=(omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
  omx_base_video_PortType *pOutPort=(omx_base_video_PortType *)pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
	OMX_PARAM_PORTDEFINITIONTYPE *pPortDef = &(pOutPort->sPortParam);
  OMX_ERRORTYPE err;
  OMX_U32 j;
  OMX_STATETYPE eCurrentState = pComponentPrivate->state;

  DEBUG(DEB_LEV_ERR,"In %s,messageType is %d,messageParam is %d\n", __func__,message->messageType,message->messageParam);
  switch(message->messageType){
  	case OMX_CommandStateSet:{

     if ((message->messageParam == OMX_StateIdle ) && (pComponentPrivate->state == OMX_StateLoaded)) {
      		err = omx_videodec_component_Init(openmaxStandComp);
      		if(err!=OMX_ErrorNone) { 
        	DEBUG(DEB_LEV_SIMPLE_SEQ,"In %s Video Decoder Init Failed Error=%x\n",__func__,err); 
        	return err;
      		} 
 
      		omx_base_component_MessageHandler(openmaxStandComp,message);
      
   	 } else if ((message->messageParam == OMX_StateLoaded) && (pComponentPrivate->state == OMX_StateIdle)) {
      		omx_base_component_MessageHandler(openmaxStandComp,message);
      		DEBUG(DEB_LEV_ERR,"component loaded \n");
      		err = omx_videodec_component_Deinit(openmaxStandComp);
      		if(err!=OMX_ErrorNone) { 
        		DEBUG(DEB_LEV_SIMPLE_SEQ,"In %s Video Decoder Deinit Failed Error=%x\n",__func__,err); 
       	 		return err;
      		} 
      		if(pComponentPrivate->fb_port != NULL) {
      			DEBUG(DEB_LEV_ERR,"===fb disposeing,line %d===\n",__LINE__);
						fb_fifo_dispose(pComponentPrivate->fb_port,pComponentPrivate->IsGraphicBuffer);
						pComponentPrivate->fb_port=NULL;
						pComponentPrivate->is_videodec_fifo_init = OMX_FALSE;
					}
					if(pComponentPrivate->vce_handle!=NULL){
						VceReSize_Close(pComponentPrivate->vce_handle);
						pComponentPrivate->vce_handle=NULL;
					}
   	 } else if ((message->messageParam == OMX_StateIdle  ) && (eCurrentState == OMX_StateExecuting)){
   	 	    

   		 	 	raw_fifo_timeout_wakeup(pComponentPrivate->vout,&(pComponentPrivate->suspend_flag));
   		 	 	if(PORT_IS_ENABLED(pInPort)) {
   		 	 		pComponentPrivate->is_inputbuffer_flushing_pending = 1;
   		 	 		DEBUG(DEB_LEV_ERR,"========input flushinging start at stop==========\n");
   		 	 		pInPort->FlushProcessingBuffers(pInPort);
   		 	 		DEBUG(DEB_LEV_ERR,"========input flushinging end at stop==========\n");
   		 	 		pComponentPrivate->is_inputbuffer_flushing_pending = 0;
          }   		 	 	
   		 	 	if(PORT_IS_ENABLED(pOutPort)) {
   		 	 		pComponentPrivate->is_outputbuffer_flushing_pending = 1;
   		 	 		DEBUG(DEB_LEV_ERR,"========output flushinging start at stop==========\n");
   		 	 		if(pComponentPrivate->bResizeEnabled==OMX_TRUE){
   		 	 			omx_videodec_FlushProcessingBuffersResize(pOutPort);
   		 	 		}else{
   		 	 			omx_videodec_FlushProcessingBuffers(pOutPort);
   		 	 		}
            
            DEBUG(DEB_LEV_ERR,"========output flushinging end at stop==========\n");
            pComponentPrivate->is_outputbuffer_flushing_pending = 0 ;
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
  		pComponentPrivate->bIsEOSReached = OMX_FALSE;
  		pComponentPrivate->is_end_of_stream = OMX_FALSE;
  		if(message->messageParam == OMX_ALL) {
  	
       pInPort->bIsPortFlushed = OMX_TRUE;
       pOutPort->bIsPortFlushed = OMX_TRUE;
       /*---flush inputport---*/
       DEBUG(DEB_LEV_ERR,"flush all input start ,%d\n",__LINE__);
       pComponentPrivate->is_inputbuffer_flushing_pending = 1;
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
       pComponentPrivate->is_inputbuffer_flushing_pending = 0;
       DEBUG(DEB_LEV_ERR,"flush all input end ,%d\n",__LINE__);
       
       /*---flush outputport---*/
       DEBUG(DEB_LEV_ERR,"flush all output start ,%d\n",__LINE__);
       pComponentPrivate->is_outputbuffer_flushing_pending = 1;
       pComponentPrivate->is_cur_stream_buf = OMX_FALSE;
       if(pComponentPrivate->bResizeEnabled==OMX_TRUE){      	
       	err = omx_videodec_FlushProcessingBuffersResize(pOutPort);
       }else{
       	err =  omx_videodec_FlushProcessingBuffers(pOutPort);
       }       
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
  		pComponentPrivate->is_outputbuffer_flushing_pending = 0;   
      DEBUG(DEB_LEV_ERR,"flush all output end ,%d\n",__LINE__);

    	} else if ( message->messageParam==OMX_BASE_FILTER_OUTPUTPORT_INDEX ){
  		
  			pComponentPrivate->is_outputbuffer_flushing_pending = 1;
  		
				pComponentPrivate->is_cur_stream_buf = OMX_FALSE;
				if(pComponentPrivate->bResizeEnabled==OMX_TRUE){				
					err = omx_videodec_FlushProcessingBuffersResize(pOutPort);
				}else{
					err =  omx_videodec_FlushProcessingBuffers(pOutPort);
				}
				
  			if(err == OMX_ErrorNone){
  				(*(pComponentPrivate->callbacks->EventHandler))
        	(openmaxStandComp,
        	pComponentPrivate->callbackData,
        	OMX_EventCmdComplete, /* The command was completed */
        	OMX_CommandFlush, /* The commands was a OMX_CommandStateSet */
        	message->messageParam, /* The state has been changed in message->messageParam */
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
  			pComponentPrivate->is_outputbuffer_flushing_pending = 0;
  		} else{
  			 pComponentPrivate->is_inputbuffer_flushing_pending =1;
  			 err =  omx_base_component_MessageHandler(openmaxStandComp,message);
  			 pComponentPrivate->is_inputbuffer_flushing_pending =0;
  		}
  	}
  	break;
  	case OMX_CommandPortDisable:{
  		
  		if ( message->messageParam==OMX_BASE_FILTER_OUTPUTPORT_INDEX ){
  			DEBUG(DEB_LEV_ERR,"output port disabled start \n");
  			if(pComponentPrivate->state!=OMX_StateLoaded && pComponentPrivate->is_fifo_disposing==OMX_FALSE) {
  				
  				pComponentPrivate->is_outputbuffer_flushing_pending = 1;
  				if(pComponentPrivate->bResizeEnabled==OMX_TRUE){
  					  omx_videodec_FlushProcessingBuffersResize(pOutPort);
   		 	 	}else{
   		 	 			omx_videodec_FlushProcessingBuffers(pOutPort);
   		 	 	}
   		 	 
        	pComponentPrivate->is_outputbuffer_flushing_pending = 0;
      	}
      	
      	if ((pComponentPrivate->state == OMX_StateIdle ||
        	pComponentPrivate->state == OMX_StatePause  ||
        	pComponentPrivate->state == OMX_StateExecuting) &&
        	(pComponentPrivate->IsPortBeingDisabled==OMX_TRUE)) {
        	if(pOutPort->pInternalBufferStorage && pComponentPrivate->nNewBufferCountActual>pPortDef->nBufferCountActual ) {
        		DEBUG(DEB_LEV_ERR,"==realloc pInternalBufferStorage:%d==\n",pComponentPrivate->nNewBufferCountActual);
          	pOutPort->pInternalBufferStorage = realloc(pOutPort->pInternalBufferStorage,pComponentPrivate->nNewBufferCountActual*sizeof(OMX_BUFFERHEADERTYPE *));
        	}
        	if(pOutPort->bBufferStateAllocated && pComponentPrivate->nNewBufferCountActual>pPortDef->nBufferCountActual) {
        		DEBUG(DEB_LEV_ERR,"==realloc bBufferStateAllocated:%d==\n",pComponentPrivate->nNewBufferCountActual);
          	pOutPort->bBufferStateAllocated = realloc(pOutPort->bBufferStateAllocated,pComponentPrivate->nNewBufferCountActual*sizeof(BUFFER_STATUS_FLAG));
          	for(j=pPortDef->nBufferCountActual; j < pComponentPrivate->nNewBufferCountActual; j++) {
          		pOutPort->bBufferStateAllocated[j] = BUFFER_FREE;
          	}
        	}
        	if(pComponentPrivate->bBufferNotEngouh){
        		pComponentPrivate->bBufferNotEngouh = OMX_FALSE;
        	}
        
        	pComponentPrivate->IsPortBeingDisabled=OMX_FALSE;
        	if(pComponentPrivate->PortDisableSem->semval==0){
        		DEBUG(DEB_LEV_ERR,"+++++++tsem_up PortDisableSem+++++\n");
        		tsem_up(pComponentPrivate->PortDisableSem);
        	}
        }
        DEBUG(DEB_LEV_ERR,"output port disabled end \n");
       pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX]->Port_DisablePort(pComponentPrivate->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX]);     
     	(*(pComponentPrivate->callbacks->EventHandler))
        (openmaxStandComp,
        pComponentPrivate->callbackData,
        OMX_EventCmdComplete, /* The command was completed */
        OMX_CommandPortDisable, /* The commands was a OMX_CommandStateSet */
        message->messageParam, /* The state has been changed in message->messageParam */
        NULL);
  		}else{
  			pComponentPrivate->is_inputbuffer_flushing_pending = 1;
  			err =  omx_base_component_MessageHandler(openmaxStandComp,message);
  			pComponentPrivate->is_inputbuffer_flushing_pending = 0;
  			
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
  omx_base_PortType *openmaxStandPort,
  OMX_U32 nPortIndex,
  OMX_BUFFERHEADERTYPE* pBuffer) {

  OMX_S32 i;
  OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)(omxComponent->pComponentPrivate);
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)omxComponent->pComponentPrivate;
  omx_base_video_PortType *pInPort=(omx_base_video_PortType*)pComponentPrivate->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
  DEBUG(DEB_LEV_FULL_SEQ, "In %s for port %p\n", __func__, openmaxStandPort);
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
      		if (nPortIndex== OMX_BASE_FILTER_INPUTPORT_INDEX){
      			actal_free_cached_manual(openmaxStandPort->pInternalBufferStorage[i]->pBuffer - sizeof(packet_header_t));
					}else{
						if(pInPort->sVideoParam.eCompressionFormat == OMX_VIDEO_CodingVP8 && pComponentPrivate->is_Thumbnail==OMX_FALSE){
							actal_free_uncache(openmaxStandPort->pInternalBufferStorage[i]->pBuffer);
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
      	DEBUG(DEB_LEV_ERR, "nNumAssignedBuffers free normal,nPortIndex is %d \n",nPortIndex);
        openmaxStandPort->sPortParam.bPopulated = OMX_FALSE;
        openmaxStandPort->bIsEmptyOfBuffers = OMX_TRUE;
        pComponentPrivate->bResizeEnabled=OMX_FALSE;
        tsem_up(openmaxStandPort->pAllocSem);
        if(nPortIndex==OMX_BASE_FILTER_OUTPUTPORT_INDEX)
        {
        	tsem_init(pComponentPrivate->PortDisableSem, 0);
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
  OMX_COMPONENTTYPE *openmaxStandComp = (OMX_COMPONENTTYPE *)hComponent;
  omx_base_component_PrivateType* omx_base_component_Private = (omx_base_component_PrivateType*)((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate;
  VIDDEC_COMPONENT_PRIVATE* pComponentPrivate = (VIDDEC_COMPONENT_PRIVATE*)omx_base_component_Private;
  omx_base_PortType *pPort;
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
  if(pComponentPrivate->IsPortBufferRealloc==OMX_FALSE){
  	err = omx_videodec_component_Deinit(openmaxStandComp);
  	if(err!=OMX_ErrorNone) { 
  		DEBUG(DEB_LEV_SIMPLE_SEQ,"In %s Video Decoder Deinit Failed Error=%x\n",__func__,err); 
  		return err;
  	} 
  }
  

  if(pComponentPrivate->IsPortBeingDisabled==OMX_TRUE && nPortIndex==OMX_BASE_FILTER_OUTPUTPORT_INDEX){
  	DEBUG(DEB_LEV_ERR,"++++waitPortDisableSem+++++\n");
  	err = tsem_timed_down(pComponentPrivate->PortDisableSem,5000);
  	if(err != OMX_ErrorNone){
  		DEBUG(DEB_LEV_ERR,"+++++tsem_timed_down PortDisableSem timetout+++++\n");
  		pComponentPrivate->IsPortBeingDisabled=OMX_FALSE; 
  	}
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

	DEBUG(DEB_LEV_SIMPLE_SEQ,"Entry ComponentRoleEnum,Getting configuration %i\n", nIndex);
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
		DEBUG(DEB_LEV_SIMPLE_SEQ,"In %s EXIT Error=%x\n",__func__,eError);
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
  DEBUG(DEB_LEV_SIMPLE_SEQ,"   Getting configuration %i\n", nIndex);
  
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
		DEBUG(DEB_LEV_SIMPLE_SEQ,"Get Extension Index in Invalid State\n");
		return OMX_ErrorInvalidState;
	}
	else if(!strncmp(cParameterName,"OMX.google.android.index.enableAndroidNativeBuffers", sizeof("OMX.google.android.index.enableAndroidNativeBuffers")-1)) {
		*pIndexType = (OMX_INDEXTYPE)OMX_GoogleAndroidIndexEnableAndroidNativeBuffers;
		 return OMX_ErrorNone;
	}
	else if(!strncmp(cParameterName,"OMX.google.android.index.useAndroidNativeBuffer2", sizeof("OMX.google.android.index.useAndroidNativeBuffer2")-1)) {
	
		pComponentPrivate->IsGraphicBuffer=OMX_TRUE;
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
	}else if(!strncmp(cParameterName,"OMX.google.android.index.storeMetaDataInBuffers", sizeof("OMX.google.android.index.storeMetaDataInBuffers")-1)){
			*pIndexType = (OMX_INDEXTYPE)OMX_IndexParameterStoreMediaData;
			 return OMX_ErrorNone;	
	
	}
	else {
		DEBUG(DEB_LEV_SIMPLE_SEQ,"Extension: %s not implemented\n", cParameterName);
		return OMX_ErrorNotImplemented;
	}
  
  return OMX_ErrorNoMore;
}

