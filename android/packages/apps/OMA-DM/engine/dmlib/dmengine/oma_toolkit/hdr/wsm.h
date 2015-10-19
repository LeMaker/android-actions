/*************************************************************************/
/* module:          SyncML WorkSpace Manager                             */
/* file:            WSM.h                                                */
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
 */

#ifndef _WSM_H
#define _WSM_H

#include "smlerr.h"
#include "smldef.h"

#ifndef NOWSM

#include "wsm_sm.h"


typedef struct WsmOptions_s {
  MemSize_t maxAvailMem;   // maximum amount of memory available for all wsm buffers
} WsmOptions_t;


#ifdef __SML_LITE__
  #ifdef MAX_WSM_BUFFERS
    #error "for __SML_LITE__, MAX_WSM_BUFFERS must not be predefined!" 
  #endif
  #define MAX_WSM_BUFFERS 1
#else
  #ifndef MAX_WSM_BUFFERS
    // use default value of 4 (not much for a multi-connection server)
    #define MAX_WSM_BUFFERS 4
  #endif
#endif


/** WSM internal buffer structure */
typedef struct WsmBuf_s {
  String_t    bufName;     // external name of buffer
  MemHandle_t memH;        // memory handle
  MemPtr_t    pFirstFree;  // pointer to first free element in buffer
  MemPtr_t    pFirstData;  // pointer to first data element in buffer
  MemSize_t   size;        // size of buffer
  MemSize_t   usedBytes;   // used bytes in buffer
  Byte_t      flags;
} WsmBuf_t;


/** WSM globals for use with global Anchor */
typedef struct WsmGlobals_s {
  Ret_t           wsmRet;          // last WSM return code
  Byte_t          initWasCalled;   // was wsmInit() called?
  WsmBuf_t        wsmBuf[MAX_WSM_BUFFERS];   
  Short_t         wsmIndex;        // Index of actual buffer
  WsmSmGlobals_t  wsmSm;           // WSM_SM global; device dependent!
} *WsmGlobalsPtr_t, WsmGlobals_t;


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
 *          <LI> tbd
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
Ret_t wsmInit (const WsmOptions_t *wsmOpts);


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
Ret_t wsmCreate (String_t bufName, MemSize_t bufSize, MemHandle_t *wsmH);


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
Ret_t wsmOpen (String_t bufName, MemHandle_t *wsmH);

 
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
Ret_t wsmClose (MemHandle_t wsmH);


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
Ret_t wsmDestroy (String_t bufName);


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
Ret_t wsmTerminate (void);

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
Ret_t wsmProcessedBytes (MemHandle_t wsmH, MemSize_t noBytes);


/**
 * FUNCTION: wsmLockH
 *
 * Locks handle wsmH and get a pointer to the contents of wsmH. <BR>
 * RequestedPos describes the position in the buffer to which the returned 
 * pointer should point to. Valid values are: 
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
		MemPtr_t *pMem);


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
Ret_t wsmGetFreeSize(MemHandle_t wsmH, MemSize_t *freeSize);


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
Ret_t wsmGetUsedSize(MemHandle_t wsmH, MemSize_t *usedSize);


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
Ret_t wsmUnlockH (MemHandle_t wsmH);


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
Ret_t wsmSetUsedSize (MemHandle_t wsmH, MemSize_t usedSize);
 
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
Ret_t wsmReset (MemHandle_t wsmH) ;

#endif // !defined(NOWSM)


#endif
