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

/*
 *  DESCRIPTION:
 *      Implementation for class DMClientServerCreds.
 */

#include "dmClientSrvCreds.h"
#include "SYNCML_DM_BuildPackage.H"
#include "xpl_dm_Manager.h"

//------------------------------------------------------------------------
// FUNCTION        : constructor for class DMClientServerCreds
// DESCRIPTION     : This function inits integer members
//
// ARGUMENTS PASSED: none
//                   
// RETURN VALUE    : none
// PRE-CONDITIONS  : 
// POST-CONDITIONS : none
// IMPORTANT NOTES :
// REQUIREMENT #   :
//------------------------------------------------------------------------
DMClientServerCreds::DMClientServerCreds()
{
  LOGE("DMClientServerCreds::DMClientServerCreds()");
  ServerChalType = AuthPrefCredType = SYNCML_DM_CHAL_UNDEFINED;
  m_bAuthPrefUpdated = FALSE;
  
}

//------------------------------------------------------------------------
// FUNCTION        : LoadInitialValues
// DESCRIPTION     : Loads initial values for client credentials based on
//                    "AuthPref" node value or "Ext/LastAuthPref"
//
// ARGUMENTS PASSED: none
//                   
// RETURN VALUE    : error code in case of failure, "SUCCESS" otherwise
// PRE-CONDITIONS  : 
// POST-CONDITIONS : none
// IMPORTANT NOTES :
// REQUIREMENT #   :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T  DMClientServerCreds::LoadInitialValues()
{
  LOGE("DMClientServerCreds::LoadInitialValues()");
  // restore last used auth type for this server
  m_bAuthPrefUpdated = FALSE;
  
  DMGetData               oData;
  DMString                strLastAuthPref;
  SYNCML_DM_RET_STATUS_T  dm_stat = SYNCML_DM_FAIL;

  if ( dmTreeObj.IsVersion_12() )
  {
  	dm_stat = dmTreeObj.GetLastClientAuthType(pDMAccNodeName, oData );
    LOGE("dmTreeObj.GetLastClientAuthType() returned %d\n", dm_stat);
  }

  if ( dm_stat == SYNCML_DM_SUCCESS  )
  {
    strLastAuthPref = oData.getCharData();
  LOGE("strLastAuthPref from GetLastClientAuthType = %s\n", strLastAuthPref.c_str());
  }
  else
  {     // try to get aauthpref if "last" is not available
    
    DMString strUri = ::XPL_DM_GetEnv( SYNCML_DM_DMACC_ROOT_PATH ) + 
                      DMString( DM_STR_SLASH ) +
                      pDMAccNodeName + 
                      (  dmTreeObj.IsVersion_12()
            	            ? DM_STR_SLASH DM_AAUTHPREF
            	            : DM_STR_SLASH DM_AUTHPREF );

    dm_stat = dmTreeObj.Get(strUri, oData, SYNCML_DM_REQUEST_TYPE_INTERNAL);
    
    if ( dm_stat == SYNCML_DM_SUCCESS )
    {
      strLastAuthPref = oData.getCharData();
  LOGE("strLastAuthPref from dmTreeObj.Get(strUri...) = %s\n", strLastAuthPref.c_str());
    }
  }
  LOGE("strLastAuthPref = %s\n", strLastAuthPref.c_str());

  if( 0 == strLastAuthPref.length() )
  {
    strLastAuthPref = GetDefaultAuthType();
  LOGE("strLastAuthPref from GetDefaultAuthType() = %s\n", strLastAuthPref.c_str());
    dm_stat = SYNCML_DM_SUCCESS;
  }

  if ( dm_stat == SYNCML_DM_SUCCESS )
  {
    // load settings from tree
    dm_stat = SetClientAuth( strLastAuthPref, TRUE);
  LOGE("SetClientAuth(strLastAuthPref, TRUE) returns %d\n", dm_stat);
  }  

  // server part is loaded whenever server credentials required
  return dm_stat;
}

//------------------------------------------------------------------------
// FUNCTION        : SetPrefClientAuth
// DESCRIPTION     : Sets preferred AuthType for client and
//                    reloads username/password/nonce from the tree
//
// ARGUMENTS PASSED: nClientAuthType - new auth type
//                   
// RETURN VALUE    : error code in case of failure, "SUCCESS" otherwise
// PRE-CONDITIONS  : 
// POST-CONDITIONS : none
// IMPORTANT NOTES : 
// REQUIREMENT #   :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T  DMClientServerCreds::SetPrefClientAuth( SYNCML_DM_CHAL_TYPE_T nClientAuthType )
{
  LOGE("DMClientServCreds::SetPrefClientAuth(%d): AuthPrefCredType == %d", nClientAuthType, AuthPrefCredType);
  if ( nClientAuthType == AuthPrefCredType )
    return SYNCML_DM_SUCCESS;

  LOGE("ChalType2Str(%d) = %s", nClientAuthType, ChalType2Str(nClientAuthType));
  return SetClientAuth( ChalType2Str(nClientAuthType), FALSE);
}

//------------------------------------------------------------------------
// FUNCTION        : SetPrefServerAuth
// DESCRIPTION     : Sets preferred AuthType for server and
//                    reloads username/password/nonce from the DMTR
//
// ARGUMENTS PASSED: nServerAuthType - new auth type
//                   
// RETURN VALUE    : error code in case of failure, "SUCCESS" otherwise
// PRE-CONDITIONS  : 
// POST-CONDITIONS : none
// IMPORTANT NOTES : 
// REQUIREMENT #   :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T  DMClientServerCreds::SetPrefServerAuth( SYNCML_DM_CHAL_TYPE_T nServerAuthType )
{
  LOGE("DMClientServCreds::SetPrefServerAuth(%d): ServerChalType == %d", nServerAuthType, ServerChalType);
 	SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

	if ( ServerChalType == nServerAuthType )
            return dm_stat;

  	if ( nServerAuthType == SYNCML_DM_CHAL_NONE )
  	{
      	    ServerChalType = nServerAuthType;
      	    return dm_stat;
  	}

    	// load server settings
    	DMGetData oAuthSecret;
    	DMGetData oAuthData;
    	DMGetData oAuthType;
    	DMGetData oAuthName;

       if ( dmTreeObj.IsVersion_12()  )
       {
	       for (int index = nServerAuthType; index <= SYNCML_DM_CHAL_HMAC; index++)
	       {
LOGE("DM 1.2 tree, index = %d", index);
		    	dm_stat = dmTreeObj.GetServerAuthInfo(pDMAccNodeName,
		       	                                   ChalType2Str(index),
		              	                            oAuthName,
		                     	                     oAuthSecret,
		                            	              oAuthData,
		                                   	       m_strServerProfileURI ); 
		    
			//  profile found
			if( dm_stat == SYNCML_DM_SUCCESS ) 
			{
			        nServerAuthType = index;
LOGE("DM 1.2 tree, nServerAuthType = %d", index);
				break;
		       }	
	       }		
      }
      else
      {
	  	dm_stat = dmTreeObj.GetServerAuthInfo(pDMAccNodeName,
		       	                                   ChalType2Str(nServerAuthType),
		              	                            oAuthName,
		                     	                     oAuthSecret,
		                            	              oAuthData,
		                                   	       m_strServerProfileURI ); 
      	}		

      if( dm_stat != SYNCML_DM_SUCCESS ) 
      	    return dm_stat;
		
    	ServerChalType = nServerAuthType;
    
    	DMString device_id;
    	dm_stat = GetDeviceID( device_id );
    	if( dm_stat != SYNCML_DM_SUCCESS )
    		return dm_stat;
LOGE("device_id = %s", device_id.c_str());

    	pServerPW.attach( (const char*)oAuthSecret.m_oData.detach() );
LOGE("pServerPW = %s", pServerPW.c_str());

    	dm_stat = SYNCML_DM_BuildPackage::GetServerAuthValues( device_id,
                                                           pServerId,
                                                           pServerPW );
    	if( dm_stat != SYNCML_DM_SUCCESS )
    		return dm_stat;

    	// DM: assume it is base64 encoded value
    	pServerNonce.attach( (const char*)oAuthData.m_oData.detach() );
LOGE("pServerNonce = %s", pServerNonce.c_str());

    	if ( dmTreeObj.IsVersion_12() )
        	dm_stat = SaveServerAttribute(DM_AAUTHSECRET, pServerPW );
    	else
		      dm_stat = SaveServerAttribute(DM_SERVERPW, pServerPW);

	return dm_stat;
}

//------------------------------------------------------------------------
// FUNCTION        : Str2ChalType
// DESCRIPTION     : Converts string AuthPref representation into integer
//                    SYNCML_DM_CHAL_TYPE_T enumeration
//
// ARGUMENTS PASSED: szChal - string auth type
//                   
// RETURN VALUE    : integer auth pref, "none" for unknown types
// PRE-CONDITIONS  : 
// POST-CONDITIONS : none
// IMPORTANT NOTES : 
// REQUIREMENT #   :
//------------------------------------------------------------------------
SYNCML_DM_CHAL_TYPE_T DMClientServerCreds::Str2ChalType( CPCHAR szChal )
{
  if ( !szChal )
    return SYNCML_DM_CHAL_UNDEFINED;

  if ( dmTreeObj.IsVersion_12()  )
  {
	  if ( DmStrcmp(szChal, DM_AUTHTYPE_BASIC ) == 0 )
	    return SYNCML_DM_CHAL_BASIC;
	  
	  if ( DmStrcmp(szChal, DM_AUTHTYPE_DIGEST ) == 0 )
	    return SYNCML_DM_CHAL_MD5;
	  
	  if ( DmStrcmp(szChal, DM_AUTHTYPE_HMAC ) == 0 )
	    return SYNCML_DM_CHAL_HMAC;
  }
  else
  {
	 if ( DmStrcmp(szChal, SYNCML_AUTH_BASIC ) == 0 )
	    return SYNCML_DM_CHAL_BASIC;
	  
	  if ( DmStrcmp(szChal, SYNCML_AUTH_MD5 ) == 0 )
	    return SYNCML_DM_CHAL_MD5;
	  
	  if ( DmStrcmp(szChal, SYNCML_AUTH_MAC ) == 0 )
	    return SYNCML_DM_CHAL_HMAC;
  }

  
  // all other types requires transport-level auth  
  return SYNCML_DM_CHAL_NONE;
}

//------------------------------------------------------------------------
// FUNCTION        : ChalType2Str
// DESCRIPTION     : Converts integer AuthPref representation into string
//                    
//
// ARGUMENTS PASSED: nChal - SYNCML_DM_CHAL_TYPE_T enumeration 
//                    auth representation
//                   
// RETURN VALUE    : string auth pref, empty string for "none",
//                    NULL for unknown types
// PRE-CONDITIONS  : 
// POST-CONDITIONS : none
// IMPORTANT NOTES : 
// REQUIREMENT #   :
//------------------------------------------------------------------------
CPCHAR DMClientServerCreds::ChalType2Str( SYNCML_DM_CHAL_TYPE_T nChal )
{

  if ( dmTreeObj.IsVersion_12()  )
  {
	   switch ( nChal ) 
	 {
	    case SYNCML_DM_CHAL_BASIC:
	      return DM_AUTHTYPE_BASIC;

	    case SYNCML_DM_CHAL_MD5:
	      return DM_AUTHTYPE_DIGEST;

	    case SYNCML_DM_CHAL_HMAC:
	      return DM_AUTHTYPE_HMAC;

	    case SYNCML_DM_CHAL_NONE:
	      return "";
	 }
  }
  else
  {
	 switch ( nChal ) 
	  {
	     case SYNCML_DM_CHAL_BASIC:
	       return SYNCML_AUTH_BASIC;

	     case SYNCML_DM_CHAL_MD5:
	       return SYNCML_AUTH_MD5;

	     case SYNCML_DM_CHAL_HMAC:
	       return SYNCML_AUTH_MAC;

	     case SYNCML_DM_CHAL_NONE:
	       return "";
	  }	
  }	  
  
  return NULL;
}


DMString DMClientServerCreds::GetDefaultAuthType()
{
  CPCHAR  szStr = ::XPL_DM_GetEnv(SYNCML_DM_SECURITY_LEVEL);

  return ChalType2Str( szStr ? DmAtoi( szStr ) :  SYNCML_DM_CHAL_BASIC );
}
//------------------------------------------------------------------------
// FUNCTION        : SetClientAuth
// DESCRIPTION     : Sets preferred AuthType for client and
//                    reloads username/password/nonce from the DMTR
//
// ARGUMENTS PASSED: szLastAuthPref - new auth type,
//                   bTryDefault - if true, tries "default" one if 
//                    preferred one missing
//                   
// RETURN VALUE    : error code in case of failure, "SUCCESS" otherwise
// PRE-CONDITIONS  : 
// POST-CONDITIONS : none
// IMPORTANT NOTES : 
// REQUIREMENT #   :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T  DMClientServerCreds::SetClientAuth( CPCHAR szLastAuthPref, BOOLEAN bTryDefault )
{
  LOGE("DMClientServCreds::SetClientAuth(\"%s\", %d)", szLastAuthPref, bTryDefault);
  // load client settings
  DMGetData               oAuthSecret;
  DMGetData               oAuthData; 
  DMGetData               oAuthType;
  DMGetData               oAuthName;

  
  SYNCML_DM_RET_STATUS_T  dm_stat = SYNCML_DM_FAIL;

  if ( dmTreeObj.IsVersion_12()  )
  {
	  dm_stat = dmTreeObj.GetClientAuthInfo(pDMAccNodeName,
                                          szLastAuthPref,
                                          oAuthName,
                                          oAuthSecret,
                                          oAuthData,
                                          m_strClientProfileURI,
                                          oAuthType ); 

    if( dm_stat != SYNCML_DM_SUCCESS && 
        szLastAuthPref && 
        bTryDefault )
    {
  LOGE("DMClientServCreds::SetClientAuth() loading default settings");
      // try to load default settings if last one failed
      szLastAuthPref = NULL;

      dm_stat = dmTreeObj.GetClientAuthInfo(pDMAccNodeName,
                                            NULL,
                                            oAuthName,
                                            oAuthSecret,
                                            oAuthData,
                                            m_strClientProfileURI,
                                            oAuthType ); 
    }

    // no profile found
    if (dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;
      
    if ( !szLastAuthPref )
    {
      szLastAuthPref = oAuthType.getCharData();
  LOGE("szLastAuthPref is now %s", szLastAuthPref);
    }
  }
  else
  {
    // DM: there is only one set of auth params in 1.1.2, so just load default settings
    dm_stat = dmTreeObj.GetClientAuthInfo(pDMAccNodeName,
                                          NULL,
                                          oAuthName,
                                          oAuthSecret,
                                          oAuthData,
                                          m_strClientProfileURI,
                                          oAuthType ); 
    // no profile found
    if (dm_stat != SYNCML_DM_SUCCESS )
      return dm_stat;
      
    if ( !szLastAuthPref )
    {
      szLastAuthPref = oAuthType.getCharData();
    }
  }

  if ( AuthPrefCredType != SYNCML_DM_CHAL_UNDEFINED )
  {
    m_bAuthPrefUpdated = TRUE;
  }
  
  AuthPrefCredType = Str2ChalType( szLastAuthPref );

  DMString device_id;
  dm_stat = GetDeviceID( device_id );
LOGE("device_id = %s", device_id.c_str());

  if( SYNCML_DM_SUCCESS != dm_stat  ) return dm_stat;

  pClientUserName= oAuthName.getCharData();
  pClientPW=oAuthSecret.getCharData();
LOGE("pClientUserName = %s", pClientUserName.c_str());
LOGE("pClientPW = %s", pClientPW.c_str());
 
  dm_stat = SYNCML_DM_BuildPackage::GetClientAuthValues( device_id,
                                                         pServerId,
                                                         pClientUserName,
                                                         pClientPW );

  if( SYNCML_DM_SUCCESS != dm_stat  ) return dm_stat;

  pClientNonce=oAuthData.getCharData();
LOGE("pClientNonce = %s", pClientNonce.c_str());
 
  // Motorola, <e50324>, <12/08/09>, <ikmap-2156> / Nonce Resynchronization
  if ( dmTreeObj.IsVersion_12()  )
  {
     dm_stat = dmTreeObj.GetServerAuthInfo(pDMAccNodeName,
                                           oAuthType.getCharData(),
                                           oAuthName,
                                           oAuthSecret,
                                           oAuthData,
                                           m_strServerProfileURI );

     if( dm_stat == SYNCML_DM_SUCCESS && NULL != oAuthData.getCharData() )
     {
LOGE("comparing %s to %s", oAuthData.getCharData(), SERVER_RESYNC_NONCE);
        if ( 0 == memcmp(oAuthData.getCharData(), SERVER_RESYNC_NONCE, 8) )
           pClientNonce = SERVER_RESYNC_NONCE;
     }
  }

  /* change the tree node value*/ 
  if ( dmTreeObj.IsVersion_12()  )
  {
LOGE("changing tree value of DM_AAUTHNAME to %s", pClientUserName.c_str());
	  dm_stat = SaveClientAttribute(DM_AAUTHNAME, pClientUserName);
	  if ( dm_stat != SYNCML_DM_SUCCESS )
	      return dm_stat;

LOGE("changing tree value of DM_AAUTHSECRET to %s", pClientPW.c_str());
	  dm_stat = SaveClientAttribute(DM_AAUTHSECRET, pClientPW);
	  if ( dm_stat != SYNCML_DM_SUCCESS )
	      return dm_stat;
   }
  else
  {
         dm_stat = SaveClientAttribute(DM_USERNAME, pClientUserName);
	 if ( dm_stat != SYNCML_DM_SUCCESS )
	      return dm_stat;

	 dm_stat = SaveClientAttribute(DM_CLIENTPW, pClientPW);
	 if ( dm_stat != SYNCML_DM_SUCCESS )
	      return dm_stat;
  }

  return SYNCML_DM_SUCCESS;
}

//------------------------------------------------------------------------
// FUNCTION        : GetDeviceID
// DESCRIPTION     : returns device ID; retrieves it only once and cache
//                    the value; mainly used for Factory bootstrap
//
// ARGUMENTS PASSED: none
//                   
// RETURN VALUE    : device id; empty string in case of error
// PRE-CONDITIONS  : 
// POST-CONDITIONS : none
// IMPORTANT NOTES : 
// REQUIREMENT #   :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T 
DMClientServerCreds::GetDeviceID( DMString& device_id )
{
  SYNCML_DM_RET_STATUS_T result = SYNCML_DM_SUCCESS;

  if ( m_strdevIMEI.empty() )
  {
    result = SYNCML_DM_BuildPackage::GetDeviceID( device_id );

    if( SYNCML_DM_SUCCESS == result )
    {
      m_strdevIMEI = device_id;
    }
  }
  else
     device_id = m_strdevIMEI;  

  return result;
}


//------------------------------------------------------------------------
// FUNCTION        : SaveClientAttribute
// DESCRIPTION     : saves client attribute in node 
//                    "./SyncML/DMAcc/<x>/AppAuth/<x>/<attribute>" in DMTR
//                    
//
// ARGUMENTS PASSED:  szAttribute - attribute name like "AAuthData"
//                    szValue - new value
//                   
// RETURN VALUE    : error code in case of failure, "SUCCESS" otherwise
// PRE-CONDITIONS  : 
// POST-CONDITIONS : none
// IMPORTANT NOTES : 
// REQUIREMENT #   :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T  DMClientServerCreds::SaveClientAttribute( CPCHAR szAttribute, CPCHAR szValue )
{
  return SaveAttribute(m_strClientProfileURI, szAttribute, szValue);
}

//------------------------------------------------------------------------
// FUNCTION        : SaveServerAttribute
// DESCRIPTION     : saves server attribute in node 
//                    "./SyncML/DMAcc/<x>/AppAuth/<x>/<attribute>" in DMTR
//                    
//
// ARGUMENTS PASSED:  szAttribute - attribute name like "AAuthData"
//                    szValue - new value
//                   
// RETURN VALUE    : error code in case of failure, "SUCCESS" otherwise
// PRE-CONDITIONS  : 
// POST-CONDITIONS : none
// IMPORTANT NOTES : 
// REQUIREMENT #   :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T  DMClientServerCreds::SaveServerAttribute( CPCHAR szAttribute, CPCHAR szValue )
{
  return SaveAttribute(m_strServerProfileURI, szAttribute, szValue);
}

//------------------------------------------------------------------------
// FUNCTION        : SaveAttribute
// DESCRIPTION     : saves an attribute in node 
//                    "<profile>/<attribute>" in DMTR
//                    
//
// ARGUMENTS PASSED:  szProfile - "profile" name; cached value for last loaded
//                      server or client account
//                    szAttribute - attribute name like "AAuthData"
//                    szValue - new value
//                   
// RETURN VALUE    : error code in case of failure, "SUCCESS" otherwise
// PRE-CONDITIONS  : 
// POST-CONDITIONS : none
// IMPORTANT NOTES : 
// REQUIREMENT #   :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T  DMClientServerCreds::SaveAttribute( CPCHAR szProfile, CPCHAR szAttribute, CPCHAR szValue )
{
    SYNCML_DM_RET_STATUS_T  dm_stat = SYNCML_DM_SUCCESS;
    DMAddData  oReplace;
    DMString strURI = szProfile;

    strURI += szAttribute;

    dm_stat = oReplace.set(strURI, SYNCML_DM_FORMAT_CHR, szValue,DmStrlen(szValue),"text/plain"); 

    if ( dm_stat == SYNCML_DM_SUCCESS )
        dm_stat = dmTreeObj.Replace(oReplace,SYNCML_DM_REQUEST_TYPE_INTERNAL);
    
    return (dm_stat);
}

//------------------------------------------------------------------------
// FUNCTION        : SaveAuthPref
// DESCRIPTION     : saves AuthPref if different from initial value 
//
// ARGUMENTS PASSED:  none
//                   
// RETURN VALUE    : error code in case of failure, "SUCCESS" otherwise
// PRE-CONDITIONS  : 
// POST-CONDITIONS : none
// IMPORTANT NOTES : 
// REQUIREMENT #   :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T  DMClientServerCreds::SaveAuthPref()
{
  if ( !m_bAuthPrefUpdated || dmTreeObj.IsVersion_12()  == FALSE )
    return SYNCML_DM_SUCCESS;

  m_bAuthPrefUpdated = FALSE;
  return dmTreeObj.SetLastClientAuthType(pDMAccNodeName, ChalType2Str(AuthPrefCredType));
}
