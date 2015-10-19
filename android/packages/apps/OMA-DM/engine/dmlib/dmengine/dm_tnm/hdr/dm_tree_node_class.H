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

#ifndef _DM_TREE_NODE_CLASS_H
#define _DM_TREE_NODE_CLASS_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

//------------------------------------------------------------------------
//            Header Name: dm_tree_node_class.H
//
//            General Description: This file contains the definition of
//                                 DMNode class.
//------------------------------------------------------------------------

#include "syncml_dm_data_types.h"        //For DM data type definitions
#include "xpl_Logger.h"
#include "dm_tree_typedef.h"    //For tree type definitions
#include "dmstring.h"
#include "dmbuffer.h"
#include "dmdefs.h"

#include "dmt.hpp"                     
#include "dmtPlugin.hpp"       
#include "dmPlugin.h"                  

class SyncML_DM_Archive;

class DMNode
{
protected:   
   //Those values should not be called directly as For
   // plugin proxy node, you must access them through accessor function.
   // For archiver, it is OK.

   DMString abNodeName;
   DMNode *pcParentOfNode;  //Pointer to Parent of the node
   DMNode *pcFirstChild;   //Pointer to  First Child of the node
   DMNode *pcNextSibling;  //Pointer to  Next Sibling of the node

   // psType cannot be accessed directly for mime type optimization
   DMString psType_;
   DMString m_strTitle;   
   SYNCML_DM_FORMAT_T bFormat;  
   UINT16  m_nFlags;  // bit 0 set if plugin, bit 1 set if permanent: see next enum for details


#ifndef DM_IGNORE_TSTAMP_AND_VERSION
   XPL_CLK_CLOCK_T wTStamp;
   UINT16                wVerNo;
#endif

   CPCHAR getType() const;
   SYNCML_DM_RET_STATUS_T setType(const char * strType);

   SYNCML_DM_RET_STATUS_T set(const DMGetData * pData);

   SYNCML_DM_RET_STATUS_T set(CPCHAR strName, CPCHAR strTitle, const DMGetData * pData);

   SYNCML_DM_RET_STATUS_T set(const DMAddNodeProp * pNodeProp);

public:
	 SyncML_DM_Archive 	* pArchive;
   
   enum {
      enum_NodePlugin = 1,
      enum_NodePermanent = 2,
      enum_NodeSkeleton = 4,     // fake node for lazy loading
      enum_NodeOverlayPI = 8,   // mount point for Overlay Pi
      enum_NodeStoresPID = 16,  // multinode with Overlay PI data
      enum_NodeOPISyncNeeded = 32,
      enum_NodeOPISyncNotNeeded = 64,
      enum_NodeNoGetAccess = 128,  // multinode with Overlay PI data
      enum_NodeOPISyncUptodate = 256,
      enum_NodeESN = 512,
      enum_NodeNotPersisted = enum_NodePlugin | enum_NodeSkeleton | enum_NodeOverlayPI |enum_NodeOPISyncUptodate
   };

   friend class DMTree; 
   friend class SyncML_DM_Archive;
   friend class DMMetaDataManager;
   friend class SyncML_PlugIn_WBXMLLog;
   friend class SyncML_DM_WBXMLWriter;


   DMNode(BOOLEAN bPlugin);
   virtual ~DMNode();
   //Overloading new and delete operators
         
   inline void* operator new(size_t dwSize)
   {
      return (DmAllocMem(dwSize));
   }

   inline void operator delete(void *pvBuf)
   {
      DmFreeMem(pvBuf);
   }

   inline DMNode *GetParent() {return pcParentOfNode;}
               
   //The following are Pure virtual functions.Leaf or Interior node 
   //Classes deriving from this class MUST implement the functions.
   virtual SYNCML_DM_RET_STATUS_T Add(DMAddData & oAddData) = 0;  
   
   virtual SYNCML_DM_RET_STATUS_T Delete(CPCHAR pbUri) = 0;  
   
   virtual SYNCML_DM_RET_STATUS_T Get(CPCHAR pbUri, DMGetData & oReturnData) = 0;  

   virtual SYNCML_DM_RET_STATUS_T Find(CPCHAR pbUri) = 0;

   virtual SYNCML_DM_RET_STATUS_T SetAddedNode(CPCHAR pbUri) = 0;

   virtual SYNCML_DM_RET_STATUS_T RemoveAddedNode(CPCHAR pbUri) = 0;	
   
   virtual SYNCML_DM_RET_STATUS_T GetFormat(CPCHAR pbUri,
                                           SYNCML_DM_FORMAT_T * pdwRetPropertyData) = 0;
   
   virtual SYNCML_DM_RET_STATUS_T GetSize(CPCHAR pbUri,
                                          UINT32 *pdwRetPropertyData) = 0;  
   
   virtual SYNCML_DM_RET_STATUS_T GetType(CPCHAR pbUri,
                                         DMString& strType) = 0;
   
   virtual SYNCML_DM_RET_STATUS_T Replace(DMAddData & oReplaceData) = 0;  
   
   virtual SYNCML_DM_RET_STATUS_T Rename(CPCHAR pbUri, CPCHAR psNewNodeName) = 0;  
   
   virtual SYNCML_DM_RET_STATUS_T Rollback(SYNCML_DM_COMMAND_T  bDMCommand,
                                             CPCHAR pbUri) = 0;
   
   virtual SYNCML_DM_RET_STATUS_T GetName(CPCHAR pbUri, DMString& strName);
   
   virtual SYNCML_DM_RET_STATUS_T SetName(CPCHAR pbUri, CPCHAR pbNewName);
   
   virtual SYNCML_DM_RET_STATUS_T GetTitle(CPCHAR pbUri, DMString& ppbTitle);
   
   virtual SYNCML_DM_RET_STATUS_T SetTitle(CPCHAR pbUri, CPCHAR pbNewTitle);

#ifndef DM_IGNORE_TSTAMP_AND_VERSION

   virtual XPL_CLK_CLOCK_T GetTStamp(CPCHAR /*pbUri*/)
   { 
//     pbUri = NULL; 
     return wTStamp; 
   }
  
   virtual SYNCML_DM_RET_STATUS_T SetTStamp(CPCHAR pbUri, XPL_CLK_CLOCK_T timeStamp);
   
   virtual UINT16 GetVerNo(const char* /*pbUri*/) 
   { 
//     pbUri = NULL; 
     return wVerNo;
   }
 
   virtual SYNCML_DM_RET_STATUS_T SetVerNo (CPCHAR pbUri, UINT16 wVerNo);
#endif
       
   // DP: leaf node data access
   virtual DMBuffer * getData() { return NULL; }
   virtual const DMBuffer * getData() const { return NULL; }

   // overlay plug-in access
   virtual DmtOverlayPluginData*  getOverlayPIData() { return NULL;}
   virtual const DmtOverlayPluginData*  getOverlayPIData() const { return NULL;}
   virtual PDMPlugin  getOverlayPI() {return PDMPlugin();}
   
   //-----------Work on THIS NODE ONLY --------------
   // And should used only by persistence layer etc.
   //More methods implemented in the DMNode class.       
 
   //returns the Format of the node
   inline SYNCML_DM_FORMAT_T getFormat() const {  return(bFormat);}

   inline BOOLEAN  isPermanent() const {return (m_nFlags & enum_NodePermanent) != 0; }
   
   inline BOOLEAN  isPlugin() const {return (m_nFlags & enum_NodePlugin) != 0; }
   
   inline BOOLEAN  IsSkeletonNode() const {return (m_nFlags & enum_NodeSkeleton) != 0; }
   
   // synchronization state of Overlay PI
   inline BOOLEAN  opiSyncNeeded() const {return (m_nFlags & enum_NodeOPISyncNeeded) != 0; }
   inline BOOLEAN  opiSyncNotNeeded() const {return (m_nFlags & enum_NodeOPISyncNotNeeded) != 0; }
   inline BOOLEAN  opiSyncMayNeeded() const {return (m_nFlags & (enum_NodeOPISyncNeeded |enum_NodeOPISyncNotNeeded)) == 0; }
   inline BOOLEAN  opiInSync() const {return (m_nFlags & enum_NodeOPISyncUptodate) != 0; }
   inline BOOLEAN  IsOverlayPI() const {return (m_nFlags & enum_NodeOverlayPI) != 0; }
   inline BOOLEAN  IsOverlayPIData() const {return (m_nFlags & enum_NodeStoresPID) != 0; }
   virtual BOOLEAN  IsGetAccess(CPCHAR pURI = NULL) const {return (m_nFlags & enum_NodeNoGetAccess) == 0; }
  
  

   inline UINT16  getFlags() const {return m_nFlags;}
   
   inline void     setFlags( UINT16 n ) { m_nFlags = n;}
   inline void     addFlags( UINT16 n ) { m_nFlags |= n;}

   inline CPCHAR getName() const {  return abNodeName.c_str(); }
   
   
   inline CPCHAR getTitle() const { return m_strTitle.c_str();}
   
   DMNode* GetChildByName( const char* szName ) const;
   
   DMNode* GetNextSerializeItem() ;
   
   void ConvertPathToSkeleton( DMNode* psStartNode ) ;
#ifdef LOB_SUPPORT
   inline BOOLEAN  IsESN() const {return (m_nFlags & enum_NodeESN) != 0; }
   virtual SYNCML_DM_RET_STATUS_T  IsESN(CPCHAR pbUri, BOOLEAN& bESN);
   inline void  SetESN()  { m_nFlags |= enum_NodeESN; }
#endif

};


//------------------------------------------------------------------------
#endif //_DM_TREE_NODE_CLASS_H
