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

#ifndef DMACLITEM_H
#define DMACLITEM_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

Header Name: dmACLItem.h

General Description: This file contains declaration of the dmACLItem class

==================================================================================================*/

#include "dmstring.h"
#include "dmvector.h"
#include "syncml_dm_data_types.h"
#include "dmtAcl.hpp"
#include "dmConfigItem.h"

/**
 * DMAclItem represents an acl config record.
 */
class DMAclItem : public DmtAcl, public DMConfigItem {
public:

 /**
  * Default constructor
  */
  DMAclItem() {}

   /**
  * Constructor, which sets path of configuration item and parses encoded acl record
  * \param szPath [in] - configuration path (URI of a node)
  * \param szAcl [in] - encoded acl record
  */
  DMAclItem(CPCHAR szPath, CPCHAR szAcl );

 /**
  * Parses encoded acl record and assign values of object members 
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
  * Saves acl record back to a file 
  * \param dmf [in] - file handler object
  * \param aDict [in] - dictionary of principals
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  virtual SYNCML_DM_RET_STATUS_T Serialize( DMFileHandler& dmf,
                                           const DMMap<DMString, INT32>& aDict );  

 /**
  * Adds principals stored in the acl item into dictionary before serialization of config file 
  * \param aDict [out] - dictionary of principals
  */  
  virtual void UpdateDictionary( DMMap<DMString, INT32>& aDict);


private:

/**
  * Parses  "key=value" segment of acl property 
  * (where key is a permission and value is a  "+" separated list of principals indexes )
  * \param szSegment [in] - segment of acl config record
  * \param aDict [in] - dictionary of principals
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T  ParseSegment(CPCHAR szSegment,
                                       const DMMap<INT32, DMString>& aDict); 

/**
  * Parses list of principals indexes (value)
  * \param szSegment [in] - list of principals
  * \param nPermission [in] - permission 
  * \param aDict [in] - dictionary of principals
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T ParsePrincipal(CPCHAR szSegment,
                                        SYNCML_DM_ACL_PERMISSIONS_T nPermission,
                                        const DMMap<INT32, DMString>& aDict );

/**
  * Parses permission 
  * \param szKeyWord [in] - key letter that specifies a permission ( A/G/R/D )
  * \param pPermission [out] - permission 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  static SYNCML_DM_RET_STATUS_T ParsePermission(CPCHAR szkeyWord, 
                                          SYNCML_DM_ACL_PERMISSIONS_T * pPermission );
 

};  

#endif 
