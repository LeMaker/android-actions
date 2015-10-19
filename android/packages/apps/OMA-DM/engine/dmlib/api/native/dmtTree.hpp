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

#ifndef __DMTTREE_H__
#define __DMTTREE_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/**  
   \file dmtTree.hpp
   \brief  The dmtTree.hpp header file contains DmtTree class  definition. <br>
                 DmtTree is the object carrying atomicity (when supported) and<br>
                 authentication functionality associated with DMT access, as well<br>
                 as basic node access.
*/

#include "jem_defs.hpp"

#include "dmt.hpp"


/**
DmtTree is the object carrying atomicity (when supported) and
authentication functionality associated with DMT access, as well
as basic node access. The object is obtained from the static create() 
method of the DmtTree's implementation class.
Almost all methods return a smart pointer to the 
error description object if failed, NULL if succeeded

\par Category: General

\par Persistence: Transient

\par Security: Non-Secure

\par Migration State: FINAL


\par Creating DmtTree object

The factory class DmtTreeFactory is used to create a tree object. \n
Alternatively, a sub-tree can be created using the GetSubTree method 
 to minimize blocking.

\par Release DmtTree object

 DmtTree is a smart-pointer. Smart-pointers are used in the DMT API to 
 return references to DmtTree, DmtNode and DmtErrorDescription. Usually you 
 can treat it as a normal pointer to a class, but in fact, it’s a small class 
 which takes care of the object’s life time. Here are some simple rules for 
working with smart pointers:\n\n

    - warning never call delete on a smart pointer\n
    - warning assign NULL to a smart pointer as soon as you don’t need the object anymore \n\n

 After you have got a hold of a DmtTree object, you can access its child nodes and
 read and modify them as desired. However it is important to release the object by
assigning NULL to it after you have finished “get” operations, i.e. read only type of
 operations. In case you have done “set” operations, i.e. write type of operations, a
 call to Flush() will “commit” the change and release the lock on the tree. \n
 Remember to:\n
 - release (assign NULL) after read\n
 - call Flush() after write\n

 \par Example using DMT functions

  \code
   DmtPrincipal principal("localhost");
   PDmtTree ptrTree;
   SYNCML_DM_RET_STATUS_T  ret_status;

   if ( (ret_status = DmtFactory::GetTree(principal, ptrTree ) ) != SYNCML_DM_SUCCESS )
   {
     ... error handling
     return;
   }

   PDmtNode ptrNode;
   
   if ( (ret_status = ptrTree->GetNode( "./SyncML/DMAcc/GUID/AddrType", ptrNode ) ) != SYNCML_DM_SUCCESS )
   {
      ... error handling
     return;
   }
   
   std::string str;
   if ( (ret_status = ptrNode->GetStringValue( str )) != SYNCML_DM_SUCCESS )
   {
      ... error handling
     return;
   }
   
   printf( "String value is %`s\n", str.c_str() );
 \endcode
 
*/


class DmtTree : public JemBaseObject
{ 
protected:
/** Destructor - freeing all dynamic resources */
  virtual ~DmtTree(){}

public:
  /**
   * Locates and return node by specified path.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - full path to the node
  * \param ptrNode [out] - result node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T  GetNode(CPCHAR path, PDmtNode& ptrNode )=0;

  /**
  * Deletes a node according to the specified path  
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - full path to the node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  * \par Example of how DeleteNode() function would be called
  *
  * \code
  * PDmtTree ptrTree;
  * SYNCML_DM_RET_STATUS_T  ret_status;
  * if ( (ret_status=DmtTreeFactory::GetSubtree(principal, "./SyncML/DMAcc”, ptrTree))!=SYNCML_DM_SUCCESS  ){
  *  ... error processing here
  * return;
  * }
  * if ( (ret_status=ptrTree->DeleteNode( "TEST" ) ) != SYNCML_DM_SUCCESS) {
  *    ... error
  *    return;
  * }
  * \endcode
  */
  virtual SYNCML_DM_RET_STATUS_T DeleteNode(CPCHAR path )=0;


  /**
   * Creates a sibling of the node specified by its URI "path".
   * This new node's name is user-specified as "szNewNodename".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - URI of node to be cloned.
  * \param szNewNodename [in] - new node name as specified by user.
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T Clone(CPCHAR path, CPCHAR szNewNodename) = 0;


  /**
  * Changes node's name. For example: RenameNode( "./SyncML/DMAcc/Test", "NewTest" );
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param szNewNodeName [in] - new node name
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T RenameNode(CPCHAR path, CPCHAR szNewNodeName ) = 0;

  /**
  * Creates a leaf node in the tree.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param ptrCreatedNode [out] - new created node
  * \param value [in] - data value DmtData type
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  * \par Example of how this function would be called
  *
  * \code
  * DmtPrincipal principal("localhost");
  *   PDmtTree ptrTree;
  *   SYNCML_DM_RET_STATUS_T  ret_status;
  *
  *   if ( (ret_status = DmtFactory::GetTree(principal, ptrTree ) ) != SYNCML_DM_SUCCESS )
  *   {
  *     ... error handling
  *     return;
  *   }
  * PDmtNode ptrNode;
  * if ( (ret_status =ptrTree->CreateLeafNode( "./SyncML/DMAcc/TESTLeaf", ptrNode, DmtData(123) ) ) != SYNCML_DM_SUCCESS) ){
  *      … error handling
  *     return;
  * }
  * if ( (ret_statu= ptrTree->Flush() SYNCML_DM_SUCCESS) ){
  *  .... error processing here …
  * }
  * \endcode
  *
  */
  virtual SYNCML_DM_RET_STATUS_T CreateLeafNode(CPCHAR path, PDmtNode& ptrCreatedNode, const DmtData& value )=0;
  
  /**
  * Creates a External Storage leaf node in the tree.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param ptrCreatedNode [out] - new created node
  * \param value [in] - data value DmtData type
  * \param isESN [in] - flag to indicate that it is External Storage Node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  *
  * \code
  * DmtPrincipal principal("localhost");
  *   PDmtTree ptrTree;
  *   SYNCML_DM_RET_STATUS_T  ret_status;
  *
  *   if ( (ret_status = DmtFactory::GetTree(principal, ptrTree ) ) != SYNCML_DM_SUCCESS )
  *   {
  *     ... error handling
  *     return;
  *   }
  * PDmtNode ptrNode;
  * if ( (ret_status =ptrTree->CreateLeafNode( "./SyncML/DMAcc/TESTLeaf", ptrNode, DmtData(123), TRUE ) ) != SYNCML_DM_SUCCESS) ){
  *     … error handling
  *     return;
  * }
  * if ( (ret_statu= ptrTree->Flush() SYNCML_DM_SUCCESS) ){
  *  .... error processing here …
  * }
  * \endcode
  */
  virtual SYNCML_DM_RET_STATUS_T CreateLeafNode(CPCHAR path, PDmtNode& ptrCreatedNode, const DmtData& value , BOOLEAN isESN)=0;

  /**
  * Creates an interior node in the tree.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param ptrCreatedNode [out] - new created node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  *
  * \par Example of how this function would be called
  *
  * \code
  * DmtPrincipal principal("localhost");
  *   PDmtTree ptrTree;
  *   SYNCML_DM_RET_STATUS_T  ret_status;
  *
  *   if ( (ret_status = DmtFactory::GetTree(principal, ptrTree ) ) != SYNCML_DM_SUCCESS )
  *   {
  *     ... error handling
  *     return;
  *   }
  * PDmtNode ptrNode;
  * if ( ((ret_status =ptrTree->CreateInteriorNode( "./SyncML/DMAcc/TEST", ptrNode ) )  != SYNCML_DM_SUCCESS) {
  *     ... error handling
  *     return;
  * }
  * if ( (ret_statu= ptrTree->Flush() SYNCML_DM_SUCCESS) ){
  *  .... error processing here …
  * }
  * \endcode
  *
  */
  virtual SYNCML_DM_RET_STATUS_T CreateInteriorNode(CPCHAR path, PDmtNode& ptrCreatedNode )=0;

  /**
  * This is a helper method. It returns a table of all leaf nodes for the current node.
  * The table key is the child node name, and value is the string node value.  It sets leaf nodes only
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param mapNodes [out] - map with leaf nodes only
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  *
  * \par Example of how GetChildValuesMap() function would be called
  *
  * \code
  * PDmtTree ptrTree;
  * PDmtNode ptrNode;
  * DMString strDefaultProfile;
  * SYNCML_DM_RET_STATUS_T  ret_status;
  * .... initialisation ptrTree and ptrNode...
  *
  * if ( (ret_status=ptrNode->GetStringValue(strDefaultProfile) ) != SYNCML_DM_SUCCESS) {
  *     ... error
  *     return;
  *}
  * DMString strProfileURI = “Profiles/”;
  * strProfileURI += strDefaultProfile;
  * DMMap<DMString, DmtData> map;
  * if ( (ret_status=ptrTree-> GetChildValuesMap (strProfileURI, map ))!=SYNCML_DM_SUCCESS ){
  *    ...error
  *    return;
  * }
  * \endcode
  */
  virtual SYNCML_DM_RET_STATUS_T GetChildValuesMap(CPCHAR path, DMMap<DMString, DmtData>& mapNodes ) = 0; 


  /**
  * This is a helper method. It deletes all leaf nodes and creates new ones, provided in the map. The table key is the child node name, and value is the node value.  
  * It changes leaf nodes only
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param mapNodes [in] - map with leaf nodes only
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  *
  * \par Example of how GetChildValuesMap() function would be called
  *
  * \code
  * ... all init the same as with GetChildValuesMap() function...
  * if ( (ret_status=ptrTree-> SetChildValuesMap (strProfileURI, map ))!=SYNCML_DM_SUCCESS ){
  *    ...error
  *    return;
  * }
  * \endcode
  */
  virtual SYNCML_DM_RET_STATUS_T SetChildValuesMap(CPCHAR path, const DMMap<DMString, DmtData>& mapNodes ) = 0; 

  /**
  * This is a helper method. It sets an array of all child node names for the current node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param mapNodes [out] - map with leaf nodes only
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T GetChildNodeNames(CPCHAR path, DMStringVector& mapNodes ) = 0;

  /**
  * Flush all changes to the persistent layer and check the constrains.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T Flush()=0; 
  
  /**
  * Commits a series of atomic operations.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T Commit()=0; 

  /**
  * Rollbacks a series of atomic operations
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T Rollback()=0;

  /**
  * Begins an atomic operation that will end with commit() or rollback();
  * \warning An error code will be returned if a transaction is running already.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T Begin()=0;

  /**
  * Checks for atomicity.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return 'true' if the begin() was invoked, but no commit() or rollback().
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual BOOLEAN IsAtomic() const =0;

  /**
  * Retrieves a principal
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return the DmtPrincipal object that the session was created with.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual DmtPrincipal GetPrincipal() const =0;

  /**
   * Function checks if specified path is "gettable" in the context of current tree
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
   * \param path [in] - path to a node
   * \return "true" if corresponding call GetNode is successful, otherwise "false"
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  inline BOOLEAN IsValidNode ( const char* path ) {
    PDmtNode ptrNode;
    return (GetNode( path, ptrNode ) == SYNCML_DM_SUCCESS ? TRUE : FALSE) ;
  }
};

#endif
