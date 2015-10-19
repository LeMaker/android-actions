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

#ifndef DMMETADATANODE_H
#define DMMETADATANODE_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "dmConstraints.h"

/**
 * DMMetaDataNode represents a metadata node.
 */
class DMMetaDataNode
{

  friend class DMMetaDataManager;
  
public:
  DMMetaDataNode();
 
  /**
  * Operator to allocate memory
  * \param sz [in] - number of bytes to be allocated
  */
  inline void* operator new(size_t sz)
  {
    return (DmAllocMem(sz));
  }

  /**
  * Operator to free memory
  * \param buf [in] - pointer on buffer to be freed
  */
  inline void operator delete(void* buf)
  {
    DmFreeMem(buf);
  }

  /**
  * Retrieves pointer on constraints associated with meta node
  * \return pointer on DMConstraints object
  */
  DMConstraints * GetConstraints() const;

   /**
  * Checks if meta node has child defined as multinode
  * \return TRUE if has 
  */
  inline BOOLEAN IsHasMultiNodes() const { return m_bIsHasMultiChildren; }

   /**
  * Checks if meta node is a multinode
  * \return TRUE if it is a multinode 
  */
  inline BOOLEAN IsMultiNode() const { return m_bIsMultiNode; }

   /**
  * Checks if node is a plugin node
  * \return TRUE if it is a multinode 
  */
  inline BOOLEAN IsPluginNode() const { return m_bPluginNode; }

   /**
  * Sets offset in the Meta Data buffer on specified child
  * \param pBuffer [in] - pointer on Meta Data buffer
  * \param index [in] - child index ( starts from 0 )
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T SetChildrenOffset(DMMetaDataBuffer * pBuffer,
                                                              UINT8 index); 

  /**
  * Retrieves full path of meta 
  * \return Return Type (DMBuffer &) 
  */
  const DMBuffer & GetPath() const { return m_oPath; }

  /**
  * Sets starting path of meta node
  * \param szPath [in] - starting path of multinode
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T SetPath(CPCHAR szPath); 

  /**
  * Allocates memory for full MDF path
  * \param size [in] - number of bites to allocate
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T AllocatePath(UINT32 size); 

  /**
  * Appends node name to built MDF path
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T AppendSegment(); 
  
  /**
  * Sets access type for node inherited from parent node
  * \param accessType [in] - access type
  */
  void SetAccessType(SYNCML_DM_ACCESS_TYPE_T accessType); 

  /**
  * Retrieves mime type  
  * \param strType [out] - myme type
  */
  void GetMimeType(DMString & strType);

  /**
  * Checks if node has a "Local" attribute
  * \return TRUE if "Local" attribute  is set 
  */
  BOOLEAN IsLocal();

  /**
  * Checks if node is a leaf
  * \return TRUE if is a leaf 
  */
  BOOLEAN IsLeaf();

  /**
  * Verifies if access is specified for meta node
  * \param accessType [in] - access type to check
  * \return TRUE if operation is allowed 
  */
  BOOLEAN VerifyAccessType(SYNCML_DM_ACCESS_TYPE_T accessType) const;

  /**
  * Verifies mime type
  * \param mimeType [in] - mime type to checks
  * \return TRUE if mime type is correct 
  */
  BOOLEAN VerifyMimeType(CPCHAR mimeType) const;

  /**
  * Verifies max number of children multinodes that can by created under a node
  * \param count [in] - current number of nodes 
  * \param bOPiDataParent [in] - specifies if node is a parent of node containing plug-in data
  * \return TRUE if max number isn't reached 
  */
  BOOLEAN VerifyChildrenMultiNodesCount(UINT16 count, 
                                     BOOLEAN& bOPiDataParent) const;

  /**
  * Reads max number of children multinodes may by created under a node
  * \param oBuffer [in] - Meta Data buffer 
  * \param bOPiDataParent [in] - specifies if node is a parent of node containing plug-in data
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T GetMaxMultiNodeChildren(DMMetaDataBuffer oBuffer);   

  /**
  * Reads node info from MDF file
  * \param oBuffer [in] - Meta Data buffer 
  * \param bReadConstraints [in] - specifies if node constraints should be read
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T Read(DMMetaDataBuffer oBuffer, 
                        BOOLEAN bReadConstraints);   

  /**
  * Reads node name from MDF file
  * \param oBuffer [in] - Meta Data buffer 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T ReadName(DMMetaDataBuffer oBuffer);  

  /**
  * Verifies if node is a overlay plug-in node
  * \param szID [out] - node id 
  * \param wAccessType [out] - node access type
  * \param nNodeFormat [out] - node format 
  * \return TRUE if node ia a overlay plug-in node 
  */
  BOOLEAN VerifyOPINode(CPCHAR& szID,   
                        SYNCML_DM_ACCESS_TYPE_T&  wAccessType,
                        SYNCML_DM_FORMAT_T& nNodeFormat)  const;

 /**
  * Verifies if node is a parent of node containing plg-in data
  * \return TRUE if node is a parent of node containing plg-in data
  */
 inline BOOLEAN IsOPiDataParent() const { return m_bOPiDataParent;}
 
#ifdef LOB_SUPPORT
 /**
  * Verifies if node is a large object
  * \return TRUE if node is a large object
  */
 inline BOOLEAN IsESN() const { return m_bESN;}

 /**
  * Verifies if node is a progress bar is required for large object
  * \return TRUE if progress bar is required
  */
 inline BOOLEAN IsProgressBarNeeded() const { return m_bProgressBarNeeded;}
#endif

private:  
  enum {
    /* Mask for node type verification */
    nodeTypeMask = 0x7f,
    /* Specifies if node a multinode */
    nodeMultiNode = 0x80,
    /* Specifies if node stores plug-in data */
    nodeStoresPD = 0x100,
    /* Specifies if node a ovelraly plug-in node */
    nodePluginNode = 0x200,
    /* Specifies if node has an ID */
    nodeHasID = 0x400,
#ifdef LOB_SUPPORT
    /* Specifies if node is a large object */ 
    nodeESN = 0x800,
    /* Specifies if progress bar is needed */
    nodeProgressBar = 0x1000
#endif
  };

  /**
  * Verifies if node has multinode children
  * \return TRUE if hode has multinode children
  */
  void CheckHasMultiNode(DMMetaDataBuffer oBuffer);

   /**
  * Initializes object
  */
  void Init();

  /* Node name */
  CPCHAR m_psName;
   /* Node ID for overlay plug-in node */
  CPCHAR m_szID;
   /* Full MDF path to a node */
  DMBuffer m_oPath;
  /* Access type */ 
  SYNCML_DM_ACCESS_TYPE_T m_wAccessType;
  /* Node format */
  SYNCML_DM_FORMAT_T m_nNodeFormat;
  /* Mime Type */
  SYNCML_DM_DDF_MIME_TYPE_T m_nMimeType;
  /* Number of constraints a node has */
  UINT8 m_nNumConstraints;
  /* Number of children a node has */
  UINT16 m_nNumChildren; 
  /* Offset to first child */
  UINT32 m_nOffsetChildren;
  /* Node constraints */
  DMConstraints m_oConstraints;
  /* Max number of multinode children */
  UINT16 m_nMaxChildrenMultiNodes;
  /* Specifies if node has multinode children */
  BOOLEAN m_bIsHasMultiChildren;
  /* Specifies if node is a multinode */
  BOOLEAN m_bIsMultiNode; 
  /* Specifies if node stores a plug-in data */
  BOOLEAN m_bStoresPD;
  /* Specifies if node is a overlay plug-in node */
  BOOLEAN m_bPluginNode;
  /* Specifies if node is a parent of node that stores a plug-in data */ 
  BOOLEAN m_bOPiDataParent;
#ifdef LOB_SUPPORT
  /* Specifies if node is a large object */
  BOOLEAN m_bESN;
  /* Specifies if progress bar is needed */
  BOOLEAN m_bProgressBarNeeded;
#endif
};

#endif
