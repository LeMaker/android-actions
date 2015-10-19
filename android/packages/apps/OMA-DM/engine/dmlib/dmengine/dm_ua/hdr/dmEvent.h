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

#ifndef DMEVENT_H
#define DMEVENT_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

Header Name: dmEvent.h

General Description: This file contains declaration of the DMEventPath and DMEventData classes, 
                            which define DMT event    

==================================================================================================*/

#include "dmtEventData.hpp" 
#include "dmBufferWriter.h"
#include "dmBufferReader.h"

/**
* DMEventPath represents a path to parent of upadted node 
*/
class DMEventPath : public JemBaseObject
{

public:
 /**
 * Default constructor
 */
  DMEventPath() {}

  /**
  * Constructor, which sets parent path
  * \param szPath [in] - parent path
  */
  DMEventPath(CPCHAR szPath);

 /**
 * Default destructor
 */
  virtual ~DMEventPath() {}

 /**
 * Retrieves path
 */
  const DMString & GetPath() const { return m_strPath; }

 /**
 * Retrieves path
 */
  DMString & GetPath() { return m_strPath; }

  /**
  * Serializes path into a binary buffer 
  * \param oBuffer [out] - binary buffer
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Serialize(DMBufferWriter & oBuffer);

   /**
  * Calculates size of path for serialization
  * \return Return Type (UINT32)  
  */ 
  UINT32 GetSize();

  /**
  * Deserializes path from a binary buffer 
  * \param oBuffer [out] - binary buffer
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T Deserialize(DMBufferReader & oBuffer);


protected:
  /* parent path */
  DMString  m_strPath;

};




/**
* DMEventPath represents actual updates performed on node 
*/
class DMEventData : public DmtEventData 
{

public:
  /**
 * Default constructor
 */
  DMEventData() 
  {
      m_bIsEnabledByParent = FALSE;
  } 

  /**
  * Constructor, that sets object members
  * \param bIsLeaf [in] - specifies if node is a leaf
  * \param eAction [in] - bit-wised actions performed on a node
  * \param bIsEnabledByParent [in] - specifies if action is enabled by parent node
  * \param szName [in] - node name
  * \param szNewName [in] - new node name in case of "Rename" operation
  */
  DMEventData(BOOLEAN bIsLeaf,
                      SYNCML_DM_EVENT_ACTION_T eAction,
                      BOOLEAN bIsEnabledByParent,
                      CPCHAR strName,
                      CPCHAR strNewName = NULL); 
  

   /**
  * Sets flag that specifies if node is a leaf
  * \param bIsLeaf [in] - specifies if node is a leaf
  */
  void SetLeaf(BOOLEAN bIsLeaf);

  /**
  * Sets flag that specifies if action is a enabled by parent node
  * \param bIsEnabledByparent [in] - specifies if event is enabled by parent node
  */
  void SetEnabledByParent(BOOLEAN bIsEnabledByParent);

   /**
  * Sets bit-wised action performed on node
  * \param eAction [in] - bit-wised actions performed on a node
  */
  void SetAction(SYNCML_DM_EVENT_ACTION_T eAction);

   /**
  * Sets node name
  * \param szName [in] - node name
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T SetName(CPCHAR strName); 

  /**
  * Sets node new name
  * \param szNewName [in] - node name
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T SetNewName(CPCHAR strNewName); 

  /**
  * Adds action performed on node
  * \param eAction [in] - action performed on a node
  */
  void AddAction(SYNCML_DM_EVENT_ACTION_T eAction) ; 

  /**
  * Reset action performed on node
  * \param eAction [in] - action should be removed
  */
  void RemoveAction(SYNCML_DM_EVENT_ACTION_T eAction) ; 

   /**
  * Serializes object into a binary buffer 
  * \param oBuffer [out] - binary buffer
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Serialize(DMBufferWriter & oBuffer);

   /**
  * Deserializes object from a binary buffer 
  * \param oBuffer [out] - binary buffer
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T Deserialize(DMBufferReader & oBuffer);

   /**
  * Calculates size of object for serialization
  * \return Return Type (UINT32)  
  */ 
  UINT32 GetSize();

  /**
  * Check if event is enabled by parent node 
  * \return TRUE if enabled by parent
  */ 
  BOOLEAN IsEnabledByParent() const { return m_bIsEnabledByParent; }

private:
  /* Specifies if event is enabled by parent node */
  BOOLEAN m_bIsEnabledByParent;


};

typedef JemSmartPtr<DMEventPath> PDMEventPath;

typedef DMMap<PDMEventPath, DmtEventDataVector > DMEventMap;

#endif 
