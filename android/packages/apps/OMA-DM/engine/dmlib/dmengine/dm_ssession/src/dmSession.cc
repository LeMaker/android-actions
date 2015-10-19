/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*==================================================================================================

    Header Name: DMSession.cc

    General Description: Implementation of DMSession class.

==================================================================================================*/

#include "dmSession.h"

extern "C" {
#include "xpt-b64.h"
#include "stdio.h"
}

#include "xpl_Logger.h"

/*==================================================================================================
FUNCTION        : DMSession::DMSession

DESCRIPTION     : The class constructor.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
DMSession::DMSession()
{   
 
    smlContentType = NULL;
    smlEncodingType = SML_UNDEF;
  
    sendInstanceId = 0xFF;

    pWritePos = NULL;
    pReadPos  = NULL;

    memset( &sendSmlDoc, 0, sizeof(sendSmlDoc));
    memset( &recvSmlDoc, 0, sizeof(recvSmlDoc));

    workspaceFreeSize = 0;
    workspaceUsedSize = 0;

}

/*==================================================================================================
FUNCTION        : DMSession::~DMSession

DESCRIPTION     : The class destructor.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
DMSession::~DMSession ()
{
    /* Free all the Client and Server Credential memory.*/
   UnRegisterDmEngineWithSyncmlToolkit();
   if ( smlContentType ) 
        DmFreeMem(smlContentType);
}

/*==================================================================================================
FUNCTION        : DMSession::Init

DESCRIPTION     : The class constructor.
ARGUMENT PASSED : sml_ContentType
                  sml_EncodingType
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T DMSession::Init(BOOLEAN isWBXML)
{   
    INT32 size;
   
    size = DmStrlen(SYNCML_CONTENT_TYPE_DM_WBXML)+1;
    smlContentType = (UINT8 *)DmAllocMem(size);
    if ( smlContentType == NULL )
        return SYNCML_DM_DEVICE_FULL;
    
    if ( isWBXML )
    {
        DmStrncpy((char *)smlContentType, SYNCML_CONTENT_TYPE_DM_WBXML, size);
        smlEncodingType = SML_WBXML;
    }
    else
    {
        DmStrncpy((char *)smlContentType, SYNCML_CONTENT_TYPE_DM_XML, size);
        smlEncodingType = SML_XML;    
    }    
 
    return SYNCML_DM_SUCCESS;
}

/*==================================================================================================
FUNCTION        : DMSession::SetToolkitCallbacks

DESCRIPTION     : This function will to set toolkit callback functions.
             
                  
ARGUMENT PASSED :
OUTPUT PARAMETER:
RETURN VALUE    : It returns SYNCML_DM_SUCCESS.
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMSession::SetToolkitCallbacks(SmlCallbacks_t * pSmlCallbacks)
{

    pSmlCallbacks->startMessageFunc = NULL;
    pSmlCallbacks->endMessageFunc   = NULL;

    pSmlCallbacks->addCmdFunc       = NULL;

    pSmlCallbacks->alertCmdFunc     = NULL;
    pSmlCallbacks->copyCmdFunc      = NULL;
    pSmlCallbacks->deleteCmdFunc    = NULL;
    pSmlCallbacks->getCmdFunc       = NULL;

    pSmlCallbacks->startAtomicFunc  = NULL;
    pSmlCallbacks->endAtomicFunc    = NULL;

    pSmlCallbacks->startSequenceFunc = NULL;
    pSmlCallbacks->endSequenceFunc   = NULL;
   
    pSmlCallbacks->execCmdFunc      = NULL;
    pSmlCallbacks->statusCmdFunc    = NULL;
    pSmlCallbacks->replaceCmdFunc   = NULL;

    /* The transmitChunkFunc callback is required for support of Large Objects. */
    pSmlCallbacks->transmitChunkFunc  = NULL;

    return SYNCML_DM_SUCCESS;
}

/*==================================================================================================
FUNCTION        : DMSession::RegisterDmEngineWithSyncmlToolkit

DESCRIPTION     : This function will to register the DM Engine with the SyncML Toolkit.
             
                  The function will perform the following operations:
                  1) Call smlInit() to initialize the SyncML Toolkit.
                  2) Allocate all callback functions.
                  3) Call smlInitInstance() to initialize the SyncML instances.
                  
ARGUMENT PASSED :
OUTPUT PARAMETER:
RETURN VALUE    : It returns SYNCML_DM_SUCCESS when the DM Engine is registered with the SyncML 
                  Toolkit successfully, SYNCML_DM_FAIL when any error occurs.
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMSession::RegisterDmEngineWithSyncmlToolkit(VoidPtr_t pUserData)
{
    SmlOptions_t         smlOptions;            /* Structure describing the options and 
                                                 * setting of this syncml process */
    SmlCallbacks_t       smlCallbacks;          /* Structure defining references to the 
                                                 * applications callback implementations */
    SmlInstanceOptions_t smlInstOptionsRecvDoc; /* Structure specifying options to be used for 
                                                   Recv DM doc instance */
    SmlInstanceOptions_t smlInstOptionsSendDoc; /* Structure specifying options to be used for 
                                                   Send DM doc instance */

    Ret_t                sml_ret_stat;
    SYNCML_DM_RET_STATUS_T dm_ret_stat = SYNCML_DM_SUCCESS;

    //fdp101: make sure we don't have un-inited fields in structs...
    memset( &smlOptions, 0, sizeof(smlOptions));
    memset( &smlCallbacks, 0, sizeof(smlCallbacks));
    memset( &smlInstOptionsRecvDoc, 0, sizeof(smlInstOptionsRecvDoc));
    memset( &smlInstOptionsSendDoc, 0, sizeof(smlInstOptionsSendDoc));
    
    /* Set Toolkit options and setting of this syncml process */
    smlOptions.defaultPrintFunc = NULL;

    /* maxWorkspaceAvailMem is the size which all workspaces in total MUST not exceed. Since we have
     * send and recv document exist at the same time, the available memory size is 2 times of the 
     * DM_SMLTK_WORKSPACE_SIZE */
    smlOptions.maxWorkspaceAvailMem = NUMBER_OF_WORKSPACE * g_iDMWorkspaceSize; //DM_SMLTK_WORKSPACE_SIZE;   

    /* Initialize SyncML Toolkit */
    sml_ret_stat = smlInit(&smlOptions);
    if (sml_ret_stat != SML_ERR_OK)
    {
        return (SYNCML_DM_FAIL);
    }

    /* Receiving DM document instance */
    smlInstOptionsRecvDoc.encoding = smlEncodingType; /* Use WBXML or XML encoding */
    smlInstOptionsRecvDoc.workspaceName = (char *)DmAllocMem(WORKSPACE_NAME_LEN);
    DmStrcpy(smlInstOptionsRecvDoc.workspaceName, RECV_WORKSPACE_NAME);
    smlInstOptionsRecvDoc.workspaceSize = g_iDMWorkspaceSize; //DM_SMLTK_WORKSPACE_SIZE;

    /* Sending DM dcoument instance */
    smlInstOptionsSendDoc.encoding = smlEncodingType; /* Use WBXML or XML encoding */
    smlInstOptionsSendDoc.workspaceName = (char *)DmAllocMem(WORKSPACE_NAME_LEN);
    smlLibStrcpy(smlInstOptionsSendDoc.workspaceName, SEND_WORKSPACE_NAME);
    smlInstOptionsSendDoc.workspaceSize = g_iDMWorkspaceSize; //DM_SMLTK_WORKSPACE_SIZE;

    /* Allocate function pointers for callbacks structure */
    /* Protocol Management Callbacks */
    dm_ret_stat = SetToolkitCallbacks(&smlCallbacks);
    
    /* The transmitChunkFunc callback is required for support of Large Objects. */

    smlCallbacks.transmitChunkFunc  = NULL;

    /* Initialize the SyncML instance, get an instance Id,  and assigns to it a workspace buffer in 
       which SyncML will assemble or parse XML documents.
    */
    sml_ret_stat = smlInitInstance(&smlCallbacks, &smlInstOptionsRecvDoc, pUserData, &recvInstanceId);
    if (sml_ret_stat != SML_ERR_OK)
    {
        /* Free the memory and exit now.*/
        DmFreeMem(smlInstOptionsRecvDoc.workspaceName);
        DmFreeMem(smlInstOptionsSendDoc.workspaceName);
        smlTerminate();
        return (SYNCML_DM_FAIL);
    }
    sml_ret_stat = smlInitInstance(&smlCallbacks, &smlInstOptionsSendDoc, pUserData, &sendInstanceId);
    if (sml_ret_stat != SML_ERR_OK)
    {
        /* Undo the first instance, free the memory and exit.*/
        smlTerminateInstance(recvInstanceId);
        DmFreeMem(smlInstOptionsRecvDoc.workspaceName);
        DmFreeMem(smlInstOptionsSendDoc.workspaceName);
        smlTerminate();
        return (SYNCML_DM_FAIL);
    }

    /* Free the memory which we allocated */
    DmFreeMem(smlInstOptionsRecvDoc.workspaceName);
    DmFreeMem(smlInstOptionsSendDoc.workspaceName);

    return (dm_ret_stat);
}

/*==================================================================================================
FUNCTION        : DMSession::UnRegisterDmEngineWithSyncmlToolkit

DESCRIPTION     : This function will be called by the SYNCML_DM_Session::SessionEnd() to un-register
                  the DM Engine with the SyncML Toolkit.
             
                  The function will perform the following operations:
                  1) Call smlTerminateInstance() to close the SyncML instance.
                  2) Call smlTerminate() to close the Toolkit session.
             
ARGUMENT PASSED :
OUTPUT PARAMETER:
RETURN VALUE    : It returns SML_ERR_OK when the DM Engine is un-registered with the SyncML Toolkit 
                  successfully, error code when any error occurs.
IMPORTANT NOTES :
==================================================================================================*/
void
DMSession::UnRegisterDmEngineWithSyncmlToolkit()
{
    /* Close SyncML instance and Toolkit session */
    smlTerminateInstance(recvInstanceId);
    smlTerminateInstance(sendInstanceId);
    smlTerminate();
}
