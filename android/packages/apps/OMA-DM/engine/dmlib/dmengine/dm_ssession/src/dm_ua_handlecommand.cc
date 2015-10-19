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

    Header Name: dm_ua_handlecommand.cc

    General Description: Implementation of SyncML toolkit callback functions.

==================================================================================================*/

#include "dmstring.h"
#include "dmStringUtil.h"
#include "dm_tree_util.h"
#include "xpl_dm_Manager.h"
#include "dm_ua_handlecommand.h"
#include "dmProcessScriptSession.h"
#include "dmServerSession.h"
#include "dm_security.h"
#include "dmLockingHelper.h"
#include "SYNCML_DM_DisplayAlert.H"
#include "SYNCML_DM_ConfirmAlert.H"
#include "SYNCML_DM_TextInputAlert.H"
#include "SYNCML_DM_SingleChoiceAlert.H"
#include "SYNCML_DM_MultipleChoiceAlert.H"
#include "xpl_Logger.h"

#include "xlttags.h"

extern "C" {
#include "smlerr.h"
#include "xpt-b64.h"
}

/*==================================================================================================
                                 TYPEDEFS
==================================================================================================*/
/* Note that the order of this table MUST match the order of the SYNCML_DM_COMMAND_T enum in
 * syncml_dm_data_types.h
 */
static const CPCHAR dm_command_name_table[] = {
  "", //SYNCML_DM_NO_COMMAND
  "Add",  //SYNCML_DM_ADD
  "Delete", //SYNCML_DM_DELETE
  "Replace",  //SYNCML_DM_REPLACE
  "Get",  //SYNCML_DM_GET
  "Rename", //SYNCML_DM_RENAME
  "Exec", //SYNCML_DM_EXEC
  "Copy", //SYNCML_DM_COPY
  "Alert",  //SYNCML_DM_ALERT
  "SyncHdr",  //SYNCML_DM_HEADER
  "Status", //SYNCML_DM_STATUS
  "Atomic", //SYNCML_DM_ATOMIC
  "Sequence" //SYNCML_DM_SEQUENCE
};

static SYNCML_DM_BuildPackage *pDmBuildPackage;
static DMProcessScriptSession *pDmMgmtSessionObj;

/* Since SyncHdr STATUS doesn't get constructed until the receiving package's STATUS command is
 * handled, we need to save synchdr_dm_stat when the starting message is handled. Keep it until
 * SyncHdr STATUS is constructed. */
static SYNCML_DM_RET_STATUS_T synchdr_dm_stat = SYNCML_DM_SUCCESS;

static SYNCML_DM_CHAL_TYPE_T s_nSrvSecLevel = SYNCML_DM_CHAL_UNDEFINED;

/*==================================================================================================
                                 LOCAL FUNCTION PROTOTYPES
==================================================================================================*/


/*==================================================================================================
                                     LOCAL FUNCTIONS
==================================================================================================*/
static inline SYNCML_DM_RET_STATUS_T SyncML2DMCode( const char* szSyncMLCode )
{
  SYNCML_DM_RET_STATUS_T  nRet = DmAtoi( szSyncMLCode );

  if ( nRet == 200 )
    return SYNCML_DM_SUCCESS;

  return nRet;
}



/*==================================================================================================
                                 FUNCTIONS
==================================================================================================*/

/*==================================================================================================
FUNCTION        : SetMetaData

DESCRIPTION     : The utility function to set up the meta data.

ARGUMENT PASSED : p_meta_info
OUTPUT PARAMETER: pp_type
                  p_format
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_RET_STATUS_T SetMetaData(SmlPcdataPtr_t p_meta_data,
                                    DMBuffer& pp_type,
                                    SYNCML_DM_FORMAT_T *p_format)
{
    SmlMetInfMetInfPtr_t p_meta_info;
    CPCHAR p_temp_content;

    /* Setup OUTPUT parameter type and format. If p_meta_data is not set, assign default value
       to OUTPUT parameters, type as "text/plain", format as "chr". */
    if (p_meta_data != NULL)
    {
        if (p_meta_data->content != NULL)
        {
            if ( SML_PCDATA_EXTENSION != p_meta_data->contentType &&
                 SML_EXT_METINF != p_meta_data->extension )
            {
                *p_format = SYNCML_DM_FORMAT_CHR;
                pp_type.assign("text/plain");
            }
            else
            {
                p_meta_info = (SmlMetInfMetInfPtr_t)p_meta_data->content;
                if ((p_meta_info->format != NULL) &&
                   (p_meta_info->format->length != 0))
                {
                    p_temp_content = (CPCHAR)p_meta_info->format->content;
                    *p_format = DMTree::ConvertFormatStr(p_temp_content);
                }
                else
                {
                    /* If there is no format information, set p_format as default format "chr" */
                    *p_format = SYNCML_DM_FORMAT_CHR;
                }
                /* Set p_temp_type to the passed in type */
                if((p_meta_info->type != NULL) && (p_meta_info->type->length != 0))
                {
                    pp_type.assign((UINT8*)p_meta_info->type->content,p_meta_info->type->length);
                }
                else
                {
                    /* If there is no type information, set the type as 'text/plain' */
                    pp_type.assign("text/plain");
                }
            }
        }
    }
    else
    {
        *p_format = SYNCML_DM_FORMAT_CHR;
        pp_type.assign("text/plain");
    }

    if ( pp_type.getBuffer() == NULL )
        return SYNCML_DM_DEVICE_FULL;

    return SYNCML_DM_SUCCESS;
}


/*==================================================================================================
FUNCTION        : SetExecResultsData

DESCRIPTION     : The utility function to set up the results data for Exec command.

ARGUMENT PASSED : p_passedin_result_item. p_target_uri
                  p_exec_ret_data
OUTPUT PARAMETER: p_passedin_result_item

RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
SetExecResultsData(SmlItemPtr_t  p_passedin_result_item,
               CPCHAR pURI,
               const DMString & execResults)
{
    Ret_t sml_ret_stat = SML_ERR_OK;
    char data_size_str[UINT32_TYPE_STR_SIZE_10];


    p_passedin_result_item->source = smlAllocSource();
    if ( p_passedin_result_item->source == NULL )
        return SYNCML_DM_FAIL;

    pDmBuildPackage->BuildPcData(p_passedin_result_item->source->locURI, SML_PCDATA_STRING,
                                       SML_EXT_UNDEFINED,DmStrlen(pURI),(UINT8*)pURI);

    /* Convert the dwRetDataSize to a string */
    DmSprintf(data_size_str, "%d", execResults.length());
    p_passedin_result_item->meta = smlAllocPcdata();
    if ( p_passedin_result_item->meta == NULL )
    {
        return SYNCML_DM_FAIL;
    }


        pDmBuildPackage->BuildMetaInfo(
            p_passedin_result_item->meta,
            NULL, NULL, NULL,
            (UINT8*)data_size_str,
            NULL, NULL, NULL, NULL);

    /* Set the p_passedin_result_item->data */
        /* Client construct <Data> element no matter there are data from GET command or not. */
        p_passedin_result_item->data = smlAllocPcdata();
        if ( p_passedin_result_item->data == NULL )
        {
            return SYNCML_DM_FAIL;
        }

        pDmBuildPackage->BuildPcData(p_passedin_result_item->data, SML_PCDATA_STRING,
                                           SML_EXT_UNDEFINED,execResults.length(),(UINT8*)execResults.c_str());
    return sml_ret_stat;
}


static SYNCML_DM_RET_STATUS_T SaveStatus(UINT8          *p_CmdRefData,
                                    UINT8          *p_CmdName,
                                    UINT8          *p_SourceRefData,
                                    UINT8          *p_TargetRefData,
                                    UINT16          status_Code,
                                    const DMStringVector*           responses,
                                    SYNCML_DM_USER_DATA_T   *pUserData )
{
  SmlStatusPtr_t pStatus = pDmBuildPackage->AllocateStatus(
        p_CmdRefData, p_CmdName, p_SourceRefData, p_TargetRefData, NULL,
        status_Code, responses );

  if ( !pStatus )
    return SYNCML_DM_FAIL;

  pUserData->aStatuses.push_back((UINT32)pStatus);
  return SYNCML_DM_SUCCESS;
}


static SYNCML_DM_RET_STATUS_T
SaveCommandRefStatus(UINT8  *p_CmdRefData,
                                    UINT8          *p_CmdName,
                                    SmlItemPtr_t  pCommandItem,
                                    SYNCML_DM_RET_STATUS_T   status_Code,
                                    SYNCML_DM_USER_DATA_T   *pUserData )
{

  UINT8  *p_SourceRefData = NULL;
  UINT8  *p_TargetRefData = NULL;
  SmlStatusPtr_t pStatus = NULL;

  if(pCommandItem->target != NULL && pCommandItem->target->locURI != NULL)
        p_TargetRefData = (UINT8 *)pCommandItem->target->locURI->content;
  else
  {
       if ( status_Code == SYNCML_DM_SUCCESS )
                  status_Code = SYNCML_DM_BAD_REQUEST;
  }

  if (pCommandItem->source != NULL &&  pCommandItem->source->locURI != NULL)
       p_SourceRefData = (UINT8*)pCommandItem->source->locURI->content;

  return SaveStatus(p_CmdRefData,
                                 p_CmdName,
                                 p_SourceRefData,
                                 p_TargetRefData,
                                 status_Code,
                                 NULL,
                                 pUserData);

  if ( !pStatus )
    return SYNCML_DM_FAIL;

  pUserData->aStatuses.push_back((UINT32)pStatus);
  return SYNCML_DM_SUCCESS;
}

static SYNCML_DM_RET_STATUS_T SaveResult(CPCHAR pStrTargetUri,
                                          CPCHAR p_CmdIdRef,
                                          DMGetData *p_get_ret_data,
                                          BOOLEAN is_ThisGetStructResult,
                                          BOOLEAN isFirstGetStruct,
                                          BOOLEAN isThisGetPropResult,
                                          SYNCML_DM_USER_DATA_T   *pUserData,
                                          UINT8 type, // exec/get/getstruct
                                          const SYNCML_DM_GET_ON_LIST_RET_DATA_T& oGetStructData )
{
  SmlResultsPtr_t  p_results = NULL;
  SmlPcdataPtr_t  p_data = NULL;
  SYNCML_DM_RET_STATUS_T nRes = SYNCML_DM_SUCCESS;
  CPCHAR p_target_uri = pStrTargetUri;

#ifdef TNDS_SUPPORT
  SmlPcdata_t  pcData;
  if ( type == SYNCML_DM_RESULT_VALUE::Enum_Result_GetTnds )
  {
     p_target_uri = oGetStructData._pbURI;
     nRes = pDmBuildPackage->AllocateTndsResult(pStrTargetUri, p_get_ret_data, oGetStructData, &pcData);
     if ( nRes == SYNCML_DM_SUCCESS )
     {
        p_data = &pcData;
     }
  }
#endif

  nRes = pDmBuildPackage->AllocateResult( p_results, p_target_uri,
                                          p_CmdIdRef, p_get_ret_data,
                                          is_ThisGetStructResult, isFirstGetStruct,
                                          isThisGetPropResult, NULL, p_data);

  if ( nRes != SYNCML_DM_SUCCESS )
  {
    if ( NULL != p_results )
    {
       smlFreeResults(p_results);
       p_results = NULL;
    }
    return nRes;
  }

  pUserData->aResults.push_back( SYNCML_DM_RESULT_VALUE( type, p_results,
                                                         oGetStructData, p_CmdIdRef,
                                                         (CPCHAR)pDmBuildPackage->GetMsgRef()) );
  return nRes;
}

static SYNCML_DM_RET_STATUS_T AtomicRollback (VoidPtr_t    userData)
{
    SYNCML_DM_USER_DATA_T   *pUserData = (SYNCML_DM_USER_DATA_T *)userData;
    SYNCML_DM_RET_STATUS_T retStatus=SYNCML_DM_FAIL;

    if( !pUserData->rollback )
    {
       retStatus = dmTreeObj.GetLockContextManager().ReleaseIDInternal( SYNCML_DM_LOCKID_CURRENT, SYNCML_DM_ROLLBACK);

        pUserData->pAtomicStatus.status = SYNCML_DM_ATOMIC_FAILED;

        if (retStatus != SYNCML_DM_SUCCESS)
          retStatus = SYNCML_DM_ATOMIC_ROLLBACK_FAILED;
        else
          retStatus = SYNCML_DM_ATOMIC_ROLLBACK_OK;

        for ( int i = 0; i <pUserData->oStatus.size(); i++ )
          pUserData->oStatus[i].status = retStatus;

        pUserData->rollback = TRUE;
    }
    return retStatus;
}


static void SequenceStatus(VoidPtr_t userData, SYNCML_DM_RET_STATUS_T status)
{
    SYNCML_DM_USER_DATA_T  *pUserData = (SYNCML_DM_USER_DATA_T *)userData;

    if ( pUserData->IsSequence() )
    {
        if ( status != SYNCML_DM_SUCCESS )
            pUserData->sequenceFailed = TRUE;
    }
}

static void  CheckAuthorization( SmlCredPtr_t pCred )
{
  DMClientServerCreds *pClientServerCreds = pDmMgmtSessionObj->GetClientServerCreds();
  SmlMetInfMetInfPtr_t pMeta = NULL;

  // check default required level
  if ( s_nSrvSecLevel == SYNCML_DM_CHAL_UNDEFINED )
  {
    CPCHAR szStr = XPL_DM_GetEnv(SYNCML_DM_SECURITY_LEVEL);
    if ( !szStr )
      s_nSrvSecLevel = SYNCML_DM_CHAL_BASIC;
    else {
      s_nSrvSecLevel = DmAtoi(szStr);

      if ( s_nSrvSecLevel < SYNCML_DM_CHAL_NONE ||
        s_nSrvSecLevel > SYNCML_DM_CHAL_HMAC )
        s_nSrvSecLevel = SYNCML_DM_CHAL_BASIC;
    }
  }

  if ( pClientServerCreds->ServerChalType <= SYNCML_DM_CHAL_NONE )
    pClientServerCreds->SetPrefServerAuth(s_nSrvSecLevel);  // use default only  if it's not set yet

  if ( pClientServerCreds->ServerChalType == SYNCML_DM_CHAL_HMAC )
    return; // authorization performed in onStatus handler

  // if none - just authorize at once, but if server provides credentials - verify it
  if ( pClientServerCreds->ServerChalType == SYNCML_DM_CHAL_NONE )
    pDmMgmtSessionObj->SetSecStateSrv( TRUE );

  if ( !pCred || !pCred->data || !pCred->meta)
      return; // no credentials

  const char* szType = NULL;
  const char* szData = NULL;

  szData = (const char*)pCred->data->content;

  if ( pCred->meta->contentType == SML_PCDATA_EXTENSION &&
    pCred->meta->extension == SML_EXT_METINF ){
    pMeta = (SmlMetInfMetInfPtr_t)pCred->meta->content;

    if ( pMeta && pMeta->type && pMeta->type->contentType == SML_PCDATA_STRING )
      szType =  (const char*)pMeta->type->content;
  }

  if ( !szType || !szData )
    return;

  SYNCMLDM_SEC_CREDENTIALS_T  *pGenCred = NULL;

  if ( DmStrcmp(szType, SYNCML_AUTH_BASIC) == 0 )
  {
        if ( pClientServerCreds->ServerChalType > SYNCML_DM_CHAL_BASIC)
          return; // basic is not allowed

        pClientServerCreds->SetPrefServerAuth(SYNCML_DM_CHAL_BASIC);  // use basic after that for this session

        SYNCMLDM_BASIC_SEC_INFO_T   basicSecInfo;

        basicSecInfo.pb_password = (UINT8*)pClientServerCreds->pServerPW.c_str();
        basicSecInfo.pb_user_name_or_server_id = (UINT8*)pClientServerCreds->pServerId.c_str();

        pGenCred = syncmldm_sec_build_basic_cred(&basicSecInfo);
  }
  else
      if ( DmStrcmp(szType, SYNCML_AUTH_MD5) == 0 )
      {
            if ( pClientServerCreds->ServerChalType > SYNCML_DM_CHAL_MD5)
              return; // MD5 digest is not allowed

            pClientServerCreds->SetPrefServerAuth( SYNCML_DM_CHAL_MD5 );  // use MD5 after that for this session

            SYNCMLDM_MD5_SEC_INFO_T     md5SecInfo;
            UINT8                       decodedNonce[MAX_BIN_NONCE_LEN];
            UINT32                      encodedNonceLen;
            UINT32                      decodedNonceLen;

            /* The ClientNonce string is b64 encoded and must be decoded now.*/
            encodedNonceLen = DmStrlen((const char *)pClientServerCreds->pServerNonce);
            decodedNonceLen = base64Decode((unsigned char *)decodedNonce,
                                    MAX_BIN_NONCE_LEN,
                                    (unsigned char*)pClientServerCreds->pServerNonce.c_str(),
                                    (unsigned long*)&encodedNonceLen);
            md5SecInfo.pb_user_name_or_server_id = (UINT8*)pClientServerCreds->pServerId.c_str();
            md5SecInfo.pb_password = (UINT8*)pClientServerCreds->pServerPW.c_str();
            md5SecInfo.pb_nonce = decodedNonce;
            md5SecInfo.o_encode_base64 = FALSE;
            if ( pMeta->format )
                md5SecInfo.o_encode_base64 = TRUE;
            md5SecInfo.w_nonce_length = decodedNonceLen;

            pGenCred = syncmldm_sec_build_md5_cred(&md5SecInfo);
      }

  if ( !pGenCred )
    return;

  if ( memcmp(szData, (const char*)pGenCred->ab_credential_string, pGenCred->w_credential_string_length) == 0 )
    pDmMgmtSessionObj->SetSecStateSrv( TRUE );
  else
    pDmMgmtSessionObj->SetSecStateSrv( FALSE );

  DmFreeMem(pGenCred);
}

/*==================================================================================================
FUNCTION        : HandleEndMessage

DESCRIPTION     : This method calls SyncML Toolkit smlEndMessage to add the Final element in the
                  SyncBody for the sending message.
ARGUMENT PASSED : id
                  userData
                  final
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/

Ret_t
HandleEndMessage (InstanceID_t id,
                  VoidPtr_t    userData,
                  Boolean_t    final)
{
  SYNCML_DM_USER_DATA_T   *pUserData = (SYNCML_DM_USER_DATA_T *)userData;
  SYNCML_DM_RET_STATUS_T  ret_stat = SYNCML_DM_SUCCESS;
  BOOLEAN isLastChunk = FALSE;

 // Is session aborted?
 if( pDmMgmtSessionObj->IsSessionAborted())
         return SYNCML_DM_SUCCESS;

  // put cahced statuses (if any)
  while ( ret_stat == SYNCML_DM_SUCCESS && pUserData->aStatuses.size() > 0 ) {
    ret_stat = pDmBuildPackage->BuildStatus( (SmlStatusPtr_t)pUserData->aStatuses[0] );
    if ( ret_stat == SYNCML_DM_SUCCESS ){
      smlFreeStatus((SmlStatusPtr_t)pUserData->aStatuses[0]);
      pUserData->aStatuses.remove(0);
    }
  }
 // Large Object delivery
#ifdef LOB_SUPPORT
if( !pDmBuildPackage->LargeObjectSendNextChunk(ret_stat, isLastChunk))
#endif
 {
#ifdef LOB_SUPPORT
  if(ret_stat != SYNCML_DM_SUCCESS)
  {        pUserData->aResults.remove(0);
        isLastChunk = FALSE;
  }
#endif

  // pur cached results
  while ( ret_stat == SYNCML_DM_SUCCESS && pUserData->aResults.size() > 0 ) {
        // Large Object delivery
#ifdef LOB_SUPPORT
        if(!isLastChunk)
                ret_stat =  pDmBuildPackage->LargeObjectSendFirstChunk(pUserData->aResults[0]);
        isLastChunk = FALSE;
#else
    // try to insert result first
    ret_stat = pDmBuildPackage->BuildResultsCommand(pUserData->aResults[0]._pGetExecResult);
#endif
    if ( ret_stat == SYNCML_DM_RESULTS_TOO_LARGE )
      break;
#ifndef LOB_SUPPORT
    smlFreeResults( pUserData->aResults[0]._pGetExecResult ); pUserData->aResults[0]._pGetExecResult= NULL;
#endif

    if ( pUserData->aResults[0]._type == SYNCML_DM_RESULT_VALUE::Enum_Result_GetStruct ||
        pUserData->aResults[0]._type == SYNCML_DM_RESULT_VALUE::Enum_Result_GetStructData )
    {
            ret_stat = dmTreeObj.GetListNextItem(pUserData->aResults[0]._oGetStructPos);

        if (ret_stat == SYNCML_DM_SUCCESS && pUserData->aResults[0]._oGetStructPos.psRetData ) {
            ret_stat = pDmBuildPackage->AllocateResult(
              pUserData->aResults[0]._pGetExecResult,
              pUserData->aResults[0]._oGetStructPos._pbURI,
              pUserData->aResults[0]._cmdRef,
              pUserData->aResults[0]._oGetStructPos.psRetData,
              pUserData->aResults[0]._type == SYNCML_DM_RESULT_VALUE::Enum_Result_GetStruct,
              FALSE, FALSE,
              pUserData->aResults[0]._msgID, NULL );


          delete pUserData->aResults[0]._oGetStructPos.psRetData;
          pUserData->aResults[0]._oGetStructPos.psRetData = NULL;
              if(ret_stat != SYNCML_DM_SUCCESS)
                      pUserData->aResults.remove(0);

          continue; // write more data
        }
    }
    pUserData->aResults.remove(0);
  }
}
 pDmBuildPackage->EndSyncmlDoc( ret_stat != SYNCML_DM_RESULTS_TOO_LARGE );
 pDmBuildPackage->Cleanup();

  return SML_ERR_OK;
}


/*==================================================================================================
FUNCTION        : VerifyProtocolVersion

DESCRIPTION     :
ARGUMENT PASSED : pContent
OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_SUCCESS or SYNCML_DM_FAIL code
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_RET_STATUS_T
VerifyProtocolVersion( SmlSyncHdrPtr_t pContent)
{
        if ( dmTreeObj.IsVersion_12()  )
           {
           /* Verify that the presentation protocol is what we support. */
                if (DmStrcmp((const char *)pContent->version->content, SYNCML_REP_PROTOCOL_VERSION_1_2) != 0)
                        return SYNCML_DM_FAIL;
                    /* Verify that the DM protocol is what we support. */
                  if (DmStrcmp((const char *)pContent->proto->content, SYNCML_DM_PROTOCOL_VERSION_1_2) != 0)
                         return SYNCML_DM_FAIL;
            }
            else
            {
                      /* Verify that the presentation protocol is what we support. */
                    if (DmStrcmp((const char *)pContent->version->content, SYNCML_REP_PROTOCOL_VERSION_1_1) != 0)
                        return SYNCML_DM_FAIL;
                /* Verify that the DM protocol is what we support. */
                  if (DmStrcmp((const char *)pContent->proto->content, SYNCML_DM_PROTOCOL_VERSION_1_1) != 0)
                        return SYNCML_DM_FAIL;
       }
       return SYNCML_DM_SUCCESS;


}

/*==================================================================================================
FUNCTION        : HandleStartMessage

DESCRIPTION     : This function should analyze SyncHeader data from received DM document, and build
                  responding SyncML header.
ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleStartMessage (InstanceID_t    id,
                    VoidPtr_t       userData,
                    SmlSyncHdrPtr_t pContent)
{

    SYNCML_DM_RET_STATUS_T ret_stat;
    UINT32             temp_max_msg_size;
    SmlMetInfMetInfPtr_t p_temp_meta_info;
    DMString           strRespUri;
    UINT16             server_session_id;
    SYNCML_DM_USER_DATA_T *pUserData = (SYNCML_DM_USER_DATA_T *)userData;

    /* Reset the synchdr_dm_stat for this message.*/
    synchdr_dm_stat = SYNCML_DM_SUCCESS;

    /* Get the Session Object.*/
    pDmMgmtSessionObj = pUserData->pSessionMng;
    pDmBuildPackage = pUserData->pPkgBuilder;
    // ignore session ID when processing XML script
    if(pDmMgmtSessionObj->IsProcessScript() == FALSE)
    {
        // Verify that this is the same session, hex string
       if(pDmBuildPackage->IsSessionId() == TRUE)
       {
         // Verify that this is the same session, hex string
         server_session_id = DmStrtol((const char *)pContent->sessionID->content, NULL, 16 );
       }
       else
       {
         // Verify that this is the same session, dec string
         server_session_id = DmStrtol((const char *)pContent->sessionID->content, NULL, 10 );
       }

        if (server_session_id != pDmMgmtSessionObj->GetServerSessionId())
        {
            smlFreeSyncHdr(pContent);
            return SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
    }

    ret_stat = VerifyProtocolVersion(pContent);
    if ( ret_stat != SYNCML_DM_SUCCESS )
    {
            smlFreeSyncHdr(pContent);
             return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    /* Create the BuildPackage object instance. */
    pDmBuildPackage->Init(pUserData->pSessionMng);

    /* If respURI exists, call transport API to set the URI. */
    if ((pContent->respURI != NULL) && (pContent->respURI->length != 0))
    {
        /* Copy over the response URI */
       strRespUri.assign((const char *)pContent->respURI->content,  pContent->respURI->length);
       if(strRespUri.Encode() == FALSE)
            synchdr_dm_stat = SYNCML_DM_PROCESSING_ERROR;

        /* Call the MgmtSessionObj to set the response URI */
       ret_stat = pDmMgmtSessionObj->SetURI(strRespUri.c_str() );
       if (ret_stat != SYNCML_DM_SUCCESS)
       {
            synchdr_dm_stat = SYNCML_DM_PROCESSING_ERROR;
       }
    }

    /* If the MaxMsgSize is sent by the server, when it's greater than our MaxMsgSize, don't need
       to do anything; if it's smaller than our MaxMsgSize, set our MaxMsgSize to the new value. */
    if ((pContent->meta != NULL) && (pContent->meta->content != NULL))
    {
        p_temp_meta_info = (SmlMetInfMetInfPtr_t)pContent->meta->content;
        if ((p_temp_meta_info->maxmsgsize != NULL) &&
            (p_temp_meta_info->maxmsgsize->length != 0))
        {
            temp_max_msg_size = DmAtoi((const char *)p_temp_meta_info->maxmsgsize->content);
            /* If the Server MaxMsgSize is smaller than what we have, reset our MaxMessageSize */
            if (temp_max_msg_size < pDmBuildPackage->GetMaxMessageSize())
            {
                pDmBuildPackage->SetMaxMessageSize(temp_max_msg_size);
            }
        }
#ifdef LOB_SUPPORT
        if ((p_temp_meta_info->maxobjsize != NULL) &&
                (p_temp_meta_info->maxobjsize->length != 0))
        {
                temp_max_msg_size = DmAtoi((const char *)p_temp_meta_info->maxobjsize->content);
                /* If the Server MaxObjSize is smaller than what we have, reset our MaxObjectSize */
                if (pDmBuildPackage->GetMaxObjectSize()==0 || temp_max_msg_size < pDmBuildPackage->GetMaxObjectSize())
                {
                        pDmBuildPackage->SetMaxObjectSize(temp_max_msg_size);
                }
        }
#endif
    }

    /* Build up the SyncML document header */
    ret_stat = pDmBuildPackage->BuildStartSyncHdr(pContent,FALSE);
    if (ret_stat != SYNCML_DM_SUCCESS)
    {
        synchdr_dm_stat = SYNCML_DM_BAD_REQUEST;
        smlFreeSyncHdr(pContent);
        return SML_ERR_UNSPECIFIC;
    }

    CheckAuthorization( pContent->cred );


    if ( pDmMgmtSessionObj->IsServerAuthorized())
        pDmMgmtSessionObj->ResetServerRetryCount();

    if( pDmMgmtSessionObj->IsProcessScript() )
    {
       ret_stat = pDmBuildPackage->BuildFinishSyncHdr(SYNCML_DM_CHAL_NONE);
       if (ret_stat != SYNCML_DM_SUCCESS)
       {
         synchdr_dm_stat = SYNCML_DM_BAD_REQUEST;
         smlFreeSyncHdr(pContent);
         return SML_ERR_UNSPECIFIC;
       }
    }
    /* We need to check the source and target URI and set the dm_stat to the correct status before
       we free the pContent.*/
    if ((pContent->target->locURI == NULL) || (pContent->source->locURI == NULL) ||
        (pContent->target->locURI->length == 0) || (pContent->source->locURI->length == 0))
    {
        synchdr_dm_stat = SYNCML_DM_PERMISSION_FAILED;
    }

    /* Free the memory */
    smlFreeSyncHdr(pContent);

    return SML_ERR_OK;
}



/*==================================================================================================
FUNCTION        : PrepareCommandItem

DESCRIPTION     : Check if command should be skipped and decode item URI

ARGUMENT PASSED :
                  userData
                  pContent
                  command
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
static SYNCML_DM_RET_STATUS_T
PrepareCommandItem (SmlItemPtr_t  pCommandItem,
                                              SYNCML_DM_USER_DATA_T   *pUserData,
                                              DMString & strCommandUri )
{

        BOOLEAN res;

         if ( pUserData->IsCommandSkipped() )
             return SYNCML_DM_NOT_EXECUTED;

       if ((pCommandItem->target == NULL) || (pCommandItem->target->locURI == NULL))
              return SYNCML_DM_BAD_REQUEST;

        res = strCommandUri.assign((CPCHAR)pCommandItem->target->locURI->content,
                                                    pCommandItem->target->locURI->length);
        if ( res == FALSE )
                return SYNCML_DM_DEVICE_FULL;

       if(strCommandUri.Decode() == FALSE)
              return SYNCML_DM_BAD_REQUEST;

        return SYNCML_DM_SUCCESS;

}

/*==================================================================================================
FUNCTION        : ProcessStatus

DESCRIPTION     : Process status of operationd

ARGUMENT PASSED :
                  userData
                  pContent
                  command
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/

static SYNCML_DM_RET_STATUS_T
ProcessStatus (SYNCML_DM_RET_STATUS_T dm_stat,
                              VoidPtr_t  userData,
                              UINT8  * p_CmdRefData,
                              SmlItemPtr_t  pCommandItem,
                              SYNCML_DM_COMMAND_T command)
{

       SYNCML_DM_USER_DATA_T *pUserData = (SYNCML_DM_USER_DATA_T *)userData;
        SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;

       if ( pCommandItem == NULL )
                   return SYNCML_DM_FAIL;


        UINT8 *pTargetURL = NULL;
    //Fix for Upmerge CR# LIBoo12975
     if((pCommandItem->target != NULL) && (pCommandItem->target->locURI != NULL))
     {
      if(command == SYNCML_DM_GET)
      {
        DMString strTargetEncode;
        strTargetEncode = DMString((const char *)pCommandItem->target->locURI->content, (int)pCommandItem->target->locURI->length);
           if(strTargetEncode.Encode() == FALSE)
           {
            return SYNCML_DM_FAIL;
           }
         pTargetURL  = (UINT8 *)strTargetEncode.c_str();
      }
      else
      {
          pTargetURL = (UINT8 *)pCommandItem->target->locURI->content;
      }
     }

        UINT8* pSourceURL = NULL;
        pSourceURL = (UINT8*)(pCommandItem->source != NULL ? pCommandItem->source->locURI->content : NULL);

       if( pDmMgmtSessionObj->GetInAtomicCommand() )
       {
#ifdef DM_ATOMIC_SUPPORTED
             // Save the data for the status and results
             if(dm_stat != SYNCML_DM_SUCCESS &&  !pUserData->rollback)
                  AtomicRollback(userData);
#endif
              pUserData->oStatus.push_back( SYNCML_DM_STATUS_DATA_T(
                                                                     (CPCHAR)p_CmdRefData,
                                                                     dm_command_name_table[command],
                                                                     (CPCHAR)pSourceURL,
                                                                     (CPCHAR)pTargetURL,
                                                                      dm_stat) );

       }
       else
       {
              ret_stat = SaveCommandRefStatus((UINT8 *)p_CmdRefData,
                                                   (UINT8 *)dm_command_name_table[command],
                                                    pCommandItem,
                                                    dm_stat,
                                                    pUserData );

       }

        return ret_stat;

}

#ifdef TNDS_SUPPORT
SYNCML_DM_RET_STATUS_T ProcessTndsNode( SYNCML_DM_COMMAND_T command,
                                        SmlDmTndNodeListPtr_t p_nodelist,
                                        const DMString &parentURI,
                                        const DMString &targetURI)
{
   SYNCML_DM_RET_STATUS_T ret = SYNCML_DM_SUCCESS;
   DMAddData oCommandData;
   while ( NULL != p_nodelist && ret == SYNCML_DM_SUCCESS)
   {
      SmlDmTndNodePtr_t p_tnd_node = p_nodelist->node;
      if ( p_tnd_node != NULL )
      {
         // node name
         DMString nodeName;
         nodeName = (CPCHAR)p_tnd_node->nodename->content;
         KCDBG("ProcessTndsNode: node name = %s", nodeName.c_str());

         // node format and type
         SYNCML_DM_FORMAT_T nodeFormat = SYNCML_DM_FORMAT_NODE;
         DMString nodeType = "text/plain", nodeValue = "";
         if ( p_tnd_node->rtprops != NULL )
         {
             if ( p_tnd_node->rtprops->format != NULL )
             {
                 nodeFormat = DMTree::ConvertFormatStr((CPCHAR)p_tnd_node->rtprops->format->value->content);
                 if ( (nodeFormat == SYNCML_DM_FORMAT_BIN) && (nodeName == DM_AAUTHDATA) )
                 {
                     nodeFormat = SYNCML_DM_FORMAT_CHR;
                 }
             }
             if ( p_tnd_node->rtprops->type != NULL )
             {
                 if ( p_tnd_node->rtprops->type->mime != NULL )
                 {
                     nodeType = (CPCHAR)p_tnd_node->rtprops->type->mime->content;
                 }
                 else if ( p_tnd_node->rtprops->type->ddfname != NULL )
                 {
                     nodeType = (CPCHAR)p_tnd_node->rtprops->type->ddfname->content;
                 }
             }
         }
         KCDBG("ProcessTndsNode: nodeType = %s", nodeType.c_str());

         DMString nodeTargetURI;
         if ( ( nodeType == MNG_OBJID_DMACC1 ) || (nodeType == MNG_OBJID_DMACC2) )
         {
             KCDBG("ProcessTndsNode: modified Target URI");
             nodeTargetURI = DM_DMACC_1_2_URI;   // TNDS is defined for 1.2 only
         }

         // calculate node path
         DMString nodePath;
         if (!nodeTargetURI.empty())
         {
            nodePath = nodeTargetURI + "/";
         }
         else
         {
            nodePath = parentURI + "/";
         }

         if ( p_tnd_node->path != NULL )
         {
            DMString path = (CPCHAR)p_tnd_node->path->content;
            nodePath = nodeTargetURI + "/" + path + "/";
         }
         nodePath += nodeName;
         KCDBG("ProcessTndsNode: node path = %s", nodePath.c_str());

         // node value
         if ( p_tnd_node->value != NULL )
         {
            if ( DmStrncmp((CPCHAR)p_tnd_node->value->content, DM_INBOX, strlen(DM_INBOX)) == 0 )
            {
               nodeValue = (CPCHAR)p_tnd_node->value->content + strlen(DM_INBOX) + 1;
            }
            else
            {
               nodeValue = (CPCHAR)p_tnd_node->value->content;
            }
         }

         // Construct Add/Replace Data
         oCommandData.clear();
         oCommandData.m_oURI.assign(nodePath.c_str());
         oCommandData.m_nFormat = nodeFormat;
         oCommandData.m_oMimeType.assign(nodeType.c_str());
         oCommandData.m_oData.assign(nodeValue.c_str());

         // Add/Replace node in DMT
         if ( command == SYNCML_DM_ADD )
         {
            KCDBG("ProcessTndsNode: command == ADD");
            ret = dmTreeObj.Add(oCommandData,SYNCML_DM_REQUEST_TYPE_SERVER);
            // Interior node exist
            if ( nodeFormat == SYNCML_DM_FORMAT_NODE  && ret == SYNCML_DM_TARGET_ALREADY_EXISTS )
            {
               ret = SYNCML_DM_SUCCESS;
            }
         }
             else
         {
            KCDBG("ProcessTndsNode: command == REPLACE");
            ret = dmTreeObj.Replace(oCommandData,SYNCML_DM_REQUEST_TYPE_SERVER);
         }

         // Process children node
         if ( ret == SYNCML_DM_SUCCESS )
         {
            if (nodeTargetURI.empty())
            {
                ret = ProcessTndsNode(command, p_tnd_node->nodelist, nodePath, targetURI);
            }
            else
            {
                ret = ProcessTndsNode(command, p_tnd_node->nodelist, nodePath, nodeTargetURI);
            }
         }
         else
         {
            KCDBG("Failed to handle TNDS node: %s, format: %d, value: %s, status:%d\n", nodePath.c_str(), nodeFormat, nodeValue.c_str(), ret);
         }
      }

      // Process sibling node
      p_nodelist = p_nodelist->next;
   }

   return ret;
}

SYNCML_DM_RET_STATUS_T ProcessTndsCommand(
                                        SYNCML_DM_COMMAND_T command,
                                        VoidPtr_t    userData,
                                        DMAddData & oCommand,
                                        DMCommandType cmdType,
                                        SmlItemPtr_t p_command_item)
{
   DMString dataStr;
   if ( NULL == p_command_item->data || NULL == p_command_item->data->content )
   {
      return SML_ERR_UNSPECIFIC;
   }
   if ( SML_PCDATA_EXTENSION !=  p_command_item->data->contentType &&
        SML_EXT_DMTND !=  p_command_item->data->extension )
   {
      return SML_ERR_UNSPECIFIC;
   }

   SmlDmTndPtr_t p_tnd_info = NULL;
   p_tnd_info = (SmlDmTndPtr_t)p_command_item->data->content;
   if ( oCommand.m_oURI.compare(DM_INBOX) )
   {
       oCommand.m_oURI.assign(".");
   }

   SYNCML_DM_RET_STATUS_T dm_stat=dmTreeObj.GetLockContextManager().ReleaseIDInternal(SYNCML_DM_LOCKID_CURRENT, SYNCML_DM_ATOMIC);

   if ( dm_stat != SYNCML_DM_SUCCESS && dm_stat != SYNCML_DM_FEATURE_NOT_SUPPORTED )
   {
     return SML_ERR_UNSPECIFIC;
   }

   /* Remember that we are in an Atomic command.*/
   bool bIsInAtomic = pDmMgmtSessionObj->GetInAtomicCommand();

   pDmMgmtSessionObj->SetInAtomicCommand(TRUE);

   SmlDmTndNodeListPtr_t p_nodelist = p_tnd_info->nodelist;
   DMString path = (CPCHAR)oCommand.m_oURI.getBuffer();
   if ( oCommand.m_oURI.getBuffer()[0] == '/' )
   {
      DMString tmpPath = ".";
      path = tmpPath + path;
   }
   dm_stat = ProcessTndsNode(command, p_nodelist, path.c_str(), path.c_str());
   if ( dm_stat != SYNCML_DM_SUCCESS && dm_stat != SYNCML_DM_FEATURE_NOT_SUPPORTED )
   {
      // Roll back TNDS objects
      AtomicRollback(userData);
   }

   pDmMgmtSessionObj->SetInAtomicCommand(bIsInAtomic);
   return dm_stat;
}
#endif // TNDS_SUPPORT

/*==================================================================================================
FUNCTION        : ProcessCommand

DESCRIPTION     : Process ADD?REPLACE command

                  This function will perform the following operations:
                  1) Call DMTree::Add() or DMTree::Replace() function to perform ADD command on the DM tree.
                  2) Call SYNCML_DM_BuildPackage::BuildStatus() to build up the staus command with
                     return status for each ADD command performed.
ARGUMENT PASSED : id
                  userData
                  pContent
                  command
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t ProcessCommand (InstanceID_t id,
                                              VoidPtr_t    userData,
                                              SmlAddPtr_t  pContent,
                                              SYNCML_DM_COMMAND_T command)
{
    Ret_t       sml_ret_stat = SML_ERR_OK;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;
    SmlItemListPtr_t    p_command_list_item;
    SmlItemPtr_t        p_command_item;
    DMBuffer            oCommandType;
    SYNCML_DM_FORMAT_T  commandFormat;

    DMAddData oCommandData;
    SYNCML_DM_USER_DATA_T *pUserData = (SYNCML_DM_USER_DATA_T *)userData;

    /* Get the data we need to work on */
    p_command_list_item = pContent->itemList;
    p_command_item      = p_command_list_item->item;

    XPL_LOG_DM_TMN_Debug(("dm_ua_handlecommand::ProcessCommand command=%d", command));

   if ( command == SYNCML_DM_ADD )
           dm_stat = pDmBuildPackage->GenerateAlertForLOB(DM_COMMAND_ADD);
   else
           dm_stat = pDmBuildPackage->GenerateAlertForLOB(DM_COMMAND_REPLACE);

    if (dm_stat != SYNCML_DM_SUCCESS)
    {
            ret_stat = ProcessStatus (dm_stat,
                                                   userData,
                                                   (UINT8*)pContent->cmdID->content,
                                                   p_command_item,
                                                   command);
    /* Free the memeory we allocated (p_plugin_add), and passed in (pContent). */
               smlFreeGeneric((SmlGenericCmdPtr_t)pContent);
           return SML_ERR_OK;
    }

    pDmMgmtSessionObj->IncCommandCount(); /* This variable is used to check if syncml document has
                                           * any management commands */

    /* Fill in meta data when meta data info is defined outside of the ITEM in the receiving
       package.
       if meta data is not set, default format "chr" and type "text/plain" will be filled. */
    dm_stat = SetMetaData(pContent->meta, oCommandType, &commandFormat);
    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
        smlFreeGeneric((SmlGenericCmdPtr_t)pContent);
        return SML_ERR_UNSPECIFIC;
    }

    /* Make sure we are not in an atomic and the Server is authenticated before performing any
     * DM commands.*/
    if (pDmMgmtSessionObj->IsAuthorized())
    {
        /* Loop on every ADD ITEM */
        while (p_command_item != NULL)
        {
            oCommandData.clear();

            // perform operation

            while ( TRUE )
            {
                    DMString tempURI;
                dm_stat = PrepareCommandItem (p_command_item,
                                                                    pUserData,
                                                                    tempURI);
                if ( dm_stat != SYNCML_DM_SUCCESS )
                          break;

              oCommandData.m_oURI.assign(tempURI);

              if(oCommandData.m_oURI.getBuffer() == NULL)
              {
                    dm_stat = SYNCML_DM_DEVICE_FULL;
                    break;
              }

              /* Set the meta data for ADD/REPLACE command if it's defined for each ITEM */

              if (p_command_item->meta != NULL)
              {
                 dm_stat = SetMetaData(p_command_item->meta, oCommandData.m_oMimeType, &oCommandData.m_nFormat);
                 if ( dm_stat != SYNCML_DM_SUCCESS )
                      break;
              }
              else
              {
                /* This particular item does not have meta data, so we need to use the meta data
                 * from outside the command.
                 */
                oCommandData.m_oMimeType = oCommandType;
                if ( oCommandData.getType() == NULL )
                {
                     dm_stat = SYNCML_DM_DEVICE_FULL;
                     break;
                }
                oCommandData.m_nFormat = commandFormat;
              }

#ifdef TNDS_SUPPORT
              /* Handle TNDS object */
              if ( oCommandData.m_oMimeType.compare(SYNCML_CONTENT_TYPE_DM_TNDS_XML, strlen(SYNCML_CONTENT_TYPE_DM_TNDS_XML)) ||
                   oCommandData.m_oMimeType.compare(SYNCML_CONTENT_TYPE_DM_TNDS_WBXML, strlen(SYNCML_CONTENT_TYPE_DM_TNDS_WBXML)) )
              {
                  dm_stat = ProcessTndsCommand( command,
                                                userData,
                                                oCommandData,
                                                command == SYNCML_DM_ADD ?
                                                           DM_COMMAND_ADD :
                                                           DM_COMMAND_REPLACE,
                                                p_command_item);
              }
              else
              {
#endif // TNDS_SUPPORT

#ifdef LOB_SUPPORT

                if(pDmBuildPackage->IsProcessingLargeObject())
                        {
                       XPL_LOG_DM_TMN_Debug(("dm_ua_handlecommand::ProcessCommand processing lob\n"));
                   if ( command == SYNCML_DM_ADD )
                                dm_stat = pDmBuildPackage->LargeObjectRecvNextChunk(oCommandData,
                                                                                                                   DM_COMMAND_ADD,
                                                                                                                   p_command_item);
                            else
                                   dm_stat = pDmBuildPackage->LargeObjectRecvNextChunk(oCommandData,
                                                                                                                   DM_COMMAND_REPLACE,
                                                                                                                   p_command_item);


                        }
                else
                        {
                     XPL_LOG_DM_TMN_Debug(("not processing large obj\n"));
                       if ( command == SYNCML_DM_ADD )
                                dm_stat = pDmBuildPackage->LargeObjectRecvFirstChunk(oCommandData,
                                                                                                                           DM_COMMAND_ADD,
                                                                                                                           p_command_item);
                                 else
                                dm_stat = pDmBuildPackage->LargeObjectRecvFirstChunk(oCommandData,
                                                                                                                           DM_COMMAND_REPLACE,
                                                                                                                           p_command_item);

                        }
#else // LOB_SUPPORT

               /* Call TNM module to perform ADD command */
#ifdef DM_ATOMIC_SUPPORTED
               if ( command == SYNCML_DM_ADD ) {
                XPL_LOG_DM_TMN_Debug(("about to add atomic supported\n"));
                       dm_stat = dmTreeObj.Add(oCommandData,SYNCML_DM_REQUEST_TYPE_SERVER);
                XPL_LOG_DM_TMN_Debug(("add atomic supported dm_stat=%d\n", dm_stat));
                }
                else
                        dm_stat = dmTreeObj.Replace(oCommandData,SYNCML_DM_REQUEST_TYPE_SERVER);
#else // DM_ATOMIC_SUPPORTED
               if ( pDmMgmtSessionObj->GetInAtomicCommand() )
                    dm_stat =  SYNCML_DM_COMMAND_FAILED;
               else
               {
                     if ( command == SYNCML_DM_ADD ) {
                     XPL_LOG_DM_TMN_Debug(("about to add atomic not supported\n"));
                                dm_stat = dmTreeObj.Add(oCommandData,SYNCML_DM_REQUEST_TYPE_SERVER);
                         XPL_LOG_DM_TMN_Debug(("add atomic not supported dm_stat=%d\n", dm_stat));
                               }
                        else
                                dm_stat = dmTreeObj.Replace(oCommandData,SYNCML_DM_REQUEST_TYPE_SERVER);
               }
#endif // DM_ATOMIC_SUPPORTED
#endif // LOB_SUPPORT

#ifdef TNDS_SUPPORT
              }
#endif // TNDS_SUPPORT

               SequenceStatus(userData, dm_stat);

               break;
            }

            XPL_LOG_DM_TMN_Debug(("dm_ua_handlecommand::ProcessCommand dm_stat=%d, command=%d\n",dm_stat, command));

            ret_stat = ProcessStatus (dm_stat,
                                                   userData,
                                                   (UINT8*)pContent->cmdID->content,
                                                   p_command_item,
                                                   command);
           if (ret_stat != SYNCML_DM_SUCCESS)
           {
                  sml_ret_stat = SML_ERR_UNSPECIFIC;
                  break;
           }

           if (p_command_list_item->next != NULL)
           {
                       p_command_list_item = p_command_list_item->next;
                  p_command_item = p_command_list_item->item;
           }
           else
                 p_command_item = NULL;
        } /* End of while */
    } /* !inAtomicCommand && dmSecState */
    else /*  dmSecState not authenticated */
    {
        /* Call the toolkit to construct the STATUS for ADD command */
        ret_stat = SaveCommandRefStatus((UINT8 *)pContent->cmdID->content,
                                 (UINT8 *)dm_command_name_table[command],
                                 p_command_item,
                                 pDmMgmtSessionObj->GetNotAuthorizedStatus(),
                                 pUserData );
         if (ret_stat != SYNCML_DM_SUCCESS)
               sml_ret_stat = SML_ERR_UNSPECIFIC;
    }
    /* Free the memeory we allocated (p_plugin_add), and passed in (pContent). */
    smlFreeGeneric((SmlGenericCmdPtr_t)pContent);

    return sml_ret_stat;
}



/*==================================================================================================
FUNCTION        : HandleAddCommand

DESCRIPTION     : When the ADD element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:
                  1) Call DMTree::Add() function to perform ADD command on the DM tree.
                  2) Call SYNCML_DM_BuildPackage::BuildStatus() to build up the staus command with
                     return status for each ADD command performed.
ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t HandleAddCommand (InstanceID_t id, VoidPtr_t    userData, SmlAddPtr_t  pContent)
{
    XPL_LOG_DM_TMN_Debug(("dm_ua_handlecommand::HandleAdd enter"));
    return ProcessCommand(id, userData, pContent,SYNCML_DM_ADD);
}

/*==================================================================================================
FUNCTION        : HandleCopyCommand

DESCRIPTION     : When the COPY element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:
                  1) Set DM status as SYNCML_DM_FEATURE_NOT_SUPPORTED.
                  2) Call SYNCML_DM_BuildPackage::BuildStatus() to build up the staus command with
                     return status for each COPY command performed.
ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleCopyCommand (InstanceID_t  id,
                   VoidPtr_t     userData,
                   SmlCopyPtr_t  pContent)
{
    Ret_t                  sml_ret_stat = SML_ERR_OK;
    SYNCML_DM_RET_STATUS_T ret_stat;
    SYNCML_DM_USER_DATA_T *pUserData = (SYNCML_DM_USER_DATA_T *)userData;

    pDmMgmtSessionObj->IncCommandCount(); /* This variable is used to check if syncml document has
                                           * any management commands */

    /* Call the toolkit to construct the STATUS for COPY command */
    ret_stat = SaveStatus(
        (UINT8 *)pContent->cmdID->content,
        (UINT8 *)dm_command_name_table[SYNCML_DM_COPY],
        NULL,
        NULL,
        SYNCML_DM_FEATURE_NOT_SUPPORTED,
        NULL,
        pUserData);

    if (ret_stat != SYNCML_DM_SUCCESS)
    {
        sml_ret_stat = SML_ERR_UNSPECIFIC;
    }

    /* Free the memory of pContent. */
    smlFreeGeneric((SmlGenericCmdPtr_t)pContent);

    return sml_ret_stat;
}


/*==================================================================================================
FUNCTION        : HandleDeleteCommand

DESCRIPTION     : When the DELETE element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:
                  1) Call DMTree::Delete() function to perform DELETE command on the DM tree.
                  2) Call SYNCML_DM_BuildPackage::BuildStatus() to build up the staus command with
                     return status for each DELETE command performed.
ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleDeleteCommand (InstanceID_t   id,
                     VoidPtr_t      userData,
                     SmlDeletePtr_t pContent)
{
    Ret_t   sml_ret_stat = SML_ERR_OK;
    SYNCML_DM_RET_STATUS_T     dm_stat = 0;
    SYNCML_DM_RET_STATUS_T ret_stat = 0;
    SYNCML_DM_USER_DATA_T   *pUserData = (SYNCML_DM_USER_DATA_T *)userData;

    SmlItemListPtr_t p_delete_list_item;
    SmlItemPtr_t     p_delete_item;

    p_delete_list_item = pContent->itemList;
    p_delete_item      = p_delete_list_item->item;

    XPL_LOG_DM_TMN_Debug(("dm_ua_handlecommand::HandleDelete enter"));

           ret_stat = pDmBuildPackage->GenerateAlertForLOB(DM_COMMAND_REPLACE);

    if (ret_stat != SYNCML_DM_SUCCESS)
    {
          ret_stat = ProcessStatus (dm_stat,
                                                   userData,
                                                   (UINT8*)pContent->cmdID->content,
                                                   p_delete_item,
                                                   SYNCML_DM_DELETE);
    /* Free the memory of pContent and p_target_uri. */
            smlFreeGeneric((SmlGenericCmdPtr_t)pContent);
                return SML_ERR_OK;
    }

    pDmMgmtSessionObj->IncCommandCount(); /* This variable is used to check if syncml document has
                                           * any management commands */

    /* Make sure we are not in an atomic and the Server is authenticated before performing any
     * DM commands.*/
    if (pDmMgmtSessionObj->IsAuthorized())
    {
        /* Loop on each DELETE item */
        while (p_delete_item != NULL) {


          // perform operation
          while ( true )
          {
              DMString strDeleteUri;
                dm_stat = PrepareCommandItem (p_delete_item,
                                                                    pUserData,
                                                                    strDeleteUri);
                if ( dm_stat != SYNCML_DM_SUCCESS )
                          break;

                XPL_LOG_DM_TMN_Debug(("dm_ua_handlecommand::HandleDelete uri=%s\n", strDeleteUri.c_str()));

#ifdef DM_ATOMIC_SUPPORTED
               dm_stat = dmTreeObj.Delete (strDeleteUri.c_str(),SYNCML_DM_REQUEST_TYPE_SERVER );
#else
               if ( pDmMgmtSessionObj->GetInAtomicCommand() )
                    dm_stat =  SYNCML_DM_COMMAND_FAILED;
               else
                    dm_stat = dmTreeObj.Delete (strDeleteUri.c_str(),SYNCML_DM_REQUEST_TYPE_SERVER );
#endif
               SequenceStatus(userData, dm_stat);
               break;
          }

          ret_stat = ProcessStatus (dm_stat,
                                                   userData,
                                                   (UINT8*)pContent->cmdID->content,
                                                   p_delete_item,
                                                   SYNCML_DM_DELETE);
          if (ret_stat != SYNCML_DM_SUCCESS)
           {
                  sml_ret_stat = SML_ERR_UNSPECIFIC;
                  break;
           }

            /* Move to the next item on the list */
            if (p_delete_list_item->next != NULL) {
                  p_delete_list_item = p_delete_list_item->next;
                  p_delete_item = p_delete_list_item->item;
            }
            else
                  p_delete_item = NULL;
        } /* End of while */
    } /* !inAtomicCommand && dmSecState */
    else /* dmSecState not authenticated */
    {
            ret_stat =  SaveCommandRefStatus(
                                      (UINT8 *)pContent->cmdID->content,
                                      (UINT8 *)dm_command_name_table[SYNCML_DM_DELETE],
                                      p_delete_item,
                                      pDmMgmtSessionObj->GetNotAuthorizedStatus(),
                                      pUserData );
            if (ret_stat != SYNCML_DM_SUCCESS)
            {
                sml_ret_stat = SML_ERR_UNSPECIFIC;
            }
    }

    /* Free the memory of pContent and p_target_uri. */
    smlFreeGeneric((SmlGenericCmdPtr_t)pContent);

    return sml_ret_stat;
}


/*==================================================================================================
FUNCTION        : ProcessAlertCommand

DESCRIPTION     : When the ALERT element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:
                  1) Process alert via XPL.
                     return status for each ALERT command performed.
ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
static SYNCML_DM_RET_STATUS_T
ProcessAlertCommand (VoidPtr_t   userData,
                    SmlAlertPtr_t  pContent,
                    SYNCML_DM_Alert * pAlert,
                    DMStringVector   &  responses)
{
    SYNCML_DM_USER_DATA_T  *pUserData = (SYNCML_DM_USER_DATA_T *)userData;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    if ( pAlert == NULL )
        return SYNCML_DM_FAIL;

    if ( pUserData->IsCommandSkipped() || pDmMgmtSessionObj->IsSessionAborted() )
         return SYNCML_DM_NOT_EXECUTED;

    pAlert->parse(pContent);
    dm_stat = pAlert->show();
    switch ( dm_stat )
    {
        case SYNCML_DM_SESSION_CANCELED:
            pDmMgmtSessionObj->SetSessionAborted();
            return SYNCML_DM_SUCCESS;

        case SYNCML_DM_SUCCESS:
             dm_stat = pAlert->processResponse(responses,&pUserData->alertState);
             break;

        default:
            pUserData->alertState =  SYNCML_DM_ALERT_CANCEL;
            break;
     }

    return dm_stat;

}


/*==================================================================================================
FUNCTION        : HandleAlertCommand

DESCRIPTION     : When the ALERT element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:
                  1) Check the ALERT value, set the DM status accordingly.
                  2) Call SYNCML_DM_BuildPackage::BuildStatus() to build up the staus command with
                     return status for each ALERT command performed.
ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleAlertCommand (InstanceID_t   id,
                    VoidPtr_t      userData,
                    SmlAlertPtr_t  pContent)
{
  Ret_t              sml_ret_stat = SML_ERR_OK;
  SYNCML_DM_RET_STATUS_T  ret_stat = 0;
  SYNCML_DM_RET_STATUS_T dm_stat = 0;
  SYNCML_DM_USER_DATA_T   *pUserData = (SYNCML_DM_USER_DATA_T *)userData;
  DMStringVector    responses;         // holds all the user responses

  pDmMgmtSessionObj->IncCommandCount(); /* This variable is used to check if syncml document has
                                         * any management commands */

  if (pDmMgmtSessionObj->IsAuthorized())
  {

    UINT32  alert_code_value;

    /* Get the data we need to work on */

    alert_code_value = DmAtoi((const char *)pContent->data->content);

    /* Please refer syncml_dm_represent_v111_20021002.pdf section 8 */
    switch (alert_code_value)
    {
        case DM_ALERT_SERVER_INITIATED_MGMT:
        case DM_ALERT_CLIENT_INITIATED_MGMT:
        case DM_ALERT_NEXT_MESSAGE:
            dm_stat = SYNCML_DM_SUCCESS;
            break;

        case DM_ALERT_SESSION_ABORT:
            pDmMgmtSessionObj->SetSessionAborted();
            dm_stat = SYNCML_DM_SUCCESS;
            break;

        // handle user interaction alerts
        case DM_ALERT_DISPLAY:
        {
            if ( !VerifyAlertItems(pContent) )
            {
               XPL_LOG_DM_TMN_Debug(("HandleAlertCommand, alert command items incorrect."));
               dm_stat = SYNCML_DM_INCOMPLETE_COMMAND;
            }
            else  {
               SYNCML_DM_DisplayAlert displayAlert;
               dm_stat = ProcessAlertCommand(userData,pContent,&displayAlert,responses);
            }
        }
        break;

        case DM_ALERT_CONTINUE_OR_ABORT:
        {
            if ( !VerifyAlertItems(pContent) )
            {
               XPL_LOG_DM_TMN_Debug(("HandleAlertCommand, alert command items incorrect."));
               dm_stat = SYNCML_DM_INCOMPLETE_COMMAND;
            }
            else  {
               SYNCML_DM_ConfirmAlert confirmAlert;
               dm_stat = ProcessAlertCommand(userData,pContent,&confirmAlert,responses);
            }
        }
        break;

        case DM_ALERT_TEXT_INPUT:
        {
            if ( !VerifyAlertItems(pContent) )
            {
               XPL_LOG_DM_TMN_Debug(("HandleAlertCommand, alert command items incorrect."));
               dm_stat = SYNCML_DM_INCOMPLETE_COMMAND;
            }
            else  {
               SYNCML_DM_TextInputAlert textInputAlert;
               dm_stat = ProcessAlertCommand(userData,pContent,&textInputAlert,responses);
            }
        }
        break;

        case DM_ALERT_SINGLE_CHOICE:
        {
            SYNCML_DM_SingleChoiceAlert singleChoiceAlert;
            dm_stat = ProcessAlertCommand(userData,pContent,&singleChoiceAlert,responses);
        }
        break;

        case DM_ALERT_MULTIPLE_CHOICE:
        {
            SYNCML_DM_MultipleChoiceAlert multipleChoiceAlert;
            dm_stat = ProcessAlertCommand(userData,pContent,&multipleChoiceAlert,responses);
        }
        break;

        default:
            dm_stat = SYNCML_DM_FEATURE_NOT_SUPPORTED; /* Optional feature not supported */
            break;
    }

    if(pDmMgmtSessionObj->GetInAtomicCommand())
    {
#ifdef DM_ATOMIC_SUPPORTED
        // Save the data for the status and results
      if ( dm_stat != SYNCML_DM_SUCCESS && dm_stat != SYNCML_DM_FEATURE_NOT_SUPPORTED )
          AtomicRollback(userData);
#endif
      pUserData->oStatus.push_back(
                      SYNCML_DM_STATUS_DATA_T((const char*)pContent->cmdID->content,
                                                         dm_command_name_table[SYNCML_DM_ALERT],
                                                         NULL,
                                                         NULL,
                                                         dm_stat,
                                                         &responses) );
    }
    else
    {
      ret_stat = SaveStatus((UINT8 *)pContent->cmdID->content,
                               (UINT8 *)dm_command_name_table[SYNCML_DM_ALERT],
                                NULL,
                                NULL,
                                dm_stat,
                                &responses,
                                pUserData );
      if (ret_stat != SYNCML_DM_SUCCESS)
      {
        sml_ret_stat = SML_ERR_UNSPECIFIC;
      }
    }
  }
  else
  {

    ret_stat = SaveStatus(
              (UINT8 *)pContent->cmdID->content,
              (UINT8 *)dm_command_name_table[SYNCML_DM_ALERT],
              NULL,
              NULL,
              pDmMgmtSessionObj->GetNotAuthorizedStatus(),
              &responses,
              pUserData );
    if (ret_stat != SYNCML_DM_SUCCESS)
      sml_ret_stat = SML_ERR_UNSPECIFIC;
  }

  /* Free the memory of the pContent */
  smlFreeAlert(pContent);

  return sml_ret_stat;
}


/*==================================================================================================
FUNCTION        : HandleExecCommand

DESCRIPTION     : When the EXEC element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:
                  1) Call DMTree::Exec() function to perform EXEC command on the DM tree.
                  2) Call SYNCML_DM_BuildPackage::BuildStatus() to build up the staus command with
                     return status for each EXEC command performed.
ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleExecCommand (InstanceID_t  id,
                   VoidPtr_t     userData,
                   SmlExecPtr_t  pContent)
{
    SYNCML_DM_RET_STATUS_T dm_stat=SYNCML_DM_SUCCESS;
    SYNCML_DM_USER_DATA_T  *pUserData = (SYNCML_DM_USER_DATA_T *)userData;

    SYNCML_DM_RET_STATUS_T ret_stat = 0;
    SYNCML_DM_URI_RESULT_T dm_uri_result = 0;
    SmlItemPtr_t p_exec_item;
    DMString strExecUri;
    DMString strOriExecUri;

    DMString execResults;
    DMString execData;

    p_exec_item = pContent->item;

           ret_stat = pDmBuildPackage->GenerateAlertForLOB(DM_COMMAND_EXEC);

    if (ret_stat != SYNCML_DM_SUCCESS)
    {
       smlFreeExec((SmlExecPtr_t)pContent);
        return SML_ERR_OK;
    }

    pDmMgmtSessionObj->IncCommandCount(); /* This variable is used to check if syncml document has
                                           * any management commands */

    /* Make sure we are not in an atomic and the Server is authenticated before performing any
     * DM commands.*/
    if (pDmMgmtSessionObj->GetInAtomicCommand() == FALSE &&
          pDmMgmtSessionObj->IsAuthorized())
    {

       if ((p_exec_item->target == NULL) || (p_exec_item->target->locURI == NULL))
       {
           smlFreeExec((SmlExecPtr_t)pContent);
            return SML_ERR_UNSPECIFIC;
       }

       strExecUri.assign((CPCHAR)p_exec_item->target->locURI->content,p_exec_item->target->locURI->length);

       strOriExecUri = strExecUri;
       if(strExecUri.Decode() == FALSE)
       {
         smlFreeExec((SmlExecPtr_t)pContent);
          return SML_ERR_UNSPECIFIC;
       }
        /* Fill the data for p_exec_data */
       if (p_exec_item->data != NULL && p_exec_item->data->length > 0)
       {
            execData.assign((CPCHAR)p_exec_item->data->content,p_exec_item->data->length);
            if ( execData == NULL )
            {
               smlFreeExec((SmlExecPtr_t)pContent);
               return SML_ERR_UNSPECIFIC;
            }
       }

       const char* szCorrelator = NULL;

        if ( pContent->correlator && pContent->correlator->contentType == SML_PCDATA_STRING )
            szCorrelator = (const char*)pContent->correlator->content;


       /* Validate the URI */
       dm_uri_result = dmTreeObj.URIValidateAndParse(strExecUri);
       switch (dm_uri_result)
       {
           case SYNCML_DM_COMMAND_ON_UNKNOWN_PROPERTY:
           case SYNCML_DM_COMMAND_INVALID_URI:
           case SYNCML_DM_COMMAND_URI_TOO_LONG:
             dm_stat = SYNCML_DM_BAD_REQUEST;
             break;

           default:

             // schen link error, code is not there
             // dm_stat = SYNCML_DM_FEATURE_NOT_SUPPORTED;
             dm_stat = dmTreeObj.Exec(strExecUri.c_str(), execData, execResults, szCorrelator);
             SequenceStatus(userData, dm_stat);
             break;
       }

    }
    else
           if (pDmMgmtSessionObj->GetInAtomicCommand())
          {
                AtomicRollback(userData);
                /* We don't apply commands within the Atomic.*/
                dm_stat = SYNCML_DM_COMMAND_FAILED;
          }
          else
                dm_stat = pDmMgmtSessionObj->GetNotAuthorizedStatus();

    UINT8  *p_TargetRefData = NULL;
    if(p_exec_item->target != NULL && p_exec_item->target->locURI != NULL){
       p_TargetRefData = (UINT8 *)p_exec_item->target->locURI->content;
    }

    if (0 == DmStrcmp((char *)p_TargetRefData, FDR_URI))
    {
       /* Build tha status with item tag for the EXEC command for FDR*/
       ret_stat = SaveStatus((UINT8 *)pContent->cmdID->content,
                                      (UINT8 *)dm_command_name_table[SYNCML_DM_EXEC],
                                      NULL,
                                      p_TargetRefData,
                                      dm_stat,
                                      NULL,
                                      pUserData );
    }
    else
    {
       /* Build tha status for the EXEC command */
       ret_stat = SaveStatus((UINT8 *)pContent->cmdID->content,
                                      (UINT8 *)dm_command_name_table[SYNCML_DM_EXEC],
                                      NULL,
                                      NULL,
                                      dm_stat,
                                      NULL,
                                      pUserData );

    }

    if (dm_stat == SYNCML_DM_SUCCESS && execResults.length() )
    {
         XPL_LOG_DM_TMN_Debug(("dm_ua_handlecommand::HandleExecCommand, inside success chk\n"));
         SmlResultsPtr_t  p_results=NULL;      /* To hold RESULTS structure */
         SmlItemPtr_t     p_results_item;      /* To hold GET results item */
         SmlItemListPtr_t p_results_list_item; /* To hold GET results list */

         // Allocate the memory for p_results
         p_results = smlAllocResults();

         if ( !p_results )
         {
            smlFreeExec((SmlExecPtr_t)pContent);
            return SML_ERR_UNSPECIFIC;
         }

         XPL_LOG_DM_TMN_Debug(("dm_ua_handlecommand::HandleExecCommand, before buildpcdata\n"));
         pDmBuildPackage->BuildPcData(p_results->cmdRef,
                                            SML_PCDATA_STRING,
                                            SML_EXT_UNDEFINED,
                                            DmStrlen((char *)pContent->cmdID->content),
                                            (UINT8*)pContent->cmdID->content);


         /* Fill in item fileds */
         p_results_list_item = p_results->itemList;

         p_results_item = p_results_list_item->item;
         ret_stat = SetExecResultsData(  p_results_item, strOriExecUri, execResults);
         pUserData->aResults.push_back(
            SYNCML_DM_RESULT_VALUE( SYNCML_DM_RESULT_VALUE::Enum_Result_Exec, p_results,
            SYNCML_DM_GET_ON_LIST_RET_DATA_T(), (CPCHAR)pContent->cmdID->content,
            (const char*)pDmBuildPackage->GetMsgRef()) );
    }


    if (ret_stat != SYNCML_DM_SUCCESS)
    {
        smlFreeExec((SmlExecPtr_t)pContent);
        return SML_ERR_UNSPECIFIC;
    }

    smlFreeExec((SmlExecPtr_t)pContent);

    XPL_LOG_DM_TMN_Debug(("dm_ua_handlecommand::HandleExecCommand leaving\n"));
    return SML_ERR_OK;
}


/*==================================================================================================
FUNCTION        : HandleGetCommand

DESCRIPTION     : When the GET element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:
                  1) Call DMTree::Get() function to perform GET command on the DM tree.
                  2) Call BuildResultsCommand() to build the results from GET command.
                  3) Call SYNCML_DM_BuildPackage::BuildStatus() to build up the staus command with
                     return status for each GET command performed.
ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleGetCommand (InstanceID_t id,
                  VoidPtr_t    userData,
                  SmlGetPtr_t  pContent)
{
    DMString strTargetUri;
    Ret_t sml_ret_stat = SML_ERR_OK;
    SYNCML_DM_RET_STATUS_T dm_stat = 0;
    SYNCML_DM_RET_STATUS_T ret_stat = 0;
    DMGetData getData;

    /* p_get_struct_data is OUTPUT parameter of dmTreeObj.InitListAndGetListFirstItem call. We
       need to initialize it as NULL before we pass it to the call, p_get_struct_data could not
       be set if the return is not SUCCESS. */

    SYNCML_DM_GET_ON_LIST_RET_DATA_T p_get_struct_data;
    SYNCML_DM_URI_RESULT_T dm_uri_result = 0;
    SYNCML_DM_USER_DATA_T *pUserData = (SYNCML_DM_USER_DATA_T *)userData;

    SmlItemPtr_t     p_get_item;
    SmlItemListPtr_t p_get_list_item;

    pDmMgmtSessionObj->IncCommandCount();

    /* Point the GET item to the correct spot */
    p_get_list_item = pContent->itemList;
    p_get_item      = p_get_list_item->item;

   dm_stat = pDmBuildPackage->GenerateAlertForLOB(DM_COMMAND_GET);
    if (dm_stat != SYNCML_DM_SUCCESS)
    {
            ret_stat = ProcessStatus (dm_stat,
                                                   userData,
                                                   (UINT8*)pContent->cmdID->content,
                                                   p_get_item,
                                                   SYNCML_DM_GET);
    /* Free the memory */
          smlFreeGetPut(pContent);
        return SML_ERR_OK;
    }

    /* Make sure we are not in an atomic and the Server is authenticated before performing any
     * DM commands.*/
    if (pDmMgmtSessionObj->GetInAtomicCommand() == FALSE &&
         pDmMgmtSessionObj->IsAuthorized())
    {

      while (p_get_item != NULL)
      {                              /* Loop through each GET ITEM */

          getData.clear();
          while ( true )
          {
              DMString strTargetUri;
              XPL_LOG_DM_TMN_Debug(("\ninside dm_ua_handlecommand::HandleGet, uri=%s\n", strTargetUri.c_str()));
                dm_stat = PrepareCommandItem (p_get_item,
                                                                       pUserData,
                                                                       strTargetUri);
                if ( dm_stat != SYNCML_DM_SUCCESS )
                          break;

                  /* Validete the URI */
              dm_uri_result = dmTreeObj.URIValidateAndParse((char*)strTargetUri.c_str());
              switch (dm_uri_result) {
                    case SYNCML_DM_COMMAND_INVALID_URI:
                    case SYNCML_DM_COMMAND_ON_UNKNOWN_PROPERTY:
                    case SYNCML_DM_COMMAND_URI_TOO_LONG:
                       dm_stat = SYNCML_DM_BAD_REQUEST;
                       break;

                    case SYNCML_DM_COMMAND_ON_NODE:
                    case SYNCML_DM_COMMAND_ON_ACL_PROPERTY:
                    case SYNCML_DM_COMMAND_ON_FORMAT_PROPERTY:
                    case SYNCML_DM_COMMAND_ON_NAME_PROPERTY:
                    case SYNCML_DM_COMMAND_ON_SIZE_PROPERTY:
                    case SYNCML_DM_COMMAND_ON_TYPE_PROPERTY:
                    case SYNCML_DM_COMMAND_ON_TITLE_PROPERTY:
                    case SYNCML_DM_COMMAND_ON_TSTAMP_PROPERTY:
                    case SYNCML_DM_COMMAND_ON_VERNO_PROPERTY:
                       dm_stat = dmTreeObj.Get(strTargetUri.c_str(), getData,SYNCML_DM_REQUEST_TYPE_SERVER);
                       XPL_LOG_DM_TMN_Debug(("\ninside dm_ua_handlecommand::HandleGet, dmTreeObj.Get=%d", dm_stat));
                    if (dm_stat == SYNCML_DM_SUCCESS)
                       {
                         dm_stat = SaveResult( strTargetUri, (const char *)pContent->cmdID->content,
                                               &getData, FALSE, FALSE,
                                               (dm_uri_result == SYNCML_DM_COMMAND_ON_NODE ? FALSE : TRUE),
                                               pUserData, SYNCML_DM_RESULT_VALUE::Enum_Result_Get,
                                               p_get_struct_data );
                       }
                       break;

                    case SYNCML_DM_COMMAND_LIST_STRUCT:
                    case SYNCML_DM_COMMAND_LIST_STRUCTDATA:
                    case SYNCML_DM_COMMAND_LIST_TNDS:
                       /* Call TNM to get the first item on the Get Struct results list */
                      {
                       DMGetData * pGetData = NULL;
                       DMString strTargetUriNoQuery(strTargetUri);

                       // remove query from URI
                       char *psQPos = DmStrstr(strTargetUri, SYNCML_DM_LIST);
                       if (psQPos != NULL)
                       {
                           strTargetUriNoQuery.assign(strTargetUri.c_str(), psQPos - strTargetUri.c_str());
                       }

                       dm_stat = dmTreeObj.InitListAndGetListFirstItem(strTargetUriNoQuery,
                                                                SYNCML_DM_GET_ON_LIST_STRUCT,
                                                                p_get_struct_data);

                       if (dm_stat == SYNCML_DM_SUCCESS)
                       {
                           UINT8 type = 0;
                           if ( dm_uri_result == SYNCML_DM_COMMAND_LIST_STRUCT )
                           {
                               type = SYNCML_DM_RESULT_VALUE::Enum_Result_GetStruct;
                           }
                           else if ( dm_uri_result == SYNCML_DM_COMMAND_LIST_STRUCTDATA )
                           {
                               type = SYNCML_DM_RESULT_VALUE::Enum_Result_GetStructData;
                           }
                           else if ( dm_uri_result == SYNCML_DM_COMMAND_LIST_TNDS )
                           {
                               type = SYNCML_DM_RESULT_VALUE::Enum_Result_GetTnds;
                           }
                           else
                           {
                               dm_stat = SYNCML_DM_BAD_REQUEST;
                               break;
                           }
                           pGetData = p_get_struct_data.psRetData; p_get_struct_data.psRetData = NULL;
                           dm_stat = SaveResult( strTargetUriNoQuery.c_str(), (CPCHAR)pContent->cmdID->content,
                                                 pGetData,
                                                 dm_uri_result == SYNCML_DM_COMMAND_LIST_STRUCT,
                                                 type != SYNCML_DM_RESULT_VALUE::Enum_Result_GetTnds,
                                                 FALSE,
                                                 pUserData,
                                                 type,
                                                 p_get_struct_data);
                           if ( pGetData )
                              delete pGetData;
                       }
                       else
                           /* Free the memory of p_passedin_get_struct_data */
                           pDmBuildPackage->FreeGetStructData(p_get_struct_data);
                       }
                       break;
                    default:
                       dm_stat = SYNCML_DM_FEATURE_NOT_SUPPORTED;
                       break;

              }  // end switch
              break;

           }
           ret_stat = ProcessStatus (dm_stat,
                                                   userData,
                                                   (UINT8*)pContent->cmdID->content,
                                                   p_get_item,
                                                   SYNCML_DM_GET);
           if (ret_stat != SYNCML_DM_SUCCESS)
           {
                  sml_ret_stat = SML_ERR_UNSPECIFIC;
                  break;
           }

          /* Move to the next item */
          if (p_get_list_item->next != NULL)
           {
                      p_get_list_item = p_get_list_item->next;
                p_get_item = p_get_list_item->item;
          }
          else
                p_get_item = NULL;
      } /* End of while */

   }
   else
   { /* !inAtomicCommand && dmSecState */  /* inAtomicCommand == TRUE || dmSecState not authenticated */

      if (pDmMgmtSessionObj->GetInAtomicCommand() == TRUE)
      {
         AtomicRollback(userData);
         dm_stat = SYNCML_DM_COMMAND_FAILED; /* We don't apply commands within the Atomic.*/
      }
      else
          dm_stat = pDmMgmtSessionObj->GetNotAuthorizedStatus();

      ret_stat = SaveCommandRefStatus(
                                               (UINT8 *)pContent->cmdID->content,
                                           (UINT8 *)dm_command_name_table[SYNCML_DM_GET],
                                            p_get_item,
                                            dm_stat,
                                            pUserData);
       if (ret_stat != SYNCML_DM_SUCCESS)
             sml_ret_stat = SML_ERR_UNSPECIFIC;
  }

    /* Free the memory */
  smlFreeGetPut(pContent);
  pContent = NULL;

  return (sml_ret_stat);
}


/*==================================================================================================
FUNCTION        : HandleReplaceCommand

DESCRIPTION     : When the REPLACE element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:
                  1) Call DMTree::Replace() function to perform REPLACE command on the DM
                     tree.
                  2) Call SYNCML_DM_BuildPackage::BuildStatus() to build up the staus command with
                     return status for each REPLACE command performed.
ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleReplaceCommand (InstanceID_t    id,
                      VoidPtr_t       userData,
                      SmlReplacePtr_t pContent)
{
      return ProcessCommand(id, userData, pContent,SYNCML_DM_REPLACE);
}


/*==================================================================================================
FUNCTION        : HandleStartSequenceCommand

DESCRIPTION     : When the SEQUENCE element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:

ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleStartSequenceCommand(InstanceID_t id,
                           VoidPtr_t    userData,
                           SmlSequencePtr_t  pContent)
{
    Ret_t                  sml_ret_stat = SML_ERR_OK;
    SYNCML_DM_RET_STATUS_T ret_stat;
    SYNCML_DM_USER_DATA_T   *pUserData = (SYNCML_DM_USER_DATA_T *)userData;
    SYNCML_DM_RET_STATUS_T     dm_stat = SYNCML_DM_SUCCESS;

          ret_stat = pDmBuildPackage->GenerateAlertForLOB(DM_COMMAND_SEQUENCE_START);

    if (ret_stat != SYNCML_DM_SUCCESS)
    {
    /* Free the memory of pContent. Toolkit use same API to free the memory for atomic and
       sequence */
    smlFreeAtomic((SmlAtomicPtr_t)pContent);
        return SML_ERR_OK;
    }
  pUserData->StartSequence();

  if (pDmMgmtSessionObj->IsAuthorized())
        dm_stat = SYNCML_DM_SUCCESS;
  else
        dm_stat = pDmMgmtSessionObj->GetNotAuthorizedStatus();

    pDmMgmtSessionObj->IncCommandCount(); /* This variable is used to check if syncml document has
                                           * any management commands */

    /* Call the toolkit to construct the STATUS for SEQUENCE command */
    ret_stat = SaveStatus(
                          (UINT8 *)pContent->cmdID->content,
                          (UINT8 *)dm_command_name_table[SYNCML_DM_SEQUENCE],
                           NULL,
                           NULL,
                           dm_stat,
                           NULL,
                           pUserData );

    if (ret_stat != SYNCML_DM_SUCCESS)
    {
        sml_ret_stat = SML_ERR_UNSPECIFIC;
    }

    /* Free the memory of pContent. Toolkit use same API to free the memory for atomic and
       sequence */
    smlFreeAtomic((SmlAtomicPtr_t)pContent);

    return sml_ret_stat;
}

/*==================================================================================================
FUNCTION        : HandleEndSequenceCommand

DESCRIPTION     : When the SEQUENCE element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:

ARGUMENT PASSED : id
                  userData
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleEndSequenceCommand(InstanceID_t id,
                         VoidPtr_t    userData)
{
    /* We don't need to do anything special for the End Sequence.*/
    SYNCML_DM_USER_DATA_T *pUserData = (SYNCML_DM_USER_DATA_T *)userData;

    if(pUserData)
        pUserData->EndSequence();
    return SML_ERR_OK;
}



/*==================================================================================================
FUNCTION        : HandleStartAtomicCommand

DESCRIPTION     : When the ATOMIC element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:

ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleStartAtomicCommand(InstanceID_t id,
                         VoidPtr_t    userData,
                         SmlAtomicPtr_t  pContent)
{
  Ret_t                  sml_ret_stat = SML_ERR_OK;
  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
  SYNCML_DM_USER_DATA_T   *pUserData = (SYNCML_DM_USER_DATA_T *)userData;

  dm_stat = pDmBuildPackage->GenerateAlertForLOB(DM_COMMAND_ATOMIC_START);

    if (dm_stat != SYNCML_DM_SUCCESS)
    {
         smlFreeAtomic(pContent  );
        return SML_ERR_OK;
    }

  if (pDmMgmtSessionObj->IsAuthorized())
  {
    if(pDmMgmtSessionObj->GetInAtomicCommand())
    { // nested atomics are not allowed
      SaveStatus((UINT8 *)pContent->cmdID->content,
                          (UINT8 *)dm_command_name_table[SYNCML_DM_ATOMIC],
                           NULL,
                           NULL,
                           SYNCML_DM_COMMAND_FAILED,
                           NULL,
                           pUserData);
      smlFreeAtomic(pContent  );
      return sml_ret_stat;
    }

    dm_stat=dmTreeObj.GetLockContextManager().ReleaseIDInternal(SYNCML_DM_LOCKID_CURRENT, SYNCML_DM_ATOMIC);

    if ( dm_stat != SYNCML_DM_SUCCESS && dm_stat != SYNCML_DM_FEATURE_NOT_SUPPORTED )
    {
      smlFreeAtomic(pContent  );
      return SML_ERR_UNSPECIFIC;
    }

    /* Remember that we are in an Atomic command.*/
    pDmMgmtSessionObj->SetInAtomicCommand(TRUE);

    // Save the data for the status
    pUserData->pAtomicStatus.bValueSet = TRUE;
    pUserData->pAtomicStatus.pCmdId = (CPCHAR)pContent->cmdID->content;
    pUserData->pAtomicStatus.pCmdName = dm_command_name_table[SYNCML_DM_ATOMIC];
    pUserData->pAtomicStatus.status = dm_stat;
  }
  else
  {
    dm_stat = SaveStatus((UINT8 *)pContent->cmdID->content,
                                      (UINT8 *)dm_command_name_table[SYNCML_DM_ATOMIC],
                                       NULL,
                                       NULL,
                                       pDmMgmtSessionObj->GetNotAuthorizedStatus(),
                                       NULL,
                                       pUserData);
    if (dm_stat != SYNCML_DM_SUCCESS)
      sml_ret_stat = SML_ERR_UNSPECIFIC;
  }
  smlFreeAtomic(pContent  );
  return sml_ret_stat;
}


/*==================================================================================================
FUNCTION        : HandleEndAtomicCommand

DESCRIPTION     : When the End ATOMIC element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:

ARGUMENT PASSED : id
                  userData
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleEndAtomicCommand(InstanceID_t id,
                       VoidPtr_t    userData)
{
  Ret_t                  sml_ret_stat = SML_ERR_OK;
  SYNCML_DM_USER_DATA_T   *pUserData = (SYNCML_DM_USER_DATA_T *)userData;
  SYNCML_DM_RET_STATUS_T retStatus=SYNCML_DM_SUCCESS;

  if (pDmMgmtSessionObj->IsAuthorized())
  {
     pDmMgmtSessionObj->SetInAtomicCommand(FALSE);

    if ( !pUserData->rollback )
    {
      retStatus=dmTreeObj.GetLockContextManager().ReleaseIDInternal(SYNCML_DM_LOCKID_CURRENT, SYNCML_DM_COMMIT);

      if ( retStatus != SYNCML_DM_SUCCESS && retStatus != SYNCML_DM_FEATURE_NOT_SUPPORTED)
        return SML_ERR_UNSPECIFIC;
     }

    /* We are now out of the Atomic command.*/
    UINT8 *p_SourceRefData = !pUserData->pAtomicStatus.pSource.empty() ?
                                                     (UINT8 *)(pUserData->pAtomicStatus.pSource.c_str()) : NULL;
    UINT8 *p_TargetRefData = !pUserData->pAtomicStatus.pTarget.empty() ?
                                                     (UINT8 *)pUserData->pAtomicStatus.pTarget.c_str() : NULL;

    retStatus = SaveStatus((UINT8 *)pUserData->pAtomicStatus.pCmdId.c_str(),
                                         (UINT8 *)pUserData->pAtomicStatus.pCmdName.c_str(),
                                         p_SourceRefData,
                                         p_TargetRefData,
                                         pUserData->pAtomicStatus.status,
                                         NULL,
                                         pUserData);

    if (retStatus != SYNCML_DM_SUCCESS)
       sml_ret_stat = SML_ERR_UNSPECIFIC;

    for ( int i = 0; i <pUserData->oStatus.size(); i++ )
    {
       const SYNCML_DM_STATUS_DATA_T& ptrStatus = pUserData->oStatus[i];

         UINT8 *p_SourceRefData = !ptrStatus.pSource.empty() ?
                                                     (UINT8 *)(ptrStatus.pSource.c_str()) : NULL;
       retStatus = SaveStatus((UINT8 *)ptrStatus.pCmdId.c_str(),
                              (UINT8 *)ptrStatus.pCmdName.c_str(),
                              p_SourceRefData,
                              (UINT8 *)ptrStatus.pTarget.c_str(),
                              ptrStatus.status,
                              &ptrStatus.responses,
                              pUserData);

       if (retStatus != SYNCML_DM_SUCCESS)
          sml_ret_stat = SML_ERR_UNSPECIFIC;
    }

    pUserData->EndAtomic();
  }

  return sml_ret_stat;
}


/*==================================================================================================
FUNCTION        : HandleStatusCommand

DESCRIPTION     : When the STATUS element is processed from the received message, this callback
                  function will be called.

                  This function will perform the following operations:
                  1) Check the status code.
                  2) Call TNMTree::Replace() to replace the nonce value.
                  3) Update the UserAgent's Security state based on the client authentication
                     status received from the server.
                  4) Call SYNCML_DM_BuildPackage::BuildFinishSyncHdr() to finish our SyncHdr
                  5) Determine if we need to Challenge the server
                  6) Call SYNCML_DM_BuildPackage::BuildStatus() to build up our first status
                     which contains our server authentication disposition.

ARGUMENT PASSED : id
                  userData
                  pContent
OUTPUT PARAMETER:
RETURN VALUE    : SML_ERR_OK or ERR code
IMPORTANT NOTES :


==================================================================================================*/
Ret_t
HandleStatusCommand (InstanceID_t   id,
                     VoidPtr_t      userData,
                     SmlStatusPtr_t pContent)
{
    SYNCML_DM_USER_DATA_T  *pUserData = (SYNCML_DM_USER_DATA_T *)userData;
    Ret_t                   sml_ret_stat = SML_ERR_OK;
    SYNCML_DM_RET_STATUS_T  ret_stat;
    SYNCML_DM_RET_STATUS_T      local_dm_stat = SYNCML_DM_SUCCESS;
    UINT8                   *pClientNonce = NULL;
    UINT8                   *pServerNonce = NULL;
    UINT8                   *p_auth = NULL;
    SYNCML_DM_RET_STATUS_T      clientAuthStatus;
    SYNCML_DM_RET_STATUS_T      serverStatus;
    SYNCML_DM_CHAL_TYPE_T   serverChalType = SYNCML_DM_CHAL_NONE;
    UINT8                   command_id_str[UINT16_TYPE_STR_SIZE_5] = "0";
    /* command_id_str reference should be "0" for header */
    DM_CHALLENGE_T                  *pClientChal = NULL;
    DMClientServerCreds     *pClientServerCreds;
    SYNCML_DM_SEC_STATE_FLAG_T   currSecState;
    SYNCMLDM_NONCE_STRING_INFO_T    *pNonceStruct = NULL;
    SYNCMLDM_NONCE_GENERATE_PARAMETER_INFO_T    nonceInfo;
    SmlMetInfMetInfPtr_t p_meta_info = NULL;

    /* First check if this status is for the "SyncHdr".*/
    if (DmStrncmp((char *)pContent->cmd->content, SYNCML_SYNCHDR, pContent->cmd->length) == 0)
    {
        XPL_LOG_DM_TMN_Debug(("\ninside dm_ua_handlecommand::handleStatusCommand, synchdr command ref:%s\n\n", (char*)pContent->cmdRef->content));
        /* Get the Client and Server Cred info and current Security state.*/
        pClientServerCreds = pDmMgmtSessionObj->GetClientServerCreds();
        currSecState = pDmMgmtSessionObj->GetSecState();

        // DP: if we have postponed nonce - save it
        if ( pUserData->bNonceGenerated )
        {
          pUserData->bNonceGenerated = FALSE;
          if ( pClientServerCreds && pClientServerCreds->pServerNonce )
          {
                if ( dmTreeObj.IsVersion_12()  )
                            pClientServerCreds->SaveServerAttribute(DM_AAUTHDATA, pClientServerCreds->pServerNonce);
                  else
                          pClientServerCreds->SaveServerAttribute(DM_SERVERNONCE, pClientServerCreds->pServerNonce);
          }
        }

        /* We must check the new Status value from the Server in case our Security State changed.*/
        p_auth = (UINT8 *)DmAllocMem(pContent->data->length+1);
        if ( p_auth == NULL )
           return SYNCML_DM_DEVICE_FULL;
        DmStrncpy((char *)p_auth,
                (const char *)pContent->data->content,
                pContent->data->length);
        p_auth[pContent->data->length] = '\0';

        /* We always need to check the clientAuthStatus since we send creds on every message.*/
        clientAuthStatus = SyncML2DMCode((char *)p_auth);
        DmFreeMem(p_auth);

        XPL_LOG_DM_TMN_Debug(("\ninside dm_ua_handlecommand::handleStatusCommand, synchdr command clientAuthStatus: %d\n\n", clientAuthStatus));

        if (clientAuthStatus == SYNCML_DM_AUTHENTICATION_REQUIRED)
        {
            return clientAuthStatus;
        }

        /* Update the client retry count */
        if (clientAuthStatus  == SYNCML_DM_AUTHENTICATION_ACCEPTED ||
            clientAuthStatus == SYNCML_DM_SUCCESS)
        {
            /* Reset client retry count when client is authenticated */
            pDmMgmtSessionObj->SetClientRetryCount(0);

            // Save authPref value if needed
            pClientServerCreds->SaveAuthPref();
        }
        else
        {
            /* Increment the retry count when client is not authenticated.*/
            pDmMgmtSessionObj->IncClientRetryCount();
        }

        /* Update the security state with the client authentication status. Note that we only
         * check for cases that cause a change in the SecurityState.
         */
        switch (currSecState)
        {
            case DM_CLIENT_NO_SERVER_NO_AUTH:
                if (clientAuthStatus == SYNCML_DM_AUTHENTICATION_ACCEPTED ||
                    clientAuthStatus == SYNCML_DM_SUCCESS)
                {
                    currSecState = DM_CLIENT_Y_SERVER_NO_AUTH;
                }
                break;

            case DM_CLIENT_NO_SERVER_Y_AUTH:
                if (clientAuthStatus == SYNCML_DM_AUTHENTICATION_ACCEPTED ||
                    clientAuthStatus == SYNCML_DM_SUCCESS)
                {
                    currSecState = DM_BOTH_CLIENT_SERVER_AUTH;
                }
                break;

            case DM_CLIENT_Y_SERVER_NO_AUTH:
                if (clientAuthStatus != SYNCML_DM_AUTHENTICATION_ACCEPTED &&
                    clientAuthStatus != SYNCML_DM_SUCCESS)
                {
                    currSecState = DM_CLIENT_NO_SERVER_NO_AUTH;
                }
                break;

            case DM_BOTH_CLIENT_SERVER_AUTH:
                if (clientAuthStatus != SYNCML_DM_AUTHENTICATION_ACCEPTED &&
                    clientAuthStatus != SYNCML_DM_SUCCESS)
                {
                    currSecState = DM_CLIENT_NO_SERVER_Y_AUTH;
                }
                break;
        }

        /* Update the UserAgent Security state in case it changed.*/
        pDmMgmtSessionObj->SetSecState(currSecState);

        if ((pContent->chal != NULL) && (pContent->chal->meta != NULL))
        {
            /* We were challenged in the SyncHdr, so we need to build our creditials in the package.*/
            if (currSecState == DM_CLIENT_NO_SERVER_NO_AUTH ||
                currSecState == DM_CLIENT_NO_SERVER_Y_AUTH)
            {
                /* If the client has not been authenticated yet, increment the commandCount so
                 * we'll still respond even if there are no other operational commmands.
                 * But if the client has already been authenticated, then we won't increment since
                 * this may be the end of the session.  In that case, the nonce will be used
                 * in the next session.
                 */
                pDmMgmtSessionObj->IncCommandCount();
            }

            p_meta_info = (sml_metinf_metinf_s *)pContent->chal->meta->content;

            /* Check the Type.*/
            if (p_meta_info->type != NULL)
            {
                /* Check which type of challenge was sent.*/
                if (smlLibStrncmp((char *)p_meta_info->type->content, SYNCML_AUTH_MAC,
                                  p_meta_info->type->length) == 0)
                {
                    /* We received a challenge for HMAC-MD5.*/
                    serverChalType = SYNCML_DM_CHAL_HMAC;
                }
                else if (smlLibStrncmp((char *)p_meta_info->type->content, SYNCML_AUTH_MD5,
                                       p_meta_info->type->length) == 0)
                {
                    /* We received a challenge for MD5.*/
                    serverChalType = SYNCML_DM_CHAL_MD5;
                }
                else
                {
                    /* We received a challenge for Basic security.*/
                    serverChalType = SYNCML_DM_CHAL_BASIC;
                }
            }

            // DP switch to new auth type if needed
            if ( dmTreeObj.IsVersion_12()  )
            {
                local_dm_stat = pClientServerCreds->SetPrefClientAuth(serverChalType);
                if ( local_dm_stat != SYNCML_DM_SUCCESS )
                {
                        pDmMgmtSessionObj->SetClientRetryCount(MAX_AUTH_RETRY+1);
                }
            }
            /* Get the nextnonce sent to us from the server.*/
            if (p_meta_info->nextnonce != NULL)
            {
                pClientNonce = (UINT8 *)DmAllocMem(p_meta_info->nextnonce->length + 1);
                if ( pClientNonce == NULL )
                   return SYNCML_DM_DEVICE_FULL;
                memcpy(pClientNonce,
                       p_meta_info->nextnonce->content,
                       p_meta_info->nextnonce->length);
                pClientNonce[p_meta_info->nextnonce->length] = '\0';

                /* Save the new ClientNonce in the Tree.*/
                  if ( dmTreeObj.IsVersion_12()  )
                        local_dm_stat = pClientServerCreds->SaveClientAttribute(DM_AAUTHDATA, (CPCHAR)pClientNonce);
                  else
                          local_dm_stat = pClientServerCreds->SaveClientAttribute(DM_CLIENTNONCE, (CPCHAR)pClientNonce);
                /* Note that we continue even if we failed to store the clientNonce.*/
                pClientServerCreds->pClientNonce = (const char*)pClientNonce;
                DmFreeMem(pClientNonce);

                /* Note that we continue even if we failed to store the clientNonce.*/
            }
        } /* If chal != NULL */

        /* Call the method to finish our SyncHdr and start the toolkit message.*/
        ret_stat = pDmBuildPackage->BuildFinishSyncHdr(serverChalType);
        if (ret_stat != SYNCML_DM_SUCCESS)
        {
            sml_ret_stat = SML_ERR_UNSPECIFIC;
            synchdr_dm_stat = SYNCML_DM_BAD_REQUEST;
        }

        /* Determine if we need to challenge the server. Note that we will challange
         * the Server even if it has already been authenticated (hmac/md5 only).*/
        if ( (currSecState == DM_CLIENT_NO_SERVER_NO_AUTH ||
            currSecState == DM_CLIENT_Y_SERVER_NO_AUTH) &&
            pClientServerCreds->ServerChalType > SYNCML_DM_CHAL_BASIC )
        { // generate new nonce if required
            DMGetData devID;
            /* First we need to generate a new nextNonce for the server.*/
            nonceInfo.pb_user_name = (UINT8*)pClientServerCreds->pClientUserName.c_str();
            nonceInfo.pb_password = (UINT8*)pClientServerCreds->pServerPW.c_str();
            nonceInfo.pb_server_id = (UINT8*)pClientServerCreds->pServerId.c_str();

            dmTreeObj.Get(DM_DEV_INFO_DEVID_URI, devID,SYNCML_DM_REQUEST_TYPE_INTERNAL);

            pNonceStruct = syncmldm_sec_generate_nonce(&nonceInfo,devID.getCharData());
            if ( pNonceStruct == NULL )
            {
                return SYNCML_DM_COMMAND_FAILED;
            }

            /* Copy the new Server Nonce.*/
            pServerNonce = (UINT8 *)DmAllocMem(pNonceStruct->w_nonce_string_length + 1);
            if ( pServerNonce == NULL )
            {
                DmFreeMem(pNonceStruct);
                return SYNCML_DM_DEVICE_FULL;
            }
            memcpy(pServerNonce, pNonceStruct->ab_nonce_string,
                   pNonceStruct->w_nonce_string_length);
            pServerNonce[pNonceStruct->w_nonce_string_length] = '\0';

            /* Store the new ServerNonce in the Tree.*/
            // DP: don't save nonce in the tree yet, since server answer can be empty packet and
            // we can end session without sending it to the server
            pUserData->bNonceGenerated = TRUE;
            DmFreeMem(pNonceStruct);

            /* Note we continue even if we failed to store the serverNonce.*/
            pClientServerCreds->pServerNonce = (const char*)pServerNonce;
            DmFreeMem(pServerNonce);
        }

        if (currSecState == DM_CLIENT_NO_SERVER_NO_AUTH ||
            currSecState == DM_CLIENT_Y_SERVER_NO_AUTH ||
            pClientServerCreds->ServerChalType >= SYNCML_DM_CHAL_MD5 ) { // no authenticated or needs chal
          /* Setup the Client Challenge information.*/
          pClientChal = (DM_CHALLENGE_T *)DmAllocMem(sizeof(DM_CHALLENGE_T));
          if ( pClientChal == NULL )
          {
             return SYNCML_DM_DEVICE_FULL;
          }

          if ( pClientServerCreds->ServerChalType == SYNCML_DM_CHAL_HMAC )
                pClientChal->pChalType = (UINT8 *) SYNCML_AUTH_MAC;
          else if ( pClientServerCreds->ServerChalType == SYNCML_DM_CHAL_MD5 )
                pClientChal->pChalType = (UINT8 *) SYNCML_AUTH_MD5;
          else
                pClientChal->pChalType = (UINT8 *) SYNCML_AUTH_BASIC;

          pClientChal->pChalFormat = (UINT8 *)SYNCML_B64;
          pClientChal->pChalNonce = pClientServerCreds->ServerChalType >= SYNCML_DM_CHAL_MD5 ?
            (UINT8*)pClientServerCreds->pServerNonce.c_str() : NULL;
        }

        /* Set the Server's authentication status.*/
        if (currSecState == DM_CLIENT_NO_SERVER_NO_AUTH ||
            currSecState == DM_CLIENT_Y_SERVER_NO_AUTH)
          synchdr_dm_stat = pDmMgmtSessionObj->GetNotAuthorizedStatus();

        /* Now that our SyncHdr is header is closed, we need to create our first status.
         * Call the toolkit to construct STATUS for SyncHdr */
        SmlStatusPtr_t pStatus = pDmBuildPackage->AllocateStatus(
            command_id_str,
            (UINT8 *)dm_command_name_table[SYNCML_DM_HEADER],
            NULL,   /* Source URI, it's not needed for SyncHdr STATUS */
            NULL,   /* Target URI, it's not needed for SyncHdr STATUS */
            pClientChal,
            synchdr_dm_stat, NULL );
        if ( pStatus ) {
            ret_stat = pDmBuildPackage->BuildStatus( pStatus );
            smlFreeStatus(pStatus);
        }
        else
          ret_stat = SYNCML_DM_FAIL;

        DmFreeMem(pClientChal);

        if (ret_stat != SYNCML_DM_SUCCESS)
        {
            sml_ret_stat = SML_ERR_UNSPECIFIC;
        }
    } /* SyncHdr check */

    /* Check if this Status is for our "Replace" command (Our DevInfo) */
    else if (DmStrncmp((char *)pContent->cmd->content, SYNCML_REPLACE, pContent->cmd->length) == 0)
    {
          XPL_LOG_DM_TMN_Debug(("\ninside dm_ua_handlecommand::handleStatusCommand, replace command ref:%s\n\n", (char*)pContent->cmdRef->content));
          /* Determine the serverStatus for our DevInfo.*/
          p_auth = (UINT8 *)DmAllocMem(pContent->data->length+1);
          if ( p_auth == NULL )
          {
             return SYNCML_DM_DEVICE_FULL;
          }
          DmStrncpy((char *)p_auth,
                    (const char *)pContent->data->content,
                    pContent->data->length);
          p_auth[pContent->data->length] = '\0';
          serverStatus = SyncML2DMCode((char *)p_auth);
          DmFreeMem(p_auth);

          /* We only resend the DevInfo in the case of Authentication failure.*/
          if (serverStatus == SYNCML_DM_UNAUTHORIZED)
          {
              /* Call the method to build our DevInfo into the REPLACE command for this package. */
              ret_stat = pDmBuildPackage->BuildReplaceCommand();
              if (ret_stat != SYNCML_DM_SUCCESS)
              {
                  sml_ret_stat = SML_ERR_UNSPECIFIC;
              }
          }
          else if (serverStatus == SYNCML_DM_AUTHENTICATION_REQUIRED)
          {
              return serverStatus;
          }
    } /* Replace check */

    /* Check if this Status is for our "Alert" command (Our SessionDirection) */
    else if (DmStrncmp((char *)pContent->cmd->content, SYNCML_ALERT, pContent->cmd->length) == 0 &&
             DmStrncmp((char *)pContent->cmdRef->content, "1", pContent->cmdRef->length) == 0)
    {
          XPL_LOG_DM_TMN_Debug(("\ninside dm_ua_handlecommand::handleStatusCommand, alert command ref:%s\n\n", (char*)pContent->cmdRef->content));
          /* Determine the serverStatus for our Alert (Session Direction).*/
          p_auth = (UINT8 *)DmAllocMem(pContent->data->length+1);
          if ( p_auth == NULL )
          {
              return SYNCML_DM_DEVICE_FULL;
          }
          DmStrncpy((char *)p_auth,
                    (const char *)pContent->data->content,
                    pContent->data->length);
          p_auth[pContent->data->length] = '\0';
          serverStatus = SyncML2DMCode((char *)p_auth);
          DmFreeMem(p_auth);

          /* We only resend the DevInfo in the case of Authentication failure.*/
          if (serverStatus == SYNCML_DM_UNAUTHORIZED)
          {
              /* Call the method to build our DevInfo into the REPLACE command for this package. */
              ret_stat = pDmBuildPackage->BuildAlertCommand(pDmBuildPackage->getDirection(), NULL, NULL);
              if (ret_stat != SYNCML_DM_SUCCESS)
              {
                  sml_ret_stat = SML_ERR_UNSPECIFIC;
              }
              /* Call the method to build 1226 command for this package. */
              ret_stat = pDmBuildPackage->BuildAlert1226Command();
              if (ret_stat != SYNCML_DM_SUCCESS)
              {
                  sml_ret_stat = SML_ERR_UNSPECIFIC;
              }
          }
          else if (serverStatus == SYNCML_DM_AUTHENTICATION_REQUIRED)
          {
              return serverStatus;
          }
    } /* Replace check */

    /* Free the memory we allocated. */
    smlFreeStatus(pContent);

    return sml_ret_stat;
}

bool VerifyAlertItems(SmlAlertPtr_t  pContent)
{
   if ( pContent == NULL )
   {
     return false;
   }

   // Verify first alert item is not NULL.
   SmlItemListPtr_t  p_alert_list_item;
   p_alert_list_item = pContent->itemList;
   if (p_alert_list_item == NULL)
   {
      return false;
   }

   SmlItemPtr_t         p_alert_item;
   p_alert_item = p_alert_list_item->item;
   if (p_alert_item == NULL)
   {
      return false;
   }

   // Verify second alert item is not NULL.
   p_alert_list_item = p_alert_list_item->next;
   if (p_alert_list_item == NULL)
   {
      return false;
   }

   p_alert_item = p_alert_list_item->item;
   if (p_alert_item == NULL)
   {
      return false;
   }

   // Verify no more than 2 items are specified in alert.
   p_alert_list_item = p_alert_list_item->next;
   if (p_alert_list_item != NULL)
   {
      return false;
   }

   return true;
}
