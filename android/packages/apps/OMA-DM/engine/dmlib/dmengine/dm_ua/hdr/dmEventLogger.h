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

#ifndef DMEVENTLOGGER_H
#define DMEVENTLOGGER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "dmstring.h"
#include "dmvector.h"
#include "dmEvent.h"
#include "dmConfigItem.h"
#include "dmPlugin.h"

class DMTree;

typedef  DMMap<PDMConfigItem, DMEventMap> DMPostEventMap;
typedef  DMMap<PDMPlugin, DMEventMap> DMPluginEventMap;

/**
 * DMEventLogger represents event storage for broadcast events and commit plug-ins events.
 */
class DMEventLogger
{
public:
  /**
  * Default constructor
  */
  DMEventLogger();

  /**
  * Default destructor
  */
  ~DMEventLogger();

  /**
  * Initializes event logger  
  * \param pTree [in] - pointer on DM tree object
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Init( DMTree* tree );

 /**
  * Adds a commit-plug-in event
  * \param szPath [in/out] - path of updated node
  * \param nAction [in] - action performed on a node
  * \param pPlugin [in] - smart pointer on commit plug-in
  * \param bIsEnabledByParent [in] - specifies if action is enabled by parent node
  * \param szNewName [in] - new node name in case of "Rename" operation
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T OnNodeChanged(CPCHAR szPath , 
                                       SYNCML_DM_EVENT_ACTION_T nAction, 
                                       const PDMPlugin & pPlugin,
                                       BOOLEAN bIsEnabledByParent,
                                       CPCHAR szNewName = NULL );

 /**
  * Adds a broadcast event
  * \param szPath [in/out] - path of updated node
  * \param nAction [in] - action performed on a node
  * \param pItem [in] - smart pointer on event subscription
  * \param bIsEnabledByParent [in] - specifies if action is enabled by parent node
  * \param szNewName [in] - new node name in case of "Rename" operation
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T OnNodeChanged(CPCHAR szPath , 
                                       SYNCML_DM_EVENT_ACTION_T nAction, 
                                       const PDMConfigItem & pItem,
                                       BOOLEAN bIsEnabledByParent,
                                       CPCHAR szNewName = NULL );

 /**
  * Post broadcast events via XPL layer. 
  */
  void OnTreeSaved();

 /**
  * Reset internal storage. 
  */
  void Reset();

  /**
  * Retrieves events map for specified commit plug-in
  * \param pPlugin [in] - smart pointer on commit plug-in
  * \param aUpdatedNodes [out] - events to be passed to commit plug-in
  */
  void GetCommitPluginEvents(const PDMPlugin & pPlugin,
                             DmtEventMap & aUpdatedNodes ) const;


  /**
  * Deserializes events map
  * \param oBuffer [in] - binary buffer that contains events data
  * \param aEventMap [out] - events map
  * \param pItem [in] - smart pointer on event subscription
  * \param bIsEnabledByParent [in] - specifies if action is enabled by parent node
  * \param szNewName [in] - new node name in case of "Rename" operation
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  static SYNCML_DM_RET_STATUS_T Deserialize(DMBufferReader & oBuffer,
                                            DmtEventMap & aEventMap); 


 /**
  * Deserializes event data for leaf node 
  * \param oBuffer [in] - binary buffer that contains events data
  * \param strParent [out] - parent node path
  * \param aData [out] - smart pointer on event data (update description)
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  static SYNCML_DM_RET_STATUS_T Deserialize(DMBufferReader & oBuffer,
                                            DMString & strParent,
                                            PDmtEventData & aData); 

  /**
  * Cleans records in the Event Logger in case of "Delete" operation performed on node.
  * \param szPath [in] - path of deleted node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T CleanEvents(CPCHAR szPath); 

  /**
  * Updates records in the Event Logger in case of "Rename" operation performed on node.
  * \param szPath [in] - path of updated node
  * \param szNewName [in] - new node name
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T UpdateEvents(CPCHAR szPath, 
                                      CPCHAR szNewName); 


  /**
  * Verifies if update on node should be ignored (because it was already logged)
  * \param szPath [in] - path of updated node
  * \param nAction [in] - action performed on a node
  * \param szNewName [in] - new node name
  * \return TRUE if should be ignored 
  */ 
  BOOLEAN IsIgnoreSubscriptionEvent(CPCHAR szPath,
                                    SYNCML_DM_EVENT_ACTION_T nAction, 
                                    CPCHAR szNewName = NULL);

  /**
  * Verifies if update on node should be ignored (because it was already logged)
  * \param szPath [in] - path of updated node
  * \param nAction [in] - action performed on a node
  * \param szNewName [in] - new node name
  * \return TRUE if should be ignored 
  */ 
  BOOLEAN IsIgnorePluginEvent(CPCHAR szPath,
                              SYNCML_DM_EVENT_ACTION_T nAction, 
                              CPCHAR szNewName = NULL);

  
private:
  /* Map to store broadcast events */
  DMPostEventMap m_aPostEvents;
  /* Map to store commit plug-ins events */
  DMPluginEventMap m_aPluginEvents;
  /* Pointer on DM tree */
  DMTree*   m_pTree;

   /**
  * Calculates size of map for serialization
  * \param aEventMap [in] - map of events
  * \return Return Type (UINT32)  
  */ 
  static UINT32 GetSize(const DMEventMap & aEventMap);

 
  /**
  * Finds records in the map by specified parent path
  * \param szPath [in] - parent path to search
  * \param aEventMap [in] - map of events
  * \param pEventPath [out] - smart pointer on found path to parent node ( key )
  * \param aVector [out] - found events vector ( value )
  * \return TRUE if found 
  */ 
  BOOLEAN Find(CPCHAR szPath, 
               const DMEventMap & aEventMap,
               PDMEventPath & pEventPath,
               DmtEventDataVector & aVector);

   /**
  * Finds record in the vector by specified name
  * \param aVector [in] - vector of logged events 
  * \param nAction [in] - action performed on a node
  * \param bIsCumulative [in] - specifies if event is a Cumulative
  * \param bIsEnabledByParent [in] - specifies if event is enabled by parent node (Cumulative or Detailed)
  * \param szName [in] - node name
  * \param szNewName [in] - new node name in case of "Rename"
  * \return Return Type (PDmtEventData) 
  *  returns smart pointer on event data if found, null otherwise   
  */ 
  PDmtEventData Find(const DmtEventDataVector & aVector,
                     SYNCML_DM_EVENT_ACTION_T nAction, 
                     BOOLEAN bIsCumulative,
                     BOOLEAN bIsEnabledByParent,
                     CPCHAR szName,
                     CPCHAR szNewName);

 /**
  * Adds action into existing event data
  * \param pEventData [in/out] - smart pointer on event data 
  * \param nAction [in] - action performed on a node
  * \param szNewName [in] - new node name in case of "Rename"
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T AddEvent(PDmtEventData & pEventData,
                                  SYNCML_DM_EVENT_ACTION_T nAction, 
                                  CPCHAR szNewName);

  /**
  * Adds new event data into storage
  * \param aVector [in] - vector of logged events 
  * \param nAction [in] - action performed on a node
  * \param bIsLeaf [in] - specifies if node is a leaf
  * \param bIsEnabledByParent [in] - specifies if event is enabled by parent node (Cumulative or Detailed)
  * \param szName [in] - node name
  * \param szNewName [in] - new node name in case of "Rename"
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T AddEvent(DmtEventDataVector & aVector,
                                  SYNCML_DM_EVENT_ACTION_T nAction, 
                                  BOOLEAN bIsLeaf,
                                  BOOLEAN bIsEnabledByParent,
                                  CPCHAR szName,
                                  CPCHAR szNewName);


   /**
  * Adds an event (new event data or new action on existing record)
  * \param szPath [in/out] - path of updated node
  * \param nAction [in] - action performed on a node
  * \param aEventMap [in/out] - map of events
  * \param bIsCumulative [in] - specifies if event is a Cumulative
  * \param bIsEnabledByParent [in] - specifies if action is enabled by parent node
  * \param szNewName [in] - new node name in case of "Rename" operation
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T OnNodeChanged(CPCHAR szPath ,
                                       SYNCML_DM_EVENT_ACTION_T nAction, 
                                       DMEventMap & aEventMap,
                                       BOOLEAN bIsCumulative,
                                       BOOLEAN bIsEnabledByParent,
                                       CPCHAR szNewName );


   /**
  * Cleans broadcast records in the Event Logger in case of "Delete" operation performed on node.
  * \param szPath [in] - path of deleted node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T CleanConfigEvents(CPCHAR szPath);

   /**
  * Cleans commit plug-ins records in the Event Logger in case of "Delete" operation performed on node.
  * \param szPath [in] - path of deleted node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T CleanPluginEvents(CPCHAR szPath);

  /**
  * Cleans records in the event map in case of "Delete" operation performed on node.
  * \param aEventMap [in/out] - events map 
  * \param szPath [in] - path of deleted node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
 SYNCML_DM_RET_STATUS_T CleanEvents(DMEventMap & aEventMap, 
                                     CPCHAR szPath);

  /**
  * Cleans event data records in the vectore in case of "Delete" operation performed on node.
  * It removes only data enabled by parent node
  * \param aVector [in/out] - events data vector
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
 SYNCML_DM_RET_STATUS_T CleanEvents(DmtEventDataVector & aVector);


   /**
  * Updates records in the Event Logger in case of "Rename" operation performed on node.
  * \param aEventMap [in/out] - events map 
  * \param szPath [in] - path of updated node
  * \param szNewName [in] - new node name
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
 SYNCML_DM_RET_STATUS_T UpdateEvents(DMEventMap & aEventMap,
                                     CPCHAR szPath, 
                                     CPCHAR szNewName);

   /**
  * Updates broadcast records in the Event Logger in case of "Rename" operation performed on node.
  * \param szPath [in] - path of updated node
  * \param szNewName [in] - new node name
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
 SYNCML_DM_RET_STATUS_T UpdateSubscriptionEvents(CPCHAR szPath,
                                                 CPCHAR szNewName);

  /**
  * Updates commit plug-ins records in the Event Logger in case of "Rename" operation performed on node.
  * \param szPath [in] - path of updated node
  * \param szNewName [in] - new node name
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
 SYNCML_DM_RET_STATUS_T UpdatePluginEvents(CPCHAR szPath,
                                           CPCHAR szNewName);


   /**
  * Finds and updates record for node in case of "Add" operation
  * "Delete"-"Add" sequence on leaf node converted into "Replace"
  * \param aVector [in/out] - events data vector
  * \param szName [in] - node name
  * \return TRUE if found 
  */ 
 BOOLEAN FindRecordForAdd(const DmtEventDataVector & aVector, CPCHAR szName);

  /**
  * Finds record for node in case of "Replace" operation
  * \param aVector [in/out] - events data vector
  * \param szName [in] - node name
  * \return TRUE if found 
  */ 
 BOOLEAN FindRecordForReplace(const DmtEventDataVector & aVector,
                              CPCHAR szName);


  /**
  * Finds and updates record for node in case of "Rename" operation
  * \param aVector [in/out] - events data vector
  * \param szName [in] - node name
  * \param szNewName [in] - node name
 * \return TRUE if found 
  */ 
 BOOLEAN FindRecordForRename(DmtEventDataVector & aVector,
                             CPCHAR szName,
                             CPCHAR szNewName);

  /**
  * Finds and remove rocord about add operation in case of "Delete" operation
  * \param aVector [in/out] - events data vector
  * \param szName [in] - node name
 * \return TRUE if found 
  */ 
 BOOLEAN FindRecordForDelete(DmtEventDataVector & aVector, 
                             CPCHAR szName); 

 /**
  * Finds and updates record for same node (Only 'Rename" checked in case of Cumulative events)
  * \param aVector [in/out] - events data vector
  * \param nAction [in] - action performed on a node
  * \param szName [in] - node name
  * \param bIsCumulative [in] - specifies if event is a Cumulative
  * \param szNewName [in] - node name
 * \return TRUE if found 
  */  
 BOOLEAN CheckEventOnSameNode(DmtEventDataVector & aVector,
                              SYNCML_DM_EVENT_ACTION_T nAction, 
                              CPCHAR szName,
                              BOOLEAN bIsCumulative,
                              CPCHAR szNewName);
 
/**
  * Checks if update should be ignored by Event Logger 
  * \param szPath [in/out] - path of updated node
  * \param nAction [in] - action performed on a node
  * \param aEventMap [in/out] - events map 
  * \param bIsCumulative [in] - specifies if event is a Cumulative
  * \param szNewName [in] - node name
  * \return TRUE if event should be ignored 
  */
 BOOLEAN IsIgnoreEvent(CPCHAR szPath ,
                        SYNCML_DM_EVENT_ACTION_T nAction, 
                        DMEventMap & aEventMap,
                        BOOLEAN bIsCumulative,
                        CPCHAR szNewName);


  /**
  * Deserializes events data vector
  * \param oBuffer [in] - binary buffer that contains events data
  * \param aVector [out] - events data vector
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
 static SYNCML_DM_RET_STATUS_T Deserialize(DMBufferReader & oBuffer,
                                           DmtEventDataVector & aVector); 

 /**
  * Serializes events data vector into a binary buffer
  * \param aVector [in] - events data vector
  * \param oBuffer [out] - binary buffer 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
 static SYNCML_DM_RET_STATUS_T Serialize(const DmtEventDataVector & aVector, 
                                         DMBufferWriter & oBuffer);

 /**
  * Serializes events map into a binary buffer
  * \param aEventMap [in] - events map
  * \param oBuffer [out] - binary buffer 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
static SYNCML_DM_RET_STATUS_T Serialize(const DMEventMap & aEventMap,
                                         DMBufferWriter & oBuffer);

  
};

#endif 
