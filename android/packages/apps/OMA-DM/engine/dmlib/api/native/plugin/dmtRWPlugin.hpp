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

#ifndef __DMTRWPLUGIN_H__
#define __DMTRWPLUGIN_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/** 
  \file dmtRWPlugin.hpp
  \brief Constants and datatypes for plugin API.\n
       The dmtRWPlugin.hpp header file contains constants and data types  of plugin API. \n
       Also it contains DmtRWPluginNode, DmtRWPluginTree classes definition. \n
       <b>Warning:</b> All functions, structures, and classes from this header file are for internal usage only!!!

       The <i>DmtRWPluginTree</i> is a base plugin tree class with read/write functionality; \n
       inheritance from  DmtPluginTree \n
       The <i>DmtRWPluginNode</i> is a base plugin node class with read/write functionality; \n
       inheritance from  DmtPluginNode \n
*/

#include "dmtPlugin.hpp"  

class DmtRWPluginTree;
class DmtRWPluginNode;

/** Define DMT RW Plugin Tree smart pointer for plugins*/
typedef JemSmartPtr< DmtRWPluginTree > PDmtRWPluginTree;

/** Define DMT RW Plugin Node smart pointer for plugins*/
typedef JemSmartPtr< DmtRWPluginNode > PDmtRWPluginNode;

class SyncML_PlugIn_WBXMLLog;
class DMFileHandler;

/**
* Based plugin tree class with read/write functionality. This class is inherited from the "DmtPluginTree".
* \par Category: General
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/

class DmtRWPluginTree : public DmtPluginTree
{
   //With some default Implementation of the DmtTree API including helper functions
private:
	  SYNCML_DM_RET_STATUS_T DeleteSubTree( PDmtNode ptrNode);
	  BOOLEAN IsESNSetComplete(CPCHAR pbURI);
	  SYNCML_DM_RET_STATUS_T CommitESN(CPCHAR pbURI);
	  SYNCML_DM_RET_STATUS_T InitLog ();
	  DMFileHandler *fpLog;
	  SyncML_PlugIn_WBXMLLog* log;
	  BOOLEAN  m_Playback;
	  BOOLEAN  m_ESNDirty;
	  DMString m_strLogPath;
	  BOOLEAN  m_bIsAtomic;
protected:
	
/**
* Protected destructor
*/
  virtual ~DmtRWPluginTree();
  
public:

  /**
  * Default constructor - no memory allocation performed.
  */
   DmtRWPluginTree();

  /**
  * Collects all commands that has been executed on plugin tree for the recovery and rollback actions.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param type [in] - plugin command type
  * \param pbURI [in] -node URI
  * \param attribute [in] - plugin command attribute
  * \param inNode [in] - pointer to DMT node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual  SYNCML_DM_RET_STATUS_T LogCommand(SYNCML_DM_PLUGIN_COMMAND_T type,
                         CPCHAR pbURI,
                         SYNCML_DM_PLUGIN_COMMAND_ATTRIBUTE_T attribute,
                         const DmtNode * inNode);

/**
  * For RW Trees, default atomicity support will be provided here.
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
   virtual SYNCML_DM_RET_STATUS_T Flush();

/**
  * Begins an atomic operation that will end with commit() or rollback();
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
   virtual SYNCML_DM_RET_STATUS_T Begin();

  /**
  * Commits a series of atomic operations
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
   virtual SYNCML_DM_RET_STATUS_T Commit();

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
   virtual SYNCML_DM_RET_STATUS_T Rollback();


  /**
  * Retrieves if operation is atomic
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
   virtual BOOLEAN IsAtomic() const;


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
  */
   virtual SYNCML_DM_RET_STATUS_T DeleteNode( CPCHAR path );

  /**
  * Changes node's name. For example: <i>RenameNode( "./SyncML/DMAcc/Test", "NewTest" )</i>;
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
   virtual SYNCML_DM_RET_STATUS_T RenameNode( CPCHAR path, 
                                            CPCHAR szNewNodeName );
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
  */
   virtual SYNCML_DM_RET_STATUS_T RemoveNode(CPCHAR path);

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
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T CreateLeafNode( CPCHAR path, 
                                                PDmtNode& ptrCreatedNode, 
                                                const DmtData& value );
  /**
  * Creates a leaf node in the tree.
  * \par Important Notes:
  * -Note: This method provides default log implementation.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param ptrCreatedNode [out] - new created node
  * \param value [in] - data value DmtData type
  * \param isESN [in] - TRUE if it is an ESN, otherwise set to FALSE.
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  virtual SYNCML_DM_RET_STATUS_T CreateLeafNode(CPCHAR path,
  							PDmtNode& ptrCreatedNode,
  							const DmtData& value ,
  							BOOLEAN isESN);

  /**
  * Creates a leaf node in the tree.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param ptrCreatedNode [out] - new created node
  * \param value [in] - data value DmtData type
  * \param isESN [in] - TRUE if it is an ESN, otherwise set to FALSE.
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T CreateLeafNodeInternal( CPCHAR path, 
                                                             PDmtNode& ptrCreatedNode, 
                                                             const DmtData& value ,
                                                             BOOLEAN isESN);
  
#ifdef LOB_SUPPORT
  /**
  * Creates temporary storage for an ESN.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   SYNCML_DM_RET_STATUS_T BackupESNdata( CPCHAR path);

  /** 
  * Indicates the ESN value is modified...
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   void SetESNDirty() {m_ESNDirty = TRUE;}
#endif


  /**
  * Sets handler to the log file.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param fileHandle [in] - handler to the log file
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T setLogFileHandle(DMFileHandler *fileHandle);

  /**
  * Creates an interior node in the tree.
  * \par Important Notes:
  * -Note: The plugin developers should provide implementation !!!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param ptrCreatedNode [out] - new created node
  * \param childNodeNames [in] - vector with child nodes
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T CreateInteriorNodeInternal( CPCHAR path, 
                                                            PDmtNode& ptrCreatedNode, 
                                                            const DMStringVector & childNodeNames);
   
  /**
  * Creates a leaf node in the tree.
  * \par Important Notes:
  * -Note: The plugin developers should provide implementation !
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \param ptrCreatedNode [out] - new created node
  * \param value [in] - data value DmtData type
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T CreateLeafNodeInternal( CPCHAR path, 
                                                             PDmtNode& ptrCreatedNode, 
                                                             const DmtData& value );

  /**
  * Creates link (virtual) to a parent  for the given node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param path [in] - path to the node
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */   
   SYNCML_DM_RET_STATUS_T LinkToParentNode( CPCHAR path);

 /**
  * Additional API for Recovery and 2-phase commit for multiple plugins. 
  * There is a default implementation using backward for forward logging.
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
   virtual SYNCML_DM_RET_STATUS_T Verify();    


   /**
  * Retrieves if plugin is in a playback mode (recovery or rollback).
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (BOOLEAN)
  *  - TRUE - if plugin is in the playback mode
  *  - FALSE - if plugin is NOT in the playback mode
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   BOOLEAN IsPlaybackMode();

   /**
  * Sets plugin in playback mode (recovery or rollback).
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param bPlayback [in] -  true or false
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   SYNCML_DM_RET_STATUS_T SetPlaybackMode(boolean bPlayback);
};



/**
* Base plugin node class with the read/write functionality. This class is inherited from the "DmtPluginNode".
* \par Category: General
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DmtRWPluginNode :public DmtPluginNode
{ 
private:
#ifdef LOB_SUPPORT
  BOOLEAN	m_LobComplete;
  BOOLEAN	m_LobDirty;
  BOOLEAN	m_LobLogging;
  DMString abStorageName;
#endif
    SYNCML_DM_RET_STATUS_T RenameChildNodes( const char* szParentPath, const char* szNodeName );

protected:
   /** Protected destructor */
  virtual ~DmtRWPluginNode();
  
public:
  /**
  * Default constructor - no memory allocation performed.
  */
   DmtRWPluginNode();

/**
  * Updates title information for the node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szTitle [in] - node titles string
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T SetTitle( CPCHAR szTitle );

  /**
  * Changes the value of a node. If not successful, return an error code
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param value [in] - new node value 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   virtual SYNCML_DM_RET_STATUS_T SetValue( const DmtData& value ); 

 /**
  * Changes the value of a node. If not successful, return an error code
  * \par Important Notes:
  * -Note: The plugin developers should provide implementation !
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param value [in] - new node value 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   SYNCML_DM_RET_STATUS_T SetValueInternal( const DmtData& value );

  /**
  * Renames a node.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * param szNewName [in] - new name 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
   SYNCML_DM_RET_STATUS_T Rename( CPCHAR szNewName );
  
#ifdef LOB_SUPPORT

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
    SYNCML_DM_RET_STATUS_T GetFirstChunk(DmtDataChunk&  dmtChunkData);

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
    SYNCML_DM_RET_STATUS_T GetNextChunk(DmtDataChunk& dmtChunkData); 

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
    SYNCML_DM_RET_STATUS_T SetFirstChunk(DmtDataChunk& dmtChunkData);  

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
    SYNCML_DM_RET_STATUS_T SetNextChunk(DmtDataChunk& dmtChunkData);	

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
    SYNCML_DM_RET_STATUS_T SetLastChunk(DmtDataChunk& dmtChunkData);	
  
  /**
  * Checks if the settings of all the ESN(External Storage Node)  under a subtree are complete.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (BOOLEAN) \n
  * - TRUE - indicate that settings are complete. \n
  * - FALSE - indicate that  settings are not complete. \n
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    BOOLEAN IsESNSetComplete() const { return m_LobComplete;}
	
  /**
  * The method will commit a series of atomic operations.
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
  * Place holder (for internal usage only !).
  * \warning This method should not be called !
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * Internal classes
  */
    virtual SYNCML_DM_RET_STATUS_T Delete();

  /**
  *  Gets the backup file name of an ESN.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return backup file name
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    CPCHAR GetESNBackupFileName(void) const {return ( !m_LobLogging &&  abStorageName != NULL) ? abStorageName.c_str() : NULL;};

  /**
  * Backs  all ESN data of a subteee.
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return status code. Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - SYNCML_DM_FEATURE_NOT_SUPPORTED - indicate that this operation is not supported for "read only" plugins.
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * Internal classes
  */
    SYNCML_DM_RET_STATUS_T BackupESNData();

  /**
  * Restores an ESN data from a file. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szBackupFileName [in] - reference to a backup file
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicates that the operation is completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    SYNCML_DM_RET_STATUS_T RestoreESNData( CPCHAR szBackupFileName );
#endif
};

#endif
