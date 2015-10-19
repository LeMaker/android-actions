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

#ifndef DMLOCK_H
#define DMLOCK_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: dmLock.h

    General Description: Declaration of DMFileLockingThread, Clock and DMLockManager classes.

==================================================================================================*/

#include "dmdefs.h"
#include "SyncML_DM_FileHandle.H"
#include "dmvector.h"
#include "file_manager.h"
#include "dmtDefs.h"

#ifndef DM_NO_LOCKING 
#include "dmThreadQueue.h"    
#endif

enum {
     SYNCML_DM_LOCKID_NONE = 0, 
     SYNCML_DM_LOCKID_IGNORE = 2,
     SYNCML_DM_LOCKID_CURRENT = 4
}; // predefined lock ids: ignore - means "no lock", current - means "whatever is registered"
typedef UINT8 SYNCML_DM_LOCKID_T;

enum {
     SYNCML_DM_FILE_LOCK_STATUS_IN_PROGRESS, 
     SYNCML_DM_FILE_LOCK_STATUS_OK, 
     SYNCML_DM_FILE_LOCK_STATUS_TRY_AGAIN, 
     SYNCML_DM_FILE_LOCK_STATUS_ERROR
};
typedef UINT8 SYNCML_DM_FILE_LOCK_STATUS_T;

enum { 
    SYNCML_DM_THREAD_STARTING, 
    SYNCML_DM_THREAD_STARTED, 
    SYNCML_DM_THREAD_FAILED 
};
typedef UINT8 SYNCML_DM_THREAD_STATUS_T;


enum {
    /** Lock applied on ACL file */
    SYNCML_DM_FILE_ACL = 0,
    /** Lock applied on Event file */
    SYNCML_DM_FILE_EVENT = 1,
};
typedef UINT8 SYNCML_DM_FILE_TYPE_T;


#ifndef DM_NO_LOCKING 
struct DMFileLockParam 
{
  INT32* m_pDoneFlag;
  BOOLEAN  m_bIsLock;
  FILESETTYPE m_nLockSet;
  SYNCML_DM_TREE_LOCK_TYPE_T m_eLockType;
};

class DMLock 
{
public:
   /**
  * Default constructor
  */
  DMLock() ;

   /**
  * Destructor
  */
  ~DMLock() ;

  /**
  * Initializes file lock
  * \param szLockFileName [in] - lock file name
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T Init( CPCHAR szLockFileName) ;

   /**
  * Cleanups file lock
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T Destroy() ;

  /**
  * Locks file 
  * \param eLockType [in] - type of DM lock
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T Lock( SYNCML_DM_TREE_LOCK_TYPE_T eLockType); 

  /**
  * Unlocks file 
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T Unlock() ;

  /**
  * Verifies if lock counter equal 1 (last reader)
  */  
  inline BOOLEAN IsLastLock() {  return ( (m_nReadWriteCounter <= 1) ? TRUE : FALSE ); } 

protected:
  /** Tree pointer */
  DMFileHandler* m_pFile;
  /**Number of readers (>0) or writers (<0)*/
  INT32 m_nReadWriteCounter; 

public:
  /** Saves system handles and use only 1 mutex, since it's locked time very short */
  static  DMCriticalSection m_csLock ;

};

class DMLockManager;


class DMFileLockingThread : public DMThread
{
public:

  /**
  * Constructor
  * \param pLockManager [in] - pointer on file lock manager
  */
  DMFileLockingThread ( DMLockManager* pLockManager ) ;

  /**
  * Retrieves thread state 
  */  
  SYNCML_DM_THREAD_STATUS_T GetThreadState() const;
  
protected:
   /**
  * Thread run method 
  */  
  virtual void* Run(); 
   /**Thread state */
  SYNCML_DM_THREAD_STATUS_T m_nReady; 
  /** Memory check interval time in msec */
  INT32 m_nAgingCheckInterval;
  /** File lock manager */
  DMLockManager  * m_pLockManager;
};


class DMLockManager
{
public:
  /**
  * Default constructor
  */
  DMLockManager() ;

  /**
  * Destructor
  */
  ~DMLockManager() ;

  /**
  * Initializes locking manager
  * \param szLockFileNamePrefix [in] - lock file name preffix
  * \param nNumberOfLocks [in] - maximum number of locks to be applied
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T Init(CPCHAR szLockFileNamePrefix,
                                                 INT32 nNumberOfLocks ) ;

  /**
  * Cleanups locking manager
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T Destroy() ;

  /**
  * Locks file set
  * \param nLockSet [in] - lock set
  * \param eLockType [in] - type of lock
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T LockSet( FILESETTYPE nLockSet, 
                                   SYNCML_DM_TREE_LOCK_TYPE_T eLockType) ;

 /**
  * Unlocks file set
  * \param nLockSet [in] - lock set
  * \param eLockType [in] - type of lock
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T UnlockSet( FILESETTYPE nLockSet,
                                          SYNCML_DM_TREE_LOCK_TYPE_T eLockType) ;

  /**
  * Processes lock request by service thread
  * \param pFileLockMsg [in] - locking request
  * \returns TRUE if request is successful 
  */  
  BOOLEAN ProcessLockRequest( DMFileLockParam* pFileLockMsg ) ;

  /**
  * Retrieves pointer on message queue
  */  
  DMThreadQueue* GetQueue() const {return m_ptrQueue;}

  
  /**
  * Unlocks acl or event file
  * \param eFileType [in] - type of a file
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T ReleaseFile(SYNCML_DM_FILE_TYPE_T eFileType);

   /**
  * Unlocks acl or event file
  * \param eFileType [in] - type of a file
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T AcquireFile(SYNCML_DM_FILE_TYPE_T eFileType); 
  
private:
  /**
  * Sends locking request to management thread Unlocks acl or event file
  * \param nOperation [in] - type requested operation (lock/unlock)
  * \param nLockSet [in] - file set to lock
  * \param eLockType [in] - type of requested lock
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T SendAndWait(BOOLEAN bIsLock, 
                                                     FILESETTYPE nLockSet, 
                                                     INT32 eLockType ) ;
  
private:
  /** Vector of locks */
  DMVector<DMLock>  m_aLocks;
  /** Housekeeping thread */
  DMFileLockingThread  m_oThread;
  /** Tree pointer */
  PDMThreadQueue    m_ptrQueue;

  /**
  * Starts DM house keeping thread
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */
  SYNCML_DM_RET_STATUS_T StartThread();
  
};

inline SYNCML_DM_THREAD_STATUS_T DMFileLockingThread::GetThreadState() const  
{
  return m_nReady;
}

#endif  // !DM_NO_LOCKING
#endif  // !DMLOCK_H
