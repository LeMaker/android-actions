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

/*==================================================================================================

    Source Name: SYNCML_DM_LockCtxMgr.cc

    General Description: Implementation of the DMLockContextManager class 

==================================================================================================*/

#include "dmprofile.h"
#include "xpl_Logger.h"
#include "dmLockingHelper.h"
#include "dm_tree_class.H"
#include "SyncML_DM_Archiver.H"
#include "dmLockCtxMgr.h"

// memory aging time in seconds
#ifndef DM_NO_LOCKING  
#define DEFAULT_AGING_TIME 120
#else
#define DEFAULT_AGING_TIME 60
#endif

void DmOnMemoryTimer()
{
  dmTreeObj.CheckMemoryAging();
}


DMLockContextManager::DMLockContextManager()
  : m_pTree( NULL )
{
    m_nLockWithConfig_ID[0] = SYNCML_DM_LOCKID_NONE;
    m_nLockWithConfig_ID[1] = SYNCML_DM_LOCKID_NONE;
    m_nCurrentLockID = SYNCML_DM_LOCKID_NONE;

    m_nAgingTime = DEFAULT_AGING_TIME;
    CPCHAR dm_aging_env = XPL_DM_GetEnv(SYNCML_DM_MEMORY_AGING_TIME);
    if ( dm_aging_env != NULL ) 
    {
        INT32 agingTime = DmAtoi(dm_aging_env);
        m_nAgingTime = ((agingTime>=0) ? agingTime : 0); 
    }  
  
#ifndef DM_NO_LOCKING  
    m_ptrFM = NULL;
    m_nNewUniqueID = 1;
    m_pGlobalLockFile = NULL;
#else
    m_bIsLocked = FALSE;
    m_nMemoryTimer = XPL_CLK_HANDLE_INVALID;
#endif
}

DMLockContextManager::~DMLockContextManager()
{
    Destroy();
}


SYNCML_DM_RET_STATUS_T 
DMLockContextManager::Init(DMTree* tree, 
                                     CMultipleFileManager* ptrFM)
{
    if( !tree || !ptrFM ) 
        return SYNCML_DM_FAIL;
  
    m_ptrFM = ptrFM;
    m_pTree = tree;

    m_nLockWithConfig_ID[0] = SYNCML_DM_LOCKID_NONE;
    m_nLockWithConfig_ID[1] = SYNCML_DM_LOCKID_NONE;
 #ifndef DM_NO_LOCKING  
    int nLocksNumber =  m_ptrFM->GetFileNumber();

    if ( nLocksNumber >= 32 ) 
    {
        XPL_LOG_DM_TMN_Error(("DMLockContextManager::Init: too many files - simple locking supports only up to 31 files\n"));
        return SYNCML_DM_TOO_MANY_DATA_FILES;
    }

    DMString strLockFileNamePrefix; 

    strLockFileNamePrefix = "/tmp/dm";
    XPL_FS_MkDir(strLockFileNamePrefix);
    strLockFileNamePrefix += "/##lock##";

    DMString strName = strLockFileNamePrefix;
    strName += "g";
    m_pGlobalLockFile = new DMFileHandler(strName);

    if ( !m_pGlobalLockFile || m_pGlobalLockFile->open(XPL_FS_FILE_RDWR) != SYNCML_DM_SUCCESS )
        return SYNCML_DM_FAIL;
  
    return m_oLockManager.Init( strLockFileNamePrefix, nLocksNumber);
#else
    return SYNCML_DM_SUCCESS;
#endif
}

SYNCML_DM_RET_STATUS_T DMLockContextManager::Destroy()
{
    SYNCML_DM_RET_STATUS_T  result = SYNCML_DM_SUCCESS;
    if(m_ptrFM == NULL)
        return SYNCML_DM_SUCCESS;
  
    m_ptrFM = NULL;
    m_mapContexts.clear();

#ifndef DM_NO_LOCKING
    if ( m_pGlobalLockFile ) 
    {
        m_pGlobalLockFile->close();
        delete m_pGlobalLockFile;
        m_pGlobalLockFile = NULL;
    }  
    result = m_oLockManager.Destroy();
#endif

    m_pTree = NULL;
    return result;
}

SYNCML_DM_RET_STATUS_T 
DMLockContextManager::ValidateAndLockTNM( INT32& nID, 
                                                              CPCHAR szURI, 
                                                              CPCHAR szPrincipal, 
                                                              SYNCML_DM_TREE_LOCK_TYPE_T eLockType, 
                                                              BOOLEAN bInTransaction)
{

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    if ( DmGetMemFailedFlag() ) // device runs out of memory
        return SYNCML_DM_DEVICE_FULL;
  
#ifndef DM_NO_LOCKING
 
    m_csTNMLock.Enter();
    DMLockContext ctx;

    if ( m_mapContexts.lookup( nID, ctx ) ) 
    { 
        if ( eLockType == SYNCML_DM_LOCK_TYPE_EXCLUSIVE &&  eLockType != ctx.m_eLockType )
        { 
            m_csTNMLock.Leave();
            ReleaseID( nID, SYNCML_DM_RELEASE);
            return ValidateAndLockTNM( nID, szURI, szPrincipal, eLockType, bInTransaction );
        }

        // found appropriate lock; return without UNLOCKING if no error occurs
        m_pTree->SetServerId( szPrincipal ); // substitute principal

        m_nCurrentLockID = nID;
        return SYNCML_DM_SUCCESS;
    }
    
    m_csTNMLock.Leave(); // unlock area

    if ( bInTransaction ) //in transactions we do not upgrade the lock, but throw exception instead
        return SYNCML_DM_LOCKCTX_NOTFOUND;
  
    FILESETTYPE nFileSet = m_ptrFM->GetFileSetByURI( szURI, eLockType != SYNCML_DM_LOCK_TYPE_EXCLUSIVE );

    dm_stat = m_oLockManager.LockSet(nFileSet, eLockType);

    if ( dm_stat != SYNCML_DM_SUCCESS ) 
    {
        XPL_LOG_DM_TMN_Error(("unable to lock file set %d nFileSet %d with lock type %d error\n",nFileSet,eLockType,dm_stat));
        return dm_stat;
    }

    // Locked area again
    m_csTNMLock.Enter();
  
    if ( nFileSet )
    {
        XPL_LOG_DM_TMN_Debug(("LockFileSet szURI:%s\n", szURI));
        dm_stat = m_ptrFM->LockFileSet(szURI, nFileSet, eLockType );

        if ( dm_stat != SYNCML_DM_SUCCESS ) 
        {
            m_oLockManager.UnlockSet( nFileSet,eLockType );
            m_csTNMLock.Leave();
            XPL_LOG_DM_TMN_Error(("DM_LOCKING: failed to load files %d\n",dm_stat));
            return dm_stat;
        }
        
        XPL_LOG_DM_TMN_Debug(("after LockFileSet szURI:%s\n", szURI));
        if ( eLockType == SYNCML_DM_LOCK_TYPE_SHARED )
            m_oLockManager.UnlockSet( nFileSet,eLockType );
    }

    m_nNewUniqueID += 2; // 0 is predefined for new lock (non existing), start value is 1, so we never get 0
    nID = m_nNewUniqueID;

    DMLockContext oLC;

    oLC.Set(nFileSet, eLockType, szURI,FALSE);
  
    OnTreeAccessed();
   
    m_mapContexts.put(  nID, oLC );
    m_pTree->SetServerId( szPrincipal ); 
    m_nCurrentLockID = nID;
  
    return SYNCML_DM_SUCCESS;
#else
    DMLockContext ctx;

    FILESETTYPE nFileSet = m_ptrFM->GetFileSetByURI( szURI, FALSE );

    if ( m_mapContexts.lookup( nID, ctx ) ) 
    {
        m_pTree->SetServerId( szPrincipal ); 
        return SYNCML_DM_SUCCESS;
    }

        
    dm_stat = m_ptrFM->LockFileSet(szURI, nFileSet, eLockType);
    if ( dm_stat == SYNCML_DM_SUCCESS )
    {
        m_pTree->SetServerId( szPrincipal ); 
        DMLockContext oLC;

        oLC.Set(nFileSet, eLockType, szURI,FALSE);
  
        OnTreeAccessed();
        m_mapContexts.put(  nID, oLC );
        m_nCurrentLockID = nID;
   }
   else 
   {
        XPL_LOG_DM_TMN_Error(("DM_NO_LOCKING: failed to load files %d\n",dm_stat));
   }
   return dm_stat;
#endif
  
}

SYNCML_DM_RET_STATUS_T 
DMLockContextManager::ReleaseID( INT32 nID,
                                               SYNCML_DM_COMMAND_T command)
{
    DM_PROFILE ( "Release lock including unloading files" );
  
    SYNCML_DM_RET_STATUS_T dm_stat=SYNCML_DM_SUCCESS; 
      

    if ( SYNCML_DM_LOCKID_IGNORE == nID ) // ignore lock
        return SYNCML_DM_SUCCESS;

#ifndef DM_NO_LOCKING
    DMSingleLock oLock( m_csTNMLock );
#endif

    if ( SYNCML_DM_LOCKID_CURRENT == nID )
    { 
        if ( m_mapContexts.size() != 1 )
            return SYNCML_DM_LOCKCTX_NOTFOUND; // works only with one lock
      
        nID = m_mapContexts.get_key( 0 );
    }

    DMLockContext ctx;

    if ( !m_mapContexts.lookup(nID, ctx ) )
        return SYNCML_DM_LOCKCTX_NOTFOUND;

    dm_stat=ReleaseContext( nID, ctx, command );

    if ((command != SYNCML_DM_ATOMIC) && 
         (command != SYNCML_DM_COMMIT) && 
         ( m_mapContexts.size() == 0 ))
    {
        StopTimer();
    }   
    return dm_stat;
}


SYNCML_DM_RET_STATUS_T 
DMLockContextManager::ReleaseIDInternal(INT32 nID,
                                                         SYNCML_DM_COMMAND_T command)
{
    DM_PROFILE ( "Release lock including unloading files" );
  
    SYNCML_DM_RET_STATUS_T dm_stat=SYNCML_DM_SUCCESS; 
      
    if ( SYNCML_DM_LOCKID_IGNORE == nID ) // ignore lock
        return SYNCML_DM_SUCCESS;

    if ( SYNCML_DM_LOCKID_CURRENT == nID )
    { 
        if ( m_mapContexts.size() != 1 )
            return SYNCML_DM_LOCKCTX_NOTFOUND; // works only with one lock
      
        nID = m_mapContexts.get_key( 0 );
    }

    DMLockContext ctx;

    if ( !m_mapContexts.lookup(nID, ctx ) )
        return SYNCML_DM_LOCKCTX_NOTFOUND;

    dm_stat=ReleaseContext( nID, ctx, command );

    if (m_mapContexts.size() == 0 )
        StopTimer();

    return dm_stat;
}

SYNCML_DM_RET_STATUS_T 
DMLockContextManager::UnlockTNM()
{
#ifndef DM_NO_LOCKING   
    m_nCurrentLockID = SYNCML_DM_LOCKID_NONE;
    m_csTNMLock.Leave();
#endif  
  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T 
DMLockContextManager::ReleaseAll()
{
    SYNCML_DM_RET_STATUS_T dm_stat=SYNCML_DM_SUCCESS; 
#ifndef DM_NO_LOCKING     
    DMSingleLock oLock( m_csTNMLock );
#endif  
  
    StopTimer();

    for ( DMMap< int, DMLockContext >::POS nK = m_mapContexts.begin(); nK < m_mapContexts.end(); nK++ ) 
    {
        //Pass through errors if not the same 
        DMLockContext ctx = m_mapContexts.get_value( nK );
        SYNCML_DM_RET_STATUS_T retStatus1=ReleaseContext( m_mapContexts.get_key(nK),ctx, 
                                                      ctx.m_bAtomic ? SYNCML_DM_ROLLBACK : SYNCML_DM_RELEASE);
        if (dm_stat == SYNCML_DM_SUCCESS)
            dm_stat=retStatus1;
    }
    return dm_stat;
}



BOOLEAN 
DMLockContextManager::IsReleaseConfigFile(INT32 nLockID, 
                                                                        SYNCML_DM_FILE_TYPE_T & index)
{

    for (index=0; index<2; index++)
    {
        if ( nLockID == m_nLockWithConfig_ID[index] )
            return TRUE;
    }    
    return FALSE;

}


SYNCML_DM_RET_STATUS_T  
DMLockContextManager::ReleaseConfigFile(SYNCML_DM_FILE_TYPE_T index, 
                                                                     SYNCML_DM_RET_STATUS_T  dm_stat)
{
       if ( dm_stat == SYNCML_DM_SUCCESS  )
            return m_pTree->SaveFile(index);
        else
            return m_pTree->RevertFile(index);
   
}


SYNCML_DM_RET_STATUS_T
DMLockContextManager::ReleaseContext(INT32 nLockID, 
                                                        const DMLockContext& ctx, 
                                                        SYNCML_DM_COMMAND_T command)
{
    SYNCML_DM_RET_STATUS_T dm_stat=SYNCML_DM_SUCCESS; 

    SYNCML_DM_FILE_TYPE_T index=0;
    BOOLEAN bReleaseFile = IsReleaseConfigFile(nLockID,index);
  
    FILESETTYPE nLocksSet = ctx.m_nFileSet;


    if ( command == SYNCML_DM_COMMIT || command == SYNCML_DM_ROLLBACK )
    { // reset 'atomic' flag
        DMLockContext oUpdateCtx = ctx;
        oUpdateCtx.m_bAtomic = FALSE;
        m_mapContexts.put(  nLockID, oUpdateCtx );
    }

 
    switch(command) 
    {
        case SYNCML_DM_RELEASE:
            dm_stat=m_ptrFM->UnlockFileSet(ctx.m_strRoot.c_str(), nLocksSet, ctx.m_eLockType);   
            break;
          
        case SYNCML_DM_ROLLBACK:
            if ( bReleaseFile )
            {
                m_pTree->RevertFile(index);
            }

            dm_stat=m_ptrFM->RollbackFileSet(ctx.m_strRoot.c_str(), nLocksSet );   
            if (dm_stat != SYNCML_DM_SUCCESS)
                return dm_stat;
            
            dm_stat=m_ptrFM->LockFileSet(ctx.m_strRoot.c_str(), nLocksSet, ctx.m_eLockType );   
            return dm_stat;

        case SYNCML_DM_ATOMIC: // update context to set 'atomic' flag
        case SYNCML_DM_COMMIT:
            dm_stat=m_ptrFM->FlushFileSet(ctx.m_strRoot.c_str(), nLocksSet ,command);   
            if ( bReleaseFile ) 
            {
                ReleaseConfigFile(index,dm_stat);
            }
            if ( dm_stat == SYNCML_DM_SUCCESS && command == SYNCML_DM_ATOMIC )
            {
                DMLockContext oUpdateCtx = ctx;
                oUpdateCtx.m_bAtomic = TRUE;
                m_mapContexts.put(  nLockID, oUpdateCtx );
            }
            return dm_stat;

            default:
                return SYNCML_DM_INVALID_PARAMETER;
    }

#ifndef DM_NO_LOCKING  
    if ( ctx.m_eLockType != SYNCML_DM_LOCK_TYPE_SHARED )
        m_oLockManager.UnlockSet( nLocksSet, ctx.m_eLockType );
#endif  

    if ( bReleaseFile ) 
    {
         ReleaseConfigFile(index,dm_stat);
#ifndef DM_NO_LOCKING 
        m_nLockWithConfig_ID[index] = SYNCML_DM_LOCKID_NONE;
        m_oLockManager.ReleaseFile(index);
#endif
    }

    m_mapContexts.remove( nLockID );  // delete context

    if ( m_mapContexts.size() == 0 && DmGetMemFailedFlag() ) 
    { 
        DmResetMemFailedFlag();

        // force to unload all loaded sub-trees and plug-ins
        m_pTree->Flush(); 
    }
  
    return dm_stat;
}

#ifdef DM_NO_LOCKING
void DMLockContextManager::Lock() 
{ 
    XPL_LOG_DM_TMN_Debug(("Locking...\n"));
    m_bIsLocked = TRUE; 
    if ( m_nMemoryTimer != XPL_CLK_HANDLE_INVALID )
        XPL_CLK_StopTimer(m_nMemoryTimer);
    m_nMemoryTimer = XPL_CLK_HANDLE_INVALID;
}

void DMLockContextManager::UnLock() 
{
    XPL_LOG_DM_TMN_Debug(("UnLocking...\n"));
    XPL_LOG_DM_TMN_Debug(("and returning\n"));
    m_bIsLocked = FALSE; 
    if ( m_nAgingTime > 0 ) 
    {  
        if (  m_nMemoryTimer == XPL_CLK_HANDLE_INVALID )
            m_nMemoryTimer = XPL_CLK_StartTimer(XPL_PORT_DM_TASK, m_nAgingTime*1000, DmOnMemoryTimer);
    }
    else
    {
        SyncML_DM_Archiver& oArchiver = m_pTree->GetArchiver();
        oArchiver.CheckMemoryAging(m_nAgingTime);

        DMPluginManager & oPluginManager = m_pTree->GetPluginManager();
        oPluginManager.CheckPluginAging(m_nAgingTime);
    }
}
#endif

void DMLockContextManager::OnTreeAccessed()
{
#ifndef DM_NO_LOCKING  
    if ( m_oLockManager.GetQueue() != NULL )
    {
#ifdef DEBUG
        m_oLockManager.GetQueue()->SetTimer( 7000 );  
#else
        m_oLockManager.GetQueue()->SetTimer( 3000 );  
#endif
    }

#endif
}

void DMLockContextManager::StopTimer()
{
#ifndef DM_NO_LOCKING  
    // DP: the only place where queue ptr can be null, is here during failed initi function call.
    if ( m_oLockManager.GetQueue() != NULL )
        m_oLockManager.GetQueue()->KillTimer();  
#endif
}

SYNCML_DM_RET_STATUS_T DMLockContextManager::GlobalLock() 
{
#ifndef DM_NO_LOCKING  
    if ( !m_pGlobalLockFile )
        return SYNCML_DM_INVALID_PARAMETER;

    SYNCML_DM_RET_STATUS_T dm_stat;

     while ( (dm_stat = m_pGlobalLockFile->lock(TRUE) ) == SYNCML_DM_LOCK_TRY_AGAIN )
        DmThSleep( 1000 );

    return dm_stat;
#else
    return SYNCML_DM_SUCCESS;
#endif
 
}

SYNCML_DM_RET_STATUS_T DMLockContextManager::GlobalUnlock()
{
#ifndef DM_NO_LOCKING  
    if ( !m_pGlobalLockFile )
        return SYNCML_DM_INVALID_PARAMETER;

    return m_pGlobalLockFile->unlock();
#else
    return SYNCML_DM_SUCCESS;  
#endif  
}

void DMLockContextManager::InsureFileLocked(SYNCML_DM_FILE_TYPE_T eFileType)
{

    if ( m_nCurrentLockID == m_nLockWithConfig_ID[eFileType] )
        return; // already locked by us
    
    DMString strPrincipal = m_pTree->GetServerId();
    int nCurID = m_nCurrentLockID;
    DMLockContext ctx;

    // temporary unlock the tree cs, unregister context to prevent expiration of it and wait for acl lock
    if ( !m_mapContexts.lookup( nCurID, ctx ) ) 
    {
        XPL_LOG_DM_TMN_Debug(("DM: ACL locking internal error!\n"));
        return; // some error happened
    }
    m_mapContexts.remove(nCurID);
    UnlockTNM();
  
#ifndef DM_NO_LOCKING     
    // acquire lock
    m_oLockManager.AcquireFile( eFileType );

    // restore state
    m_csTNMLock.Enter();
#endif  
    m_mapContexts.put(  nCurID, ctx );
    m_pTree->SetServerId( strPrincipal ); // substitute principal
    m_nCurrentLockID = nCurID;
    m_nLockWithConfig_ID[eFileType] = nCurID;
}


void DMLockContextManager::CheckMemoryAging()
{
    if ( m_nAgingTime <= 0 ) 
        return;
    
#ifndef DM_NO_LOCKING
    DMSingleLock oLock(m_csTNMLock);
    if ( m_mapContexts.size() == 0 ) 
    {
#else
    if ( m_nMemoryTimer != XPL_CLK_HANDLE_INVALID && !m_bIsLocked ) 
    {
        XPL_CLK_StopTimer(m_nMemoryTimer);
        m_nMemoryTimer = XPL_CLK_HANDLE_INVALID;
#endif 
        SyncML_DM_Archiver& oArchiver = m_pTree->GetArchiver();
        oArchiver.CheckMemoryAging(m_nAgingTime);

        DMPluginManager & oPluginManager = m_pTree->GetPluginManager();
        oPluginManager.CheckPluginAging(m_nAgingTime);
    }
}
