/*************************************************************************/
/* module:          Managing SyncML Instances                            */
/*                                                                       */   
/* file:            mgrinstancemgr.c                                     */
/* target system:   all                                                  */
/* target OS:       all                                                  */   
/*                                                                       */   
/* Description:                                                          */   
/* Core module for managing creation and usage of instances              */
/*************************************************************************/

/*
 * Copyright Notice
 * Copyright (c) Ericsson, IBM, Lotus, Matsushita Communication 
 * Industrial Co., Ltd., Motorola, Nokia, Openwave Systems, Inc., 
 * Palm, Inc., Psion, Starfish Software, Symbian, Ltd. (2001).
 * All Rights Reserved.
 * Implementation of all or part of any Specification may require 
 * licenses under third party intellectual property rights, 
 * including without limitation, patent rights (such a third party 
 * may or may not be a Supporter). The Sponsors of the Specification 
 * are not responsible and shall not be held responsible in any 
 * manner for identifying or failing to identify any or all such 
 * third party intellectual property rights.
 * 
 * THIS DOCUMENT AND THE INFORMATION CONTAINED HEREIN ARE PROVIDED 
 * ON AN "AS IS" BASIS WITHOUT WARRANTY OF ANY KIND AND ERICSSON, IBM, 
 * LOTUS, MATSUSHITA COMMUNICATION INDUSTRIAL CO. LTD, MOTOROLA, 
 * NOKIA, PALM INC., PSION, STARFISH SOFTWARE AND ALL OTHER SYNCML 
 * SPONSORS DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING 
 * BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION 
 * HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF 
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
 * SHALL ERICSSON, IBM, LOTUS, MATSUSHITA COMMUNICATION INDUSTRIAL CO., 
 * LTD, MOTOROLA, NOKIA, PALM INC., PSION, STARFISH SOFTWARE OR ANY 
 * OTHER SYNCML SPONSOR BE LIABLE TO ANY PARTY FOR ANY LOSS OF 
 * PROFITS, LOSS OF BUSINESS, LOSS OF USE OF DATA, INTERRUPTION OF 
 * BUSINESS, OR FOR DIRECT, INDIRECT, SPECIAL OR EXEMPLARY, INCIDENTAL, 
 * PUNITIVE OR CONSEQUENTIAL DAMAGES OF ANY KIND IN CONNECTION WITH 
 * THIS DOCUMENT OR THE INFORMATION CONTAINED HEREIN, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH LOSS OR DAMAGE.
 * 
 * The above notice and this paragraph must be included on all copies 
 * of this document that are made.
 * 
 */


/*************************************************************************
 *  Definitions
 *************************************************************************/


/* Include Headers */
#include <smldef.h>
#include <sml.h>
#include <smlerr.h>
#include "libmem.h"
#include "libstr.h"
#include "liblock.h"
#include "wsm.h"
#include "mgr.h"



/* Used external functions */
#ifndef NOWSM
  #ifndef __SML_LITE__  /* Only ONE instance is supported in the Toolkit lite version */
    extern Ret_t addInfo(InstanceInfoPtr_t pInfo);
    extern InstanceInfoPtr_t findInfo(InstanceID_t id);
    extern Ret_t removeInfo(InstanceID_t id);
  #endif
  SyncMLInfoPtr_t mgrGetSyncMLAnchor(void);
#endif

/* Prototypes of exported SyncML API functions */
SML_API Ret_t smlInitInstance(SmlCallbacksPtr_t callbacks, SmlInstanceOptionsPtr_t pOptions, VoidPtr_t pUserData, InstanceID_t *pInstanceID);
SML_API Ret_t smlTerminateInstance (InstanceID_t id);
SML_API Ret_t smlLockReadBuffer(InstanceID_t id, MemPtr_t *pReadPosition, MemSize_t *usedSize);
SML_API Ret_t smlUnlockReadBuffer(InstanceID_t id, MemSize_t processedBytes);
#ifdef NOWSM
SML_API Ret_t smlSetMaxOutgoingSize(InstanceID_t id, MemSize_t maxOutgoingSize);
SML_API Ret_t smlSetOutgoingBegin(InstanceID_t id);
#endif
SML_API Ret_t smlLockWriteBuffer(InstanceID_t id, MemPtr_t *pWritePosition, MemSize_t *freeSize);
SML_API Ret_t smlUnlockWriteBuffer(InstanceID_t id, MemSize_t writtenBytes);
SML_API Ret_t smlSetCallbacks (InstanceID_t id, SmlCallbacksPtr_t pCallbacks);
SML_API Ret_t smlSetUserData (InstanceID_t id, VoidPtr_t pUserData);
// added by luz %%%:
SML_API Ret_t smlGetUserData(InstanceID_t id, VoidPtr_t *ppUserData);
SML_API Ret_t smlGetEncoding(InstanceID_t id, SmlEncoding_t *pEncoding);
#ifndef __SML_LITE__  /* these API calls are NOT included in the Toolkit lite version */
  SML_API Ret_t smlSetEncoding (InstanceID_t id, SmlEncoding_t encoding);
#endif



/* Private function prototypes */
Ret_t freeInstanceOptions (InstanceInfoPtr_t pInstanceInfo);
static Ret_t freeInstanceInfo (InstanceInfoPtr_t pInfo);
Ret_t mgrResetWorkspace (InstanceID_t id);
Ret_t setInstanceOptions (InstanceID_t id, SmlInstanceOptionsPtr_t pOptions);


/*************************************************************************
 *  Public SyncML API Functions
 *************************************************************************/


/**
 * FUNCTION:   smlInitInstance
 *
 * Creates a SyncML instance and assigns a corresponding workspace buffer in
 * which XML documents are assembled or parsed.
 * All callback functions implemented by a particular application are defined.
 * Instance specific options can be passed. This function has to be called 
 * before the first synchronization tasks can be performed. A reference valid
 * for a SyncML instance is returned.
 * An instance is active when processing a synchronization request
 * otherwise it is idle. An instance is terminated when smlTerminateInstance 
 * is called.
 *
 * IN:              SmlCallbacks_t
 *                  A structure holding references to the callback functions
 *                  implemented by the application
 *
 * IN:              SmlInstanceOptionsPtr_t
 *                  Option settings of a particular SyncML instance
 *
 * IN:              VoidPtr_t
 *                  UserData is a pointer to a void structure the application 
 *                  can pass into the SyncML Toolkit instance info. 
 *                  It will be returned to the application with every called 
 *                  callback function call!
 *                  NOTE: This is only a pointer, the memory object itself 
 *                  remains within the responsibility of the calling application.
 *                  The memory object will not be copied, moved or freed by the 
 *                  Toolkit.
 *
 * OUT:             InstanceID_t
 *                  Instance ID assigned to the initialized instance
 *
 * RETURN:          Ret_t
 *                  Error Code
 */
SML_API Ret_t smlInitInstance(SmlCallbacksPtr_t pCallbacks, SmlInstanceOptionsPtr_t pOptions, VoidPtr_t pUserData, InstanceID_t *pInstanceID)
{

  /* --- Definitions --- */
  InstanceInfoPtr_t pInstanceInfo;
  Ret_t             rc;


  #ifndef NOWSM
    /* --- Check pOptions, which have been passed by the application --- */
    if (!pOptions || !pOptions->workspaceName)
  	  return SML_ERR_WRONG_USAGE;

    #ifdef __SML_LITE__  /* Only ONE instance is supported in the Toolkit lite version */
      /* if ONE instance is already initialized */
      if (mgrGetInstanceListAnchor()!=NULL) 
        return SML_ERR_WRONG_USAGE;
    #endif 

    /* --- check wether we already know about this instance --- */
    #ifdef __SML_LITE__  /* Only ONE instance is supported in the Toolkit lite version */
      pInstanceInfo = mgrGetInstanceListAnchor();
    #else
      pInstanceInfo = (InstanceInfoPtr_t) findInfo(*pInstanceID);
    #endif

    /* --- bail outh when we already have a instance with that id --- */
    if (pInstanceInfo != NULL) return SML_ERR_WRONG_USAGE;


    /* --- Create a workspace for this instance --- */
    LOCKTOOLKIT("smlInitInstance"); 
    if ((rc = wsmCreate(pOptions->workspaceName, pOptions->workspaceSize, pInstanceID)) != SML_ERR_OK) {
    	RELEASETOOLKIT("smlInitInstance after wsmCreate failure");
    	return rc;
    }
    RELEASETOOLKIT("smlInitInstance");
  #else // NOWSM
    /* --- Check pOptions, which have been passed by the application --- */
    if (!pOptions || !pOptions->workspaceSize)
  	  return SML_ERR_WRONG_USAGE;
  	// ok so far
  	rc=SML_ERR_OK;
  #endif
  
  /* --- Create an instance info memory object --- */
  pInstanceInfo = (InstanceInfoPtr_t)smlLibMalloc((MemSize_t)sizeof(InstanceInfo_t));
  if (pInstanceInfo==NULL) {
    #ifndef NOWSM
    wsmDestroy(pOptions->workspaceName);   
    return SML_ERR_NOT_ENOUGH_SPACE;
    #endif
  }
  #ifdef NOWSM
  else {
    // instance info created, return pointer as instanceID
    *pInstanceID = (InstanceID_t)pInstanceInfo;
  }
  #endif
   
  smlLibMemset(pInstanceInfo,0,(MemSize_t)sizeof(InstanceInfo_t));



  /* --- Set mandatory instance infos for this instance to defaults --- */
  pInstanceInfo->status=MGR_IDLE;
  pInstanceInfo->encoderState=NULL;                  // no encoding in progress, currently not used
  pInstanceInfo->decoderState=NULL;                  // no decoding in progress, currently not used
  #ifndef NOWSM
  pInstanceInfo->id=*pInstanceID;
  pInstanceInfo->workspaceState=NULL;                // to do: some workspace status info
  pInstanceInfo->nextInfo=NULL;
  #else
  // create a instance buffer
  pInstanceInfo->instanceBufSiz=pOptions->workspaceSize; // get requested size for the buffer
  pInstanceInfo->maxOutgoingSize=pOptions->maxOutgoingSize; // set max outgoing message size
  pInstanceInfo->instanceBuffer=smlLibMalloc(pInstanceInfo->instanceBufSiz);
  if (pInstanceInfo->instanceBuffer==NULL)
    return SML_ERR_NOT_ENOUGH_SPACE;
  // init buffer pointers
  pInstanceInfo->readPointer=pInstanceInfo->instanceBuffer;
  pInstanceInfo->writePointer=pInstanceInfo->instanceBuffer;
  pInstanceInfo->readLocked=0;
  pInstanceInfo->writeLocked=0;
  pInstanceInfo->outgoingMsgStart=NULL;
  #endif


  #ifndef NOWSM
    /* --- Add instance infos memory object to the instance info list --- */
    #ifdef __SML_LITE__  /* Only ONE instance is supported in the Toolkit lite version */
      mgrSetInstanceListAnchor(pInstanceInfo);
    #else
      rc = addInfo( pInstanceInfo );
      if (rc!=SML_ERR_OK) return rc;
    #endif 
  #endif


  /* --- Set the values of instance Infos as defined by the calling application ---*/

  /* Set user data pointer */
  pInstanceInfo->userData=pUserData;
  /* Set callback functions implemented by applications */
  if (smlSetCallbacks(*pInstanceID, pCallbacks) != SML_ERR_OK) {
    #ifndef NOWSM
	  wsmDestroy(pOptions->workspaceName);
	  #endif
	  return rc;
  }

  // luz: %%% this was called twice, probably this is a bug, so I disabled the second call
  //smlSetCallbacks(*pInstanceID, pCallbacks);

  /* Set other application defined options for that instance */
  if (setInstanceOptions (*pInstanceID, pOptions) != SML_ERR_OK) {
    #ifndef NOWSM
 	  wsmDestroy(pOptions->workspaceName);
 	  #endif
 	  return rc;
  }

  return SML_ERR_OK;
  
}



/**
 * FUNCTION:   smlTerminateInstance
 *
 * Terminates a SyncML instance. The instance info is removed from the instances
 * list. Allmemory allocated for the workspace and the options variables is freed.
 *
 * IN:              InstanceID_t
 *                  ID of the instance to be terminated
 *
 * RETURN:          Ret_t
 *                  Error Code
 */
SML_API Ret_t smlTerminateInstance (InstanceID_t id)
{

  /* --- Definitions --- */
  InstanceInfoPtr_t pInstanceInfo;

  #ifdef NOWSM
    pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
  #else  
    Ret_t             rc;
    
    /* --- Find that instance --- */
    #ifdef __SML_LITE__  /* Only ONE instance is supported in the Toolkit lite version */
      pInstanceInfo = mgrGetInstanceListAnchor();
    #else
      pInstanceInfo = (InstanceInfoPtr_t) findInfo(id);
    #endif
  #endif
  
  if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;

  #ifndef NOWSM
    /* --- Close the workspace --- */
    if (pInstanceInfo->instanceOptions != NULL) {
  		LOCKTOOLKIT("smlTerminateInstance");
    	rc = wsmDestroy(pInstanceInfo->instanceOptions->workspaceName);
    	RELEASETOOLKIT("smlTerminateInstance");
    	if (rc!=SML_ERR_OK) {
      	//	  freeInstanceInfo(pInstanceInfo);  
    		return rc;
    	}
    }	
      
    /* --- Delete instance info and options --- */
    #ifdef __SML_LITE__  /* Only ONE instance is supported in the Toolkit lite version */
      mgrSetInstanceListAnchor(NULL);
    #else
      removeInfo(id);
    #endif
  #endif
  
  freeInstanceInfo (pInstanceInfo);
  
  return SML_ERR_OK;
}



/**
 * FUNCTION:  smlSetCallbacks
 *
 * Sets new callback functions to an instance
 *
 * IN:        InstanceID_t              
 *            ID of the Instance
 *
 * IN:        SmlCallbacksPtr_t
 *            A structure holding references to the callback functions
 *            implemented by the application
 *
 * RETURN:    Return value,            
 *            SML_ERR_OK if successful
 */
SML_API Ret_t smlSetCallbacks(InstanceID_t id, SmlCallbacksPtr_t pCallbacks)
{

  /* --- Definitions --- */
  InstanceInfoPtr_t	pInstanceInfo;
  SmlCallbacksPtr_t    pCallbacksCopy;

  /* --- Check pCallbacks, which have been passed by the application --- */
  if (!pCallbacks)
    return SML_ERR_WRONG_USAGE;


  #ifdef NOWSM
    pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
  #else  
    /* --- Find that instance --- */
    #ifdef __SML_LITE__  /* Only ONE instance is supported in the Toolkit lite version */
      pInstanceInfo = mgrGetInstanceListAnchor();
    #else
      pInstanceInfo = (InstanceInfoPtr_t) findInfo(id);
    #endif

    if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;
  #endif

  /* --- free old callback structure ---*/
  smlLibFree(pInstanceInfo->callbacks);


  /* --- Use a copy of pCallbacksCopy --- */
  pCallbacksCopy = (SmlCallbacksPtr_t)smlLibMalloc((MemSize_t)sizeof(SmlCallbacks_t));
  if (pCallbacksCopy==NULL) return SML_ERR_NOT_ENOUGH_SPACE;
  smlLibMemcpy(pCallbacksCopy,pCallbacks,(MemSize_t)sizeof(SmlCallbacks_t));

  
	/* --- set new Callbacks --- */
	pInstanceInfo->callbacks = pCallbacksCopy;
  
	return SML_ERR_OK;
}



/**
 * FUNCTION:  smlSetUserData
 *
 * Sets a new Pointer to application specific user data, 
 * which is passed to all invoked callback functions
 *
 * IN:        InstanceID_t              
 *            ID of the Instance
 *
 * IN:        VoidPtr_t
 *            UserData is a pointer to a void structure the application 
 *            can pass into the SyncML Toolkit instance info. 
 *            It will be returned to the application with every called 
 *            callback function call!
 *            NOTE: This is only a pointer, the memory object itself 
 *            remains within the responsibility of the calling application.
 *            The memory object will not be copied, moved or freed by the 
 *            Toolkit.
 *
 * RETURN:    Return value,            
 *            SML_ERR_OK if successful
 */
SML_API Ret_t smlSetUserData(InstanceID_t id, VoidPtr_t pUserData)
{

  /* --- Definitions --- */
  InstanceInfoPtr_t	pInstanceInfo;


  #ifdef NOWSM
    pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
  #else  
    /* --- Find that instance --- */
    #ifdef __SML_LITE__  /* Only ONE instance is supported in the Toolkit lite version */
      pInstanceInfo = mgrGetInstanceListAnchor();
    #else
      pInstanceInfo = (InstanceInfoPtr_t) findInfo(id);
    #endif
  #endif

  if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;


  /* --- set new user data pointer ---*/
  pInstanceInfo->userData=pUserData;

	return SML_ERR_OK;
}


/**
 * FUNCTION:  smlGetUserData  (added by luz %%%)
 *
 * Returns Pointer to application specific user data, 
 * which is passed to all invoked callback functions
 *
 * IN:        InstanceID_t              
 *            ID of the Instance
 *
 * IN:        *VoidPtr_t
 *            Receives current Userdata pointer
 *
 * RETURN:    Return value,            
 *            SML_ERR_OK if successful
 */
SML_API Ret_t smlGetUserData(InstanceID_t id, VoidPtr_t *ppUserData)
{

  /* --- Definitions --- */
  InstanceInfoPtr_t	pInstanceInfo;

  #ifdef NOWSM
    pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
  #else  
    /* --- Find that instance --- */
    #ifdef __SML_LITE__  /* Only ONE instance is supported in the Toolkit lite version */
      pInstanceInfo = mgrGetInstanceListAnchor();
    #else
      pInstanceInfo = (InstanceInfoPtr_t) findInfo(id);
    #endif
  #endif

  if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;


  /* --- get userdata pointer ---*/
  *ppUserData = pInstanceInfo->userData;

	return SML_ERR_OK;
} // smlGetUserData


/**
 * FUNCTION:  smlGetEncoding (added by luz %%%)
 *
 * Returns Currently set encoding type
 *
 * IN:        InstanceID_t              
 *            ID of the Instance
 *
 * IN:        *SmlEncoding_t
 *            Receives current encoding
 *
 * RETURN:    Return value,            
 *            SML_ERR_OK if successful
 */
SML_API Ret_t smlGetEncoding(InstanceID_t id, SmlEncoding_t *pEncoding)
{

  /* --- Definitions --- */
  InstanceInfoPtr_t	pInstanceInfo;

  #ifdef NOWSM
    pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
  #else  
    /* --- Find that instance --- */
    #ifdef __SML_LITE__  /* Only ONE instance is supported in the Toolkit lite version */
      pInstanceInfo = mgrGetInstanceListAnchor();
    #else
      pInstanceInfo = (InstanceInfoPtr_t) findInfo(id);
    #endif
  #endif

  if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;

  /* --- get encoding ---*/
  *pEncoding = pInstanceInfo->instanceOptions->encoding;

	return SML_ERR_OK;
} // smlGetEncoding


/**
 * FUNCTION:  smlSetEncoding
 *
 * Sets new encoding type for this Instance
 *
 * IN:        InstanceID_t              
 *            ID of the Instance
 *
 * IN:        SmlEncoding_t
 *            Type of Encoding to be used within this Instance
 *
 * RETURN:    Return value,            
 *            SML_ERR_OK if successful
 */
#ifndef __SML_LITE__  /* these API calls are NOT included in the Toolkit lite version */
SML_API Ret_t smlSetEncoding(InstanceID_t id, SmlEncoding_t encoding)
{

  /* --- Definitions --- */
  InstanceInfoPtr_t	pInstanceInfo;

  /* --- Check pCallbacks, which have been passed by the application --- */
  if (encoding==SML_UNDEF)
    return SML_ERR_WRONG_USAGE;


  #ifdef NOWSM
    pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
  #else  
    /* --- Find that instance --- */
    #ifdef __SML_LITE__  /* Only ONE instance is supported in the Toolkit lite version */
      pInstanceInfo = mgrGetInstanceListAnchor();
    #else
      pInstanceInfo = (InstanceInfoPtr_t) findInfo(id);
    #endif
  #endif

  if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;


  /* --- free old callback structure ---*/
  pInstanceInfo->instanceOptions->encoding = encoding;

	return SML_ERR_OK;
}
#endif




/**
 * FUNCTION:  smlLockReadBuffer
 *
 * Locks the workspace buffer, which is assigned to the given instance
 * for reading. After this function is called, the application has 
 * access to the workspace buffer, beginning at the address pReadPosition which 
 * is returned by this function. SyncML will not change the workspace 
 * buffer until smlUnlockReadBuffer is called.
 * pReadPosition returns a pointer to a valid position in the SyncML workspace 
 * buffer. The pointer can be used by the application for copying outgoing 
 * synchronization data from the buffer into some transport layer. usedSize 
 * retrieves the size of synchronization data currently stored in the 
 * workspace buffer beginning from the address to which pReadPosition points to. 
 * This information is needed by the application when copying XML code out 
 * of the buffer (while sending synchronization data)
 *
 * IN:        InstanceID_t              
 *            ID of the Instance
 *
 * OUT:       MemPtr_t              
 *            Workspace Pointer from which data can be read
 *
 * OUT:       MemSize_t              
 *            Size of used data in workspace which may be read
 *
 * RETURN:    Return value,            
 *            SML_ERR_OK if successful
 */
SML_API Ret_t smlLockReadBuffer(InstanceID_t id, MemPtr_t *pReadPosition, MemSize_t *usedSize)
{  
  #ifdef NOWSM
    InstanceInfoPtr_t	pInstanceInfo;
    
    pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
    if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;
    // must not be already locked here
    if (pInstanceInfo->readLocked)
      return SML_ERR_WRONG_USAGE;
    // everything that is already written can also be read
    *pReadPosition = pInstanceInfo->readPointer;
    // used portion is what is between read and write pointers
    *usedSize = pInstanceInfo->writePointer-pInstanceInfo->readPointer;
    // lock
    pInstanceInfo->readLocked=1;
  #else
    Ret_t rc;
    
    LOCKTOOLKIT("smlLockReadBuffer");
    /* --- Lock Workspace exclusively for reading and get a "Read" pointer --- */
    rc = wsmLockH(id, SML_FIRST_DATA_ITEM, pReadPosition);
    RELEASETOOLKIT("smlLockReadBuffer");
    if (rc!=SML_ERR_OK) return rc;
    
    /* --- Check, how much data has to be read ---*/
    LOCKTOOLKIT("smlLockReadBuffer");
    rc = wsmGetUsedSize(id,usedSize);  
    RELEASETOOLKIT("smlLockReadBuffer");
    if (rc!=SML_ERR_OK) return rc;
  #endif
  
  return SML_ERR_OK;
}




/**
 * FUNCTION:  smlUnlockReadBuffer
 *
 * End the read access of the application to the workspace buffer. 
 * SyncML is now owner of the buffer again and is able to manipulate its contents. 
 * processedBytes passes the number of bytes, which the application has 
 * successfully read and processed (e.g. when the application has copied 
 * outgoing synchronization data from the workspace into a communication module). 
 * SyncML removes the given number of bytes from the workspace!
 *
 * IN:        InstanceID_t              
 *            ID of the Instance
 *
 * IN:        MemSize_t              
 *            Actually read and processed bytes
 *
 * RETURN:    Return value,            
 *            SML_ERR_OK if successful
 */
SML_API Ret_t smlUnlockReadBuffer(InstanceID_t id, MemSize_t processedBytes)
{
  #ifdef NOWSM
    InstanceInfoPtr_t	pInstanceInfo;
    
    pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
    if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;
    // must be already locked here
    if (!pInstanceInfo->readLocked)
      return SML_ERR_WRONG_USAGE;
    // advance read pointer by number of bytes processed
    if (pInstanceInfo->readPointer+processedBytes>pInstanceInfo->writePointer)
      return SML_ERR_WRONG_USAGE; // too many bytes processed
    // update read pointer
    pInstanceInfo->readPointer+=processedBytes;
    // auto-reset pointers if we have now read everything
    if (pInstanceInfo->readPointer ==  pInstanceInfo->writePointer) {
      // clear the buffer
      mgrResetWorkspace(pInstanceInfo);
    }
    // unlock
    pInstanceInfo->readLocked=0;
  #else
    Ret_t rc;
    
    /* --- Pass the number of bytes which have been read --- */
    LOCKTOOLKIT("smlUnlockReadBuffer");
    rc = wsmProcessedBytes (id,processedBytes);
    RELEASETOOLKIT("smlUnlockReadBuffer");
    if (rc!=SML_ERR_OK) return rc;

    /* --- Unlock Workspace --- */
    LOCKTOOLKIT("smlUnlockReadBuffer");
    rc = wsmUnlockH(id);
    RELEASETOOLKIT("smlUnlockReadBuffer");
    if (rc!=SML_ERR_OK) return rc;
  #endif

  return SML_ERR_OK;
}


#ifdef NOWSM

/**
 * FUNCTION:  smlSetMaxOutgoingSize
 *
 * marks the current write pointer position as beginning of a new outgoing
 * message. This is used to track outgoing message size while writing it
 *
 * IN:        InstanceID_t              
 *            ID of the Instance
 *
 * IN:        MemSize_t              
 *            maximum size of outgoing message (0=no limit except buffer size)
 *
 * RETURN:    Return value,            
 *            SML_ERR_OK if successful
 */
SML_API Ret_t smlSetMaxOutgoingSize(InstanceID_t id, MemSize_t maxOutgoingSize)
{
  InstanceInfoPtr_t	pInstanceInfo;

  pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
  if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;

  // set max outgoing message size
  pInstanceInfo->maxOutgoingSize = maxOutgoingSize;
  
  return SML_ERR_OK;
}


/**
 * FUNCTION:  smlSetOutgoingBegin
 *
 * marks the current write pointer position as beginning of a new outgoing
 * message. This is used to track outgoing message size while writing it
 *
 * IN:        InstanceID_t              
 *            ID of the Instance
 *
 * RETURN:    Return value,            
 *            SML_ERR_OK if successful
 */
SML_API Ret_t smlSetOutgoingBegin(InstanceID_t id)
{
  InstanceInfoPtr_t	pInstanceInfo;
  
  pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
  if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;

  // remember current write pointer
  pInstanceInfo->outgoingMsgStart=pInstanceInfo->writePointer;
  
  return SML_ERR_OK;
}

#endif

/**
 * FUNCTION:  smlLockWriteBuffer
 *
 * Locks the workspace buffer, which is assigned to the given 
 * instance for writing. After this function is called, the 
 * application has access to the workspace buffer, beginning 
 * at the address pWritePosition which is returned by this 
 * function. SyncML will not change the workspace buffer until 
 * smlUnlockWriteBuffer is called.
 * pWritePosition returns a pointer to a valid position in the 
 * SyncML workspace buffer. The pointer can be used by the application 
 * for copying incoming synchronization data from some transport 
 * layer into the buffer. freeSize retrieves the maximum usable 
 * size of the workspace buffer beginning from the address to 
 * which pWritePosition points to. This information is needed by 
 * the application when copying XML code into the buffer (while 
 * receiving synchronization data)
 *
 * IN:        InstanceID_t              
 *            ID of the Instance
 *
 * OUT:       MemPtr_t              
 *            Workspace Pointer to which data can be written
 *
 * OUT:       MemSize_t              
 *            Max free Size of available space for data
 *
 * RETURN:    Return value,            
 *            SML_ERR_OK if successful
 */
SML_API Ret_t smlLockWriteBuffer(InstanceID_t id, MemPtr_t *pWritePosition, MemSize_t *freeSize)
{
  #ifdef NOWSM
    InstanceInfoPtr_t	pInstanceInfo;
    
    pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
    if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;
    // must not be already locked here
    if (pInstanceInfo->writeLocked)
      return SML_ERR_WRONG_USAGE;
    // return current write pointer
    *pWritePosition = pInstanceInfo->writePointer;
    // free portion is either determined by actual room in buffer, or maximum outgoing size if set
    if (
      pInstanceInfo->maxOutgoingSize &&
      pInstanceInfo->outgoingMsgStart &&
      pInstanceInfo->outgoingMsgStart<pInstanceInfo->writePointer
    ) {
      // calculate what is allowed according to maxOutgoingSize
      *freeSize =
        (pInstanceInfo->maxOutgoingSize) - // maximum outgoing size
        (pInstanceInfo->writePointer-pInstanceInfo->outgoingMsgStart); // size of outgoing message so far
      if (pInstanceInfo->writePointer+*freeSize > pInstanceInfo->instanceBuffer+pInstanceInfo->instanceBufSiz) {
        // actual space in buffer is smaller
        *freeSize =
          (pInstanceInfo->instanceBuffer+pInstanceInfo->instanceBufSiz) - // end of buffer
          pInstanceInfo->writePointer; // current write position
      }
    }
    else {
      // simply return available size in buffer
      *freeSize =
        (pInstanceInfo->instanceBuffer+pInstanceInfo->instanceBufSiz) - // end of buffer
        pInstanceInfo->writePointer; // current write position
    }
    // lock
    pInstanceInfo->writeLocked=1;
  #else
    Ret_t rc;
    
    /* --- Lock Workspace exclusively for writing and get a "Write" pointer --- */
    LOCKTOOLKIT("smlLockWriteBuffer");
    rc = wsmLockH(id, SML_FIRST_FREE_ITEM, pWritePosition);
    RELEASETOOLKIT("smlLockWriteBuffer");
    if (rc!=SML_ERR_OK) return rc;

    /* --- Check, how much free space is available for writing --- */
    LOCKTOOLKIT("smlLockWriteBuffer");
    rc = wsmGetFreeSize(id, freeSize);
    RELEASETOOLKIT("smlLockWriteBuffer");
    if (rc!=SML_ERR_OK) return rc;
  #endif
  
  return SML_ERR_OK;
}




/**
 * FUNCTION:  smlUnlockWriteBuffer
 *
 * End the write access of the application to the workspace buffer. 
 * SyncML is now owner of the buffer again and is able to manipulate its 
 * contents. writtenBytes passes the number of bytes which have been 
 * written into the workspace buffer (e.g. when the application has copied 
 * incoming synchronization data from a communication module into the 
 * workspace). This information is needed by SyncML when processing received 
 * synchronization data.
 *
 * IN:        InstanceID_t              
 *            ID of the Instance
 *
 * IN:        MemSize_t              
 *            Actually written bytes
 *
 * RETURN:    Return value,            
 *            SML_ERR_OK if successful
 */
SML_API Ret_t smlUnlockWriteBuffer(InstanceID_t id, MemSize_t writtenBytes)
{
  #ifdef NOWSM
    InstanceInfoPtr_t	pInstanceInfo;
    
    pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
    if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;
    // must be already locked here
    if (!pInstanceInfo->writeLocked)
      return SML_ERR_WRONG_USAGE;
    if (writtenBytes > 0) {
      // advance write pointer by number of bytes written
      if (pInstanceInfo->writePointer+writtenBytes>pInstanceInfo->instanceBuffer+pInstanceInfo->instanceBufSiz)
        return SML_ERR_WRONG_USAGE; // too many bytes written
      // update write pointer
      pInstanceInfo->writePointer+=writtenBytes;
    }
    // unlock
    pInstanceInfo->writeLocked=0;
  #else
    Ret_t rc;

    if (writtenBytes > 0)
    {
      /* --- Pass the number of bytes which have been written --- */
      LOCKTOOLKIT("smlUnlockWriteBuffer");
      rc = wsmSetUsedSize(id,writtenBytes);
      RELEASETOOLKIT("smlUnlockWriteBuffer");
      if (rc!=SML_ERR_OK) return rc;
    }
    /* --- Unlock Workspace --- */
    LOCKTOOLKIT("smlUnlockWriteBuffer");
    rc = wsmUnlockH(id);
    RELEASETOOLKIT("smlUnlockWriteBuffer");
    if (rc!=SML_ERR_OK) return rc;
  #endif

  return SML_ERR_OK;
}




/*************************************************************************
 *  SyncML internal functions 
 *************************************************************************/


/**
 * FUNCTION:  mgrResetWorkspace
 * Reset the Workspace Buffer position to the beginning of the workspace
 * (clears all data in the buffer)
 *
 * IN:        InstanceID_t              
 *            ID of the Instance
 * RETURN:    Return value,            
 *            SML_ERR_OK if successful
 */
Ret_t mgrResetWorkspace (InstanceID_t id) {
  #ifdef NOWSM
    InstanceInfoPtr_t	pInstanceInfo;
    
    pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
    if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;

    pInstanceInfo->readPointer=pInstanceInfo->instanceBuffer;
    pInstanceInfo->writePointer=pInstanceInfo->instanceBuffer;
    pInstanceInfo->outgoingMsgStart=NULL; // no outgoing message in the buffer
    return SML_ERR_OK; // ok
  #else
    Ret_t rc;
    LOCKTOOLKIT("mgrResetWorkspace");
    rc=wsmReset (id);
    RELEASETOOLKIT("mgrResetWorkspace");
    return rc;
  #endif
}



/**
 * FUNCTION:   setInstanceOptions
 *
 * the options settings of an instance are set to a new value
 *
 * IN:              InstanceID_t
 *                  Instance ID assigned to the instance
 *
 * IN:              SmlInstanceOptionsPtr_t
 *                  New option settings of that particular SyncML instance
 *                  NOTE: only the encoding can be changed during life-time 
 *                  of an instance
 *                  The other parameters of the instance options 
 *                  (workspace size and name cannot be changed) 
 *
 * RETURN:          Ret_t
 *                  Error Code
 */
Ret_t setInstanceOptions (InstanceID_t id, SmlInstanceOptionsPtr_t pOptions)
{

  /* --- Definitions --- */
  InstanceInfoPtr_t         pInstanceInfo;
  SmlInstanceOptionsPtr_t      pOptionsCopy;


  #ifdef NOWSM
    /* --- Ckeck pOptions, which have been passed by the application --- */
    if (!pOptions || (pOptions->encoding==SML_UNDEF))
  	  return SML_ERR_WRONG_USAGE;

    pInstanceInfo = (InstanceInfoPtr_t)id; // ID is the instance info pointer
  #else  
    /* --- Ckeck pOptions, which have been passed by the application --- */
    if (!pOptions || !pOptions->workspaceName|| (pOptions->encoding==SML_UNDEF))
  	  return SML_ERR_WRONG_USAGE;

    /* --- Find that instance --- */
    #ifdef __SML_LITE__  /* Only ONE instance is supported in the Toolkit lite version */
      pInstanceInfo = mgrGetInstanceListAnchor();
    #else
      pInstanceInfo = (InstanceInfoPtr_t) findInfo(id);
    #endif
  #endif

  if (pInstanceInfo==NULL) return SML_ERR_MGR_INVALID_INSTANCE_INFO;
  
  /* --- free old instance options ---*/
  freeInstanceOptions(pInstanceInfo);

  /* --- Use a copy of pOptionsCopy --- */
  pOptionsCopy = (SmlInstanceOptionsPtr_t)smlLibMalloc((MemSize_t)sizeof(SmlInstanceOptions_t));
  if (pOptionsCopy==NULL) return SML_ERR_NOT_ENOUGH_SPACE;
  smlLibMemcpy(pOptionsCopy,pOptions,(MemSize_t)sizeof(SmlInstanceOptions_t));

  #ifndef NOWSM
  pOptionsCopy->workspaceName=smlLibStrdup(pOptions->workspaceName);

  if (pOptionsCopy->workspaceName == NULL) {
	  pInstanceInfo->instanceOptions=NULL;
 	  smlLibFree(pOptionsCopy);
 	  return SML_ERR_NOT_ENOUGH_SPACE;
   }
  #endif
   
  /* --- Assign the new options --- */
  pInstanceInfo->instanceOptions=pOptionsCopy;
  

  /* --- Let the new settingds take effect --- */
  /* --- Adjust workspace size ---*/
  /* --- Change workspace name ---*/
  // NOT SUPPORTED FOR YELLOW 

  return SML_ERR_OK;
}



/**
 * FUNCTION:  freeInstanceOptions
 * Free Instances Options
 *
 * RETURN:    InstanceInfoPtr_t         
 *            Pointer to the pInstance Info, which options should be freed
 */
Ret_t freeInstanceOptions (InstanceInfoPtr_t pInfo) {

  /* --- Delete instance options (if there are any) --- */
  if (pInfo->instanceOptions!=NULL) {
    #ifndef NOWSM
    if (pInfo->instanceOptions->workspaceName!=NULL) 
      smlLibFree(pInfo->instanceOptions->workspaceName);  // don't forget the substructures
    #endif
    smlLibFree(pInfo->instanceOptions);
  }

 return SML_ERR_OK;  
}

/**
 * FUNCTION: 
 * Free the memory of an removed Instance Info (including referenced sub structures) 
 *
 * IN:        InstanceID_t              
 *            ID of the InstanceInfo structure to be freed
 */
static Ret_t freeInstanceInfo(InstanceInfoPtr_t pInfo) {

	if (pInfo) {

	  #ifdef NOWSM
    // return the instance buffer
    if (pInfo->instanceBuffer)
      smlLibFree(pInfo->instanceBuffer);
    #else
		if (pInfo->workspaceState) 
			smlLibFree(pInfo->workspaceState);
    #endif
		if (pInfo->encoderState) 
			smlLibFree(pInfo->encoderState);
		if (pInfo->decoderState)
			smlLibFree(pInfo->decoderState);
		if (pInfo->callbacks)
			smlLibFree(pInfo->callbacks);
      
   	freeInstanceOptions(pInfo);
    
		smlLibFree(pInfo);
	}
    
	return SML_ERR_OK;
}
