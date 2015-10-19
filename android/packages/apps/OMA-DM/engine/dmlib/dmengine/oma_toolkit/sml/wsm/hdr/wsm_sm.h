/*************************************************************************/
/* module:          SyncML WorkSpace Manager                             */
/*                                                                       */
/* file:            WSM_SM.h                                             */
/* target system:   All                                                  */
/* target OS:       All                                                  */
/*                                                                       */
/* Description                                                           */
/* Storage Management for Workspace Manager API <BR>                     */
/* Encapsulates OS dependent parts of WSM.                               */
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
 * @version  @label
 */

#ifndef _WSM_SM_H
#define _WSM_SM_H

#include <smldef.h>


#ifdef __ANSI_C__
/* sbuffer list */
typedef struct smWinList_s {
  char               *memName;      // name of buffer
  char               *winH;         // reference to memory block
  MemHandle_t         memH;         // handle of memory block
  Byte_t              locked;       // is handle locked?
  MemSize_t           memSize;      // size of memory block    
  struct smWinList_s *next;         // next list item
} smWinList_t;
typedef smWinList_t *WsmSmGlobals_t;
#endif

#ifdef __PALM_OS__
#include <Pilot.h>
/* dynamic buffer array */
typedef struct smPalm_s {
  Handle          smPalmH;          // reference to only memory block
  MemHandle_t     smMemH;           // handle of only memory block
  Byte_t          smLocked;         // is handle locked?
} WsmSmGlobals_t;
#endif

#ifdef __EPOC_OS__
/* sbuffer list */
typedef struct smWinList_s {
  char               *memName;      // name of buffer
  char               *winH;         // reference to memory block
  MemHandle_t         memH;         // handle of memory block
  Byte_t              locked;       // is handle locked?
  MemSize_t           memSize;      // size of memory block    
  struct smWinList_s *next;         // next list item
} smWinList_t;
typedef smWinList_t *WsmSmGlobals_t;
#endif


/**
 * FUNCTION: smCreate
 *
 * Creates a new memory block with name memName and size memSize.
 *
 * PRE-Condition:   OS does not know memName; memSize > 0
 *
 * POST-Condition:  memName exists with size memSize; 
 *                  memH refers to new memory block.
 *
 * IN:      memName
 *          Name of new memory block
 * IN:      memSize
 *          Size of new memory block
 * 
 * OUT:     memH
 *          Handle to new memory block
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_USAGE, if memName is already known to the OS
 *          SML_ERR_INVALID_SIZE, if memSize <= 0
 *          SML_ERR_NOT_ENOUGH_SPACE, if available memory < memSize
 *
 * @see  smDestroy
 */
Ret_t smCreate (String_t memName, MemSize_t memSize, MemHandle_t *memH);


/**
 * FUNCTION: smOpen
 *
 * Open connection to memory block with name memName.
 *
 * PRE-Condition:   OS does know memName
 *
 * POST-Condition:  memH refers to memory block memName
 *
 * IN:      memName
 *          Name of memory block to open
 * 
 * OUT:     memH
 *          Handle to opened memory block
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_PARAM, if memName is unknown
 *
 * @see  smClose
 */
Ret_t smOpen (String_t memName, MemHandle_t *memH);


/**
 * FUNCTION: smClose
 *
 * Close link to memory block.
 *
 * PRE-Condition:   memH is a valid memory block handle; memH is unlocked;
 *                  no pointers to records are in use
 *
 * POST-Condition:  memH is not valid anymore
 *
 * IN:      memH
 *          Handle to close
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_USAGE, if memH is locked
 *
 * @see  smOpen
 */
Ret_t smClose (MemHandle_t memH);


/**
 * FUNCTION: smDestroy
 *
 * Remove memory block memName within OS.
 *
 * PRE-Condition:   memName is a valid memory block name; 
 *                  memory block is not in use (i.e. no handles and 
 *                  pointers to this memory block are in use)
 *
 * POST-Condition:  memName is not a valid memory block name anymore
 *
 * IN:      memName
 *          Name of memory block to remove
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_PARAM, if memName is unknown
 *          SML_ERR_WRONG_USAGE, if memory block is still locked
 *
 * @see  smCreate
 */
Ret_t smDestroy (String_t memName);


/**
 * FUNCTION: smLock
 *
 * Map memory block memH to local address space.
 *
 * PRE-Condition:   memH is a valid handle; memory block is not locked
 *
 * POST-Condition:  pMem points to memory block memH; 
 *                  memory block is locked
 *
 * IN:      memH
 *          Handle to memory block
 * 
 * OUT:     pMem
 *          Pointer to memory block memH mapped in local address space
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_PARAM, if memH is unknown
 *          SML_ERR_WRONG_USAGE, if memH was already locked
 *          SML_ERR_UNSPECIFIC, if lock failed
 *
 * @see  smUnlock
 */
Ret_t smLock (MemHandle_t memH, MemPtr_t *pMem);


/**
 * FUNCTION: smUnlock
 *
 * Free pointer mapped to memH memory block.
 *
 * PRE-Condition:   memH is a valid handle; memory block is locked
 *
 * POST-Condition:  memory block is unlocked
 *
 * IN:      memH
 *          Handle to memory block
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_PARAM, if memH is unknown
 *          SML_ERR_WRONG_USAGE, if memH was already unlocked
 *          SML_ERR_UNSPECIFIC, if unlock failed
 *
 * @see  smLock
 */
Ret_t smUnlock (MemHandle_t memH);


/**
 * FUNCTION: smSetSize
 *
 * Set size of memory block memH to newSize.
 *
 * PRE-Condition:   memH is a valid handle; newSize > 0; 
 *                  memory block is unlocked
 *
 * POST-Condition:  memory block size = newSize
 *
 * IN:      memH
 *          Handle to memory block
 * IN:      newSize
 *          New size of memory block
 * 
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_PARAM, if memH is unknown
 *          SML_ERR_WRONG_USAGE, if memH is locked
 *          SML_ERR_INVALID_SIZE, if newSize <= 0
 *          SML_ERR_NOT_ENOUGH_SPACE, if available memory < newSize
 *
 * @see  smGetSize
 */
Ret_t smSetSize (MemHandle_t memH, MemSize_t newSize);


/**
 * FUNCTION: smGetSize
 *
 * Get size of memory block memH.
 *
 * PRE-Condition:   memH is a valid handle
 *
 * POST-Condition:  actSize = memory block size
 *
 * IN:      memH
 *          Handle to memory block
 * 
 * OUT:     actSize
 *          Actual size of memory block
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_PARAM, if memH is unknown
 *
 * @see  smSetSize
 */
Ret_t smGetSize (MemHandle_t memH, MemSize_t *actSize);

#endif

