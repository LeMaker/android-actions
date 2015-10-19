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

    Source Name: dmt.cc 

    General Description: Implementation of DmtTreeFactory class.

==================================================================================================*/

#include "dmt.hpp"
#include "dmtTreeImpl.hpp"
#include "dm_tree_class.H" 
#include "dmSessionFactory.h"
#include "xpl_Logger.h"
#include "dmNotification.h"
#include "dmprofile.h"
#include "dmLockingHelper.h"

//------------------------------------------------------------------------
//            Source Name: dmt.cpp
//  General Description: This file contains External API for DMTree C++ API
//------------------------------------------------------------------------

BOOLTYPE DmtTreeFactory::Initialize()
{

  DM_PERFORMANCE(DM_INITIALIZE_ENTER); 

   BOOLTYPE dm_stat = true;
  if ( dmTreeObj.Init() == SYNCML_DM_SUCCESS )
      dm_stat = true;
  else
      dm_stat = false;

  DM_PERFORMANCE(DM_INITIALIZE_EXIT);
  return dm_stat;
}



SYNCML_DM_RET_STATUS_T DmtTreeFactory::Uninitialize()
{
   DM_PERFORMANCE(DM_UNINITIALIZE_ENTER); 
   SYNCML_DM_RET_STATUS_T dm_stat = dmTreeObj.DeInit(FALSE);
   DM_PERFORMANCE(DM_UNINITIALIZE_EXIT);
   return dm_stat;
}

SYNCML_DM_RET_STATUS_T DmtTreeFactory::GetTree(     
      const DmtPrincipal& principal,     
      PDmtTree& ptrTree               
   )
{
  return GetSubtree( principal, NULL, ptrTree );
}


SYNCML_DM_RET_STATUS_T DmtTreeFactory::GetSubtree(
      const DmtPrincipal& principal,     
      CPCHAR szSubtreeRoot,
      PDmtTree& ptrTree               
   )
{
  return GetSubtreeEx( principal, szSubtreeRoot, SYNCML_DM_LOCK_TYPE_AUTOMATIC, ptrTree );
}

SYNCML_DM_RET_STATUS_T DmtTreeFactory::GetSubtreeEx(
      const DmtPrincipal& principal,     
      CPCHAR szSubtreeRoot,
      SYNCML_DM_TREE_LOCK_TYPE_T nLockType,  
      PDmtTree& ptrTree               
   )
{
  DM_PERFORMANCE(DM_GET_TREE_ENTER); 

  ptrTree = NULL;

  if ( !dmTreeObj.IsInitialized() ) {
    return SYNCML_DM_FAIL;
  }

  XPL_LOG_DM_API_Debug(("GetSubtreeEx  path=%s, nLockType=%dn", szSubtreeRoot, nLockType));

#ifdef DM_NO_LOCKING 
  if ( IsLocked() )
    return SYNCML_DM_SESSION_BUSY; 
#endif 
  
  PDmtTree ptrNewTree;

  DmtTreeImpl* pNewTree = new DmtTreeImpl;
  ptrNewTree = pNewTree;

  if ( !pNewTree )
    return SYNCML_DM_DEVICE_FULL;

  SYNCML_DM_RET_STATUS_T ret_status = pNewTree->StartSession( principal, szSubtreeRoot, nLockType );

  if ( ret_status == SYNCML_DM_SUCCESS )
  {
      ptrTree = ptrNewTree;
#ifdef DM_NO_LOCKING      
      // dmTreeObj.GetLockContextManager().Lock();
#endif
  }    

  DM_PERFORMANCE(DM_GET_TREE_EXIT); 

  return ret_status;
}

SYNCML_DM_RET_STATUS_T DmtTreeFactory::ProcessScript(const DmtPrincipal& principal,
                                                    const UINT8 * buf,
                                                    INT32 len,
                                                    BOOLEAN isWBXML,
                                                    DMString& oResult)
{

  if ( !dmTreeObj.IsInitialized() ) 
    return SYNCML_DM_FAIL;

  if ( !buf || len <= 0 )
    return SYNCML_DM_INVALID_PARAMETER;

#ifdef DM_NO_LOCKING 
  if ( IsLocked() )
     return SYNCML_DM_SESSION_BUSY;
#endif  


  DMGlobalOMAWorkspaceSharing  oOMAWorkspaceLock;  // global lock for OMA workspace 


  SYNCML_DM_RET_STATUS_T ret_status;

  XPL_FS_HANDLE_T hLockFile;
  XPL_FS_RET_STATUS_T xpl_status;

  hLockFile = XPL_FS_Open(DM_ISP_LOCK, XPL_FS_FILE_WRITE, &xpl_status);
  
  INT32 nLock = 0;

  {
    DMLockingHelper oLock( 0, ".", principal.getName().c_str(), SYNCML_DM_LOCK_TYPE_EXCLUSIVE, FALSE );
    nLock = oLock.GetID();

      if ( !oLock.IsLockedSuccessfully() ){
        return oLock.GetError();
      }


    DMBuffer oResultDoc;
   
    ret_status = DmProcessScriptData( buf ,(UINT32)len, isWBXML, oResultDoc);

    if( ret_status == SYNCML_DM_SUCCESS ) 
       oResultDoc.copyTo(oResult);
  }

  dmTreeObj.ReleaseLock( nLock );

  XPL_FS_Remove(DM_ISP_LOCK);

  return ret_status;
}


SYNCML_DM_RET_STATUS_T DmtTreeFactory::ProcessScript(const DmtPrincipal& principal,
                                                    const UINT8 * buf,
                                                    INT32 len,
                                                    BOOLEAN isWBXML,
                                                    DMVector<UINT8> & oResult)
{

  if ( !dmTreeObj.IsInitialized() ) 
    return SYNCML_DM_FAIL;

  if ( !buf || len <= 0 )
    return SYNCML_DM_INVALID_PARAMETER;

#ifdef DM_NO_LOCKING 
  if ( IsLocked() )
     return SYNCML_DM_SESSION_BUSY;
#endif  


  DMGlobalOMAWorkspaceSharing  oOMAWorkspaceLock;  // global lock for OMA workspace 

  XPL_FS_HANDLE_T hLockFile;
  XPL_FS_RET_STATUS_T xpl_status;
  hLockFile = XPL_FS_Open(DM_ISP_LOCK, XPL_FS_FILE_WRITE, &xpl_status);
  
  
  INT32 nLock = 0;
  SYNCML_DM_RET_STATUS_T ret_status;
  {
    DMLockingHelper oLock( 0, ".", principal.getName().c_str(), SYNCML_DM_LOCK_TYPE_EXCLUSIVE, FALSE );
    nLock = oLock.GetID();

      if ( !oLock.IsLockedSuccessfully() ){
        return oLock.GetError();
      }


    DMBuffer oResultDoc;
   
    ret_status = DmProcessScriptData( buf ,(UINT32)len, isWBXML, oResultDoc);

    XPL_LOG_DM_SESS_Debug(("DmtTreeFactory::ProcessScript before release lock, ret_status=%d, result-size=%d\n", ret_status, oResultDoc.getSize()));
    
    if( ret_status == SYNCML_DM_SUCCESS ) {

      oResult.set_size(oResultDoc.getSize());

      memcpy(oResult.get_data(), oResultDoc.getBuffer(), oResultDoc.getSize());
    }
    
    if (oResultDoc.getSize() > 0 ) {
         char *szResult = new char[oResultDoc.getSize()+1];
         memcpy(szResult, oResultDoc.getBuffer(), oResultDoc.getSize());
         szResult[oResultDoc.getSize()] = 0;
         XPL_LOG_DM_SESS_Debug(("DmtTreeFactory::ProcessScript szResult=%s\n", szResult));
         delete [] szResult;
    }

  }

    

             
  dmTreeObj.ReleaseLock( nLock );

  XPL_FS_Remove(DM_ISP_LOCK);

  return ret_status;
}


SYNCML_DM_RET_STATUS_T DmtTreeFactory::Bootstrap(const DmtPrincipal& principal,
                                                   const UINT8 * buf,
                                                   INT32 len,
                                                   BOOLEAN isWBXML,
                                                   BOOLEAN isProcess,
                                                   DMString & serverID)
{

    if ( !dmTreeObj.IsInitialized() ) 
        return SYNCML_DM_FAIL;
    
    if ( !buf || len <= 0 )
        return SYNCML_DM_INVALID_PARAMETER;

#ifdef DM_NO_LOCKING 
    if ( IsLocked() )
        return SYNCML_DM_SESSION_BUSY;
#endif  


    DMGlobalOMAWorkspaceSharing  oOMAWorkspaceLock;  // global lock for OMA workspace 

    
    INT32 nLock = 0;
    SYNCML_DM_RET_STATUS_T ret_status;
    {
        DMLockingHelper oLock( 0, ".", principal.getName().c_str(), SYNCML_DM_LOCK_TYPE_EXCLUSIVE, FALSE );
        nLock = oLock.GetID();

        if ( !oLock.IsLockedSuccessfully() )
            return oLock.GetError();

        ret_status = DmBootstrap( buf ,(UINT32)len, isWBXML, isProcess, serverID);
  }

  dmTreeObj.ReleaseLock( nLock );

  return ret_status;
}



/**
* Starts server session based on principal information
*/
SYNCML_DM_RET_STATUS_T DmtTreeFactory::StartServerSession( const DmtPrincipal& principal, 
                                                          const DmtSessionProp& sessionProp)
{

  if ( !dmTreeObj.IsInitialized() )
    return SYNCML_DM_FAIL;

#ifdef DM_NO_LOCKING 
  if ( IsLocked() ) {
    XPL_LOG_DM_SESS_Error(("StartServerSession locked\n"));
    //return SYNCML_DM_SESSION_BUSY; 
  }
#endif

 
  DMGlobalOMAWorkspaceSharing  oOMAWorkspaceLock;  // global lock for OMA workspace

  XPL_LOG_DM_SESS_Debug(("Opening session lock file\n"));

  XPL_FS_HANDLE_T hLockFile;
  XPL_FS_RET_STATUS_T xpl_status;
  hLockFile = XPL_FS_Open(DM_ISP_LOCK, XPL_FS_FILE_WRITE, &xpl_status);
  
  SYNCML_DM_RET_STATUS_T ret_status;
  INT32 nLock = 0;
  {
      DMLockingHelper oLock( 0, ".", principal.getName().c_str(), SYNCML_DM_LOCK_TYPE_EXCLUSIVE, FALSE );
      nLock = oLock.GetID();

      if ( !oLock.IsLockedSuccessfully() ) {
    XPL_LOG_DM_SESS_Error(("Opening session lock file failed reason=%d\n", oLock.GetError()));
        return oLock.GetError();
      }
  }


#ifndef DM_NO_LOCKING 
  dmTreeObj.ReleaseLock( nLock );
#endif

  ret_status = DmProcessServerData(principal.getName().c_str(), sessionProp);
#ifdef DM_NO_LOCKING     
  dmTreeObj.ReleaseLock( nLock );
#endif

  XPL_FS_Remove(DM_ISP_LOCK);

  DM_MEMORY_STATISTICS_WRITE("ServerSession done\n");
  XPL_LOG_DM_SESS_Debug(("Returning from StartServerSession status=%d\n", ret_status));
  
  return ret_status;
}


SYNCML_DM_RET_STATUS_T DmtTreeFactory::ProcessNotification(
  const DmtPrincipal& principal, 
  const UINT8 *buf, 
  INT32 len, 
  DmtNotification & notification)
{

  if ( !dmTreeObj.IsInitialized() )
    return SYNCML_DM_FAIL;

#ifdef DM_NO_LOCKING 
  if ( IsLocked() )
    return SYNCML_DM_SESSION_BUSY; 
#endif

 
  SYNCML_DM_RET_STATUS_T ret_status;
  INT32 nLock = 0;
    {
      DMLockingHelper oLock( 0, ".", principal.getName().c_str(), SYNCML_DM_LOCK_TYPE_EXCLUSIVE, FALSE );
      nLock = oLock.GetID();

      if ( !oLock.IsLockedSuccessfully() )
        return oLock.GetError();

    }

  ret_status = DmProcessNotification((const UINT8*)buf,len,notification);

  dmTreeObj.ReleaseLock( nLock );

  return ret_status;
}


BOOLEAN DmtTreeFactory::IsLocked()
{
#ifdef DM_NO_LOCKING
    BOOLEAN result = dmTreeObj.GetLockContextManager().IsLocked();
    XPL_LOG_DM_SESS_Debug(("isLocked()  returning=%d\n", result));
    return result;
#else
    XPL_LOG_DM_SESS_Debug(("isLocked()  returning false\n"));
    return FALSE;
#endif
}


BOOLEAN DmtTreeFactory::IsSessionInProgress()
{
    return XPL_FS_Exist(DM_ISP_LOCK);
}


SYNCML_DM_RET_STATUS_T  
DmtTreeFactory::SubscribeEvent(CPCHAR szPath, 
                               const DmtEventSubscription & oEvent)
{

     if ( !dmTreeObj.IsInitialized() ) 
        return SYNCML_DM_FAIL;

     if ( !szPath )
        return SYNCML_DM_INVALID_PARAMETER;


     INT32 nLockID = 0;
     SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
     {
        DMLockingHelper oLock( 0, ".", "localhost", SYNCML_DM_LOCK_TYPE_EXCLUSIVE, FALSE );
        nLockID = oLock.GetID();

        if ( !oLock.IsLockedSuccessfully() )
            return SYNCML_DM_FAIL;
      
        DMSubscriptionManager & oEventManager =  dmTreeObj.GetSubscriptionManager();
        
        dm_stat = oEventManager.EnableEvent(szPath, oEvent); 
    }    

     dmTreeObj.ReleaseLock( nLockID );
 
     return dm_stat;

}


SYNCML_DM_RET_STATUS_T 
DmtTreeFactory::UnSubscribeEvent(CPCHAR szPath)
{
     if ( !dmTreeObj.IsInitialized() ) 
        return SYNCML_DM_FAIL;

     if ( !szPath )
        return SYNCML_DM_INVALID_PARAMETER;
     
     INT32 nLockID = 0;
     SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
     {
        DMLockingHelper oLock( 0, ".", "localhost", SYNCML_DM_LOCK_TYPE_EXCLUSIVE, FALSE );
        nLockID = oLock.GetID();

        if ( !oLock.IsLockedSuccessfully() )
            return SYNCML_DM_FAIL;
      
        DMSubscriptionManager & oEventManager =  dmTreeObj.GetSubscriptionManager();
        
        dm_stat = oEventManager.Delete(szPath); 
    }    

    dmTreeObj.ReleaseLock( nLockID );
   
    return dm_stat;


}
  
SYNCML_DM_RET_STATUS_T 
DmtTreeFactory::GetEventSubscription(CPCHAR szPath, 
                                    DmtEventSubscription & oEvent)
{
     if ( !dmTreeObj.IsInitialized() ) 
        return SYNCML_DM_FAIL;

     if ( !szPath )
        return SYNCML_DM_INVALID_PARAMETER;
     
     INT32 nLockID = 0;
     SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
     {
        DMLockingHelper oLock( 0, ".", "localhost", SYNCML_DM_LOCK_TYPE_EXCLUSIVE, FALSE );
        nLockID = oLock.GetID();

        if ( !oLock.IsLockedSuccessfully() )
            return SYNCML_DM_FAIL;
      
        DMSubscriptionManager & oEventManager =  dmTreeObj.GetSubscriptionManager();
        
        dm_stat = oEventManager.GetEvent(szPath, oEvent);

    }
    dmTreeObj.ReleaseLock( nLockID );
 
    return dm_stat;

}


SYNCML_DM_RET_STATUS_T 
DmtTreeFactory::ParseUpdateEvent(UINT8 * pBuffer, 
                                UINT32 size,  
                                DmtEventMap & aEventMap)
{
    if ( pBuffer == NULL || size == 0 )
        return SYNCML_DM_INVALID_PARAMETER;

    SYNCML_DM_RET_STATUS_T dm_stat  = SYNCML_DM_SUCCESS;

    DMBufferReader oBuffer(pBuffer,size);
    dm_stat = DMEventLogger::Deserialize(oBuffer,aEventMap); 

    return dm_stat;

}




void*  DmtMemAllocEx( size_t nSize, CPCHAR szFile, INT32 nLine )
{
  return DmAllocMemEx( nSize, szFile,  nLine );
}

void   DmtMemFree( void* p )
{
  DmFreeMem( p );
}
