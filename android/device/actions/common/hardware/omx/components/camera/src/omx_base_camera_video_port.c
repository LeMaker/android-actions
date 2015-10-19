/**
  src/base/omx_base_video_port.c

  Base Video Port class for OpenMAX ports to be used in derived components.

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

#include <string.h>
#include <unistd.h>
#include "ext_mem.h"
#include <OMX_Core.h>
#include <OMX_Component.h>
#include "omx_base_camera_video_port.h"
#include "log.h"


int get_base_bufferidx(OMX_BUFFERHEADERTYPE_ACTEXT *pBufferHeadAct, OMX_BUFFERHEADERTYPE *pBufferTarget, int bufferNum)
{
    int i;

    if(pBufferHeadAct == NULL || pBufferTarget == NULL)
    {
        OMXDBUG(OMXDBUG_ERR, "In %s: The port buffers is NULL %p,%p\n", __func__, pBufferHeadAct, pBufferTarget);
        return -1;
    }

    for(i = 0; i < bufferNum; i++)
    {
        if(pBufferTarget == pBufferHeadAct[i].pBuffHead)
        {
            return i;
        }
    }

    OMXDBUG(OMXDBUG_ERR, "In %s: The port buffers is not equal to backup's \n", __func__);
    return -1;
}
/** @brief Called by the standard allocate buffer, it implements a base functionality.
 *
 * This function can be overriden if the allocation of the buffer is not a simply alloc call.
 * The parameters are the same as the standard function, except for the handle of the port
 * instead of the handler of the component
 * When the buffers needed by this port are all assigned or allocated, the variable
 * bIsFullOfBuffers becomes equal to OMX_TRUE
 */
OMX_ERRORTYPE camera_video_port_AllocateBuffer(
    omx_base_PortType *openmaxStandPort,
    OMX_BUFFERHEADERTYPE **pBuffer,
    OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate,
    OMX_U32 nSizeBytes)
{

    unsigned int i;
    OMX_COMPONENTTYPE *omxComponent = openmaxStandPort->standCompContainer;
    omx_base_component_PrivateType *omx_base_component_Private = (omx_base_component_PrivateType *)omxComponent->pComponentPrivate;
    omx_base_camera_video_PortType *omx_base_video_Port = (omx_base_camera_video_PortType *)openmaxStandPort;
    OMXDBUG(OMXDBUG_VERB, "In %s for port %p\n", __func__, openmaxStandPort);

    if(nPortIndex != openmaxStandPort->sPortParam.nPortIndex)
    {
        return OMX_ErrorBadPortIndex;
    }

    if(PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort))
    {
        return OMX_ErrorBadPortIndex;
    }

    if(omx_base_component_Private->transientState != OMX_TransStateLoadedToIdle)
    {
        if(!openmaxStandPort->bIsTransientToEnabled)
        {
            OMXDBUG(OMXDBUG_ERR, "In %s: The port is not allowed to receive buffers\n", __func__);
            return OMX_ErrorIncorrectStateTransition;
        }
    }

    if(nSizeBytes < openmaxStandPort->sPortParam.nBufferSize)
    {
        OMXDBUG(OMXDBUG_ERR, "In %s: Requested Buffer Size %lu is less than Minimum Buffer Size %lu\n", __func__, nSizeBytes, openmaxStandPort->sPortParam.nBufferSize);
        return OMX_ErrorIncorrectStateTransition;
    }

    if(omx_base_video_Port->pBufferHeadAct == NULL)
    {
        omx_base_video_Port->pBufferHeadAct = calloc(openmaxStandPort->sPortParam.nBufferCountActual, sizeof(OMX_BUFFERHEADERTYPE_ACTEXT));

        if(omx_base_video_Port->pBufferHeadAct == NULL)
        {
            return OMX_ErrorInsufficientResources;
        }
    }

    for(i = 0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++)
    {
        if(openmaxStandPort->bBufferStateAllocated[i] == BUFFER_FREE)
        {
            openmaxStandPort->pInternalBufferStorage[i] = calloc(1, sizeof(OMX_BUFFERHEADERTYPE));

            if(!openmaxStandPort->pInternalBufferStorage[i])
            {
                return OMX_ErrorInsufficientResources;
            }

            setHeader(openmaxStandPort->pInternalBufferStorage[i], sizeof(OMX_BUFFERHEADERTYPE));
            /* allocate the buffer */
#if 0
            openmaxStandPort->pInternalBufferStorage[i]->pBuffHead.pBuffer = calloc(1, nSizeBytes);
#else
            {
                unsigned long pVirAddr = 0;
                openmaxStandPort->pInternalBufferStorage[i]->pBuffer = (OMX_U8 *)ext_phycalloc_mem(1, nSizeBytes, (void *)&pVirAddr);
                omx_base_video_Port->pBufferHeadAct[i].pConfigParam.VirAddr = (OMX_U8 *)pVirAddr;
                omx_base_video_Port->pBufferHeadAct[i].pConfigParam.phyAddr = openmaxStandPort->pInternalBufferStorage[i]->pBuffer;
                openmaxStandPort->pInternalBufferStorage[i]->pBuffer = omx_base_video_Port->pBufferHeadAct[i].pConfigParam.VirAddr;
                omx_base_video_Port->pBufferHeadAct[i].pConfigParam.bAllocByComp = OMX_TRUE;
            }
#endif

            if(openmaxStandPort->pInternalBufferStorage[i]->pBuffer == NULL)
            {
                return OMX_ErrorInsufficientResources;
            }

            omx_base_video_Port->pBufferHeadAct[i].pBuffHead = openmaxStandPort->pInternalBufferStorage[i];

            openmaxStandPort->pInternalBufferStorage[i]->nAllocLen = nSizeBytes;
            openmaxStandPort->pInternalBufferStorage[i]->pPlatformPrivate = openmaxStandPort;
            openmaxStandPort->pInternalBufferStorage[i]->pAppPrivate = pAppPrivate;
            *pBuffer = (OMX_BUFFERHEADERTYPE *)openmaxStandPort->pInternalBufferStorage[i];
            openmaxStandPort->bBufferStateAllocated[i] = BUFFER_ALLOCATED;
            openmaxStandPort->bBufferStateAllocated[i] |= HEADER_ALLOCATED;

            if(openmaxStandPort->sPortParam.eDir == OMX_DirInput)
            {
                openmaxStandPort->pInternalBufferStorage[i]->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
            }
            else
            {
                openmaxStandPort->pInternalBufferStorage[i]->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
            }

            openmaxStandPort->nNumAssignedBuffers++;
            OMXDBUG(OMXDBUG_VERB, "openmaxStandPort->nNumAssignedBuffers %i\n", (int)openmaxStandPort->nNumAssignedBuffers);

            if(openmaxStandPort->sPortParam.nBufferCountActual == openmaxStandPort->nNumAssignedBuffers)
            {
                openmaxStandPort->sPortParam.bPopulated = OMX_TRUE;
                openmaxStandPort->bIsFullOfBuffers = OMX_TRUE;
                OMXDBUG(OMXDBUG_VERB, "In %s nPortIndex=%d\n", __func__, (int)nPortIndex);
                tsem_up(openmaxStandPort->pAllocSem);
            }

            OMXDBUG(OMXDBUG_VERB, "Out of %s for port %p\n", __func__, openmaxStandPort);
            return OMX_ErrorNone;
        }
    }

    OMXDBUG(OMXDBUG_ERR, "Out of %s for port %p. Error: no available buffers\n", __func__, openmaxStandPort);
    return OMX_ErrorInsufficientResources;
}

/** @brief Called by the standard use buffer, it implements a base functionality.
 *
 * This function can be overriden if the use buffer implicate more complicated operations.
 * The parameters are the same as the standard function, except for the handle of the port
 * instead of the handler of the component.
 * When the buffers needed by this port are all assigned or allocated, the variable
 * bIsFullOfBuffers becomes equal to OMX_TRUE
 */
OMX_ERRORTYPE camera_video_port_UseBuffer(
    omx_base_PortType *openmaxStandPort,
    OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_U32 nPortIndex,
    OMX_PTR pAppPrivate,
    OMX_U32 nSizeBytes,
    OMX_U8 *pBuffer)
{

    unsigned int i;
    OMX_BUFFERHEADERTYPE *returnBufferHeader;
    omx_base_camera_video_PortType  *pBasePort = (omx_base_camera_video_PortType *)openmaxStandPort;
    OMX_COMPONENTTYPE *omxComponent = openmaxStandPort->standCompContainer;
    omx_base_component_PrivateType *omx_base_component_Private = (omx_base_component_PrivateType *)omxComponent->pComponentPrivate;
    omx_base_camera_video_PortType *omx_base_video_Port = (omx_base_camera_video_PortType *)openmaxStandPort;
    OMXDBUG(OMXDBUG_VERB, "In %s for port %p\n", __func__, openmaxStandPort);

    if(nPortIndex != openmaxStandPort->sPortParam.nPortIndex)
    {
        return OMX_ErrorBadPortIndex;
    }

    if(PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort))
    {
        return OMX_ErrorBadPortIndex;
    }

    if(omx_base_component_Private->transientState != OMX_TransStateLoadedToIdle)
    {
        if(!openmaxStandPort->bIsTransientToEnabled)
        {
            OMXDBUG(OMXDBUG_ERR, "In %s: The port of Comp %s is not allowed to receive buffers\n", __func__, omx_base_component_Private->name);
            return OMX_ErrorIncorrectStateTransition;
        }
    }

    if(pBasePort->bStoreMediadata == OMX_FALSE && nSizeBytes < openmaxStandPort->sPortParam.nBufferSize)
    {
        OMXDBUG(OMXDBUG_ERR, "In %s: Port %d Given Buffer Size %u is less than Minimum Buffer Size %u\n", __func__, (int)nPortIndex, (int)nSizeBytes, (int)openmaxStandPort->sPortParam.nBufferSize);
        return OMX_ErrorIncorrectStateTransition;
    }

    if(omx_base_video_Port->pBufferHeadAct == NULL)
    {
        omx_base_video_Port->pBufferHeadAct = calloc(openmaxStandPort->sPortParam.nBufferCountActual, sizeof(OMX_BUFFERHEADERTYPE_ACTEXT));

        if(omx_base_video_Port->pBufferHeadAct == NULL)
        {
            return OMX_ErrorInsufficientResources;
        }
    }

    for(i = 0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++)
    {
        if(openmaxStandPort->bBufferStateAllocated[i] == BUFFER_FREE)
        {
            openmaxStandPort->pInternalBufferStorage[i] = calloc(1, sizeof(OMX_BUFFERHEADERTYPE));

            if(!openmaxStandPort->pInternalBufferStorage[i])
            {
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
            returnBufferHeader = calloc(1, sizeof(OMX_BUFFERHEADERTYPE));

            if(!returnBufferHeader)
            {
                return OMX_ErrorInsufficientResources;
            }

            omx_base_video_Port->pBufferHeadAct[i].pBuffHead = returnBufferHeader;

            setHeader(returnBufferHeader, sizeof(OMX_BUFFERHEADERTYPE));
            returnBufferHeader->pBuffer = pBuffer;
            returnBufferHeader->nAllocLen = nSizeBytes;
            returnBufferHeader->pPlatformPrivate = openmaxStandPort;
            returnBufferHeader->pAppPrivate = pAppPrivate;

            if(openmaxStandPort->sPortParam.eDir == OMX_DirInput)
            {
                openmaxStandPort->pInternalBufferStorage[i]->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
                returnBufferHeader->nInputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
            }
            else
            {
                openmaxStandPort->pInternalBufferStorage[i]->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
                returnBufferHeader->nOutputPortIndex = openmaxStandPort->sPortParam.nPortIndex;
            }

            *ppBufferHdr = (OMX_BUFFERHEADERTYPE *)returnBufferHeader;
            openmaxStandPort->nNumAssignedBuffers++;
            OMXDBUG(OMXDBUG_VERB, "openmaxStandPort->nNumAssignedBuffers %d,%d\n", (unsigned int)openmaxStandPort->sPortParam.nBufferCountActual, (unsigned int)openmaxStandPort->nNumAssignedBuffers);

            if(openmaxStandPort->sPortParam.nBufferCountActual == openmaxStandPort->nNumAssignedBuffers)
            {
                openmaxStandPort->sPortParam.bPopulated = OMX_TRUE;
                openmaxStandPort->bIsFullOfBuffers = OMX_TRUE;
                tsem_up(openmaxStandPort->pAllocSem);
            }

            OMXDBUG(OMXDBUG_VERB, "Out of %s for port %p\n", __func__, openmaxStandPort);
            return OMX_ErrorNone;
        }
    }

    OMXDBUG(OMXDBUG_ERR, "In %s Error: no available buffers CompName=%s\n", __func__, omx_base_component_Private->name);
    return OMX_ErrorInsufficientResources;
}

/** @brief Called by the standard function.
 *
 * It frees the buffer header and in case also the buffer itself, if needed.
 * When all the buffers are done, the variable bIsEmptyOfBuffers is set to OMX_TRUE
 */
OMX_ERRORTYPE camera_video_port_FreeBuffer(
    omx_base_PortType *openmaxStandPort,
    OMX_U32 nPortIndex,
    OMX_BUFFERHEADERTYPE *pBuffer)
{

    unsigned int i;
    OMX_COMPONENTTYPE *omxComponent = openmaxStandPort->standCompContainer;
    omx_base_component_PrivateType *omx_base_component_Private = (omx_base_component_PrivateType *)omxComponent->pComponentPrivate;
    omx_base_camera_video_PortType *omx_base_video_Port = (omx_base_camera_video_PortType *)openmaxStandPort;
    OMXDBUG(OMXDBUG_VERB, "In %s for port %p,%d\n", __func__, openmaxStandPort, (int)nPortIndex);

    if(nPortIndex != openmaxStandPort->sPortParam.nPortIndex)
    {
        return OMX_ErrorBadPortIndex;
    }

    if(PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort))
    {
        return OMX_ErrorBadPortIndex;
    }

    //printf("b,,,,1\n");
    if(omx_base_component_Private->transientState != OMX_TransStateIdleToLoaded)
    {
        if(!openmaxStandPort->bIsTransientToDisabled)
        {

            OMXDBUG(OMXDBUG_VERB, "In %s: The port is not allowed to free the buffers\n", __func__);
            (*(omx_base_component_Private->callbacks->EventHandler))
            (omxComponent,
             omx_base_component_Private->callbackData,
             OMX_EventError, /* The command was completed */
             OMX_ErrorPortUnpopulated, /* The commands was a OMX_CommandStateSet */
             nPortIndex, /* The state has been changed in message->messageParam2 */
             NULL);
        }
    }


    for(i = 0; i < openmaxStandPort->sPortParam.nBufferCountActual; i++)
    {
        if(openmaxStandPort->bBufferStateAllocated[i] & (BUFFER_ASSIGNED | BUFFER_ALLOCATED))
        {

            openmaxStandPort->bIsFullOfBuffers = OMX_FALSE;

            if(openmaxStandPort->bBufferStateAllocated[i] & BUFFER_ALLOCATED)
            {
                if(openmaxStandPort->pInternalBufferStorage[i]->pBuffer)
                {
                    OMXDBUG(OMXDBUG_VERB, "In %s freeing %i pBuffer=%p\n", __func__, (int)i, openmaxStandPort->pInternalBufferStorage[i]->pBuffer);

                    if(omx_base_video_Port->pBufferHeadAct[i].pConfigParam.bAllocByComp == OMX_TRUE)
                    {
                        ext_phyfree_mem((void *)openmaxStandPort->pInternalBufferStorage[i]->pBuffer, \
                                        (void *)omx_base_video_Port->pBufferHeadAct[i].pConfigParam.VirAddr);
                        omx_base_video_Port->pBufferHeadAct[i].pConfigParam.bAllocByComp = OMX_FALSE;
                        omx_base_video_Port->pBufferHeadAct[i].pBuffHead = NULL;
                    }
                    else
                    { free(openmaxStandPort->pInternalBufferStorage[i]->pBuffer); }

                    openmaxStandPort->pInternalBufferStorage[i]->pBuffer = NULL;
                    omx_base_video_Port->pBufferHeadAct[i].pConfigParam.bUseBufFlag = OMX_FALSE;
                }

            }
            else if(openmaxStandPort->bBufferStateAllocated[i] & BUFFER_ASSIGNED)
            {
                // printf("buffer free....????%x\n",pBuffer);
                if(omx_base_video_Port->pBufferHeadAct[i].pConfigParam.bAllocByComp == OMX_TRUE)
                {
                    ext_phyfree_mem((void *)omx_base_video_Port->pBufferHeadAct[i].pConfigParam.phyAddr, \
                                    (void *)omx_base_video_Port->pBufferHeadAct[i].pConfigParam.VirAddr);
                    omx_base_video_Port->pBufferHeadAct[i].pConfigParam.bAllocByComp = OMX_FALSE;
                    omx_base_video_Port->pBufferHeadAct[i].pConfigParam.phyAddr = NULL;
                    omx_base_video_Port->pBufferHeadAct[i].pConfigParam.VirAddr = NULL;
                    omx_base_video_Port->pBufferHeadAct[i].pBuffHead = NULL;
                }

                omx_base_video_Port->pBufferHeadAct[i].pConfigParam.bUseBufFlag = OMX_FALSE;
                free(pBuffer);
            }

            //printf("b,,,,2\n");
            if(openmaxStandPort->bBufferStateAllocated[i] & HEADER_ALLOCATED)
            {
                free(openmaxStandPort->pInternalBufferStorage[i]);
                openmaxStandPort->pInternalBufferStorage[i] = NULL;
            }

            openmaxStandPort->bBufferStateAllocated[i] = BUFFER_FREE;

            openmaxStandPort->nNumAssignedBuffers--;
            OMXDBUG(OMXDBUG_VERB, "openmaxStandPort->nNumAssignedBuffers %i\n", (int)openmaxStandPort->nNumAssignedBuffers);

            if(openmaxStandPort->nNumAssignedBuffers == 0)
            {
                if(omx_base_video_Port->pBufferHeadAct)
                {
                    free(omx_base_video_Port->pBufferHeadAct);
                    omx_base_video_Port->pBufferHeadAct = NULL;
                    printf("Free pBufferHeadAct now\n");
                }

                openmaxStandPort->sPortParam.bPopulated = OMX_FALSE;
                openmaxStandPort->bIsEmptyOfBuffers = OMX_TRUE;
                tsem_up(openmaxStandPort->pAllocSem);
            }

            OMXDBUG(OMXDBUG_VERB, "Out of %s for port %p\n", __func__, openmaxStandPort);
            return OMX_ErrorNone;
        }
    }

    OMXDBUG(OMXDBUG_ERR, "Out of %s for port %p with OMX_ErrorInsufficientResources\n", __func__, openmaxStandPort);
    return OMX_ErrorInsufficientResources;
}


/** @brief Releases buffers under processing.
 * This function must be implemented in the derived classes, for the
 * specific processing
 */
OMX_ERRORTYPE camera_video_port_FlushProcessingBuffers(omx_base_PortType *openmaxStandPort)
{
    omx_base_component_PrivateType *omx_base_component_Private;
    OMX_BUFFERHEADERTYPE *pBuffer;
    int errQue;
    omx_base_camera_video_PortType *omx_base_video_Port;
    int nPrePare_Num = 0;

    OMXDBUG(OMXDBUG_VERB, "In %s for port %p\n", __func__, openmaxStandPort);
    omx_base_component_Private = (omx_base_component_PrivateType *)openmaxStandPort->standCompContainer->pComponentPrivate;
    omx_base_video_Port = (omx_base_camera_video_PortType *)openmaxStandPort;

    if(openmaxStandPort->sPortParam.eDomain != OMX_PortDomainOther) /* clock buffers not used in the clients buffer managment function */
    {
        pthread_mutex_lock(&omx_base_component_Private->flush_mutex);
        openmaxStandPort->bIsPortFlushed = OMX_TRUE;

        /*Signal the buffer management thread of port flush,if it is waiting for buffers*/
        if(omx_base_component_Private->bMgmtSem->semval == 0)
        {
            tsem_up(omx_base_component_Private->bMgmtSem);
        }

        if(omx_base_component_Private->state != OMX_StateExecuting)
        {
            /*Waiting at paused state*/
            tsem_signal(omx_base_component_Private->bStateSem);
        }

        OMXDBUG(OMXDBUG_VERB, "In %s waiting for flush all condition port index =%d\n", __func__, (int)openmaxStandPort->sPortParam.nPortIndex);
        /* Wait until flush is completed */
        pthread_mutex_unlock(&omx_base_component_Private->flush_mutex);
        tsem_down(omx_base_component_Private->flush_all_condition);
    }

    OMXDBUG(OMXDBUG_VERB, "In %s flushed all the buffers under processing\n", __func__);

    tsem_reset(omx_base_component_Private->bMgmtSem);

    /* Flush all the buffers not under processing */
    while(openmaxStandPort->pBufferSem->semval > 0)
    {
        OMXDBUG(OMXDBUG_VERB, "In %s TFlag=%x Flusing Port=%d,Semval=%d Qelem=%d\n",
              __func__, (int)openmaxStandPort->nTunnelFlags, (int)openmaxStandPort->sPortParam.nPortIndex,
              (int)openmaxStandPort->pBufferSem->semval, (int)openmaxStandPort->pBufferQueue->nelem);

        tsem_down(openmaxStandPort->pBufferSem);
        pBuffer = dequeue(openmaxStandPort->pBufferQueue);

        if(PORT_IS_TUNNELED(openmaxStandPort) && !PORT_IS_BUFFER_SUPPLIER(openmaxStandPort))
        {
            OMXDBUG(OMXDBUG_VERB, "In %s: Comp %s is returning io:%d buffer\n",
                  __func__, omx_base_component_Private->name, (int)openmaxStandPort->sPortParam.nPortIndex);

            if(openmaxStandPort->sPortParam.eDir == OMX_DirInput)
            {
                ((OMX_COMPONENTTYPE *)(openmaxStandPort->hTunneledComponent))->FillThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
            }
            else
            {
                ((OMX_COMPONENTTYPE *)(openmaxStandPort->hTunneledComponent))->EmptyThisBuffer(openmaxStandPort->hTunneledComponent, pBuffer);
            }
        }
        else if(PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort))
        {
            errQue = queue(openmaxStandPort->pBufferQueue, pBuffer);

            if(errQue)
            {
                /* /TODO the queue is full. This can be handled in a fine way with
                 * some retrials, or other checking. For the moment this is a critical error
                 * and simply causes the failure of this call
                 */
                return OMX_ErrorInsufficientResources;
            }
        }
        else
        {
            (*(openmaxStandPort->BufferProcessedCallback))(
                openmaxStandPort->standCompContainer,
                omx_base_component_Private->callbackData,
                pBuffer);
        }
    }

    /*Port is tunneled and supplier and didn't received all it's buffer then wait for the buffers*/
    if(PORT_IS_TUNNELED_N_BUFFER_SUPPLIER(openmaxStandPort))
    {
        while(openmaxStandPort->pBufferQueue->nelem != openmaxStandPort->nNumAssignedBuffers)
        {
            tsem_down(openmaxStandPort->pBufferSem);
            OMXDBUG(OMXDBUG_VERB, "In %s Got a buffer qelem=%d\n", __func__, openmaxStandPort->pBufferQueue->nelem);
        }

        tsem_reset(openmaxStandPort->pBufferSem);
    }

    nPrePare_Num = getquenelem(omx_base_video_Port->queue_dq);

    while(nPrePare_Num > 0)
    {
        pBuffer = dequeue(omx_base_video_Port->queue_dq);
        pBuffer = NULL;
        nPrePare_Num = getquenelem(omx_base_video_Port->queue_dq);
    }

    pthread_mutex_lock(&omx_base_component_Private->flush_mutex);
    openmaxStandPort->bIsPortFlushed = OMX_FALSE;
    pthread_mutex_unlock(&omx_base_component_Private->flush_mutex);

    tsem_up(omx_base_component_Private->flush_condition);

    OMXDBUG(OMXDBUG_VERB, "Out %s Port Index=%d bIsPortFlushed=%d Component %s\n", __func__,
          (int)openmaxStandPort->sPortParam.nPortIndex, (int)openmaxStandPort->bIsPortFlushed, omx_base_component_Private->name);

    OMXDBUG(OMXDBUG_VERB, "In %s TFlag=%x Qelem=%d BSem=%d bMgmtsem=%d component=%s\n", __func__,
          (int)openmaxStandPort->nTunnelFlags,
          (int)openmaxStandPort->pBufferQueue->nelem,
          (int)openmaxStandPort->pBufferSem->semval,
          (int)omx_base_component_Private->bMgmtSem->semval,
          omx_base_component_Private->name);

    OMXDBUG(OMXDBUG_VERB, "Out %s Port %p Index=%d\n", __func__, openmaxStandPort, (int)openmaxStandPort->sPortParam.nPortIndex);
    return OMX_ErrorNone;
}

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

OMX_ERRORTYPE base_camera_video_port_Constructor(OMX_COMPONENTTYPE *openmaxStandComp, omx_base_PortType **openmaxStandPort, OMX_U32 nPortIndex, OMX_BOOL isInput)
{

    omx_base_camera_video_PortType *omx_base_video_Port;

    if(!(*openmaxStandPort))
    {
        OMXDBUG(OMXDBUG_ERR, "Input openmaxStandPort is NULL!\n");
        *openmaxStandPort = calloc(1, sizeof(omx_base_camera_video_PortType));
    }

    if(!(*openmaxStandPort))
    {
        return OMX_ErrorInsufficientResources;
    }

    base_video_port_Constructor(openmaxStandComp, openmaxStandPort, nPortIndex, isInput);

    omx_base_video_Port = (omx_base_camera_video_PortType *)*openmaxStandPort;
    omx_base_video_Port->nSensorSelect = -1;
    omx_base_video_Port->isCapture = OMX_FALSE;
    omx_base_video_Port->bCapturePause  = OMX_FALSE;
    omx_base_video_Port->pCapMode = calloc(1, sizeof(OMX_CONFIG_CAPTUREMODETYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pCapMode, OMX_CONFIG_CAPTUREMODETYPE);
    omx_base_video_Port->pCapMode->nPortIndex  = nPortIndex;
    omx_base_video_Port->pCapMode->bContinuous = OMX_FALSE;
    omx_base_video_Port->pCapMode->bFrameLimited = OMX_FALSE;
    omx_base_video_Port->pCapMode->nFrameLimit = 0;

    omx_base_video_Port->pCapExtMode = calloc(1, sizeof(OMX_CONFIG_EXTCAPTUREMODETYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pCapExtMode, OMX_CONFIG_EXTCAPTUREMODETYPE);
    omx_base_video_Port->pCapExtMode->nPortIndex  = nPortIndex;
    omx_base_video_Port->pCapExtMode->nFrameBefore = 0;
    omx_base_video_Port->pCapExtMode->bPrepareCapture = OMX_FALSE;

    /* Banding Config, default Auto */
    omx_base_video_Port->pFlicktype = calloc(1, sizeof(OMX_CONFIG_FLICKERREJECTIONTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pFlicktype, OMX_CONFIG_FLICKERREJECTIONTYPE);
    omx_base_video_Port->pFlicktype->nPortIndex = nPortIndex;
    omx_base_video_Port->pFlicktype->eFlickerRejection = OMX_FlickerRejectionAuto;

    /* Noise reduce, default as closed */
    omx_base_video_Port->pNs_level = calloc(1, sizeof(OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pNs_level, OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE);
    omx_base_video_Port->pNs_level->nPortIndex = nPortIndex;
    omx_base_video_Port->pNs_level->bAuto = OMX_FALSE;
    omx_base_video_Port->pNs_level->nLevel = 0;

    /* Sharpness enhance,only supoort one level */
    omx_base_video_Port->pSharp_level = calloc(1, sizeof(OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pSharp_level, OMX_IMAGE_CONFIG_PROCESSINGLEVELTYPE);
    omx_base_video_Port->pSharp_level->nPortIndex = nPortIndex;
    omx_base_video_Port->pNs_level->bAuto = OMX_FALSE;
    omx_base_video_Port->pNs_level->nLevel = 0;

    omx_base_video_Port->pAF_Dis = calloc(1, sizeof(OMX_ACT_CONFIG_FOCUSDISTANCETYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pAF_Dis, OMX_ACT_CONFIG_FOCUSDISTANCETYPE);
    omx_base_video_Port->pAF_Dis->nPortIndex = nPortIndex;

    //Gamma Table,Default is 2.2
    omx_base_video_Port->pGamma = calloc(1, sizeof(OMX_CONFIG_GAMMATYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pGamma, OMX_CONFIG_GAMMATYPE);
    omx_base_video_Port->pGamma->nPortIndex = nPortIndex;
    omx_base_video_Port->pGamma->nGamma = (22 * (1 << 16)) / 10;

    omx_base_video_Port->pBlitComp = calloc(1, sizeof(OMX_ACT_CONFIG_BLITCOMPENSATIONTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pBlitComp, OMX_ACT_CONFIG_BLITCOMPENSATIONTYPE);
    omx_base_video_Port->pBlitComp->nPortIndex = nPortIndex;
    omx_base_video_Port->pBlitComp->eControl = OMX_ACT_BceModeOff;

    //omx_base_video_Port->pCapMode = calloc(1,sizeof(OMX_CONFIG_CAPTUREMODETYPE));
    //OMX_CONF_INIT_STRUCT(omx_base_video_Port->pCapMode,OMX_CONFIG_CAPTUREMODETYPE);
    //omx_base_video_Port->pCapMode->nPortIndex = nPortIndex;
    //omx_base_video_Port->pCapMode->bContinuous = OMX_FALSE;
    //omx_base_video_Port->pCapMode->bFrameLimited = OMX_FALSE;

    omx_base_video_Port->pFlashType = calloc(1, sizeof(OMX_IMAGE_PARAM_FLASHCONTROLTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pFlashType, OMX_IMAGE_PARAM_FLASHCONTROLTYPE);
    omx_base_video_Port->pFlashType->nPortIndex = nPortIndex;
    omx_base_video_Port->pFlashType->eFlashControl = OMX_IMAGE_FlashControlOff;

    omx_base_video_Port->pFocusType = calloc(1, sizeof(OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pFocusType, OMX_IMAGE_CONFIG_FOCUSCONTROLTYPE);
    omx_base_video_Port->pFocusType->nPortIndex = nPortIndex;
    omx_base_video_Port->pFocusType->eFocusControl = OMX_IMAGE_FocusControlOff;//Control by user only when nFocusSteps > 0

    omx_base_video_Port->pSensorMode = calloc(1, sizeof(OMX_PARAM_SENSORMODETYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pSensorMode, OMX_PARAM_SENSORMODETYPE);
    omx_base_video_Port->pSensorMode->nPortIndex = nPortIndex;
    omx_base_video_Port->pSensorMode->nFrameRate = DEFAULT_FPS;
    omx_base_video_Port->pSensorMode->bOneShot = OMX_FALSE;//Default is Video Mode
    omx_base_video_Port->pSensorMode->sFrameSize.nSize = sizeof(OMX_FRAMESIZETYPE);
    omx_base_video_Port->pSensorMode->sFrameSize.nVersion.s.nVersionMajor = 1;
    omx_base_video_Port->pSensorMode->sFrameSize.nVersion.s.nVersionMinor = 1;
    omx_base_video_Port->pSensorMode->sFrameSize.nVersion.s.nRevision = 0;
    omx_base_video_Port->pSensorMode->sFrameSize.nVersion.s.nStep = 0;
    omx_base_video_Port->pSensorMode->sFrameSize.nPortIndex = nPortIndex;
    omx_base_video_Port->pSensorMode->sFrameSize.nWidth = DEFAULT_WIDTH;
    omx_base_video_Port->pSensorMode->sFrameSize.nHeight = DEFAULT_HEIGHT;

    omx_base_video_Port->pColorFix = calloc(1, sizeof(OMX_CONFIG_COLORCONVERSIONTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pColorFix, OMX_CONFIG_COLORCONVERSIONTYPE);
    omx_base_video_Port->pColorFix->nPortIndex = nPortIndex;
    omx_base_video_Port->pColorFix->xColorMatrix[0][0] = 257 * (1 << 16) / 1000;
    omx_base_video_Port->pColorFix->xColorMatrix[0][1] = 504 * (1 << 16) / 1000;
    omx_base_video_Port->pColorFix->xColorMatrix[0][2] = 98  * (1 << 16) / 1000;
    omx_base_video_Port->pColorFix->xColorMatrix[1][0] = -148 * (1 << 16) / 1000;
    omx_base_video_Port->pColorFix->xColorMatrix[1][1] = -291 * (1 << 16) / 1000;
    omx_base_video_Port->pColorFix->xColorMatrix[1][2] = 439  * (1 << 16) / 1000;
    omx_base_video_Port->pColorFix->xColorMatrix[2][0] = 439 * (1 << 16) / 1000;
    omx_base_video_Port->pColorFix->xColorMatrix[2][1] = -368 * (1 << 16) / 1000;
    omx_base_video_Port->pColorFix->xColorMatrix[2][2] = -71  * (1 << 16) / 1000;
    omx_base_video_Port->pColorFix->xColorOffset[0] = 16  * (1 << 16);
    omx_base_video_Port->pColorFix->xColorOffset[1] = 128 * (1 << 16);
    omx_base_video_Port->pColorFix->xColorOffset[2] = 128 * (1 << 16);

    omx_base_video_Port->pColorEft = calloc(1, sizeof(OMX_CONFIG_COLORENHANCEMENTTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pColorEft, OMX_CONFIG_COLORENHANCEMENTTYPE);
    omx_base_video_Port->pColorEft->bColorEnhancement = OMX_FALSE;

    omx_base_video_Port->pImageFilter = calloc(1, sizeof(OMX_CONFIG_IMAGEFILTERTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pImageFilter, OMX_CONFIG_IMAGEFILTERTYPE);
    omx_base_video_Port->pImageFilter->nPortIndex = nPortIndex;
    omx_base_video_Port->pImageFilter->eImageFilter = OMX_ImageFilterNone;

    omx_base_video_Port->pImageMirror = calloc(1, sizeof(OMX_CONFIG_MIRRORTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pImageMirror, OMX_CONFIG_MIRRORTYPE);
    omx_base_video_Port->pImageMirror->nPortIndex = nPortIndex;
    omx_base_video_Port->pImageMirror->eMirror = OMX_MirrorNone;

    omx_base_video_Port->pOpticZoomType = calloc(1, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pOpticZoomType, OMX_CONFIG_SCALEFACTORTYPE);
    omx_base_video_Port->pOpticZoomType->nPortIndex = nPortIndex;
    omx_base_video_Port->pOpticZoomType->xWidth = 0;//Not Used
    omx_base_video_Port->pOpticZoomType->xHeight = 0;

    omx_base_video_Port->pWBType = calloc(1, sizeof(OMX_CONFIG_WHITEBALCONTROLTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pWBType, OMX_CONFIG_WHITEBALCONTROLTYPE);
    omx_base_video_Port->pWBType->nPortIndex = nPortIndex;
    omx_base_video_Port->pWBType->eWhiteBalControl = OMX_WhiteBalControlAuto;

    omx_base_video_Port->pExpType = calloc(1, sizeof(OMX_CONFIG_EXPOSURECONTROLTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pExpType, OMX_CONFIG_EXPOSURECONTROLTYPE);
    omx_base_video_Port->pExpType->nPortIndex = nPortIndex;
    omx_base_video_Port->pExpType->eExposureControl = OMX_ExposureControlAuto;

    omx_base_video_Port->pContrast = calloc(1, sizeof(OMX_CONFIG_CONTRASTTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pContrast, OMX_CONFIG_CONTRASTTYPE);
    omx_base_video_Port->pContrast->nPortIndex = nPortIndex;
    omx_base_video_Port->pContrast->nContrast = 0;

    omx_base_video_Port->pBright = calloc(1, sizeof(OMX_CONFIG_BRIGHTNESSTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pBright, OMX_CONFIG_BRIGHTNESSTYPE);
    omx_base_video_Port->pBright->nPortIndex = nPortIndex;
    omx_base_video_Port->pBright->nBrightness = 0;

    omx_base_video_Port->pSat = calloc(1, sizeof(OMX_CONFIG_SATURATIONTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pSat, OMX_CONFIG_SATURATIONTYPE);
    omx_base_video_Port->pSat->nPortIndex = nPortIndex;
    omx_base_video_Port->pSat->nSaturation = 0;

    omx_base_video_Port->pExpVal = calloc(1, sizeof(OMX_CONFIG_EXPOSUREVALUETYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pExpVal, OMX_CONFIG_EXPOSUREVALUETYPE);
    omx_base_video_Port->pExpVal->nPortIndex = nPortIndex;
    omx_base_video_Port->pExpVal->eMetering = OMX_MeteringModeAverage;

    omx_base_video_Port->pHdrParam = calloc(1, sizeof(OMX_ACT_CONFIG_HDR_EVParams));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pHdrParam, OMX_ACT_CONFIG_HDR_EVParams);
    omx_base_video_Port->pExpVal->nPortIndex = nPortIndex;

    omx_base_video_Port->pAFRegionL = calloc(1, sizeof(OMX_CONFIG_FOCUSREGIONTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pAFRegionL, OMX_CONFIG_FOCUSREGIONTYPE);
    omx_base_video_Port->pAFRegionL->nPortIndex = nPortIndex;
    omx_base_video_Port->pAFRegionL->bCenter = OMX_FALSE;
    omx_base_video_Port->pAFRegionL->bLeft = OMX_FALSE;
    omx_base_video_Port->pAFRegionL->bRight = OMX_FALSE;
    omx_base_video_Port->pAFRegionL->bTop = OMX_FALSE;
    omx_base_video_Port->pAFRegionL->bBottom = OMX_FALSE;
    omx_base_video_Port->pAFRegionL->bTopLeft = OMX_FALSE;
    omx_base_video_Port->pAFRegionL->bTopRight = OMX_FALSE;
    omx_base_video_Port->pAFRegionL->bBottomLeft = OMX_FALSE;
    omx_base_video_Port->pAFRegionL->bBottomRight = OMX_FALSE;

    omx_base_video_Port->pAFStatusL = calloc(1, sizeof(OMX_PARAM_FOCUSSTATUSTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pAFStatusL, OMX_PARAM_FOCUSSTATUSTYPE);
    omx_base_video_Port->pAFStatusL->eFocusStatus = OMX_FocusStatusOff;
    omx_base_video_Port->pAFStatusL->bCenterStatus = OMX_FALSE;
    omx_base_video_Port->pAFStatusL->bLeftStatus = OMX_FALSE;
    omx_base_video_Port->pAFStatusL->bRightStatus = OMX_FALSE;
    omx_base_video_Port->pAFStatusL->bTopStatus = OMX_FALSE;
    omx_base_video_Port->pAFStatusL->bBottomStatus = OMX_FALSE;
    omx_base_video_Port->pAFStatusL->bTopLeftStatus = OMX_FALSE;
    omx_base_video_Port->pAFStatusL->bTopRightStatus = OMX_FALSE;
    omx_base_video_Port->pAFStatusL->bBottomLeftStatus = OMX_FALSE;
    omx_base_video_Port->pAFStatusL->bBottomRightStatus = OMX_FALSE;

    omx_base_video_Port->pAFRegion = calloc(1, sizeof(OMX_CONFIG_FOCUSREGIONCONTROLTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pAFRegion, OMX_CONFIG_FOCUSREGIONCONTROLTYPE);
    //omx_base_video_Port->pAFRegion->nPortIndex = nPortIndex;
    omx_base_video_Port->pAFRegion->nFAreas = 1;
    omx_base_video_Port->pAFRegion->eFocusRegionsControl  = OMX_FocusRegionControlAuto;
    omx_base_video_Port->pAFRegion->sManualFRegions[0].nRectX = DEFAULT_WIDTH / 2 - 32;
    omx_base_video_Port->pAFRegion->sManualFRegions[0].nRectY = DEFAULT_HEIGHT / 2 - 32;
    omx_base_video_Port->pAFRegion->sManualFRegions[0].nRectWidth = 64;
    omx_base_video_Port->pAFRegion->sManualFRegions[0].nRectHeight = 64;

    omx_base_video_Port->pAFStatus = calloc(1, sizeof(OMX_CONFIG_FOCUSREGIONSTATUSTYPE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->pAFStatus, OMX_CONFIG_FOCUSREGIONSTATUSTYPE);
    omx_base_video_Port->pAFStatus->bFocused = OMX_FALSE;
    omx_base_video_Port->pAFStatus->nMaxFAreas = 1;
    omx_base_video_Port->pAFStatus->nFAreas = 1;
    omx_base_video_Port->pAFStatus->sFROIs[0].nRectX = DEFAULT_WIDTH / 2 - 32;
    omx_base_video_Port->pAFStatus->sFROIs[0].nRectY = DEFAULT_HEIGHT / 2 - 32;
    omx_base_video_Port->pAFStatus->sFROIs[0].nRectWidth = 64;
    omx_base_video_Port->pAFStatus->sFROIs[0].nRectHeight = 64;
    omx_base_video_Port->pAFStatus->sFROIs[0].xFocusDistance = 0;
    omx_base_video_Port->pAFStatus->sFROIs[0].eFocusStatus = OMX_FocusStatusOff;

    omx_base_video_Port->act_agc = calloc(1, sizeof(OMX_ACT_CONFIG_AGCVALUE));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->act_agc, OMX_ACT_CONFIG_AGCVALUE);
    omx_base_video_Port->act_agc->nPortIndex = 0;
    omx_base_video_Port->act_flashstrobe = calloc(1, sizeof(OMX_ACT_CONFIG_FlashStrobeParams));
    OMX_CONF_INIT_STRUCT(omx_base_video_Port->act_flashstrobe, OMX_ACT_CONFIG_FlashStrobeParams);
    omx_base_video_Port->act_flashstrobe->nPortIndex = 0;

    omx_base_video_Port->queue_dq = calloc(1, sizeof(queue_t));
    queue_init(omx_base_video_Port->queue_dq);

    if(isInput == OMX_FALSE && nPortIndex == 0)
    {
        //OUTPUT
        setHeader(&omx_base_video_Port->sVideoParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
        omx_base_video_Port->sVideoParam.nPortIndex = nPortIndex;
        omx_base_video_Port->sVideoParam.nIndex = 0;
        omx_base_video_Port->sVideoParam.eCompressionFormat = OMX_VIDEO_CodingUnused;
        omx_base_video_Port->sVideoParam.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        omx_base_video_Port->sVideoParam.xFramerate = (25 << 16);

        omx_base_video_Port->sPortParam.eDomain = OMX_PortDomainVideo;
#ifndef _OPENMAX_V1_2_
        omx_base_video_Port->sPortParam.format.video.cMIMEType = malloc(DEFAULT_MIME_STRING_LENGTH);
        strcpy(omx_base_video_Port->sPortParam.format.video.cMIMEType, "video/camera");
#endif
        omx_base_video_Port->sPortParam.format.video.pNativeRender = 0;
        omx_base_video_Port->sPortParam.format.video.bFlagErrorConcealment = OMX_FALSE;
        omx_base_video_Port->sPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

        omx_base_video_Port->sPortParam.format.video.nFrameWidth = 0;
        omx_base_video_Port->sPortParam.format.video.nFrameHeight = 0;
        omx_base_video_Port->sPortParam.format.video.nStride = 0;
        omx_base_video_Port->sPortParam.format.video.nSliceHeight = 0;
        omx_base_video_Port->sPortParam.format.video.nBitrate = 0;
        omx_base_video_Port->sPortParam.format.video.xFramerate = (25 << 16);
        omx_base_video_Port->sPortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        omx_base_video_Port->sPortParam.format.video.pNativeWindow = NULL;

        //Video Capture
        omx_base_video_Port->sPortParam.nBufferCountMin = 6;
        omx_base_video_Port->sPortParam.nBufferCountActual = 6;
    }
    else if(isInput == OMX_FALSE && nPortIndex == 1)
    {
        //OUTPUT
        omx_base_video_Port->pSensorMode->bOneShot = OMX_TRUE;//Default is Video Mode
        setHeader(&omx_base_video_Port->sImageParam, sizeof(OMX_IMAGE_PARAM_PORTFORMATTYPE));
        omx_base_video_Port->sImageParam.nPortIndex = nPortIndex;
        omx_base_video_Port->sImageParam.nIndex = 0;
        omx_base_video_Port->sImageParam.eCompressionFormat = OMX_VIDEO_CodingUnused;
        omx_base_video_Port->sImageParam.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;

        omx_base_video_Port->sPortParam.eDomain = OMX_PortDomainImage;
#ifndef _OPENMAX_V1_2_

        if(omx_base_video_Port->sPortParam.format.video.cMIMEType)
        {
            free(omx_base_video_Port->sPortParam.format.video.cMIMEType);
            omx_base_video_Port->sPortParam.format.video.cMIMEType = NULL;
        }

        omx_base_video_Port->sPortParam.format.image.cMIMEType = malloc(DEFAULT_MIME_STRING_LENGTH);
        strcpy(omx_base_video_Port->sPortParam.format.image.cMIMEType, "video/camera");
#endif
        omx_base_video_Port->sPortParam.format.image.pNativeRender = 0;
        omx_base_video_Port->sPortParam.format.image.bFlagErrorConcealment = OMX_FALSE;
        omx_base_video_Port->sPortParam.format.image.eCompressionFormat = OMX_VIDEO_CodingUnused;

        omx_base_video_Port->sPortParam.format.image.nFrameWidth = 0;
        omx_base_video_Port->sPortParam.format.image.nFrameHeight = 0;
        omx_base_video_Port->sPortParam.format.image.nStride = 0;
        omx_base_video_Port->sPortParam.format.image.nSliceHeight = 0;
        omx_base_video_Port->sPortParam.format.image.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        omx_base_video_Port->sPortParam.format.image.pNativeWindow = NULL;
        //Image Capture
        omx_base_video_Port->sPortParam.nBufferCountMin = 2;
        omx_base_video_Port->sPortParam.nBufferCountActual = 2;
    }

    omx_base_video_Port->pBufferHeadAct = NULL;
    //Module_Cap_Init(omx_base_video_Port,nPortIndex);

    omx_base_video_Port->omx_camera = calloc(1, sizeof(OMX_CAMERATYPE));
    omx_base_video_Port->omx_camera->nSize = sizeof(OMX_CAMERATYPE);
    omx_base_video_Port->omx_camera->pApplicationPrivate = (void *)omx_base_video_Port;
    omx_base_video_Port->omx_camera->pIppHnale = omx_base_video_Port->device_handle;

    omx_base_video_Port->sPortParam.nBufferSize = DEFAULT_WIDTH * DEFAULT_HEIGHT * 3 / 2 + 4096;
    omx_base_video_Port->PortDestructor = &camera_video_port_Destructor;
    omx_base_video_Port->Port_AllocateBuffer = &camera_video_port_AllocateBuffer;
    omx_base_video_Port->Port_UseBuffer = &camera_video_port_UseBuffer;
    omx_base_video_Port->Port_FreeBuffer = &camera_video_port_FreeBuffer;

    return OMX_ErrorNone;
}

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

OMX_ERRORTYPE camera_video_port_Destructor(omx_base_PortType *openmaxStandPort)
{
    omx_base_camera_video_PortType *omx_base_filter_Private;
    omx_base_filter_Private = (omx_base_camera_video_PortType *)openmaxStandPort;

#ifndef _OPENMAX_V1_2_

    if(openmaxStandPort->sPortParam.format.image.cMIMEType)
    {
        free(openmaxStandPort->sPortParam.format.image.cMIMEType);
        openmaxStandPort->sPortParam.format.image.cMIMEType = NULL;
    }

    if(openmaxStandPort->sPortParam.format.video.cMIMEType)
    {
        free(openmaxStandPort->sPortParam.format.video.cMIMEType);
        openmaxStandPort->sPortParam.format.video.cMIMEType = NULL;
    }

#endif

    if(omx_base_filter_Private->pAFStatusL)
    {
        free(omx_base_filter_Private->pAFStatusL);
        omx_base_filter_Private->pAFStatusL = NULL;
    }

    if(omx_base_filter_Private->pHdrParam)
    {
        free(omx_base_filter_Private->pHdrParam);
        omx_base_filter_Private->pHdrParam = NULL;
    }

    if(omx_base_filter_Private->pAFRegionL)
    {
        free(omx_base_filter_Private->pAFRegionL);
        omx_base_filter_Private->pAFRegionL = NULL;
    }

    if(omx_base_filter_Private->pAFStatus)
    {
        free(omx_base_filter_Private->pAFStatus);
        omx_base_filter_Private->pAFStatus = NULL;
    }

    if(omx_base_filter_Private->pAFRegion)
    {
        free(omx_base_filter_Private->pAFRegion);
        omx_base_filter_Private->pAFRegion = NULL;
    }

    if(omx_base_filter_Private->pExpVal)
    {
        free(omx_base_filter_Private->pExpVal);
        omx_base_filter_Private->pExpVal = NULL;
    }

    if(omx_base_filter_Private->pSat)
    {
        free(omx_base_filter_Private->pSat);
        omx_base_filter_Private->pSat = NULL;
    }

    if(omx_base_filter_Private->pBright)
    {
        free(omx_base_filter_Private->pBright);
        omx_base_filter_Private->pBright = NULL;
    }

    if(omx_base_filter_Private->act_agc)
    {
        free(omx_base_filter_Private->act_agc);
        omx_base_filter_Private->act_agc = NULL;
    }

    if(omx_base_filter_Private->act_flashstrobe)
    {
        free(omx_base_filter_Private->act_flashstrobe);
        omx_base_filter_Private->act_flashstrobe = NULL;
    }

    if(omx_base_filter_Private->pContrast)
    {
        free(omx_base_filter_Private->pContrast);
        omx_base_filter_Private->pContrast = NULL;
    }

    if(omx_base_filter_Private->pExpType)
    {
        free(omx_base_filter_Private->pExpType);
        omx_base_filter_Private->pExpType = NULL;
    }

    if(omx_base_filter_Private->pWBType)
    {
        free(omx_base_filter_Private->pWBType);
        omx_base_filter_Private->pWBType = NULL;
    }

    if(omx_base_filter_Private->pOpticZoomType)
    {
        free(omx_base_filter_Private->pOpticZoomType);
        omx_base_filter_Private->pOpticZoomType = NULL;
    }

    if(omx_base_filter_Private->pImageMirror)
    {
        free(omx_base_filter_Private->pImageMirror);
        omx_base_filter_Private->pImageMirror = NULL;
    }

    if(omx_base_filter_Private->pImageFilter)
    {
        free(omx_base_filter_Private->pImageFilter);
        omx_base_filter_Private->pImageFilter = NULL;
    }

    if(omx_base_filter_Private->pColorFix)
    {
        free(omx_base_filter_Private->pColorFix);
        omx_base_filter_Private->pColorFix = NULL;
    }

    if(omx_base_filter_Private->pColorEft)
    {
        free(omx_base_filter_Private->pColorEft);
        omx_base_filter_Private->pColorEft = NULL;
    }

    if(omx_base_filter_Private->pSensorMode)
    {
        free(omx_base_filter_Private->pSensorMode);
        omx_base_filter_Private->pSensorMode = NULL;
    }

    if(omx_base_filter_Private->pFocusType)
    {
        free(omx_base_filter_Private->pFocusType);
        omx_base_filter_Private->pFocusType =  NULL;
    }

    if(omx_base_filter_Private->pCapMode)
    {
        free(omx_base_filter_Private->pCapMode);
        omx_base_filter_Private->pCapMode = NULL;
    }

    if(omx_base_filter_Private->pBlitComp)
    {
        free(omx_base_filter_Private->pBlitComp);
        omx_base_filter_Private->pBlitComp = NULL;
    }

    if(omx_base_filter_Private->pFlashType)
    {
        free(omx_base_filter_Private->pFlashType);
        omx_base_filter_Private->pFlashType = NULL;
    }


    if(omx_base_filter_Private->pGamma)
    {
        free(omx_base_filter_Private->pGamma);
        omx_base_filter_Private->pGamma = NULL;
    }

    if(omx_base_filter_Private->pAF_Dis)
    {
        free(omx_base_filter_Private->pAF_Dis);
        omx_base_filter_Private->pAF_Dis = NULL;
    }

    if(omx_base_filter_Private->pSharp_level)
    {
        free(omx_base_filter_Private->pSharp_level);
        omx_base_filter_Private->pSharp_level = NULL;
    }

    if(omx_base_filter_Private->pNs_level)
    {
        free(omx_base_filter_Private->pNs_level);
        omx_base_filter_Private->pNs_level = NULL;
    }

    if(omx_base_filter_Private->pFlicktype)
    {
        free(omx_base_filter_Private->pFlicktype);
        omx_base_filter_Private->pFlicktype = NULL;
    }

    if(omx_base_filter_Private->pCapExtMode)
    {
        free(omx_base_filter_Private->pCapExtMode);
        omx_base_filter_Private->pCapExtMode = NULL;
    }

    if(omx_base_filter_Private->omx_camera)
    {
        if(omx_base_filter_Private->omx_camera->pCameraPrivate && omx_base_filter_Private->bCopy == OMX_FALSE)
        {
            OMXDBUG(OMXDBUG_VERB, "%s,%d\n", __FILE__, __LINE__);
            omx_base_filter_Private->omx_camera->omx_camera_close(omx_base_filter_Private->omx_camera);

            if(omx_base_filter_Private->nSensorType == 0)
            {
                if(omx_base_filter_Private->camera_isp_base_Destructor)
                { omx_base_filter_Private->camera_isp_base_Destructor(omx_base_filter_Private->omx_camera); }
            }
            else
            {
                if(omx_base_filter_Private->camera_direct_base_Destructor)
                { omx_base_filter_Private->camera_direct_base_Destructor(omx_base_filter_Private->omx_camera); }
            }
        }

        free(omx_base_filter_Private->omx_camera);
        omx_base_filter_Private->omx_camera = NULL;
    }

    if(omx_base_filter_Private->queue_dq)
    {
        queue_deinit(omx_base_filter_Private->queue_dq);
        free(omx_base_filter_Private->queue_dq);
        omx_base_filter_Private->queue_dq = NULL;
    }

#if 0

    if(omx_base_filter_Private->dMoudleIsp)
    {
        dlclose(omx_base_filter_Private->dMoudleIsp);
        omx_base_filter_Private->dMoudleIsp = NULL;
    }

    if(omx_base_filter_Private->pBufferQueueInDriver)
    {
        queue_deinit(omx_base_filter_Private->pBufferQueueInDriver);
        free(omx_base_filter_Private->pBufferQueueInDriver);
        omx_base_filter_Private->pBufferQueueInDriver = NULL;
    }

#endif
    base_video_port_Destructor(openmaxStandPort);

    return OMX_ErrorNone;
}
