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

#include "file_manager.h"
#include "SyncML_DM_Archiver.H"  //for Archiver class functions
#include "xpl_Logger.h"
#include "dmLockingHelper.h"
#include "dm_tree_class.H"
#include "dmtTreeImpl.hpp"
#include "dmPluginManager.h"
#include "dm_tree_plugin_util.H"

#include "dmprofile.h"

// assume that we support 32 or less files, so we can pass "file set" as an integer (32 bits)
// every bit means file "selected" if it set. For example, value 6, binary 0110 means files "1" and "2"

CMultipleFileManager::CMultipleFileManager()
  : m_pTree( NULL )
{ 
}

CMultipleFileManager::~CMultipleFileManager()
{
  DeInit();
}

SYNCML_DM_RET_STATUS_T CMultipleFileManager::Init( DMTree* tree )
{
  if( !tree ) return SYNCML_DM_FAIL;
  
  m_pTree = tree;
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T CMultipleFileManager::DeInit()
{
  m_pTree = NULL;
  return SYNCML_DM_SUCCESS;
}

INT32 CMultipleFileManager::GetFileNumber() const
{
    const SyncML_DM_Archiver& archiver = m_pTree->GetArchiver();
    
    INT32 numFiles = archiver.getNumArchives();
    XPL_LOG_DM_TMN_Debug(("CMultipleFileManager::GetFileNumber, numFiles=%d\n", numFiles));

    return numFiles;
}

FILESETTYPE CMultipleFileManager::GetFileSetByURI( CPCHAR szURI, BOOLEAN bSharedLock ) const
{
   const SyncML_DM_Archiver&  archiver = m_pTree->GetArchiver();
   FILESETTYPE                nFileSet=0;

   nFileSet = archiver.getArchivesByURI(szURI);

   if (!bSharedLock && DmStrlen(szURI) >1 ) //writable and it is NOT . (if . already covered everyone) )
   {
      //Check constraint overlap.
      //Logic is this: if any plugin's ONE root path is contained in the path, then THE plugins'
      // all root paths must also go into the locked fileset.

      DMPluginVector  plugins;
      DMPluginManager & oPluginManager = m_pTree->GetPluginManager();

      oPluginManager.GetPlugins( szURI, SYNCML_DM_CONSTRAINT_PLUGIN, plugins);

      INT32 size = plugins.size();
      for (int i=0; i<size;i++)
      {
            PDMPlugin plugin=plugins[i];

            nFileSet |= archiver.getArchivesByURI(plugin->GetPath() );
      }
   }   
   XPL_LOG_DM_TMN_Debug(("GetFileSetByURI, szURI=%s nFileSet=0x%x, bSharedLock=%d\n", szURI, nFileSet, bSharedLock));
   return nFileSet;
}

//DeSerialize files if Not loaded
SYNCML_DM_RET_STATUS_T 
CMultipleFileManager::LockFileSet(CPCHAR szURI, 
                                            FILESETTYPE nFileSet,
                                            SYNCML_DM_TREE_LOCK_TYPE_T eLockType)
{
    SYNCML_DM_RET_STATUS_T retStat=SYNCML_DM_SUCCESS;
    SyncML_DM_Archiver&    archiver = m_pTree->GetArchiver();

    //In deserialize. Note: ACL is handled from lock manager.
    return archiver.deserialize(nFileSet,eLockType);

//    if ( retStat != SYNCML_DM_SUCCESS )
//        return retStat;
         
//    DMPluginManager & oPluginManager = m_pTree->GetPluginManager();
//    return oPluginManager.UpdatePluginNodes(szURI);
    
}
SYNCML_DM_RET_STATUS_T CMultipleFileManager::FlushFileSet(CPCHAR szURI, FILESETTYPE nFileSet, SYNCML_DM_COMMAND_T type)
{
    SYNCML_DM_RET_STATUS_T retStat=SYNCML_DM_SUCCESS;
    SYNCML_DM_RET_STATUS_T retStat1=SYNCML_DM_SUCCESS;
    FILESETTYPE nFileSetToRollback = 0;
      
    XPL_LOG_DM_TMN_Debug(("Enter CMultipleFileManager::UnLockFileSet,szURI=%s nFileSet=0x%x\n",szURI, nFileSet));

    if ( DmGetMemFailedFlag() ) // device runs out of memory
        return SYNCML_DM_DEVICE_FULL;

    SyncML_DM_Archiver&    archiver = m_pTree->GetArchiver();
  
    retStat=DmCheckConstraint(szURI, &nFileSet,&nFileSetToRollback);
    if (retStat != SYNCML_DM_SUCCESS)
    {
        retStat = RollbackFileSet(szURI, nFileSetToRollback);
        if ( nFileSet == 0 )
            return retStat;
    }
         
    if ( nFileSet )
    {
        retStat = DmCallPluginFunction(szURI, nFileSet, type);
        retStat1 = archiver.serialize(nFileSet, szURI);
        if (retStat == SYNCML_DM_SUCCESS)
            retStat=retStat1;
    }

    XPL_LOG_DM_TMN_Debug(("Leave CMultipleFileManager::UnlockFileSet, nFileSet=0x%x, retStat=%d\n",nFileSet, retStat));
    return retStat;
   
}

SYNCML_DM_RET_STATUS_T CMultipleFileManager::RollbackFileSet(CPCHAR szURI, FILESETTYPE nFileSet)
{
    SYNCML_DM_RET_STATUS_T retStat=SYNCML_DM_SUCCESS;
    SYNCML_DM_RET_STATUS_T retStat1=SYNCML_DM_SUCCESS;
   
    XPL_LOG_DM_TMN_Debug(("Enter CMultipleFileManager::RollbackFileSet,szURI=%s nFileSet=0x%x\n",szURI, nFileSet));

    SyncML_DM_Archiver&    archiver = m_pTree->GetArchiver();
    if ( archiver.IsDirty(&nFileSet) )
    {
        retStat = DmCallPluginFunction(szURI, nFileSet, SYNCML_DM_ROLLBACK);
        retStat1 = archiver.rollback(nFileSet); 
        if (retStat == SYNCML_DM_SUCCESS)
            retStat=retStat1;
    }

    XPL_LOG_DM_TMN_Debug(("Leave CMultipleFileManager::RollbackFileSet, nFileSet=0x%x, retStat=%d\n",nFileSet, retStat));
    return retStat;
   
}

SYNCML_DM_RET_STATUS_T 
CMultipleFileManager::UnlockFileSet(CPCHAR szURI, 
                                                    FILESETTYPE nFileSet,
                                                    SYNCML_DM_TREE_LOCK_TYPE_T eLockType)
{
    SYNCML_DM_RET_STATUS_T retStat=SYNCML_DM_SUCCESS;
    SYNCML_DM_RET_STATUS_T retStat1=SYNCML_DM_SUCCESS;
    FILESETTYPE nFileSetToRollback = 0;
      
    XPL_LOG_DM_TMN_Debug(("Enter CMultipleFileManager::UnLockFileSet,szURI=%s nFileSet=0x%x\n",szURI, nFileSet));

    if ( DmGetMemFailedFlag() ) // device runs out of memory
        return SYNCML_DM_DEVICE_FULL;

    if ( eLockType != SYNCML_DM_LOCK_TYPE_SHARED )
    {
        retStat=DmCheckConstraint(szURI, &nFileSet, &nFileSetToRollback);
        if (retStat != SYNCML_DM_SUCCESS)
        {
            retStat = RollbackFileSet(szURI, nFileSetToRollback);
            if ( nFileSet == 0 )
                return retStat;
        }
    }    
         
    if ( nFileSet )
    {
        SyncML_DM_Archiver&    archiver = m_pTree->GetArchiver();
        retStat = DmCallPluginFunction(szURI, nFileSet, SYNCML_DM_RELEASE);
        retStat1 = archiver.serialize(nFileSet, szURI);
        if (retStat == SYNCML_DM_SUCCESS)
            retStat=retStat1;
    }

   XPL_LOG_DM_TMN_Debug(("Leave CMultipleFileManager::UnlockFileSet, nFileSet=0x%x, retStat=%d\n",nFileSet, retStat));
   return retStat;
}
