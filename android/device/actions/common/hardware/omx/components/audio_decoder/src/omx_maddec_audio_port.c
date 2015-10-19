/**
  @file src/base/omx_base_audio_port.c

  Base Audio Port class for OpenMAX ports to be used in derived components.

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

#include <string.h>
#include <unistd.h>
#include <omxcore.h>
#include <OMX_Core.h>
#include <OMX_Component.h>

#include "omx_base_component.h"
#include "omx_maddec_audio_port.h"

/**
  * @brief The base contructor for the generic OpenMAX ST Audio port
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

OMX_ERRORTYPE omx_audio_port_Constructor(OMX_COMPONENTTYPE *openmaxStandComp,omx_base_PortType **openmaxStandPort,OMX_U32 nPortIndex, OMX_BOOL isInput) {
	OMX_ERRORTYPE err;
  omx_maddec_audio_PortType *omx_base_audio_Port;

  DEBUG(DEB_LEV_FUNCTION_NAME, "In %s of component %p\n", __func__, openmaxStandComp);
  if (!(*openmaxStandPort)) {
    *openmaxStandPort = calloc(1,sizeof (omx_maddec_audio_PortType));
  }

  if (!(*openmaxStandPort)) {
    return OMX_ErrorInsufficientResources;
  }

  err = base_port_Constructor(openmaxStandComp,openmaxStandPort,nPortIndex, isInput);
  if (err != OMX_ErrorNone) {
	  DEBUG(DEB_LEV_ERR, "In %s base port constructor failed\n", __func__);
	  return err;
  }

  omx_base_audio_Port = (omx_maddec_audio_PortType *)*openmaxStandPort;

  setHeader(&omx_base_audio_Port->sAudioParam, sizeof(OMX_AUDIO_PARAM_PORTFORMATTYPE));
  omx_base_audio_Port->sAudioParam.nPortIndex = nPortIndex;
  omx_base_audio_Port->sAudioParam.nIndex = 0;
  omx_base_audio_Port->sAudioParam.eEncoding = OMX_AUDIO_CodingUnused;

  omx_base_audio_Port->sPortParam.eDomain = OMX_PortDomainAudio;
  omx_base_audio_Port->sPortParam.format.audio.cMIMEType = malloc(DEFAULT_MIME_STRING_LENGTH);
  if (!omx_base_audio_Port->sPortParam.format.audio.cMIMEType) {
	  DEBUG(DEB_LEV_ERR, "Memory allocation failed in %s\n", __func__);
	  return OMX_ErrorInsufficientResources;
  }
  strcpy(omx_base_audio_Port->sPortParam.format.audio.cMIMEType, "raw/audio");
  omx_base_audio_Port->sPortParam.format.audio.pNativeRender = 0;
  omx_base_audio_Port->sPortParam.format.audio.bFlagErrorConcealment = OMX_FALSE;
  omx_base_audio_Port->sPortParam.format.audio.eEncoding = OMX_AUDIO_CodingUnused;

  omx_base_audio_Port->sPortParam.nBufferSize = (isInput == OMX_TRUE)?DEFAULT_AUDIO_IN_BUFFER_SIZE:DEFAULT_AUDIO_OUT_BUFFER_SIZE ;

  omx_base_audio_Port->PortDestructor = &omx_audio_port_Destructor;
  omx_base_audio_Port->Port_FreeBuffer = &omx_audio_port_FreeBuffer;

  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s of component %p\n", __func__, openmaxStandComp);
  return OMX_ErrorNone;
}

/**
  * @brief The base audio port destructor for the generic OpenMAX ST Audio port
  *
  * This function is executed by the component that uses a port.
  * The parameter contains the info about the port.
  * It takes care of destructing the instance of the port
  *
  * @param openmaxStandPort the ST port to be destructed
  *
  * @return OMX_ErrorNone
  */

OMX_ERRORTYPE omx_audio_port_Destructor(omx_base_PortType *openmaxStandPort){
	OMX_ERRORTYPE err;
	DEBUG(DEB_LEV_FUNCTION_NAME, "In %s of port %p\n", __func__, openmaxStandPort);
  if(openmaxStandPort->sPortParam.format.audio.cMIMEType) {
    free(openmaxStandPort->sPortParam.format.audio.cMIMEType);
    openmaxStandPort->sPortParam.format.audio.cMIMEType = NULL;
  }

  err = base_port_Destructor(openmaxStandPort);

  if (err != OMX_ErrorNone) {
	DEBUG(DEB_LEV_ERR, "In %s base port destructor failed\n", __func__);
	return err;
  }
  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s of port %p\n", __func__, openmaxStandPort);
  return OMX_ErrorNone;
}

/** @brief Called by the standard function.
 *
 * It frees the buffer header and in case also the buffer itself, if needed.
 * When all the buffers are done, the variable bIsEmptyOfBuffers is set to OMX_TRUE
 */
OMX_ERRORTYPE omx_audio_port_FreeBuffer(
  omx_base_PortType *openmaxStandPort,
  OMX_U32 nPortIndex,
  OMX_BUFFERHEADERTYPE* pBuffer) {
  
  unsigned int i;
  DEBUG(DEB_LEV_FUNCTION_NAME, "In  %s for port %p\n", __func__, openmaxStandPort);
  OMX_COMPONENTTYPE* omxComponent = openmaxStandPort->standCompContainer;
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
  DEBUG(DEB_LEV_PARAMS, "In  %s, freeing pBuffer: %p, nBufferCountActual: %d \n", __func__, pBuffer,openmaxStandPort->sPortParam.nBufferCountActual);
  for(i=0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++){
    DEBUG(DEB_LEV_PARAMS, "In  %s, index %d, pBuffer: %p \n", __func__, i, openmaxStandPort->pInternalBufferStorage[i]->pBuffer);
    if (openmaxStandPort->bBufferStateAllocated[i] & (BUFFER_ASSIGNED | BUFFER_ALLOCATED)) {

      openmaxStandPort->bIsFullOfBuffers = OMX_FALSE;
      if ((openmaxStandPort->bBufferStateAllocated[i] & BUFFER_ALLOCATED) && (openmaxStandPort->pInternalBufferStorage[i] == pBuffer)) {
        if(openmaxStandPort->pInternalBufferStorage[i]->pBuffer){
          DEBUG(DEB_LEV_PARAMS, "In %s freeing %i pBuffer=%p\n",__func__, (int)i, openmaxStandPort->pInternalBufferStorage[i]->pBuffer);
          free(openmaxStandPort->pInternalBufferStorage[i]->pBuffer);
          openmaxStandPort->pInternalBufferStorage[i]->pBuffer=NULL;
        }
       
      } else if ((openmaxStandPort->bBufferStateAllocated[i] & BUFFER_ASSIGNED) && (openmaxStandPort->pInternalBufferStorage[i] == pBuffer)) {	
          free(pBuffer);
      }
      else
      {
      	continue;	
      }
      if(openmaxStandPort->bBufferStateAllocated[i] & HEADER_ALLOCATED) {
        free(openmaxStandPort->pInternalBufferStorage[i]);
        openmaxStandPort->pInternalBufferStorage[i]=NULL;
      }

      openmaxStandPort->bBufferStateAllocated[i] = BUFFER_FREE;

      openmaxStandPort->nNumAssignedBuffers--;
      DEBUG(DEB_LEV_PARAMS, "openmaxStandPort->nNumAssignedBuffers %i\n", (int)openmaxStandPort->nNumAssignedBuffers);

      if (openmaxStandPort->nNumAssignedBuffers == 0) {
        openmaxStandPort->sPortParam.bPopulated = OMX_FALSE;
        openmaxStandPort->bIsEmptyOfBuffers = OMX_TRUE;
        tsem_up(openmaxStandPort->pAllocSem);
      }
      DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for port %p\n", __func__, openmaxStandPort);
      return OMX_ErrorNone;
    }
  }
  DEBUG(DEB_LEV_FUNCTION_NAME, "Out of %s for port %p with OMX_ErrorInsufficientResources\n", __func__, openmaxStandPort);
  return OMX_ErrorInsufficientResources;
}
