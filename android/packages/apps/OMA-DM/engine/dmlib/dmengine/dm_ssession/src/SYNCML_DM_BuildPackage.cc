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

    Header Name: SYNCML_DM_BuildPackage.cc 

    General Description: Implementation of SYNCML_DM_BuildPackage class.

==================================================================================================*/

#include "SYNCML_DM_BuildPackage.H"
#include "dm_security.h"
#include "dmProcessScriptSession.h"
#include "GeneratePassword.H"
#include "dmt.hpp"
#include "dm_tree_util.h"   
#include "xpl_File.h"

extern "C" {
#include "xpt-b64.h"
#include "md5.h"
#ifdef __SML_XML__
#include <smldef.h>
#endif
#include <string.h>

#include "xlttags.h"

#ifdef __SML_XML__
extern const ESCAPE_CHAR_TABLE_T escape_char_table[];
#endif
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::SYNCML_DM_BuildPackage

DESCRIPTION     : The SYNCML_DM_BuildPackage class constructor.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_BuildPackage::SYNCML_DM_BuildPackage()
{
    commandId = 1;
    messageId = 1;
    MaxMessageSize = g_iDMWorkspaceSize; //DM_MAX_MSG_SIZE;
    MaxObjectSize = 0;

    pSyncHdr = NULL;

    /* We need the MgmtSessionObj pointer and current sendInstanceId.*/
    pDmMgmtSessionObj = NULL;
    sendInstanceId = 0;
    sessionDirection = SYNCML_DM_CLIENT_INITIATED_SESSION;
#ifdef LOB_SUPPORT
  	m_pChunkData = NULL;
	LargeObjectClear();
#endif
}


/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::~SYNCML_DM_BuildPackage

DESCRIPTION     : The SYNCML_DM_BuildPackage class destructor.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_BuildPackage::~SYNCML_DM_BuildPackage()
{
  Cleanup();
#ifdef LOB_SUPPORT
  LargeObjectClear();
#endif
}

void SYNCML_DM_BuildPackage::Cleanup()
{
    smlFreeSyncHdr(pSyncHdr);

    pMessageIdOfServer = NULL;
    pSyncHdr = NULL;
  
}

void SYNCML_DM_BuildPackage::Init(DMProcessScriptSession * pSession)
{
    pMessageIdOfServer = NULL;
    commandId = 1;
    MaxMessageSize = g_iDMWorkspaceSize; //DM_MAX_MSG_SIZE;
    pSyncHdr = NULL;

    /* We need the MgmtSessionObj pointer and current sendInstanceId.*/
    pDmMgmtSessionObj = pSession;
    sendInstanceId = pSession->GetSendInstanceId();
  
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::operator new

DESCRIPTION     : Operators to allocate memory for operation.
ARGUMENT PASSED : sz
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
void * SYNCML_DM_BuildPackage::operator new(size_t sz)
{
    return (DmAllocMem(sz));
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::operator delete

DESCRIPTION     : Operators to delete memory for operation.
ARGUMENT PASSED : buf
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
void SYNCML_DM_BuildPackage::operator delete(void *buf)
{
    DmFreeMem(buf);
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::GetMaxMessageSize

DESCRIPTION     : This function returns the value of MaxMessageSize.
ARGUMENT PASSED : 
OUTPUT PARAMETER: 
RETURN VALUE    : MaxMessageSize
IMPORTANT NOTES :
==================================================================================================*/
UINT32
SYNCML_DM_BuildPackage::GetMaxMessageSize() const
{
    return (MaxMessageSize);
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::GetMaxObjectSize

DESCRIPTION     : This function returns the value of MaxObjectSize.
ARGUMENT PASSED : 
OUTPUT PARAMETER: 
RETURN VALUE    : MaxMessageSize
IMPORTANT NOTES :
==================================================================================================*/
UINT32
SYNCML_DM_BuildPackage::GetMaxObjectSize() const
{
    return (MaxObjectSize);
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::SetMaxMessageSize

DESCRIPTION     : This function sets the value of MaxMessageSize.
ARGUMENT PASSED : passedin_MaxMsgSize
OUTPUT PARAMETER: 
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
void
SYNCML_DM_BuildPackage::SetMaxMessageSize(UINT32 passedin_MaxMsgSize)
{
    MaxMessageSize = passedin_MaxMsgSize;
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::SetMaxObjectSize

DESCRIPTION     : This function sets the value of MaxObjectSize.
ARGUMENT PASSED : passedin_MaxObjSize
OUTPUT PARAMETER: 
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
void
SYNCML_DM_BuildPackage::SetMaxObjectSize(UINT32 passedin_MaxObjSize)
{
    MaxObjectSize = passedin_MaxObjSize;
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::FreeGetStructData

DESCRIPTION     : This method will free the memeroy for the struct SYNCML_DM_GET_STRUCT_RET_DATA_T.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
void 
SYNCML_DM_BuildPackage:: FreeGetStructData(SYNCML_DM_GET_ON_LIST_RET_DATA_T& p_GetStructData)
{
  if (p_GetStructData.psRetData != NULL) 
  {
      delete p_GetStructData.psRetData;
      p_GetStructData.psRetData = NULL;
  }
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::BuildPcData

DESCRIPTION     : This method will build up the PC data structure.
ARGUMENT PASSED : content_type
                  extension
                  p_Content
OUTPUT PARAMETER: p_PcData
RETURN VALUE    : 
IMPORTANT NOTES : The calling function will not check the return value, since when memory allocation
                  fails, the phone will panic.
==================================================================================================*/
void
SYNCML_DM_BuildPackage::BuildPcData (SmlPcdataPtr_t   p_PcData,
                                     SmlPcdataType_t  content_type,
                                     SmlPcdataExtension_t extension,
                                     UINT32 dataSize,
                                     UINT8 *p_Content)
{
    BOOLEAN bEscapeChar = FALSE;
#ifdef __SML_XML__
    CPCHAR dm_escape_env = XPL_DM_GetEnv(SYNCML_DM_ESCAPE_CHAR);
#endif

    p_PcData->contentType = content_type;
    p_PcData->extension   = extension;
    p_PcData->length      = dataSize;

    if (p_PcData->content != NULL) {
      DmFreeMem(p_PcData->content);
    }

#ifdef __SML_XML__
    if (dm_escape_env && content_type == SML_PCDATA_STRING ) 
    {
        INT32 nLen = dataSize;

        while ( nLen > 0 )
        {
            nLen--;
        
            if ( p_Content[nLen] == '&' || p_Content[nLen] == '<'  ||
                 p_Content[nLen] == '>' || p_Content[nLen] == '\\' ||
                 p_Content[nLen] == '"' ) 
            {

                UINT32 i=0;
                for (i = 0; escape_char_table[i].escape_str != NULL; i++) 
                    if ( escape_char_table[i].token == p_Content[nLen])
                    {
                        dataSize += smlLibStrlen( escape_char_table[i].escape_str);
                        bEscapeChar = FALSE;
                        break;
                    }
            }
        }
    }
#endif
  

    p_PcData->content = (UINT8 *)DmAllocMem(dataSize+1);
    if ( p_PcData->content == NULL )
       return;

    memset(p_PcData->content, 0, dataSize+1);

    if(bEscapeChar == FALSE)
    {
        memcpy(p_PcData->content, p_Content, dataSize);

        if ( content_type == SML_PCDATA_STRING && !dm_escape_env) 
        {
            BOOLEAN bSendAsPCDATA = FALSE;
            INT32 nLen = dataSize;

            while ( nLen > 0 )
            {
                nLen--;
        
                if ( p_Content[nLen] == '&' || p_Content[nLen] == '<' ||
                     p_Content[nLen] == '>' || p_Content[nLen] == '\\' ||
                     p_Content[nLen] == '"') 
                {
                    bSendAsPCDATA = TRUE;
                    break;
                }
            }

            if ( bSendAsPCDATA )
                p_PcData->contentType = SML_PCDATA_CDATA;
        }
    }
#ifdef __SML_XML__
    else 
    {
        MemPtr_t buffer =(MemPtr_t) p_PcData->content;
        for(INT32 nLen = 0; nLen < p_PcData->length; nLen++)
        {
            if ( p_Content[nLen] == '&' || p_Content[nLen] == '<'  ||
                 p_Content[nLen] == '>' || p_Content[nLen] == '\\' ||
                 p_Content[nLen] == '"' ) 
            {

                *buffer++ = '&' ;
                UINT32 i = 0;
                for (i = 0; escape_char_table[i].escape_str != NULL; i++) 
                {
                    if ( escape_char_table[i].token == p_Content[nLen])
                    {
                        INT32 str_len = smlLibStrlen( escape_char_table[i].escape_str);
                        memcpy(buffer, (MemPtr_t)escape_char_table[i].escape_str, str_len);
                        buffer += str_len;
                        break;
                    }
                }
            } 
            else
                *buffer++=p_Content[nLen];
        }
        p_PcData->length = dataSize;
    }
#endif
    
    
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::BuildPcDataWAllocMem

DESCRIPTION     : This method will allocate the memory for PC data structure and call BuildPcData 
                  to build up the PC data structure.
ARGUMENT PASSED : 
                  p_Content
OUTPUT PARAMETER: p_PcData
RETURN VALUE    : 
IMPORTANT NOTES : The calling function will not check the return value, since when memory allocation
                  fails, the phone will panic.
==================================================================================================*/
void
SYNCML_DM_BuildPackage::BuildPcDataWAllocMem (SmlPcdataPtr_t   *pp_PcData,
                                              UINT32 dataSize,
                                              UINT8 *p_Data)
{
    SmlPcdataPtr_t         p_temp_pc_data;

    if (*pp_PcData != NULL) {
      smlFreePcdata(*pp_PcData);
    }

    p_temp_pc_data = smlAllocPcdata();
    if ( p_temp_pc_data != NULL)
    {
    	BuildPcData(p_temp_pc_data, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED, dataSize, p_Data);

    }
    *pp_PcData = p_temp_pc_data;
    p_temp_pc_data = NULL;
}


/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::BuildMetaInfo

DESCRIPTION     : This method will build up the meta data structure.
ARGUMENT PASSED : p_Format
                  p_Type
                  p_Size
                  p_NextNonce
                  p_Version
                  p_Maxmsgsize
                  p_Maxobjsize
OUTPUT PARAMETER: p_PcData
RETURN VALUE    : 
IMPORTANT NOTES : The calling function will not check the return value, since when memory allocation
                  fails, the phone will panic.
==================================================================================================*/
void
SYNCML_DM_BuildPackage::BuildMetaInfo (SmlPcdataPtr_t       p_PcData,
                                       UINT8 *p_Format,     UINT8 *p_Type,
                                       UINT8 *p_Mark,       UINT8 *p_Size, 
                                       UINT8 *p_NextNonce,  UINT8 *p_Version, 
                                       UINT8 *p_Maxmsgsize, UINT8 *p_Maxobjsize)
{
    SmlMetInfMetInfPtr_t p_meta_info;
    UINT32               data_length = 0;
    UINT32               data_size;

    p_meta_info = smlAllocMetInfMetInf();

	if ( p_meta_info != NULL)
	{
	    if (p_Format != NULL)        
    	{
        	data_size = DmStrlen((const char*)p_Format);
        	BuildPcDataWAllocMem (&p_meta_info->format, data_size, p_Format);
        	data_length += data_size;
    	}

    	if (p_Type != NULL)
    	{
        	data_size = DmStrlen((const char*)p_Type);
        	BuildPcDataWAllocMem (&p_meta_info->type, data_size, p_Type);
        	data_length += data_size;
    	}

    	if (p_Mark != NULL)
    	{
        	data_size = DmStrlen((const char*)p_Mark);
        	BuildPcDataWAllocMem (&p_meta_info->mark, data_size, p_Mark);
        	data_length += data_size;
    	}

    	if (p_Size != NULL)
    	{
        	data_size = DmStrlen((const char*)p_Size);
        	BuildPcDataWAllocMem(&p_meta_info->size, data_size, p_Size);
        	data_length += data_size;
    	}

    	if (p_NextNonce != NULL)
    	{
        	data_size = DmStrlen((const char*)p_NextNonce);
        	BuildPcDataWAllocMem(&p_meta_info->nextnonce, data_size, p_NextNonce);
        	data_length += data_size;
    	}

    	if (p_Version != NULL)
    	{
        	data_size = DmStrlen((const char*)p_Version);
        	BuildPcDataWAllocMem(&p_meta_info->version, data_size, p_Version);
        	data_length += data_size;
    	}

    	if (p_Maxmsgsize != NULL)
    	{
        	data_size = DmStrlen((const char*)p_Maxmsgsize);
        	BuildPcDataWAllocMem(&p_meta_info->maxmsgsize, data_size, p_Maxmsgsize);
        	data_length += data_size;
    	}

    	if (p_Maxobjsize != NULL)
    	{
        	data_size = DmStrlen((const char*)p_Maxobjsize);
        	BuildPcDataWAllocMem(&p_meta_info->maxobjsize, data_size, p_Maxobjsize);
        	data_length += data_size;
    	}

    	/* These elements are not used now. */
    	p_meta_info->mem = NULL;
    	p_meta_info->emi    = NULL;
    	p_meta_info->anchor = NULL;
	}

    p_PcData->contentType = SML_PCDATA_EXTENSION;
    p_PcData->extension   = SML_EXT_METINF;
    p_PcData->length      = sizeof(p_meta_info) + data_length;

    p_PcData->content     = p_meta_info;
}


/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::BuildMetaSizeInfo

DESCRIPTION     : This method will build up the meta size data structure.
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::BuildMetaSizeInfo(SmlPcdataPtr_t 	  p_PcData, int datasize)
{
    SmlMetInfMetInfPtr_t p_meta_info;
    UINT32				 data_length = 0;
    if(p_PcData->content != NULL)
    {		p_meta_info = (SmlMetInfMetInfPtr_t)p_PcData->content;
		data_length = p_PcData->length;
    }
    else
   {	
      p_meta_info = smlAllocMetInfMetInf();
	if(p_meta_info == NULL)
		return SYNCML_DM_DEVICE_FULL;
	data_length	 = sizeof(p_meta_info);
	p_PcData->contentType = SML_PCDATA_EXTENSION;
	p_PcData->extension   = SML_EXT_METINF;
    }
   // <Size> information already built
   if( p_meta_info->size!= NULL && p_meta_info->size->content != NULL)
   	return SYNCML_DM_SUCCESS;

   UINT32				data_size;
   UINT8 data_size_str[UINT32_TYPE_STR_SIZE_10]; 

   DmSprintf((char *)data_size_str, "%d", datasize);

   data_size = DmStrlen((const char*)data_size_str);
   BuildPcDataWAllocMem(&p_meta_info->size, data_size, data_size_str);
   data_length += data_size;
   
   p_PcData->length 	 =  data_length;
   
   p_PcData->content	 = p_meta_info;
   return SYNCML_DM_SUCCESS;
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::BuildStartSyncHdr

DESCRIPTION     : This function will be called by BuildPackageOne and HandleStartMessage to build up 
                  the SyncML package header.
             
                  The function composes the SyncML header data.
ARGUMENT PASSED : p_Content
OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_SUCCESS or SYNCML_DM_FAIL if memory allocation failed.
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SYNCML_DM_BuildPackage::BuildStartSyncHdr (SmlSyncHdrPtr_t p_Content, BOOLEAN isPackageOne)
{
    UINT8  message_id_str[UINT16_TYPE_STR_SIZE_5];
  
    pSyncHdr = smlAllocSyncHdr();

	if (pSyncHdr == NULL)
		return SYNCML_DM_FAIL;
		
    /* Convert message id of the server */
    if (p_Content->msgID->length == 0)
        pMessageIdOfServer = DEFAULT_MESSAGE_ID;
    else
    {
      /* Allocate the memory for pMessageIdOfServer, copy over the data. */
      pMessageIdOfServer.assign((const char *)p_Content->msgID->content,
            p_Content->msgID->length);
    }

 
    if (  dmTreeObj.IsVersion_12()  )
    {
	    /* Build the <VerDTD> element */
	    BuildPcData(pSyncHdr->version, SML_PCDATA_STRING,SML_EXT_UNDEFINED, 
	                DmStrlen((char *)SYNCML_REP_PROTOCOL_VERSION_1_2),
	                (UINT8 *)SYNCML_REP_PROTOCOL_VERSION_1_2);

	    /* Build the <VerProto> elelemnt */
	    BuildPcData(pSyncHdr->proto, SML_PCDATA_STRING, SML_EXT_UNDEFINED,
	                DmStrlen((char *)SYNCML_DM_PROTOCOL_VERSION_1_2),
	                (UINT8 *)SYNCML_DM_PROTOCOL_VERSION_1_2);
    }
    else
    {
	   /* Build the <VerDTD> element */
	    BuildPcData(pSyncHdr->version, SML_PCDATA_STRING,SML_EXT_UNDEFINED, 
	                DmStrlen((char *)SYNCML_REP_PROTOCOL_VERSION_1_1),
	                (UINT8 *)SYNCML_REP_PROTOCOL_VERSION_1_1);

	    /* Build the <VerProto> elelemnt */
	    BuildPcData(pSyncHdr->proto, SML_PCDATA_STRING, SML_EXT_UNDEFINED,
	                DmStrlen((char *)SYNCML_DM_PROTOCOL_VERSION_1_1),
	                (UINT8 *)SYNCML_DM_PROTOCOL_VERSION_1_1);	

    }	

    /* Build the <SessionId> element */
    BuildPcData(pSyncHdr->sessionID, SML_PCDATA_STRING, SML_EXT_UNDEFINED,
                p_Content->sessionID->length,
                (UINT8 *)p_Content->sessionID->content);

    /* Keep the sessionID information in the MgmtSessionObj's serverSessionId when building
       package one */
    if ( isPackageOne )
    {    
        /* The message ID has to be set as "1" for the package one, this is required by the
         * SyncML DM spec. */
        messageId = 1;

    }

    /* Build the <MsgId> element */
    DmSprintf((char *)message_id_str, "%d", messageId++);
    BuildPcData(pSyncHdr->msgID, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                DmStrlen((char *)message_id_str),
                message_id_str);

    /* Need to swap the <target> and the <source> element when constructing the DM document.
       The server SOURCE element from the receiving package will be client TARGET when sending back.
       The server TARGET element from the receiving package will be client SOURCE. */
    /* build the <target> element */ 

    /* build the <target> element, Need to use the response URI if it exists */ 
    DMString strSourceUri;
   if((p_Content->source != NULL) && (p_Content->source->locURI != NULL) &&(p_Content->source->locURI->content !=NULL))
    {         strSourceUri = DMString((const char *)p_Content->source->locURI->content, (int)p_Content->source->locURI->length);
        if(strSourceUri.Encode() == FALSE)
        return SYNCML_DM_FAIL;
   }

    if (p_Content->respURI != NULL)
    {
        BuildPcData(pSyncHdr->target->locURI, SML_PCDATA_STRING,
                    SML_EXT_UNDEFINED,
                    p_Content->respURI->length,
                    (UINT8 *)p_Content->respURI->content);
    }
    else
    {
        BuildPcData(pSyncHdr->target->locURI, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                strSourceUri.length(),
                (UINT8 *)strSourceUri.c_str());
    }
    if ((p_Content->source !=NULL) && (p_Content->source->locName != NULL))
    {
        if (p_Content->source->locName->content != NULL)
        {
            BuildPcDataWAllocMem(&pSyncHdr->target->locName,
                                 p_Content->source->locName->length,
                                 (UINT8 *)p_Content->source->locName->content);
        }
        else 
        {
            pSyncHdr->target->locName = NULL;
        }
    }

    /* Build the <source> element */
    DMString strTargetUri;
    if((p_Content->target != NULL) && (p_Content->target->locURI != NULL) && (p_Content->target->locURI->content != NULL))
    {
        strTargetUri = DMString((CPCHAR)p_Content->target->locURI->content, (INT32)p_Content->target->locURI->length);
        if(strTargetUri.Encode() == FALSE)
           return SYNCML_DM_FAIL;
    }
    BuildPcData(pSyncHdr->source->locURI, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                strTargetUri.length(),
                (UINT8 *)strTargetUri.c_str());
    
    if((p_Content->target != NULL) && (p_Content->target->locName != NULL))
    {
        if (p_Content->target->locName->content != NULL)
        {
            BuildPcDataWAllocMem(&pSyncHdr->source->locName,
                                 p_Content->target->locName->length,
                                 (UINT8 *)p_Content->target->locName->content);
        }
        else 
        {
            pSyncHdr->source->locName = NULL;
        } 
    }

    /* Don't free the pSyncHdr memory until the BuildFinishSyncHdr() is called.*/
    return SYNCML_DM_SUCCESS;
}


/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::BuildFinishSyncHdr

DESCRIPTION     : This function will be called by BuildPackageOne and HandleStartMessage to
                  build up the SyncML package header.
             
                  The function finishes composing the SyncML header data, and calls the SyncML
                  toolkit smlStartMessageExt() to build the SyncHdr.
ARGUMENT PASSED : serverChalType
OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_SUCCESS or SYNCML_DM_FAIL
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SYNCML_DM_BuildPackage::BuildFinishSyncHdr (SYNCML_DM_CHAL_TYPE_T serverChalType)
{
    UINT8           max_msg_size_str[UINT32_TYPE_STR_SIZE_10];
    UINT8          *p_CredFormat = NULL;
    UINT8          *p_CredType   = NULL;
    UINT8          *p_CredData   = NULL;
    Ret_t           sml_ret_stat;
    SYNCML_DM_RET_STATUS_T      ret_stat = SYNCML_DM_SUCCESS;
    SYNCMLDM_MD5_SEC_INFO_T     md5SecInfo;
    SYNCMLDM_BASIC_SEC_INFO_T   basicSecInfo;
    DMClientServerCreds *pClientServerCreds;
    SYNCMLDM_SEC_CREDENTIALS_T  *pGenCred = NULL;
    UINT8                       decodedNonce[MAX_BIN_NONCE_LEN];
    UINT32                      encodedNonceLen;
    UINT32                      decodedNonceLen;
    MemSize_t                   freeSpace;


    pClientServerCreds = pDmMgmtSessionObj->GetClientServerCreds();

    // change the auth method from server.
    pClientServerCreds->SetPrefClientAuth(serverChalType);

    /* The HMAC Credentials are always sent. So we do not need to do anything extra if challenged
     * for HMAC.*/
    if ((serverChalType != SYNCML_DM_CHAL_NONE) && (serverChalType != SYNCML_DM_CHAL_HMAC))
    {

        if (serverChalType == SYNCML_DM_CHAL_MD5)
        {
            /* The ClientNonce string is b64 encoded and must be decoded now.*/
            encodedNonceLen = DmStrlen((const char *)pClientServerCreds->pClientNonce);
            decodedNonceLen = base64Decode((unsigned char *)decodedNonce,
                                    MAX_BIN_NONCE_LEN, 
                                    (unsigned char*)pClientServerCreds->pClientNonce.c_str(),
                                    (unsigned long*)&encodedNonceLen); 
    
            /* We need to build up our MD5 credentials.*/            
            md5SecInfo.pb_user_name_or_server_id = (UINT8*)pClientServerCreds->pClientUserName.c_str();
            md5SecInfo.pb_password = (UINT8*)pClientServerCreds->pClientPW.c_str();
            md5SecInfo.pb_nonce = decodedNonce;
            md5SecInfo.o_encode_base64 = TRUE;
            md5SecInfo.w_nonce_length = decodedNonceLen;
            
            pGenCred = syncmldm_sec_build_md5_cred(&md5SecInfo);
            
            p_CredFormat = (UINT8 *)SYNCML_B64;
            p_CredType = (UINT8 *)SYNCML_AUTH_MD5;
        }
        else /* SYNCML_DM_CHAL_BASIC */
        {
            /* We need to build up our Basic credentials.*/
            basicSecInfo.pb_user_name_or_server_id = (UINT8*)pClientServerCreds->pClientUserName.c_str();
            basicSecInfo.pb_password = (UINT8*)pClientServerCreds->pClientPW.c_str();
            
            pGenCred = syncmldm_sec_build_basic_cred(&basicSecInfo);
            
            p_CredFormat = (UINT8 *)SYNCML_B64;
            p_CredType = (UINT8 *)SYNCML_AUTH_BASIC;
        }
        
        if (pGenCred == NULL)
        {
            /* Something went wrong in the Security Library.*/
            p_CredData = NULL;
            p_CredFormat = NULL;
            p_CredType = NULL;
        }
        else
        {
            /* Copy over the new credential string and add the NULL char.*/
            p_CredData = (UINT8 *)DmAllocMem(pGenCred->w_credential_string_length + 1);
            if ( p_CredData == NULL )
            {
               DmFreeMem(pGenCred);
               return SYNCML_DM_FAIL;
            }

            memcpy(p_CredData, pGenCred->ab_credential_string,
                   pGenCred->w_credential_string_length);
            p_CredData[pGenCred->w_credential_string_length] = '\0';
            DmFreeMem(pGenCred);
            
            /* Build the <Cred> element */
        if( pSyncHdr != NULL) {
            pSyncHdr->cred    = smlAllocCred();
            if  (pSyncHdr->cred != NULL)
            {
            	pSyncHdr->cred->meta = smlAllocPcdata();

            	/* Build the MetaInfo and the PcData sections.*/   
				if (pSyncHdr->cred->meta != NULL)
            		BuildMetaInfo(pSyncHdr->cred->meta, p_CredFormat, p_CredType,
                          NULL, NULL, NULL, NULL, NULL, NULL);

            	BuildPcData(pSyncHdr->cred->data, SML_PCDATA_STRING,
                        SML_EXT_UNDEFINED,
                        DmStrlen((char *)p_CredData),
                        p_CredData);
            	}
            }
            DmFreeMem(p_CredData);
        }
    } /* If Challenged */
    
    /* For package one header, fill in the maximum message size which client can support to the 
       pSyncHdr->meta */
    {
      if( pSyncHdr != NULL) {
        pSyncHdr->meta = smlAllocPcdata();
        
        /* Convert MaxmessageSize to a string */
        DmSprintf((char *)max_msg_size_str, "%d", MaxMessageSize);

        /* Call BuildMetaInfo to set MaxMsgSize element */
        if (pSyncHdr->meta != NULL)
        	BuildMetaInfo(pSyncHdr->meta,
                      NULL, NULL, NULL,
                      NULL, NULL, NULL, 
                      max_msg_size_str,
                      NULL);
          }
    }

    /* Start a new message, toolkit will build up the DM package SyncHdr part */
    if( pSyncHdr != NULL) {
	if ( dmTreeObj.IsVersion_12()  )
        	sml_ret_stat = smlStartMessageExt(sendInstanceId, pSyncHdr, SML_VERS_1_2);
	else
		sml_ret_stat = smlStartMessageExt(sendInstanceId, pSyncHdr, SML_VERS_1_1);
        if (sml_ret_stat != SML_ERR_OK)
        {
            ret_stat = SYNCML_DM_FAIL;
        }    
   }
    freeSpace = smlGetFreeBuffer(sendInstanceId);

    /* Free the memory. smlFreeSyncHdr will free every element in SmlSyncHdrPtr_t structure, 
       including ones we allocated. */
    if( pSyncHdr != NULL) 
        smlFreeSyncHdr(pSyncHdr);
    pSyncHdr = NULL;

    return (ret_stat);
}


/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::BuildAlertCommand

DESCRIPTION     : This method calls the SyncML Toolkit functions to build up the SyncML package 
                  ALERT command as part of the SyncBody.
ARGUMENT PASSED : session_Direction
OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_SUCCESS or SYNCML_DM_FAIL if memory allocation failed.
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SYNCML_DM_BuildPackage::BuildAlert1226Command ()
{
    SmlAlertPtr_t  p_alert; 
    UINT8  command_id_str[UINT16_TYPE_STR_SIZE_5];
    if(firmAlertVec.size() < 1)
    {
        return SYNCML_DM_SUCCESS;
    }
    DMFirmAlertVector * pFirmAlerts = &firmAlertVec;
    char * alertStr = "1226";

    for (INT32 i=0; i<pFirmAlerts->size(); i++)     
    {
        p_alert = smlAllocAlert();

        if ( p_alert == NULL )
            return (SYNCML_DM_FAIL);

       /* Convert the commandId to a string */
        DmSprintf((char *)command_id_str, "%d", commandId++);
        BuildPcData(p_alert->cmdID, SML_PCDATA_STRING,SML_EXT_UNDEFINED,
                    DmStrlen((char *)command_id_str),command_id_str);

        /* Credentials are not sent in the Alert.*/
        p_alert->cred = NULL;

        BuildPcDataWAllocMem (&p_alert->data,4,(UINT8*)alertStr);
        if ( p_alert->data == NULL )
        {
            smlFreeAlert(p_alert);
            return (SYNCML_DM_FAIL);  
        }
      
        SmlItemPtr_t p_item = p_alert->itemList->item;    
        const DmtFirmAlert& alert = (*pFirmAlerts)[i];
        DMString uri = alert.getPackageURI();
      
        if(uri.Encode() == FALSE)
        {
            smlFreeAlert(p_alert);
            return (SYNCML_DM_FAIL);  
        }

        if ( !alert.getCorrelator().empty() ) 
        {
            p_alert->correlator = smlAllocPcdata();
            if ( !p_alert->correlator )
            {
                smlFreeAlert(p_alert);
                return (SYNCML_DM_FAIL);  
            }
          
            BuildPcData(p_alert->correlator, SML_PCDATA_STRING,SML_EXT_UNDEFINED,
                        alert.getCorrelator().length(), (UINT8*)alert.getCorrelator().c_str() );
        }
          
         /* Dont build the LocURI  if its length is zero */
        if( uri.length() != 0 )
        {
            p_item->source = smlAllocSource();
            if ( p_item->source == NULL )
            {
               smlFreeAlert(p_alert);
               return (SYNCML_DM_FAIL);  
            }
            BuildPcData(p_item->source->locURI,SML_PCDATA_STRING,
                        SML_EXT_UNDEFINED,uri.length(),(UINT8*)uri.c_str());
        }

        UINT8* pType = alert.getAlertType().empty() ? NULL : (UINT8*)alert.getAlertType().c_str();
        UINT8* pFormat = alert.getAlertFormat().empty() ? NULL : (UINT8*)alert.getAlertFormat().c_str();
        UINT8* pMark = alert.getAlertMark().empty() ? NULL : (UINT8*)alert.getAlertMark().c_str();

        if ( pType || pFormat || pMark )
        {
             if ((p_item->meta = smlAllocPcdata()) != NULL)
                BuildMetaInfo(p_item->meta, pFormat, pType, pMark,NULL,NULL,NULL,NULL, NULL);
        }
          
        BuildPcDataWAllocMem (&p_item->data,
                              alert.getResultData().length(),
                              (UINT8*)alert.getResultData().c_str() );

        if ( p_item->data == NULL )
        {
                smlFreeAlert(p_alert);
                return (SYNCML_DM_FAIL);  
        } 

  
         /* Call the toolkit to build ALERT command to the DM package body. */
         smlAlertCmd(sendInstanceId, p_alert);   

        /* Free the memory. smlFreeAlert will free every element in SmlAlertPtr_t structure, 
           including ones we allocated. */
        smlFreeAlert(p_alert);
        p_alert = NULL;
           
    }

    return (SYNCML_DM_SUCCESS);
}


/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::BuildAlertCommand

DESCRIPTION     : This method calls the SyncML Toolkit functions to build up the SyncML package 
                  ALERT command as part of the SyncBody.
ARGUMENT PASSED : session_Direction
OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_SUCCESS or SYNCML_DM_FAIL if memory allocation failed.
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SYNCML_DM_BuildPackage::BuildAlertCommand (UINT16 alertCode, CPCHAR pSource, CPCHAR pTarget)
{
    SmlAlertPtr_t  p_alert;
    UINT8          command_id_str[UINT16_TYPE_STR_SIZE_5];
    UINT8          alert_str[UINT16_TYPE_STR_SIZE_5];
    SmlItemPtr_t     p_alert_item;

    if ( (p_alert = smlAllocAlert() ) == NULL)
    	return SYNCML_DM_FAIL;

    /* Convert the commandId to a string */
    DmSprintf((char *)command_id_str, "%d", commandId++);
    BuildPcData(p_alert->cmdID, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                DmStrlen((char *)command_id_str),
                command_id_str);

    /* Credentials are not sent in the Alert.*/
    p_alert->cred = NULL;

    if ( (p_alert->data = smlAllocPcdata()) != NULL)
	{
    /* We need to indicate whether the alert type is SERVER_INITIATED_SESSION or
     * CLIENT_INITIATED_SESSION */
    DmSprintf((char *)alert_str, "%d", alertCode);
    BuildPcData(p_alert->data, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                DmStrlen((char *)alert_str),
                alert_str);
	}
	
    /* Free p_alert->itemList, set it as NULL since we don't have data for it. Otherwise, empty
       <ITEM/> will be generated for SyncHdr. */
	if(pSource == NULL && pTarget== NULL)
	{    smlFreeItemList(p_alert->itemList);
	    p_alert->itemList = NULL;
	}
	else
	{
		p_alert_item =  p_alert->itemList->item;

   // Source
		if(pSource != NULL)
	  	{	p_alert_item->source = smlAllocSource();
		  	if (p_alert_item->source == NULL )
	   		{
			   smlFreeAlert(p_alert);
			   return (SYNCML_DM_FAIL);  
	   		}
	   		DMString   strSourceUri(pSource);
   
   			if(strSourceUri.Encode() == FALSE)
		   	{
			   smlFreeAlert(p_alert);
			   return (SYNCML_DM_INVALID_URI);
	   		}
  
   			BuildPcData(p_alert_item->source->locURI, SML_PCDATA_STRING,
			   SML_EXT_UNDEFINED,
			   strSourceUri.length(),
			  (UINT8 *) strSourceUri.c_str());
		}

   // Target
		if(pTarget != NULL)
	  	{	p_alert_item->target = smlAllocSource();
		  	if (p_alert_item->target == NULL )
   			{
			   smlFreeAlert(p_alert);
			   return (SYNCML_DM_FAIL);  
   			}
	   		DMString   strTargetUri(pTarget);
   
   			if(strTargetUri.Encode() == FALSE)
	   		{
			   smlFreeAlert(p_alert);
			   return (SYNCML_DM_INVALID_URI);
	   		}
  
   			BuildPcData(p_alert_item->target->locURI, SML_PCDATA_STRING,
			   SML_EXT_UNDEFINED,
			   strTargetUri.length(),
			  (UINT8 *) strTargetUri.c_str());
		}
  }

    /* Call the toolkit to build ALERT command to the DM package body. */
    smlAlertCmd(sendInstanceId, p_alert);   

    /* Free the memory. smlFreeAlert will free every element in SmlAlertPtr_t structure, 
       including ones we allocated. */
    smlFreeAlert(p_alert);

    return SYNCML_DM_SUCCESS;
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::BuildReplaceCommand

DESCRIPTION     : This method calls the SyncML Toolkit functions to build up the SyncML package 
                  REPLACE command as part of the SyncBody of DM package one. It will query the DM 
                  tree to get all DevInfo data to place in the REPLACE command.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_SUCCESS or SYNCML_DM_FAIL if memory allocation failed.
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SYNCML_DM_BuildPackage::BuildReplaceCommand ()
{
    UINT8            command_id_str[UINT16_TYPE_STR_SIZE_5];
    SmlReplacePtr_t  p_replace; 
    SmlItemPtr_t     p_replace_item;
    SmlItemListPtr_t p_replace_item_list;
    DMGetData devInfoData;
    SYNCML_DM_RET_STATUS_T     dm_stat  = SYNCML_DM_SUCCESS;
    SYNCML_DM_URI_RESULT_T dm_uri_result;
    BOOLEAN                first_item_on_list = TRUE;

    SYNCML_DM_GET_ON_LIST_RET_DATA_T p_get_struct_data;

    if ( (p_replace = (SmlReplacePtr_t)smlAllocReplace()) == NULL)
    	return (SYNCML_DM_FAIL);

    /* Convert the commandId to a string */
    DmSprintf((char *)command_id_str, "%d", commandId++);
    BuildPcData(p_replace->cmdID, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                DmStrlen((char *)command_id_str),
                (UINT8 *)command_id_str);

    p_replace_item = p_replace->itemList->item;
    p_replace_item_list = p_replace->itemList;
   

    /* Call DMTNM to get all children nodes of devinfo from DM tree */
    dm_stat = dmTreeObj.InitListAndGetListFirstItem( DM_DEV_INFO_URI_PAK1,
                                                     SYNCML_DM_GET_ON_LIST_STRUCT,
                                                     p_get_struct_data);

    if (dm_stat != SYNCML_DM_SUCCESS)
    {
        smlFreeGeneric(p_replace);
        FreeGetStructData(p_get_struct_data);
        return (SYNCML_DM_FAIL);
    }

    /* Loop through all data from Get Struct */
    while (p_get_struct_data.psRetData != NULL)
    {
        /* If it is an interior node, we don't send it in the package one. */
        if (p_get_struct_data.psRetData->m_nFormat != SYNCML_DM_FORMAT_NODE)
        {
            if (first_item_on_list != TRUE)
            {
                /* It is the first item, allocate the memory for the next replace item. */
                p_replace_item_list->next = smlAllocItemList();
                if (p_replace_item_list->next  != NULL)
                {
                	p_replace_item_list = p_replace_item_list->next;
                	p_replace_item = p_replace_item_list->item;
                }
            }
            else
            {
                /* This is the first item. Set the flag to False for the next time around.*/
                first_item_on_list = FALSE;
            }

            /* Allocate the memory for p_replace_item->source */
            p_replace_item->source = smlAllocSource();

            if ( p_replace_item->source == NULL )
            {
                smlFreeGeneric(p_replace);
                FreeGetStructData(p_get_struct_data);
                return (SYNCML_DM_FAIL);  
            }

            DMString   strGetetUri( p_get_struct_data._pbURI );

            if(strGetetUri.Encode() == FALSE)
            {
                smlFreeGeneric(p_replace);
                FreeGetStructData(p_get_struct_data);
                return (SYNCML_DM_INVALID_URI);
            }

            BuildPcData(p_replace_item->source->locURI, SML_PCDATA_STRING,
                        SML_EXT_UNDEFINED,
                        strGetetUri.length(),
                       (UINT8 *) strGetetUri.c_str());

            /* Validate the URI */
            dm_uri_result = dmTreeObj.URIValidateAndParse( p_get_struct_data._pbURI);
            if ((dm_uri_result == SYNCML_DM_COMMAND_ON_UNKNOWN_PROPERTY) ||
                (dm_uri_result == SYNCML_DM_COMMAND_INVALID_URI) ||
                (dm_uri_result == SYNCML_DM_COMMAND_URI_TOO_LONG) ||
                (dm_uri_result == SYNCML_DM_COMMAND_LIST_STRUCT))
            {
                smlFreeGeneric(p_replace);
                FreeGetStructData(p_get_struct_data);
                return (SYNCML_DM_INVALID_URI);
            }

            /* Get data for each node of devinfo */
            dm_stat = dmTreeObj.Get(p_get_struct_data._pbURI, devInfoData,SYNCML_DM_REQUEST_TYPE_INTERNAL);

            FreeGetStructData(p_get_struct_data);

            /* Check if there is data on this node.*/
            if (dm_stat != SYNCML_DM_SUCCESS)
            {
               smlFreeGeneric(p_replace);
               return (SYNCML_DM_FAIL);
            }

            p_replace_item->data = smlAllocPcdata();
            if (p_replace_item->data != NULL) 
            	BuildPcData(p_replace_item->data, SML_PCDATA_STRING, SML_EXT_UNDEFINED,
                        devInfoData.m_oData.getSize(),
                        devInfoData.m_oData.getBuffer());

            /* Replace doesn't have any meta.*/
            p_replace_item->meta = NULL;
          
        }
        
        else /* bDataFormat == SYNCML_DM_FORMAT_NODE */
        {
            /* We need to free the memory for the interior node.*/
            FreeGetStructData(p_get_struct_data);
        }

        /* Move to the next item. */
        dm_stat = dmTreeObj.GetListNextItem(p_get_struct_data);
        if (dm_stat != SYNCML_DM_SUCCESS)
        {
            smlFreeGeneric(p_replace);
            FreeGetStructData(p_get_struct_data);
            return (SYNCML_DM_FAIL);
        }
    }

    /* All replace command data are set, call toolkit to build REPLACE into the DM package body. */
    smlReplaceCmd(sendInstanceId, p_replace);
   
    /* Free the memory. smlFreeGeneric will free every element of SmlReplacePtr_t structure. */
    smlFreeGeneric(p_replace);

    return (SYNCML_DM_SUCCESS);
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::AllocateStatus

DESCRIPTION     : This function calls the SyncML Toolkit functions to build up the STATUS command
ARGUMENT PASSED : p_CmdRefData
                  p_CmdName
                  p_TargetRefData
                  p_SourceRefData
                  pCientChal
                  status_Code
OUTPUT PARAMETER:
RETURN VALUE    : newly constructed status
IMPORTANT NOTES :
==================================================================================================*/
SmlStatusPtr_t SYNCML_DM_BuildPackage::AllocateStatus(
    UINT8          *p_CmdRefData,        /* Receiving DM package command Id */
    UINT8          *p_CmdName,
    UINT8          *p_SourceRefData,
    UINT8          *p_TargetRefData,
    DM_CHALLENGE_T *pClientChal,
    SYNCML_DM_RET_STATUS_T status_Code,
    const DMStringVector*   responses )
{
    UINT8          status_code_str[UINT16_TYPE_STR_SIZE_5];
    SmlStatusPtr_t p_status = smlAllocStatus();

    if ( !p_status )
      return NULL;
    
    BuildPcData(p_status->msgRef, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                DmStrlen(pMessageIdOfServer),
                (UINT8*)pMessageIdOfServer.c_str());

    if (p_CmdRefData != NULL)
    {
        BuildPcData(p_status->cmdRef, SML_PCDATA_STRING,
                    SML_EXT_UNDEFINED,
                    DmStrlen((char *)p_CmdRefData),
                    p_CmdRefData);
    }

    if (p_CmdName != NULL)
    {
        BuildPcData(p_status->cmd, SML_PCDATA_STRING,
                    SML_EXT_UNDEFINED,
                    DmStrlen((char *)p_CmdName),
                    p_CmdName);
    }

    if (p_SourceRefData != NULL)
    {
       if ( (p_status->sourceRefList = smlAllocSourceRefList()) != NULL)
		{
        	BuildPcData(p_status->sourceRefList->sourceRef, SML_PCDATA_STRING,
                    SML_EXT_UNDEFINED,
                    DmStrlen((char *)p_SourceRefData),
                    p_SourceRefData);
		}
    }

    if (p_TargetRefData != NULL)
    {
        if (0 == DmStrcmp((char *)p_TargetRefData, FDR_URI))
        {
            SmlItemPtr_t     p_status_item = NULL;
            SmlItemListPtr_t p_status_list_item = NULL;
            p_status->itemList = smlAllocItemList();
            p_status_list_item = p_status->itemList;
            p_status_item = p_status_list_item->item;
            if ((p_status_item->source = smlAllocSource()) != NULL)
                 BuildPcData(p_status_item->source->locURI, SML_PCDATA_STRING,
                             SML_EXT_UNDEFINED,
                             DmStrlen((char *)p_TargetRefData),
                             p_TargetRefData);
        }
        else
        {
            if ((p_status->targetRefList = smlAllocTargetRefList()) != NULL)
                 BuildPcData(p_status->targetRefList->targetRef, SML_PCDATA_STRING,
                             SML_EXT_UNDEFINED,
                             DmStrlen((char *)p_TargetRefData),
                             p_TargetRefData);
        }
    }

    if (pClientChal != NULL)
    {
        if (( pClientChal->pChalFormat != NULL ) ||
            ( pClientChal->pChalType   != NULL ) ||
            ( pClientChal->pChalNonce  != NULL ))
        {
            if ( (p_status->chal = smlAllocChal() ) != NULL) 
            	BuildMetaInfo(p_status->chal->meta, 
                          pClientChal->pChalFormat,
                          pClientChal->pChalType,
                          NULL,
                          NULL, 
                          pClientChal->pChalNonce,
                          NULL, NULL, NULL);
        }
    }

    /* Convert the status_Code to a string */
    if ( status_Code == SYNCML_DM_SUCCESS )
        status_Code = 200;
    else if ( status_Code == SYNCML_DM_PROCESS_ACCEPTED )
        status_Code = 1200;
    else
    {
      	if ( status_Code < 100 || status_Code > 600 )
       	status_Code = 500;
    }
    
    
    DmSprintf((char *)status_code_str, "%d", (int)status_Code);
    BuildPcData(p_status->data, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                DmStrlen((char *)status_code_str),
                status_code_str);

    if (responses && responses->size() > 0) {

      p_status->itemList = smlAllocItemList();
      SmlItemListPtr_t itemListPtr_t = p_status->itemList;
      int i = 0;
      do {
      			if ( itemListPtr_t != NULL)
    			{
    				if ( (itemListPtr_t->item->data = smlAllocPcdata() )  != NULL)
    					BuildPcData(itemListPtr_t->item->data, SML_PCDATA_STRING,
            								SML_EXT_UNDEFINED,
            								(UINT32)DmStrlen((*responses)[i].c_str()),
            								(UINT8*)(*responses)[i].c_str());
    			}
    			i++;
    			if (i < responses->size()) {
      				if ( itemListPtr_t != NULL)
    				{
      					itemListPtr_t->next = smlAllocItemList();
      					itemListPtr_t = itemListPtr_t->next;
    				}
    			}
      } while (i < responses->size());
    }
  return p_status;    
}

SYNCML_DM_RET_STATUS_T SYNCML_DM_BuildPackage::BuildStatus(SmlStatusPtr_t p_status )
{
    MemSize_t freeSpace = 0;
    Ret_t sml_ret_stat = 0;
    SYNCML_DM_RET_STATUS_T  ret_stat = SYNCML_DM_SUCCESS;
    UINT8          command_id_str[UINT16_TYPE_STR_SIZE_5];

    /* Convert the commandId to a string */
    DmSprintf((char *)command_id_str, "%d", commandId++);

    BuildPcData(p_status->cmdID, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                DmStrlen((char *)command_id_str),
                command_id_str);

    
    /* Call the toolkit to construct the STATUS command */
    smlStartEvaluation(sendInstanceId);
    sml_ret_stat = smlStatusCmd(sendInstanceId, p_status);
    smlEndEvaluation(sendInstanceId, &freeSpace);

    /* Note that freeSpace accounts for the EndSyncmlDoc overhead as well.*/
    if (freeSpace > 0)
    {
        sml_ret_stat = smlStatusCmd(sendInstanceId, p_status);
    }
    else
    {
        ret_stat = SYNCML_DM_RESULTS_TOO_LARGE;
    }

    /* Free the memory. smlFreeStatus will free every element in the SmlStatusPtr_t structure. */
    //smlFreeStatus(p_status);

    return (ret_stat);
}

#ifdef TNDS_SUPPORT
BOOLEAN SYNCML_DM_BuildPackage::ListTndsProp( CPCHAR uri, CPCHAR prop, BOOLEAN &listProp)
{
   // Include property '+' and remove property '-' are not possible 
   // to combine in the same Get command
   BOOLEAN includeProp =  NULL != DmStrchr(uri, '+');
   BOOLEAN removeProp  =  NULL != DmStrchr(uri, '-');
   if ( includeProp && removeProp )
   {
      listProp = false;
      return  false;
   }

   // Return all properties by default
   if ( !includeProp && !removeProp )
   {
      listProp = true;
      return  true;
   }

   // Return selected properties only
   char *szQPos = DmStrstr(uri, prop);
   if ( includeProp )
   {
      listProp = NULL != szQPos;
      if ( listProp && *(szQPos-1) != '+' )
      {
         KCDBG("Invalid URI: %s, prop: %s\n", uri, prop);
         listProp = false;
         return false;
      }
   }

   if ( removeProp )
   {
      listProp = NULL == szQPos;
      if ( !listProp && *(szQPos-1) != '-' )
      {
         KCDBG("Invalid URI: %s, prop: %s\n", uri, prop);
         listProp = false;
         return false;
      }
   }

   return true;
}
#endif //TNDS_SUPPORT

#ifdef TNDS_SUPPORT
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::AllocateTndsResult( CPCHAR p_target_uri,
                                         DMGetData *p_get_ret_data,
                                         const SYNCML_DM_GET_ON_LIST_RET_DATA_T& oGetStructData, 
                                         SmlPcdataPtr_t pPcDataPtr )
{
   BOOLEAN listACL, listFormat, listName, listSize, listTitle, listTStamp, listVerNo, listValue;

   BOOLEAN listProps = ListTndsProp(p_target_uri, "ACL", listACL) &&
                       ListTndsProp(p_target_uri, "Format", listFormat) &&
                       ListTndsProp(p_target_uri, "Name", listName) &&
                       ListTndsProp(p_target_uri, "Size", listSize) &&
                       ListTndsProp(p_target_uri, "Title", listTitle) &&
                       ListTndsProp(p_target_uri, "TStamp", listTStamp) &&
                       ListTndsProp(p_target_uri, "VerNo", listVerNo) &&
                       ListTndsProp(p_target_uri, "Value", listValue);
   if ( !listProps )
   {
      return SYNCML_DM_COMMAND_INVALID_URI;
   }

   SmlDmTndPtr_t p_tnd_info = NULL;
   UINT32        data_length = 0;
   UINT32        data_size;

   // Allocate TNDS object
   data_size = sizeof(SmlDmTndPtr_t);
   p_tnd_info = smlAllocDmTnd();
   if ( p_tnd_info == NULL)
   {
      return SYNCML_DM_DEVICE_FULL;
   }
   data_length += data_size;
   
   // Append VerDTD Tag
   data_size = 3;
   BuildPcDataWAllocMem (&p_tnd_info->verdtd, data_size, (UINT8*)SYNCML_DM_PROTOCOL_VERSION_1_2+3);
   data_length += data_size;

   // Append Mod Tag
   SYNCML_DM_RET_STATUS_T nRes = SYNCML_DM_SUCCESS;
   DMGetData tmpGetData;
   nRes = dmTreeObj.Get(DM_DEV_INFO_MOD_URI, tmpGetData, SYNCML_DM_REQUEST_TYPE_INTERNAL);
   if (nRes != SYNCML_DM_SUCCESS) 
   {
       smlFreeDmTnd(p_tnd_info);
       return (SYNCML_DM_FAIL);
   }
   data_size = tmpGetData.m_oData.getSize();
   if (data_size != 0)
   {
      BuildPcDataWAllocMem(&p_tnd_info->mod, data_size, (UINT8*) tmpGetData.getCharData());
      data_length += data_size;
   }

   // Append Man Tag
   nRes = dmTreeObj.Get(DM_DEV_INFO_MAN_URI, tmpGetData, SYNCML_DM_REQUEST_TYPE_INTERNAL);
   if (nRes != SYNCML_DM_SUCCESS) 
   {
       smlFreeDmTnd(p_tnd_info);
       return (SYNCML_DM_FAIL);
   }
   data_size = tmpGetData.m_oData.getSize();
   if (data_size != 0)
   {
      BuildPcDataWAllocMem(&p_tnd_info->man, data_size, (UINT8*) tmpGetData.getCharData());
      data_length += data_size;
   }

   // TNDS last node list
   p_tnd_info->nodelist = NULL;
   SmlDmTndNodeListPtr_t last = NULL;

   // Allocate TNDS node object(s)
   SYNCML_DM_GET_ON_LIST_RET_DATA_T oTmpGetStructData = oGetStructData;
   DMGetData *pTmpGetData = NULL;
   CPCHAR   pTmpTargetUri = p_target_uri;
   nRes = dmTreeObj.GetListNextItem(oTmpGetStructData);
   if ( nRes == SYNCML_DM_SUCCESS )
   {
      pTmpGetData = oTmpGetStructData.psRetData;
      oTmpGetStructData.psRetData = NULL;
      pTmpTargetUri = oTmpGetStructData._pbURI;
   }

   DMString targetUri = oTmpGetStructData.m_strStartURI;
   DMString tmpNodeUri = "";
   DMString tmpStr = "";
   DmtAttributes oAttr;
   while ( nRes == SYNCML_DM_SUCCESS && NULL != pTmpGetData )
   {
      // Get DMT node attribute
      nRes = dmTreeObj.GetAttributes( pTmpTargetUri, oAttr, SYNCML_DM_REQUEST_TYPE_INTERNAL );
      if ( nRes != SYNCML_DM_SUCCESS )
      {
         break;
      }

      // Allocate TNDS node list object
      data_size = sizeof(SmlDmTndNodeListPtr_t);
      SmlDmTndNodeListPtr_t p_tnd_nodelist = smlAllocDmTndNodeList();
      if ( p_tnd_info == NULL)
      {
         return SYNCML_DM_DEVICE_FULL;
      }
      if ( NULL == p_tnd_info->nodelist )
      {
         p_tnd_info->nodelist = p_tnd_nodelist;
      }
      else
      {
         last->next = p_tnd_nodelist;
      }
      last = p_tnd_nodelist;
      data_length += data_size;
   
      // Append Node tag
      data_size = sizeof(SmlDmTndNodePtr_t);
      SmlDmTndNodePtr_t p_tnd_node = smlAllocDmTndNode();
      if ( p_tnd_node == NULL)
      {
         return SYNCML_DM_DEVICE_FULL;
      }
      p_tnd_nodelist->node = p_tnd_node;
      data_length += data_size;

      // Append NodeName tag
      data_size = oAttr.GetName().length();
      BuildPcDataWAllocMem (&p_tnd_node->nodename, data_size,  (UINT8*)oAttr.GetName().c_str());
      data_length += data_size;

      // Append Path tag
      tmpNodeUri = "";
      if ( (int)strlen(pTmpTargetUri) > targetUri.length() +
                                    oAttr.GetName().length() + 1)
      {
         tmpNodeUri.assign(pTmpTargetUri + targetUri.length() + 1, 
                          strlen(pTmpTargetUri) - 
                          targetUri.length() - 
                          oAttr.GetName().length() - 2 );
      }
      if ( tmpNodeUri.length() > 0 )
      {
         data_size = tmpNodeUri.length();
         BuildPcDataWAllocMem (&p_tnd_node->path, data_size,  (UINT8*)tmpNodeUri.c_str());
         data_length += data_size;
      }

      // Append RTProperties start tag
      data_size = sizeof(SmlDmTndRTPropsPtr_t);
      SmlDmTndRTPropsPtr_t p_tnd_node_rtprops = smlAllocDmTndRTProps();
      if ( p_tnd_node_rtprops == NULL)
      {
         return SYNCML_DM_DEVICE_FULL;
      }
      p_tnd_node->rtprops = p_tnd_node_rtprops;
      data_length += data_size;

      // Append ACL Property 
      if ( listACL )
      {
         data_size = oAttr.GetAcl().toString().length();
         BuildPcDataWAllocMem (&p_tnd_node_rtprops->acl, data_size, (UINT8*)oAttr.GetAcl().toString().c_str());
         data_length += data_size;
      }

      // Append Format Property 
      if ( listFormat )
      {
         data_size = sizeof(SmlDmTndFormatPtr_t);
         SmlDmTndFormatPtr_t p_tnd_node_format = smlAllocDmTndFormat();
         if ( p_tnd_node_format == NULL)
         {
            return SYNCML_DM_DEVICE_FULL;
         }
         p_tnd_node_rtprops->format = p_tnd_node_format;
         data_length += data_size;

         data_size = oAttr.GetFormat().length();
         BuildPcDataWAllocMem (&p_tnd_node_rtprops->format->value, data_size, (UINT8*)oAttr.GetFormat().c_str());
         data_length += data_size;
      }

      // Append Name Property 
      if ( listName )
      {
         data_size = oAttr.GetName().length();
         BuildPcDataWAllocMem (&p_tnd_node_rtprops->name, data_size, (UINT8*)oAttr.GetName().c_str());
         data_length += data_size;
      }

      // Append Size Property 
      if ( listSize )
      {
         char pTmpStr[UINT32_TYPE_STR_SIZE_10+1]; 
         DmSprintf(pTmpStr, "%d", oAttr.GetSize());
         data_size = DmStrlen(pTmpStr);
         BuildPcDataWAllocMem (&p_tnd_node_rtprops->size, data_size, (UINT8*)pTmpStr);
         data_length += data_size;
      }

      // Append Title Property 
      if ( listTitle )
      {
         data_size = oAttr.GetTitle().length();
         BuildPcDataWAllocMem (&p_tnd_node_rtprops->title, data_size, (UINT8*)oAttr.GetTitle().c_str());
         data_length += data_size;
      }

      // Append TStamp Property 
      if ( listTStamp )
      {
         tmpStr = "";
         if ( oAttr.GetTimestamp() != 0 ) 
         {
            // convert msec to sec
            time_t timestamp = (time_t)(oAttr.GetTimestamp()/1000L);
            tmpStr = (CPCHAR)ctime(&timestamp);
            tmpStr.SetAt(tmpStr.length()-1, '\0');
         }
         data_size = tmpStr.length();
         BuildPcDataWAllocMem (&p_tnd_node_rtprops->tstamp, data_size, (UINT8*)tmpStr.c_str());
         data_length += data_size;
      }

      // Append Type Property 
      data_size = sizeof(SmlDmTndTypePtr_t);
      SmlDmTndTypePtr_t p_tnd_node_type = smlAllocDmTndType();
      if ( p_tnd_node_type == NULL)
      {
         return SYNCML_DM_DEVICE_FULL;
      }
      p_tnd_node->rtprops->type = p_tnd_node_type;
      data_length += data_size;

      data_size = oAttr.GetType().length();
      BuildPcDataWAllocMem (&p_tnd_node_rtprops->type->mime, data_size, (UINT8*)oAttr.GetType().c_str());
      data_length += data_size;

      // Append VerNo Property 
      if ( listVerNo )
      {
         char pTmpStr[UINT32_TYPE_STR_SIZE_10+1]; 
         DmSprintf(pTmpStr, "%d", oAttr.GetVersion());
         data_size = DmStrlen(pTmpStr);
         BuildPcDataWAllocMem (&p_tnd_node_rtprops->verno, data_size, (UINT8*)pTmpStr);
         data_length += data_size;
      }

      // Append Value Tag 
      if ( listValue )
      {
         data_size = DmStrlen(pTmpGetData->getCharData());
         BuildPcDataWAllocMem (&p_tnd_node->value, data_size, (UINT8*)pTmpGetData->getCharData());
         data_length += data_size;
      }

      if ( pTmpGetData )
      {
          delete pTmpGetData;
          pTmpGetData = NULL;
      }

      nRes = dmTreeObj.GetListNextItem(oTmpGetStructData);
      if ( nRes != SYNCML_DM_SUCCESS || oTmpGetStructData._pbURI == p_target_uri )
      {
         break;
      }
      pTmpGetData = oTmpGetStructData.psRetData;
      oTmpGetStructData.psRetData = NULL;
      pTmpTargetUri = oTmpGetStructData._pbURI;
   }

   if ( pTmpGetData )
   {
      delete pTmpGetData;
      pTmpGetData = NULL;
   }

   if ( nRes == SYNCML_DM_SUCCESS )
   {
      p_get_ret_data->m_nFormat = SYNCML_DM_FORMAT_XML;
      p_get_ret_data->m_oMimeType.clear();
      p_get_ret_data->m_oMimeType.assign( pDmMgmtSessionObj->IsWBXMLEncoding() ? SYNCML_CONTENT_TYPE_DM_TNDS_WBXML: SYNCML_CONTENT_TYPE_DM_TNDS_XML);
      p_get_ret_data->m_oData.clear();

      pPcDataPtr->content = p_tnd_info; 
      pPcDataPtr->length = data_length; 
      pPcDataPtr->contentType = SML_PCDATA_EXTENSION; 
      pPcDataPtr->extension = SML_EXT_DMTND; 
   }

   return SYNCML_DM_SUCCESS;
}
#endif //TNDS_SUPPORT

/*
 Allocates and fill in memory structure for result
*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::AllocateResult(SmlResultsPtr_t & p_results,
                                        CPCHAR p_target_uri,
                                        CPCHAR p_CmdIdRef,
                                        DMGetData *p_get_ret_data,
                                        BOOLEAN is_ThisGetStructResult,
                                        BOOLEAN isFirstGetStruct,
                                        BOOLEAN isThisGetPropResult,
                                        CPCHAR szMsgID,
                                        SmlPcdataPtr_t p_data )
{
    DMString strTargetUri(p_target_uri);

    if ( !strTargetUri.Encode() ) 
        return SYNCML_DM_DEVICE_FULL;

    p_results = smlAllocResults();
    if ( !p_results ) 
        return SYNCML_DM_DEVICE_FULL;

    SmlItemListPtr_t p_results_list_item = p_results->itemList;
    SmlItemPtr_t p_results_item = p_results_list_item->item;

    UINT8 data_size_str[UINT32_TYPE_STR_SIZE_10]; 
    UINT8 *pEncData = NULL;

    p_results_item->source = smlAllocSource();
    if ( p_results_item->source == NULL )
    {
        smlFreeResults(p_results); 
        p_results = NULL;
        return (SYNCML_DM_FAIL);  
    }

    BuildPcData( p_results_item->source->locURI, 
                 SML_PCDATA_STRING,
                 SML_EXT_UNDEFINED, 
                 strTargetUri.length(), 
                 (UINT8*)strTargetUri.c_str());

    /* Set receiving package command id reference */
    BuildPcData(p_results->cmdRef, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                DmStrlen((char *)p_CmdIdRef),
                (UINT8*)p_CmdIdRef);

    /* Call BuildPcDataWAllocMem to allocate the memory for p_results->msgRef, also construct the
    data. */
    CPCHAR sMsg = szMsgID ? szMsgID : (CPCHAR)pMessageIdOfServer;
    BuildPcDataWAllocMem(&p_results->msgRef,DmStrlen(sMsg),(UINT8*)sMsg);
   
    /* If p_get_ret_data is NULL, we don't construct further <RESULT> */
    if (p_get_ret_data == NULL)
        return SYNCML_DM_SUCCESS;
    

    INT32 totalSize = p_get_ret_data->m_oData.getSize(); 
    // special case for binary data
    if ( !is_ThisGetStructResult && 
		p_get_ret_data->m_nFormat == SYNCML_DM_FORMAT_BIN && 
		!p_get_ret_data->IsESN())
    {
        if ( totalSize  > 0 )
        {
            UINT32 encLen = base64GetSize((BufferSize_t)p_get_ret_data->m_oData.getSize());
            pEncData = (UINT8 *)DmAllocMem(encLen+1);
            if (pEncData == NULL)
            {
                smlFreeResults(p_results); p_results = NULL;
                return SYNCML_DM_DEVICE_FULL;   
            }
            
            memset(pEncData, 0, encLen+1);
               
            UINT32 offset = 0;
            UINT32 dataSize = p_get_ret_data->m_oData.getSize();
            totalSize =  base64Encode (pEncData, encLen,
                                            p_get_ret_data->m_oData.getBuffer(),
                                            (BufferSize_t *)&dataSize,
                                            (BufferSize_t *)&offset, 0, NULL); 
        }
        p_get_ret_data->m_nFormat = SYNCML_DM_FORMAT_B64;
    }        
	 /* Convert the dwRetDataSize to a string */
    if(p_get_ret_data->IsESN())
    {	 
		 if ( !is_ThisGetStructResult && 
			 p_get_ret_data->m_nFormat == SYNCML_DM_FORMAT_BIN && 
			 p_get_ret_data->IsESN())
		 {
			 if(p_get_ret_data->m_TotalSize != 0)
				 totalSize = base64GetSize((BufferSize_t)p_get_ret_data->m_TotalSize);
			 p_get_ret_data->m_nFormat = SYNCML_DM_FORMAT_B64;
		 }
		else
			 totalSize = p_get_ret_data->m_TotalSize;
   }
   DmSprintf((char *)data_size_str, "%d", p_get_ret_data->m_oData.getSize());

   // Is data size too large ?
	if(IsLargerThanMaxObjSize(totalSize, p_get_ret_data->IsESN(), TRUE))
	{        
          if (pEncData != NULL) 
          {
             DmFreeMem(pEncData);
          }

 		smlFreeResults(p_results); p_results = NULL;
		 return SYNCML_DM_REQUEST_ENTITY_TOO_LARGE;
	}

    if (!isThisGetPropResult)
    {
        p_results_item->meta = smlAllocPcdata();
        if (p_results_item->meta == NULL) 
        {
          if (pEncData != NULL) 
          {
             DmFreeMem(pEncData);
          }
          smlFreeResults(p_results); p_results = NULL;
           return SYNCML_DM_DEVICE_FULL;   
        }

        DMString strFormat;
        SYNCML_DM_RET_STATUS_T res = DMTree::ConvertFormat( p_get_ret_data->m_nFormat, strFormat);

        if (res != SYNCML_DM_SUCCESS) 
        {
          if (pEncData != NULL) 
          {
             DmFreeMem(pEncData);
          }
          smlFreeResults(p_results); p_results = NULL;
            return res; 
        }

        if ((p_get_ret_data->m_oMimeType.getSize() == 0) || isFirstGetStruct)
        {
            BuildMetaInfo(p_results_item->meta,
                (UINT8 *)strFormat.GetBuffer(),
                NULL,NULL, NULL, NULL, NULL, NULL, NULL);
        }
        else 
        {
            if (p_get_ret_data->m_nFormat == SYNCML_DM_FORMAT_B64)
            { 
                BuildMetaInfo(p_results_item->meta,
                             (UINT8 *)strFormat.GetBuffer(),
                              NULL,NULL,
                              data_size_str,
                              NULL, NULL, NULL, NULL);       
            }
            else 
            {
                if (p_get_ret_data->m_nFormat == SYNCML_DM_FORMAT_CHR )
                {
                    BuildMetaInfo(p_results_item->meta,
                                 NULL,NULL,NULL,data_size_str,
                                 NULL, NULL, NULL, NULL);
                } 
                else
                {
                    BuildMetaInfo(p_results_item->meta,
                                  (UINT8 *)strFormat.GetBuffer(),
                                  (UINT8*)p_get_ret_data->getType(),
                                  NULL,
                                  data_size_str,
                                  NULL, NULL, NULL, NULL);
                }
            }
        }
    }

    /* Set the p_results_item->data */
    /* When data is from GET command on Struct, no need to construct <Data> element. */
    if (!is_ThisGetStructResult)
    {
       /* Client construct <Data> element no matter there are data from GET command or not. */
        p_results_item->data = smlAllocPcdata();
	  if(p_results_item->data == NULL)
     {
          if (pEncData != NULL) 
          {
             DmFreeMem(pEncData);
          }
	  		 smlFreeResults(p_results); p_results = NULL;
			 return SYNCML_DM_DEVICE_FULL;   
	  }
#ifdef LOB_SUPPORT
	if(pDmMgmtSessionObj->IsLargeObjectSupported() && p_get_ret_data->IsESN())
	{
	 	p_results_item->data->length = totalSize;
		p_results_item->flags |= SmlESNData_f;
		// Need to encode binary data
		if(p_get_ret_data->m_nFormat == SYNCML_DM_FORMAT_B64)
			p_results_item->flags |= SmlESNBinary_f;
	}
	else
#endif
        {
	    if(p_get_ret_data->IsESN())
       {
          if (pEncData != NULL) 
          {
             DmFreeMem(pEncData);
          }
          smlFreeResults(p_results); p_results = NULL;
          return SYNCML_DM_FEATURE_NOT_SUPPORTED;   
	    }
          UINT8 *pData = (pEncData == NULL) ? p_get_ret_data->m_oData.getBuffer() : pEncData;
  
#ifdef TNDS_SUPPORT
        if ( NULL != p_data )
        {
           p_results_item->data->content = p_data->content; 
           p_results_item->data->length = p_data->length; 
           p_results_item->data->contentType = p_data->contentType;
           p_results_item->data->extension = p_data->extension;
        }
        else
#endif
        {
          BuildPcData(p_results_item->data, 
                    SML_PCDATA_STRING,
                    SML_EXT_UNDEFINED,
                    totalSize,
                    pData);
        
        }
	 }
    }
  
    if (pEncData != NULL) 
         DmFreeMem(pEncData);

#ifdef LOB_SUPPORT
	if(pDmMgmtSessionObj->IsLargeObjectSupported())
	{	
		return SYNCML_DM_SUCCESS;
	}
#endif
    // check the size
    MemSize_t freeSpace = 0, initialBufferSize = smlGetFreeBuffer(sendInstanceId);
    SML_API Ret_t smlErr = SML_ERR_OK;
    smlStartEvaluation(sendInstanceId);
    smlErr = smlResultsCmd(sendInstanceId, p_results);
    smlEndEvaluation(sendInstanceId, &freeSpace);

    if ( smlErr != SML_ERR_OK )
    {
        smlFreeResults(p_results); p_results = NULL;
        return (SYNCML_DM_FAIL);
    }

    /* DM_MSG_OVERHEAD is defined as the size of the syncHdr and the status
       to Alert (DM_NEXT_MESSAGE).  The size should be 531 bytes for XML and  ? for WBXML */
    
    MemSize_t nRequired = initialBufferSize - freeSpace;
      
    if (nRequired > (g_iDMWorkspaceSize - DM_MSG_OVERHEAD) ) 
    {
        smlFreeResults(p_results); p_results = NULL;
        return SYNCML_DM_RESULTS_TOO_LARGE;
    }
    
    return SYNCML_DM_SUCCESS;
}

/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::BuildResultsCommand

DESCRIPTION     : This function will be called by HandleGetCommand to add the GET results to the DM
                  package.

                  This method calls the SyncML Toolkit function smlResultsCommand() to add results
                  into the SyncBody for the DM message.
ARGUMENT PASSED : p_CmdIdRef
                  p_TargetUri
                  p_results
OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_SUCCESS or SYNCML_DM_FAIL
IMPORTANT NOTES : 


==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SYNCML_DM_BuildPackage::BuildResultsCommand( SmlResultsPtr_t p_results )
{
    UINT8     command_id_str[UINT16_TYPE_STR_SIZE_5];
    
    /* Convert the commandId to a string */
    DmSprintf((char *)command_id_str, "%d", commandId++);
    BuildPcData(p_results->cmdID, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                DmStrlen((char *)command_id_str),
                command_id_str);

    SML_API Ret_t smlErr = SML_ERR_OK;
#ifdef LOB_SUPPORT
    smlErr = smlResultsCmd(sendInstanceId, p_results);
    if ( smlErr != SML_ERR_OK )
    {
	   return SYNCML_DM_RESULTS_TOO_LARGE;
    }

#else
    MemSize_t freeSpace;
    MemSize_t initialBufferSize;
    initialBufferSize = smlGetFreeBuffer(sendInstanceId);
    
    smlStartEvaluation(sendInstanceId);
    smlErr = smlResultsCmd(sendInstanceId, p_results);
    smlEndEvaluation(sendInstanceId, &freeSpace);
    if ( smlErr != SML_ERR_OK )
    {
        return (SYNCML_DM_FAIL);
    }

    /* Note that freeSpace accounts for the EndSyncmlDoc overhead as well.*/
    if (freeSpace > 0) {
        /* Call the toolkit to construct RESULTS for GET command. */
        smlErr = smlResultsCmd(sendInstanceId, p_results);
    } else 
        return SYNCML_DM_RESULTS_TOO_LARGE;
#endif    
    /* We cannot free any memory here since it may be used later in multiple messages.*/
    if ( smlErr != SML_ERR_OK )
    {
        return (SYNCML_DM_FAIL);
    }

    return (SYNCML_DM_SUCCESS);
}


/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::EndSyncmlDoc

DESCRIPTION     : This function will be called by BuildPackageOne and HandleStartMessage to add the
                  last element to the DM package.

                  This method calls the SyncML Toolkit function smlEndMessage() to add a Final element 
                  into the SyncBody for the DM message.
ARGUMENT PASSED : final
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SYNCML_DM_BuildPackage::EndSyncmlDoc(Boolean_t  final)
{
    Ret_t sml_ret_stat;
    SYNCML_DM_RET_STATUS_T ret_stat = SYNCML_DM_SUCCESS;

    /* This ends the SyncML document it has been assembled */
    /* SmlFinal_f says this is the last message in the SyncML package */
#ifdef LOB_SUPPORT
	if (m_bProcessingLargeObject && largeObjectCmd!= DM_COMMAND_RESULTS)
	{
		   ret_stat = BuildAlertCommand(DM_ALERT_NEXT_MESSAGE, NULL, NULL);
		   if (ret_stat != SYNCML_DM_SUCCESS)
			   return ret_stat;
		// No <Final> statement
		   final = FALSE;
	}
#endif

    sml_ret_stat = smlEndMessage(sendInstanceId, final);
    if (sml_ret_stat != SML_ERR_OK)
    {
        ret_stat = SYNCML_DM_FAIL;
    }
    return (ret_stat);
}


/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::getDirection

DESCRIPTION     : This function returns session direction in unsigned short.
ARGUMENT PASSED :
OUTPUT PARAMETER:
RETURN VALUE    :  session direction
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_SESSION_DIRECTION_T 
SYNCML_DM_BuildPackage::getDirection() const
{
   return sessionDirection;
}


/*==================================================================================================
FUNCTION        : SYNCML_DM_BuildPackage::BuildPackageOne

DESCRIPTION     : This function calls SYNCML_DM_BuildPackage class functions to build up the package one.
ARGUMENT PASSED : session_Direction
                  p_ParsedPk0
OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_SUCCESS or SYNCML_DM_FAIL
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SYNCML_DM_BuildPackage::BuildPackageOne(CPCHAR pServerID, DmtSessionProp * pSessionProp)
{
    SYNCML_DM_RET_STATUS_T ret_stat;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    SmlSyncHdrPtr_t        p_hdr_content;
    char                   session_id_str[UINT16_TYPE_STR_SIZE_5];
    DMGetData portNbrData;
    DMGetData devID;
    char  *p_server_uri = NULL;
    CPCHAR parent_name = NULL;
    DMString  strServerURI;
    CPCHAR devIMEI = NULL;
    bool sessionid_check = false;
    UINT32    finalUriLength;
    DMClientServerCreds *pClientServerCreds;
    
    /* Allocate the memory for pSyncHdr */

    sessionDirection = pSessionProp->getDirection();
    p_hdr_content = smlAllocSyncHdr();
    if ( p_hdr_content == NULL)
    {
       return (SYNCML_DM_DEVICE_FULL);
    }

   	 	/* Fill in message id as 1 into the sync header */
    	BuildPcData(p_hdr_content->msgID, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                DmStrlen((char *)DEFAULT_MESSAGE_ID),
                (UINT8 *)DEFAULT_MESSAGE_ID);

        sessionid_check = IsSessionId();
    if(sessionid_check == true)
    {
    	/* Fill in the SessionId data which come from p_ParsedPk0, must be in upper case hex */
         DmSprintf(session_id_str, "%X", pSessionProp->getSessionID());
    }
    else
    {
     /* Fill in the SessionId data which come from p_ParsedPk0, must be in decimal */
     DmSprintf(session_id_str, "%d", pSessionProp->getSessionID());
    } 
    
  	BuildPcData(p_hdr_content->sessionID,
                SML_PCDATA_STRING,SML_EXT_UNDEFINED,
                DmStrlen(session_id_str),
                (UINT8*)session_id_str);

    /* We can use the DMAccNodeName that was stored in ConnectServer().*/
    pClientServerCreds = pDmMgmtSessionObj->GetClientServerCreds();
    parent_name = (CPCHAR)pClientServerCreds->pDMAccNodeName;
    if (parent_name == NULL)
    {
        smlFreeSyncHdr(p_hdr_content);
        return (SYNCML_DM_FAIL);
    }

    /* Call the local method to retrieve the value of "./SyncML/DMAcc/<parent_name>/Addr" */
    DMGetData oDataAddrType, oAddr;

    ret_stat = dmTreeObj.GetDefAccountAddrInfo( parent_name,
                                                                          oAddr,
                                                                          oDataAddrType, 
                                                                          portNbrData );
   
    if ( (dm_stat != SYNCML_DM_SUCCESS) || (oAddr.m_oData.getSize() == 0) )
    {
       smlFreeSyncHdr(p_hdr_content);
       return (SYNCML_DM_FAIL);
    }

    /* If port number is NULL, means its value is defaulted as 80, we don't need to do anything. 
       Otherwise, insert port number into the server address */
    if ( portNbrData.m_oData.getSize() != 0 )
    {
        char * pUriBeg;
        char * pUriMid;

        /* Locate the position of '://' */
        pUriBeg = (char*)DmStrstr(oAddr.getCharData(), "://");
        if (pUriBeg == NULL)
        {
            pUriBeg = (char*)oAddr.getCharData();
        }
        else
        {
            pUriBeg += 3;
        }
        char* pUriBeg2 = NULL;
        char* pPortNumber = NULL;

        /* Locate the position of ':' */
        pPortNumber = DmStrchr(pUriBeg, ':');

        /* Locate the position of '/' */
        pUriMid = DmStrchr(pUriBeg, '/');
        if (pUriMid == NULL)
        {
            pUriBeg2 = (char*)DmAllocMem(DmStrlen(pUriBeg)+2); 
            if ( pUriBeg2 == NULL )
            {
               smlFreeSyncHdr(p_hdr_content);
               return (SYNCML_DM_DEVICE_FULL);
            }

            DmStrcpy(pUriBeg2,pUriBeg);
          
            /* Insert '/' at the end of URL */
            DmStrcat(pUriBeg2, "/");
            DmFreeMem(pUriBeg);
          
            pUriBeg = pUriBeg2;
            pUriMid = DmStrchr(pUriBeg, '/');
            if ( pUriMid == NULL )
            {
               if ( pUriBeg2 != NULL ) 
               {
                  DmFreeMem(pUriBeg2); 
               }
               smlFreeSyncHdr(p_hdr_content);
               return (SYNCML_DM_FAIL);
            }
        }
        /* Calculate the length of the final server address */
        finalUriLength = oAddr.m_oData.getSize() + portNbrData.m_oData.getSize() + 2;

        /* Allocate memory for the new URL, copy addr and portNbr to new uri */
        p_server_uri = (char*)DmAllocMem(finalUriLength);
        if ( p_server_uri == NULL )
        {
            if ( pUriBeg2 != NULL ) 
            {
               DmFreeMem(pUriBeg2); 
            }
            smlFreeSyncHdr(p_hdr_content);
            return (SYNCML_DM_DEVICE_FULL);
        }
        memset(p_server_uri, 0, finalUriLength);
        DmStrncpy(p_server_uri, oAddr.getCharData(),pUriMid - oAddr.getCharData());
        
        if(portNbrData.m_oData.getSize() == 0 || pPortNumber != NULL)
           DmStrcat(p_server_uri,pUriMid);
        else
        {
           DmStrcat((char*)p_server_uri, ":");
           DmStrcat((char*)p_server_uri, portNbrData.getCharData());
           DmStrcat((char*)p_server_uri, pUriMid);
        }   

        if ( pUriBeg2 != NULL ) 
        {
           DmFreeMem(pUriBeg2); 
        }
    }
    else
    {
        oAddr.m_oData.copyTo(&p_server_uri);
        if ( p_server_uri == NULL )
        {
           smlFreeSyncHdr(p_hdr_content);
           return (SYNCML_DM_DEVICE_FULL);
        }
    }


    /* Call MgmtSession object to set server URI here to reduce code for transport module, since
       we have constructed server uri with server address and port number */ 
    strServerURI = (CPCHAR)p_server_uri;
    pDmMgmtSessionObj->SetURI(strServerURI.c_str());
    if(strServerURI.Encode()== FALSE)
    { 
        if ( p_server_uri != NULL )
           DmFreeMem(p_server_uri);
        smlFreeSyncHdr(p_hdr_content);
        return (SYNCML_DM_FAIL);
    }

    if(p_hdr_content->source == NULL)
    {   
      if ( p_server_uri != NULL )
           DmFreeMem(p_server_uri);
       smlFreeSyncHdr(p_hdr_content);
	return (SYNCML_DM_FAIL);
    }
   
    /* Fill in the server address information into pSyncHdr structure target->locURI element.*/
    BuildPcData(p_hdr_content->source->locURI, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                strServerURI.length(),
               (UINT8 *) strServerURI.c_str());

    /* Free the memory of p_server_uri */
    DmFreeMem(p_server_uri);

    /* Fill in the server id information into pSyncHdr structure target->locName element. */
    if (pServerID != NULL)
    {
        BuildPcDataWAllocMem(&p_hdr_content->source->locName,DmStrlen(pServerID),(UINT8*)pServerID);
    }

    /* While we are retrieving things from the DM Tree, we'll get the rest of the Security info.*/
    pClientServerCreds->pServerId = pServerID;
    if ( pClientServerCreds->pServerId != pServerID )
    {
        XPL_LOG_DM_SESS_Error(("SYNCML_DM_BuildPackage::BuildPackageOne : unable allocate memory"));
        smlFreeSyncHdr(p_hdr_content);
	return SYNCML_DM_DEVICE_FULL;
    }
    /* Call the local method to retrieve the value of "./DevInfo/DevId" */
    dm_stat = dmTreeObj.Get(DM_DEV_INFO_DEVID_URI, devID,SYNCML_DM_REQUEST_TYPE_INTERNAL);
  
    if (dm_stat != SYNCML_DM_SUCCESS) 
    {
        smlFreeSyncHdr(p_hdr_content);
	return (SYNCML_DM_FAIL);
    }

    if (devID.m_oData.getSize() != 0)
        devIMEI = GetIMEINumber(devID.getCharData());
    
    /* Call the local method to retrieve the remaining pieces of Security Info.*/
    
    dm_stat = GetRemainingSecInfo(parent_name, pClientServerCreds, devIMEI);
    if ( dm_stat != SYNCML_DM_SUCCESS )
    {
        smlFreeSyncHdr(p_hdr_content);
	return (SYNCML_DM_FAIL);
    }
    
    BuildPcDataWAllocMem(&p_hdr_content->target->locName,
                         DmStrlen((CPCHAR)(pClientServerCreds->pClientUserName)),
                         (UINT8*)pClientServerCreds->pClientUserName.c_str());

    /* Fill the client dev id information into pSyncHdr structure source->locURI element. */
    if (devID.m_oData.getSize() != 0)
    {
        BuildPcData(p_hdr_content->target->locURI, SML_PCDATA_STRING,
                    SML_EXT_UNDEFINED,
                    devID.m_oData.getSize(),
                    devID.m_oData.getBuffer());
    }
    
 
    /* Call BuildSyncHdr to construct the <SyncHdr> */
    ret_stat = BuildStartSyncHdr(p_hdr_content,TRUE);
    smlFreeSyncHdr(p_hdr_content);
    if (ret_stat != SYNCML_DM_SUCCESS)
    {
	return(ret_stat);
    }
    ret_stat = BuildFinishSyncHdr(pClientServerCreds->AuthPrefCredType);

    if (ret_stat != SYNCML_DM_SUCCESS)
    {
	return(ret_stat);
    }

    /* Build up the Alert command to indicate server or client initiate the session */
    ret_stat = BuildAlertCommand(sessionDirection, NULL, NULL);
    if (ret_stat != SYNCML_DM_SUCCESS)
    {
	return(ret_stat);
    }

    /* Build the DevInfo data in REPLACE command */
    ret_stat = BuildReplaceCommand();
    if (ret_stat != SYNCML_DM_SUCCESS)
    {
       return(ret_stat);
    }

    //DMFirmAlertVector aFirmAlert;
    //pSessionProp->getFirmAlerts(aFirmAlert);
    firmAlertVec.clear();
    pSessionProp->getFirmAlerts(firmAlertVec);
    //if ( aFirmAlert.size() > 0 )
    //{
    ret_stat = BuildAlert1226Command();  
    if (ret_stat != SYNCML_DM_SUCCESS)
    {
       return(ret_stat);
    }
    //}

    /* End the SyncML document */
    ret_stat = EndSyncmlDoc(SmlFinal_f);
    return (ret_stat);
}



/*==================================================================================================
Function:    SYNCML_DM_BuildPackage::GetRemainingSecInfo (private)

Description: This method is called to retrieve the remaining pieces of Security Info needed for
             creating credentials during this session.  This method will retrieve:
                - ServerPW
                - ServerNonce
                - user name (client)
                - ClientPW
                - ClientNonce
ARGUMENT PASSED :
OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_SUCCESS or SYNCML_DM_FAIL
IMPORTANT NOTES : This method assumes the DM Tree is locked and the management session is in
                  progress.
             
==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SYNCML_DM_BuildPackage::GetRemainingSecInfo (CPCHAR pParentName,
                                             DMClientServerCreds *pClientServerCreds,
                                             CPCHAR devIMEI)
{
    DMGetData serverData;
    DMGetData clientData;

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    dm_stat = pClientServerCreds->LoadInitialValues();

    if ( dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;

    pDmMgmtSessionObj->SetSecState(DM_CLIENT_NO_SERVER_NO_AUTH);

    return (dm_stat);
}

/*===============================================================================
FUNCTION        : CreateFactoryBootStrapUserName 

DESCRIPTION     : This function will create factory boot strap's user name

OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_STATUS_T
=================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::CreateFactoryBootStrapUserName( SYNCML_DM_FACTORY_BOOTSTRAP_T bootStrapCode,
                                                     const DMString&               server_id,
                                                     const DMString&               devIMEI,
                                                     DMString&                     user_name )
{
  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

  switch ( bootStrapCode ) 
  {
    case SYNCML_DM_FACTORY_BOOTSTRAP_FCS11384:
    {
      dm_stat = BuildFactoryBootstrapCredData(user_name, 
                                              devIMEI + ":" + server_id,
                                              DM_NAME);
      break;
    }
    case SYNCML_DM_FACTORY_BOOTSTRAP_FCS14345:
    {
      // IMEI becomes the UserName
      user_name = devIMEI;
      break;
    }
    default:
    {
      break;
    }
  }
  
  if( ( SYNCML_DM_SUCCESS != dm_stat ) ||
      ( 0 == user_name.length() ) )
  {
    dm_stat = SYNCML_DM_DEVICE_FULL;
  }

  return dm_stat;
}



/*===============================================================================
FUNCTION        : CreateFactoryBootStrapPW 

DESCRIPTION     : This function will create factory boot strap's server or client pass word

OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_STATUS_T


=================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::CreateFactoryBootStrapServerPW( SYNCML_DM_FACTORY_BOOTSTRAP_T bootStrapCode,
                                                     const DMString&       server_id,
                                                     const DMString&       abIMEI,
                                                     DMString&             server_password )
{
  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

  switch ( bootStrapCode ) 
  {
    case SYNCML_DM_FACTORY_BOOTSTRAP_FCS11384:
    {
      dm_stat = BuildFactoryBootstrapCredData(server_password, 
                                              server_id, 
                                              abIMEI);

      if ( dm_stat != SYNCML_DM_SUCCESS )
          return SYNCML_DM_DEVICE_FULL;
      break;
    } 
    case SYNCML_DM_FACTORY_BOOTSTRAP_FCS14345:
    {
      
      GeneratePassword gp;       

      gp.setIMEI(abIMEI);
      gp.setServerId(server_id);

      char* srvpw = gp.generateServerPassword();
      server_password = srvpw;
      DmFreeMem(srvpw);
      break;
    }
    default:
    {
      return dm_stat;
    }
  }

  return dm_stat;
}


/*===============================================================================
FUNCTION        : CreateFactoryBootStrapPW 

DESCRIPTION     : This function will create factory boot strap's server or client pass word

OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_STATUS_T


=================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::CreateFactoryBootStrapClientPW( SYNCML_DM_FACTORY_BOOTSTRAP_T bootStrapCode,
                                                    const DMString&       server_id,
                                                    const DMString&       abIMEI,
                                                    DMString&             client_password )
{
  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
  
  switch (bootStrapCode) 
  {
    case SYNCML_DM_FACTORY_BOOTSTRAP_FCS11384:
    {
      dm_stat = BuildFactoryBootstrapCredData(  client_password, 
                                                abIMEI, 
                                                server_id );

      if ( dm_stat != SYNCML_DM_SUCCESS )
      {
        dm_stat = SYNCML_DM_DEVICE_FULL;
      }

      break;
    }
    case SYNCML_DM_FACTORY_BOOTSTRAP_FCS14345:
    {
      GeneratePassword gp;

      gp.setIMEI(abIMEI);
      gp.setServerId( server_id );

      char* clnpw = gp.generateClientPassword();
      client_password = clnpw;
      DmFreeMem(clnpw);
      break;
    }
    default:
    {
      break;
    }
  }

  return dm_stat;
}


/*=============================================================================
FUNCTION:BuildFactoryBootstrapCredData
DESCRIPTION:
   This function builds the B64 encoded MD5 credential information.
   The credential string is credential data:IMEI number . That string is then b64 encoded.

ARGUMENTS PASSED:
   
REFERENCE ARGUMENTS PASSED:
   char **pp_credential_data  - Output variable containing the encoded credential data
   const char *p_CredData - credential_data  .
   const char *p_IMEI - Phone IMEI number.
  

RETURN VALUE:
   N/A

PRE-CONDITIONS:

POST-CONDITIONS:
  
================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::BuildFactoryBootstrapCredData(DMString& pp_credential_data,
                                                             CPCHAR p_CredData,
                                                             CPCHAR p_CredData1 )
 {
    char *p_CredData_IMEI;
    MD5_CTX md5_context;
    char md5hash[SYNCML_DM_HASHLEN + 1];  /* Add 1 character for NULL */
    BufferSize_t offset = 0;
    BufferSize_t md5_hash_length = SYNCML_DM_HASHLEN;
    BufferSize_t total_length;
    
    memset(md5hash, '\0', SYNCML_DM_HASHLEN + 1);

    char* pCredData = pp_credential_data.AllocateBuffer(SYNCML_DM_BAS64_ENCODING_SIZE_IN_MD5 + 1);
    
    if ( pCredData == NULL ) 
    {
        XPL_LOG_DM_SESS_Error(("BuildFactoryBootstrapCredData : unable allocate memory"));
        return SYNCML_DM_DEVICE_FULL; 
    }    
    
    memset(pCredData, 0, SYNCML_DM_BAS64_ENCODING_SIZE_IN_MD5+1);
    /* Add space for the ":" and NULL character */
    total_length= DmStrlen(p_CredData) + DmStrlen(p_CredData1) + 2;
    p_CredData_IMEI = (char *)DmAllocMem(total_length);

    if ( p_CredData_IMEI == NULL ) 
    {
        XPL_LOG_DM_SESS_Error(("BuildFactoryBootstrapCredData : unable allocate memory"));
        return SYNCML_DM_DEVICE_FULL; 
    }
    
    memset(p_CredData_IMEI, '\0', total_length);
    DmStrcpy(p_CredData_IMEI, p_CredData);
    DmStrcat(p_CredData_IMEI, ":");
    DmStrcat(p_CredData_IMEI, p_CredData1);
    
    smlMD5Init(&md5_context);
    smlMD5Update(&md5_context, (unsigned char *)p_CredData_IMEI,DmStrlen(p_CredData_IMEI));
    smlMD5Final((unsigned char*)md5hash, &md5_context);
    md5hash[SYNCML_DM_HASHLEN] = 0;
    DmFreeMem(p_CredData_IMEI);
    
    base64Encode((UINT8*)pCredData, 
                 SYNCML_DM_BAS64_ENCODING_SIZE_IN_MD5+1,
                 (unsigned char *)md5hash, 
                 &md5_hash_length, 
                 &offset,
                 1, /* Encode as single block */
                 NULL); /* No incomplete bl*/
    return SYNCML_DM_SUCCESS;

 }

SYNCML_DM_FACTORY_BOOTSTRAP_T SYNCML_DM_BuildPackage::GetBootStrapCodeUserName(const DMString& user_name )
{
  if( user_name == DMACC_FACTORY_BOOTSTRAP_USERNAME_FCS11384 ) 
  {
    return SYNCML_DM_FACTORY_BOOTSTRAP_FCS11384;
  } 
  else if ( user_name == DMACC_FACTORY_BOOTSTRAP_USERNAME_FCS14345 ) 
  {
    return SYNCML_DM_FACTORY_BOOTSTRAP_FCS14345;
  }

  return SYNCML_DM_FACTORY_BOOTSTRAP_UNKNOWN;
}

SYNCML_DM_FACTORY_BOOTSTRAP_T SYNCML_DM_BuildPackage::GetBootStrapCodeClientPW(const DMString& password )
{
  if ( password == DMACC_FACTORY_BOOTSTRAP_CLIENTPW_FCS11384 ) 
  {
    return SYNCML_DM_FACTORY_BOOTSTRAP_FCS11384;
  } 
  else if ( password == DMACC_FACTORY_BOOTSTRAP_CLIENTPW_FCS14345 ) 
  {
    return SYNCML_DM_FACTORY_BOOTSTRAP_FCS14345;
  }

  return SYNCML_DM_FACTORY_BOOTSTRAP_UNKNOWN;
}

SYNCML_DM_FACTORY_BOOTSTRAP_T SYNCML_DM_BuildPackage::GetBootStrapCodeServerPW(const DMString& password )
{
  if ( password == DMACC_FACTORY_BOOTSTRAP_SERVERPW_FCS11384 ) 
  {
    return SYNCML_DM_FACTORY_BOOTSTRAP_FCS11384;
  } 
  else if ( password == DMACC_FACTORY_BOOTSTRAP_SERVERPW_FCS14345 ) 
  {
    return SYNCML_DM_FACTORY_BOOTSTRAP_FCS14345;
  }
  
  return SYNCML_DM_FACTORY_BOOTSTRAP_UNKNOWN;
}
  
/*=================================================================

===================================================================*/
CPCHAR 
SYNCML_DM_BuildPackage::GetIMEINumber(CPCHAR devID)
{
    char *retValue = (char*)DmStrchr(devID, ':');
    if( retValue == NULL )
        return devID;
    else
        return (++retValue);
}

/*==================================================================================================
FUNCTION        :DecodBase64Data

DESCRIPTION     : Handling of SYNCML_DM_FORMAT_B64
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::DecodBase64Data(SmlItemPtr_t &p_add_item, DMAddData & oAddData, BOOLEAN decodeB64)
{
  SYNCML_DM_RET_STATUS_T dm_stat =	  SYNCML_DM_SUCCESS;
  if (p_add_item->data != NULL) 
  {
	  // special-case handling of SYNCML_DM_FORMAT_B64
	  if (oAddData.m_nFormat == SYNCML_DM_FORMAT_B64 && decodeB64) 
	  {
		 UINT8	*decDataBuf;
		 UINT32 dataLen = p_add_item->data->length;;
		 UINT32 decDataLen;
		 UINT32 decMaxBufLen = dataLen*3/4+2;
	  
		 decDataBuf = (UINT8*)DmAllocMem(decMaxBufLen);
		 if(decDataBuf == NULL)
			 return SYNCML_DM_DEVICE_FULL;
		 decDataLen =  base64Decode(decDataBuf , decMaxBufLen, (DataBuffer_t) p_add_item->data->content,
								   (BufferSize_t *)&dataLen);
		 if (decDataLen == 0) 
		 {	 
			 DmFreeMem(decDataBuf);
			 return SYNCML_DM_BAD_REQUEST;
		 }
	  
		 oAddData.m_oData.assign(decDataBuf, decDataLen); 
	  
		 if ( oAddData.m_oData.getBuffer() == NULL )
		 {	
			 DmFreeMem(decDataBuf);
			 return SYNCML_DM_DEVICE_FULL;
		 }
		 DmFreeMem(decDataBuf);
		 oAddData.m_nFormat = SYNCML_DM_FORMAT_BIN;
	   }
	   else
	   {
		 if ( p_add_item->data->length )
		 {
			 oAddData.m_oData.assign((UINT8*)p_add_item->data->content, p_add_item->data->length);
			 if ( oAddData.m_oData.getBuffer() == NULL )
				dm_stat = SYNCML_DM_DEVICE_FULL;
		 }	  
	   }  
	   
   }
  return dm_stat;
}
/*==================================================================================================
FUNCTION        :IsLargerThanMaxObjSize

DESCRIPTION     : Is data size larger than MaxObjSize
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
BOOLEAN 
SYNCML_DM_BuildPackage::IsLargerThanMaxObjSize( int dataSize, BOOLEAN isESN, BOOLEAN isToServer)
{
	// If data size is larger then MaxObjSize 
	XPL_FS_SIZE_T maxSize = MaxObjectSize;

#ifdef LOB_SUPPORT
	if(!isToServer)
	{	if(isESN)
		{
			DMString m_strEsnDir;
	     dmTreeObj.GetWritableFileSystemFullPath( m_strEsnDir );
			 maxSize = XPL_FS_FreeDiskSpace(m_strEsnDir.c_str());
		}
		else
		{
            maxSize = MaxObjectSize = pDmMgmtSessionObj->GetDefaultMaxObjectSize();
		}
	}
#endif  

	// <MaxObjSize> is missing
	if(maxSize == 0)
	{
	  if(isESN)
		  maxSize = SYNCML_DM_MAX_OBJ_SIZE;
	  else
		  maxSize = MaxMessageSize * 2;
	}
	if(dataSize > (int)maxSize)
		return TRUE;
	else 
		return FALSE;
}
#ifdef LOB_SUPPORT
/*==================================================================================================
FUNCTION        :LargeObjectClear

DESCRIPTION     : Clears  large object related variables
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
void 
SYNCML_DM_BuildPackage::LargeObjectClear()
{
	m_bProcessingLargeObject = FALSE;
  	m_bESN = FALSE;
      m_bBinary = FALSE;
	if(m_pChunkData != NULL)
		delete m_pChunkData;

  	m_pChunkData = NULL;
  	m_ChunkOffset = 0; 

	largeObjectCmdSize = 0;
	largeObjectSize = 0;
	largeObjectBufferUsedSize = 0;
	largeObjectMsgRef = 0;
	largeObjectChunkSize = 0;
	largeObjectChunkOffset = 0;

	largeObjectBuffer.free();
	largeObjectCmdRef = NULL;
	largeObjectMsgRef = NULL;

	largeObjectTURI.free();
	largeObjectSURI.free();
	largeObjectType.free();
	largeObjectFormat = SYNCML_DM_FORMAT_INVALID;
	largeObjectCmd = DM_COMMAND_INVALID;
	if(largeObjFileHandle != NULL)
	{	largeObjFileHandle->deleteFile();
		delete largeObjFileHandle;
		largeObjFileHandle = NULL;
	}
	largeObjFileName = NULL;

}
/*==================================================================================================
FUNCTION        : AllocateChunkBuffer

DESCRIPTION     : Allocate chunk buffer
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES : 

==================================================================================================*/
DmtDataChunk * 
SYNCML_DM_BuildPackage::AllocateChunkBuffer()
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    DmtDataChunk *chunkData;  
      
    chunkData = new DmtDataChunk();
    if(chunkData == NULL)
        return NULL;
      
    dm_stat = chunkData->AllocateChunkBuffer();
    if (dm_stat != SYNCML_DM_SUCCESS)
    {
        delete chunkData;
        return NULL;
    }      
    return chunkData;
}

/*==================================================================================================
FUNCTION        : SetEngineChunkData

DESCRIPTION     : Set chunk data to engine
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES : 

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage:: SetChunkDataToEngine(DMAddData & oAddData, BOOLEAN isAllSet)
{
 SYNCML_DM_RET_STATUS_T retStatus =  SYNCML_DM_SUCCESS;
DMAddData oReplace;

oReplace.m_oURI.assign(oAddData.getURI());
if(oReplace.m_oURI.getBuffer() == NULL)
	return SYNCML_DM_DEVICE_FULL;

oReplace.m_bLastChunk = isAllSet;
oReplace.chunkData = m_pChunkData;
oReplace.m_chunkOffset = m_ChunkOffset;

retStatus =  dmTreeObj.Replace(oReplace,SYNCML_DM_REQUEST_TYPE_SERVER);
 return retStatus;
}
/*==================================================================================================
FUNCTION        : SetLastChunkDataFromFile

DESCRIPTION     : Set last chunk data from file
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES : 

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::SetLastChunkDataFromFile(DMAddData & oAddData,
													    DMBuffer  &lastData,
													    UINT8**szBuf,
													    int chunksize,
													    UINT8  *decDataBuf,
													    UINT32 decMaxBufLen,
													    UINT32 &lastChunkSize)
{
  SYNCML_DM_RET_STATUS_T retStatus =	SYNCML_DM_SUCCESS;
   int setLen = 0;
   BOOLEAN isAllSet = FALSE;
   int offset = 0;
   UINT32 dataLen = largeObjectSize;
   UINT32 decDataLen = 0;
   DMBuffer  remainData;

		// Close it first
   largeObjFileHandle->close();
   delete largeObjFileHandle;
  largeObjFileHandle = NULL;
  retStatus = OpenESNTempFile();
  if (retStatus != SYNCML_DM_SUCCESS)
		return retStatus;

  m_pChunkData->GetChunkData(szBuf);  // the chunk data is available  

 while (offset <  largeObjectBufferUsedSize && !isAllSet)
 {
			 setLen =  largeObjectBufferUsedSize - offset;
			if(setLen > chunksize)
				setLen = chunksize;
			else
				isAllSet = TRUE;
			if(largeObjFileHandle->seek(XPL_FS_SEEK_SET, offset) != SYNCML_DM_SUCCESS) 
					return SYNCML_DM_IO_FAILURE;

			if(largeObjFileHandle->read(*szBuf, setLen) != SYNCML_DM_SUCCESS) 
					return	SYNCML_DM_IO_FAILURE;

			// special-case handling of SYNCML_DM_FORMAT_B64
			if(decDataBuf != NULL)
			{
				dataLen = setLen;
				decDataLen =  base64Decode(decDataBuf , decMaxBufLen, (DataBuffer_t) *szBuf,
										  (BufferSize_t *)&dataLen);
				if (decDataLen == 0) 
					return   SYNCML_DM_BAD_REQUEST;
				if(dataLen != 0)
					remainData.assign((CPCHAR)& (*szBuf[0]), dataLen); 

				m_pChunkData->SetChunkData(decDataBuf, decDataLen);
				setLen -= dataLen;

			}
			else
				m_pChunkData->SetChunkData(NULL, setLen);

			retStatus = SetChunkDataToEngine(oAddData, FALSE);
			if(retStatus !=SYNCML_DM_SUCCESS)
				return retStatus;

			if(decDataBuf != NULL)
				m_ChunkOffset += decDataLen;
			else
				m_ChunkOffset += setLen;

			offset += setLen;
		 }

		 if(decDataBuf != NULL && dataLen != 0)
		 {
		 // Allocate the Large Object Buffer based on the size
			 lastChunkSize = lastData.getSize() + dataLen;

		 	largeObjectBuffer.allocate( lastChunkSize + 1);
			 if ( largeObjectBuffer.getBuffer() == NULL )
				return  SYNCML_DM_DEVICE_FULL;

			 // All non-processed data are shifted to the start of the input buffer
			largeObjectBuffer = remainData;
			largeObjectBuffer.append( lastData.getBuffer(),lastData.getSize());
			*szBuf = largeObjectBuffer.getBuffer();
		 }
		 else
		 {
			*szBuf = lastData.getBuffer();
			lastChunkSize = lastData.getSize();
		 }
   return retStatus;
}
/*==================================================================================================
FUNCTION        : SetLastChunkRemainData

DESCRIPTION     : Processing all the remaining data
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES : 

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::SetLastChunkRemainData(DMAddData & oAddData,
													    DMBuffer  &lastData,
													    UINT8* szBuf,
													    int chunksize,
													    UINT8  *decDataBuf,
													    UINT32 decMaxBufLen,
													    UINT32 lastChunkSize)
{
  SYNCML_DM_RET_STATUS_T retStatus =	SYNCML_DM_SUCCESS;
  BOOLEAN isAllSet = FALSE;
  int offset = 0;
  int setLen = 0;
  UINT32 dataLen = largeObjectSize;
  UINT32 decDataLen;

	offset = 0;
	while (offset <=  (int)lastChunkSize && !isAllSet)
	{   setLen =	lastChunkSize - offset;
	   if(setLen > chunksize)
			setLen = chunksize;
		else
			isAllSet = TRUE;

	  if(szBuf != NULL)
	  {	 
		  // special-case handling of SYNCML_DM_FORMAT_B64
		  if(decDataBuf != NULL)
		  {
			  dataLen = setLen;
			  decDataLen =	base64Decode(decDataBuf , decMaxBufLen, (DataBuffer_t) &szBuf[offset],
										(BufferSize_t *)&dataLen);
			  if (decDataLen == 0) 
			     return   SYNCML_DM_BAD_REQUEST;
			  
			  m_pChunkData->SetChunkData(decDataBuf, decDataLen);
			  setLen -= dataLen;
		  }
		  else
		  {
		  	m_pChunkData->SetChunkData((const UINT8 *)&szBuf[offset], setLen);
		  }
	  }

	  retStatus = SetChunkDataToEngine(oAddData,isAllSet);
	  if(retStatus !=SYNCML_DM_SUCCESS)
		  return  retStatus;
	  offset += setLen;

	}

  return  retStatus;

}


/*==================================================================================================
FUNCTION        : SetLastChunkData

DESCRIPTION     : Set last chunk data
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES : 

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::SetLastChunkData(DMAddData & oAddData, DMCommandType cmdType)
{
  SYNCML_DM_RET_STATUS_T retStatus =	SYNCML_DM_SUCCESS;
  UINT8  *decDataBuf = NULL;
	DMBuffer lastData;
	m_ChunkOffset = 0;
	UINT8* szBuf;
	int chunksize = m_pChunkData->GetChunkSize();
	UINT32 decMaxBufLen = chunksize*3/4+2;

	if(oAddData.m_oData.getSize() != 0)
		lastData.attach(oAddData.m_oData.getBuffer(), oAddData.m_oData.getSize()+1);
	oAddData.m_oData.reset();
	UINT32 lastChunkSize = lastData.getSize();

	// special-case handling of SYNCML_DM_FORMAT_B64
	if (oAddData.m_nFormat == SYNCML_DM_FORMAT_B64) 
	{
		  decDataBuf = (UINT8*)DmAllocMem(decMaxBufLen);
		  if(decDataBuf == NULL)
			 return SYNCML_DM_DEVICE_FULL;
		  oAddData.m_nFormat = SYNCML_DM_FORMAT_BIN;
	}

	// Create the leaf node first with empty data
	if(cmdType == DM_COMMAND_ADD)
	{
		retStatus = dmTreeObj.Add( oAddData, SYNCML_DM_REQUEST_TYPE_SERVER );
		if ( retStatus != SYNCML_DM_SUCCESS )
			goto setchunkdatafailed;

		if(largeObjFileHandle == NULL && m_ChunkOffset == 0 && lastData.getSize() ==0)
	         {
                       if(decDataBuf != NULL)
	                   DmFreeMem(decDataBuf);
			return retStatus;
	         }
	}

	 if(m_pChunkData == NULL)
	 {	 m_pChunkData = AllocateChunkBuffer();
	   if(m_pChunkData == NULL)
	   {	   retStatus = SYNCML_DM_DEVICE_FULL;
		   goto setchunkdatafailed;
	   }
	}
	// Is there is temoprary file ?
	if(largeObjFileHandle != NULL)
	{
	    retStatus = SetLastChunkDataFromFile(oAddData,lastData, &szBuf, chunksize,decDataBuf,decMaxBufLen,lastChunkSize);
	    if ( retStatus != SYNCML_DM_SUCCESS )
			goto setchunkdatafailed;
	}
	else
	{	szBuf =	lastData.getBuffer();
		lastChunkSize = lastData.getSize();
	}

	retStatus = SetLastChunkRemainData(oAddData,
									lastData,
									szBuf,
									chunksize,
									decDataBuf,
									decMaxBufLen,
									lastChunkSize);

	 if ( retStatus != SYNCML_DM_SUCCESS )
			goto setchunkdatafailed;

    if(decDataBuf != NULL)
	  DmFreeMem(decDataBuf);
return retStatus;

setchunkdatafailed:
  if(decDataBuf != NULL)
		DmFreeMem(decDataBuf);
  if(cmdType == DM_COMMAND_ADD)
	dmTreeObj.Delete( oAddData.getURI(), SYNCML_DM_REQUEST_TYPE_SERVER );
   return retStatus;
}

/*==================================================================================================
FUNCTION        : SetChunkData

DESCRIPTION     : Set chunk data
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES : 

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::SetChunkData(DMAddData & oAddData,DMCommandType cmdType, BOOLEAN isLastChunk)
{
  SYNCML_DM_RET_STATUS_T retStatus =	SYNCML_DM_SUCCESS;

// Save data in temporary file
 if(!isLastChunk)
 {
 
	 if(largeObjFileHandle->seek(XPL_FS_SEEK_SET, largeObjectBufferUsedSize) != SYNCML_DM_SUCCESS) 
		 return  SYNCML_DM_IO_FAILURE;
	 if(largeObjFileHandle->write((CPCHAR)oAddData.m_oData.getBuffer(), oAddData.m_oData.getSize()) != SYNCML_DM_SUCCESS) 
	 	return  SYNCML_DM_IO_FAILURE;
 }
 else
	 retStatus = SetLastChunkData(oAddData, cmdType);

 return retStatus;
}
/*==================================================================================================
FUNCTION        : CreateESNTempFile

DESCRIPTION     : Create and open a temporary ESN file
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::CreateESNTempFile()
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
	INT32 modeFlag = XPL_FS_FILE_WRITE;

    largeObjFileName = XPL_FS_TempEsnDir();
    XPL_FS_MkDir(largeObjFileName);
    largeObjFileName += "##lob##.lob";

    largeObjFileHandle = new DMFileHandler(largeObjFileName.c_str(), FALSE);
    if(largeObjFileHandle == NULL)
	  return	  SYNCML_DM_IO_FAILURE;
	
    if (largeObjFileHandle->open(modeFlag) != SYNCML_DM_SUCCESS)
		return	  SYNCML_DM_IO_FAILURE;
	return dm_stat;
}
/*==================================================================================================
FUNCTION        : OpenESNTempFile

DESCRIPTION     :  Open the temporary ESN file
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::OpenESNTempFile()
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
	INT32 modeFlag = XPL_FS_FILE_RDWR;
	// Open again using different flag
	largeObjFileHandle = new DMFileHandler(largeObjFileName.c_str(), FALSE);
	if(largeObjFileHandle == NULL)
		return  SYNCML_DM_IO_FAILURE;
	
	if (largeObjFileHandle->open(modeFlag) != SYNCML_DM_SUCCESS)
			dm_stat	 = SYNCML_DM_IO_FAILURE;
	return dm_stat;
}
/*==================================================================================================
FUNCTION        : PreProcessFirstMoreData

DESCRIPTION     : Handle more data case
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::PreProcessFirstMoreData(DMAddData & oAddData,
												  DMCommandType cmdType,
												  SmlItemPtr_t p_data_item,
												  int &size )
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
	   SmlMetInfMetInfPtr_t p_meta_info;
	   if(!pDmMgmtSessionObj->IsLargeObjectSupported())
		 return SYNCML_DM_FEATURE_NOT_SUPPORTED;

	  /* Set the ADD data */
	   dm_stat = DecodBase64Data(p_data_item, oAddData, FALSE);
	   if ( dm_stat != SYNCML_DM_SUCCESS )
		 return dm_stat;

	   // Get size	
	if(p_data_item->meta == NULL || p_data_item->meta->content == NULL)
		 return SYNCML_DM_INCOMPLETE_COMMAND;
	   
	   p_meta_info = (SmlMetInfMetInfPtr_t)p_data_item->meta->content;
	 // <Size> element MUST only be specified in the first data chunk 
	if(p_meta_info->size != NULL &&
	  p_meta_info->size->content != 0)
	{
	size = DmAtoi((const char *)p_meta_info->size->content);
	if(IsLargerThanMaxObjSize(size, m_bESN, FALSE))
		return  SYNCML_DM_REQUEST_ENTITY_TOO_LARGE;

	// Size of data chunk greater than/equal to specified object size
	if((int)oAddData.m_oData.getSize() >= size)
		return  SYNCML_DM_SIZE_MISMATCH;

	  largeObjectCmd = cmdType;
	  largeObjectTURI = oAddData.m_oURI;
	  largeObjectFormat = oAddData.m_nFormat;
	  largeObjectSize = size;
	  largeObjectType = oAddData.m_oMimeType;
	  m_ChunkOffset = 0;
	  largeObjectBufferUsedSize = 0;
	// Create temporary file for the Large Object
	  if(m_bESN)
	   	dm_stat =CreateESNTempFile();

	  if(dm_stat == SYNCML_DM_SUCCESS)
		  m_bProcessingLargeObject = TRUE;
   }
   else
   	{
	// Meta->Size /Item->Meta-Size missing
	   dm_stat =  SYNCML_DM_INCOMPLETE_COMMAND;
   	}
   return dm_stat;
}

/*==================================================================================================
FUNCTION        : LargeObjectRecvFirstChunk

DESCRIPTION     : Receives the first chunk of a large object data
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::LargeObjectRecvFirstChunk(DMAddData & oAddData,
												  DMCommandType cmdType,
												  SmlItemPtr_t p_data_item)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    int size = 0;
    BOOLEAN lsLastChunk = FALSE;
    BOOLEAN bMoreData = (p_data_item->flags & SmlMoreData_f) ==SmlMoreData_f;

    if(bMoreData &&  pDmMgmtSessionObj->IsProcessScript())
		return SYNCML_DM_FEATURE_NOT_SUPPORTED;

	// Is ESN
     dm_stat = dmTreeObj.IsESN(oAddData.getURI(), m_bESN);
     if ( dm_stat != SYNCML_DM_SUCCESS )
	  return dm_stat;

	// Normal nodes
     if( !bMoreData)
	{	
	  if(m_bESN)
	  {	
		   /* Set the ADD data */
	      dm_stat = DecodBase64Data(p_data_item, oAddData, FALSE);
		if ( dm_stat != SYNCML_DM_SUCCESS )
				 return  dm_stat;
	  	lsLastChunk = TRUE;
	  }
	  else
	  	{
		 /* Set the ADD data */
		dm_stat = DecodBase64Data(p_data_item, oAddData, TRUE);
		if ( dm_stat != SYNCML_DM_SUCCESS )
			  return  dm_stat;

		if(cmdType == DM_COMMAND_ADD)
			dm_stat = dmTreeObj.Add( oAddData, SYNCML_DM_REQUEST_TYPE_SERVER );
		else
			dm_stat = dmTreeObj.Replace( oAddData, SYNCML_DM_REQUEST_TYPE_SERVER );
		return dm_stat;
	  	}
     }
     else
    {
	dm_stat= PreProcessFirstMoreData(oAddData, cmdType, p_data_item, size );
	if(dm_stat != SYNCML_DM_SUCCESS)
	{	BuildAlertCommand(DM_ALERT_SESSION_ABORT, NULL, NULL);
		return dm_stat;
	}

    }
   if(m_bESN)
   {
	dm_stat = SetChunkData(oAddData, cmdType, lsLastChunk);
	if(dm_stat != SYNCML_DM_SUCCESS)
		goto  receivefirstchunkfailed;

	if(bMoreData)
		dm_stat = SYNCML_DM_CHUNK_BUFFERED;
   }
   else
   {
	// Allocate the Large Object Buffer based on the size
	largeObjectBuffer.allocate(size+1);
	if ( largeObjectBuffer.getBuffer() == NULL )
		goto  receivefirstchunkfailed;
	largeObjectBuffer = oAddData.m_oData;

	dm_stat = SYNCML_DM_CHUNK_BUFFERED;
   }
   largeObjectBufferUsedSize += oAddData.m_oData.getSize();
  return dm_stat;
  receivefirstchunkfailed:
   if(m_bProcessingLargeObject)
   {	
	BuildAlertCommand(DM_ALERT_SESSION_ABORT, NULL, NULL);
   	LargeObjectClear();
   }
   return dm_stat;

}
/*==================================================================================================
FUNCTION        : LargeObjectRecvDecodeData

DESCRIPTION     : Decode binary LOB data
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::LargeObjectRecvDecodeData(DMAddData & oAddData,
													  DMCommandType cmdType,
													  SmlItemPtr_t p_data_item)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    SmlMetInfMetInfPtr_t p_meta_info = NULL;

	// Check if it is the correct item 
	if (m_bProcessingLargeObject && (largeObjectCmd!=cmdType || 
								DmStrcmp((CPCHAR)largeObjectTURI.getBuffer(), oAddData.getURI()) !=0))

	{
		// Alert 1225 : End of data for chunked object not received
		BuildAlertCommand(DM_ALERT_END_OF_DATA_NOT_RECEIVED, NULL,(CPCHAR) largeObjectTURI.getBuffer());
		return SYNCML_DM_INCOMPLETE_COMMAND;
	}
	/* Set the ADD data */
	 dm_stat = DecodBase64Data(p_data_item, oAddData, FALSE);
	 if ( dm_stat != SYNCML_DM_SUCCESS )
	{	
		 LargeObjectClear();
		return  dm_stat;
	 }
	// Check if size has been specified again
	if(p_data_item->meta != NULL)
	{	p_meta_info = (SmlMetInfMetInfPtr_t)p_data_item->meta->content;

		if (p_meta_info != NULL  && p_meta_info->size != NULL )
		{	 dm_stat = SYNCML_DM_BAD_REQUEST;
			LargeObjectClear();
			return dm_stat;
		}
	}
	return dm_stat;
}
/*==================================================================================================
FUNCTION        : LargeObjectRecvLastChunk

DESCRIPTION     : Processing the last chunk of a large object data
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::LargeObjectRecvLastChunk(DMAddData & oAddData,
													  DMCommandType cmdType,
													  SmlItemPtr_t p_data_item)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
	if(largeObjectCmd == DM_COMMAND_ADD ||largeObjectCmd == DM_COMMAND_REPLACE)
	{
			// Replace data 
				oAddData.m_oData.free();
				oAddData.m_oData.attach(largeObjectBuffer.getBuffer(),  largeObjectBuffer.getSize() +1);

				 // special-case handling of SYNCML_DM_FORMAT_B64
				 if (oAddData.m_nFormat == SYNCML_DM_FORMAT_B64) 
				 {
					UINT8  *decDataBuf;
					UINT32 dataLen = largeObjectSize;
					UINT32 decDataLen;
				 	UINT32 decMaxBufLen = largeObjectSize*3/4+2;

					decDataBuf = (UINT8*)DmAllocMem(decMaxBufLen);
					if(decDataBuf == NULL)
					{
						return  SYNCML_DM_DEVICE_FULL;
					}
					decDataLen =  base64Decode(decDataBuf , decMaxBufLen, (DataBuffer_t) oAddData.m_oData.getBuffer(),
											  (BufferSize_t *)&dataLen);
					if (decDataLen == 0) 
					{  	
						DmFreeMem(decDataBuf);
						return   SYNCML_DM_BAD_REQUEST;
					}
				 
					oAddData.m_oData.assign(decDataBuf, decDataLen); 
					// largeObjectBuffer is freed
					largeObjectBuffer.reset();

					if ( oAddData.m_oData.getBuffer() == NULL )
					{ 
					    DmFreeMem(decDataBuf);
					    return SYNCML_DM_DEVICE_FULL;
					}
					DmFreeMem(decDataBuf);
					oAddData.m_nFormat = SYNCML_DM_FORMAT_BIN;
				  }

			
				if(largeObjectCmd == DM_COMMAND_ADD)
					dm_stat = dmTreeObj.Add( oAddData, SYNCML_DM_REQUEST_TYPE_SERVER );
				else
					dm_stat = dmTreeObj.Replace( oAddData, SYNCML_DM_REQUEST_TYPE_SERVER);
			
				oAddData.m_oData.reset();
	 }
	return dm_stat;
}
/*==================================================================================================
FUNCTION        : IsRecveivingLastChunk

DESCRIPTION     : Is the last chunk of a large object data received?
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::IsRecveivingLastChunk(DMAddData & oAddData,
													  BOOLEAN bMoreData,
													  int  &size,
													  BOOLEAN &lsLastChunk)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    lsLastChunk = FALSE;
	size = oAddData.m_oData.getSize();
	// Is data size too large
	if(IsLargerThanMaxObjSize(size, m_bESN, FALSE))
		return  SYNCML_DM_REQUEST_ENTITY_TOO_LARGE;
	// Object size is larger then MaxObjSize
	if(size > largeObjectSize - largeObjectBufferUsedSize)
		return  SYNCML_DM_SIZE_MISMATCH;

	// is it the last chunk ?
	if(bMoreData == FALSE )
	{
		if(( size+ largeObjectBufferUsedSize)!= largeObjectSize)
			return  SYNCML_DM_SIZE_MISMATCH;
		lsLastChunk = TRUE;
	}
	return dm_stat;
}
/*==================================================================================================
FUNCTION        : LargeObjectRecvNextChunk

DESCRIPTION     : Receives the next/last chunk of a large object data
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::LargeObjectRecvNextChunk(DMAddData & oAddData,
													  DMCommandType cmdType,
													  SmlItemPtr_t p_data_item)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    BOOLEAN bMoreData = (p_data_item->flags & SmlMoreData_f) ==SmlMoreData_f;
    int size = 0;
    BOOLEAN lsLastChunk = FALSE;

    dm_stat = LargeObjectRecvDecodeData(oAddData, cmdType,p_data_item);
    if(dm_stat != SYNCML_DM_SUCCESS)
			goto receivechunkfailed;

   // Is it the last chunk?
    dm_stat = IsRecveivingLastChunk(oAddData, bMoreData, size, lsLastChunk);
    if(dm_stat != SYNCML_DM_SUCCESS)
			goto receivechunkfailed;

	// Append to the buffer
	if(m_bESN)
	{
		dm_stat = SetChunkData(oAddData, cmdType, lsLastChunk);
		if(dm_stat != SYNCML_DM_SUCCESS)
			goto receivechunkfailed;
	}
	else
	{
		largeObjectBuffer.append(oAddData.m_oData.getBuffer(), oAddData.m_oData.getSize());

		// All the chunked data received
		if(lsLastChunk)
		{	dm_stat = LargeObjectRecvLastChunk(oAddData,
											cmdType,
											p_data_item);
		
			if(dm_stat != SYNCML_DM_SUCCESS)
				goto receivechunkfailed;
		}
	}
	if(bMoreData)
	{	dm_stat = SYNCML_DM_CHUNK_BUFFERED;
		largeObjectBufferUsedSize += size;
	}
	else
	{
		if(m_bProcessingLargeObject)
			LargeObjectClear();
	}
	return dm_stat;
receivechunkfailed:
 if(m_bProcessingLargeObject)
 {
	BuildAlertCommand(DM_ALERT_SESSION_ABORT, NULL, NULL);
 	LargeObjectClear();
 }
  return dm_stat;


}
/*==================================================================================================
FUNCTION        : GetTargetURI

DESCRIPTION     : Get target URI.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::GetLargeObjectTargetURI(SmlItemPtr_t p_get_item)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    UINT8 * pTargetURL = NULL;
    if(p_get_item==NULL)
    {
        return SYNCML_DM_BAD_REQUEST;
    }
	if((p_get_item->source != NULL) && (p_get_item->source->locURI != NULL))
	{
		DMString tempURI;
		pTargetURL = (UINT8*)p_get_item->source->locURI->content;
		tempURI.assign((CPCHAR)p_get_item->source->locURI->content,p_get_item->source->locURI->length);
		if ( tempURI == NULL )
		   return SYNCML_DM_DEVICE_FULL;

		largeObjectTURI.assign(tempURI);

		if(tempURI.Decode() == FALSE)
		 return SYNCML_DM_BAD_REQUEST;
		
		largeObjectSURI.assign(tempURI);
		
		if(largeObjectSURI.getBuffer() == NULL)
		 return SYNCML_DM_DEVICE_FULL;
	}
	else
		dm_stat = SYNCML_DM_BAD_REQUEST;

	return dm_stat;

}
/*==================================================================================================
FUNCTION        : SetEngineChunkData

DESCRIPTION     : Set chunk data to engine
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES : 

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage:: GetChunkDataFromEngine()
{
  SYNCML_DM_RET_STATUS_T retStatus =  SYNCML_DM_SUCCESS;
  DMGetData getData;
getData.chunkData = m_pChunkData;
getData.m_chunkOffset = m_ChunkOffset;

  retStatus = dmTreeObj.Get((CPCHAR)largeObjectSURI.getBuffer(),getData, SYNCML_DM_REQUEST_TYPE_SERVER);
	
  if ( retStatus != SYNCML_DM_SUCCESS )
	return retStatus;
	
  UINT32 returnLen =0;
  retStatus = m_pChunkData->GetReturnLen(returnLen);
  m_ChunkOffset += returnLen;
	  
  return retStatus;
}

/*==================================================================================================
FUNCTION        : GetChunkData

DESCRIPTION     :  Read one chunk of data from ESN 
                  package.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::GetChunkData(DMBuffer & oSendData, int bufSize)
{
  SYNCML_DM_RET_STATUS_T retStatus =	SYNCML_DM_SUCCESS;
  UINT8 *bufp;

  if(m_pChunkData == NULL)
  {   m_pChunkData = AllocateChunkBuffer();
    if(m_pChunkData == NULL)
		return SYNCML_DM_DEVICE_FULL;
 }
  // Is it an empty buffer
  if ( oSendData.getBuffer() == NULL )
	return  SYNCML_DM_DEVICE_FULL;

  // Is data in temporary file already?
  if(largeObjFileHandle != NULL)
  {
  
  	if(largeObjFileHandle->seek(XPL_FS_SEEK_SET, largeObjectBufferUsedSize) != SYNCML_DM_SUCCESS) 
		 return 	SYNCML_DM_IO_FAILURE;
	if(largeObjFileHandle->read( oSendData.getBuffer() , bufSize) != SYNCML_DM_SUCCESS) 
			return  SYNCML_DM_IO_FAILURE;
	oSendData.setSize(bufSize);
  }
  else
  {
   UINT8 *pEncData = NULL;
   UINT32 encLen =0;
   // Allocate buffer for binary data encoding
   if(m_bBinary)
   {
	   encLen = base64GetSize(bufSize);
	   pEncData = (UINT8 *)DmAllocMem(encLen + 1);
   
	   if (pEncData == NULL)
		   return SYNCML_DM_DEVICE_FULL;
	   memset(pEncData, 0, encLen + 1);
   }
   m_pChunkData->GetChunkData(&bufp);  // the chunk data is available  

  while (true) 
  {
	  retStatus = GetChunkDataFromEngine();
	  if( retStatus != SYNCML_DM_SUCCESS)
	  {
		  if(pEncData != NULL)
			  DmFreeMem(pEncData);
	  	return retStatus;
	  }
	  m_pChunkData->GetReturnLen(largeObjectChunkSize); 
	  if(largeObjectChunkSize == 0)
		 break;
	  // Is binary data?
	  if( pEncData != NULL)
	  {
		  UINT32 offset = 0;
		  UINT32 dataSize = largeObjectChunkSize;
		  UINT32 encSize =	base64Encode (pEncData, encLen,
										  (DataBuffer_t)bufp,
										  (BufferSize_t *)&dataSize,
										  (BufferSize_t *)&offset, 0, NULL); 
		  largeObjectChunkSize = encSize; 
		  oSendData.append(pEncData, encSize);
	  }
	  else
		  oSendData.append(bufp, largeObjectChunkSize);
	 
	 }
  	if(pEncData != NULL)
	  DmFreeMem(pEncData);

 } 										 
 return retStatus;
}
/*==================================================================================================
FUNCTION        : BuildLargeObjectDataNext

DESCRIPTION     :  Build the <data> item for the next chunk of a Large Object
                  package.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::BuildLargeObjectDataNext(SmlResultsPtr_t & p_results, int dataBufferSize)
{
   SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
   DMBuffer sendData;
   SmlItemListPtr_t p_results_list_item = p_results->itemList;
   SmlItemPtr_t p_results_item = p_results_list_item->item;

   sendData.allocate(dataBufferSize+1);
   if ( sendData.getBuffer() == NULL )
		return SYNCML_DM_DEVICE_FULL;
   
   
  if(m_bESN)
  {
	dm_stat = GetChunkData(sendData, dataBufferSize);
	if(dm_stat != SYNCML_DM_SUCCESS)
			   return dm_stat;
  }
  else
  {
	 largeObjectBuffer.copyTo(largeObjectBufferUsedSize, dataBufferSize, sendData);
  }
  largeObjectBufferUsedSize += sendData.getSize();
  p_results_item->flags |= SmlMoreData_f;
   
   // Build data item
   BuildPcData(p_results_item->data, 
						 SML_PCDATA_STRING,
						 SML_EXT_UNDEFINED,
						 sendData.getSize(),
						 sendData.getBuffer());
  return  dm_stat;
}

/*==================================================================================================
FUNCTION        : BuildLargeObjectDataLast

DESCRIPTION     :  Build the <data> item for the last chunk of a Large Object
                  package.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::BuildLargeObjectDataLast(SmlResultsPtr_t & p_results, int size, BOOLEAN isFirstChunk)
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
	DMBuffer sendData;
	SmlItemListPtr_t p_results_list_item = p_results->itemList;
	SmlItemPtr_t p_results_item = p_results_list_item->item;
	SmlMetInfMetInfPtr_t p_meta_info;

	// Is ESN data?
	if(m_bESN && size >0)
	{
		// Allocate the Large Object Buffer based on the size
		sendData.allocate(size+1);
		if ( sendData.getBuffer() == NULL )
			return SYNCML_DM_DEVICE_FULL;
	
		dm_stat = GetChunkData(sendData, size);
		if(dm_stat != SYNCML_DM_SUCCESS)	
			return dm_stat;
		// If ESN data can hold one message
		if(isFirstChunk )
		{
			largeObjectSize = sendData.getSize();
			// Replace the <size> statement for binary data
			if( m_bBinary && p_results_item->meta != NULL)
			{
				p_meta_info = (SmlMetInfMetInfPtr_t)p_results_item->meta->content;
				if(p_meta_info != NULL && p_meta_info->size!= NULL && p_meta_info->size->content != NULL)
				{
				  smlFreePcdata(p_meta_info->size);
				  p_meta_info->size = NULL;
				   dm_stat = BuildMetaSizeInfo(p_results_item->meta, sendData.getSize());
				  if(dm_stat != SYNCML_DM_SUCCESS)
							return dm_stat;

				}

		 	}
		}
		// Build data item
		BuildPcData(p_results_item->data, 
				  SML_PCDATA_STRING,
				  SML_EXT_UNDEFINED,
				  sendData.getSize(),
				  sendData.getBuffer());

		largeObjectBufferUsedSize += sendData.getSize();
		if( largeObjectBufferUsedSize != largeObjectSize)
			dm_stat =  SYNCML_DM_SIZE_MISMATCH;

	}
	else
	{
		if(!isFirstChunk && size > 0)
		{
			// Copy all the remaining data
			largeObjectBuffer.copyTo(largeObjectBufferUsedSize, size, sendData);
			largeObjectBufferUsedSize += sendData.getSize();
			if( largeObjectBufferUsedSize != largeObjectSize)
				dm_stat = SYNCML_DM_SIZE_MISMATCH;

			// Build data item
			BuildPcData(p_results_item->data, 
					  SML_PCDATA_STRING,
					  SML_EXT_UNDEFINED,
					  sendData.getSize(),
					  sendData.getBuffer());
			
			
		}
	}
	return dm_stat;
}
/*==================================================================================================
FUNCTION        : PrepareESNDataBuffer

DESCRIPTION     :  Allocate ESN buffer and encoding binary data
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::PrepareESNDataBuffer(UINT8 **pEncData, UINT32 & encLen , UINT8 **bufp)
{
 SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
 // Get chunk buffer
  if(m_pChunkData == NULL)
  {   m_pChunkData = AllocateChunkBuffer();
    if(m_pChunkData == NULL)
		return SYNCML_DM_DEVICE_FULL;
 }
  m_pChunkData->GetChunkData(bufp);  // the chunk data is available  

 // Allocate buffer for binary data encoding
 if(m_bBinary)
 {
	 encLen = base64GetSize((BufferSize_t)DmtDataChunk::GetChunkSize());
	 *pEncData = (UINT8 *)DmAllocMem(encLen + 1);
 
	 if (*pEncData == NULL)
	 	 return SYNCML_DM_DEVICE_FULL;
	 memset(*pEncData, 0, encLen + 1);
  }
  dm_stat =CreateESNTempFile();
  if(dm_stat != SYNCML_DM_SUCCESS)
  {
	if(*pEncData != NULL)
	{	  DmFreeMem(*pEncData);
		*pEncData = NULL;
	}
  }
  return  dm_stat;
}

/*==================================================================================================
FUNCTION        : GetESNData2TempFile

DESCRIPTION     :  Read ESN data to a temporary file and encoding binary data
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::GetESNData2TempFile(int & dataSize)
{
 SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
 UINT8 *pEncData = NULL;
 UINT32 encLen =0;
 UINT8 *bufp;

  dm_stat = PrepareESNDataBuffer(&pEncData, encLen , &bufp);
  if(dm_stat != SYNCML_DM_SUCCESS)
  	return  dm_stat;

  int offset =0;
  UINT8 *tempPtr = bufp;
   if( pEncData != NULL)
	  tempPtr = pEncData;
  while (true) 
 {
		dm_stat = GetChunkDataFromEngine();  
		if( dm_stat != SYNCML_DM_SUCCESS)
		   goto  getESNdatafailed;

	 	 m_pChunkData->GetReturnLen(largeObjectChunkSize);	 
		 if(largeObjectChunkSize == 0)
		   break;
		 if(largeObjFileHandle->seek(XPL_FS_SEEK_SET, offset) != SYNCML_DM_SUCCESS) 
		 {
		 	dm_stat =  SYNCML_DM_IO_FAILURE;
			goto  getESNdatafailed;
		 }
		// Is binary data?
		 if( pEncData != NULL)
		 {
			UINT32 offset = 0;
			UINT32 dataSize = largeObjectChunkSize;
			UINT32 encSize =  base64Encode (pEncData, encLen,
										   (DataBuffer_t) bufp,
											(BufferSize_t *)&dataSize,
											(BufferSize_t *)&offset, 0, NULL); 
			largeObjectChunkSize = encSize; 
		  }
		 if(largeObjFileHandle->write((CPCHAR)tempPtr, largeObjectChunkSize) != SYNCML_DM_SUCCESS) 
		 {
				dm_stat =  SYNCML_DM_IO_FAILURE;
				goto  getESNdatafailed;
		 }
		 offset += largeObjectChunkSize;
  }
  largeObjFileHandle->close();
  delete largeObjFileHandle;
  largeObjFileHandle = NULL;
  if(pEncData != NULL)
	  DmFreeMem(pEncData);
  dm_stat = OpenESNTempFile();
  if(dm_stat != SYNCML_DM_SUCCESS)
  	goto getESNdatafailed;
  
// Real total size
 dataSize = offset;

 return  dm_stat;
 getESNdatafailed:
 if(pEncData != NULL)
	 DmFreeMem(pEncData);
 return dm_stat;
}
/*==================================================================================================
FUNCTION        : LargeObjectSendFirstChunk

DESCRIPTION     : PrepareSend result data 
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::LargeObjectSendFirstChunkNoSpace(SYNCML_DM_RESULT_VALUE &pResult ,int size , int dataBufferSize )
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    SmlResultsPtr_t p_results  = pResult._pGetExecResult;
    if (p_results == NULL)
       return (SYNCML_DM_FAIL);

   SmlItemListPtr_t p_results_list_item =  p_results->itemList;
   SmlItemPtr_t p_results_item = p_results_list_item->item;

	  // Is Large Object supported?
	 if(!pDmMgmtSessionObj->IsLargeObjectSupported() || dataBufferSize<=0)
	{	
		dm_stat =  SYNCML_DM_RESULTS_TOO_LARGE;
		return dm_stat;
	 }
	dm_stat = GetLargeObjectTargetURI(p_results_item);
	if(dm_stat != SYNCML_DM_SUCCESS)
			goto sendNoSpacefailed;

	if(m_bESN)
		dm_stat = GetESNData2TempFile(size);
	
	if(dm_stat != SYNCML_DM_SUCCESS)
		goto sendNoSpacefailed;
	
      SmlMetInfMetInfPtr_t p_meta_info;
	// At least <Size> meta information need to be built
	if(p_results_item->meta == NULL)
       	 p_results_item->meta = smlAllocPcdata();
	if(p_results_item->meta == NULL)
	{
		dm_stat = SYNCML_DM_DEVICE_FULL;
		goto sendNoSpacefailed;
	}
	// Save meta type information
       p_meta_info = (SmlMetInfMetInfPtr_t)p_results_item->meta->content;
	// Replace the <size> statement for binary data
	if(p_meta_info != NULL && p_meta_info->size!= NULL && p_meta_info->size->content != NULL)
	{
	  smlFreePcdata(p_meta_info->size);
	  p_meta_info->size = NULL;
	}
	 // For Large Object delivery, <Size> must be built in first message  
	 dm_stat = BuildMetaSizeInfo(p_results_item->meta, size);
	if(dm_stat != SYNCML_DM_SUCCESS)
			  goto sendNoSpacefailed;

	 if(p_meta_info != NULL && p_meta_info->type!= NULL && p_meta_info->type->content != NULL)
			largeObjectType.assign((CPCHAR)p_meta_info->type->content ,(INT32) p_meta_info->type->length);


	largeObjectSize = size;
	m_ChunkOffset = 0;
	m_bProcessingLargeObject = TRUE;
	largeObjectBufferUsedSize = 0;
	largeObjectCmd = DM_COMMAND_RESULTS;
	largeObjectChunkSize = 0;
	largeObjectChunkOffset =0;

	largeObjectCmdRef = pResult._cmdRef;
	largeObjectMsgRef = pResult._msgID;
		 // Allocate the Large Object Buffer based on the size
	if(dataBufferSize > size)
			 dataBufferSize = size;

	// Data buffer already allocated
	if(!m_bESN)
	{
		largeObjectBuffer.allocate(size+1);
		if ( largeObjectBuffer.getBuffer() == NULL )
			goto sendNoSpacefailed;
		// Copy data in <data> to Large Object buffer
		largeObjectBuffer.attach((UINT8 *)p_results_item->data->content ,size +1);
		// Clear buffer pointer
		p_results_item->data->content = NULL;
	}
	dm_stat = BuildLargeObjectDataNext(p_results, dataBufferSize);
	if(dm_stat != SYNCML_DM_SUCCESS)
		goto sendNoSpacefailed;

	 // Insert a result command
	 dm_stat = BuildResultsCommand(p_results);
	 if(dm_stat != SYNCML_DM_SUCCESS)
		 goto sendNoSpacefailed;

	 smlFreeResults(p_results);
	 pResult._pGetExecResult = NULL;

	//	No <Final> statement	
	dm_stat = SYNCML_DM_RESULTS_TOO_LARGE;
       return dm_stat;

sendNoSpacefailed:
  if(p_results != NULL)
		smlFreeResults(p_results);
   pResult._pGetExecResult = NULL;
   if(m_bProcessingLargeObject)
	LargeObjectClear();
   return dm_stat;

}
/*==================================================================================================
FUNCTION        : LargeObjectSendGetFreeSpace

DESCRIPTION     : Get free space left
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::LargeObjectSendGetFreeSpace(  SmlResultsPtr_t p_results,
															SmlItemPtr_t p_results_item ,
															int &size , 
															int  &originalSize , 
															int  &dataBufferSize, 
															MemSize_t &freeSpace  )
{
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
     size = 0;
     originalSize = 0;
     m_bESN = FALSE;
     m_bBinary = FALSE;
	
 

   // Raw data size
   if(p_results_item !=  NULL && p_results_item->data != NULL)
   {	 size = p_results_item->data->length;
   	 originalSize = size;
	 p_results_item->data->length = 0;
   }
    // check the size
    smlStartEvaluation(sendInstanceId);
    smlResultsCmd(sendInstanceId, p_results);
    smlEndEvaluation(sendInstanceId, &freeSpace);

   // External Storage Node processing
    if( p_results_item != NULL && p_results_item->flags & SmlESNData_f)
   {	   m_bESN = TRUE;
	 // Always need to clearn up 
	 m_bProcessingLargeObject = TRUE;
	 if( p_results_item->flags &  SmlESNBinary_f)
		m_bBinary = TRUE;
  }
  // Adjust for </moredata> statement <size>
   dataBufferSize = freeSpace - 100;
   freeSpace =freeSpace -size - 100;
    return   	dm_stat;
}
/*==================================================================================================
FUNCTION        : LargeObjectSendFirstChunk

DESCRIPTION     : PrepareSend result data 
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :
==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::LargeObjectSendFirstChunk(SYNCML_DM_RESULT_VALUE &pResult)
{
   SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    SmlResultsPtr_t p_results = pResult._pGetExecResult;
   SmlItemListPtr_t p_results_list_item = NULL;
   SmlItemPtr_t p_results_item = NULL;
   int size = 0;
   int originalSize = 0;
   int dataBufferSize = 0;


   m_bESN = FALSE;
   m_bBinary = FALSE;
   DMBuffer sendData;
   MemSize_t freeSpace = 0;
   
  if( p_results == NULL)
    return   	dm_stat;
  	
  p_results_list_item = p_results->itemList;
  p_results_item = p_results_list_item->item;

   dm_stat = LargeObjectSendGetFreeSpace(p_results, p_results_item , size , originalSize , dataBufferSize, freeSpace  );

   // Are there any free buffer left? ( </MoreData>
    if(freeSpace <=0)
    {
	  // Is Large Object supported?
	 if(!pDmMgmtSessionObj->IsLargeObjectSupported() || dataBufferSize<=0)
	{	
		dm_stat =  SYNCML_DM_RESULTS_TOO_LARGE;
		return dm_stat;
	 }
	dm_stat = LargeObjectSendFirstChunkNoSpace( pResult , size, dataBufferSize );
    }
  else
  	{
	  // Restore the length field
	  if(p_results_item !=	NULL && p_results_item->data != NULL)
		  p_results_item->data->length = originalSize;

	  if(m_bESN)
	  {	dm_stat = GetLargeObjectTargetURI(p_results_item);
		  if(dm_stat != SYNCML_DM_SUCCESS)
			  goto sendfirstchunkfailed;
		  m_bProcessingLargeObject = TRUE;

	  }
	  dm_stat = BuildLargeObjectDataLast(p_results, size,TRUE);
	  if(dm_stat != SYNCML_DM_SUCCESS)
		 return dm_stat;
	   // Insert a result command
	   dm_stat = BuildResultsCommand(p_results);
	   smlFreeResults(p_results);
	   pResult._pGetExecResult = NULL;
	   if(m_bProcessingLargeObject)
		LargeObjectClear();

  	}
  return dm_stat;

sendfirstchunkfailed:
  if(p_results != NULL)
		smlFreeResults(p_results);
   pResult._pGetExecResult = NULL;
   if(m_bProcessingLargeObject)
	LargeObjectClear();
   return dm_stat;

}

/*==================================================================================================
FUNCTION        : AllocateLargeObjectResult

DESCRIPTION     : Allocates and fill in memory structure for result  
                  package.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/

SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::AllocateLargeObjectResult(SmlResultsPtr_t & p_results)
{
    p_results = smlAllocResults();
    if ( !p_results ) 
        return SYNCML_DM_DEVICE_FULL;

    SmlItemListPtr_t p_results_list_item = p_results->itemList;
    SmlItemPtr_t p_results_item = p_results_list_item->item;

    p_results_item->source = smlAllocSource();
    if ( p_results_item->source == NULL )
    {
        smlFreeResults(p_results); 
        p_results = NULL;
        return (SYNCML_DM_FAIL);  
    }

    BuildPcData( p_results_item->source->locURI, 
                 SML_PCDATA_STRING,
                 SML_EXT_UNDEFINED, 
                 largeObjectTURI.getSize(), 
                 (UINT8*)largeObjectTURI.getBuffer());

    /* Set receiving package command id reference */
    BuildPcData(p_results->cmdRef, SML_PCDATA_STRING,
                SML_EXT_UNDEFINED,
                largeObjectCmdRef.length(),
                (UINT8*)largeObjectCmdRef.c_str());

    CPCHAR sMsg = largeObjectMsgRef.c_str();
    BuildPcDataWAllocMem(&p_results->msgRef,DmStrlen(sMsg),(UINT8*)sMsg);
   
   if(largeObjectType.getSize() !=0)
   {
     if ((p_results_item->meta = smlAllocPcdata()) != NULL)
                    BuildMetaInfo(p_results_item->meta,
                                  NULL,
                                  (UINT8*)largeObjectType.getBuffer(),
                                  NULL,
                                  NULL,
                                  NULL, NULL, NULL, NULL);
   }

   p_results_item->data = smlAllocPcdata();
    if ( !p_results_item->data  ) 
        return SYNCML_DM_DEVICE_FULL;

    return SYNCML_DM_SUCCESS;
}

/*==================================================================================================
FUNCTION        : LargeObjectSendNextChunk

DESCRIPTION     : Send next Large Object chunk data
                  package.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
BOOLEAN 
SYNCML_DM_BuildPackage::LargeObjectSendNextChunk(SYNCML_DM_RET_STATUS_T &dm_stat, BOOLEAN &isLastChunk )
{
 DMBuffer sendData;

 isLastChunk = FALSE;

 if (!m_bProcessingLargeObject || largeObjectCmd != DM_COMMAND_RESULTS)
 {
	dm_stat = SYNCML_DM_SUCCESS;
 	return FALSE;
 }

 SmlResultsPtr_t  p_results = NULL;
 int size = largeObjectSize - largeObjectBufferUsedSize;
 int dataBufferSize = 0;

 isLastChunk = TRUE;
 // Need to send result back to server
 pDmMgmtSessionObj->IncCommandCount();
 
 dm_stat = AllocateLargeObjectResult(p_results);
 
 if ( dm_stat != SYNCML_DM_SUCCESS ) 
{	 if(p_results != NULL)
		 smlFreeResults(p_results); 
	 return FALSE;

 }

 // check the size
 MemSize_t freeSpace = 0;
 
 smlStartEvaluation(sendInstanceId);
 smlResultsCmd(sendInstanceId, p_results);
 smlEndEvaluation(sendInstanceId, &freeSpace);

 // Adjust for </MoreData>
 dataBufferSize = freeSpace - 100;
 freeSpace =freeSpace -size - 100;
  // Are there any free buffer left?
   if(freeSpace <=  0)
   {
	if(dataBufferSize > size)
		dataBufferSize = size;

	dm_stat = BuildLargeObjectDataNext(p_results, dataBufferSize);
	if(dm_stat != SYNCML_DM_SUCCESS)
	   goto sendchunkfailed;
   
   // Insert a result command
	dm_stat = BuildResultsCommand(p_results);
      if(dm_stat != SYNCML_DM_SUCCESS)
	   goto sendchunkfailed;
     
	 smlFreeResults(p_results);
	 p_results = NULL;
	 dm_stat = SYNCML_DM_RESULTS_TOO_LARGE;
   }
 else
   {
	 dm_stat = BuildLargeObjectDataLast(p_results, size,FALSE);
	 if(dm_stat != SYNCML_DM_SUCCESS)
		 goto sendchunkfailed;

	// Insert a result command
	dm_stat = BuildResultsCommand(p_results);
	
	if(dm_stat != SYNCML_DM_SUCCESS)
		goto sendchunkfailed;

	smlFreeResults(p_results);
	LargeObjectClear();
	return FALSE;
   }
 return TRUE;

sendchunkfailed:
 if(p_results != NULL)
	smlFreeResults(p_results); 

  if(m_bProcessingLargeObject)
		LargeObjectClear();
  return FALSE;

}

/*==================================================================================================
FUNCTION        : GenerateAlertForLOB

DESCRIPTION     : Generate alert in error conditions.
                  package.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/

 SYNCML_DM_RET_STATUS_T SYNCML_DM_BuildPackage::GenerateAlertForLOB(DMCommandType currentComm)
 {
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    if(m_bProcessingLargeObject)
    {
    	if(currentComm != largeObjectCmd)
	{	
		BuildAlertCommand(DM_ALERT_END_OF_DATA_NOT_RECEIVED, NULL,(CPCHAR) largeObjectTURI.getBuffer());
		LargeObjectClear();
		dm_stat = SYNCML_DM_INCOMPLETE_COMMAND;
    	}
    }
    return dm_stat;
 }

// End of defination of LOB_SUPPORT
#endif


SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::GetClientAuthValues( const DMString& device_id,
                                          const DMString& server_id,
                                          DMString&       client_name,
                                          DMString&       client_password )
{
  SYNCML_DM_RET_STATUS_T result = SYNCML_DM_SUCCESS;

  for( ; ; )
  {
    SYNCML_DM_FACTORY_BOOTSTRAP_T bootstrapCode = GetBootStrapCodeUserName( client_name );

    if( SYNCML_DM_FACTORY_BOOTSTRAP_UNKNOWN != bootstrapCode ) 
    { 
      result = CreateFactoryBootStrapUserName( bootstrapCode, 
                                               server_id,
                                               device_id,
                                               client_name );
      if (SYNCML_DM_SUCCESS != result ) break;
    }

    SYNCML_DM_FACTORY_BOOTSTRAP_T clientPWCode = GetBootStrapCodeClientPW( client_password );

    if( SYNCML_DM_FACTORY_BOOTSTRAP_UNKNOWN != clientPWCode )
    {
      result = CreateFactoryBootStrapClientPW( clientPWCode,
                                               server_id,
                                               device_id,
                                               client_password );

      if (SYNCML_DM_SUCCESS != result ) break;
    }
    
    break;
  }

  return result;
}


SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::GetServerAuthValues( const DMString& device_id,
                                           const DMString& server_id,
                                           DMString&       server_password )
{
  SYNCML_DM_RET_STATUS_T result = SYNCML_DM_SUCCESS;

  for( ; ; )
  {
    SYNCML_DM_FACTORY_BOOTSTRAP_T serverPWCode = GetBootStrapCodeServerPW( server_password );
    if ( SYNCML_DM_FACTORY_BOOTSTRAP_UNKNOWN != serverPWCode )
    {

      result = CreateFactoryBootStrapServerPW( serverPWCode, 
                                               server_id,
                                               device_id,
                                               server_password );
      if (SYNCML_DM_SUCCESS != result ) break;
    }

    break;
  }

  return result;
}

SYNCML_DM_RET_STATUS_T 
SYNCML_DM_BuildPackage::GetDeviceID( DMString& device_id )
{
  DMGetData              devID;
  SYNCML_DM_RET_STATUS_T result = dmTreeObj.Get(DM_DEV_INFO_DEVID_URI, devID, SYNCML_DM_REQUEST_TYPE_INTERNAL);

  if( ( SYNCML_DM_SUCCESS == result ) && 
      ( devID.m_oData.getSize() != 0 ) )
  {
    device_id = GetIMEINumber( devID.getCharData() );
  }

  return result;
 }

//SessionId option/FLEX is  flexible feature.
//TRUE -  Session id will be converted into hex format which will be used for Dm sessions
//FALSE -  Session id will be in decimal format which will be used for Dm sessions
 //PATH,Value Stored : ./DevDetail/Ext/Conf/PMF/Agents/syncmldm/Sessionid

BOOLEAN
SYNCML_DM_BuildPackage::IsSessionId()
{
  
  CPCHAR path = XPL_DM_GetEnv(SYNCML_DM_SESSION_ID);
  if ( !path )
      return TRUE;

  DMGetData sessionId;
  SYNCML_DM_RET_STATUS_T result = dmTreeObj.Get(path, 
                                                sessionId, 
                                                SYNCML_DM_REQUEST_TYPE_INTERNAL);

  if( result != SYNCML_DM_SUCCESS )
    return TRUE;

  return sessionId.m_oData.compare("true");

}
