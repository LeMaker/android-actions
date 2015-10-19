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

#include "SyncML_DM_Archiver.H"
#include "SyncML_DM_WBXMLArchive.H"
#include "xpl_Logger.h"
#include "dm_tree_class.H"
#include "dm_uri_utils.h"
#include "dm_tree_plugin_util.H"  //for Util function defns
#include "dmprofile.h"
#include "dmLockingHelper.h"
#include "dmtTreeImpl.hpp"

#ifdef TEST_DM_RECOVERY
char power_fail_point[20] = {0};
#endif

SyncML_DM_Archiver::SyncML_DM_Archiver()
  : m_numArchives( 0 ),
    m_pTree( NULL ),
    m_pEnv( NULL )
{
  // [0] is always root archive
    
  for( int i = 0; i < MAX_ARCHIVES; ++i )
  {
    m_pArchives[ i ] = NULL;
  }
}

SYNCML_DM_RET_STATUS_T SyncML_DM_Archiver::initArchives( CEnv* env, DMTree* tree )
{
    SYNCML_DM_RET_STATUS_T retStat=SYNCML_DM_SUCCESS;

    if( !tree || !env ) return SYNCML_DM_FAIL;
    m_pTree = tree;
    m_pEnv  = env;
    
    //Load All the archives
    m_numArchives=0;
    m_pArchives[0]=NULL;

   //Load from TreeMountList and new all the Archives
    const char *pEntryTreePath     = NULL;
    const char *pEntryURI          = NULL;

    INT32 i = 0;  //tree mount start from 0

    // Get the first mount list entry
    m_pTree->GetTreeMountEntry(pEntryURI, pEntryTreePath, i);
  
    while(pEntryURI != NULL && pEntryTreePath != NULL) 
    {
        /* Create the archive if it exists in the mount list */
        m_pArchives[m_numArchives]=NULL;

        if ( DmStrlen(pEntryTreePath) )
        {
	        m_pArchives[m_numArchives]=new SyncML_DM_WBXMLArchive(env, pEntryURI, pEntryTreePath);

	        if ( !m_pArchives[m_numArchives] )
	          return SYNCML_DM_DEVICE_FULL;

              if ( m_pArchives[m_numArchives]->Init(tree) != SYNCML_DM_SUCCESS )
                return SYNCML_DM_FAIL;  
	      
              // XPL_LOG_DM_TMN_Debug(("pEntryURI=%s, archive=%x\n", pEntryURI, m_pArchives[m_numArchives]));
	      
	        INT32 parentindex=-1;
	        INT32 parentlen=0;
	        for (INT32 j=0; j< m_numArchives; j++) 
	        {
	            if ( DmStrstr(pEntryURI, m_pArchives[j]->getURI())== pEntryURI) 
	            {
	                INT32 len=DmStrlen(m_pArchives[j]->getURI());
	          
	                if (parentlen < len) 
	                {
	                    parentlen=len;
	                    parentindex=j;
	                }
	            }
	        }
	      
	        if (parentindex >=0)
	            m_pArchives[m_numArchives]->setParentArchive(m_pArchives[parentindex]);
	      
	        m_numArchives++;
	        //Does not support More than 32 archives...
	        if (m_numArchives >=32)      
	            break;
        }		
      
        /* In case the GetTreeMountEntry service has an error,
        * Ensure these are sensible values going in
        */         
        pEntryURI = NULL;
        pEntryTreePath = NULL;
        m_pTree->GetTreeMountEntry(pEntryURI, pEntryTreePath, ++i);
    }
   
    if (m_numArchives ==0)
        retStat=SYNCML_DM_FAIL;

    XPL_LOG_DM_TMN_Debug(("m_numArchives=%d\n", m_numArchives));

#ifdef TEST_DM_RECOVERY
  if(XPL_DM_GetEnv(SYNCML_DM_POWER_FAIL_IJECTION) != NULL) {
    strcpy((char *)power_fail_point, (const char *)XPL_DM_GetEnv(SYNCML_DM_POWER_FAIL_IJECTION));
    XPL_LOG_DM_TMN_Debug(("Power Fail Injected at: %s\n", power_fail_point));
  }
#endif

  return retStat;
}

// A helper method for making sure that ALL archives is freed
SYNCML_DM_RET_STATUS_T SyncML_DM_Archiver::deinitArchives()
{
    SYNCML_DM_RET_STATUS_T retStat=SYNCML_DM_SUCCESS;

    //serialize if needed, Do we need to do so ? The lock manager should have already do it.

    //delete the archives
    for (int i=0; i<m_numArchives; i++)
    {
       if ( m_pArchives[i] !=NULL)
       {
          delete m_pArchives[i];
          m_pArchives[i]=NULL;
       }
    }

    m_numArchives = 0;
    m_pTree = NULL;
    m_pEnv = NULL;
    
    return retStat;
}


/*==================================================================================================

Function:    SyncML_DM_Archiver::serialize

Description: Distributes a serialization request to the proper archive(s).  A search is performed
             for the right archive relative to the root archive.  All tree data will be serialized 
             to the point possible.

Returns:     SYNCML_DM_FAIL         - tree or archive objects are NULL or an unspecified error occurred
             SYNCML_DM_SUCCESS      - tree was successfully serialized to proper files
             SYNCML_DM_TREE_CORRUPT - tree storage entity was corrupt to the point where 
                                      serialization was partially or completely impossible
             SYNCML_DM_IO_FAILURE   - I/O failure occured while trying to write to the file handle

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SyncML_DM_Archiver::serialize(FILESETTYPE nFileSet, CPCHAR szURI)
{
    DMGlobalLockHelper oGlobalLock;  // protect serialize/recovery from multi process access
    FILESETTYPE set=1;
    SYNCML_DM_RET_STATUS_T retStat=SYNCML_DM_SUCCESS;
    INT32 i=0;
   
    if( !m_pTree ) return SYNCML_DM_FAIL;

    XPL_LOG_DM_TMN_Debug(("Enter to serialize\n"));

    for (i=0; i< m_numArchives && m_pArchives[i] !=NULL; i++)
    {
        if (  (nFileSet & set) !=0 ) {
            if (m_pArchives[i]->isDirty()) {
#ifdef TEST_DM_RECOVERY
                if ((power_fail_point != NULL) && (DmStrcmp(power_fail_point, "DMTR_PF1") == 0)) {
                    printf("Type Ctrl-C to simulate Power Fail ...\n");
                    sleep(30);
                }
#endif
                XPL_LOG_DM_TMN_Debug(("m_pArchives[%d] %s is dirty, to be serialized", i, m_pArchives[i]->getURI()));
                retStat=m_pArchives[i]->serialize(m_pTree);
                XPL_LOG_DM_TMN_Debug(("serialize retStat = %d", retStat));
                if (retStat != SYNCML_DM_SUCCESS )
                    break;
            }
        }
        set = set <<1;
    }

    XPL_LOG_DM_TMN_Debug(("serialize retStat = %d", retStat));            
    if (retStat != SYNCML_DM_SUCCESS )
        return retStat;       

    /* Begin the power loss-tolerant steps to protect against a partially written Archive file:
    * 1. Rename the original file by adding ".bak" to the name; this indicates the .temp file
    *    was completely written
    * 2. Rename the .temp file to the original name (i.e., without the .temp extention)
    * 3. Delete the .bak file
    */
    DMString strOrgFilePathBuffer, strTmpFilePathBuffer, strBakFilePathBuffer;
    
    char *orgFilePath = strOrgFilePathBuffer.AllocateBuffer(XPL_FS_MAX_FILE_NAME_LENGTH);
    char *tmpFilePath =strTmpFilePathBuffer.AllocateBuffer(XPL_FS_MAX_FILE_NAME_LENGTH);
    char *bakFilePath = strBakFilePathBuffer.AllocateBuffer(XPL_FS_MAX_FILE_NAME_LENGTH);
    XPL_FS_RET_STATUS_T ret=0;
    BOOLEAN   bAtLeastOneDirty = FALSE; // check if at least one change exist to invoke commit plug-in

    if ( !orgFilePath || !tmpFilePath || !bakFilePath)
      return SYNCML_DM_DEVICE_FULL;
    
   
    // 2nd Change File names, orig->bak
    for (i=0, set=1; i< m_numArchives && m_pArchives[i] !=NULL; i++, set = set <<1)
    {
        if ( (nFileSet & set) !=0  && m_pArchives[i]->isDirty() && m_pArchives[i]->isWritableExist() )
        {
#ifdef TEST_DM_RECOVERY
            if ((power_fail_point != NULL) && (DmStrcmp(power_fail_point, "DMTR_PF2") == 0)) {
                printf("Type Ctrl-C to simulate Power Fail ...\n");
                sleep(30);
            }
#endif
#ifdef LOB_SUPPORT
		retStat = m_pArchives[i]->CloseCommitLog();
		if (retStat != SYNCML_DM_SUCCESS )
			return retStat; 	  
#endif
            m_pArchives[i]->getFilePath(orgFilePath, "");
            m_pArchives[i]->getFilePath(bakFilePath, ".bak");
            ret=XPL_FS_Rename(  orgFilePath, bakFilePath );    
            if (ret != XPL_FS_RET_SUCCESS)
            {
                ret=XPL_FS_Rename(  orgFilePath, bakFilePath );    
                retStat=SYNCML_DM_IO_FAILURE;
                break;
            }
        }
    }

    for (i=0, set=1; i< m_numArchives && m_pArchives[i] !=NULL; i++, set = set <<1)
    {
        if ( (nFileSet & set) !=0  && m_pArchives[i]->isDirty() )
        {
#ifdef TEST_DM_RECOVERY
            if ((power_fail_point != NULL) && (DmStrcmp(power_fail_point, "DMTR_PF3") == 0)) {
                printf("Type Ctrl-C to simulate Power Fail ...\n");
                sleep(30);
            }
#endif

            bAtLeastOneDirty = TRUE;

            m_pArchives[i]->getFilePath(tmpFilePath, ".temp");
            m_pArchives[i]->getFilePath(orgFilePath, "");
            m_pArchives[i]->getFilePath(bakFilePath, ".bak");
            ret=XPL_FS_Rename(tmpFilePath, orgFilePath);      
            if (ret != XPL_FS_RET_SUCCESS)
            {
                ret=XPL_FS_Rename(bakFilePath, orgFilePath);      
                retStat=SYNCML_DM_IO_FAILURE;
                break;
            }
        }
    }

    // commit plug-in support - invoke it only when some changes were done
    if ( bAtLeastOneDirty )
      InvokeCommitPlugins( nFileSet, szURI );
    
    for (i=0, set=1; i< m_numArchives && m_pArchives[i] !=NULL; i++, set = set <<1)
    {
        //set dirty to false
        if ( (nFileSet & set) !=0  && m_pArchives[i]->isDirty() )
        {
#ifdef TEST_DM_RECOVERY
            if ((power_fail_point != NULL) && (DmStrcmp(power_fail_point, "DMTR_PF4") == 0)) {
                printf("Type Ctrl-C to simulate Power Fail ...\n");
                sleep(30);
            }
#endif
#ifdef LOB_SUPPORT  
		// Commit changes for ESN
		retStat=m_pArchives[i]->commitESN(m_pTree);
#endif
            m_pArchives[i]->setDirty(false);

            //Let each archive finished its own task, reset timestamp etc.
            m_pArchives[i]->serializeDone();
            XPL_LOG_DM_TMN_Debug(("serializeDone for %s\n", orgFilePath));

            if ( m_pArchives[i]->isWritableExist() ) 
            {
                m_pArchives[i]->getFilePath(bakFilePath, ".bak");

#ifndef DM_NO_LOCKING                
                ret=XPL_FS_Unlink(bakFilePath);
#else
                ret=XPL_FS_Remove(bakFilePath);
#endif
                XPL_LOG_DM_TMN_Debug(("delete archives[%d] %s ret=%d\n", i, bakFilePath, ret));
                if (ret != XPL_FS_RET_SUCCESS)
                {
                    retStat = SYNCML_DM_IO_FAILURE;
                    break;
                }
            }
        }
        if ( (nFileSet & set) !=0  && m_pArchives[i]->isDirty() )
            m_pArchives[i]->setWritableExist(TRUE);
        
   }   
 
    return retStat; 
}

/*==================================================================================================

Function:    SyncML_DM_Archiver::deserialize

Description: Distributes a deserialization request to the proper archive(s).  Each archive (starting
             with the root archive) will deserialize into the DM T/N Manager using the AddNode
             method for passing nodes.

Returns:     SYNCML_DM_FAIL         - tree or archive objects are NULL or an unspecified error occured
             SYNCML_DM_SUCCESS      - tree was successfully deserialized from proper files
             SYNCML_DM_TREE_CORRUPT - tree storage entity was corrupt to the point where 
                                deserialization was partially or completely impossible
             SYNCML_DM_FILE_NOT_FOUND - file was unavailable for deserialization
             SYNCML_DM_IO_FAILURE   - I/O failure occured while trying to read from the file handle

==================================================================================================*/
SYNCML_DM_RET_STATUS_T 
SyncML_DM_Archiver::deserialize(FILESETTYPE nFileSet,
                                              SYNCML_DM_TREE_LOCK_TYPE_T eLockType)
{
    DMGlobalLockHelper oGlobalLock;  // protect serialize/recovery from multi process access
    FILESETTYPE set=1;
    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    if( !m_pTree ) return SYNCML_DM_FAIL;

    checkRecovery();

    // do not load files itself, instead create only skeleton based on file set.
    for (INT32 i=0; i< m_numArchives && m_pArchives[i] !=NULL; i++) 
    {
        if (  (nFileSet & set) !=0 ) 
        { 
            BOOLEAN bIsPermitted = TRUE;
            bIsPermitted = m_pArchives[i]->verifyPermission(XPL_FS_RDONLY_MODE);
            
            if ( bIsPermitted )
            {
         
                if ( !m_pArchives[i]->LoadSkeleton(m_pTree)) 
                      dm_stat = SYNCML_DM_DEVICE_FULL;

                if ( dm_stat != SYNCML_DM_SUCCESS )
                    return dm_stat;

                if ( eLockType == SYNCML_DM_LOCK_TYPE_SHARED )
                {
                   dm_stat =  m_pArchives[i]->deserialize(m_pTree,TRUE);
                }
            }
            
            m_pArchives[i]->setDirty(FALSE);  
                
        }
        set = set <<1;
    }
  
    XPL_LOG_DM_TMN_Debug(("End of all deserialize retStat=%d\n", dm_stat));
    return dm_stat;
}


//Get all required Archives containing the URI
FILESETTYPE SyncML_DM_Archiver::getArchivesByURI(CPCHAR pURI) const
{
    FILESETTYPE nFileSet=0;
    FILESETTYPE set=1;
    SyncML_DM_Archive* pArchive=NULL;

    INT32 containingArchive=0;
    INT32 containingArchiveURIlen=0;
    BOOLEAN bContainingNeeded = TRUE;

   //set = set <<1; //start from 2nd file
    for (int i=0; i< m_numArchives; i++)  //NOT detect root !
    {
        pArchive = m_pArchives[i]; 

        //For now assume no recursive files
        const char * archivePath=pArchive->getURI();

        char * ptr=NULL;

        //we need any file that is subtree of the pURI
        //For example, archivePath=./abc pURI=. then
        //if we found pURI under archivePath, we go
        ptr=(char*)DmStrstr(archivePath, pURI );
      
        if ( ptr !=NULL && ptr == archivePath ) {         
            nFileSet= nFileSet | set;
        } 

        // in case when we hit exactly root path , we don't need parent
        if ( DmStrcmp( archivePath, pURI ) == 0  )
            bContainingNeeded = FALSE;

        //search for longest archives that has the pURI as a childnode
        //For example, archivePath=./abc pURI=./abc/def then
        //if we found archivePath under pURI, we go
        ptr=(char*)DmStrstr(pURI, archivePath );
      
        if (ptr !=NULL && ptr == pURI) {
            if (containingArchiveURIlen < (INT32)DmStrlen(archivePath)) {
                containingArchive=i;
                containingArchiveURIlen = DmStrlen(archivePath);
            }
        }
      
        set = set <<1;
    } 

    if ( bContainingNeeded )
        nFileSet=nFileSet | (1 << (containingArchive)); //we need the containing archive
    
   //Debug("Add file [%d] to set, %s\n", containingArchive, m_pArchives[containingArchive]);

    return nFileSet;
}



//Get exact Archive containing the URI
SyncML_DM_Archive* SyncML_DM_Archiver::getArchiveByURI(CPCHAR pURI) const
{


    FILESETTYPE nFileSet ;
    FILESETTYPE set=1;
    INT32 index = 0;

    nFileSet = getArchivesByURI(pURI);
 
    for (INT32 i=0; i< m_numArchives && m_pArchives[i] !=NULL; i++) 
    {
        if (  (nFileSet & set) !=0 ) 
        {
            if ( DmIsParentURI(m_pArchives[i]->getURI(), pURI) )
            {
                index = i;
                break;
            }    
        }    
         set = set <<1;
    }
    
    //XPL_LOG_DM_TMN_Debug(("returning archive=%x, uri=%s\n",m_pArchives[index], pURI )); 
    return m_pArchives[index];
}

/*==================================================================================================

Function:    SyncML_DM_Archiver::checkRecovery

Description: Resolve recovery records after a serious phone critical failure event

Returns:     SYNCML_DM_SUCCESS
             SYNCML_DM_IO_FAILURE
             SYNCML_DM_FILE_NOT_FOUND
             SYNCML_DM_LOG_CORRUPT

==================================================================================================*/
SYNCML_DM_RET_STATUS_T
SyncML_DM_Archiver::checkRecovery()
{
    if( !m_pTree ) return SYNCML_DM_FAIL;

    SYNCML_DM_RET_STATUS_T retStat=SYNCML_DM_SUCCESS;

    //Check and recover All nodes all at once.
    DMString strOrgFilePathBuffer, strTmpFilePathBuffer, strBakFilePathBuffer, strCmtFilePathBuffer;
    
    char *orgFilePath = strOrgFilePathBuffer.AllocateBuffer(XPL_FS_MAX_FILE_NAME_LENGTH);
    char *tmpFilePath =strTmpFilePathBuffer.AllocateBuffer(XPL_FS_MAX_FILE_NAME_LENGTH);
    char *bakFilePath = strBakFilePathBuffer.AllocateBuffer(XPL_FS_MAX_FILE_NAME_LENGTH);
    char *cmtFilePath = strCmtFilePathBuffer.AllocateBuffer(XPL_FS_MAX_FILE_NAME_LENGTH);

    if ( !orgFilePath || !tmpFilePath || !bakFilePath || !cmtFilePath)
      return SYNCML_DM_DEVICE_FULL;
    
    XPL_FS_RET_STATUS_T ret=0;

    //Logic for checking corrupted files
    BOOLEAN tmpFileExists=FALSE;
    BOOLEAN bakFileExists=FALSE;
    BOOLEAN cmtFileExists=FALSE;
    INT32 i;

    if ( DmGetMemFailedFlag() ) // device runs out of memory
      return SYNCML_DM_DEVICE_FULL;

    m_pTree->RecoverPlugin();

    for (i=0; i< m_numArchives; i++)
    {
        m_pArchives[i]->getFilePath(orgFilePath, "");
        m_pArchives[i]->getFilePath(bakFilePath, ".bak");
        m_pArchives[i]->getFilePath(tmpFilePath, ".temp");
        m_pArchives[i]->getFilePath(cmtFilePath, ".cmt");

        if (XPL_FS_Exist(tmpFilePath))
            tmpFileExists=TRUE;
        if (XPL_FS_Exist(bakFilePath))
            bakFileExists=TRUE;
        if (XPL_FS_Exist(cmtFilePath))
            cmtFileExists=TRUE;
    }

    if ( tmpFileExists || bakFileExists )
        XPL_FS_Remove(DM_ISP_LOCK);

    if (tmpFileExists)
    {
        if (bakFileExists)
        {
            //Both temp file and bak file exists, rename bak to orig
            // rename bak files to orig files, then delete tmp files and commit log files
            for (i=0; i< m_numArchives; i++)
            {
                m_pArchives[i]->getFilePath(orgFilePath, "");
                m_pArchives[i]->getFilePath(bakFilePath, ".bak");
                ret=XPL_FS_Rename(bakFilePath, orgFilePath);
                XPL_LOG_DM_TMN_Debug(("DmFsRename %s->%s ret=%d\n", bakFilePath, orgFilePath, ret));
            }                  
        }
        for (i=0; i< m_numArchives; i++)
        {
            m_pArchives[i]->getFilePath(tmpFilePath, ".temp");
            m_pArchives[i]->getFilePath(cmtFilePath, ".cmt");
#ifndef DM_NO_LOCKING                
            ret=XPL_FS_Unlink(tmpFilePath);
            ret=XPL_FS_Unlink(cmtFilePath);
#else
            ret=XPL_FS_Remove(tmpFilePath);
            ret=XPL_FS_Remove(cmtFilePath);
#endif            
            XPL_LOG_DM_TMN_Debug(("delete %s ret=%d\n", tmpFilePath, ret));
        }         
    } 
    else
    {
        if (bakFileExists | cmtFileExists)
        {
            //Only bak file exists
            // Delete bak files
            for (i=0; i< m_numArchives; i++)
            {
                m_pArchives[i]->getFilePath(bakFilePath, ".bak");
                m_pArchives[i]->getFilePath(cmtFilePath, ".cmt");
                // Remove .bak file		   
                if (XPL_FS_Exist(bakFilePath))
                {
#ifndef DM_NO_LOCKING                
                    ret=XPL_FS_Unlink(bakFilePath);
#else
                    ret=XPL_FS_Remove(bakFilePath);
#endif                
                    XPL_LOG_DM_TMN_Debug(("delete %s ret=%d\n", bakFilePath, ret));
               }
#ifdef LOB_SUPPORT  
                // Play and remove commit log file
               if (XPL_FS_Exist(cmtFilePath))
               {
                    m_pArchives[i]->PlayCommitLog();
               }
#endif   
            }                  
        }
    }

   // Remove temorary LOB files
    {
        DMString lobDir;
        m_pEnv->GetWFSFullPath(NULL,lobDir);
        DmRemoveTempfile(lobDir);
    }

    //Logic for checking newer files
    for (i=0; i< m_numArchives && m_pArchives[i] !=NULL; i++)
    {
        if (m_pArchives[i]->getRootNode() !=NULL && !m_pArchives[i]->getRootNode()->IsSkeletonNode())
        {
            //Already loaded in memory
            if (m_pArchives[i]->getLastModifiedTime() != m_pArchives[i]->getLastSavedTime())
            {
                //The files needed to be reloaded.
                XPL_LOG_DM_TMN_Debug(("File[%d] %s is newer, need delete subtree & reload\n", i, m_pArchives[i]->getURI()));
                XPL_LOG_DM_TMN_Debug((" last file modified time=0x%x memory time=0x%x\n",m_pArchives[i]->getLastModifiedTime(), m_pArchives[i]->getLastSavedTime()));
                m_pTree->UnloadArchive(m_pArchives[i]->getRootNode());
            }
        }
    }                  

    return retStat;
}

SYNCML_DM_RET_STATUS_T
SyncML_DM_Archiver::rollback(INT32 nFileSet)
{
    if( !m_pTree ) return SYNCML_DM_FAIL;

    SYNCML_DM_RET_STATUS_T retStat=SYNCML_DM_SUCCESS;

    INT32 i=0;
    INT32 set=1;
    for (i=0, set=1; i< m_numArchives && m_pArchives[i] !=NULL; i++, set = set <<1 )
    {
        if ( (nFileSet & set) !=0 && m_pArchives[i]->getRootNode() != NULL )
        {
            //Already loaded in memory
            if ( m_pArchives[i]->isDirty() )
            {
                //The files needed to be reloaded.
                XPL_LOG_DM_TMN_Debug((" last file modified time=0x%x memory time=0x%x\n", m_pArchives[i]->getLastModifiedTime(), 
                                                         m_pArchives[i]->getLastSavedTime()));

#ifdef LOB_SUPPORT  
                // Rollback all changes for ESN
              m_pArchives[i]->rollbackESN(m_pTree);
#endif
               m_pTree->UnloadArchive(m_pArchives[i]->getRootNode());
               m_pArchives[i]->setDirty(FALSE);
               m_pArchives[i]->GetEventLogger().Reset();
            }
        }
    }

    return retStat;
}

void 
SyncML_DM_Archiver::CheckMemoryAging(INT32 nAgingTime)
{   
    if( !m_pTree ) return;

    // current time in seconds
    XPL_CLK_CLOCK_T currentTime = XPL_CLK_GetClock();
    for (INT32 i=0; i < m_numArchives && m_pArchives[i] !=NULL; i++)
    {
      // free from memory
        if ( m_pArchives[i]->getRootNode()
            && !m_pArchives[i]->getRootNode()->IsSkeletonNode()
            && !m_pArchives[i]->isDirty() )
        {
            // Check the last accessed time in seconds
            XPL_CLK_CLOCK_T lastAccessedTime = m_pArchives[i]->getLastAccessedTime();
            if ( (currentTime - lastAccessedTime) >= (XPL_CLK_CLOCK_T)nAgingTime ) 
            {
                XPL_LOG_DM_TMN_Debug(("m_pArchives[%d]->getURI() : %s is being unloaded, unused for %d >= aging time = %d\n",
                            i, m_pArchives[i]->getURI(), (currentTime - lastAccessedTime), nAgingTime));
            }

            m_pTree->UnloadArchive(m_pArchives[i]->getRootNode());
        }
    }
}

//------------------------------------------------------------------------
// FUNCTION        : InvokeCommitPlugins
// DESCRIPTION     : This function iterates all commit plug-ins 
//                   and calls "OnCommit" on each which has update event(s)
// ARGUMENTS PASSED: szURI - sub-tree root uri
//                   nFileSet - currently used file set 
// RETURN VALUE    : void
// PRE-CONDITIONS  : called after successful serialization
// POST-CONDITIONS : 
// IMPORTANT NOTES :
// REQUIREMENT #   :
//------------------------------------------------------------------------
void SyncML_DM_Archiver::InvokeCommitPlugins(FILESETTYPE nFileSet, CPCHAR szURI)
{
    PDmtTree tree( new DmtTreeImpl(true) );
    DMPluginVector  plugins;
    FILESETTYPE set=1;
    INT32 i=0;

    DMPluginManager & oPluginManager = m_pTree->GetPluginManager();
  
    oPluginManager.GetPlugins(szURI, SYNCML_DM_COMMIT_PLUGIN, plugins);

    for (INT32 nPlugin = 0; nPlugin < plugins.size(); nPlugin++ )
    {
        DmtEventMap updatedNodes;
      
        for (i=0, set=1; i< m_numArchives && m_pArchives[i] !=NULL; i++, set = set <<1)
        {
            if ( (nFileSet & set) !=0  && m_pArchives[i]->isDirty() )
            {
                m_pArchives[i]->GetEventLogger().GetCommitPluginEvents(plugins[nPlugin], updatedNodes );
            }
        }

        if ( updatedNodes.size() > 0 )
            plugins[nPlugin]->OnCommit( updatedNodes, tree );
    }
}


//------------------------------------------------------------------------
// FUNCTION        : CleanEvents
// DESCRIPTION     : This function iterates all archives and remove stored events 
// ARGUMENTS PASSED: szURI - sub-tree root uri
// RETURN VALUE    : void
// PRE-CONDITIONS  : called after successful serialization
// POST-CONDITIONS : 
// IMPORTANT NOTES :
// REQUIREMENT #   :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T 
SyncML_DM_Archiver::CleanEvents(CPCHAR szURI)

{
    FILESETTYPE set=1;
    FILESETTYPE nFileSet=0;

    nFileSet = getArchivesByURI(szURI);
      
    for (FILESETTYPE i=0, set=1; i < m_numArchives; i++, set <<= 1)
    {
        if ( (nFileSet & set) !=0 && m_pArchives[i]->isDirty() )
        {
            m_pArchives[i]->GetEventLogger().CleanEvents(szURI);
        }
    }
    return SYNCML_DM_SUCCESS;
}


//------------------------------------------------------------------------
// FUNCTION        : UpdateEvents
// DESCRIPTION     : This function iterates all archives and remove stored events 
// ARGUMENTS PASSED: szURI - sub-tree root uri
// RETURN VALUE    : void
// PRE-CONDITIONS  : called after successful serialization
// POST-CONDITIONS : 
// IMPORTANT NOTES :
// REQUIREMENT #   :
//------------------------------------------------------------------------
SYNCML_DM_RET_STATUS_T 
SyncML_DM_Archiver::UpdateEvents(CPCHAR szURI, CPCHAR szNewName)

{
    FILESETTYPE set=1;
    FILESETTYPE nFileSet=0;

    nFileSet = getArchivesByURI(szURI);
      
    for (FILESETTYPE i=0, set=1; i< m_numArchives; i++, set = set <<1)
    {
        if ( (nFileSet & set) !=0  && m_pArchives[i]->isDirty() )
        {
            m_pArchives[i]->GetEventLogger().UpdateEvents(szURI,szNewName);
        }
    }
    return SYNCML_DM_SUCCESS;
}

//------------------------------------------------------------------------
// FUNCTION        : IsDirty
// DESCRIPTION     : Checks if file set has been modified
//                   
// ARGUMENTS PASSED: nFileSet - currently used file set 
// RETURN VALUE    : void
// PRE-CONDITIONS  : called before serialization
// POST-CONDITIONS : 
// IMPORTANT NOTES :
// REQUIREMENT #   :
//------------------------------------------------------------------------
BOOLEAN SyncML_DM_Archiver::IsDirty(FILESETTYPE * pFileSet)
{
    FILESETTYPE i=0;
    FILESETTYPE set=1;

    if ( pFileSet == NULL )
        return TRUE;

    for (i=0, set=1; i< m_numArchives;   i++, set = set <<1)
    {
        if ( ((*pFileSet) & set) !=0  && m_pArchives[i]->isDirty() == FALSE )
            (*pFileSet) &= ~set;
    }

    if ( (*pFileSet) == 0 )
        return FALSE;
    else
        return TRUE;
}
