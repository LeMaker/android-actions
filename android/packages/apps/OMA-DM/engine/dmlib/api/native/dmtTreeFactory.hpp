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

#ifndef __DMTTREEFACTORY_H__
#define __DMTTREEFACTORY_H__

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/** 
  \file dmtTreeFactory.hpp
  \brief The dmtTreeFactory.hpp header file contains DmtTreeFactory class  definition. \n
       This class represents a tree factory. It is used to create a DmtTree from a given Principal. 
*/

#include "jem_defs.hpp"
#include "dmtPrincipal.hpp"
#include "dmtNotification.hpp"
#include "dmtSessionProp.hpp"
#include "dmtEvent.hpp"
#include "dmtEventData.hpp"

/**
* DmtTreeFactory represents a tree factory. 
* It is used to create a DmtTree  from a given Principal.  
* \par Category: General  
* \par Persistence: Transient
* \par Security: Non-Secure
* \par Migration State: FINAL
*/
class DmtTreeFactory
{
  public:
/**
* Enumaration that presents Device management tree lock types.
*/
  enum 
  {
  /** Lock type "Shared"*/
    LOCK_TYPE_SHARED = SYNCML_DM_LOCK_TYPE_SHARED,
    /** Lock type "Exclusive"*/
    LOCK_TYPE_EXCLUSIVE = SYNCML_DM_LOCK_TYPE_EXCLUSIVE,
    /** Lock type "Automatic"*/
    LOCK_TYPE_AUTOMATIC = SYNCML_DM_LOCK_TYPE_AUTOMATIC
  };
    
    
  /** 
  * Initialization - calls this function prior any other function.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return result as  BOOLTYPE
  * - true - indicates success and Dmt functions are ready to be called. \n
  * - false - indicates failure and no more Dmt functions can be called. \n
  *   typical cause of failure is due to invalid Dmt root file path from "dm_setting_root" environment variable.
  * \par Prospective Clients:
  * All potential applications that require configuration settings.
  */
  static BOOLTYPE Initialize();


  /**
  * Clean up resources.
  * \par Important Notes:
  * - Note: no one Dmt function can be called after clean up.
  * - Note: all tree and node handles should be released prior Uninitialized call
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  static SYNCML_DM_RET_STATUS_T Uninitialize();

  /**
  * Gets a logic DmtTree for a given principal. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - DMT principal
  * \param ptrTree [out] - pointer to DMT
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  *   Make sure Initialize() function is called successfully prior to calling this function.
  * \par Example of how this function would be called
  *
  * \code
  *   DmtPrincipal principal("localhost");
  *   PDmtTree ptrTree;
  *   SYNCML_DM_RET_STATUS_T  ret_status;
  *
  *   if ( (ret_status = DmtFactory::GetTree(principal, ptrTree ) ) != SYNCML_DM_SUCCESS )
  *   {
  *     ... error handling
  *     return;
  *   }
  *
  *  \endcode
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  static SYNCML_DM_RET_STATUS_T GetTree(const DmtPrincipal& principal,     
                                       PDmtTree& ptrTree);

  /**
  * Gets a logic part of DmtTree for a given principal; This tree 
  * object gives access only to subtree located by path. 
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - DMT principal
  * \param szSubtreeRoot [in] - sub tree root name
  * \param ptrTree [out] - pointer to sub tree
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  *   Make sure Initialize() function is called successfully prior to calling this function. \n
  *   This function will return error for invalid subtree path.
  *
  * \par Example of how GetSubtree() function would be called
  *
  * \code
  *  DmtPrincipal principal("localhost");
  * PDmtTree ptrTree;
  * SYNCML_DM_RET_STATUS_T  ret_status; 
  * if ( (ret_status =DmtTreeFactory::GetSubtree(principal, "./SyncML/DMAcc”, ptrTree))!=SYNCML_DM_SUCCESS  ){
  *  ... error processing here 
  * return;
  * }
  * \endcode
  *
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
    static SYNCML_DM_RET_STATUS_T GetSubtree(const DmtPrincipal& principal,       
                                           CPCHAR szSubtreeRoot,
                                           PDmtTree& ptrTree);

  /**
  * Gets a logic part of  DmtTree for a given principal 
  * This tree object gives access only to subtree located by path.
  * All path access to subtree should be NOT have "./" or "/" before the path !
  * Additional parameter lockType allows to specify desired access to the tree. 
  * If lockType is "shared" tree is read-only and any "Set" function will throw an exception.
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - DMT principal
  * \param szSubtreeRoot [in] - sub tree root name
  * \param nLockType [in] - tree lock type
  * \param ptrTree [out] - pointer to sub tree
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  *   Make sure Initialize() function is called successfully prior to calling this function. \n
  *   This function will return error for invalid subtree path.
  *
  * \par Example of how GetSubtreeEx() function would be called
  *
  * \code
  *  DmtPrincipal principal("localhost");
  * PDmtTree ptrTree;
  * SYNCML_DM_RET_STATUS_T  ret_status; 
  * if ( (ret_status =DmtTreeFactory::GetSubtree(principal, "./SyncML/DMAcc”, DmtTreeFactory::LOCK_TYPE_EXCLUSIVE, ptrTree))!=SYNCML_DM_SUCCESS  ){
  *  ... error processing here 
  * return;
  * }
  * \endcode
  *
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  static SYNCML_DM_RET_STATUS_T GetSubtreeEx(const DmtPrincipal& principal,     
                                             CPCHAR szSubtreeRoot,
                                             SYNCML_DM_TREE_LOCK_TYPE_T nLockType,  
                                             PDmtTree& ptrTree);

  /**
  * Reads SyncML scripts from the InputStream then process to the tree
  * \warning This functions is  for internal usage only!!!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - DMT principal
  * \param buf [in] - input stream
  * \param len [in] - length of the input stream
  * \param isWBXML [in] - flag to use binary xml
  * \param oResult [out] - string with error description
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h \n
  *
  *  \par Example of how ProcessScript() function would be called
  *
  *  \code
  *  const char* szFile  = <path to file>;
  *  DmtPrincipal principal ("localhost");
  *  DMString oResult;
  *  char* szBuf = <script>
  *  int n = <size of 'script'>   
  * SYNCML_DM_RET_STATUS_T res = DmtTreeFactory::ProcessScript( principal ,  (const byte*)szBuf, n, false, oResult);
  * \endcode
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */

  static SYNCML_DM_RET_STATUS_T ProcessScript(const DmtPrincipal& principal, 
                                                const UINT8 * buf, 
                                                INT32 len, 
                                                BOOLEAN isWBXML, 
                                                DMString& oResult);

 /**
  * Reads SyncML scripts from the InputStream then process to the tree and set the result 
  * in binary format.
  * \warning This functions is  for internal usage only!!!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - DMT principal
  * \param buf [in] - input stream
  * \param len [in] - length of the input stream
  * \param isWBXML [in] - to use binary xml
  * \param oResult [out] - DMVector<UINT8> with error code
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * - All other codes indicates failure. The description can be found in the dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */

  static SYNCML_DM_RET_STATUS_T ProcessScript(const DmtPrincipal& principal, 
                                                const UINT8 * buf, 
                                                INT32 len, 
                                                BOOLEAN isWBXML, 
                                                DMVector<UINT8> & oResult);
   
   
  /**
  * Starts communication with Syncml DM server
  * \warning This functions is  for internal usage only!!!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - Syncml DM server ID
  * \param sessionProp [in] - additional session parameters
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  *
  * \par Example of how StartServerSession()  function would be called
  *
  * \code
  * DmtSessionProp oClInitiated( true );
  * DmtPrincipal principal(“ServerID”);
  * SYNCML_DM_RET_STATUS_T  ret_status;
  *
  * if ( (ret_status=DmtTreeFactory::StartServerSession(principal, oClInitiated )) == SYNCML_DM_SUCCESS )
  *    .....OK
  * else
  *    .....Error
  * );
  * \endcode
  *
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  static SYNCML_DM_RET_STATUS_T StartServerSession(const DmtPrincipal& principal, 
                                                  const DmtSessionProp& sessionProp);

  /**
  * Notification for the DMT
  * \warning This functions is  for internal usage only!!!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - Syncml DM server ID
  * \param buf [in] - input stream
  * \param len [in] - length of the input stream
  * \param notification [out] - string with a notification
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  static SYNCML_DM_RET_STATUS_T ProcessNotification(const DmtPrincipal& principal, 
                                                   const UINT8 *buf, 
                                                   INT32 len,
                                                   DmtNotification & notification);


  /**
   * Notification for the process
  * \warning This functions is  for internal usage only!!!
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param principal [in] - Syncml DM server ID
  * \param buf [in] - input stream
  * \param len [in] - length of the input stream
  * \param isWBXML [in] - to use binary xml
  * \param isProcess [in] - flag to process script
  * \param serverID [out] - reference to string with server ID
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation has completed successfully. \n
  * - All other codes indicates failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  static SYNCML_DM_RET_STATUS_T Bootstrap(const DmtPrincipal& principal,
                                            const UINT8 * buf,
                                            INT32 len,
                                            BOOLEAN isWBXML,
                                            BOOLEAN isProcess,
                                            DMString & serverID);
  

  /**
  * Checks if DMT is locked
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return "true" if locked
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */

  static BOOLEAN IsLocked();


   /**
  * Check if OTA DM session is in progress
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \return "true" if session is started
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */

  static BOOLEAN IsSessionInProgress();

  
   /**
   * Subscribes on DMT update event (tells DM engine to track updates on specific node or sub tree
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szPath [in] - node path
  * \param oEvent [in] - event subscription
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. \n
  * - All other codes indicate failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  static SYNCML_DM_RET_STATUS_T SubscribeEvent(CPCHAR szPath, 
                                               const DmtEventSubscription & oEvent);

   /**
  * Unsubscribes from DMT update event (tells DM engine to stop tracking updates on specific node or sub tree
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szPath [in] - node path
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. \n
  * - All other codes indicate failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  static SYNCML_DM_RET_STATUS_T UnSubscribeEvent(CPCHAR szPath);

   /**
  * Retrieves event subscription for specific path
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param szPath [in] - node path
  * \param oEvent [out] - event subscription
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. \n
  * - All other codes indicate failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */
  static SYNCML_DM_RET_STATUS_T GetEventSubscription(CPCHAR szPath,
                                                     DmtEventSubscription & oEvent);

  
   /**
  * Parses data received by application about DMT update on sub tree
  * \par Sync (or) Async:
  * This is a Synchronous function.
  * \par Secure (or) Non-Secure (or) N/A:
  * This is a Non-Secure function.
  * \param pBuffer [in] - pointer on a data buffer
  * \param size [in] - size of a buffer
  * \param aEventMap [out] - map of events for sub tree
  * \return Return Type (SYNCML_DM_RET_STATUS_T) \n
  * - SYNCML_DM_SUCCESS - indicate that operation is completed successfully. \n
  * - All other codes indicate failure. The description can be found in dmtError.h 
  * \par Prospective Clients:
  * All potential applications that require configuration settings and Internal Classes.
  */

  static SYNCML_DM_RET_STATUS_T ParseUpdateEvent(UINT8 * pBuffer, 
                                                 UINT32 size,
                                                 DmtEventMap & aEventMap);


  
 
};

#endif
