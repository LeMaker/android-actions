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

    File Name: LockingHelper.cc

    General Description: Implementation of the  locking helper classes

==================================================================================================*/

#include "dm_tree_class.H"
#include "dmtTreeImpl.hpp"
#include "dmLockingHelper.h"

#ifndef DM_NO_LOCKING 
BOOLEAN  DMGlobalOMAWorkspaceSharing::m_bWorkspaceLocked = FALSE;
DMCriticalSection DMGlobalOMAWorkspaceSharing::m_csInitLock;
#endif

DMGlobalOMAWorkspaceSharing::DMGlobalOMAWorkspaceSharing()
{
#ifndef DM_NO_LOCKING  
  while ( true ) {
    {
      DMSingleLock oLock(m_csInitLock);

      if ( !m_bWorkspaceLocked ){
        m_bWorkspaceLocked = TRUE;
        break;
      }
    }
    DmThSleep( 1000 );
  }
#else
  dmTreeObj.GetLockContextManager().Lock();
#endif
}

DMGlobalOMAWorkspaceSharing::~DMGlobalOMAWorkspaceSharing()
{
#ifndef DM_NO_LOCKING  
  DMSingleLock oLock(m_csInitLock);
  m_bWorkspaceLocked = FALSE;
#else
  dmTreeObj.GetLockContextManager().UnLock();
#endif
}


DMGlobalLockHelper::DMGlobalLockHelper() 
{
  dmTreeObj.GetLockContextManager().GlobalLock();
}

DMGlobalLockHelper::~DMGlobalLockHelper() 
{
  dmTreeObj.GetLockContextManager().GlobalUnlock();
}


BOOLEAN DMLockingHelper::Lock( INT32 nCurID, 
                               CPCHAR szURI, 
                               CPCHAR szPrincipal, 
                               SYNCML_DM_TREE_LOCK_TYPE_T eLockType, 
                               BOOLEAN bInTransaction ) 
{
    if ( SYNCML_DM_LOCKID_IGNORE == nCurID )
    {
        m_nLockID = nCurID; m_nError = SYNCML_DM_SUCCESS; m_bIDChanged = FALSE;
        return FALSE;
    }
  
    m_nLockID = nCurID;

    m_nError = dmTreeObj.GetLockContextManager().ValidateAndLockTNM( m_nLockID, 
                                                                                                      szURI,
                                                                                                      szPrincipal, 
                                                                                                      eLockType, 
                                                                                                      bInTransaction );
  
    m_bIDChanged = (nCurID != m_nLockID);
    return m_nError == SYNCML_DM_SUCCESS;
}



DMLockingHelper::DMLockingHelper(DmtTreeImpl* pTree, 
                                                           SYNCML_DM_TREE_LOCK_TYPE_T nLockType )
{
    if ( nLockType == SYNCML_DM_LOCK_TYPE_EXCLUSIVE && 
        pTree->m_nLockType == SYNCML_DM_LOCK_TYPE_SHARED )
    {
        m_nError = SYNCML_DM_TREE_READONLY;
        return;
    }
  
    if ( Lock( pTree->m_nLockID, 
                 pTree->m_strRootPath,
                 pTree->m_strServerID,
                 nLockType, 
                 pTree->m_isAtmoic )  )
        pTree->m_nLockID = GetID(); 
}

DMLockingHelper::~DMLockingHelper() 
{
    if (  SYNCML_DM_LOCKID_IGNORE != m_nLockID && m_nError == SYNCML_DM_SUCCESS )
    {
        dmTreeObj.GetLockContextManager().UnlockTNM();
    }
}
