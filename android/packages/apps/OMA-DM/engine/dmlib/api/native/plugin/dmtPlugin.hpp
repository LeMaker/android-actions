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

#ifndef __DMTPLUGIN_H__
#define __DMTPLUGIN_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/** 
  \file dmtPlugin.hpp
  \brief  The dmtPlugin.hpp is a helper  header file contains constants and data types\n
                of plugin API. Also it contains DmtPluginNode, DmtAPIPluginTree and \n
                DmtAPIPluginTree classes definition.\n
               <b>Warning:</b>  All functions, structures, and classes from this header file are for internal usage only!!!
   
<P>

Plugin DLLs' exported function types:<P>

\code

    //Mandatory function:
    int DMT_PluginLib_GetVersion();  //Should return DMT_PLUGIN_VERSION_1_1

    //Data plug-in:
    extern "C" 
    SYNCML_DM_RET_STATUS_T DMT_PluginLib_Data_GetTree(
    	const char * pluginRootNodePath, 
    	DMStringMap & mapParameters,	//For the Tree
    	PDmtAPIPluginTree & pPluginTree	      //root tree for the current path
    );

    DmtPluginTree and DmtPluginNode are only used for Data plug-ins.

    //Exec plug-in:
    extern "C" 
    SYNCML_DM_RET_STATUS_T DMT_PluginLib_Execute2(
    	const char * path, 
        DMStringMap & mapParameters,
        CPCHAR args, 
        CPCHAR szCorrelator,
        PDmtTree tree, 
        DMString & results);

    //Constraint plug-in:
    extern "C" 
    SYNCML_DM_RET_STATUS_T DMT_PluginLib_CheckConstraint(
    	const char * path, 
    	DMStringMap & mapParameters,
    	PDmtTree tree        //Global Tree with same access rights for current session
       );

    //Commit plug-in:
    extern "C" 
    SYNCML_DM_RET_STATUS_T DMT_PluginLib_OnCommit(
    	const DMSubscriptionVector &updatedNodes,
    	DMStringMap& mapParameters,
    	PDmtTree tree );
 \endcode
*/

#include "jem_defs.hpp"
#include "dmt.hpp"      // Use Dmt C++ API for Data Plugin

/**  Version definition that should be used by both DLL and plugin*/
#define DMT_PLUGIN_VERSION_1_1      0x00010001

/** Define DMStringMap for plugins*/
typedef  DMMap<DMString, DMString> DMStringMap;

/** Define DmtOverlayPluginData for plugins*/
typedef DMVector<unsigned char> DmtOverlayPluginData;

/** Overlay plugin synchronization data */
struct DmtOverlayPluginSyncData
{
/** possible values for a status */
  enum { 
  	/** status deleted */
  	enum_StatusDeleted, 
  	/** status unchanged */
  	enum_StatusUnchanged, 
  	/** status added */
  	enum_StatusAdded 
  };
   /** node name */
  DMString m_strNodeName;
   /** plugin data */
  DmtOverlayPluginData m_oData;  
   /** node status */
  UINT8     m_nStatus;  
};

/**
*  Device management plugin tree API class; inherited from  the DmtTree class.
* \par Category: General
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DmtAPIPluginTree : public DmtTree
{
 public:

  /**
  * Will be called by DM Engine whenever in prior to add multi-nodes with flag "storesPD".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - node path
  * \param data [in] - reference to a node data
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T OnAdd( CPCHAR path, DmtOverlayPluginData& data );

  /**
  * Will be called by DM Engine whenever in prior to delete multi-nodes with flag "storesPD".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - node path
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T OnDelete( CPCHAR path );

  /**
  * Will be called by DM Engine to make synchronization in prior execution other functions.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - node path
  * \param data [in] - vector with data
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T Synchronize( const char* path, DMVector<DmtOverlayPluginSyncData>& data );
  
  /**
  * Called whenever plugin is unloaded to help free all resources
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual void Release();
 
 /**
  * Flushs memory
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T Flush();

  /**
  * Finds added node
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - CPCHAR path to added node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T FindAddedNode(CPCHAR path);

  /**
  * Finds added parent node
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - CPCHAR path to added parent node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T FindAddedParentNode(CPCHAR path);

  /**
  * Sets added node
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param ptrNode [in] - reference to a node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T SetAddedNode(PDmtNode ptrNode);

  /**
  * Removes added node
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - CPCHAR path to added node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T RemoveAddedNode(CPCHAR path);


  /**
  * Sets principal for the plugin tree
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param strPrincipal [in] - principal string for the session
  * \return  Return Type (BOOLEAN) 
  * - TRUE -set principal successful
  * - FALSE -set principal fail
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual BOOLEAN SetPrincipal( CPCHAR strPrincipal );

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
   virtual DmtPrincipal GetPrincipal() const;


protected: 
 /** DmtAPIPluginTree class member contains	list of added nodes*/
  DMVector<PDmtNode> m_oAddedNodes;
 /** DmtAPIPluginTree class member contains	principal*/
  DmtPrincipal m_Principal;
  
};

/** Define PDmtAPIPluginTree for plugins*/
typedef JemSmartPtr< DmtAPIPluginTree > PDmtAPIPluginTree;


class DmtPluginTree;
class DmtPluginNode;

/** Define Dmt Plugin Tree smart pointer for plugins*/
typedef JemSmartPtr< DmtPluginTree > PDmtPluginTree;

/** Define Dmt Plugin Node smart pointer for plugins*/
typedef JemSmartPtr< DmtPluginNode > PDmtPluginNode;


/** Has parent DmtAPIPluginTree class; handle operation with nodes and other tasks:\n
*    1. Provide default implementation for many DmtTree virtual functions for ease of plugin development.\n
*    2. Support default power failure recovery internally. \n
*    3. Support plugin node to check parameters for each different node (GerNodeParameters).
* \par Category: General
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/

class DmtPluginTree : public DmtAPIPluginTree
{
   //With some default Implementation of the DmtTree API including helper functions
protected:

   DMString m_strRootPath;
   DMMap<DMString, PDmtNode> m_Nodes;
   
   /** Protected destructor */
   virtual ~DmtPluginTree();
   
public:
	
  /**
  * Default constructor - no memory allocation performed.
  */
   DmtPluginTree();
  
public:

/**
  * Initializes DM tree
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param rootNodePath [in] - path to root node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_DEVICE_FULL - indicating the operation cannot be performed. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T Init( CPCHAR rootNodePath );

  /**
  * Retrieves full path to node
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to a node
  * \param fullPath [out] - full path to a node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetFullPath( CPCHAR path, 
                                                 DMString & fullPath ) const;

  /**
  * Retrieves  node by given path
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to a node
  * \param ptrNode [out] - reference to a node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_NOT_FOUND - indicating the operation cannot be performed. \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetNode( CPCHAR path, 
                                              PDmtNode& ptrNode );

  /**
  * Retrieves  child nodes names by given path
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to a node
  * \param mapNodes [out] - vector with child nodes
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetChildNodeNames( CPCHAR path, 
                                                           DMStringVector& mapNodes );
  /**
  * Default atomicity support will be provided here
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T Flush();

  /**
  * Checks for atomicity.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return '"false", not supported.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual BOOLEAN IsAtomic() const;

  /**
  * Begins an atomic operation that will end with commit() or rollback();
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * returns an error code if a transaction is running already.
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T Begin();

  /**
  * The method will commit a series of atomic operations
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T Commit();

  /**
   * The method will rollback a series of atomic operations
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T Rollback() ;

  /**
   * Deletes a node according to the specified path  
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - full path to the node
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T DeleteNode( CPCHAR path );
  
  /**
  * Changes node's name. For example: RenameNode( "./SyncML/DMAcc/Test", "NewTest" );
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param szNewNodeName [in] - new node name
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T RenameNode( CPCHAR path, 
                                                   CPCHAR szNewNodeName );

  /**
   * Creates an interior node in the tree.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param ptrCreatedNode [out] - new created node
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T CreateInteriorNode( CPCHAR path, 
                                                         PDmtNode& ptrCreatedNode );


  /**
   * Creates a leaf node in the tree.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param ptrCreatedNode [out] - new created node
  * \param value [in] - data value DmtData type
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T CreateLeafNode( CPCHAR path, 
                                                      PDmtNode& ptrCreatedNode, 
                                                      const DmtData& value );

  /**
   * Creates a leaf node in the tree.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param ptrCreatedNode [out] - new created node
  * \param value [in] - data value DmtData type
  * \param isESN [in] - TRUE if path is to an ESN node, otherwise -FALSE
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T CreateLeafNode(CPCHAR path, 
						   PDmtNode& ptrCreatedNode, 
						   const DmtData& value ,
						   BOOLEAN isESN);

   /**
  * Releases DM tree
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual void Release();

public:
	
   //Other Helper Function Implementation for the Tree, NOT used
   
  /**
  * <b>NOT used</b>. Creates a sibling of the node specified by its URI "path". 
  * This new node's name is user-specified as "szNewNodename".
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - URI of node to be cloned.
  * \param szNewNodename [in] - new node name as specified by user.
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T Clone(CPCHAR path, 
                                          CPCHAR szNewNodename);
   
  /**
  * <b>NOT used</b>. . This is a  helper method. It will return a table of all leaf nodes of the current node.
  * The table key is the child node name, and value is the string node value. It sets leaf nodes only.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param mapNodes [out] - map with leaf nodes only
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetChildValuesMap( CPCHAR path, 
                                                          DMMap<DMString, DmtData>& mapNodes ); 
  
  /**
  * <b>NOT used</b>. This is a  helper method. It will delete all leaf nodes and creates new which are provided in the map.
  * The table key is the child node name, and value is the node value. It changes leaf nodes only.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param mapNodes [in] - map with leaf nodes only
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T SetChildValuesMap( CPCHAR path, 
                                                          const DMMap<DMString, DmtData>& mapNodes ); 

public:
   //
   //Additional API for Recovery and 2-phase commit for multiple plugins.
   //For future expansion. Currently not used
   //
   /**
  * Additional API for Recovery and 2-phase commit for multiple plugins. 
  * \warning This API is for the future expansion  and currently <b>NOT used</b>.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T Verify();

  
   //Additional API to shield plugins from accessing engine functions directly
   // i.e. to get parameters for each plugin Node
   // this is more generic since they can query *ANY* other parameters
 /**
  * Additional API to shield plugins from accessing engine functions directly  (i.e. to get parameters for each plugin Node). 
  * This is more generic since they can query *ANY* other parameters.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param name [in] - parameter name
  * \param value [out] - parameter value
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_NOT_FOUND - indicating the operation cannot be performed. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetParameter(CPCHAR name, 
                                                  DMString & value);

   //
   // Helper set Functions for shared library developer to use
   //
 /**
  *  This is a  helper method. It will set Functions for shared library developer to use.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - node path
  * \param node [out] - node to be set
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */

   SYNCML_DM_RET_STATUS_T SetNode(CPCHAR path,    
                                    PDmtNode node);

 /**
  * Removes node fron the tree
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - node path
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   SYNCML_DM_RET_STATUS_T RemoveNode(CPCHAR path);

 /**
  * Removes all nodes
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
   SYNCML_DM_RET_STATUS_T ClearNodes();

};


/**
* Class DmtPluginNode for default read only plugin node implementation.
* \par Category: General
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DmtPluginNode : public DmtNode
{ 
protected:
   BOOLEAN   m_bLeaf;
   DMString  m_strPath;
   PDmtPluginTree  m_ptrTree;
   DMString  m_strName;
   DmtData   m_oData;
   DMString  m_strTitle;
   DmtAttributes m_oAttr;
   BOOLEAN  m_bESN;   
   /** Protected destructor is changed to public destructor*/
public:
   virtual ~DmtPluginNode();


public:
  /**
  * Default constructor - no memory allocation performed.
  */
   DmtPluginNode(); 
  
 /**
  * Initializes plugin
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param  ptrTree [in] - DM tree pointer
  * \param path [in] - path
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T Init(PDmtPluginTree ptrTree, 
                                      CPCHAR path);
 /**
  * Initializes plugin
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param ptrTree [in] - DM tree pointer
  * \param path [in] - path
  * \param isleaf [in] - TRUE if path is to leaf node, otherwise -FALSE
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T Init(PDmtPluginTree ptrTree, 
                                      CPCHAR path,
                                      BOOLEAN isleaf);

   /**
  * Initializes plugin
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param ptrTree [in] - DM tree pointer
  * \param path [in] - path
  * \param oData [in] - node data
  * \param isESN [in] - TRUE if path is to an ESN node, otherwise -FALSE
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T Init(PDmtPluginTree ptrTree, 
                                      CPCHAR path,
                                      const DmtData & oData,
                                      BOOLEAN isESN = FALSE); 
 /**
  * Initializes plugin
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param ptrTree [in] - DM tree pointer
  * \param path [in] - path
  * \param childNodeNames [in] - vector with child node names
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T Init(PDmtPluginTree ptrTree, 
                                      CPCHAR path,
                                      const DMStringVector & childNodeNames);

 
  /**
  * Could set empty DmtData object if there is no value associated with it.
  * The DmtValue is a copy of current data of the node. The node value will not 
  * be changed until a <i>setValue()</i> is called.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param oData [out] - reference to DmtData object
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetValue( DmtData& oData ) const;
	 

   //Attributes May also need to be replaced IF DmtData size is not WWW
 /**
  * Gets a copy of DmtAttributes, user can modify the DmtAttributes individually
  * but any change made to DmtAttributes will not propagated to the Node until a <i>setAttributes()</i> is called.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param  oAttr [out] - reference to DM tree attributes
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetAttributes( DmtAttributes& oAttr ) const;

 /** 
  * Retrieves tree
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param ptrTree [out] - reference to the DM tree
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetTree( PDmtTree& ptrTree ) const;
 
  /** 
  * Retrieves path
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [out] - reference to path in the tree
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_DEVICE_FULL - indicating the operation cannot be performed. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetPath(DMString & path) const;

   /**
  * Updates title information for the node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szTitle [in] - node titles string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T SetTitle( CPCHAR szTitle );

  /**
   * Updates ACL for the node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param oAcl [in] - reference to DMT ACL object
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T SetAcl( const DmtAcl& oAcl );
  
  /**
  * Changes the value of a node. If not successful, return an error code
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param value [in] - new node value 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T SetValue( const DmtData& value );

  /**
  * Fills in oChildren list of child nodes
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param oChildren [out] - vector for child nodes
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetChildNodes( DMVector<PDmtNode>& oChildren ) const;

  /**
   * Checks if a node is leaf
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return TRUE if the node is a leaf, otherwise - FALSE
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual BOOLEAN IsLeaf() const;

  /**
   * Executes a node according to the specified path. Pass a String parameter to the executable code  
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param strData [in] - data to be executed as a string
  * \param result [out] - the result of executing will be set to this string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T Execute( CPCHAR strData, DMString& result );

  /**
  * Gets the name of the node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * param node [out] - name string 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_DEVICE_FULL - indicating the operation cannot be performed. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetNodeName(DMString & name) const;

  /**
  * Gets  name of a node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return  name of a node as DMString.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual DMString GetNodeName() const { return m_strName; };

  /**
   * Renames a node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * param szNewName [in] - new name 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   SYNCML_DM_RET_STATUS_T Rename( CPCHAR szNewName );
  
  /**
   * Sets child node object by name 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szPath [in] - name of node
  * \param ptrNode [out] - reference to DmtNode
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetChildNode( CPCHAR szPath, PDmtNode& ptrNode  );
  
  /**
  * Gets first chunk of an ESN (External Storage Node). 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param dmtChunkData [out] - reference to dmtChunkData
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetFirstChunk(DmtDataChunk&  dmtChunkData); 
  /**
   * Gets next chunk of an ESN  (External Storage Node).
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param dmtChunkData [out] - reference to dmtChunkData
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T GetNextChunk(DmtDataChunk& dmtChunkData); 

  /**
  * Sets first chunk of an ESN  (External Storage Node). 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param dmtChunkData [in] - reference to dmtChunkData
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T SetFirstChunk(DmtDataChunk& dmtChunkData);  
  
  /**
  * Sets next chunk of an ESN  (External Storage Node).  
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param dmtChunkData [in] - reference to dmtChunkData
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T SetNextChunk(DmtDataChunk& dmtChunkData);  
  
  /**
  * Sets last chunk of an ESN  (External Storage Node). 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param dmtChunkData [in] - reference to dmtChunkData
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T SetLastChunk(DmtDataChunk& dmtChunkData);  
  
  /**
   * Checks if a Node is  an ESN  (External Storage Node).  
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (BOOLEAN) \n
  * - TRUE - indicating it is an ESN node. \n
  * - FALSE - not an ESN
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   boolean IsExternalStorageNode(void)  const {return m_bESN;}
  
private :
   SYNCML_DM_RET_STATUS_T InitAttributes(SYNCML_DM_DATAFORMAT_T type); 
};


/**
* Engine side support for Overlay plug-ins including meta node ID and PD retrieval
*/
struct DmtOPINodeData
{
/**  plugin data*/
  DMVector< DmtOverlayPluginData >  aPD;
/** meta ID of a node */
  int   metaNodeID;
};

  /**
  * Function should be called only from the plug-in "getNode" function
  * \warning This method for internal usage only!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return cached PDs and metaNodeID (or -1 if not set) for the current node
  * \par Prospective Clients:
  * For internal usage only
  */
extern "C" const DmtOPINodeData* DmtGetCachedOPINodeData();

  /**
  * Allows to update plug-in data in DMT
  * \warning This method for internal usage only!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szURI [in] - node URI
  * \param oData [out] - node data
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * For internal usage only
  */
extern "C" SYNCML_DM_RET_STATUS_T DmtSetOPINodeData( CPCHAR szURI, const DmtOverlayPluginData& oData );

#endif
