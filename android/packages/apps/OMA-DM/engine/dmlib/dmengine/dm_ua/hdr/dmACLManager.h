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

#ifndef DMACLMANAGER_H
#define DMACLMANAGER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

Header Name: dmACLManager.h 

General Description: This file contains declaration of the dmACLManager class, which provides 
                     interfaces to read from the ACL information file and enforce ACL.

==================================================================================================*/

#include "dmACLItem.h"
#include "dmConfigManager.h"

/**
 * DMAclManager handles ACL records management.
 */
class DMAclManager : public DMConfigManager {
public:

  /**
  * Default constructor
  */
  DMAclManager() {}

   /**
  * Initializes a manager  
  * \param env [in] - pointer on env object
  * \param pTree [in] - pointer on DM tree object
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Init( CEnv* env, DMTree* tree );

   /**
  * Checks if operation is allowed for particular server  
  * \param szPath [in] - DM node path
  * \param szServerID [in] - server name
  * \param nPermissions [ in] - permission to check
  * \param bIsCheckLocal [ in] - specifies if local status of node should be checked
  * \return TRUE if permitted 
  */
  BOOLEAN IsPermitted(CPCHAR szPath, 
                      CPCHAR szServerID, 
                      SYNCML_DM_ACL_PERMISSIONS_T nPermissions,
                      BOOLEAN bIsCheckLocal);

  /**
  * Retrieves ACL property as a string for particular node 
  * \param szPath [in] - DM node path
  * \param strACL [out] - ACL property
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T GetACL(CPCHAR szPath, 
                                DMString& strACL);

  /**
  * Sets ACL property for particular node 
  * \param szPath [in] - DM node path
  * \param strACL [out] - ACL property
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T SetACL(CPCHAR szPath, 
                                CPCHAR szACL );
  
private:
   /**
  * Returns name of configuration file ("acl.dat")
  * \param strFileName [out] - file name
  */ 
  virtual void GetFileName(DMString & strFileName);

  /**
  * Allocates new acl item
  * \return pointer on new config item 
  */ 
  virtual DMConfigItem * AllocateConfigItem();

  /**
  * Checks if operation is allowed for particular server  
  * \param szPath [in] - DM node path
  * \param szServerID [in] - server name
  * \param nPermissions [ in] - permission to check
  * \return TRUE if permitted 
  */
  BOOLEAN IsPermitted(CPCHAR szPath, 
                   CPCHAR szServerID,
                   SYNCML_DM_ACL_PERMISSIONS_T nPermissions) const;
};

#endif 
