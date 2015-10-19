/**
  @file src/components/jpeg/omx_jpegdec_component.c

  This component implements an JPEG decoder based on Tom Lane's jpeg library (http://www.ijg.org/files/)

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

  $Date: 2008-09-17 10:11:10 +0200 (Wed, 17 Sep 2008) $
  Revision $Rev: 622 $
  Author $Author: gsent $

*/

#include <omxcore.h>
#include <omx_base_image_port.h>

#include <ctype.h>    /* to declare isprint() */
#include "omx_jpegdec_component.h"
#include "OMX_Image.h"
#include "errno.h"
#include <utils/Log.h>


/** Maximum Number of Image Mad Decoder Component Instance*/
#define MAX_COMPONENT_JPEGDEC 4

/** Number of Mad Component Instance*/
//static OMX_U32 nojpegdecInstance=0;

/* Create the add-on message string table. */

static void IMGDEC_CloseImagePlugin (omx_jpegdec_component_PrivateType *p)
{
	if (p->p_imgPluginInfo) {
		OMX_ActImageDecoder_Close(p);
		dlclose(p->p_so_handle);
		p->p_so_handle = NULL;
		p->p_imgPluginInfo = NULL;
	}

	return;
}

typedef void*(*FuncPtr)(void);
static OMX_ERRORTYPE IMGDEC_GetImagePlugin (omx_jpegdec_component_PrivateType *p, const char* std)
{
    FuncPtr func_handle;
    char libname[12] = "id_";
    strcat(libname, std);
    strcat(libname, ".so");
    p->p_so_handle = dlopen(libname, RTLD_NOW);
    if (p->p_so_handle == NULL){
        return OMX_ErrorBadParameter;
    }
    func_handle = (FuncPtr)dlsym(p->p_so_handle , "get_plugin_info");
	if(func_handle == NULL) {
		return OMX_ErrorBadParameter;
	}
    p->p_imgPluginInfo = (void*)func_handle();
    if(p->p_imgPluginInfo == NULL) {
        dlclose(p->p_so_handle);
        p->p_so_handle = NULL;
        return OMX_ErrorBadParameter;
    }
    int rt = OMX_ActImageDecoder_Open(p);
    if(rt != 0) {
    	return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNone;
}


OMX_ERRORTYPE OMX_ComponentInit (OMX_HANDLETYPE hComponent)
{
  OMX_ERRORTYPE err = OMX_ErrorNone;  
  OMX_COMPONENTTYPE *pHandle = NULL;
  char std_string[10]; 
  omx_jpegdec_component_PrivateType* omx_jpegdec_component_Private = NULL;
  OMX_STRING cComponentName = "OMX.Action.Image.Decoder";
  omx_base_image_PortType *pInPort,*pOutPort;
  OMX_U32 i;
	pHandle = (OMX_COMPONENTTYPE *)hComponent;
	pHandle->pComponentPrivate = calloc(1, sizeof(omx_jpegdec_component_PrivateType));

    if(pHandle->pComponentPrivate==NULL)  {
      return OMX_ErrorInsufficientResources;
    }
 
  
  omx_jpegdec_component_Private = (omx_jpegdec_component_PrivateType *)pHandle->pComponentPrivate;
  omx_jpegdec_component_Private->ports = NULL;

  /** we could create our own port structures here
    * fixme maybe the base class could use a "port factory" function pointer?  
    */
  err = omx_base_filter_Constructor(pHandle, cComponentName);
 	if (err != OMX_ErrorNone) {
	  return err;
  }

  /** Domain specific section for the ports. */  
  /** first we set the parameter common to both formats
    * parameters related to input port which does not depend upon input image format
    */
  omx_jpegdec_component_Private->sPortTypesParam[OMX_PortDomainImage].nStartPortNumber = 0;
  omx_jpegdec_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts = 2;

  /** Allocate Ports and call port constructor. */  
  if (omx_jpegdec_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts && !omx_jpegdec_component_Private->ports) {
    omx_jpegdec_component_Private->ports = calloc(omx_jpegdec_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts, sizeof(omx_base_PortType *));
    if (!omx_jpegdec_component_Private->ports) {
      return OMX_ErrorInsufficientResources;
    }
    for (i=0; i < omx_jpegdec_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts; i++) {
      omx_jpegdec_component_Private->ports[i] = calloc(1, sizeof(omx_base_image_PortType));
      if (!omx_jpegdec_component_Private->ports[i]) {
        return OMX_ErrorInsufficientResources;
      }
    }
  }

  err = base_image_port_Constructor(pHandle, &omx_jpegdec_component_Private->ports[0], 0, OMX_TRUE);
  if (err != OMX_ErrorNone) 
  {
  	return err;
  }
  err = base_image_port_Constructor(pHandle, &omx_jpegdec_component_Private->ports[1], 1, OMX_FALSE);
	if (err != OMX_ErrorNone) 
  	{
  		return err;
  	}

    pInPort = (omx_base_image_PortType *) omx_jpegdec_component_Private->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
    pInPort->sPortParam.nBufferSize = IN_BUFFER_SIZE;
    pInPort->sPortParam.nBufferCountActual = 1;
  /** parameters related to output port */
    pOutPort = (omx_base_image_PortType *) omx_jpegdec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
    pOutPort->sPortParam.nBufferCountActual = 1;
    pOutPort->sPortParam.nBufferSize = OUT_BUFFER_SIZE;

  /** initialise the semaphore to be used for mad decoder access synchronization */
  if(!omx_jpegdec_component_Private->jpegdecSyncSem) {
    omx_jpegdec_component_Private->jpegdecSyncSem = calloc(1,sizeof(tsem_t));
    if(omx_jpegdec_component_Private->jpegdecSyncSem == NULL) {
      return OMX_ErrorInsufficientResources;
    }
    tsem_init(omx_jpegdec_component_Private->jpegdecSyncSem, 0);
  }

  if(!omx_jpegdec_component_Private->jpegdecSyncSem1) {
    omx_jpegdec_component_Private->jpegdecSyncSem1 = calloc(1,sizeof(tsem_t));
    if(omx_jpegdec_component_Private->jpegdecSyncSem1 == NULL) {
      return OMX_ErrorInsufficientResources;
    }
    tsem_init(omx_jpegdec_component_Private->jpegdecSyncSem1, 0);
  }

  /** general configuration irrespective of any image formats
    *  setting values of other fields of omx_jpegdec_component_Private structure  
    */ 
  omx_jpegdec_component_Private->jpegdecReady = OMX_FALSE;
  omx_jpegdec_component_Private->hMarkTargetComponent = NULL;
  omx_jpegdec_component_Private->nFlags = 0x0;
  omx_jpegdec_component_Private->BufferMgmtCallback = omx_jpegdec_component_BufferMgmtCallback;
//  omx_jpegdec_component_Private->BufferMgmtFunction = omx_jpegdec_component_BufferMgmtFunction;
  omx_jpegdec_component_Private->messageHandler = omx_jpegdec_decoder_MessageHandler;
  omx_jpegdec_component_Private->destructor = omx_jpegdec_component_Destructor;
//  pHandle->SetParameter = omx_jpegdec_component_SetParameter;
//  pHandle->GetParameter = omx_jpegdec_component_GetParameter;

  strcpy(std_string, "jpg");
  err = IMGDEC_GetImagePlugin(omx_jpegdec_component_Private, std_string);
//  nojpegdecInstance++;

//  if(nojpegdecInstance>MAX_COMPONENT_JPEGDEC)
//    return OMX_ErrorInsufficientResources;

  return err;
	
}


/** The destructor */
OMX_ERRORTYPE omx_jpegdec_component_Destructor(OMX_COMPONENTTYPE *openmaxStandComp) {

  omx_jpegdec_component_PrivateType* omx_jpegdec_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_U32 i;
  if(omx_jpegdec_component_Private->jpegdecSyncSem) {
    tsem_deinit(omx_jpegdec_component_Private->jpegdecSyncSem);
    free(omx_jpegdec_component_Private->jpegdecSyncSem);
    omx_jpegdec_component_Private->jpegdecSyncSem = NULL;
  }
  
  if(omx_jpegdec_component_Private->jpegdecSyncSem1) {
    tsem_deinit(omx_jpegdec_component_Private->jpegdecSyncSem1);
    free(omx_jpegdec_component_Private->jpegdecSyncSem1);
    omx_jpegdec_component_Private->jpegdecSyncSem1 = NULL;
  }

  /* frees port/s */
  if (omx_jpegdec_component_Private->ports) {
    for (i=0; i < omx_jpegdec_component_Private->sPortTypesParam[OMX_PortDomainImage].nPorts; i++) {
      if(omx_jpegdec_component_Private->ports[i]) {
        omx_jpegdec_component_Private->ports[i]->PortDestructor(omx_jpegdec_component_Private->ports[i]);
      }
    }
    free(omx_jpegdec_component_Private->ports);
    omx_jpegdec_component_Private->ports=NULL;
  }

  DEBUG(DEB_LEV_FUNCTION_NAME, "Destructor of mad decoder component is called\n");
  omx_base_filter_Destructor(openmaxStandComp);
//  nojpegdecInstance--;

  return OMX_ErrorNone;

}

/** The Initialization function  */
OMX_ERRORTYPE omx_jpegdec_component_Init(OMX_COMPONENTTYPE *openmaxStandComp)  {

  omx_jpegdec_component_PrivateType* omx_jpegdec_component_Private = openmaxStandComp->pComponentPrivate;
  OMX_ERRORTYPE err = OMX_ErrorNone;
//ACT_OMX_LOG("entry:%s \n",__FUNCTION__);
	omx_jpegdec_component_Private->isFirstBuffer = 1;
  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s \n",__func__);
  return err;
}

/** The Deinitialization function  */
OMX_ERRORTYPE omx_jpegdec_component_Deinit(OMX_COMPONENTTYPE *openmaxStandComp) {
  OMX_ERRORTYPE err = OMX_ErrorNone;
  OMX_ActImageDecoder_Close((omx_jpegdec_component_PrivateType *)openmaxStandComp->pComponentPrivate);
   
  return err;
}


void omx_jpegdec_component_BufferMgmtCallback(OMX_COMPONENTTYPE *openmaxStandComp, OMX_BUFFERHEADERTYPE* pInputBuffer, OMX_BUFFERHEADERTYPE* pOutputBuffer) {
  
  omx_jpegdec_component_PrivateType* omx_jpegdec_component_Private = openmaxStandComp->pComponentPrivate;  
  omx_base_image_PortType *pOutPort = (omx_base_image_PortType *)omx_jpegdec_component_Private->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];

  DEBUG(DEB_LEV_FUNCTION_NAME, "Entry In %s\n", __func__);
  omx_jpegdec_component_Private->cur_headbuf = pInputBuffer;
  omx_jpegdec_component_Private->p_imgOutputBuf = (void*)pOutputBuffer->pBuffer;
  /*Signal fill_input_buffer*/

  if(omx_jpegdec_component_Private->isFirstBuffer == 1) {
    omx_jpegdec_component_Private->isFirstBuffer = 0;
  
	omx_jpegdec_component_Private->decode_error = OMX_ActImageDecoder_Decode(omx_jpegdec_component_Private);
	pOutputBuffer->nFilledLen = omx_jpegdec_component_Private->imgOutputBufLen;
  }
}

/*
 * An additional method that can be provided by data source modules is the
 * resync_to_restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library.  That method assumes that no backtracking
 * is possible.
 */


/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.  Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */
OMX_ERRORTYPE omx_jpegdec_decoder_MessageHandler(OMX_COMPONENTTYPE* openmaxStandComp, internalRequestMessageType *message)  {

  omx_jpegdec_component_PrivateType* omx_jpegdec_component_Private = (omx_jpegdec_component_PrivateType*)openmaxStandComp->pComponentPrivate;  
  OMX_ERRORTYPE err;
  OMX_STATETYPE eCurrentState = omx_jpegdec_component_Private->state;
  DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s\n", __func__);
//ACT_OMX_LOG("Entry:%s \n",__FUNCTION__);
  if (message->messageType == OMX_CommandStateSet){
    if ((message->messageParam == OMX_StateIdle) && (omx_jpegdec_component_Private->state == OMX_StateLoaded)) {
      err = omx_jpegdec_component_Init(openmaxStandComp);
      if(err!=OMX_ErrorNone) { 
        DEBUG(DEB_LEV_ERR, "In %s MAD Decoder Init Failed Error=%x\n",__func__,err); 
        return err;
      }
    } 
  }
  /** Execute the base message handling */
  err = omx_base_component_MessageHandler(openmaxStandComp, message);

  if (message->messageType == OMX_CommandStateSet){
    if ((message->messageParam == OMX_StateLoaded) && (eCurrentState == OMX_StateIdle)) {
      err = omx_jpegdec_component_Deinit(openmaxStandComp);
      if(err!=OMX_ErrorNone) { 
        DEBUG(DEB_LEV_ERR, "In %s MAD Decoder Deinit Failed Error=%x\n",__func__,err); 
        return err;
      }
    }
  }

  return err;  
}
