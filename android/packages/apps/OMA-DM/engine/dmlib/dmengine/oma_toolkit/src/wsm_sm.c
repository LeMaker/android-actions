/*************************************************************************/
/* module:          SyncML WorkSpace Manager                             */
/* file:            WSM_SM.c                                             */
/* target system:   MS Windows                                           */
/* target OS:       Windows 98 / NT                                      */
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
 * Storage Management for Workspace Manager API. <BR>
 * MS Windows version.
 *
 * @version  @label
 *
 */

#ifndef NOWSM
// if no WSM, we can leave this one out completely


/*************************************************************************/
/*  Definitions                                                          */
/*************************************************************************/

#include "wsm_sm.h"

#include <stdlib.h>
#include <string.h>
#include "libmem.h"
#include "liblock.h" // for THREADDEBUGPRINTF %%% luz
#include "smldef.h"
#include "smlerr.h"


/* Global Vars */
/* =========== */

/** root of buffer list */

#include "mgr.h"
#define root (mgrGetSyncMLAnchor())->wsmGlobals->wsmSm

/* private functions prototypes */
static Byte_t newListEle(const char *name, smWinList_t **newEle, MemHandle_t *newHandle);
static Byte_t locateEle(const char *eleName, smWinList_t **p);
static Byte_t locateH(MemHandle_t memH, smWinList_t **p);
static void removeEle(const char *eleName);


/*************************************************************************/
/*  Internal Functions                                                   */
/*************************************************************************/

/** create new buffer element and assign name to it
 * return pointer to new element and handle of new element
 */
/* 
 SCTSTK - 16/03/2002 S.H. 2002-04-05 : fixed so that it works even if the sequence of buffer termination
 is not in the reverse order of buffer creation
 */

// luz %%% NOTE: called only from routines which lock the toolkit already,
//               no separate lock required here
static Byte_t newListEle(const char *name, 
  smWinList_t **newEle, 
  MemHandle_t *newHandle
)
{
  smWinList_t *p;
  int i;
  for ( i=0; *newHandle < MAX_WSM_BUFFERS && (mgrGetSyncMLAnchor())->wsmGlobals->wsmBuf[i].memH != -1; ++i ) {};
  if (i == MAX_WSM_BUFFERS) return 0;
  *newHandle=i+1;
 
  if ( ((*newEle) = smlLibMalloc(sizeof(smWinList_t))) == 0 )
    return 0;  // no more memory
  if ( ((*newEle)->memName = smlLibMalloc(strlen(name)+1)) == 0 ){
    smlLibFree(*newEle);
    return 0;  // no more memory
    }
  memcpy((*newEle)->memName, name, strlen(name));
  (*newEle)->memName[strlen(name)] = '\0';
  if ( root == 0 )
    root = *newEle;
  else {
  	p=root;
	  while ( p->next != NULL) p = p->next;
    p->next = *newEle;
  }
  return 1;
}


/**
 * search for buffer with name eleName and return pointer to it in p.
 * return == 0 if not found; 1 if found
 */
// luz %%% NOTE: called only from routines which lock the toolkit already,
//               no separate lock required here
static Byte_t locateEle(const char *eleName, smWinList_t **p) {
  *p = root;
  while ( (*p != NULL) && (strcmp((*p)->memName, eleName) != 0) )  {
    *p = (*p)->next;
  }
  if ( *p == NULL ) 
    return 0;
  else 
    return 1;
}

/**
 * search for buffer with memHandle memH and return pointer to it in p.
 * return == 0 if not found; 1 if found
 */
// luz %%% NOTE: called only from routines which lock the toolkit already,
//               no separate lock required here
static Byte_t locateH(MemHandle_t memH, smWinList_t **p) {
  *p = root;
  while ( (*p != NULL) && ((*p)->memH != memH) )  {
    *p = (*p)->next;
  }
  if ( *p == NULL ) 
    return 0;
  else 
    return 1;
}

/**
 * remove buffer with name eleName from smWinList.
 */
// luz %%% NOTE: called only from routines which lock the toolkit already,
//               no separate lock required here
static void removeEle(const char *eleName) {
  smWinList_t *act, *old;

  old = act = root;
  while ( (act != NULL) && (strcmp(act->memName, eleName) != 0) )  {
    old = act;
    act = act->next;
  }
  if ( act != NULL ) {
    if ( old == act )   // delete first list ele
      root = act->next;
    else
      old->next = act->next;
    smlLibFree(act->memName);
  	smlLibFree(act);
  }
}




/*************************************************************************/
/*  External Functions                                                   */
/*************************************************************************/


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
Ret_t smCreate (String_t memName, MemSize_t memSize, MemHandle_t *memH) {
  smWinList_t *pEle;     // pointer to new buffer

  if ( memSize <= 0 ) {
    return SML_ERR_INVALID_SIZE;
  }
  if ( locateEle(memName, &pEle) ) {
    return SML_ERR_WRONG_USAGE;
  }
  
  // create new element in buffer list
  if ( ! newListEle(memName, &pEle, memH) ) {
    return SML_ERR_NOT_ENOUGH_SPACE;
  }
    
  // create memory
  if ( (pEle->winH=smlLibMalloc(memSize)) == 0 ) {
    smlLibFree(pEle->memName);
 	  smlLibFree(pEle);
    return SML_ERR_NOT_ENOUGH_SPACE;
  }

  // set new values
  pEle->locked = 0;
  pEle->memH = *memH;
  pEle->memSize = memSize;
  pEle->next = NULL;

  return SML_ERR_OK;
}


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
 *          Name of memory block to open<BR>
 *          Windows version: Name is ignored
 * 
 * OUT:     memH
 *          Handle to opened memory block
 *
 * RETURN:  SML_ERR_OK, if O.K.
 *          SML_ERR_WRONG_PARAM, if memName is unknown
 *
 * @see  smClose
 */
Ret_t smOpen (String_t memName, MemHandle_t *memH) {
  smWinList_t *pEle;     // pointer to buffer element

  if ( ! locateEle(memName, &pEle) ) {
    return SML_ERR_WRONG_PARAM;
  }

  *memH = pEle->memH;
  return SML_ERR_OK;
}


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
 *          SML_ERR_WRONG_USAGE, if memH is locked or unknown
 *
 * @see  smOpen
 */
Ret_t smClose (MemHandle_t memH) {
  smWinList_t *pEle;     // pointer to buffer element

  if ( ! locateH(memH, &pEle) ) {
    return SML_ERR_WRONG_USAGE;
  }

  // reset handle  
  smlLibFree(pEle->winH);
  pEle->memH = 0;
  pEle->locked = 0;
  pEle->memSize = 0;

  return SML_ERR_OK;
}


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
Ret_t smDestroy (String_t memName) {
  smWinList_t *pEle;     // pointer to buffer element

  if ( ! locateEle(memName, &pEle) ) {
    return SML_ERR_WRONG_PARAM;
  }
  if ( pEle->locked ) {
    return SML_ERR_WRONG_USAGE;
  }

  // remove memory buffer
  removeEle(memName);

  return SML_ERR_OK;
}


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
Ret_t smLock (MemHandle_t memH, MemPtr_t *pMem) {
  smWinList_t *pEle;     // pointer to buffer element

  if ( ! locateH(memH, &pEle) ) {
    return SML_ERR_WRONG_PARAM;
  }
  if ( pEle->locked ) {
    return SML_ERR_WRONG_USAGE;
  }

  *pMem = (MemPtr_t)pEle->winH;
  pEle->locked = 1;

  return SML_ERR_OK;
}


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
Ret_t smUnlock (MemHandle_t memH) {
  smWinList_t *pEle;     // pointer to buffer element

  if ( ! locateH(memH, &pEle) ) {
    return SML_ERR_WRONG_PARAM;
  }
  if ( ! pEle->locked ) {
    return SML_ERR_WRONG_USAGE;
  }

  pEle->locked = 0;

  return SML_ERR_OK;
}


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
Ret_t smSetSize (MemHandle_t memH, MemSize_t newSize) {
  smWinList_t *pEle;     // pointer to buffer element

  if ( ! locateH(memH, &pEle) ) {
    return SML_ERR_WRONG_PARAM;
  }
  if ( pEle->locked ) {
    return SML_ERR_WRONG_USAGE;
  }
  if ( newSize <= 0 ) {
    return SML_ERR_INVALID_SIZE;
  }

  smlLibFree(pEle->winH);
  if ( (pEle->winH=smlLibMalloc(newSize)) == 0 ) {
    return SML_ERR_NOT_ENOUGH_SPACE;
  }
  pEle->memSize = newSize;

  return SML_ERR_OK;
}


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
Ret_t smGetSize (MemHandle_t memH, MemSize_t *actSize) {
  smWinList_t *pEle;     // pointer to buffer element

  if ( ! locateH(memH, &pEle) ) {
    return SML_ERR_WRONG_PARAM;
  }

  *actSize = pEle->memSize;

  return SML_ERR_OK;
}

#endif // #ifndef NOWSM
