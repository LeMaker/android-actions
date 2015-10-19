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

    Header Name: dmProcessScriptSession.cc

    General Description: Implementation of DMProcessScriptSession class.

==================================================================================================*/

#include "dmProcessScriptSession.h"
#include "dm_ua_handlecommand.h"
#include "xpl_Logger.h"

/*==================================================================================================
FUNCTION        : DMProcessScriptSession::DMProcessScriptSession

DESCRIPTION     : The class constructor.
ARGUMENT PASSED : sml_ContentType
                  sml_EncodingType
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
DMProcessScriptSession::DMProcessScriptSession()
{   

    inAtomicCommand = FALSE;
    commandCount = 0;
    m_bSessionAborted = FALSE;

    serverSessionId  = 0;
    serverRetryCount = 0;
    clientRetryCount = 0;

    m_nSecState = DM_CLIENT_NO_SERVER_Y_AUTH;
    isServCredsMissing  = FALSE;

#ifdef LOB_SUPPORT
   DMGetData lrgObjData;
   isLargeObjectSupported = FALSE;
   dmTreeObj.Get("./DevDetail/LrgObj", lrgObjData,SYNCML_DM_REQUEST_TYPE_SERVER);
   isLargeObjectSupported =  !lrgObjData.m_oData.compare("false");
   // Get default MaxObjectSize
   DMNode* pNode= dmTreeObj.FindNodeByURI("./DevDetail/Ext/MaxObjSize");
   dmtMaxObjectSize = dmTreeObj.readOneWordFromTree(pNode, 0);
#endif
}

/*==================================================================================================
FUNCTION        : DMProcessScriptSession::~DMProcessScriptSession

DESCRIPTION     : The class destructor.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
DMProcessScriptSession::~DMProcessScriptSession ()
{
    while ( userData.aStatuses.size() > 0 ) 
    {
      smlFreeStatus((SmlStatusPtr_t)userData.aStatuses[0]);
      userData.aStatuses.remove(0);
    }

    while ( userData.aResults.size() > 0 ) 
    {
      smlFreeResults( userData.aResults[0]._pGetExecResult ); 
      userData.aResults[0]._pGetExecResult= NULL;
    
      if ( userData.aResults[0]._type == SYNCML_DM_RESULT_VALUE::Enum_Result_GetStruct || 
          userData.aResults[0]._type == SYNCML_DM_RESULT_VALUE::Enum_Result_GetStructData ) 
      {
        
        if (userData.aResults[0]._oGetStructPos.psRetData ) 
        {
          delete userData.aResults[0]._oGetStructPos.psRetData;
        }
      }
      userData.aResults.remove(0);
    }
}

/*==================================================================================================
FUNCTION        : DMProcessScriptSession::IncCommandCount

DESCRIPTION     : This function will increment the value of data member commandCount by 1.
ARGUMENT PASSED : newCommandCount
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
void
DMProcessScriptSession::IncCommandCount ()
{
    commandCount++;
}

/*==================================================================================================
Function:    DMProcessScriptSession::SetInAtomicCommand

Description: The function will set the value of data member inAtomicCommand.
ARGUMENT PASSED : newInAtomicCommand
OUTPUT PARAMETER: 
RETURN VALUE    :
IMPORTANT NOTES :
==================================================================================================*/
void
DMProcessScriptSession::SetInAtomicCommand (BOOLEAN newInAtomicCommand)
{
    inAtomicCommand = newInAtomicCommand;
}

/*==================================================================================================
Function:    DMProcessScriptSession::GetInAtomicCommand

Description: The function will return the value of data member inAtomicCommand.
ARGUMENT PASSED :                                                                                   
OUTPUT PARAMETER: 
RETURN VALUE    : inAtomicCommand
IMPORTANT NOTES :
==================================================================================================*/
BOOLEAN
DMProcessScriptSession::GetInAtomicCommand()
{
    return inAtomicCommand;
}


/*==================================================================================================
Function:    DMProcessScriptSession::GetServerSessionId

Description: The function will return the value of data member serverSessionId.
ARGUMENT PASSED : 
OUTPUT PARAMETER:                                                                                   
RETURN VALUE    : serverSessionId
IMPORTANT NOTES :
==================================================================================================*/
UINT16
DMProcessScriptSession::GetServerSessionId()
{
    return serverSessionId;
}


/*==================================================================================================
Function:    DMProcessScriptSession::GetSendInstanceId

Description: The function will return the value of data member sendInstanceId.
ARGUMENT PASSED :                                                                                   
OUTPUT PARAMETER:                                                                                   
RETURN VALUE    : sendInstanceId
IMPORTANT NOTES :
==================================================================================================*/
InstanceID_t
DMProcessScriptSession::GetSendInstanceId ()
{
    return sendInstanceId;
}

/*==================================================================================================
Function:    DMProcessScriptSession::SetClientRetryCount

Description: The function will set the clientRetryCount to a new value.
ARGUMENT PASSED :                                                                                   
OUTPUT PARAMETER:                                                                                   
RETURN VALUE    : void
IMPORTANT NOTES :
==================================================================================================*/
void
DMProcessScriptSession::SetClientRetryCount (UINT8 newcount)
{
    clientRetryCount = newcount;
}

/*==================================================================================================
Function:    DMProcessScriptSession::IncClientRetryCount

Description: The function will increment the clientRetryCount by 1.
ARGUMENT PASSED :                                                                                   
OUTPUT PARAMETER:                                                                                   
RETURN VALUE    : void
IMPORTANT NOTES :
==================================================================================================*/
void
DMProcessScriptSession::IncClientRetryCount ()
{
    clientRetryCount++;
}


/*==================================================================================================
FUNCTION        : DMProcessScriptSession::SessionStart

DESCRIPTION     : The UserAgen::SessionStart calls this function after it creates MgmtSession object.
                  The function will perform the following operations:
                  1) Call SessionStart() to setup the DM tree.
                  2) Register the DM engine with the SYNCML toolkit.
                  3) Connect the client with the server.
                  4) Build and send the package one.
ARGUMENT PASSED : p_SessionStart
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMProcessScriptSession::Start(const UINT8 *docInputBuffer,
                               UINT32 inDocSize,
                               BOOLEAN isWBXML,
                               DMBuffer & oResult)
{
    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_FAIL;
    Ret_t                  sml_ret_stat;

    XPL_LOG_DM_SESS_Debug(("Entered DMProcessScriptSession::SessionStart, buf=%x,size=%d, wbxml=%d\n", docInputBuffer, inDocSize, isWBXML));

    XPL_LOG_DM_SESS_Debug(("Entered DMProcessScriptSession::SessionStart, resultbuf=%x", &oResult));

    ret_stat = Init(isWBXML);
    if ( ret_stat != SYNCML_DM_SUCCESS )
        return ret_stat;

    /* Register the DM engine with the SYNCML toolkit. */
    userData.pSessionMng = this;
    userData.pPkgBuilder = &m_oPkgBuilder;
    ret_stat = RegisterDmEngineWithSyncmlToolkit(&userData);
    XPL_LOG_DM_SESS_Debug(("after entered\n"));
    
    if ( ret_stat != SYNCML_DM_SUCCESS )
    {
        XPL_LOG_DM_SESS_Debug(("Exiting: RegisterDmEngineWithSyncmlToolkit failed\n"));
        return (ret_stat);
    }

    
    sml_ret_stat = smlLockWriteBuffer(recvInstanceId, &pWritePos, &workspaceFreeSize);
    XPL_LOG_DM_SESS_Debug(("after smlLockWriteBuffer\n"));
    
    if ( sml_ret_stat != SML_ERR_OK )
        return SYNCML_DM_FAIL;
    
    memcpy(pWritePos,docInputBuffer,inDocSize);
    XPL_LOG_DM_SESS_Debug(("after memcpy\n"));
   
    recvSmlDoc.dataSize = inDocSize;

    ret_stat = ParseMessage();
    XPL_LOG_DM_SESS_Debug(("after ParseMessage\n"));

    if (ret_stat != SYNCML_DM_SUCCESS)
       return ret_stat;


    smlLockReadBuffer(sendInstanceId, &pReadPos, &workspaceUsedSize);
    XPL_LOG_DM_SESS_Debug(("after smlLockReadBuffer\n"));  
 
    /* Set sendSmlDoc point to workspace */
    if ( workspaceUsedSize )
    {
        oResult.assign(pReadPos,workspaceUsedSize);
        if ( oResult.getBuffer() == NULL )
            ret_stat = SYNCML_DM_DEVICE_FULL;
        else
            ret_stat = SYNCML_DM_SUCCESS;
    }

    XPL_LOG_DM_SESS_Debug(("dmProcessScriptSession:: workspaceUsedSize:%d, ret_stat:%d", workspaceUsedSize, ret_stat));
    XPL_LOG_DM_SESS_Debug(("dmProcessScriptSession:: oResult.getBuffer()=%x, size=%d\n", oResult.getBuffer(), oResult.getSize())); 
    return ret_stat;
}

/*==================================================================================================
FUNCTION        : DMProcessScriptSession::ParseMessage

DESCRIPTION     : The UserAgent::TransportMsg will call this function after RecvMessage() call
                  returned.
                  The function will use SyncML toolkit smlProcessData() function to parse and 
                  process commands in the package.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

             
             This method will perform the following operations:
             1) Unlock both of the Toolkit Buffers.
             2) Call smlProcessData for the first command.
             3) Loop around smlProcessData on the next command while checking for Multiple Messages

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
DMProcessScriptSession::ParseMessage()
{
    Ret_t        sml_ret_stat;
    SYNCML_DM_RET_STATUS_T  ret_stat = SYNCML_DM_SUCCESS;

    smlUnlockWriteBuffer(recvInstanceId, recvSmlDoc.dataSize);

    /* Process the first command in the incoming message */
    sml_ret_stat = smlProcessData(recvInstanceId, SML_FIRST_COMMAND);

    if (sml_ret_stat != SML_ERR_OK)
        return(SYNCML_DM_FAIL);

    /* Loop through the remaining commands until we reach the end of the
       current toolkit workspace */
    while (sml_ret_stat != SML_ERR_XLT_END_OF_BUFFER)
    {
        sml_ret_stat = smlProcessData(recvInstanceId, SML_NEXT_COMMAND);
        /* The callback routines set this value when they realize we need
           to begin processing the other buffer. */

        /* Anything wrong with the command, or inMultipleMessageMode is TRUE, break out here.
         * The callbacks set inMultipleMessageMode when they can no longer
         * fit anything into the outgoing message workspace
         */
        if ((sml_ret_stat != SML_ERR_OK) )
            break;
    }
    
    if (sml_ret_stat != SML_ERR_OK && sml_ret_stat != SML_ERR_XLT_END_OF_BUFFER)
    {
        ret_stat = dmTreeObj.GetLockContextManager().ReleaseIDInternal(SYNCML_DM_LOCKID_CURRENT, SYNCML_DM_ROLLBACK);
        ret_stat = SYNCML_DM_FAIL;
        KCDBG("dmProcessScriptSession:: sml_ret_stat:%d, ret_stat:%d", sml_ret_stat, ret_stat);
    }

    return (ret_stat);
}


/*==================================================================================================
FUNCTION        : DMProcessScriptSession::SetToolkitCallbacks

DESCRIPTION     : This function will to set toolkit callback functions.
             
                  
ARGUMENT PASSED :
OUTPUT PARAMETER:
RETURN VALUE    : It returns SYNCML_DM_SUCCESS.
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMProcessScriptSession::SetToolkitCallbacks(SmlCallbacks_t * pSmlCallbacks)
{
    
    pSmlCallbacks->startMessageFunc = HandleStartMessage;
    pSmlCallbacks->endMessageFunc   = HandleEndMessage;

    /* Sync Command callbacks */
    pSmlCallbacks->addCmdFunc       = HandleAddCommand;

    pSmlCallbacks->alertCmdFunc     = HandleAlertCommand;
    pSmlCallbacks->copyCmdFunc      = HandleCopyCommand;
    pSmlCallbacks->deleteCmdFunc    = HandleDeleteCommand;
    pSmlCallbacks->getCmdFunc       = HandleGetCommand;

    pSmlCallbacks->startAtomicFunc   = HandleStartAtomicCommand;
    pSmlCallbacks->endAtomicFunc     = HandleEndAtomicCommand;

    pSmlCallbacks->startSequenceFunc = HandleStartSequenceCommand;
    pSmlCallbacks->endSequenceFunc   = HandleEndSequenceCommand;
   
    pSmlCallbacks->execCmdFunc      = HandleExecCommand;
    pSmlCallbacks->statusCmdFunc    = HandleStatusCommand;
    pSmlCallbacks->replaceCmdFunc   = HandleReplaceCommand;

    /* The transmitChunkFunc callback is required for support of Large Objects. */

    pSmlCallbacks->transmitChunkFunc  = NULL;
    
    return SYNCML_DM_SUCCESS;
}

DMClientServerCreds * DMProcessScriptSession::GetClientServerCreds()
{
    return &clientServerCreds;
}
