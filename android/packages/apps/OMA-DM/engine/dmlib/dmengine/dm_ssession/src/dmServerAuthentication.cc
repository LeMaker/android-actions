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

    Source Name: dmServerAuthentication.cc

    General Description: Implementation of DMServerAuthentication class.

==================================================================================================*/

#include "dmt.hpp"
#include "dmStringUtil.h"
#include "dm_security.h"
#include "dm_tree_util.h"
#include "dmSessionDefs.h"
#include "SYNCML_DM_BuildPackage.H"
#include "xpl_dm_Manager.h"
#include "dmServerAuthentication.h"

extern "C" {
#include "xpt-b64.h"
#include "stdio.h"
}

void DMServerAuthentication::CheckCredentials( SYNCML_DM_AuthContext_T& AuthContext, const DMString& password, const DMBuffer& data, BOOLEAN bDecodeNonce )
{
  UINT8   decodedNonce[MAX_BIN_NONCE_LEN];

  XPL_LOG_DM_SESS_Debug(("CheckCredentials Entered\n"));

  memset(decodedNonce, 0, MAX_BIN_NONCE_LEN);

  UINT32 encodedNonceLen = data.getSize();
  
  UINT32 decodedNonceLen = base64Decode((unsigned char *)decodedNonce, MAX_BIN_NONCE_LEN,(unsigned char*)data.getBuffer(),(unsigned long*) &encodedNonceLen);  
  
  SYNCMLDM_HMAC_SEC_INFO_T    hmacSecInfo;

  memset(&hmacSecInfo,0,sizeof(hmacSecInfo));

  /* Call the security library to generate the credentials.*/

  hmacSecInfo.pb_user_name_or_server_id = (UINT8*)AuthContext._pServerId;
  hmacSecInfo.pb_password               = (UINT8*)password.c_str();
  
  if( bDecodeNonce )
  {
     hmacSecInfo.pb_nonce               = decodedNonce;
     hmacSecInfo.w_nonce_length         = decodedNonceLen;
  }
  else
  {
     hmacSecInfo.pb_nonce                = (unsigned char*)data.getBuffer();
     hmacSecInfo.w_nonce_length          = (unsigned long)data.getSize();
     
     if( hmacSecInfo.pb_nonce == NULL )
     {
         hmacSecInfo.pb_nonce = (UINT8*)"";
         hmacSecInfo.w_nonce_length = 0;
     }
  }

  hmacSecInfo.pb_syncml_document        = AuthContext._pTrigger; /* Pointer to the Trigger portion of the Pkg0.*/
  hmacSecInfo.dw_syncml_document_length = AuthContext._triggerLen;
  hmacSecInfo.o_encode_base64           = FALSE;  /* The MD5 digest is 16 bytes of binary, not b64.*/

  SYNCMLDM_SEC_CREDENTIALS_T *pGenCred = syncmldm_sec_build_hmac_cred(&hmacSecInfo);

 
  if( pGenCred != NULL )
  {
 
      /* Compare the newly generated Credentials to the ones passed by the server.*/
     char sdigest[2*pGenCred->w_credential_string_length+1];
     char cdigest[2*pGenCred->w_credential_string_length+1];
  
     for (int i=0; i<pGenCred->w_credential_string_length; i++ ) {
        sprintf(sdigest+2*i, "%02X", (char)AuthContext._md5Digest[i]);
     }

     for (int i=0; i<pGenCred->w_credential_string_length; i++ ) {
        sprintf(cdigest+2*i, "%02X", (char)pGenCred->ab_credential_string[i]);
     }

     sdigest[2*pGenCred->w_credential_string_length] = '\0';
     cdigest[2*pGenCred->w_credential_string_length] = '\0';
  
     XPL_LOG_DM_SESS_Debug(("Server Digest: %s\n", sdigest));
     XPL_LOG_DM_SESS_Debug(("Client Digest: %s\n", cdigest));
  
     AuthContext._AuthFlag = (pGenCred && (0 == memcmp((CPCHAR)AuthContext._md5Digest,(CPCHAR)pGenCred->ab_credential_string,pGenCred->w_credential_string_length )));
     if( !AuthContext._AuthFlag ) 
     {
        XPL_LOG_DM_SESS_Error(("CheckCredentials Failed\n"));
     }
  
     DmFreeMem(pGenCred); 
  }
  else
  {
     AuthContext._AuthFlag = FALSE;
  }

  XPL_LOG_DM_SESS_Debug(("CheckCredentials Exit\n"));
}


 DMString DMServerAuthentication::GetPreferredProfilePath( const DMString& strAccName, const DMMap<DMString, UINT32>& dmAuthProfiles )
{

  DMString  strAAuthPrefURI( ::XPL_DM_GetEnv( SYNCML_DM_DMACC_ROOT_PATH ) + DMString( DM_STR_SLASH ) + strAccName + DM_STR_SLASH + DM_AAUTHPREF );
  DMString  strPreferredProfilePath;
  DMGetData oAuthPref;

  XPL_LOG_DM_SESS_Debug(("GetPrefferdProfilePath Entered\n"));
  
  if( SYNCML_DM_SUCCESS == dmTreeObj.Get( strAAuthPrefURI, oAuthPref, SYNCML_DM_REQUEST_TYPE_INTERNAL ) )
  {
   DMString  strAuthPref( oAuthPref.getCharData() ); 

    if( strAuthPref.length() != 0 )
    {
      for( DMMap<DMString, UINT32>::POS pos = dmAuthProfiles.begin();pos != dmAuthProfiles.end(); ++pos )
      {
        DMGetData oAuthType;
        if( SYNCML_DM_SUCCESS == dmTreeObj.Get(dmAuthProfiles.get_key(pos)+DM_STR_SLASH DM_AAUTHTYPE,oAuthType,SYNCML_DM_REQUEST_TYPE_INTERNAL))
        {
          DMString strCurrentAuthType = oAuthType.getCharData();
           
          if( 0 == strCurrentAuthType.CompareNoCase( strAuthPref ) ) 
          {
            strPreferredProfilePath = dmAuthProfiles.get_key( pos );
            break;
          }
        }
      }
    }
  }

  return strPreferredProfilePath;
}

 SYNCML_DM_RET_STATUS_T DMServerAuthentication::TryProfile_1_1( const DMString& strAccName, const DMString& strProlilePath,SYNCML_DM_AuthContext_T& AuthContext )
{
       SYNCML_DM_RET_STATUS_T  result = SYNCML_DM_SUCCESS;


       DMGetData oAuthSecret;
       DMGetData oAuthData;

       XPL_LOG_DM_SESS_Debug(("TryProfile_1_1 Entered\n"));

       result = dmTreeObj.Get( strProlilePath + DM_STR_SLASH DM_SERVERPW, oAuthSecret, SYNCML_DM_REQUEST_TYPE_INTERNAL );
       if( SYNCML_DM_SUCCESS != result ) 
            return result;
       result = dmTreeObj.Get( strProlilePath + DM_STR_SLASH DM_SERVERNONCE, oAuthData, SYNCML_DM_REQUEST_TYPE_INTERNAL );
       if( SYNCML_DM_SUCCESS != result )
            return result;

       DMString device_id;
       result = SYNCML_DM_BuildPackage::GetDeviceID( device_id );
       if( SYNCML_DM_SUCCESS != result ) 
            return result;

       DMString password( oAuthSecret.getCharData() );

       result = SYNCML_DM_BuildPackage::GetServerAuthValues( device_id, AuthContext._pServerId, password );

       if( SYNCML_DM_SUCCESS != result ) 
            return result;

       CheckCredentials( AuthContext, password, oAuthData.m_oData, TRUE ); 

       return result;
}


SYNCML_DM_RET_STATUS_T DMServerAuthentication::TryProfile_1_2( const DMString& strAccName,const DMString& strProlilePath, SYNCML_DM_AuthContext_T&  AuthContext )
{
  SYNCML_DM_RET_STATUS_T  result = SYNCML_DM_SUCCESS;

  XPL_LOG_DM_SESS_Debug(("TryProfile_1_2 Entered\n"));
  for( ; ; )
  {
    DMGetData oAuthLevel;
    result = dmTreeObj.Get( strProlilePath + DM_STR_SLASH DM_AAUTHLEVEL, oAuthLevel, SYNCML_DM_REQUEST_TYPE_INTERNAL );
    if( SYNCML_DM_SUCCESS != result ) break;

    // DM: filter out server credentials only
    if( 0 != DMString( oAuthLevel.getCharData()).CompareNoCase( DM_AUTHLEVEL_SRVCRED ) ) break;

    DMGetData oAuthSecret;
    DMGetData oAuthData;

    result = dmTreeObj.Get( strProlilePath + DM_STR_SLASH DM_AAUTHSECRET, oAuthSecret, SYNCML_DM_REQUEST_TYPE_INTERNAL );
    if( SYNCML_DM_SUCCESS != result ) break;

    result = dmTreeObj.Get( strProlilePath + DM_STR_SLASH DM_AAUTHDATA, oAuthData, SYNCML_DM_REQUEST_TYPE_INTERNAL );
    if( SYNCML_DM_SUCCESS != result ) break;

    DMString device_id;
    result = SYNCML_DM_BuildPackage::GetDeviceID( device_id );
    if( SYNCML_DM_SUCCESS != result ) break;

    DMString password( oAuthSecret.getCharData() );

    result = SYNCML_DM_BuildPackage::GetServerAuthValues( device_id,AuthContext._pServerId, password );

    if( SYNCML_DM_SUCCESS != result ) break;

    CheckCredentials( AuthContext, password, oAuthData.m_oData, TRUE ); // decode nonce before calculating digest
    if ( !AuthContext._AuthFlag )
    {
       XPL_LOG_DM_SESS_Warn(("CheckCredentials: Failed check credentials with decoding\n"));
       
       CheckCredentials( AuthContext, password, oAuthData.m_oData, FALSE ); // do NOT decode nonce before calculating digest
       if ( !AuthContext._AuthFlag )
       {
          XPL_LOG_DM_SESS_Warn(("CheckCredentials: Failed check credentials without decoding\n"));
          
          oAuthData.m_oData.assign(SERVER_RESYNC_NONCE);
          BOOLEAN bAuthFlag1 = FALSE;
          BOOLEAN bAuthFlag2 = FALSE;
          
          CheckCredentials( AuthContext, password, oAuthData.m_oData, TRUE );
          bAuthFlag1 = AuthContext._AuthFlag;
          
          if( !bAuthFlag1 )
          {
             CheckCredentials( AuthContext, password, oAuthData.m_oData, FALSE );
             bAuthFlag2 = AuthContext._AuthFlag;
          }
          
          if ( bAuthFlag1 || bAuthFlag2 )
          {    
             XPL_LOG_DM_SESS_Warn(("Nonce Resynchronization request detected\n"));       
             DMNode *pNode = dmTreeObj.FindNodeByURI(strProlilePath + DM_STR_SLASH DM_AAUTHDATA);
             if ( NULL == pNode || NULL == pNode->getData() )
             {
                XPL_LOG_DM_SESS_Error(("Failed to reset server nonce!\n"));
                AuthContext._AuthFlag = false;
                result = SYNCML_DM_SESSION_AUTH_FAIL;
                break;
             }
             pNode->getData()->assign(SERVER_RESYNC_NONCE);
          } 
          else {
             result = SYNCML_DM_SESSION_AUTH_FAIL;
          }
       }
    }
    break;
  }  

  return result;
}


SYNCML_DM_RET_STATUS_T DMServerAuthentication::AuthenticateServer (SYNCML_DM_AuthContext_T& AuthContext)
{
  XPL_LOG_DM_SESS_Debug(("AutenticateServer Entered\n"));

  SYNCML_DM_RET_STATUS_T  result = SYNCML_DM_SUCCESS;
  CPCHAR szDMAccRootPath = ::XPL_DM_GetEnv( SYNCML_DM_DMACC_ROOT_PATH );
  CPCHAR szServerIdNodeName = ::XPL_DM_GetEnv( SYNCML_DM_NODENAME_SERVERID );

  /* Get the DMAcc node using the ServerId.*/
  DMString strAccName;

  AuthContext._AuthFlag = FALSE;

  if(!dmTreeObj.GetParentOfKeyValue( AuthContext._pServerId, szServerIdNodeName, szDMAccRootPath, strAccName ) )
  {
    return (SYNCML_DM_FAIL);
  }

  if( strAccName == NULL)
  {
    /* The ServerId was not found in the DM Tree.*/
    return (SYNCML_DM_FAIL);
  }

  if ( dmTreeObj.IsVersion_12()  ) 
  {
        DMString strAppAuthPath = szDMAccRootPath + DMString( DM_STR_SLASH ) + strAccName.c_str() + DM_STR_SLASH + DM_APPAUTH;
        DMMap<DMString, UINT32> dmAuthProfiles;

        result = dmTreeObj.getChildren( strAppAuthPath, dmAuthProfiles, DMTNM_NODE_INTERIOR,SYNCML_DM_REQUEST_TYPE_INTERNAL );
        if( SYNCML_DM_SUCCESS == result )
        {
            DMString strPreferredProfilePath = GetPreferredProfilePath( strAccName, dmAuthProfiles );

            if( strPreferredProfilePath.length() != 0 )
            {
                result = TryProfile_1_2( strAccName, strPreferredProfilePath, AuthContext );
                if ( SYNCML_DM_SUCCESS != result )
                {
                    dmFreeGetMap(dmAuthProfiles);
                    return result;
                }  
            }

            if( !AuthContext._AuthFlag )
            {
             // DM: preferred method didn't work. Try all profiles
                for( DMMap<DMString, UINT32>::POS pos = dmAuthProfiles.begin(); pos != dmAuthProfiles.end(); ++pos )
                {
                    DMString strProfilePath = dmAuthProfiles.get_key( pos );

                    // DM: skip preferred profile
                    if( strPreferredProfilePath != strProfilePath )
                    {
                        result = TryProfile_1_2( strAccName, strProfilePath, AuthContext );
                        if( ( SYNCML_DM_SUCCESS != result ) || AuthContext._AuthFlag ) break;      
                    }
                }
            }

            dmFreeGetMap(dmAuthProfiles);
        }      
    }
    else
    {
        result = TryProfile_1_1( strAccName, szDMAccRootPath + DMString( DM_STR_SLASH ) + strAccName.c_str(),AuthContext );
    }

    return result;
}
