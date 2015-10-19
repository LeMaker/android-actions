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

#ifndef DMSUBSCRIPTIONMANAGER_H
#define DMSUBSCRIPTIONMANAGER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

Header Name: dmSubscriptionManager.h

General Description: This file contains declaration declaration 
                            of DMSubscriptionManager class used for notification storing/handling

==================================================================================================*/

#include "dmSubscriptionItem.h"
#include "dmConfigManager.h"

class DMTree;
class CEnv;
class SyncML_DM_Archive;

/**
* DMSubscriptionManager represents a class DM event subscription records management.
* This class operates with subscriptions specified in the config file and subscriptions specified 
* by commit plug-ins 
*/
class DMSubscriptionManager : public DMConfigManager {

public:
  /**
  * Default constructor
  */
  DMSubscriptionManager() {}

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
  * Verifies if operation on DM node  should be stored into Event Logger  
  * \param pArchive [in] - pointer on archive where node is persisted
  * \param szPath [in] - path of updated node (Add/Replace/Rename/Indirect)
  * \param nAction [in] - operation performed on node
  * \param szNewName [in] - new node name in case of "Rename" operation
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T OnNodeChanged(SyncML_DM_Archive * pArchive, 
                                     CPCHAR szPath,
                                     SYNCML_DM_EVENT_ACTION_T nAction,
                                     CPCHAR szNewName = NULL );

  /**
  * Verifies if "Delete" operation on DM node  should be stored into Event Logger  
  * \param pArchive [in] - pointer on archive where node is persisted
  * \param szPath [in] - path of  deleted node
  * \param aDeletedChildren [in] - children of deleted node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T OnNodeDeleted(SyncML_DM_Archive * pArchive,
                                       CPCHAR szPath,
                                       const DMStringVector & aDeletedChildren );


  /**
  * Adds event subscription record into config file. Called from DMT API  
  * \param szPath [in] - DM node path
  * \param oEvent [in] - DM event subscription
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T EnableEvent(CPCHAR szPath, 
                                     const DmtEventSubscription & oEvent);

  /**
  * Retrieves DM event subscription 
  * \param szPath [in] - DM node path
  * \param oEvent [out] - DM event subscription
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T GetEvent(CPCHAR szPath, 
                                  DmtEventSubscription & oEvent); 
  
private:
  /**
  * Returns name of configuration file ("event.dat")
  * \param strFileName [out] - file name
  */ 
  virtual void GetFileName(DMString & strFileName);

  /**
  * Allocates new event subscription item
  * \return pointer on new config item 
  */ 
  virtual DMConfigItem * AllocateConfigItem();

  /**
  * Verifies if event is enabled on parent node by record from config file or commit plug-in
  * \param strPath [out] - DM node path - set to parent path if event is Cumulative
  * \param szParent [in] - path of parent node
  * \param nAction [in] - action performed on a node
  * \param pItem [in] - pointer on event subscription of parent node
  * \return TRUE if enabled 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  BOOLEAN IsParentEnabled( DMString & strPath, 
                           CPCHAR szParent,
                           SYNCML_DM_EVENT_ACTION_T nAction,
                           DMEventSubscription * pItem) const; 

   /**
   * Verifies if event is enabled on parent node by event subscription record 
  * \param strPath [out] - DM node path to be logged - set to parent path if event is Cumulative
  * \param szParent [in] - path of parent node
  * \param nAction [in] - action performed on a node
  * \param pItem [in] - smart pointer on event subscription of parent node
  * \return TRUE if enabled 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  BOOLEAN IsParentEnabledBySubscription(DMString & strPath, 
                                        CPCHAR szParent,
                                        SYNCML_DM_EVENT_ACTION_T nAction,
                                        PDMConfigItem & pItem) const; 

   /**
   * Verifies if event is enabled by commit plug-in mounted on parent node 
  * \param strPath [out] - DM node path to be logged - set to parent path if event is Cumulative
  * \param szParent [in] - path of parent node
  * \param nAction [in] - action performed on a node
  * \param pPlugin [in] - smart pointer on commit plug-in
  * \return TRUE if enabled 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  BOOLEAN IsParentEnabledByPlugin(DMString & strPath, 
                                  CPCHAR szParent,
                                  SYNCML_DM_EVENT_ACTION_T nAction,
                                  PDMPlugin & pPlugin) const; 

   /**
   * Verifies if event is enabled by event subscription record   
  * \param strPath [in/out] - DM node path to be logged - set to parent path if event is Cumulative
  * \param nAction [in] - action performed on a node
  * \param pItem [out] - smart pointer on event subscription
  * \param bIsEnabledByParent [out] - specifies if action is enabled by parent node (Cumulative or Detailed)
  * \param bIsChild [in] - specifies if child node is checked (used for "Delete" operation)
  * \return TRUE if enabled 
  */
 BOOLEAN IsEnabledBySubscription(DMString & strPath, 
                                 SYNCML_DM_EVENT_ACTION_T nAction,
                                 PDMConfigItem & pItem,
                                 BOOLEAN & bIsEnabledByParent,
                                 BOOLEAN bIsChild) const; 

   /**
   * Verifies if event is enabled by commit plug-in. 
  * \param strPath [in/out] - DM node path to be logged - set to parent path if event is Cumulative
  * \param nAction [in] - action performed on a node
  * \param pPlugin [out] - smart pointer on commit plug-in
  * \param bIsEnabledByParent [out] - specifies if action is enabled by parent node (Cumulative or Detailed)
  * \param bIsChild [in] - specifies if child node is checked (used for "Delete" operation)
  * \return TRUE if enabled 
  */
 BOOLEAN IsEnabledByPlugin(DMString & strPath, 
                           SYNCML_DM_EVENT_ACTION_T nAction,
                           PDMPlugin & pPlugin,
                           BOOLEAN & bIsEnabledByParent,
                           BOOLEAN bIsChild) const; 
                                    
 /**
  * Verifies if event is enabled by commit plug-in. Checks node path and node MDF path  if they are 
  * different
  * \param strPath [in/out] - DM node path to be logged - set to parent path if event is Cumulative
  * \param nAction [in] - action performed on a node
  * \param pPlugin [out] - smart pointer on commit plig-in
  * \param bIsEnabledByParent [out] - specifies if action is enabled by parent node (Cumulative or Detailed)
  * \param bIsChild [in] - specifies if child node is checked (used for "Delete" operation)
  * \return TRUE if enabled 
  */
BOOLEAN IsEnabled(DMString & strPath, 
                  SYNCML_DM_EVENT_ACTION_T nAction,
                  PDMPlugin & pPlugin,
                  BOOLEAN & bIsEnabledByParent,
                  BOOLEAN bIsChild) const;


/**
  * Verifies if event is enabled by event subscription record. Checks node path and node MDF path  if they are 
  * different  
  * \param strPath [in/out] - DM node path to be logged - set to parent path if event is Cumulative
  * \param nAction [in] - action performed on a node
  * \param pItem [out] - smart pointer on event subscription
  * \param bIsEnabledByParent [out] - specifies if action is enabled by parent node (Cumulative or Detailed)
  * \param bIsChild [in] - specifies if child node is checked (used for "Delete" operation)
  * \return TRUE if enabled 
  */
BOOLEAN IsEnabled(DMString & strPath, 
                  SYNCML_DM_EVENT_ACTION_T nAction,
                  PDMConfigItem & pItem,
                  BOOLEAN & bIsEnabledByParent,
                  BOOLEAN bIsChild) const;


/**
  * Verifies if event is enabled by event subscription record and logs it into Event Logger.
  * \param pArchive [in] - pointer on archive where node is persisted
  * \param strPath [in] - path of updated node
  * \param nAction [in] - action performed on a node
  * \param szNewName [in] - new node name in case of "Rename" operation
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
SYNCML_DM_RET_STATUS_T ProcessUpdateForSubscription(SyncML_DM_Archive * pArchive,
                                                    CPCHAR szPath,
                                                    SYNCML_DM_EVENT_ACTION_T nAction,
                                                    CPCHAR szNewName);


/**
  * Verifies if event is enabled by commit plug-in and logs it into Event Logger.
  * \param pArchive [in] - pointer on archive where node is persisted
  * \param strPath [in] - path of updated node
  * \param nAction [in] - action performed on a node
  * \param szNewName [in] - new node name in case of "Rename" operation
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
SYNCML_DM_RET_STATUS_T ProcessUpdateForPlugin(SyncML_DM_Archive * pArchive,
                                              CPCHAR szPath,
                                              SYNCML_DM_EVENT_ACTION_T nAction,
                                              CPCHAR szNewName);


/**
  * Verifies if "Delete" event is enabled by event subscription record and logs it into Event Logger.
  * \param pArchive [in] - pointer on archive where node is persisted
  * \param strPath [in] - path of deleted node
  * \param bIsChild [in] - specifies if child node of deleted node is processed
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
SYNCML_DM_RET_STATUS_T ProcessDeleteForSubscription(SyncML_DM_Archive * pArchive,
                                                    CPCHAR szPath,
                                                    BOOLEAN bIsChild );

/**
  * Verifies if "Delete" event is enabled by commit plug-in and logs it into Event Logger.
  * \param pArchive [in] - pointer on archive where node is persisted
  * \param strPath [in] - path of deleted node
  * \param bIsChild [in] - specifies if child node of deleted node is processed
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
SYNCML_DM_RET_STATUS_T ProcessDeleteForPlugin(SyncML_DM_Archive * pArchive,
                                              CPCHAR szPath,
                                              BOOLEAN bIsChild );

/**
  * Trims path for Event Logger in case if cumulitive event enabled on parent node by MDF subscription path.
  * \param strPath [in/out] - path for Event Logger (path of updated node)
  * \param szMDF [in] - MDF path of parent node on which cumulative event is enabled
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
SYNCML_DM_RET_STATUS_T GetLogPath(DMString & strPath,
                                  CPCHAR szMDF) const;


/**
  * Updates records in the Event Logger in case of Rename operation performed on node.
  * \param strPath [in] - path of updated node
  * \param nAction [in] - action performed on a node
  * \param szNewName [in] - new node name in case of "Rename" operation
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
SYNCML_DM_RET_STATUS_T UpdateEvents(CPCHAR szPath,
                                    SYNCML_DM_EVENT_ACTION_T nAction,
                                    CPCHAR szNewName);

/**
  * Cleans records in the Event Logger in case of Delete operation performed on node.
  * \param strPath [in] - path of deleted node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
SYNCML_DM_RET_STATUS_T CleanEvents(CPCHAR szPath);

/**
  * Sets new name to null for "Rename" operation if event is a Cumulative and is enabled by parent node.
  * \param bIsEnabledByparent [in] - specifies if event enebled by parent node
  * \param pItem [in] - pointer on event subscription
  * \param szNewName [in] - new node name 
  * \return pointer on new node name ( NULL or szNewName )
  */ 
CPCHAR ResetName(BOOLEAN bIsEnabledByParent,
                 DMEventSubscription * pItem, 
                 CPCHAR szNewName);



};

#endif 
