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

#ifndef SYNCML_DM_DDF_H
#define SYNCML_DM_DDF_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: dmMetaDataManager.h

    General Description: This file contains declaration of the DMMetaDataManager class

==================================================================================================*/

#include "dm_tree_class.H" 
#include "SyncML_DM_FileHandle.H"
#include "dmMetaDataNode.h"
#include "dmtoken.h"

typedef BOOLEAN (*SYNCML_DM_MDF_CBACK)(DMNode* pNode, PDmtNode ptrPluginNode, CPCHAR szNodeName, CPCHAR szOrigName);


class DMMetaDataVector : public DMVector<UINT32>
{
};

class DMMetaPCharVector : public DMVector<CPCHAR>
{
};

/**
* DMMetaNodeLocator represents a locator of Meta Data node stored in a cache 
*/
struct DMMetaNodeLocator
{
  /**
  * Default constructor
  */
  DMMetaNodeLocator()
  {
    m_szName = NULL;
    m_nOffset = 0;
    m_wAccessType = 0;
  }

   /**
  * Constructor that sets values of structures
  * \param szName [in] - node name
  * \param offset [in] - offset of  a node in the MDF file
  * \param wAccessType [in] - node access type
  * \param oPath [in] - full MDF path 
  */
  DMMetaNodeLocator(CPCHAR szName, 
                           UINT32 offset, 
                           SYNCML_DM_ACCESS_TYPE_T wAccessType,
                           const DMBuffer & oPath)
  {
    m_szName = szName;
    m_nOffset = offset;
    m_wAccessType = wAccessType;
    oPath.copyTo(m_strPath);
  }


  /* Node name */
  CPCHAR m_szName;
  /* Offset of a node in the MDF file */
  UINT32 m_nOffset;
  /* Node access type */
  SYNCML_DM_ACCESS_TYPE_T m_wAccessType;
  /* Full MDF path */
  DMString m_strPath;
};


class DMMetaDataNodeVector : public DMVector<DMMetaNodeLocator>
{
};  

/**
* DMLastMetaDataNode represents a last accessed MDF node 
*/
class DMLastMetaDataNode 
{
public:
  /**
  * Default constructor
  */
  DMLastMetaDataNode () 
  {
    m_pBuffer = NULL; 
  }

  /**
  * Initializes object
  */
  void Init()
  {
    m_pBuffer = NULL;
    m_oLocator.clear();
  }
    
  /* MDF buffer */
  MDF_BUFFER_T m_pBuffer;
  /* Meta node locator */
  DMMetaDataNodeVector m_oLocator; 
};

class DMTree;

/**
* DMMetaDataManager represents a Meda Data Manager for constraints and DMT schema verification 
*/
class DMMetaDataManager
{
public: 
  /**
  * Default constructor
  */
  DMMetaDataManager();

  /**
  * Destructor
  */
  ~DMMetaDataManager();

  /**
  * Initializes Meta Data manager  
  * \param env [in] - pointer on env object
  * \param pTree[in] - pointer on DM tree object
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T  Init( CEnv* env, DMTree* tree );

  /**
  * Deinitializes Meta Data manager  
  */ 
  void  DeInit();

 /**
  * Verifyes parameters of a node to be added  
  * \param pNode [in] - pointer on DM node to be added
  * \param oAddData [in] - data to be added
  * \param oAutoNodes [out] - comma separated list of nodes to be auto added
  * \param bNodeGetAccess [out] - specifies if node has "Get" permission 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T VerifyAddParameters(DMNode* pNode,
                                            DMAddData & oAddData,
                                            DMToken & oAutoNodes,
                                            BOOLEAN & bNodeGetAccess);

  /**
  * Verifyes parameters of a node to be replaced/renamed  
  * \param pNode [in] - pointer on DM node to be replaced
  * \param szURI [in] - path to replaced node
  * \param oAddData [in] - data to be replaced with
  * \param szOrigName [in] - original name of a node (in case of "Rename" operation  
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T VerifyReplaceParameters(DMNode* pNode,
                                                CPCHAR szURI,
                                                DMAddData & oAddData,
                                                CPCHAR szOrigName);

  /**
  * Verifyes if node can de deleted  
  * \param pNode [in] - pointer on DM node to be deleted
  * \param szURI [in] - path of node to be deleted
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T VerifyDeleteParameters(DMNode* pNode, 
                                           CPCHAR szURI);

  /**
  * Sets auto node property  
  * \param szURI [in] - path to replaced node
  * \param oAddData [out] - data to be set for auto node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T SetAutoNodeProperty(CPCHAR szURI,
                                        DMAddData & pAdd);

  /**
  * Verifyes access type od a node  
  * \param szURI [in] - node path
  * \param accessType [in] - access type to be verified
  * \param pChildDependNodes [out] - pointer on list of hard and soft dependencies of node
  * \return TRUE if access type is found (operation is allowed)
  */
  BOOLEAN VerifyAccessType(CPCHAR szURI, 
                        SYNCML_DM_ACCESS_TYPE_T accessType,
                        DMMetaPCharVector* pChildDependNodes = NULL); 

 /**
  * Verifies max number of children multinodes that can by created under a node
  * \param szURI [in] - Node path
  * \param count [in] - current number of nodes 
  * \param bOPiDataParent [in] - specifies if node is a parent of node containing plug-in data
  * \return TRUE if max number isn't reached 
  */
  BOOLEAN VerifyChildrenMultiNodesCount(CPCHAR szURI,
                                    UINT16 count, 
                                    BOOLEAN& bOPiDataParent); 

  /**
  * Verifies if node is a overlay plug-in node
  * \param szURI [in] - node path
  * \param szID [out] - node id 
  * \param wAccessType [out] - node access type
  * \param nNodeFormat [out] - node format 
  * \return TRUE if node ia a overlay plug-in node 
  */
  BOOLEAN VerifyOPINode(CPCHAR szURI, 
                        CPCHAR& szID,
                        SYNCML_DM_ACCESS_TYPE_T&  wAccessType,
                        SYNCML_DM_FORMAT_T& nNodeFormat  );

 /**
  * Retrieves children of node as "/" separated string 
  * \param szURI [in] - node path
  * \param strChildrenList [out] - list of children 
  * \return count of children 
  */
  UINT16  UpdateChildrenList(CPCHAR szURI, 
                             DMString& strChildrenList );

  /**
  * Verifies if node is a parent of node containing plg-in data
  * \param szURI [in] - node path
  * \return TRUE if node is a parent of node containing plg-in data
  */
  BOOLEAN IsOPiDataParent(CPCHAR szURI);

  /**
  * Checks if node has a "Local" attribute
  * \param szURI [in] - node path
  * \return TRUE if "Local" attribute  is set 
  */
  BOOLEAN IsLocal(CPCHAR szURI);

  /**
  * Checks if node is a leaf
  * \param szURI [in] - node path
  * \return TRUE if is a leaf 
  */
  BOOLEAN IsLeaf(CPCHAR szURI);

  /**
  * Retrieves a MDF path for a node (could different because of multinodes and reccurance pattern
  * \param szURI [in] - node path
  * \param szPath [out] - MDF path 
  * \return count of children 
  */
  SYNCML_DM_RET_STATUS_T GetPath(CPCHAR szURI, 
                            DMString & szMDF);
  
#ifdef LOB_SUPPORT
  /**
  * Verifies if node is a large object
  * \param szURI [in] - node path
  * \return TRUE if node is a large object
  */
  BOOLEAN IsESN( CPCHAR szURI );

  /**
  * Verifies if node is a progress bar is required for large object
  * \param szURI [in] - node path
  * \return TRUE if progress bar is required
  */
  BOOLEAN IsProgressBarNeeded( CPCHAR szURI );
#endif

private:
 enum 
 { 
    CurrentBMDFVersion = 1,
    BMDFHeaderSize = 6 /* file size (4) and version (2) */ 
 };

  /**
  * Copy constructor
  */
  DMMetaDataManager( const DMMetaDataManager& mdm );

  /** Assignment  operator */ 
  const DMMetaDataManager&  operator=( const DMMetaDataManager& mdm );

  /**
  * Retrieves node name (from path in case of plugin node or from URI 
  * \param pNode [in] - pointer on DM node
  * \param szURI [in] - node path
  * \return pointer on node name 
  */
  CPCHAR GetNodeName(DMNode *pNode, CPCHAR szURI);

  /**
  * Verifyes parameters of a node  for Add/Replace and Rename operation
  * \param oNodeMDF [in] - Meta node
  * \param pNode [in] - pointer on DM node to be verified
  * \param szNodeName [in] - node name
  * \param oAddData [in] - data to be added or replaced with
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T VerifyParameters(const DMMetaDataNode & oNodeMDF,
                                          DMNode* pNode,
                                          CPCHAR szNodeName,
                                          DMAddData & oAddData);

 /**
  * Verifyes data field (value) 
  * \param oAddData [in] - data to be added or replaced with
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
 SYNCML_DM_RET_STATUS_T VerifyData(DMAddData & oAddData);

 /**
  * Verifyes format of a node
  * \param oNodeMDF [in] - Meta node
  * \param oAddData [in] - data to be added or replaced with
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
 SYNCML_DM_RET_STATUS_T VerifyFormat(const DMMetaDataNode & oNodeMDF,
                                     DMAddData & oAddData);

 /**
  * Verifyes integer value 
  * \param oData [in] - value to be checked
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
 SYNCML_DM_RET_STATUS_T VerifyInteger(const DMBuffer & oData);

 /**
  * Verifyes interior node constraints
  * \param pConstraints [in] - pointer of node constraints
  * \param szNodeName [in] - node name 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
 SYNCML_DM_RET_STATUS_T VerifyInteriorNodeConstraints(DMConstraints *pConstraints,
                                                      CPCHAR szNodeName);

 /**
  * Sets default value to data structure
  * \param oNodeMDF [in] - Meta node
  * \param oAddData [out] - data to be added or replaced with
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
 SYNCML_DM_RET_STATUS_T SetDefaultValue(const DMMetaDataNode & oNodeMDF,
                                        DMAddData & oAddData);
 
 /**
  * Verifies leaf node constraints
  * \param oNodeMDF [in] - Meta node
  * \param oAddData [out] - data to be added or replaced with
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
 SYNCML_DM_RET_STATUS_T VerifyLeafNodeConstraints(const DMMetaDataNode & oNodeMDF,
                                                  DMAddData & oAddData);

 /**
  * Verifies node constraints
  * \param oNodeMDF [in] - Meta node
  * \param pNode [in] - pointer on DM node to be verified
  * \param szNodeName [in] - node name
  * \param oAddData [out] - data to be added or replaced with
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
 SYNCML_DM_RET_STATUS_T VerifyConstraints(const DMMetaDataNode & oNodeMDF,
                                          DMNode* pNode,
                                          CPCHAR szNodeName,
                                          DMAddData & oAddData);


  /**
  * Verifyes if value is a digit 
  * \param oData [in] - value to be checked
  * \return TRUE if value correct 
  */
  BOOLEAN IsDigit(const DMBuffer & oData);

  /**
  * Verifyes if value is a boolean 
  * \param oData [in] - value to be checked
  * \return TRUE if value correct 
  */
  BOOLEAN IsBoolean(const DMBuffer & oData);

  /**
  * Verifyes if value is a float 
  * \param oData [in] - value to be checked
  * \return TRUE if value correct 
  */
  BOOLEAN IsFloat(const DMBuffer & oData);

  /**
  * Verifyes if value is a date 
  * \param oData [in] - value to be checked
  * \return TRUE if value correct 
  */
  BOOLEAN IsDate(const DMBuffer & oData);

  /**
  * Verifyes if value is a time 
  * \param oData [in] - value to be checked
  * \return TRUE if value correct 
  */
  BOOLEAN IsTime(const DMBuffer & oData);

  /**
  * Verifyes node name value constraint 
  * \param pConstraints [in] - pointer of node constraints
  * \param szNodeName [in] - node name
  * \return TRUE if node name is correct 
  */
  BOOLEAN VerifynValues(DMConstraints *pConstraints, 
                       const DMString & szNodeName);

  /**
  * Verifyes node value constraint 
  * \param pConstraints [in] - pointer of node constraints
  * \param format [in] - node format
  * \param oData [in] - node value
  * \return TRUE if value is correct 
  */
  BOOLEAN VerifyValues(DMConstraints *pConstraints, 
                       SYNCML_DM_FORMAT_T format, 
                       const DMBuffer & oData);

  /**
  * Verifyes foreign key constraint 
  * \param pConstraints [in] - pointer of node constraints
  * \param oData [in] - node value
  * \return TRUE if value is correct 
  */
  BOOLEAN VerifyForeignKey(DMConstraints * pConstraints,
                         const DMBuffer & oData);

  /**
  * Verifyes regular expression constraint 
  * \param pConstraints [in] - pointer of node constraints
  * \param szData [in] - value to check
  * \return TRUE if value is correct 
  */
  SYNCML_DM_RET_STATUS_T VerifyRegExp(CPCHAR szPattern, 
                                    CPCHAR szData);

  /**
  * Searches Meta Data node name in the children list 
  * \param oBuffer [in] - Meta Data buffer (MDF file)
  * \param szName [in] - node name
  * \return Return Type (UINT32)
  *  returns offset on node if name is found, 0 otherwise
  */
  UINT32 FindNodeInChildrenList(DMMetaDataBuffer oBuffer, 
                             CPCHAR szName) const;

  /**
  * Verifies hard and soft dependency constraints
  * \param szNodeName [in] - node name
  * \param szURI [in] - node path 
  * \param szOrigName [in] - original node name (in case of Rename)
  * \param callBackSoft [in] - method to call for soft dependency constraint
  * \param callBackHard [in] - method to call for hard dependency constraint
  * \param accessType [in] - access type
  * \return TRUE if constraints verified with success 
  */
  BOOLEAN VerifyChildDependency(CPCHAR szNodeName,
                                CPCHAR szURI,
                                CPCHAR szOrigName,
                                SYNCML_DM_MDF_CBACK callBackSoft,
                                SYNCML_DM_MDF_CBACK callBackHard,
                                SYNCML_DM_ACCESS_TYPE_T accessType);

  /**
  * Verifies hard or soft dependency constraints
  * \param szNodeName [in] - node name
  * \param szDependencies [in] - soft or hard dependency constraint 
  * \param szOrigName [in] - original node name (in case of Rename)
  * \param callBack [in] - method to call for constraint
  * \param callBackSoft [in] - method to call for hard dependency constraint
  * \return TRUE if constraints verified with success 
  */
  BOOLEAN VerifyDependency(CPCHAR szNodeName,
                           CPCHAR szDependecies,
                           CPCHAR szOrigName,
                           SYNCML_DM_MDF_CBACK callBack);

  /**
  * Verifies hard or soft dependency constraint for one specified path (from constraint)
  * \param pNode [in] - pointer on DM node to be verified
  * \param szURI [in] - node path 
  * \param szNodeName [in] - node name
  * \param szOneURI [in] - constraint path
  * \param szOrigName [in] - original node name (in case of Rename)
  * \param callBack [in] - method to call for dependency constraint
  * \param bIsMultiNode [in] - specifies if node is a multinode
  * \return TRUE if constraints verified with success 
  */
  BOOLEAN VerifyOneURIDependency(DMNode* pNode, 
                                 char * sURI,
                                 CPCHAR szNodeName,
                                 CPCHAR szOneURI,
                                 CPCHAR szOrigName,
                                 SYNCML_DM_MDF_CBACK callBack,
                                 BOOLEAN bIsMultiNode);

  /**
  * Verifies hard or soft dependency constraint for one specified path (from constraint) on plug-in node
  * \param pNode [in] - pointer on DM node to be verified
  * \param pPluginNode [in] - smart pointer on plug-in node
  * \param szURI [in] - node path 
  * \param szNodeName [in] - node name
  * \param szOneURI [in] - constraint path
  * \param szOrigName [in] - original node name (in case of Rename)
  * \param callBack [in] - method to call for dependency constraint
  * \param bIsMultiNode [in] - specifies if node is a multinode
  * \return TRUE if constraints verified with success 
  */
  BOOLEAN VerifyPluginURIDependency(DMNode* pNode, 
                                 PDmtNode pPluginNode,
                                 char * sURI,
                                 CPCHAR szNodeName,
                                 CPCHAR szOneURI,
                                 CPCHAR szOrigName,
                                 SYNCML_DM_MDF_CBACK callBack,
                                 BOOLEAN bIsMultiNode);

  /**
  * Builds start point of search of MDF node base on last MDF node accessed
  * \param szURI [in] - DM node path to be found in a cache
  * \param oBuffer [in] - Meta Data buffer (MDF file)
  * \param accessType [out] - accessType (is set if path is found in a cache) 
  * \param oNode [out] - Meta Node (is set if path is found in a cache)
  * \return Return Type (CPCHAR) 
  * returns tail segments of szURI from which manager continues seach, NULL if path isn't found in a cache. 
  * - All other codes indicate failure. 
  */
  CPCHAR BuildSearchURI(CPCHAR szURI, 
                    DMMetaDataBuffer & oBuffer, 
                    SYNCML_DM_ACCESS_TYPE_T & accessType,
                    DMMetaDataNode & oNode );

  /**
  * Searches MDF node
  * \param szURI [in] - node path to be found
  * \param oBuffer [in] - Meta Data buffer (MDF file)
  * \param parentAccessType [in] - access type from parent node
  * \param bCheckMultiNode [in] - specifies if max number of multinodes should be read
  * \param oNode [out] - found Meta Data node
  * \param pChildDependNodes [out] - pointer on list of hard and soft dependencies of node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T SearchNode(CPCHAR szURI, 
                                    DMMetaDataBuffer oBuffer, 
                                    SYNCML_DM_ACCESS_TYPE_T parentAccessType,
                                    BOOLEAN bCheckMultiNode,
                                    DMMetaDataNode & oNode,
                                    DMMetaPCharVector* pChildDependNodes );

  /**
  * Retrieves a  MDF node
  * \param szURI [in] - node path to be found
  * \param bCheckMultiNode [in] - specifies if max number of multinodes should be read
  * \param oNode [out] - found Meta Data node
  * \param pChildDependNodes [out] - pointer on list of hard and soft dependencies of node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T GetNode(CPCHAR szURI, 
                               BOOLEAN bCheckMultiNode, 
                               DMMetaDataNode & pNode, 
                               DMMetaPCharVector* pChildDependNodes = NULL);

  /**
  * Removes reccursive segments form search path
  * \param sTailSegments [in] - URI to remove reccursive segments
  * \param oNode [in] - Meta Data node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T RemoveRecursiveSegments(char* sTailSegment, 
                                                 DMMetaDataNode & oNode);

  /**
  * Unloads MDF files from memory
  */
   void UnLoad();

#ifndef DM_STATIC_FILES
   /**
  * Loads MDF files located in the specified directory
  * \param szDirectory [in] - name of directory
  * \param bIgnoreRoot [in] - specifies if root.bmdf should be ignored
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T LoadDir(CPCHAR szDirectory, BOOLEAN bIgnoreRoot);

   /**
  * Loads MDF file
  * \param szPath [in] - file name
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T LoadBuffer(CPCHAR szPath);
#else
   /**
  * Loads static MDF file
  * \param szPath [in] - file index
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T LoadBuffer(UINT32 index);
#endif

  /**
  * Loads MDF files
  */
  SYNCML_DM_RET_STATUS_T Load();

  /**
  * Verifies if specified node name is a value of other node
  * \param pNode [in] - pointer on DM node to be verified
  * \param pPluginNode [in] - smart pointer on plug-in node
  * \param szNodeName [in] - node name
  * \param szOrigName [in] - original node name (in case of Rename)
  * \return TRUE if name is not value of other node 
  */
  static BOOLEAN CheckFieldInUse(DMNode* pNode,
                                 PDmtNode pPluginNode, 
                                 CPCHAR szNodeName, 
                                 CPCHAR szOrigName);

  /**
  * Clears value of node if specified node name is a value 
  * \param pNode [in] - pointer on DM node to be verified
  * \param pPluginNode [in] - smart pointer on plug-in node
  * \param szNodeName [in] - node name
  * \param szOrigName [in] - original node name (in case of Rename)
  * \return TRUE if name is not value of other node 
  */
  static BOOLEAN ClearNodeValue(DMNode* pNode, 
                                PDmtNode pPluginNode, 
                                CPCHAR szNodeName, 
                                CPCHAR szOrigName);

  /**
  * Resets value of node if specified node name is a value 
  * \param pNode [in] - pointer on DM node to be verified
  * \param pPluginNode [in] - smart pointer on plug-in node
  * \param szNodeName [in] - node name
  * \param szOrigName [in] - original node name (in case of Rename)
  * \return TRUE if name is not value of other node 
  */
  static BOOLEAN ResetNodeValue(DMNode* pNode, 
                                PDmtNode pPluginNode, 
                                CPCHAR szNodeName, 
                                CPCHAR szOrigName);

   /**
  * Retrieves root node from MDF file
  * \param oNode [out] - Meta node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T GetRootNode(DMMetaDataNode & oNode);

  /**
  * Verifies and retrieves start position of path to search (skips "./" )
  * \param szURI [in] - Meta node
  * \return Return Type (CPCHAR) 
  * adjusted path, or NULL if URI isn't correct 
  */
  CPCHAR GetStartPos(CPCHAR szURI);

  /**
  * Finds path in the cache of last accessed node 
  * \param szURI [in] - DM node path to be found in a cache
  * \param bCheckMultiNode [in] - specifies if max number of multinodes should be read
  * \param oNode [out] - Meta Data node
  * \param oBuffer [out] - Meta Data buffer 
  * \param oNode [out] - Meta Node (is set if path is found in a cache)
  * \param pChildDependNodes [out] - pointer on list of hard and soft dependencies of node
   * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T FindCacheNode(CPCHAR szURI,
                                       BOOLEAN bCheckMultiNode, 
                                       DMMetaDataNode & oNode,
                                       DMMetaDataBuffer & oBuffer,
                                       DMMetaPCharVector* pChildDependNodes);
     
  /* Regular expression pattern for float values */
  static CPCHAR   m_pFloatPattern[];
  /* Regular expression pattern for date values */
  static CPCHAR   m_pDatePattern[];
  /* Regular expression pattern for time values */
  static CPCHAR   m_pTimePattern[];


  /* pointer on env object */
  CEnv* m_pEnv;
  /* pointer on DMTree object */
  DMTree* m_pTree;
  /* Specifies if MDF file is loaded */
  BOOLEAN m_bIsLoad;
  /* Vector that hold pointers on MDF buffers loaded into memory */
  DMMetaDataVector m_oDDFInfo;  
  /* Last accessed Meta Data node */
  DMLastMetaDataNode m_oLastNodeLocator;  
};

/*================================================================================================*/
#endif 
