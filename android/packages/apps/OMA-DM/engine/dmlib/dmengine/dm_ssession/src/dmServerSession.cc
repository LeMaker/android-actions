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

    Source Name: DMServerSession.cc

    General Description: Implementation of DMServerSession class.

==================================================================================================*/

#include "dmServerSession.h"
#include "dm_ua_handlecommand.h"
#include "dm_tree_util.h"
#include "xpl_dm_Manager.h"
#include "xpl_dm_Notifications.h"
#include "dmLockingHelper.h"

#ifndef DM_NO_LOCKING
#include "dmThreadQueue.h"
#endif

extern "C" {
#include "xpt-b64.h"
#include "stdio.h"
}

#include "xpl_Logger.h"

extern int g_cancelSession;

/*==================================================================================================
                                 SOURCE FUNCTIONS
==================================================================================================*/


/*==================================================================================================
FUNCTION        : DMServerSession::DMServerSession

DESCRIPTION     : The class constructor.
ARGUMENT PASSED : sml_ContentType
                  sml_EncodingType
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
DMServerSession::DMServerSession()
{   

    m_nSecState = DM_CLIENT_NO_SERVER_NO_AUTH;
}




/*==================================================================================================
FUNCTION        : DMServerSession::ConnectServer

DESCRIPTION     : The SessionStart() function calls this function to connect to the server.
                  The function will perform the following operations:
                  1) Get the DM account URI from the DM tree. 
                  2) Call SYNCML_DM_TransportController::Connect() function to establish the DM 
                     session clent/server connection.
ARGUMENT PASSED : server_Id
OUTPUT PARAMETER: 
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMServerSession::ConnectServer(CPCHAR pServerId)
{
    SYNCML_DM_RET_STATUS_T  ret_stat = SYNCML_DM_SUCCESS;
    XPL_ADDR_TYPE_T         addressType = XPL_ADDR_DEFAULT;	
    CPCHAR                  szDMAccRootPath = ::XPL_DM_GetEnv( SYNCML_DM_DMACC_ROOT_PATH );
    CPCHAR                  szServerIdNodeName = ::XPL_DM_GetEnv( SYNCML_DM_NODENAME_SERVERID );


     XPL_LOG_DM_SESS_Debug(("Entered DMServerSession::ConnectServer\n"));

    /* Get the current server ID's DM account URI */
    if ( !dmTreeObj.GetParentOfKeyValue (pServerId,
                                         szServerIdNodeName,
                                         szDMAccRootPath,
                                         clientServerCreds.pDMAccNodeName) )
    {
      XPL_LOG_DM_SESS_Error(("Cannot Get Parent for %s\n",szServerIdNodeName ));
      return SYNCML_DM_FAIL;
    }
	    
    DMGetData oDataAddr, oDataAddrType, oDataPort, oData;

    ret_stat = dmTreeObj.GetDefAccountAddrInfo((CPCHAR)clientServerCreds.pDMAccNodeName,
                                               oDataAddr, 
                                               oDataAddrType, 
                                               oDataPort );

    if ( ret_stat != SYNCML_DM_SUCCESS )
    { 
      XPL_LOG_DM_SESS_Error(("Cannot Get Account Address Info for %s\n",(CPCHAR)clientServerCreds.pDMAccNodeName ));
      return ret_stat;
    }

    if ( dmTreeObj.IsVersion_12()  )
    {
	    // Get the value of the pbDMAccUri/PrefConRef node
	    addressType = XPL_ADDR_HTTP;	
	    ret_stat = dmTreeObj.GetAccNodeValue((CPCHAR)clientServerCreds.pDMAccNodeName,
	                                          DM_PREFCONREF,
	                                          oData);
    }	
    else 
    {
           if ( oDataAddrType.getCharData() )
   		addressType = DmAtoi(oDataAddrType.getCharData());
	    else
	       addressType = XPL_ADDR_HTTP;

	    if (  addressType != XPL_ADDR_HTTP && addressType != XPL_ADDR_WSP )
			return SYNCML_DM_FAIL;
		
   	    ret_stat = dmTreeObj.GetAccNodeValue((CPCHAR)clientServerCreds.pDMAccNodeName,
	                                          DM_CONREF,
	                                          oData);
    }	
		
    if ( ret_stat == SYNCML_DM_SUCCESS )
     { dmTreeObj.SetConRef(oData.getCharData());
      	ret_stat = m_oConnObject.Init(g_iDMWorkspaceSize,addressType,oData.getCharData()); 
    }
    else
        if ( ret_stat == SYNCML_DM_NOT_FOUND )
         { 	dmTreeObj.SetConRef(NULL);
         	ret_stat = m_oConnObject.Init(g_iDMWorkspaceSize,addressType,NULL); 
         }

    XPL_LOG_DM_SESS_Debug (("Leaving DMServerSession::ConnectServer ret_stat=%d\n",ret_stat));
    return (ret_stat);
}


/*==================================================================================================
FUNCTION        : DMServerSession::BuildSendPackageOne

DESCRIPTION     : SessionStart function calls this function to build the DM package one and send it
                  to the server.
                  The function will perform the following operations:
                  1) Create the SYNCML_DM_BuildPackage object. 
                  2) Build the package one document.
                  3) Send the package one to the Transport Binding.
ARGUMENT PASSED : session_Direction
                  p_ParsedPk0
OUTPUT PARAMETER:
RETURN VALUE    : STYNCML_DM_SUCCES for success or SYNCML_DM_FAIL for failure
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMServerSession::BuildSendPackageOne(CPCHAR pServerID, DmtSessionProp * pSessionProp)
{
    XPL_LOG_DM_SESS_Debug (("Entering DMServerSession::BuildSendPackageOne serverId = %s\n", pServerID));
    SYNCML_DM_RET_STATUS_T  ret_stat = SYNCML_DM_SUCCESS;
    m_oPkgBuilder.Init(this);

#ifndef DM_NO_LOCKING  
    INT32 nLockID = 0;
    {
        DMLockingHelper oLock( 0, ".", pServerID,SYNCML_DM_LOCK_TYPE_EXCLUSIVE, FALSE);
        nLockID = oLock.GetID();

        if ( !oLock.IsLockedSuccessfully() )
        {
            return SYNCML_DM_FAIL;
        }

        ret_stat = m_oPkgBuilder.BuildPackageOne(pServerID, pSessionProp);
    }

    dmTreeObj.ReleaseLock( nLockID );
#else
    ret_stat = m_oPkgBuilder.BuildPackageOne(pServerID, pSessionProp);
#endif
    if ( ret_stat == SYNCML_DM_SUCCESS )
    {
        ret_stat = SendPackage();
    }    

    return (ret_stat);
}


/*==================================================================================================
FUNCTION        : DMServerSession::SessionStart

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
DMServerSession::Start(CPCHAR pServerID, 
                       DmtSessionProp * pSessionProp)
{
    SYNCML_DM_RET_STATUS_T ret_stat;
    BOOLEAN bMoreMessage = TRUE;

    ret_stat = Init(pSessionProp->isWBXML());
    if ( ret_stat != SYNCML_DM_SUCCESS )
        return ret_stat;
    
    /* Register the DM engine with the SYNCML toolkit. */
    userData.pSessionMng = this;
    userData.pPkgBuilder = &m_oPkgBuilder;
    ret_stat = RegisterDmEngineWithSyncmlToolkit(&userData);
    if ( ret_stat != SYNCML_DM_SUCCESS )
    {
        XPL_LOG_DM_SESS_Debug(("Exiting: RegisterDmEngineWithSyncmlToolkit failed\n"));
        return (ret_stat);
    }

    /* Create connection between server and client */
    ret_stat = ConnectServer(pServerID);
    if (ret_stat != SYNCML_DM_SUCCESS)
    {
        XPL_LOG_DM_SESS_Error(("Cannot connect server\n"));
        return (ret_stat);
    }

    /* Remember the sessionDirection in case we have to resend the Alert.*/
    pSessionProp->generateSessionID();
    serverSessionId = pSessionProp->getSessionID();


    /* Build and send the package one to the Server. */
    ret_stat = BuildSendPackageOne(pServerID, pSessionProp);
    if (ret_stat != SYNCML_DM_SUCCESS)
        return (ret_stat);

    XPL_DM_NotifySessionProgress(TRUE);

    while (bMoreMessage == TRUE && g_cancelSession == 0)
    {
        /* Call the method to receive the package from the transport.*/
        ret_stat = RecvPackage();
        if (ret_stat != SYNCML_DM_SUCCESS)
        {
             XPL_DM_NotifySessionProgress(FALSE);
            return ret_stat;
        }    

        /* Call the method to parse and handle the commands in the package.*/
        commandCount = 0;
              
 #ifndef DM_NO_LOCKING              
        int nLockID = 0;

        {
           DMLockingHelper oLock( 0, ".", pServerID, SYNCML_DM_LOCK_TYPE_EXCLUSIVE, FALSE );
           nLockID = oLock.GetID();

           if ( !oLock.IsLockedSuccessfully() )
           {
               XPL_DM_NotifySessionProgress(FALSE);
               return SYNCML_DM_FAIL;
           }   
      
           ret_stat = ParseMessage();
        }

        dmTreeObj.ReleaseLock( nLockID );
#else
        ret_stat = ParseMessage();
#endif 

        if (ret_stat != SYNCML_DM_SUCCESS)
        {
            XPL_DM_NotifySessionProgress(FALSE);
            return ret_stat;
        }    

        /* Check to see if we are passed the max retries */
        if (serverRetryCount > MAX_AUTH_RETRY || clientRetryCount > MAX_AUTH_RETRY)
        {
             XPL_DM_NotifySessionProgress(FALSE);
             return SYNCML_DM_SESSION_AUTH_FAIL;
        }    

        /* Check to see if any operational commands were parsed.*/
        if ( commandCount != 0 && m_bSessionAborted == FALSE && g_cancelSession == 0)
        {
            ret_stat = SendPackage();
            if (ret_stat != SYNCML_DM_SUCCESS)
            {
                XPL_DM_NotifySessionProgress(FALSE);
                return ret_stat;
            }    
        }
        else
        {
            /* End the DM Session.*/
            bMoreMessage = FALSE;

        }
    }    

    XPL_DM_NotifySessionProgress(FALSE);
    return SYNCML_DM_SUCCESS;
}


/*==================================================================================================
FUNCTION        : DMServerSession::SetURI

DESCRIPTION     : The UserAgent::SetURI will call this function if receiving package set the 
                  response URI to a new value.
                  The function will call Connection object to set the URI of the SyncML server
                  for HTTP/WSP.
ARGUMENT PASSED : p_RespURI
OUTPUT PARAMETER:
RETURN VALUE    : SYNCML_DM_SUCCESS if success, 
                  SYNCML_DM_FAIL if failed.
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMServerSession::SetURI (const char *p_RespURI)
{
    return m_oConnObject.SetURI(p_RespURI);
}



/*==================================================================================================
FUNCTION        : DMServerSession::RecvPackage

DESCRIPTION     : The UserAgent::TransportMsg will call this function when data is received
                  This function calls SYNCML_DM_Connection::Recv() to get the DM docuemnt from the 
                  Transport.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :


==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMServerSession::RecvPackage()
{
    SYNCML_DM_RET_STATUS_T      serverAuthStatus = SYNCML_DM_SUCCESS;
    SYNCMLDM_SEC_CREDENTIALS_T  *pGenHmac;
    UINT8                       decodedNonce[MAX_BIN_NONCE_LEN];
    UINT32                      encodedNonceLen =0;
    UINT32                      decodedNonceLen =0;
    SYNCML_DM_RET_STATUS_T      retStat = SYNCML_DM_SUCCESS;

   
    pWritePos = recvSmlDoc.pData;
            
    /* Unlock the workspace so the Toolkit controls it again. */
    smlUnlockReadBuffer(sendInstanceId, workspaceUsedSize);

    /* Check if the HMAC-MD5 headers were included.*/
    if( m_oRecvCredHeaders.empty() == FALSE )
    {
        //XPL_LOG_DM_SESS_Debug(("m_oRecvCredHeaders is not empty.\n"));
        //XPL_LOG_DM_SESS_Debug(("m_oRecvCredHeaders: username:%s\n", (CPCHAR)(m_oRecvCredHeaders.m_oUserName.getBuffer())));
        //XPL_LOG_DM_SESS_Debug(("m_oRecvCredHeaders: mac:%s\n", (CPCHAR)(m_oRecvCredHeaders.m_oMac.getBuffer())));
         
        SYNCMLDM_HMAC_SEC_INFO_T    hmacSecInfo;
        // since server sends hmac, use hmac after that
        clientServerCreds.SetPrefServerAuth(SYNCML_DM_CHAL_HMAC);
                
         /* Remember that the Cred Headers are here.*/
        SetServCredsMissing(FALSE);
               
        /* Copy the received DM document into the SyncML workspace */
        /* Create the Server Credentials.*/
        hmacSecInfo.pb_user_name_or_server_id = (UINT8*)clientServerCreds.pServerId.c_str();
        hmacSecInfo.pb_password = (UINT8*)clientServerCreds.pServerPW.c_str();
        hmacSecInfo.o_encode_base64 = TRUE;         /* Always true for HMAC */
        hmacSecInfo.pb_syncml_document = pWritePos; /* Pointer for SyncML Doc */
        hmacSecInfo.dw_syncml_document_length = recvSmlDoc.dataSize; /* Size of Doc*/
                    
         /* Validate the HMAC-MD5 header from the HTTP header.
          * We will check every message regardless of the Security state.
          * The ServerNonce string is b64 encoded and must be decoded now.*/
        if(clientServerCreds.pServerNonce != NULL) 
        {
            encodedNonceLen = DmStrlen((const char *)clientServerCreds.pServerNonce);
            decodedNonceLen = base64Decode((unsigned char *)decodedNonce,
                                            MAX_BIN_NONCE_LEN, 
                                            (unsigned char*)clientServerCreds.pServerNonce.c_str(),
                                            (unsigned long*)&encodedNonceLen); 
        }
                
         /* Generate the Server Credentials.*/
        hmacSecInfo.pb_nonce = decodedNonce;
        hmacSecInfo.w_nonce_length = decodedNonceLen;

        pGenHmac = syncmldm_sec_build_hmac_cred((const SYNCMLDM_HMAC_SEC_INFO_T *)&hmacSecInfo);

         /* Compare the created creds to the received creds.*/
        if ( pGenHmac == NULL || !m_oRecvCredHeaders.m_oMac.compare((CPCHAR)pGenHmac->ab_credential_string,
                                                                     pGenHmac->w_credential_string_length) )

        {
             serverAuthStatus = SYNCML_DM_UNAUTHORIZED;
             serverRetryCount++;
        }
        else
        {
             /* Reset count since server is now authenticated.*/
             serverRetryCount = 0;
        }
        DmFreeMem(pGenHmac);
                
         /* Update the security state with the server authentication status. Note that
          * we only check for cases that cause a change in the SecurityState.
          */
         switch (m_nSecState)
         {
            case DM_CLIENT_NO_SERVER_NO_AUTH:
                if (serverAuthStatus == SYNCML_DM_SUCCESS)
                    m_nSecState = DM_CLIENT_NO_SERVER_Y_AUTH;
                break;
                    
            case DM_CLIENT_Y_SERVER_NO_AUTH:
                if (serverAuthStatus == SYNCML_DM_SUCCESS)
                    m_nSecState = DM_BOTH_CLIENT_SERVER_AUTH;
                break;
                    
            case DM_CLIENT_NO_SERVER_Y_AUTH:
                if (serverAuthStatus != SYNCML_DM_SUCCESS)
                     m_nSecState = DM_CLIENT_NO_SERVER_NO_AUTH;
                break;
                    
            case DM_BOTH_CLIENT_SERVER_AUTH:
                if (serverAuthStatus != SYNCML_DM_SUCCESS)
                    m_nSecState = DM_CLIENT_Y_SERVER_NO_AUTH;
                break;
                    
            default:
                /* The Security State is messed up, so reset it.*/
                m_nSecState = DM_CLIENT_NO_SERVER_NO_AUTH;
                break;
         }
         /* Update the User Agent's security state.*/

    } /* p_cred_headers != NULL */
    else
    {
        /* The HMAC-MD5 creditials are missing.*/
        if ( !IsServerAuthorized() ) 
        {
            SetServCredsMissing(TRUE);
            serverRetryCount++;
        }
        else
            serverRetryCount = 0;
        
    }
           
    return(retStat);

}


/*==================================================================================================
FUNCTION        : DMServerSession::SendPackage

DESCRIPTION     : This function will be called when a DM package is built up and ready to send.
                  The function will call SYNCML_DM_Connection::Send() to send the DM document to
                  the remote server.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES : Both sendSmlDoc and pRecvSmlDoc must pass to pConnObject->Send. pConnObject will
                  write receiving DM document to the pRecvSmlDoc after it sends sendSmlDoc.


==================================================================================================*/
SYNCML_DM_RET_STATUS_T
DMServerSession::SendPackage()
{
    SYNCMLDM_HMAC_SEC_INFO_T       hmacSecInfo;
    SYNCMLDM_SEC_CREDENTIALS_T    *pHmacCreds = NULL;
    SYNCML_DM_RET_STATUS_T         ret_stat;
    UINT8                          decodedNonce[MAX_BIN_NONCE_LEN];
    UINT32                         encodedNonceLen = 0;
    UINT32                         decodedNonceLen = 0;
    Ret_t                          sml_ret_stat;

     /* Lock the workspace for reading and writing SyncML document.
     * These buffers will be unlocked after the UA receives SyncML document */
    smlLockReadBuffer(sendInstanceId, &pReadPos, &workspaceUsedSize);

    /* Set sendSmlDoc point to workspace */
    sendSmlDoc.dataSize = workspaceUsedSize;
    sendSmlDoc.pData = pReadPos;

    /* The ClientNonce string is b64 encoded and must be decoded now.*/
    if(clientServerCreds.pClientNonce != NULL) 
    {
        const char *clientNonce = clientServerCreds.pClientNonce.c_str();
        encodedNonceLen = DmStrlen(clientNonce);
        if (encodedNonceLen == 0) {
            clientNonce = SERVER_RESYNC_NONCE;
            encodedNonceLen = DmStrlen(clientNonce);
        }
        decodedNonceLen = base64Decode((UINT8*)decodedNonce,
                                       MAX_BIN_NONCE_LEN, 
                                       (unsigned char*)clientNonce,
                                       (unsigned long*)&encodedNonceLen); 
    }
    /* Let's make up our credentials before we send the package. */
    hmacSecInfo.pb_user_name_or_server_id = (UINT8*)clientServerCreds.pClientUserName.c_str();
    hmacSecInfo.pb_password = (UINT8*)clientServerCreds.pClientPW.c_str();
    hmacSecInfo.pb_nonce = decodedNonce;
    hmacSecInfo.pb_syncml_document = pReadPos; /* Used as the pointer to the SyncML Doc */
    hmacSecInfo.o_encode_base64 = TRUE;        /* Always true for HMAC credentials */
    hmacSecInfo.w_nonce_length = decodedNonceLen;
    hmacSecInfo.dw_syncml_document_length = workspaceUsedSize; /* Size of the SyncML Doc */

    if( clientServerCreds.AuthPrefCredType == SYNCML_DM_CHAL_HMAC )
	    pHmacCreds = syncmldm_sec_build_hmac_cred((const SYNCMLDM_HMAC_SEC_INFO_T *)&hmacSecInfo);

    m_oRecvCredHeaders.clear();
    if ( pHmacCreds != NULL )
    {
        if ( pHmacCreds->w_credential_string_length )
        {
            m_oRecvCredHeaders.m_oMac.assign(pHmacCreds->ab_credential_string,pHmacCreds->w_credential_string_length); 
            if ( m_oRecvCredHeaders.m_oMac.getBuffer() == NULL )
            {
                FreeAndSetNull(pHmacCreds);
                return SYNCML_DM_DEVICE_FULL;
            }
        }    

        m_oRecvCredHeaders.m_oAlgorithm.assign(SYNCML_MAC_ALG); 
        if ( m_oRecvCredHeaders.m_oAlgorithm.getBuffer() == NULL )
        {
            FreeAndSetNull(pHmacCreds);
            return SYNCML_DM_DEVICE_FULL;
        }

        if ( clientServerCreds.pClientUserName )
        {
            m_oRecvCredHeaders.m_oUserName.assign((CPCHAR)clientServerCreds.pClientUserName); 
            if ( m_oRecvCredHeaders.m_oUserName.getBuffer() == NULL )
            {
                FreeAndSetNull(pHmacCreds);
                return SYNCML_DM_DEVICE_FULL;
            }
        }    
    }    
    
    sml_ret_stat = smlLockWriteBuffer(recvInstanceId, &pWritePos, &workspaceFreeSize);

    if ( sml_ret_stat == SML_ERR_OK )
    {
        recvSmlDoc.pData = pWritePos;
        ret_stat = m_oConnObject.Send(&sendSmlDoc, &recvSmlDoc, smlContentType, &m_oRecvCredHeaders);
    }
    else
        ret_stat = SYNCML_DM_FAIL;
  
    FreeAndSetNull(pHmacCreds);
    return (ret_stat);
}


/*==================================================================================================
FUNCTION        : DMProcessScriptSession::SetServCredsMissing

DESCRIPTION     : This function reset class data members value to defaults after one DM session is
                  ended.
ARGUMENT PASSED : 
OUTPUT PARAMETER:
RETURN VALUE    : 
IMPORTANT NOTES :

==================================================================================================*/
void 
DMServerSession::SetServCredsMissing( BOOLEAN newIsServCredsMissing ) 
{
   isServCredsMissing = newIsServCredsMissing;
}
