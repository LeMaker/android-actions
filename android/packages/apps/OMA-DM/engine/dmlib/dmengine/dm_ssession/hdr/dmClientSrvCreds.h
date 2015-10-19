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
 *      The dmClientSrvCreds.h header file provides declaration 
 *      for class DMClientServerCreds
 */

#ifndef __DMCLIENT_SRVCREDS_H__
#define __DMCLIENT_SRVCREDS_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "dmSessionDefs.h"

class DMClientServerCreds
{
public:
  DMClientServerCreds();

  SYNCML_DM_RET_STATUS_T        LoadInitialValues();
  SYNCML_DM_RET_STATUS_T        SetPrefClientAuth( SYNCML_DM_CHAL_TYPE_T nClientAuthType );
  SYNCML_DM_RET_STATUS_T        SetPrefServerAuth( SYNCML_DM_CHAL_TYPE_T nServerAuthType );

  SYNCML_DM_RET_STATUS_T        SaveClientAttribute( CPCHAR szAttribute, CPCHAR szValue );
  SYNCML_DM_RET_STATUS_T        SaveServerAttribute( CPCHAR szAttribute, CPCHAR szValue );
  SYNCML_DM_RET_STATUS_T        SaveAttribute( CPCHAR szProfile, CPCHAR szAttribute, CPCHAR szValue );
  SYNCML_DM_RET_STATUS_T        SaveAuthPref();
  
  static SYNCML_DM_RET_STATUS_T DMGetDeviceID( DMString& device_id );
  static DMString               GetDefaultAuthType();

  // implementation
private:
  static SYNCML_DM_CHAL_TYPE_T  Str2ChalType( CPCHAR szChal );
  static CPCHAR                 ChalType2Str( SYNCML_DM_CHAL_TYPE_T nChal );
  SYNCML_DM_RET_STATUS_T        SetClientAuth( CPCHAR szLastAuthPref, BOOLEAN bTryDefault );
  SYNCML_DM_RET_STATUS_T        GetDeviceID( DMString& device_id );
  
  // data
public:
  // to minimize number of changes names for data members are left as in DM 1.1
  // and members are declared public
  DMString                      pDMAccNodeName;   // Profile name for Current ServerID
  DMString                      pClientUserName;    // AAuthName for client profile
  DMString                      pClientPW;                // AAuthSecret for client profile
  DMString                      pClientNonce;           // AAuthData for client profile
  DMString                      pServerId;                // ServerID
  DMString                      pServerPW;              // AAuthSecret for server profile
  DMString                      pServerNonce;         // AAuthData for server profile
  SYNCML_DM_CHAL_TYPE_T         AuthPrefCredType; // used to choose right client credentials
  SYNCML_DM_CHAL_TYPE_T         ServerChalType;    // used to choose server credentials

private:
  DMString                      m_strClientProfileURI;
  DMString                      m_strServerProfileURI;
  DMString                      m_strdevIMEI;
  BOOLEAN                       m_bAuthPrefUpdated;
};

#endif  /*__DMCLIENT_SRVCREDS_H__*/
