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

    Source Name: SYNCML_DM_Lock.cc

    General Description: Implementation of the DMFileLockingThread, Clock and  DMLockManager classes

==================================================================================================*/

#include "dmLock.h"
#include "xpl_Logger.h"
#include "xpl_dm_Manager.h"
#include "dm_tree_class.H"

#ifndef DM_NO_LOCKING 
// memory aging check interval in seconds
#define DEFAULT_AGING_CHECK_INTERVAL 60

// Static member
DMCriticalSection DMLock::m_csLock;

DMFileLockingThread::DMFileLockingThread( DMLockManager* pLockManager )
{
    m_nReady=SYNCML_DM_THREAD_STARTING;
    m_pLockManager = pLockManager;

    m_nAgingCheckInterval = DEFAULT_AGING_CHECK_INTERVAL;
    CPCHAR dm_aging_env = XPL_DM_GetEnv(SYNCML_DM_MEMORY_AGING_INTERVAL);
    if ( dm_aging_env != NULL) {
        INT32 interval = DmAtoi(dm_aging_env);
        if (interval >= 0) {
            m_nAgingCheckInterval = interval;
        } else {
            m_nAgingCheckInterval = 0;
        }
    }
} 

void* DMFileLockingThread ::Run()
{
  BOOLEAN bDone = FALSE;
  DMVector< DMFileLockParam > aMessages;

  m_nReady = SYNCML_DM_THREAD_STARTED;
  INT32 nAgingCheck = m_nAgingCheckInterval;
  INT32 nElapsed = 0;
  XPL_CLK_CLOCK_T startTime, endTime;

  while(!bDone)
  {
    INT64 nTimeoutMS;
    if (m_nAgingCheckInterval <= 0) 
    {
      nTimeoutMS = (aMessages.size() == 0 ? DM_WAIT_FOREVER : 100 );
    } 
    else 
    {
        nTimeoutMS = (aMessages.size()==0 ? (nAgingCheck * 1000) : 100 );
    }
    startTime = XPL_CLK_GetClock();
    DMThreadEvent evt;
    
    if ( m_pLockManager->GetQueue()->Wait(nTimeoutMS, evt)) 
    {
      switch ( evt.GetEventType() )
      {
        case SYNCML_DM_THREAD_EVENT_TYPE_TIMEOUT:
          dmTreeObj.GetLockContextManager().ReleaseAll();
          break;
        case SYNCML_DM_THREAD_EVENT_TYPE_FILELOCK:
          {
              DMFileLockParam* ptr = (DMFileLockParam*)evt.GetData();
              aMessages.push_back( *ptr );
              DmFreeMem( ptr );
              break;
          }
        case SYNCML_DM_THREAD_EVENT_TYPE_SHUTDOWN:
          bDone = true;
          break;
        default:
        case SYNCML_DM_THREAD_EVENT_TYPE_NONE:
        {
          // DM: just to supress warning, do nothing here
          break;
        }
      }

      endTime = XPL_CLK_GetClock();
      nAgingCheck -= (int)(endTime - startTime);
      nElapsed += (int)(endTime - startTime);
    }
    else 
    {
    
      endTime = XPL_CLK_GetClock();
      nElapsed += (int)(endTime - startTime);

      if ( (nTimeoutMS == nAgingCheck * 1000) && 
           (!m_pLockManager->GetQueue()->IsTimerSet()) ) 
      {
        XPL_LOG_DM_TMN_Debug(("Memory aging time out = %d sec.\n", nElapsed));
        dmTreeObj.CheckMemoryAging();
        nAgingCheck = m_nAgingCheckInterval;
        nElapsed = 0;
      } 
      else 
      {
          nAgingCheck -= (int)(endTime - startTime);
      }
    }

    if (nAgingCheck < 1) 
    {
        endTime = XPL_CLK_GetClock();
               XPL_LOG_DM_TMN_Debug(("Memory aging time out = %d sec.\n", nElapsed ));
        nAgingCheck = 1;
    }

    // try to lock/unlock all postponed requests:
    for ( int i = 0; i < aMessages.size(); i++ ) {
      DMFileLockParam& pFileLockMsg = aMessages[i];

      if ( m_pLockManager->ProcessLockRequest( &pFileLockMsg ) ) {
        aMessages.remove(i);
        i--;  // don't skip next
      }
    }
  }
  
  m_nReady = SYNCML_DM_THREAD_STARTING;

  return NULL;
}

DMLock::DMLock()
{
  m_pFile = NULL;
  m_nReadWriteCounter = 0;
}

DMLock::~DMLock()
{
  if (m_pFile)
  {
    Destroy();
  }
}

SYNCML_DM_RET_STATUS_T
DMLock::Init( CPCHAR p_LockFileName)
{
  m_nReadWriteCounter = 0;
  m_pFile = new DMFileHandler(p_LockFileName);

  SYNCML_DM_RET_STATUS_T nRes =  m_pFile->open(XPL_FS_FILE_RDWR);

  if ( nRes != SYNCML_DM_SUCCESS )
    Destroy();
  
  return nRes;
}

SYNCML_DM_RET_STATUS_T
DMLock::Destroy(void)
{
  if (!m_pFile)
    return SYNCML_DM_SUCCESS;

  m_pFile->unlock();
  m_pFile->close();
  delete m_pFile;
  m_pFile = NULL;

  return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T 
DMLock::Lock( SYNCML_DM_TREE_LOCK_TYPE_T eLockType)
{
    DMSingleLock oLock( DMLock::m_csLock );

    INT32 nPrevCounter = m_nReadWriteCounter;
    SYNCML_DM_RET_STATUS_T nRes = SYNCML_DM_SUCCESS;

    if ( eLockType == SYNCML_DM_LOCK_TYPE_EXCLUSIVE ) 
    {
        if ( m_nReadWriteCounter != 0 ) 
            return SYNCML_DM_LOCK_TRY_AGAIN; // not available
        
        m_nReadWriteCounter = -1; // writer
    }
    else
    { // shared lock
        if ( m_nReadWriteCounter < 0 ) 
            return SYNCML_DM_LOCK_TRY_AGAIN; // writer in progress
    
        m_nReadWriteCounter++; // reader
    }

  
    if ( m_nReadWriteCounter <= 1 ) 
    { // only first reader/writer should lock the file
        if (!m_pFile)
            return SYNCML_DM_FAIL;

        if ( eLockType == SYNCML_DM_LOCK_TYPE_EXCLUSIVE )
            nRes = m_pFile->lock( TRUE );
        else 
            nRes = m_pFile->lock( FALSE );

        if ( nRes == SYNCML_DM_LOCK_TRY_AGAIN )
            m_nReadWriteCounter = nPrevCounter;
    }
  
    return nRes;
}


SYNCML_DM_RET_STATUS_T DMLock::Unlock()
{
  SYNCML_DM_RET_STATUS_T nRes = SYNCML_DM_SUCCESS;
  DMSingleLock oLock( DMLock::m_csLock );

  if ( m_pFile && IsLastLock() )
    nRes = m_pFile->unlock();

  if ( m_nReadWriteCounter < 0 )
    m_nReadWriteCounter = 0; // writer
  else
    m_nReadWriteCounter--;

  return nRes;
}


DMLockManager::DMLockManager() :
  m_oThread( this )
{
 m_ptrQueue = NULL;
}

DMLockManager::~DMLockManager()
{
  Destroy();
}

SYNCML_DM_RET_STATUS_T DMLockManager::Init( CPCHAR szLockFileNamePrefix, INT32 nNumberOfLocks )
{

  nNumberOfLocks += 2; // count for ACL lock as well (the last one) and Evt lock
  m_aLocks.set_size( nNumberOfLocks );
  if ( m_aLocks.size() != nNumberOfLocks )
    {
       m_aLocks.clear();
       return SYNCML_DM_DEVICE_FULL;
    }

  SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

  while( nNumberOfLocks-- )
  {
    char szIndex[10];
    DMString strName = szLockFileNamePrefix;
    DmSnprintf( szIndex, sizeof(szIndex), "%d", nNumberOfLocks );
    strName += szIndex;
    
    dm_stat = m_aLocks[ nNumberOfLocks ].Init( strName.c_str() );

    if ( dm_stat  != SYNCML_DM_SUCCESS )
        return dm_stat;
  }
  
  return dm_stat;
}


SYNCML_DM_RET_STATUS_T DMLockManager::StartThread()
{

    if ( m_ptrQueue == NULL )
    {
        m_ptrQueue = new DMThreadQueue;
        if( m_ptrQueue == NULL)
            return SYNCML_DM_DEVICE_FULL;
  
        if ( !m_oThread.StartThread() )
            return SYNCML_DM_UNABLE_START_THREAD;


        INT32 nThreadState;
      
        while ( (nThreadState = m_oThread.GetThreadState()) == SYNCML_DM_THREAD_STARTING )
            DmThSleep( 1000 ); /// give a chance to start

        if ( nThreadState != SYNCML_DM_THREAD_STARTED ) 
        {
            Destroy();
            return SYNCML_DM_UNABLE_START_THREAD;
        }
    }
  
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DMLockManager::Destroy()
{
    if(m_ptrQueue != NULL)
    {
        if ( m_oThread.GetThreadState() == SYNCML_DM_THREAD_STARTED ) 
        {
            m_ptrQueue->Post(SYNCML_DM_THREAD_EVENT_TYPE_SHUTDOWN);
            m_oThread.StopThread();
        
        }
      m_ptrQueue = NULL;
    }

     //release locks

        for( int i = 0; i < m_aLocks.size(); i++ )
            m_aLocks[i].Destroy();

        m_aLocks.clear();
        
    
    return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T DMLockManager::LockSet( FILESETTYPE nLockSet, 
                                               SYNCML_DM_TREE_LOCK_TYPE_T eLockType )
{

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;

    if  ( eLockType != SYNCML_DM_LOCK_TYPE_SHARED ) 
    {
        dm_stat = StartThread();
        if ( dm_stat != SYNCML_DM_SUCCESS )
        return dm_stat;

        dm_stat = SendAndWait( TRUE, nLockSet, eLockType );
    }    
    else
    {
        INT32 nNumber = m_aLocks.size();

        for ( int i = 0; i < nNumber; i++ )
        {
            if ( (1 << i) & nLockSet ) 
            {
                while( m_aLocks[i].Lock( eLockType ) == SYNCML_DM_LOCK_TRY_AGAIN )
                    DmThSleep( 1000 );
            }
        }
    }    
    return dm_stat;
}

SYNCML_DM_RET_STATUS_T 
DMLockManager::UnlockSet( FILESETTYPE nLockSet, 
                                           SYNCML_DM_TREE_LOCK_TYPE_T eLockType )
{
    if ( eLockType != SYNCML_DM_LOCK_TYPE_SHARED )
        return SendAndWait( FALSE, nLockSet, 0 );
    else
    {
        INT32 nNumber = m_aLocks.size();

        for ( int i = 0; i < nNumber; i++ )
        {
            if ( (1 << i) & nLockSet ) 
            {
                m_aLocks[i].Unlock();
            }
        }
        return SYNCML_DM_SUCCESS;
    }
}

SYNCML_DM_RET_STATUS_T DMLockManager::SendAndWait(BOOLEAN bIsLock, FILESETTYPE nLockSet, INT32 eLockType )
{
  BOOLEAN bOK = FALSE;

  while ( !bOK ) {
    DMFileLockParam* pFileLockMsg = (DMFileLockParam*)DmAllocMem(sizeof(DMFileLockParam) );

    if ( !pFileLockMsg )
      return SYNCML_DM_DEVICE_FULL;

    int nStatus = SYNCML_DM_FILE_LOCK_STATUS_IN_PROGRESS; 

    pFileLockMsg->m_bIsLock = bIsLock;
    pFileLockMsg->m_pDoneFlag = bIsLock ? &nStatus : NULL;
    pFileLockMsg->m_eLockType = eLockType;
    pFileLockMsg->m_nLockSet = nLockSet;

    if ( !m_ptrQueue->Post( SYNCML_DM_THREAD_EVENT_TYPE_FILELOCK, pFileLockMsg) ) {
      DmFreeMem(pFileLockMsg);
      return SYNCML_DM_FAIL;
    }
      
    if ( !bIsLock )
      break;
  
    // wait for "done" flag marked
    while ( nStatus == SYNCML_DM_FILE_LOCK_STATUS_IN_PROGRESS )
      DmThSleep(1000);

    if ( nStatus != SYNCML_DM_FILE_LOCK_STATUS_TRY_AGAIN )
      return nStatus == SYNCML_DM_FILE_LOCK_STATUS_OK ? SYNCML_DM_SUCCESS : SYNCML_DM_FAIL;
  }
  
  return SYNCML_DM_SUCCESS;
}


BOOLEAN DMLockManager::ProcessLockRequest( DMFileLockParam* pFileLockMsg )
{
  INT32 nNumber = m_aLocks.size();
  SYNCML_DM_RET_STATUS_T nRes = SYNCML_DM_SUCCESS;

  for ( int i = 0; i < nNumber; i++ ){
    if ( (1 << i) & pFileLockMsg->m_nLockSet ) {
      if ( !pFileLockMsg->m_bIsLock ) 
      {
        nRes = m_aLocks[i].Unlock();
      }
      else 
      {
        if ( (nRes = m_aLocks[i].Lock( pFileLockMsg->m_eLockType)) == SYNCML_DM_LOCK_TRY_AGAIN )
          return FALSE;
      }
      // mark processed files
      pFileLockMsg->m_nLockSet &= ~((FILESETTYPE)((FILESETTYPE)1 << i));
    }
  }

  if ( pFileLockMsg->m_pDoneFlag )
    *(pFileLockMsg->m_pDoneFlag) = (nRes == SYNCML_DM_SUCCESS ? SYNCML_DM_FILE_LOCK_STATUS_OK : SYNCML_DM_FILE_LOCK_STATUS_ERROR);

  return TRUE;
}


SYNCML_DM_RET_STATUS_T 
DMLockManager::ReleaseFile(SYNCML_DM_FILE_TYPE_T eFileType)
{
    if ( eFileType ==  SYNCML_DM_FILE_ACL ) 
        return SendAndWait( FALSE, 1 << (m_aLocks.size()-1), 0 );
    else
        return SendAndWait( FALSE, 1 << (m_aLocks.size()-2), 0 );
}

SYNCML_DM_RET_STATUS_T 
DMLockManager::AcquireFile(SYNCML_DM_FILE_TYPE_T eFileType )
{
    if ( eFileType ==  SYNCML_DM_FILE_ACL ) 
        return LockSet( 1 << (m_aLocks.size()-1), SYNCML_DM_LOCK_TYPE_EXCLUSIVE );
    else
        return LockSet( 1 << (m_aLocks.size()-2), SYNCML_DM_LOCK_TYPE_EXCLUSIVE );
}

#endif 
