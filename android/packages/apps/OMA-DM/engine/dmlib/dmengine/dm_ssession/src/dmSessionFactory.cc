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

    Source Name: dmSessionFactory.cc

    General Description: Implementation of Internal interfaces to Server Session.

==================================================================================================*/

#include "dmtSessionProp.hpp"
#include "dmMemory.h"
#include "dmSessionFactory.h"
#include "xpl_Time.h"
#include "xpl_Logger.h"
#include "dmProcessScriptSession.h"
#include "dmBootstrapSession.h"
#include "dmServerSession.h"
#include "dmServerAuthentication.h"

/*
 * The instance data of the application
 */

INT32 g_iDMWorkspaceSize = DEFAULT_DM_SMLTK_WORKSPACE_SIZE;


#ifdef __cplusplus
extern "C" {
#endif

void DmSetWorkspaceSize(UINT32 inDocSize)
{
  if (inDocSize <= DEFAULT_DM_SMLTK_WORKSPACE_SIZE) {
      g_iDMWorkspaceSize = DEFAULT_DM_SMLTK_WORKSPACE_SIZE;
  } else {
      g_iDMWorkspaceSize = ((inDocSize / 1000) + 1 ) * 1000;
  }
}

SYNCML_DM_RET_STATUS_T  DmProcessServerDataInternal(CPCHAR szPrincipal, const DmtSessionProp& sessionProp)
{
    SYNCML_DM_RET_STATUS_T retStat;
  
    g_iDMWorkspaceSize = DEFAULT_DM_SMLTK_WORKSPACE_SIZE;

    DMServerSession * pSessionObj = new DMServerSession();
    
    if ( pSessionObj == NULL )
    {
        XPL_LOG_DM_SESS_Error(("SYNCML_DM_UserAgent::SessionStart : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL;
    }

    retStat = pSessionObj->Start(szPrincipal,(DmtSessionProp*)&sessionProp);
 
    delete pSessionObj;
    XPL_LOG_DM_SESS_Debug(("Returning from DmProcessServerDataInternal, status=%d\n", retStat));
    return retStat;
}

SYNCML_DM_RET_STATUS_T  DmProcessScriptDataInternal(const UINT8 *docInputBuffer , 
                                                          UINT32 inDocSize, 
                                                          BOOLEAN isWBXML, 
                                                          DMBuffer & oResult)
{

  SYNCML_DM_RET_STATUS_T retStat;  

  DmSetWorkspaceSize(inDocSize);

  DMProcessScriptSession * pSessionObj = new DMProcessScriptSession();
  
  if ( pSessionObj == NULL )
  {
      XPL_LOG_DM_SESS_Error(("SYNCML_DM_UserAgent::SessionStart : unable allocate memory\n"));
      return SYNCML_DM_DEVICE_FULL;
  }

  retStat = pSessionObj->Start(docInputBuffer,inDocSize,isWBXML,oResult);

  delete pSessionObj;

  return retStat;
}


SYNCML_DM_RET_STATUS_T  DmBootstrapInternal(const UINT8 *docInputBuffer , 
                                                UINT32 inDocSize, 
                                                BOOLEAN isWBXML, 
                                                BOOLEAN isProcess,
                                                DMString & serverID)
{
    SYNCML_DM_RET_STATUS_T retStat;  

    DmSetWorkspaceSize(inDocSize);

    if ( isProcess )
    {
        DMBuffer oResult; 
        DMProcessScriptSession * pSessionObj = new DMProcessScriptSession();
  
        if ( pSessionObj == NULL )
        {
            XPL_LOG_DM_SESS_Error(("SYNCML_DM_UserAgent::SessionStart : unable allocate memory\n"));
            return SYNCML_DM_DEVICE_FULL;
        }

        retStat = pSessionObj->Start(docInputBuffer,inDocSize,isWBXML,oResult);
       
        delete pSessionObj;
    }
    else
    {
        DMBootstrapSession oSession;
       
        retStat = oSession.Start(docInputBuffer,inDocSize,isWBXML,serverID);
    }
    return retStat;
}

SYNCML_DM_RET_STATUS_T DmAuthenticateServerInternal(SYNCML_DM_AuthContext_T& AuthContext )
{

	DMServerAuthentication oAuth;

	return oAuth.AuthenticateServer(AuthContext);
}

#ifdef __cplusplus
}
#endif
