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

#ifndef DMSERVERSESSION_H
#define DMSERVERSESSION_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: DMServerSession.h

    General Description: This file contains declaration of DMServerSession class.
       The class provides services to management DM sessions. It receives the 
       SYNCML DM packages from the Transport Binding module, starts the DM session, processes the 
       received DM package, builds the response DM package and sends it to the Transport Binding
       module, and ends the DM session.

==================================================================================================*/

#include "dmtSessionProp.hpp"
#include "dm_tpt_connection.H"
#include "dmProcessScriptSession.h"

/*==================================================================================================
                                    CONSTANTS
==================================================================================================*/

/* According to spec, we should only have two attempts to access the server */
#define MAX_AUTH_RETRY            2

/*==================================================================================================
                                 CLASSES DECLARATION
==================================================================================================*/

class DMServerSession : public DMProcessScriptSession
{
  public:

     /* Class constructor and destructor. */ 
     DMServerSession();

  
     /* The UserAgent calls this function to start the DM session. */
     virtual SYNCML_DM_RET_STATUS_T Start(CPCHAR pServerID, 
                                          DmtSessionProp * pSessionProp);

     /* UserAgent object call this function to set the response URI */
     virtual SYNCML_DM_RET_STATUS_T SetURI (CPCHAR pRespURI);

     virtual BOOLEAN IsProcessScript() { return FALSE; }

        
  protected:
    void SetServCredsMissing( BOOLEAN newIsServCredsMissing );

    /* This function will be called when a DM package is built up and ready to send. */
    SYNCML_DM_RET_STATUS_T SendPackage();

    SYNCML_DM_RET_STATUS_T  RecvPackage();
       
    /* This function will establish the DM session connection with the server. */
    SYNCML_DM_RET_STATUS_T ConnectServer(CPCHAR pServerId);

    /* This function will build and send the package one to the Server. */
    SYNCML_DM_RET_STATUS_T BuildSendPackageOne(CPCHAR pServerID, DmtSessionProp * pSessionProp);
      
      
    SYNCML_DM_OTAConnection m_oConnObject;  /* Connection object */

 };

/*================================================================================================*/
#endif /* SYNCML_DM_MGMTSESSION_H */

