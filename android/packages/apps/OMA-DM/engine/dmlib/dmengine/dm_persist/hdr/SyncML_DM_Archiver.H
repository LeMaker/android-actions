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

#ifndef SYNCML_DM_ARCHIVER_H
#define SYNCML_DM_ARCHIVER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include "SyncML_DM_Archive.H"
#include "file_manager.h"
#include "dmdefs.h"

#define MAX_ARCHIVES  32 

/*==================================================================================================
CLASS DECLARATION
==================================================================================================*/
class DMTree;
class CEnv;

class SyncML_DM_Archiver
{
  public:
    SyncML_DM_Archiver();
    
    /* Methods for deserializing / serializing a DMTree from/into a file
    * according to the URI-file path ordered pairs suggested in the mount list
    */
     SYNCML_DM_RET_STATUS_T deserialize(FILESETTYPE nFileSet, 
                                                                  SYNCML_DM_TREE_LOCK_TYPE_T eLockType) ;
	
     SYNCML_DM_RET_STATUS_T serialize(FILESETTYPE nFileSet, CPCHAR szURI) ;

    /* A helper method for making sure that ALL archives is set up if possible */
     SYNCML_DM_RET_STATUS_T initArchives( CEnv* env, DMTree* tree);  

    // A helper method for making sure that ALL archives is freed
     SYNCML_DM_RET_STATUS_T deinitArchives() ;

    // Used for recovery purposes in the next phase of development
    //CheckRecovery should be done for ALL files
     SYNCML_DM_RET_STATUS_T checkRecovery() ;

    // rollback file set 
    SYNCML_DM_RET_STATUS_T rollback(INT32 nFileSet) ;

    //Get the Archive containing the URI, the URI may NOT be a node in the tree !
    FILESETTYPE getArchivesByURI(CPCHAR pURI) const;

    //Get the Archive containing the URI
    SyncML_DM_Archive* getArchiveByURI(CPCHAR pURI) const;

    // Get the archive which is the Root of the tree
     SyncML_DM_Archive* getRootArchive() { return m_pArchives[0]; }

    //Get the num of Files
     INT32 getNumArchives() const { return m_numArchives; }

   
    BOOLEAN IsDirty(FILESETTYPE * nFileSet);


    // Get the archive which is the Root of the tree
     SyncML_DM_Archive* getArchive(INT32 index) { return m_pArchives[index]; }
   
    // Unload files by memory aging
     void CheckMemoryAging(INT32 nAgingTime);

    SYNCML_DM_RET_STATUS_T CleanEvents(CPCHAR szURI);	

    SYNCML_DM_RET_STATUS_T UpdateEvents(CPCHAR szURI, CPCHAR szNewName);
	 

  private:
    // this function does not return status code, since it notifies only
     void InvokeCommitPlugins(FILESETTYPE nFileSet, CPCHAR szURI);



  private:
    // DM: not implemented to prevent copying
     SyncML_DM_Archiver( const SyncML_DM_Archiver& );
    const SyncML_DM_Archiver& operator =( const SyncML_DM_Archiver& );

  private:
    // The tree of archives in relative associative form 
     SyncML_DM_Archive  *m_pArchives[MAX_ARCHIVES];    //[0] is always root archive
     INT32   m_numArchives;
     DMTree  *m_pTree;
     CEnv  *m_pEnv;                    
};

#endif /* SYNCML_DM_ARCHIVER_H */
