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

#ifndef __DMTEVENTDATA_H__
#define __DMTEVENTDATA_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/** 
  \file dmtEventData.hpp
  \brief The dmtEventData.hpp header file contains DmtEventData class  definition. \n
       This class represents updates performed on particular DM node. 
*/

#include "dmtEvent.hpp"  
#include "dmstring.h"
#include "dmvector.h"

/**
* DmtEventData represents actual updates performed on DM node. 
* \par Category: General  
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DmtEventData : public JemBaseObject
{

public:
    
  /**
    * Default Constructor - no memory allocation performed.
    */   
  DmtEventData()
  { 
        m_eAction = SYNCML_DM_EVENT_NONE;
        m_bIsLeaf = FALSE;
  }

  /**
    * Default Destructor.
    */   
  virtual ~DmtEventData() {}

 /** 
  * Retrieves operations (actions) performed on node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (SYNCML_DM_EVENT_ACTION_T)\n
  * - Bit-wised actions performed on node
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  inline SYNCML_DM_EVENT_ACTION_T GetAction() const { return  m_eAction; }


  /** 
  * Retrieves type of node (leaf or interior).
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return TRUE if node is leaf\n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  inline BOOLEAN IsLeaf() const { return  m_bIsLeaf; }


  /** 
  * Retrieves node name.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return const reference on node name\n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  const DMString & GetName() const { return m_strName; }

  /** 
  * Retrieves node name.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return reference on node name\n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  DMString & GetName() { return m_strName; }


/** 
  * Retrieves new node name (Rename was performed).
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return const reference on new node name\n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  const DMString & GetNewName() const { return m_strNewName; }

/** 
  * Retrieves new node name (Rename was performed).
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return reference on new node name\n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  DMString & GetNewName() { return m_strNewName; }



protected:
 /** child node name */
  DMString  m_strName;
 /**  child node new name*/
  DMString  m_strNewName;
 
 /** child node operation */
  SYNCML_DM_EVENT_ACTION_T m_eAction;
 /** flag contains info is a node is a leaf*/
  BOOLEAN m_bIsLeaf;
};


/** 
* Type definition for DmtEventData smart pointer  
*/
typedef JemSmartPtr<DmtEventData> PDmtEventData;

/** 
* Type definition for vector containing DmtEventData smart pointers
*/
typedef DMVector<PDmtEventData> DmtEventDataVector;

/** 
* Type definition for map containing parent node path and vector of updates on children nodes
*/
typedef DMMap<DMString, DmtEventDataVector> DmtEventMap;

#endif 
