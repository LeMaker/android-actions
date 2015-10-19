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

#ifndef DMSUBSCRIPTIONITEM_H
#define DMSUBSCRIPTIONITEM_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

Header Name: dmSubscriptionItem.h

General Description: This file contains declaration declaration 
                            of DMSubscriptionItem class used for notification storing/handling

==================================================================================================*/

#include "dmstring.h"
#include "dmConfigItem.h"
#include "dmEventSubscription.h"

/**
* DMSubscriptionItem represents an DMT event subscription record.
*/
class DMSubscriptionItem : public DMConfigItem, public DMEventSubscription {

public:

   /**
  * Default constructor
  */
  DMSubscriptionItem() {}

 /**
  * Constructor, which sets subscription path 
  * \param szPath [in] - configuration path (URI of a node)
  */
  DMSubscriptionItem(CPCHAR szPath);

/**
  * Assign values of object members 
  * \param oEvent [in] - subscription event set by application to add into config file
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  virtual SYNCML_DM_RET_STATUS_T Set(const DmtEventSubscription & oEvent);

/**
  * Parses encoded event subscription record and assign values of object members 
  * \param szPath [in] - configuration path (URI of a node)
  * \param szConfig[in] - encoded acl record
  * \param aDict [in] - dictionary of principals
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  virtual SYNCML_DM_RET_STATUS_T Set(CPCHAR szPath,
                                CPCHAR szConfig, 
                                const DMMap<INT32, DMString>& aDict);


/**
  * Saves event subscription record back to a file 
  * \param dmf [in] - file handler object
  * \param aDict [in] - dictionary of principals
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  virtual SYNCML_DM_RET_STATUS_T Serialize(DMFileHandler& dmf,
                                          const DMMap<DMString, INT32>& aDict );  

   /**
  * Adds principals stored in the acl item into dictionary before serialization of config file 
  * \param aDict [out] - dictionary of principals
  */  
  virtual void UpdateDictionary(DMMap<DMString, INT32>& aDict);

private:     

  /**
  * Parses  "key=value" segment of event subscription record 
  * \param szSegment [in] - segment of event subscription record 
  * \param aDict [in] - dictionary of principals
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T ParseSegment(CPCHAR szSegment,
                                      const DMMap<INT32, DMString>& aDict);

/**
  * Parses "+" separated list of principals (value)
  * \param szSegment [in] - list of principals
  * \param nPermission [in] - permission 
  * \param aDict [in] - dictionary of principals
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T ParsePrincipal(CPCHAR szSegment,
                                        BOOLEAN bIsIgnore,
                                        const DMMap<INT32, DMString>& aDict); 

  
};

#endif 
