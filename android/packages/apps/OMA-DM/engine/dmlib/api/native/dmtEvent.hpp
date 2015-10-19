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

#ifndef __DMTEVENT_H__
#define __DMTEVENT_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/** 
  \file dmtEvent.hpp
  \brief The dmtEvent.hpp header file contains DmtEventSubscription class  definition. \n
       This class represents a subscription on DMT update event. 
*/

#include "dmtDefs.h"
#include "dmstring.h"
#include "dmvector.h"
#include "dmtPrincipal.hpp"

/**Topic of DM event about start of server session */
#define DMT_EVENT_OTA_SESSION_STARTED  "/motorola/dmt/ota/sessionstart"
/**Topic of DM event about finish of server session */
#define DMT_EVENT_OTA_SESSION_FINISHED "/motorola/dmt/ota/sessionend"
/**Topic prefix of DM event about updates in the Screen3 sub tree */
#define DMT_UPDATE_S3_PREFIX  "/motorola/dmt/S3/"
/**Topic prefix of generic of DM update event */
#define DMT_UPDATE_GENERIC_PREFIX  "/motorola/dmt/update/"

/**Generic name for OTA principals */
#define DM_SERVER_PRINCIPAL "OTA"



/**
* DmtEventSubscription represents subscription on DMT update event. 
* \par Category: General  
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DmtEventSubscription {

public:

  /**
    * Default Constructor - no memory allocation performed.
    */   
  DmtEventSubscription() 
  {
    m_eAction = SYNCML_DM_EVENT_NONE;
    m_nType = SYNCML_DM_EVENT_NODE;
  }


   /** 
  * Sets up subscription.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param eAction [in] - actions to track, Bit-wised
  * \param nType [in] - event type
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. \n
  * - All other codes indicate failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  
  SYNCML_DM_RET_STATUS_T Set(SYNCML_DM_EVENT_ACTION_T eAction,
                                                SYNCML_DM_EVENT_TYPE_T nType);


  /** 
  * Sets up subscription.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param eAction [in] - actions to track, Bit-wised
  * \param nType [in] - event type
  * \param szTopic [in] - event topic suffix ( may be NULL )
  * \param aIgnorePrincipals [in] - vector of principals, which initiated updates are to be ignored
  * \param aNotifyPrincipals [in] - vector of principals, which initiated updates are to be processed
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. \n
  * - All other codes indicate failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  
  SYNCML_DM_RET_STATUS_T Set(SYNCML_DM_EVENT_ACTION_T eAction,
                                                 SYNCML_DM_EVENT_TYPE_T nType,
                                                 CPCHAR szTopic,
                                                 const DMVector<DmtPrincipal> & aIgnorePrincipals,
                                                 const DMVector<DmtPrincipal> & aNotifyPrincipals);

   /** 
  * Sets up topic suffix for DMT update event.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szTopic [in] - event topic suffix
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. \n
  * - All other codes indicate failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  SYNCML_DM_RET_STATUS_T SetTopic(CPCHAR szTopic);
    
  /** 
  * Adds principal into internal vector.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param oPrincipal [in] - principal 
  * \param bIsIgnore [in] - specifies type of principal \n
  *   - TRUE - principal to ignore \n
  *   - FALSE - principal to process updates 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. \n
  * - All other codes indicate failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   
  SYNCML_DM_RET_STATUS_T AddPrincipal(const DmtPrincipal & oPrincipal, BOOLEAN bIsIgnore);

   /** 
  * Retrieves operations (actions) to be tracked on node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (SYNCML_DM_EVENT_ACTION_T)\n
  * - Bit-wised actions to be tracked by DM engine
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  SYNCML_DM_EVENT_ACTION_T GetAction() const { return m_eAction; }

   /** 
  * Retrieves subscription type.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (SYNCML_DM_EVENT_TYPE_T) - subscription type\n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  SYNCML_DM_EVENT_TYPE_T GetType() const { return m_nType; }
  
  /** 
  * Retrieves topic suffix for DMT update event.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return topic suffix
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */

  const DMString & GetTopic() const { return m_strTopic; }

  /** 
  * Retrieves vector of principals.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param bIsIgnore [in] - specifies type of principal \n
  *   - TRUE - principal to ignore \n
  *   - FALSE - principal to process updates 
  * \return vector of principals
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  const DMVector<DmtPrincipal> & GetPrincipals(BOOLEAN bIsIgnore) const; 

protected:  
 /** Event Action*/
  SYNCML_DM_EVENT_ACTION_T m_eAction;
 /** Event Type*/
  SYNCML_DM_EVENT_TYPE_T m_nType;
 /**Topic*/
  DMString m_strTopic;
 /** Collection with principals, which initiated updates are to be ignored*/
  DMVector<DmtPrincipal> m_aIgnorePrincipals;
 /** Collection with principals, which initiated updates are to be processed*/
  DMVector<DmtPrincipal> m_aNotifyPrincipals;

};  

#endif
