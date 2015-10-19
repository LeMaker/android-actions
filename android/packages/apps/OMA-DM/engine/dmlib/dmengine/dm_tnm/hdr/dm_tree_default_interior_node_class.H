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

#ifndef _DM_TREE_DEFAULT_INTERIOR_NODE_CLASS_H
#define _DM_TREE_DEFAULT_INTERIOR_NODE_CLASS_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

//------------------------------------------------------------------------
//            Header Name: dm_tree_default_interior_node_class.H
//
//  General Description: This file contains the definition of 
//                       DMDefaultInteriorNode class
//------------------------------------------------------------------------

#include "dm_tree_node_class.H"     //for DMNode class defn

//class Definition for Default interior nodes
//IMPORTANT NOTE: A plug-in that supports Interior nodes can derive from
//this class. Only DM_XXX methods are implemented in this class.

// The Plug-in MUST implement functions for Initialization, CreateObject 
// callback, SessionEndcallback and GetClassUUID methods.

class DMDefaultInteriorNode:public DMNode
{
public:   
   //default constructor
   DMDefaultInteriorNode():DMNode(FALSE){bFormat = SYNCML_DM_FORMAT_NODE;};
      
   //function to add an INTERIOR node for which no plug-in 
   //registered
   virtual SYNCML_DM_RET_STATUS_T Add( DMAddData & oAddData );
     
   //To delete a node's data :COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T Delete(CPCHAR pbUri);
    
   //To get node's data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T Get(CPCHAR pbUri, DMGetData & oReturnData);

   virtual SYNCML_DM_RET_STATUS_T Find(CPCHAR pbUri) { return SYNCML_DM_SUCCESS; }

   virtual SYNCML_DM_RET_STATUS_T SetAddedNode(CPCHAR pbUri) { return SYNCML_DM_SUCCESS; }

   virtual SYNCML_DM_RET_STATUS_T RemoveAddedNode(CPCHAR pbUri) { return SYNCML_DM_SUCCESS; }	
                
   //To get node's format of data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T GetFormat(CPCHAR pbUri,
                                               SYNCML_DM_FORMAT_T *pdwRetPropertyData);

   //To get node's type of data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T GetType(CPCHAR pbUri, DMString& strType);

   //To get node's size of data: COMMAND NOT ALLOWED on interior node
   virtual SYNCML_DM_RET_STATUS_T GetSize(CPCHAR pbUri, UINT32 *pdwRetPropertyData);

   //To update node's new name in database: COMMAND NOT ALLOWED 
   //on interior node
   virtual SYNCML_DM_RET_STATUS_T Rename(CPCHAR pbUri, CPCHAR psNewNodeName);

   //To replace node's size of data: COMMAND NOT ALLOWED 
   //on interior node
   virtual SYNCML_DM_RET_STATUS_T Replace(DMAddData & oReplace);

   // To rollback the operation,not supported in Phase 1
   virtual SYNCML_DM_RET_STATUS_T Rollback(SYNCML_DM_COMMAND_T bDMCommand, CPCHAR pbUri);               
};
//------------------------------------------------------------------------


class DMOverlayPINode :public DMDefaultInteriorNode
{
  DmtOverlayPluginData m_piData;
  PDMPlugin    m_ptrPI;
public:   
   DMOverlayPINode( PDMPlugin pi, const DMNode* pOrgNode ) 
   { 
     // make a copy of all attributes including pointers to parent/next/child and names
     *reinterpret_cast<DMNode*>(this) = *pOrgNode; 
     m_ptrPI = pi; 
     m_nFlags |= enum_NodeOverlayPI; 
   }
   
   DMOverlayPINode( PDMPlugin pi, const char* szName, const  DmtOverlayPluginData& data) 
   { 
     // make a copy of all attributes including pointers to parent/next/child and names
     m_ptrPI = pi; 
     m_nFlags |= enum_NodeStoresPID; 
     m_piData = data;
     abNodeName = szName;
   }

   DMOverlayPINode()
   {
      m_nFlags |= enum_NodeStoresPID; 
   }
   virtual DmtOverlayPluginData*  getOverlayPIData() { return &m_piData;}
   virtual const DmtOverlayPluginData*  getOverlayPIData() const { return &m_piData;}
   virtual PDMPlugin     getOverlayPI() {return m_ptrPI;}
};

#endif //_DM_TREE_DEFAULT_INTERIOR_NODE_CLASS_H
