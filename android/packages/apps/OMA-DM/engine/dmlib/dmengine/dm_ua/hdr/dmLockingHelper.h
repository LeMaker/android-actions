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

#ifndef DKLOCKINGHELPER_H
#define DMLOCKINGHELPER_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: dmLockingHelper.h

    General Description: This file contains declaration of the SYNCML_DM_LockCtxMgr class

==================================================================================================*/

#include "dmLockCtxMgr.h"

/**
* helper class to init/done locking stuff 
*/
class DmtTreeImpl;

class DMLockingHelper
{
public:

  /**
  * Default constructor
  * \param nID [in] - lock id
  * \param szURI [in] - locking path
  * \param szPrincipal [in] - principal that attempt to get tree access
  * \param eLockType [in] - lock type (read only/shared/exclusive)  
  * \param bInTransaction [in] - specifies if DMT within atomic transaction  
  */  

  DMLockingHelper() 
  {
    m_nLockID = 0; m_bIDChanged = FALSE; m_nError = SYNCML_DM_FAIL;
  }

  /**
  * Constructor
  * \param nID [in] - lock id
  * \param szURI [in] - locking path
  * \param szPrincipal [in] - principal that attempt to get tree access
  * \param eLockType [in] - lock type (read only/shared/exclusive)  
  * \param bInTransaction [in] - specifies if DMT within atomic transaction  
  */  
  DMLockingHelper( INT32 nCurID, 
                   CPCHAR szURI, 
                   CPCHAR szPrincipal, 
                   INT32 eLockType, 
                   BOOLEAN bInTransaction )
  {
      Lock(nCurID, szURI, szPrincipal, eLockType, bInTransaction);
  }


  /**
  * Constructor
  * \param pTree [in] - pointer on tree to be locked
  * \param eLockType [in] - lock type (read only/shared/exclusive)  
  */  

  DMLockingHelper(DmtTreeImpl* pTree, 
                                    SYNCML_DM_TREE_LOCK_TYPE_T eLockType );
  

  /**
  * Destructor
  */  
  ~DMLockingHelper();

  /**
  * Verifies if tree is locked successfully
  * returns TRUE if subtree is locked successfully
  */  
  BOOLEAN IsLockedSuccessfully() const {return m_nError == SYNCML_DM_SUCCESS;}

   /**
  * Retrieves DM status code
  */  
  SYNCML_DM_RET_STATUS_T GetError() const { return m_nError; }

   /**
  * Verifies if lock ID is updated
  * returns TRUE if lock ID is changed
  */    
  BOOLEAN IsIDChanged() const {return m_bIDChanged;}

   /**
  * Retrieves lock ID
  */  
  INT32  GetID() const {return m_nLockID;}
  
protected:
  /** Lock ID */
  INT32  m_nLockID;
  /** Locking status */
  SYNCML_DM_RET_STATUS_T m_nError;
  /** Lock update flag */
  BOOLEAN m_bIDChanged;

  /**
  * Locks subtree
  * \param nID [in] - lock id
  * \param szURI [in] - locking path
  * \param szPrincipal [in] - principal that attempt to get tree access
  * \param eLockType [in] - lock type (read only/shared/exclusive)  
  * \param bInTransaction [in] - specifies if DMT within atomic transaction  
  * returns TRUE if subtree is locked successfully
  */  
  BOOLEAN Lock( INT32 nCurID, 
                 CPCHAR szURI, 
                 CPCHAR szPrincipal, 
                 SYNCML_DM_TREE_LOCK_TYPE_T eLockType, 
                 BOOLEAN bInTransaction );

};


/** 
*     ShortTime global interprocess lock (file-based);
*     used for file operations to prevent race-conditions during power-loss recovery.
*/ 
struct DMGlobalLockHelper
{
   /**
   * Default constructor
   */
    DMGlobalLockHelper(); 

   /**
   * Destructor
   */  
   ~DMGlobalLockHelper();
};

/**
* Special lock for OMA toolkit workspace - it can't be shared in one process
*/
struct DMGlobalOMAWorkspaceSharing
{
    /**
   * Default constructor
   */
    DMGlobalOMAWorkspaceSharing();

    /**
   * Destructor
   */ 
    ~DMGlobalOMAWorkspaceSharing();

private:
#ifndef DM_NO_LOCKING 
  /** Critical section */
  static DMCriticalSection m_csInitLock;
  /** Flag to specify that workspace is locked */ 
  static  BOOLEAN  m_bWorkspaceLocked; 
#endif
};

#endif 
