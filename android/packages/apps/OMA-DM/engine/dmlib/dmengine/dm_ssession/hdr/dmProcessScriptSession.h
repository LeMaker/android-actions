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

#ifndef DMPROCESSSCRIPTSESSION_H
#define DMPROCESSSCRIPTSESSION_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: dmProcessScriptSession.h

    General Description: This file contains declaration of DMProcessScriptSession calls.

==================================================================================================*/

#include "dmSession.h"
#include "SYNCML_DM_BuildPackage.H"
#include "dm_security.h"
#include "dmSessionDefs.h"
#include "dmClientSrvCreds.h"

/*==================================================================================================
                                      CONSTANTS
==================================================================================================*/

/*==================================================================================================
                                 CLASSES DECLARATION
==================================================================================================*/

class DMProcessScriptSession : public DMSession
{

  public:

    DMProcessScriptSession();

    virtual ~DMProcessScriptSession();


      /* set response URI */
    virtual SYNCML_DM_RET_STATUS_T SetURI (CPCHAR szURI) { return SYNCML_DM_SUCCESS; }

    void IncCommandCount ();
        
    /* Get and Set the inAtomicCommand.*/
    void SetInAtomicCommand (BOOLEAN newInAtomicCommand);
        
    BOOLEAN GetInAtomicCommand ();
         
    UINT16 GetServerSessionId ();
        
    /* Get the sendInstanceId member.*/
    InstanceID_t GetSendInstanceId();
        
    void SetClientRetryCount(UINT8 newcount);
        
    void IncClientRetryCount();

    void SetSessionAborted() { m_bSessionAborted = TRUE; }

    inline BOOLEAN IsSessionAborted() { return m_bSessionAborted; }
        
    inline void  ResetServerRetryCount() {serverRetryCount = 0;}

    void SetSecState( SYNCML_DM_SEC_STATE_FLAG_T ua_SecState ) { m_nSecState = ua_SecState; }

    SYNCML_DM_SEC_STATE_FLAG_T GetSecState() { return m_nSecState; }

    inline void SetSecStateSrv( BOOLEAN bAuthorized )
    {
        if ( bAuthorized ) 
           m_nSecState |= DM_CLIENT_NO_SERVER_Y_AUTH;
        else
           m_nSecState &= (~DM_CLIENT_NO_SERVER_Y_AUTH);
    }

    inline BOOLEAN   IsServerAuthorized() const
    {
        return (m_nSecState & DM_CLIENT_NO_SERVER_Y_AUTH) != 0;
    }


    inline BOOLEAN IsAuthorized() const
    {
        return ( m_nSecState == DM_BOTH_CLIENT_SERVER_AUTH ||
                      m_nSecState == DM_CLIENT_NO_SERVER_Y_AUTH );

    }	
   
    inline SYNCML_DM_RET_STATUS_T GetNotAuthorizedStatus() 
   {
       return (  isServCredsMissing ?  SYNCML_DM_AUTHENTICATION_REQUIRED : SYNCML_DM_UNAUTHORIZED);
   }	   

     /* Get the clientServerCreds structure.*/
    virtual DMClientServerCreds * GetClientServerCreds();
     
 
    virtual SYNCML_DM_RET_STATUS_T Start(const UINT8 *docInputBuffer , 
                                         UINT32 inDocSize, 
                                         BOOLEAN isWBXML, 
                                         DMBuffer & oResult);


    virtual BOOLEAN IsProcessScript() { return TRUE; }
#ifdef LOB_SUPPORT
    BOOLEAN IsLargeObjectSupported() const{ return isLargeObjectSupported; };
    UINT32 GetDefaultMaxObjectSize() const{ return dmtMaxObjectSize; };
#endif

#ifdef TNDS_SUPPORT
    BOOLEAN IsWBXMLEncoding() const{ return smlEncodingType == SML_WBXML; };
#endif


 protected:
    /* This function will be called to register the DM Engine with the SyncML Toolkit. */
    virtual SYNCML_DM_RET_STATUS_T SetToolkitCallbacks(SmlCallbacks_t * pSmlCallbacks);

    /* This function will be called when a new DM package is received. */
    SYNCML_DM_RET_STATUS_T ParseMessage();


    BOOLEAN   inAtomicCommand;  /* Flag for atomic command */

    UINT16    commandCount; /* Keep track of the management commands.*/
                                                
    UINT8     serverRetryCount; /* Keep track of server auth failures.*/ 
    
    UINT8     clientRetryCount; /* Keep track of client auth failures.*/
    
    UINT16    serverSessionId;   /* Holds the Server's SessionID */   
    
    BOOLEAN   m_bSessionAborted;

    BOOLEAN   isServCredsMissing;

    SYNCML_DM_SEC_STATE_FLAG_T  m_nSecState;  /* DMUA Security state */

    SYNCML_DM_USER_DATA_T   userData;  /* Structure that contains data shared between our
                                          code and the callbacks. */
    DMCredHeaders m_oRecvCredHeaders;

    DMClientServerCreds clientServerCreds; /* Holds the Security Info for this
                                                      session.*/

    SYNCML_DM_BuildPackage m_oPkgBuilder;
#ifdef LOB_SUPPORT
    BOOLEAN isLargeObjectSupported;
    UINT32 	  dmtMaxObjectSize;		/* Maximum object size can be handled */
#endif   
};

#endif /* DMProcessScriptSession */
