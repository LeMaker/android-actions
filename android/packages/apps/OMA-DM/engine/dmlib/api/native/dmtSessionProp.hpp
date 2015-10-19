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

#ifndef __DMTSESSIONPROP_H__
#define __DMTSESSIONPROP_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/**  
 \file dmtSessionProp.hpp
 \brief Contains DMFirmAlertVector and  DmtSessionProp classes  definition.\n
       The dmtSessionProp.hpp header file contains DMFirmAlertVector and  DmtSessionProp\n
       classes  definition. \n
       <b>Warning:</b>  All functions, structures, and classes from this header file are for internal usage only!!!\n

       The <b>DMFirmAlertVector</b> is a helper class; due to gcc limitations, declaring separate \n
       class instead of typedef  produces smaller binary.\n
       The <b>DmtSessionProp</b> is a structure with parameters for server session
*/

#include "jem_defs.hpp"
#include "dmtDefs.h"
#include "dmtFirmAlert.hpp"
#include "dmvector.h"

/**
* Helper class; due to gcc limitations, declaring separate class instead of typedef
* produces smaller binary
* \warning This class is using <b>ONLY</b> for firmware update session!
* \par Category: General  
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DMFirmAlertVector : public DMVector<DmtFirmAlert>
{
};


/**
* This class is a structure with parameters for the server session 
* \par Category: General  
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DmtSessionProp
{ 
private:
  UINT16 m_nSessionID;            // session ID
  SYNCML_DM_SESSION_DIRECTION_T m_nDirection; // session direction
  BOOLEAN m_bWBXML;       

  DMFirmAlertVector m_aFirmAlerts; // result of firmware update


public:
 /**
  * Default constructor - no memory allocation performed.
  */
 
  DmtSessionProp()
  {
    m_nSessionID = 0;
    m_nDirection = SYNCML_DM_CLIENT_INITIATED_SESSION;
    m_bWBXML = true;
  }

  /**
  * Constructs (copy) a DMT Session Property object. The memory for this object  will be allocated.
  * \param oCopyFrom [in] - session property that should be copied
  */
  DmtSessionProp( const DmtSessionProp& oCopyFrom ) 
  {
     m_nSessionID = oCopyFrom.m_nSessionID;
     m_nDirection = oCopyFrom.m_nDirection;
     m_bWBXML = oCopyFrom.m_bWBXML;
     if ( oCopyFrom.m_aFirmAlerts.size() )
        m_aFirmAlerts =  oCopyFrom.m_aFirmAlerts;
  }
  
  /**
  * Constructs required fields for server initiated session. The memory for these fields  will be allocated.
  * \param sessionID [in] - provided by Syncml DM server in a notification package
  * \param bWBXML [in] - true to use binary xml
  */
  DmtSessionProp(UINT16 sessionID, BOOLEAN bWBXML)
  {
     m_nSessionID = sessionID;
     m_nDirection = SYNCML_DM_SERVER_INITIATED_SESSION;
     m_bWBXML = bWBXML;
  }

  /**
  * Constructs session property - no memory allocation performed.
  * \param bWBXML [in] -true to use binary xml
  */
  DmtSessionProp(BOOLEAN bWBXML)
  {
     m_nSessionID = 0;
     m_nDirection = SYNCML_DM_CLIENT_INITIATED_SESSION;
     m_bWBXML = bWBXML;
  }


  /**
  * Constructs session property for firmware. 
  * \param aFirmAlert [in] - firm alert to send to the Syncml DM server
  * \param bWBXML [in] - true to use binary xml
  */
  DmtSessionProp(const DmtFirmAlert & aFirmAlert, BOOLEAN bWBXML)
  {
     m_nSessionID = 0;
     m_nDirection = SYNCML_DM_CLIENT_INITIATED_SESSION;
     m_bWBXML = bWBXML;
     m_aFirmAlerts.push_back(aFirmAlert);
  }
  
 /**
  * Retrieves session ID
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \returns session ID as an unsigned short.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 UINT16 getSessionID() const
 {
    return m_nSessionID;
 }

 /**
  * Retrieves direction
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \returns session direction as an  unsigned short.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 SYNCML_DM_SESSION_DIRECTION_T getDirection() const
 {
    return m_nDirection;
 }

 /**
  * Sets session ID
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param sessionID [in] - session ID as an unsigned short.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 void setSessionID(UINT16 sessionID)
 {
    m_nSessionID = sessionID;
    m_nDirection = SYNCML_DM_SERVER_INITIATED_SESSION;
 }   

 /**
  * Sets session direction
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param direction [in] - session direction as an unsigned short.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 void setDirection(SYNCML_DM_SESSION_DIRECTION_T direction)
 {
    m_nDirection = direction;
 }   

/**
  * Generates session ID
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 void generateSessionID();
    
  /**
  * Sets session binary XML flag
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param bWBXML [in] - true to use binary xml
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */

 void setWBXML(BOOLEAN bWBXML)
 {
    m_bWBXML = bWBXML;
 }
 
 /**
  * Retrieves binary XML flag
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \returns binary XML flag as a boolean.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 BOOLEAN isWBXML() const
 {
    return m_bWBXML;
 }


 /**
  * Retrieves firmware alert 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param aFirmAlert [out] - firmware update result code in DMFirmAlertVector.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 void getFirmAlerts(DMFirmAlertVector & aFirmAlert) 
 {
    aFirmAlert = m_aFirmAlerts;
 }


 /**
  * Adds firmware update result
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param aFirmAlert [in] - reference to a DmtFirmAlert object
  * \return index
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
 INT32 addFirmAlert(const DmtFirmAlert & aFirmAlert) 
 {
   m_aFirmAlerts.push_back(aFirmAlert);
   return m_aFirmAlerts.size();
 }
 
};

#endif
