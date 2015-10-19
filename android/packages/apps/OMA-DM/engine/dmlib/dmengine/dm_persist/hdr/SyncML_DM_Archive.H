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

#ifndef SYNCML_DM_ARCHIVE_H
#define SYNCML_DM_ARCHIVE_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: SyncML_DM_Archive.H

    General Description: This file contains the declaration for the SyncML_DM_Archive class.
    SyncML_DM_Archive is the ABC (Abstract Base Class) of all Archive classes

==================================================================================================*/

#include "xpl_Time.h"
#include "dmMemory.h"
#include "dmEventLogger.h"
#ifdef LOB_SUPPORT
#include "SyncML_Commit_Log.H"
#endif

/*==================================================================================================
CLASS DECLARATION
==================================================================================================*/

class DMNode;
class DMTree;
class CEnv;

class SyncML_DM_Archive 
{
  public:

    DMEventLogger& GetEventLogger() { return oEventLogger; }

    /* Class constructor 
     * The caller is responsible for allocating/freeing the two fields
     * to this function
     */
    SyncML_DM_Archive(CEnv* env, CPCHAR pURI, CPCHAR path) ;

    /* Class destructor (virtual because of the inheritance of 
     * other classes who might be referenced as SyncML_DM_Archive)
     */
    virtual ~SyncML_DM_Archive() ;

    virtual SYNCML_DM_RET_STATUS_T Init(DMTree* pTree);	

    /* Serialization and Deserialization methods must be overridden
     * by any concrete archive implementation
     * The caller is responsible for allocating/freeing the DMTree
     * for the serialize function
    */
    virtual SYNCML_DM_RET_STATUS_T serialize(DMTree* tree) = 0;
    virtual SYNCML_DM_RET_STATUS_T deserialize(DMTree* tree, 
                                               BOOLEAN bIsReload = FALSE) = 0;

    /* Operators to allocate and delete memory for operation */
    inline void* operator new(size_t sz)
    {
      return (DmAllocMem(sz));
    }
 
    inline void operator delete(void* buf)
    {
       DmFreeMem(buf);
    }

    /* Accessor for the last modification time of the archive 
    */
    virtual XPL_CLK_CLOCK_T getLastModifiedTime() = 0;

    /* Get last persisted time */
    virtual XPL_CLK_CLOCK_T getLastSavedTime() = 0;

    /* Check permission */
    virtual BOOLEAN verifyPermission(XPL_FS_OPEN_MODE_T permission) const {return TRUE;}
    
    /* Update last accessed time */
    void setLastAccessedTime(XPL_CLK_CLOCK_T lastAccessedTime);

    /* Get last accessed time */
    XPL_CLK_CLOCK_T getLastAccessedTime();

    BOOLEAN  isWritableExist() const {return m_bWritableExist;}

    void  setWritableExist( BOOLEAN bWritableExist ) {m_bWritableExist = bWritableExist;}

    //Re-Set last persisted time
    virtual void serializeDone() = 0;

    /* Accessors for setting/getting the string representing the URI
     * of the archive in the memory tree
     *
     * The caller is responsible for allocating/freeing memory for the
     * set function but has no responsibilities when using the get
     * function 
     */
    CPCHAR getURI() const {return m_pURI;}

    /* Accessor for setting the root node of the archive of the DMNode type 
    * The destructor of this class later frees the created DMAddNodeProp
    * object
    */
    virtual void setRootNode(DMNode* node) { rootTreeNode=node; }
  
    virtual DMNode* getRootNode() { return rootTreeNode; }

    BOOLEAN isDirty() { return dirty; }
  
    void setDirty(BOOLEAN dirty) { this->dirty=dirty; }

    void getFilePath(char * path, CPCHAR ext);

    SyncML_DM_Archive * getParentArchive() { return parent; }
  
    void setParentArchive(SyncML_DM_Archive * parent) { this->parent=parent; }

    BOOLEAN LoadSkeleton(DMTree* pTree) ;   
#ifdef LOB_SUPPORT
    virtual SYNCML_DM_RET_STATUS_T rollbackESN(DMTree* tree) = 0;
    virtual SYNCML_DM_RET_STATUS_T commitESN(DMTree* tree) = 0;
    virtual SyncML_Commit_Log*  GetCommitLogHandler() = 0;
    virtual  SYNCML_DM_RET_STATUS_T CloseCommitLog() = 0;
    virtual SYNCML_DM_RET_STATUS_T  PlayCommitLog() = 0;
#endif

 protected:

    DMEventLogger oEventLogger;

    //Parent archive of this archive. when loaded, parent MUST Have already been loaded
    SyncML_DM_Archive * parent;   
   
    /* The top-level URI of the archive */
    CPCHAR m_pURI;

    /* The filesystem path string of the archive file */
    CPCHAR  m_path;
    /* full name to file in WFS */
    DMString  m_strWFSFileName;   

    /* if the tree associated with the archive is dirty or not.*/ 
    BOOLEAN dirty;
    BOOLEAN m_bWritableExist;
    
    /* permission mask on archive */
    int     m_permission;

    DMNode* rootTreeNode; //Different from rootNode which is a serialized form
    XPL_CLK_CLOCK_T m_lastAccessedTime;
     DMTree*   m_pTree;
	
};

#endif /* SYNCML_DM_ARCHIVE_H */
