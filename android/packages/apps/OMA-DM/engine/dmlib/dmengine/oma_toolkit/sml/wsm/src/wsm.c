/*************************************************************************/
/* module:          SyncML WorkSpace Manager                             */
/* file:            WSM.c                                                */
/* target system:   All                                                  */
/* target OS:       All                                                  */
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


/**
 * Workspace Manager API <BR>
 * Manages the SyncML document in memory.
 *
 * @version  @label
 * 
 *
 */


#ifndef NOWSM
// if no WSM, we can leave this one out completely

/*************************************************************************/
/*  Definitions                                                          */
/*************************************************************************/

#include "wsm.h"

#include "wsm_sm.h"
#include "smldef.h"
#include "libmem.h"
#include "libstr.h"
#include "liblock.h" // for THREADDEBUGPRINTF %%% luz
#include "mgr.h"   // for global anchor


/** WSM buffer flags */
#define  WSM_VALID_F     (Byte_t) 0x01
#define  WSM_LOCKED_F    (Byte_t) 0x02

#define  WSM_MEMH_UNUSED   -1

#ifndef __SML_LITE__

/* Global Vars */
/* =========== */

/* defines for convient use of global anchor */

#define wsmRet          (mgrGetSyncMLAnchor())->wsmGlobals->wsmRet
#define initWasCalled   (mgrGetSyncMLAnchor())->wsmGlobals->initWasCalled
#define maxWsmAvailMem     (mgrGetSyncMLAnchor())->syncmlOptions->maxWorkspaceAvailMem
#define wsmBuf          (mgrGetSyncMLAnchor())->wsmGlobals->wsmBuf
#define wsmIndex        (mgrGetSyncMLAnchor())->wsmGlobals->wsmIndex

void createDataStructs(void);

void createDataStructs() {
  if ( (mgrGetSyncMLAnchor())->wsmGlobals == NULL ) {
    if ( ((mgrGetSyncMLAnchor())->wsmGlobals=smlLibMalloc(sizeof(WsmGlobals_t))) == 0 ) {
      return;
    }
    smlLibMemset((mgrGetSyncMLAnchor())->wsmGlobals, 0, sizeof(WsmGlobals_t));
    wsmRet = 0;
    initWasCalled = 0;
    wsmIndex = 0;
#ifdef __ANSI_C__
    (mgrGetSyncMLAnchor())->wsmGlobals->wsmSm = NULL;
#endif
#ifdef __PALM_OS__
    (mgrGetSyncMLAnchor())->wsmGlobals->wsmSm.smMemH = 0;
    (mgrGetSyncMLAnchor())->wsmGlobals->wsmSm.smLocked = 0;
#endif
#ifdef __EPOC_OS__
    (mgrGetSyncMLAnchor())->wsmGlobals->wsmSm = NULL;
#endif
  }
}
#define freeDataStructs() smlLibFree((mgrGetSyncMLAnchor())->wsmGlobals)


/* private functions prototypes */
static Short_t getNextFreeEntry();
static Short_t lookup(MemHandle_t memH);
static MemHandle_t nameToHandle(String_t name);
static Short_t deleteBufferHandle(MemHandle_t memH);
static Short_t resetBufferGlobals(MemHandle_t memH);
static Byte_t isValidMemH(MemHandle_t memH);
static Byte_t isLockedMemH(MemHandle_t memH);
static Byte_t isMemAvailable(MemSize_t memToAlloc);




/*************************************************************************/
/*  Internal Functions                                                   */
/*************************************************************************/



/**
 * Get next free buffer entry.
 * Returns index of next free entry, or -1 if buffer table is full.
 */
static Short_t getNextFreeEntry() {
  Short_t i;

  for ( i=0; i < MAX_WSM_BUFFERS; ++i )
    if ( wsmBuf[i].memH == WSM_MEMH_UNUSED )
      return i;

  return -1;
}



/**
 * Get buffer table index for memH.
 * Returns -1 if memH not found.
 */
static Short_t lookup(MemHandle_t memH) {
  Short_t i;

  // first check cache
  if ( wsmBuf[wsmIndex].memH == memH )
    return wsmIndex;

  // search through buffer
  for ( i=0; (i < MAX_WSM_BUFFERS) && (wsmBuf[i].memH != memH); ++i )
    ;
  if ( i < MAX_WSM_BUFFERS ) {
    wsmIndex = i;
    return i;
  }
  else {
    return -1;     // memH not found  
  }
}


/**
 * Find memory handle corresponding to name. 
 * Return WSM_MEMH_UNUSED, if name not found in wsmBuf.
 */
static MemHandle_t nameToHandle(String_t name) {
  int i;

  // first check cache
  if ( (wsmBuf[wsmIndex].bufName != NULL) &&
       (smlLibStrcmp(wsmBuf[wsmIndex].bufName, name) == 0) )
    return wsmBuf[wsmIndex].memH;

  // search through buffer
  for ( i=0; ((i < MAX_WSM_BUFFERS) && 
	      (wsmBuf[i].bufName == NULL ? 1 :
	       smlLibStrcmp(wsmBuf[i].bufName, name) != 0)); ++i )
    ;
  if ( i < MAX_WSM_BUFFERS ) 
    return wsmBuf[i].memH;
  else {
    return WSM_MEMH_UNUSED;     // name not found  
  }
}

  

/**
 * Delete memory handle from buffer.
 * Return -1, if handle not found.
 */
static Short_t deleteBufferHandle(MemHandle_t memH) {
  if ( (wsmIndex = lookup(memH)) < 0 )
    return -1;  // handle not found

  // reset the values
  wsmBuf[wsmIndex].memH = WSM_MEMH_UNUSED;
  wsmBuf[wsmIndex].pFirstFree = NULL;
  wsmBuf[wsmIndex].pFirstData = NULL;
  wsmBuf[wsmIndex].size      = 0;
  wsmBuf[wsmIndex].usedBytes = 0;
  //wsmBuf[wsmIndex].flags     = ~WSM_VALID_F;
  wsmBuf[wsmIndex].flags     = ((Byte_t) ~WSM_VALID_F);
  smlLibFree(wsmBuf[wsmIndex].bufName);   // free mem
  wsmBuf[wsmIndex].bufName   = NULL;

  return 0;
}


/**
 * Reset values in buffer table for entry memH.
 * If memH doesn't exist create an entry.
 * Return index to memH in buffer table,
 * or -1 if table is full
 */
static Short_t resetBufferGlobals(MemHandle_t memH) {
  if ( (wsmIndex = lookup(memH)) < 0 ) {
    // create new one
    if ( (wsmIndex = getNextFreeEntry()) < 0 )
      return -1;  // buffer table full
    wsmBuf[wsmIndex].memH = memH;
  } else
    // use existing one, which has to be reset prior usage
    smlLibFree(wsmBuf[wsmIndex].bufName);   // free mem

  // reset the values
  wsmBuf[wsmIndex].pFirstFree = NULL;
  wsmBuf[wsmIndex].pFirstData = NULL;
  wsmBuf[wsmIndex].size      = 0;
  wsmBuf[wsmIndex].usedBytes = 0;
  wsmBuf[wsmIndex].flags     = WSM_VALID_F;
  wsmBuf[wsmIndex].bufName   = NULL;

  return wsmIndex;
}


static Byte_t isValidMemH(MemHandle_t memH) {
  Short_t i;
  i = lookup(memH); 
  if (i < 0)  
  { 
    return 0;
  }else{
    return (Byte_t) (wsmBuf[i].flags & WSM_VALID_F);
  }
}

static Byte_t isLockedMemH(MemHandle_t memH) {
  Short_t i;
  i = lookup(memH); 
  if (i < 0)  
  { 
    return 0;
  }else{
    return (Byte_t) (wsmBuf[i].flags & WSM_LOCKED_F);
  }
}

static Byte_t isMemAvailable(MemSize_t memToAlloc) {
  int i;
  MemSize_t actMem = memToAlloc;
  if ( maxWsmAvailMem == 0 )
    return 1;  // no memsize restrictions
  for (i=0; i < MAX_WSM_BUFFERS; ++i) {
    if ( wsmBuf[i].memH != WSM_MEMH_UNUSED )
      actMem += wsmBuf[i].size;
  }
  return ((Byte_t)(actMem <= maxWsmAvailMem));
}



/*************************************************************************/
/*  External Functions                                                   */
/*************************************************************************/



/**
 * FUNCTION: wsmInit
 *
 * Initializes all Workspace Manager related resources.<BR>
 * Should only be called once!
 *
 * PRE-Condition:   This is the first function call to WSM
 *
 * POST-Condition:  All WSM resources are initialized
 *
 * IN:      wsmOpts
 *          WSM options, valid options are:
 *          <UL>
 *          <LI> maxAvailMem<BR>
 *               Maximal amount of memory which wsm can use for the buffers<BR>
 *               0 == no limitation
 *          </UL>
 * 
 * OUT:     wsmH
 *          Handle to new buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_OPTIONS, if wsmOpts is not valid
 *          SML_ERR_NOT_ENOUGH_SPACE, if not enough available memory
 *          SML_ERR_WRONG_USAGE, if wsmInit was already called
 */
Ret_t wsmInit (const WsmOptions_t *wsmOpts) {
  int i;

  // create global datastructs
  createDataStructs();

  if (NULL == mgrGetSyncMLAnchor()->wsmGlobals) {
 	  return SML_ERR_NOT_ENOUGH_SPACE;
   }

  // check if init was already called
  if ( initWasCalled )
    return SML_ERR_WRONG_USAGE;

  // check options
  if ( wsmOpts != NULL ) {
    if ( wsmOpts->maxAvailMem > 0 ) {
      maxWsmAvailMem = wsmOpts->maxAvailMem;
    }
  }

  // init resources
  for ( i=0; i < MAX_WSM_BUFFERS; ++i )
    wsmBuf[i].memH = WSM_MEMH_UNUSED;
  wsmIndex = 0;
  initWasCalled = (Byte_t) 1;

  return wsmRet=SML_ERR_OK;
} 


/**
 * FUNCTION: wsmCreate
 *
 * Creates and opens a new buffer with name bufName and size bufSize.<BR> 
 * If a buffer with name bufName already exists, the existing buffer 
 * is resized to bufSize.
 *
 * PRE-Condition:   bufSize > 0
 *
 * POST-Condition:  handle refers to buffer bufName; BufferSize = size
 *
 * IN:      bufName
 *          Name of buffer to be created
 * IN:      bufSize
 *          Size of buffer to be created
 * 
 * OUT:     wsmH
 *          Handle to new buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_SIZE, if bufSize <= 0
 *          SML_ERR_NOT_ENOUGH_SPACE, if available memory < bufSize
 *          SML_ERR_WSM_BUF_TABLE_FULL, if buffer table is full
 *          SML_ERR_WRONG_USAGE, if wsmInit wasn't called before
 *
 * @see  wsmDestroy
 */
Ret_t wsmCreate (String_t bufName, MemSize_t bufSize, MemHandle_t *wsmH) {

  *wsmH = 0;    // 0 in case of error

  if ( ! initWasCalled )
    return SML_ERR_WRONG_USAGE;

  // check buffer space
  if ( getNextFreeEntry() == -1 ) {
    return wsmRet=SML_ERR_WSM_BUF_TABLE_FULL;
  }
  // check for maxMemAvailable
  if ( ! isMemAvailable(bufSize) ) {
    return SML_ERR_NOT_ENOUGH_SPACE;
  }

  // create buffer
  if ( (wsmRet = smCreate(bufName, bufSize, wsmH)) != SML_ERR_OK ) {
    if ( wsmRet == SML_ERR_WRONG_USAGE ) {    // buffer already exists
      // resize existing buffer
      // open buffer
      if ( (wsmRet = smOpen(bufName, wsmH)) != SML_ERR_OK ) {
      	return wsmRet=SML_ERR_NOT_ENOUGH_SPACE;
      }
      // resize buffer
      if ( (wsmRet = smSetSize(*wsmH, bufSize)) != SML_ERR_OK ) { 
      	return wsmRet=SML_ERR_NOT_ENOUGH_SPACE;
      }
    }
    else {
      return wsmRet;
    }
  }

  // reset buffer vars
  wsmIndex = resetBufferGlobals(*wsmH);

  // set buffer vars
  wsmBuf[wsmIndex].size = bufSize;
  wsmBuf[wsmIndex].bufName = smlLibStrdup(bufName);

   if (wsmBuf[wsmIndex].bufName == NULL) {
 	  smClose(*wsmH);
 	  smDestroy(bufName);
 	  return wsmRet=SML_ERR_NOT_ENOUGH_SPACE;
   }


  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmOpen
 *
 * Open existing buffer with name bufName.
 *
 * PRE-Condition:   WSM knows bufName
 *
 * POST-Condition:  wsmH refers to buffer bufName
 *
 * IN:      bufName
 *          Name of buffer to be opened
 * 
 * OUT:     wsmH
 *          Handle to new buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_WRONG_PARAM, if bufName is unknown
 *
 * @see  wsmClose
 */
Ret_t wsmOpen (String_t bufName, MemHandle_t *wsmH){

  // open buffer
  if ( (wsmRet = smOpen(bufName, wsmH)) != SML_ERR_OK ) {
    return wsmRet;
  }

  // reset buffer vars
  wsmIndex = resetBufferGlobals(*wsmH);

  // set buf vars
  wsmRet = smGetSize(*wsmH, &wsmBuf[wsmIndex].size);
  wsmBuf[wsmIndex].bufName = smlLibStrdup(bufName);

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmClose
 *
 * Close an open buffer.
 *
 * PRE-Condition:   handle is valid; handle is unlocked
 *
 * POST-Condition:  handle is not known to WSM any more
 *
 * IN:      wsmH
 *          Handle to the open buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *          SML_ERR_WRONG_USAGE, if handle was still locked
 *
 * @see  wsmOpen
 */
Ret_t wsmClose (MemHandle_t wsmH) {

  // check if handle is invalid
  if ( ! isValidMemH(wsmH) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }

  // close handle
  if ( (wsmRet = smClose(wsmH)) != SML_ERR_OK ) {
    return wsmRet;
  }
  wsmRet = deleteBufferHandle(wsmH);

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmDestroy
 *
 * Destroy existing buffer with name bufName.
 *
 * PRE-Condition:   WSM knows bufName; handle is unlocked
 *
 * POST-Condition:  buffer is not known to WSM any more; all resources 
 *                  connected to this buffer are freed
 *
 * IN:      bufName
 *          Name of buffer to be opened
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_PARAM, if bufName is unknown to WSM
 *          SML_ERR_WRONG_USAGE, if handle was still locked
 *
 * @see  wsmCreate
 */
Ret_t wsmDestroy (String_t bufName) {

  // free resources
  if ( (wsmRet = wsmClose(nameToHandle(bufName))) != SML_ERR_OK ) {
    return wsmRet;
  }

  // free buffer
  if ( (wsmRet = smDestroy(bufName)) != SML_ERR_OK ) {
    return wsmRet;
  }
   
  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmTerminate
 *
 * Terminate WSM; free all buffers and resources.
 *
 * PRE-Condition: all handles must be unlocked
 *
 * POST-Condition: all resources are freed
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_USAGE, if a handle was still locked
 *
 */
Ret_t wsmTerminate (void) {
  int i;

  // free all WSM resources
  for (i=0; i < MAX_WSM_BUFFERS; ++i) {
    if ( wsmBuf[i].memH != WSM_MEMH_UNUSED )
      if ( wsmDestroy(wsmBuf[i].bufName) == SML_ERR_WRONG_USAGE ) {
	return SML_ERR_WRONG_USAGE;
      }
  }

  // free global DataStructs
  freeDataStructs();

  return SML_ERR_OK;
}


/**
 * FUNCTION: wsmProcessedBytes
 *
 * Tell Workspace Manager the number of bytes already processed.
 *
 * PRE-Condition:   handle is locked; handle is valid;
 *                  noBytes <= wsmGetUsedSize
 *
 * POST-Condition:  noBytes starting at wsmGetPtr() position are deleted; 
 *                  remaining bytes are copied to 
 *                  wsmGetPtr(SML_FIRST_FREE_ITEM) position;
 *                  wsmGetUsedSize -= noBytes; wsmGetFreeSize += noBytes
 *
 * IN:      wsmH
 *          Handle to the open buffer
 * IN:      noBytes
 *          Number of bytes already processed from buffer.
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *          SML_ERR_WRONG_USAGE, if handle was not locked
 *          SML_ERR_INVALID_SIZE, if noBytes > wsmGetUsedSize
 *
 * @see  wsmGetFreeSize
 */
Ret_t wsmProcessedBytes (MemHandle_t wsmH, MemSize_t noBytes) {

  // check if handle is invalid
  if ( ! isValidMemH(wsmH) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }
  // check if handle is unlocked
  if ( ! isLockedMemH(wsmH) ) {
    return wsmRet=SML_ERR_WRONG_USAGE;
  }

  wsmIndex = lookup(wsmH);

  if ( noBytes > wsmBuf[wsmIndex].usedBytes ) {
    return wsmRet=SML_ERR_INVALID_SIZE;
  }

  // adapt usedSize
  wsmBuf[wsmIndex].usedBytes -= noBytes;

  // move memory
  // check return ?????
  smlLibMemmove(wsmBuf[wsmIndex].pFirstData,
	  (wsmBuf[wsmIndex].pFirstData + noBytes),
	  wsmBuf[wsmIndex].usedBytes);

  // move pFirstFree
  wsmBuf[wsmIndex].pFirstFree -= noBytes;

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmLockH
 *
 * Locks handle wsmH and get a pointer to the contents of wsmH. <BR>
 * RequestedPos describes the position in the buffer to which the returned 
 * pointer should point. Valid values are: 
 * <UL>
 *   <LI> SML_FIRST_DATA_ITEM
 *   <LI> SML_FIRST_FREE_ITEM
 * </UL>
 *
 * PRE-Condition:   handle is unlocked; handle is valid
 *
 * POST-Condition:  handle is locked; points to first data item, 
 *                  or first free item.
 *
 * IN:      wsmH
 *          Handle to the open buffer
 * IN:      requestedPos
 *          Requested position of the returned pointer
 *          <UL>
 *            <LI> SML_FIRST_DATA_ITEM : points to first data entry
 *            <LI> SML_FIRST_FREE_ITEM : points to first free entry
 *          </UL>
 * 
 * OUT:     pMem
 *          Pointer to requested memory          
 *          
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *          SML_ERR_WRONG_USAGE, if handle was still locked
 *          SML_ERR_UNSPECIFIC, if requested position is unknown, or lock failed
 *
 * @see  wsmUnlockH
 */
Ret_t wsmLockH (MemHandle_t wsmH, SmlBufPtrPos_t requestedPos,
		MemPtr_t *pMem) {

  // check if handle is invalid
  if ( ! isValidMemH(wsmH) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }
  // check if handle is locked
  if ( isLockedMemH(wsmH) ) {
    return wsmRet=SML_ERR_WRONG_USAGE;
  }

  // lock
  if ( (wsmRet = smLock(wsmH, pMem)) != SML_ERR_OK ) {
    return wsmRet=SML_ERR_UNSPECIFIC;
  }

  // set local pointers  
  wsmIndex = lookup(wsmH);
  wsmBuf[wsmIndex].pFirstData = *pMem;
  wsmBuf[wsmIndex].pFirstFree = *pMem + wsmBuf[wsmIndex].usedBytes;
  wsmBuf[wsmIndex].flags |= WSM_LOCKED_F;

  switch (requestedPos) {
  case SML_FIRST_DATA_ITEM:
    *pMem = wsmBuf[wsmIndex].pFirstData;
    break;
  case SML_FIRST_FREE_ITEM:
    *pMem = wsmBuf[wsmIndex].pFirstFree;
    break;
  default:
    return wsmRet=SML_ERR_UNSPECIFIC;
  }

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmGetFreeSize
 *
 * Returns the remaining unused bytes in the buffer.
 *
 * PRE-Condition:   handle is valid
 *
 * POST-Condition:  wsmGetFreeSize = BufferSize - wsmGetUsedSize
 *
 * IN:      wsmH
 *          Handle to the open buffer
 *
 * OUT:     freeSize
 *          Number of bytes which are unused in this buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *
 * @see  wsmGetUsedSize
 * @see  wsmProcessedBytes
 */
Ret_t wsmGetFreeSize(MemHandle_t wsmH, MemSize_t *freeSize) {

  // check if handle is invalid
  if ( ! isValidMemH(wsmH) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }
  
  wsmIndex = lookup(wsmH);

  *freeSize = wsmBuf[wsmIndex].size - wsmBuf[wsmIndex].usedBytes;

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmGetUsedSize
 *
 * Returns the number of bytes used in the buffer.
 *
 * PRE-Condition:   handle is valid
 *
 * POST-Condition:  usedSize = BufferSize - wsmGetFreeSize
 *
 * IN:      wsmH
 *          Handle to the open buffer
 *
 * OUT:     usedSize
 *          Number of bytes which are already used in this buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *
 * @see  wsmGetFreeSize
 * @see  wsmSetUsedSize
 */
Ret_t wsmGetUsedSize(MemHandle_t wsmH, MemSize_t *usedSize) {

  // check if handle is invalid
  if ( ! isValidMemH(wsmH) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }
  
  wsmIndex = lookup(wsmH);

  *usedSize = wsmBuf[wsmIndex].usedBytes;

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmUnlockH
 *
 * Unlock handle wsmH. <BR>
 * After this call all pointers to this memory handle are invalid 
 * and should no longer be used.
 *
 * PRE-Condition:   handle is locked; handle is valid
 *
 * POST-Condition:  handle is unlocked
 *
 * OUT:     wsmH
 *          Handle to unlock
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *          SML_ERR_WRONG_USAGE, if handle was not locked
 *          SML_ERR_UNSPECIFIC, unlock failed
 *
 * @see  wsmLockH
 */
Ret_t wsmUnlockH (MemHandle_t wsmH) {

  // check if handle is invalid
  if ( ! isValidMemH(wsmH) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }
  // check if handle is already unlocked
  if ( ! isLockedMemH(wsmH) ) {
    return wsmRet=SML_ERR_WRONG_USAGE;
  }

  // unlock
  if ( (wsmRet = smUnlock(wsmH)) != SML_ERR_OK ) {
    return wsmRet=SML_ERR_UNSPECIFIC;
  }

  // set local pointers  
  wsmIndex = lookup(wsmH);
  wsmBuf[wsmIndex].pFirstData = NULL;
  wsmBuf[wsmIndex].pFirstFree = NULL;
  wsmBuf[wsmIndex].flags &= ~WSM_LOCKED_F;

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmSetUsedSize
 *
 * Tell Workspace how many data were written into buffer.
 *
 * PRE-Condition:   handle is valid; usedSize <= wsmGetFreeSize; handle is 
 *                  locked
 *
 * POST-Condition:  wsmGetUsedSize += usedSize; wsmGetFreeSize -= usedSize;
 *                  instancePtr += usedSize;
 *
 * IN:      wsmH
 *          Handle to the open buffer
 * IN:      usedSize
 *          Number of bytes which were written into buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *          SML_ERR_INVALID_SIZE, if usedSize <= wsmGetFreeSize
 *
 * @see  wsmGetUsedSize
 */
Ret_t wsmSetUsedSize (MemHandle_t wsmH, MemSize_t usedSize) {

  // check if handle is invalid
  if ( ! isValidMemH(wsmH) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }
  // check if handle is unlocked
  if ( ! isLockedMemH(wsmH) ) {
    return wsmRet=SML_ERR_WRONG_USAGE;
  }

  wsmIndex = lookup(wsmH);

  // usedSize > freeSize?
  if ( usedSize > 
       (wsmBuf[wsmIndex].size - wsmBuf[wsmIndex].usedBytes) ) {
    return wsmRet=SML_ERR_INVALID_SIZE;
  }

  // adapt usedSize
  wsmBuf[wsmIndex].usedBytes += usedSize;

  // move pFirstFree
  wsmBuf[wsmIndex].pFirstFree += usedSize;

  return wsmRet=SML_ERR_OK;
}
 
/**
 * FUNCTION: wsmReset
 *
 * Reset the Workspace
 *
 * PRE-Condition:   -
 *
 * POST-Condition:  all data is lost. The FirstFree Position equals 
 * the First Data position
 *
 * IN:      wsmH
 *          Handle to the open buffer
 * 
 * RETURN:  SML_ERR_OK, if O.K.
 *
 */
Ret_t wsmReset (MemHandle_t wsmH) {

  wsmIndex = lookup(wsmH);
  wsmBuf[wsmIndex].pFirstFree = wsmBuf[wsmIndex].pFirstFree - wsmBuf[wsmIndex].usedBytes ;
  wsmBuf[wsmIndex].pFirstData = wsmBuf[wsmIndex].pFirstFree;
  wsmBuf[wsmIndex].usedBytes = 0;

  return SML_ERR_OK;
}

/*======================================================================================*/
#else
/* WSM_LITE Version - uses only one buffer*/
/*======================================================================================*/


/* Global Vars */
/* =========== */

/* defines for convient use of global anchor */
#define wsmRet          (mgrGetSyncMLAnchor())->wsmGlobals->wsmRet
#define initWasCalled   (mgrGetSyncMLAnchor())->wsmGlobals->initWasCalled
#define maxWsmAvailMem  (mgrGetSyncMLAnchor())->syncmlOptions->maxWorkspaceAvailMem
#define wsmBuf          (mgrGetSyncMLAnchor())->wsmGlobals->wsmBuf
#define wsmIndex        (mgrGetSyncMLAnchor())->wsmGlobals->wsmIndex

void createDataStructs(void);

void createDataStructs() {
  if ( (mgrGetSyncMLAnchor())->wsmGlobals == NULL ) {
    if ( ((mgrGetSyncMLAnchor())->wsmGlobals=smlLibMalloc(sizeof(WsmGlobals_t))) == 0 ) {
      return;
    }
    wsmRet = 0;
    initWasCalled = 0;
    wsmIndex = 0;
#ifdef __ANSI_C__
    (mgrGetSyncMLAnchor())->wsmGlobals->wsmSm = NULL;
#endif
#ifdef __PALM_OS__
    (mgrGetSyncMLAnchor())->wsmGlobals->wsmSm.smMemH = 0;
    (mgrGetSyncMLAnchor())->wsmGlobals->wsmSm.smLocked = 0;
#endif
#ifdef __EPOC_OS__
    (mgrGetSyncMLAnchor())->wsmGlobals->wsmSm = NULL;
#endif
  }
}
#define freeDataStructs() smlLibFree((mgrGetSyncMLAnchor())->wsmGlobals)



/* private functions prototypes */
static Short_t getNextFreeEntry();
static Short_t deleteBufferHandle(MemHandle_t memH);
static Short_t resetBufferGlobals(MemHandle_t memH);
static Byte_t isMemAvailable(MemSize_t memToAlloc);




/*************************************************************************/
/*  Internal Functions                                                   */
/*************************************************************************/



 
/**
 * Delete memory handle from buffer.
 * Return -1, if handle not found.
 */
static Short_t deleteBufferHandle(MemHandle_t memH) {
  // reset the values
  wsmBuf[0].memH = WSM_MEMH_UNUSED;
  wsmBuf[0].pFirstFree = NULL;
  wsmBuf[0].pFirstData = NULL;
  wsmBuf[0].size      = 0;
  wsmBuf[0].usedBytes = 0;
  wsmBuf[0].flags     = ~WSM_VALID_F;
  smlLibFree(wsmBuf[0].bufName);   // free mem
  wsmBuf[0].bufName   = NULL;

  return 0;
}


/**
 * Reset values in buffer table for entry memH.
 * If memH doesn't exist create an entry.
 * Return index to memH in buffer table,
 * or -1 if table is full
 */
static Short_t resetBufferGlobals(MemHandle_t memH) {
  if ( (wsmBuf[0].memH != memH) && (wsmBuf[0].memH == WSM_MEMH_UNUSED)) {
    // create new one
    wsmBuf[0].memH = memH;
  } else {
    // use existing one, which has to be reset prior usage
    smlLibFree(wsmBuf[0].bufName);   // free mem
  }
  // reset the values
  wsmBuf[0].pFirstFree = NULL;
  wsmBuf[0].pFirstData = NULL;
  wsmBuf[0].size      = 0;
  wsmBuf[0].usedBytes = 0;
  wsmBuf[0].flags     = WSM_VALID_F;
  wsmBuf[0].bufName   = NULL;

  return 0;
}


static Byte_t isMemAvailable(MemSize_t memToAlloc) {
  MemSize_t actMem = memToAlloc;
  if ( maxWsmAvailMem == 0 )
    return 1;  // no memsize restrictions
  if ( wsmBuf[0].memH != WSM_MEMH_UNUSED )
    actMem += wsmBuf[0].size;
  return (actMem <= maxWsmAvailMem);
}



/*************************************************************************/
/*  External Functions                                                   */
/*************************************************************************/



/**
 * FUNCTION: wsmInit
 *
 * Initializes all Workspace Manager related resources.<BR>
 * Should only be called once!
 *
 * PRE-Condition:   This is the first function call to WSM
 *
 * POST-Condition:  All WSM resources are initialized
 *
 * IN:      wsmOpts
 *          WSM options, valid options are:
 *          <UL>
 *          <LI> maxAvailMem<BR>
 *               Maximal amount of memory which wsm can use for the buffers<BR>
 *               0 == no limitation
 *          </UL>
 * 
 * OUT:     wsmH
 *          Handle to new buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_OPTIONS, if wsmOpts is not valid
 *          SML_ERR_NOT_ENOUGH_SPACE, if not enough available memory
 *          SML_ERR_WRONG_USAGE, if wsmInit was already called
 */
Ret_t wsmInit (const WsmOptions_t *wsmOpts) {
  // create global datastructs
  createDataStructs();

  if (NULL == mgrGetSyncMLAnchor()->wsmGlobals) {
	  return SML_ERR_NOT_ENOUGH_SPACE;
   }


  // check if init was already called
  if ( initWasCalled )
    return SML_ERR_WRONG_USAGE;

  // check options
  if ( wsmOpts != NULL ) {
    if ( wsmOpts->maxAvailMem > 0 ) {
      maxWsmAvailMem = wsmOpts->maxAvailMem;
    }
  }

  // init resources
  wsmBuf[0].memH = WSM_MEMH_UNUSED;
  wsmIndex = 0;
  initWasCalled = (Byte_t) 1;

  return wsmRet=SML_ERR_OK;
} 


/**
 * FUNCTION: wsmCreate
 *
 * Creates and opens a new buffer with name bufName and size bufSize.<BR> 
 * If a buffer with name bufName already exists, the existing buffer 
 * is resized to bufSize.
 *
 * PRE-Condition:   bufSize > 0
 *
 * POST-Condition:  handle refers to buffer bufName; BufferSize = size
 *
 * IN:      bufName
 *          Name of buffer to be created
 * IN:      bufSize
 *          Size of buffer to be created
 * 
 * OUT:     wsmH
 *          Handle to new buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_SIZE, if bufSize <= 0
 *          SML_ERR_NOT_ENOUGH_SPACE, if available memory < bufSize
 *          SML_ERR_WSM_BUF_TABLE_FULL, if buffer table is full
 *          SML_ERR_WRONG_USAGE, if wsmInit wasn't called before
 *
 * @see  wsmDestroy
 */
Ret_t wsmCreate (String_t bufName, MemSize_t bufSize, MemHandle_t *wsmH) {

  *wsmH = 0;    // 0 in case of error

  if ( ! initWasCalled )
    return SML_ERR_WRONG_USAGE;

  // check buffer space
  if ( wsmBuf[0].memH != WSM_MEMH_UNUSED ) {
    return wsmRet=SML_ERR_WSM_BUF_TABLE_FULL;
  }
  // check for maxMemAvailable
  if ( ! isMemAvailable(bufSize) ) {
    return SML_ERR_NOT_ENOUGH_SPACE;
  }

  // create buffer
  if ( (wsmRet = smCreate(bufName, bufSize, wsmH)) != SML_ERR_OK ) {
    if ( wsmRet == SML_ERR_WRONG_USAGE ) {    // buffer already exists
      // resize existing buffer
      // open buffer
      if ( (wsmRet = smOpen(bufName, wsmH)) != SML_ERR_OK ) {
	return wsmRet=SML_ERR_NOT_ENOUGH_SPACE;
      }
      // resize buffer
      if ( (wsmRet = smSetSize(*wsmH, bufSize)) != SML_ERR_OK ) { 
	return wsmRet=SML_ERR_NOT_ENOUGH_SPACE;
      }
    }
    else {
      return wsmRet;
    }
  }

  // reset buffer vars
  resetBufferGlobals(*wsmH);

  // set buffer vars
  wsmBuf[0].size = bufSize;
  wsmBuf[0].bufName = smlLibStrdup(bufName);

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmOpen
 *
 * Open existing buffer with name bufName.
 *
 * PRE-Condition:   WSM knows bufName
 *
 * POST-Condition:  wsmH refers to buffer bufName
 *
 * IN:      bufName
 *          Name of buffer to be opened
 * 
 * OUT:     wsmH
 *          Handle to new buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_WRONG_PARAM, if bufName is unknown
 *
 * @see  wsmClose
 */
Ret_t wsmOpen (String_t bufName, MemHandle_t *wsmH){

  // open buffer
  if ( (wsmRet = smOpen(bufName, wsmH)) != SML_ERR_OK ) {
    return wsmRet;
  }

  // reset buffer vars
  resetBufferGlobals(*wsmH);

  // set buf vars
  wsmRet = smGetSize(*wsmH, &wsmBuf[0].size);
  wsmBuf[0].bufName = smlLibStrdup(bufName);

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmClose
 *
 * Close an open buffer.
 *
 * PRE-Condition:   handle is valid; handle is unlocked
 *
 * POST-Condition:  handle is not known to WSM any more
 *
 * IN:      wsmH
 *          Handle to the open buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *          SML_ERR_WRONG_USAGE, if handle was still locked
 *
 * @see  wsmOpen
 */
Ret_t wsmClose (MemHandle_t wsmH) {

  // check if handle is invalid
  // must be buffer 0, as only this one exists
  if ( ! ((wsmBuf[0].memH == wsmH) || (wsmBuf[0].flags & WSM_VALID_F)) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }

  // close handle
  if ( (wsmRet = smClose(wsmH)) != SML_ERR_OK ) {
    return wsmRet;
  }
  wsmRet = deleteBufferHandle(wsmH);

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmDestroy
 *
 * Destroy existing buffer with name bufName.
 *
 * PRE-Condition:   WSM knows bufName; handle is unlocked
 *
 * POST-Condition:  buffer is not known to WSM any more; all resources 
 *                  connected to this buffer are freed
 *
 * IN:      bufName
 *          Name of buffer to be opened
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_PARAM, if bufName is unknown to WSM
 *          SML_ERR_WRONG_USAGE, if handle was still locked
 *
 * @see  wsmCreate
 */
Ret_t wsmDestroy (String_t bufName) {

  // free resources
  if ( (wsmRet = wsmClose(wsmBuf[0].memH)) != SML_ERR_OK ) {
    return wsmRet;
  }

  // free buffer
  if ( (wsmRet = smDestroy(bufName)) != SML_ERR_OK ) {
    return wsmRet;
  }
   
  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmTerminate
 *
 * Terminate WSM; free all buffers and resources.
 *
 * PRE-Condition: all handles must be unlocked
 *
 * POST-Condition: all resources are freed
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_USAGE, if a handle was still locked
 *
 */
Ret_t wsmTerminate () {
  int i;

  // free all WSM resources
  for (i=0; i < MAX_WSM_BUFFERS; ++i) {
    if ( wsmBuf[i].memH != WSM_MEMH_UNUSED )
      if ( wsmDestroy(wsmBuf[i].bufName) == SML_ERR_WRONG_USAGE ) {
	return SML_ERR_WRONG_USAGE;
      }
  }

  // free global DataStructs
  freeDataStructs();

  return SML_ERR_OK;
}


/**
 * FUNCTION: wsmProcessedBytes
 *
 * Tell Workspace Manager the number of bytes already processed.
 *
 * PRE-Condition:   handle is locked; handle is valid;
 *                  noBytes <= wsmGetUsedSize
 *
 * POST-Condition:  noBytes starting at wsmGetPtr() position are deleted; 
 *                  remaining bytes are copied to 
 *                  wsmGetPtr(SML_FIRST_FREE_ITEM) position;
 *                  wsmGetUsedSize -= noBytes; wsmGetFreeSize += noBytes
 *
 * IN:      wsmH
 *          Handle to the open buffer
 * IN:      noBytes
 *          Number of bytes already processed from buffer.
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *          SML_ERR_WRONG_USAGE, if handle was not locked
 *          SML_ERR_INVALID_SIZE, if noBytes > wsmGetUsedSize
 *
 * @see  wsmGetFreeSize
 */
Ret_t wsmProcessedBytes (MemHandle_t wsmH, MemSize_t noBytes) {

  // check if handle is invalid
  // must be buffer 0, as only this one exists
  if ( ! ((wsmBuf[0].memH == wsmH) || (wsmBuf[0].flags & WSM_VALID_F)) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }
  // check if handle is unlocked
  // must be buffer 0, as only this one exists
  if ( ! ((wsmBuf[0].memH == wsmH) || (wsmBuf[0].flags & WSM_LOCKED_F)) ) {
    return wsmRet=SML_ERR_WRONG_USAGE;
  }

  if ( noBytes > wsmBuf[0].usedBytes ) {
    return wsmRet=SML_ERR_INVALID_SIZE;
  }

  // adapt usedSize
  wsmBuf[0].usedBytes -= noBytes;

  // move memory
  // check return ?????
  smlLibMemmove(wsmBuf[0].pFirstData,
	  (wsmBuf[0].pFirstData + noBytes),
	  wsmBuf[0].usedBytes);

  // move pFirstFree
  wsmBuf[0].pFirstFree -= noBytes;

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmLockH
 *
 * Locks handle wsmH and get a pointer to the contents of wsmH. <BR>
 * RequestedPos describes the position in the buffer to which the returned 
 * pointer should point. Valid values are: 
 * <UL>
 *   <LI> SML_FIRST_DATA_ITEM
 *   <LI> SML_FIRST_FREE_ITEM
 * </UL>
 *
 * PRE-Condition:   handle is unlocked; handle is valid
 *
 * POST-Condition:  handle is locked; points to first data item, 
 *                  or first free item.
 *
 * IN:      wsmH
 *          Handle to the open buffer
 * IN:      requestedPos
 *          Requested position of the returned pointer
 *          <UL>
 *            <LI> SML_FIRST_DATA_ITEM : points to first data entry
 *            <LI> SML_FIRST_FREE_ITEM : points to first free entry
 *          </UL>
 * 
 * OUT:     pMem
 *          Pointer to requested memory          
 *          
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *          SML_ERR_WRONG_USAGE, if handle was still locked
 *          SML_ERR_UNSPECIFIC, if requested position is unknown, or lock failed
 *
 * @see  wsmUnlockH
 */
Ret_t wsmLockH (MemHandle_t wsmH, SmlBufPtrPos_t requestedPos,
		MemPtr_t *pMem) {

  // check if handle is invalid
  // must be buffer 0, as only this one exists
  if ( ! ((wsmBuf[0].memH == wsmH) || (wsmBuf[0].flags & WSM_VALID_F)) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }
  // check if handle is locked
  // must be buffer 0, as only this one exists
  if ( ! ((wsmBuf[0].memH == wsmH) || (wsmBuf[0].flags & WSM_LOCKED_F)) ) {
    return wsmRet=SML_ERR_WRONG_USAGE;
  }

  // lock
  if ( (wsmRet = smLock(wsmH, pMem)) != SML_ERR_OK ) {
    return wsmRet=SML_ERR_UNSPECIFIC;
  }

  // set local pointers  
  wsmBuf[0].pFirstData = *pMem;
  wsmBuf[0].pFirstFree = *pMem + wsmBuf[0].usedBytes;
  wsmBuf[0].flags |= WSM_LOCKED_F;

  switch (requestedPos) {
  case SML_FIRST_DATA_ITEM:
    *pMem = wsmBuf[0].pFirstData;
    break;
  case SML_FIRST_FREE_ITEM:
    *pMem = wsmBuf[0].pFirstFree;
    break;
  default:
    return wsmRet=SML_ERR_UNSPECIFIC;
  }

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmGetFreeSize
 *
 * Returns the remaining unused bytes in the buffer.
 *
 * PRE-Condition:   handle is valid
 *
 * POST-Condition:  wsmGetFreeSize = BufferSize - wsmGetUsedSize
 *
 * IN:      wsmH
 *          Handle to the open buffer
 *
 * OUT:     freeSize
 *          Number of bytes which are unused in this buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *
 * @see  wsmGetUsedSize
 * @see  wsmProcessedBytes
 */
Ret_t wsmGetFreeSize(MemHandle_t wsmH, MemSize_t *freeSize) {

  // check if handle is invalid
  // must be buffer 0, as only this one exists
  if ( ! ((wsmBuf[0].memH == wsmH) || (wsmBuf[0].flags & WSM_VALID_F)) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }
  
  *freeSize = wsmBuf[0].size - wsmBuf[0].usedBytes;

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmGetUsedSize
 *
 * Returns the number of bytes used in the buffer.
 *
 * PRE-Condition:   handle is valid
 *
 * POST-Condition:  usedSize = BufferSize - wsmGetFreeSize
 *
 * IN:      wsmH
 *          Handle to the open buffer
 *
 * OUT:     usedSize
 *          Number of bytes which are already used in this buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *
 * @see  wsmGetFreeSize
 * @see  wsmSetUsedSize
 */
Ret_t wsmGetUsedSize(MemHandle_t wsmH, MemSize_t *usedSize) {

  // check if handle is invalid
  // must be buffer 0, as only this one exists
  if ( ! ((wsmBuf[0].memH == wsmH) || (wsmBuf[0].flags & WSM_VALID_F)) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }
  
  *usedSize = wsmBuf[0].usedBytes;

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmUnlockH
 *
 * Unlock handle wsmH. <BR>
 * After this call all pointers to this memory handle are invalid 
 * and should no longer be used.
 *
 * PRE-Condition:   handle is locked; handle is valid
 *
 * POST-Condition:  handle is unlocked
 *
 * OUT:     wsmH
 *          Handle to unlock
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *          SML_ERR_WRONG_USAGE, if handle was not locked
 *          SML_ERR_UNSPECIFIC, unlock failed
 *
 * @see  wsmLockH
 */
Ret_t wsmUnlockH (MemHandle_t wsmH) {

  // check if handle is invalid
  // must be buffer 0, as only this one exists
  if ( ! ((wsmBuf[0].memH == wsmH) || (wsmBuf[0].flags & WSM_VALID_F)) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }
  // check if handle is already unlocked
  // must be buffer 0, as only this one exists
  if ( ! ((wsmBuf[0].memH == wsmH) || (wsmBuf[0].flags & WSM_LOCKED_F)) ) {
    return wsmRet=SML_ERR_WRONG_USAGE;
  }

  // unlock
  if ( (wsmRet = smUnlock(wsmH)) != SML_ERR_OK ) {
    return wsmRet=SML_ERR_UNSPECIFIC;
  }

  // set local pointers  
  wsmBuf[0].pFirstData = NULL;
  wsmBuf[0].pFirstFree = NULL;
  wsmBuf[0].flags &= ~WSM_LOCKED_F;

  return wsmRet=SML_ERR_OK;
}


/**
 * FUNCTION: wsmSetUsedSize
 *
 * Tell Workspace how many data were written into buffer.
 *
 * PRE-Condition:   handle is valid; usedSize <= wsmGetFreeSize; handle is 
 *                  locked
 *
 * POST-Condition:  wsmGetUsedSize += usedSize; wsmGetFreeSize -= usedSize;
 *                  instancePtr += usedSize;
 *
 * IN:      wsmH
 *          Handle to the open buffer
 * IN:      usedSize
 *          Number of bytes which were written into buffer
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_INVALID_HANDLE, if handle was invalid
 *          SML_ERR_INVALID_SIZE, if usedSize <= wsmGetFreeSize
 *
 * @see  wsmGetUsedSize
 */
Ret_t wsmSetUsedSize (MemHandle_t wsmH, MemSize_t usedSize) {

  // check if handle is invalid
  // must be buffer 0, as only this one exists
  if ( ! ((wsmBuf[0].memH == wsmH) || (wsmBuf[0].flags & WSM_VALID_F)) ) {
    return wsmRet=SML_ERR_INVALID_HANDLE;
  }
  // check if handle is unlocked
  // must be buffer 0, as only this one exists
  if ( ! ((wsmBuf[0].memH == wsmH) || (wsmBuf[0].flags & WSM_LOCKED_F)) ) {
    return wsmRet=SML_ERR_WRONG_USAGE;
  }

  // usedSize > freeSize?
  if ( usedSize > 
       (wsmBuf[0].size - wsmBuf[0].usedBytes) ) {
    return wsmRet=SML_ERR_INVALID_SIZE;
  }

  // adapt usedSize
  wsmBuf[0].usedBytes += usedSize;

  // move pFirstFree
  wsmBuf[0].pFirstFree += usedSize;

  return wsmRet=SML_ERR_OK;
}
 
/**
 * FUNCTION: wsmReset
 *
 * Reset the Workspace
 *
 * PRE-Condition:   -
 *
 * POST-Condition:  all data is lost. The FirstFree Position equals 
 * the First Data position
 *
 * IN:      wsmH
 *          Handle to the open buffer
 * 
 * RETURN:  SML_ERR_OK, if O.K.
 *
 */
Ret_t wsmReset (MemHandle_t wsmH) {
  wsmBuf[0].pFirstFree = wsmBuf[0].pFirstFree - wsmBuf[0].usedBytes ;
  wsmBuf[0].pFirstData = wsmBuf[0].pFirstFree;
  wsmBuf[0].usedBytes = 0;

  return SML_ERR_OK;
}


#endif // #ifndef __SML_LITE__

#endif // #idndef NOWSM

