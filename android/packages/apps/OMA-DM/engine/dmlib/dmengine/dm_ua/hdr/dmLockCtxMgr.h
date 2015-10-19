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

#ifndef DMLOCKCTXMANAGER_H
#define DMLOCKCTXMANAGER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: dmLockCtxMgr.h

    General Description: This file contains declaration of the DMLockContextManager  class

==================================================================================================*/

#include "dmLock.h"
#ifndef DM_NO_LOCKING
#include "dmThreadQueue.h"
#endif

class DMLockContext
{

public:
  inline DMLockContext() {m_nFileSet = 0; m_eLockType = 0; m_bAtomic = FALSE;}

  void Set(FILESETTYPE nFileSet, 
              SYNCML_DM_TREE_LOCK_TYPE_T eLockType,
              DMString strRoot,
              BOOLEAN bAtomic)
  {
       m_nFileSet = nFileSet;
       m_eLockType = eLockType;  
       m_strRoot = strRoot;
       m_bAtomic = bAtomic;
  }

  /** File set to apply lock */
  FILESETTYPE m_nFileSet;
  /** Lock type */
  SYNCML_DM_TREE_LOCK_TYPE_T m_eLockType;
  /** Root path of sub tree */
  DMString m_strRoot; 
  /** Flag to check if operation is an atomic */
  BOOLEAN m_bAtomic;
};

class DMLockContextManager
{
public:

  /**
  * Default constructor
  */  
  DMLockContextManager() ;

/**
  * Destructor
  */
  ~DMLockContextManager() ;

  /**
  * Initializes locking manager
  * \param pTree [in] - pointer on DM tree
  * \param pFileManager [in] - pointer on file manager
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T Init(DMTree* pTree, 
                              CMultipleFileManager* pFileManager) ;

  /**
  * Destroys/cleanup locking manager
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T Destroy() ;


  /**
  * Validates lock context
  * \param nID [in/out] - lock id
  * \param szURI [in] - locking path
  * \param szPrincipal [in] - principal that attempt to get tree access
  * \param eLockType [in] - lock type (read only/shared/exclusive)  
  * \param bInTransaction [in] - specifies if DMT within atomic transaction  
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully and global lock is aquired. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T ValidateAndLockTNM(INT32& nID, 
                                             CPCHAR szURI, 
                                             CPCHAR szPrincipal,
                                             SYNCML_DM_TREE_LOCK_TYPE_T eLockType, 
                                             BOOLEAN bInTransaction );

  /**
  * Releases locks and all corresponding files; acquire global lock internaly only 
  * \param nID [in] - lock id
  * \param command [in] - command performed on DMT  
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully and global lock is aquired. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T ReleaseID(INT32 nID,
                                                        SYNCML_DM_COMMAND_T command);

  /**
  * Releases locks and all corresponding files; does not acquire global lock internaly
  * \param nID [in] - lock id
  * \param command [in] - command performed on DMT  
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully and global lock is acquired. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T ReleaseIDInternal(INT32 nID,
                                                                SYNCML_DM_COMMAND_T command);

  /**
  * Releases global lock
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully and global lock is acquired. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T UnlockTNM();

  /**
  * Releases all locks
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully and global lock is acquired. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T ReleaseAll();

  /**
  * Starts auto-release timer
  */ 
  void  OnTreeAccessed();

#ifndef DM_NO_LOCKING	 
 /**
  * Returns critical section to enable direct engine locking - use with caution
  */ 
  inline DMCriticalSection& GetEngineLock() { return  m_csTNMLock;}
#else

  /**
  * Verifies if DMT is locked
  * \returns TRUE if DMT is locked 
  */ 
  inline BOOLEAN IsLocked() const { return m_bIsLocked; }

  /**
  * Locks DMT
  */ 
  void Lock();

  /**
  * Unlocks DMT
  */ 
  void UnLock();
  
#endif


 /**
  * Acquires global lock for recovery or persistence
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully and global lock is aquired. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T GlobalLock() ;

  /**
  * Releases global lock for recovery or persistence
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully and global lock is aquired. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T GlobalUnlock() ;

  /**
  * Locks event or acl file
  */ 
  void InsureFileLocked(SYNCML_DM_FILE_TYPE_T eLockType);

  /**
  * Checks memory aging
  */ 
  void CheckMemoryAging();
  
private:
  /**
  * Copy constructor
  */ 
  DMLockContextManager( const DMLockContextManager& );
  const DMLockContextManager&  operator=( const DMLockContextManager& );

  /**
  * Stops auto-release timer
  */
  void StopTimer() ;

  /**
  * Releases lock context by lock id
  * \param nID [in] - lock id
  * \param ctx [in] - lock context
  * \param command [in] - command performed on DMT  
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully and global lock is acquired. 
  * - All other codes indicate failure. 
  */ 
  SYNCML_DM_RET_STATUS_T ReleaseContext( INT32 nLockID,
                                             const DMLockContext& ctx, 
                                             SYNCML_DM_COMMAND_T command);

  /**
  * Checks if config file should be released
 * \param nID [in] - lock id
  * \param index [out] - index of file to be released
  * \returns TRUE if lock ID is found
  */ 
  BOOLEAN IsReleaseConfigFile(INT32 nLockID, 
                                                    SYNCML_DM_FILE_TYPE_T & index);

  /**
  * Releases/revert config file based on result of previous operation
  * \param index [out] - index of file to be released
  * \param dm_stat [in] - result of previous operation
  * \return Return Type (SYNCML_DM_RET_STATUS_T) 
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully and global lock is acquired. 
  * - All other codes indicate failure. 
  */  
  SYNCML_DM_RET_STATUS_T ReleaseConfigFile(SYNCML_DM_FILE_TYPE_T index, 
                                                                 SYNCML_DM_RET_STATUS_T  dm_stat);

  /** Tree pointer */
  DMTree  *m_pTree;
  /** Unique id for next context */
  INT32 m_nNewUniqueID;     
  /** Map of locked context */
  DMMap< INT32, DMLockContext >  m_mapContexts;
  /** Pointer on file manager */
  CMultipleFileManager  *m_ptrFM;
  /** Tree pointer */
  INT32  m_nCurrentLockID;
  /** Lock Ids who owns acl and evt */
  SYNCML_DM_FILE_TYPE_T m_nLockWithConfig_ID[2]; 
  /** Memory aging time */
  INT32 m_nAgingTime;
#ifndef DM_NO_LOCKING
  /** Global lock to share access to DMT */
  DMCriticalSection m_csTNMLock;  
  /** Lock manager */
  DMLockManager  m_oLockManager;
  /** File to apply global lock */
  DMFileHandler*  m_pGlobalLockFile;
#else
  /** Timer handler for memory aging */
  XPL_TIMER_HANDLE_T m_nMemoryTimer;
   /** Locking flag */
  BOOLEAN                       m_bIsLocked;
#endif
};

/*================================================================================================*/
#endif 
