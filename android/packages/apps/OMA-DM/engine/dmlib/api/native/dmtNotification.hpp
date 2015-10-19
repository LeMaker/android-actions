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

#ifndef __DMTNOTIFICATION_H__
#define __DMTNOTIFICATION_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/**  
 \file dmtNotification.hpp
 \brief  The dmtNotification.hpp header file contains DmtNotification class definition.
           This class processes and parses "package0" information that is coming from server.
           <b>Warning:</b>  All functions, structures, and classes from this header file are for internal usage only!!!
*/

#include "jem_defs.hpp"
#include "dmtDefs.h"

/**
 * This class processes and parses <i>"package0"</i> information that is coming from a server.
 * All parsed data will be stored.
 * \par Category: General
 * \par Persistence: Transient
 * \par Security: Non-Secure
 * \par Migration State: FINAL
 */
class DmtNotification
{ 
private:
  UINT8  m_nUIMode;
  UINT8  m_nInitiator;
  UINT16 m_nSessionID;
  DMString m_sServerID; 
  BOOLEAN m_bAuthFlag;

public:
 /**
  * Default constructor - no memory allocation performed.
  */
 
  DmtNotification()
  {
    m_nSessionID = 0;
    m_nUIMode = 0;
    m_nInitiator = 0;;
  }

  
 /**
  * Retrieves session ID
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return session ID as unsigned short.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
 UINT16 getSessionID() const
 {
    return m_nSessionID;
 }

 /**
  * Retrieves UI mode
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return UI mode as unsigned byte.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
 UINT8 getUIMode() const
 {
    return m_nUIMode;
 }


 /**
  * Retrieves initiator
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return session initiator as unsigned byte.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
 UINT8 getInitiator() const
 {
    return m_nInitiator;
 }


 /**
  * Retrieves authentication flag
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return authentication flag as a boolean.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
 BOOLEAN getAuthFlag() const
 {
    return m_bAuthFlag;
 }


 /**
  * Retrieves server ID
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return server ID as a string.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
 const DMString& getServerID() const
 {
    return m_sServerID;
 }

/**
   * Sets value for session ID 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param sessionID [in] - session ID  as an unsigned integer
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
 void setSessionID(UINT16 sessionID)
 {
    m_nSessionID = sessionID;
 }   


/**
   * Sets UI mode value
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param uiMode  [in] - UI Mode  as an unsigned integer
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
 void setUIMode(UINT8 uiMode)
 {
    m_nUIMode = uiMode;
 }   


/**
  * Sets Session initiator ID
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param initiator  [in] - session initiator as an unsigned integer
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
 void setInitiator(UINT8 initiator)
 {
    m_nInitiator = initiator;
 }   


/**
  * Sets authentication flag
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param authFlag [in] - authentication flag as a boolean
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
 void setAuthFlag(BOOLEAN authFlag)
 {
    m_bAuthFlag = authFlag;
 } 

/**
  * Sets server ID
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param serverID [in] - server ID as a string
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
 void setServerID(CPCHAR serverID)
 {
    m_sServerID = serverID;
 }   
 
};

#endif
