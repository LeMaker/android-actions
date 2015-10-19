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

#ifndef DMEVENTSUBSCRIPTION_H
#define DMEVENTSUBSCRIPTION_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

Header Name: dmEventSubscription.h

General Description: This file contains declaration declaration 
                            of DMEventSubscription class used for notification storing/handling

==================================================================================================*/

#include "dmstring.h"
#include "dmvector.h"
#include "dmtEvent.hpp"

class DMTree;

/**
* DMEventSubscription represents an DMT event subscription.
* Base class for DMSubscriptionItem and commit plug-in.
*/
class DMEventSubscription : public DmtEventSubscription {

public:

  /**
  * Default constructor
  */
  DMEventSubscription() {}

  /**
  * Parses encoded event subscription record from sysplugin.ini file and sets values of object members
  * \param szEvent [in] - encoded event subscription record
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T Set(CPCHAR szEvent); 

  /**
  * Verifies if particular operation/action should generate an event 
  * \param nAction [in] - action to check
  * \param pTree [in] - pointer on tree object to access to DM accounts (verify OTA principals)
  * \return TRUE if enabled 
  */  
  BOOLEAN IsEnabled(SYNCML_DM_EVENT_ACTION_T nAction, 
                    DMTree* pTree);


private:     

  /**
  * Verifies if particular principal that initiated update is found in the list of principals
  * \param pTree [in] - pointer on tree object to access to DM accounts (verify OTA principals)
  * \param aPrincipals [in] - vector of principals
  * \return TRUE if found 
  */  
  BOOLEAN VerifyPrincipal(DMTree * pTree,
                          const DMVector<DmtPrincipal> & aPrincipals);

   /**
  * Parses  "key=value" segment of event subscription record from sysplugin.ini file 
  * \param szSegment [in] - segment of event subscription record 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T ParseSegment(CPCHAR szSegment);

  /**
  * Parses "+" separated list of principals
  * \param szSegment [in] - list of principals
  * \param bIgnore [in] - TRUE if list principals to ignore is parsed 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T ParsePrincipal(CPCHAR szSegment,
                                        BOOLEAN bIsIgnore);
  
};

#endif 
