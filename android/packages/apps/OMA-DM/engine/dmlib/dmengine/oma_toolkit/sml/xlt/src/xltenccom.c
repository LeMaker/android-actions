/*************************************************************************/
/* module:          Encoder utils file                                   */
/* file:            xltenccom.c                                          */
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

/*************************************************************************/
/*  Definitions                                                          */
/*************************************************************************/


#include "xltenccom.h"
#include <smlerr.h>
#include <libmem.h>

/**
 * FUNCTION: xltAddToBuffer
 *
 * Add a string to the global buffer
 *
 * PRE-Condition:  pContent contains some content bytes to write to the (WB) XML buffer
 *
 * POST-Condition: content is written to the buffer 
 *
 * IN:             pContent, the character pointer referencing the content to
 *                           write to the buffer
 *                 size, the content length
 * 
 * IN/OUT:         pBufMgr, pointer to a structure containing buffer management elements
 * 
 * RETURN:         shows error codes of function, 
 *                 0, if OK
 */
Ret_t xltAddToBuffer(const MemPtr_t pContent, MemSize_t size, BufferMgmtPtr_t pBufMgr)
{
  // if we are doing a space evaluation, do not write the data physically - just remember its length
  if (!pBufMgr->spaceEvaluation) {
    //check if buffersize is to small to write the content
  	if ((size + pBufMgr->smlXltWrittenBytes) > pBufMgr->smlXltBufferLen) {
  	  #ifdef NCDEBUGPRINTFX
  	  #warning "%%%%% delete that message later"
  	  NCDEBUGPRINTFX(DBG_SYNCML,(
  	    "xltAddToBuffer: buffer too small, pContent='%0.30s', size=%ld, pBufMgr->smlXltWrittenBytes=%ld, pBufMgr->smlXltBufferLen=%ld",
  	    pContent,
  	    size,
  	    pBufMgr->smlXltWrittenBytes,
  	    pBufMgr->smlXltBufferLen
  	  ));
  	  #endif
      return SML_ERR_XLT_BUF_ERR;
  	}
    
    if (!(smlLibMemcpy((void*) pBufMgr->smlXltBufferP, (void*) pContent, (MemSize_t) size))) {
  	  #ifdef NCDEBUGPRINTFX
  	  #warning "%%%%% delete that message later"
  	  NCDEBUGPRINTFX(DBG_SYNCML,(
  	    "xltAddToBuffer: memCpy failed, pBufMgr->smlXltBufferP=%lX, size=%ld",
  	    (long)pBufMgr->smlXltBufferP,
  	    size
  	  ));
  	  #endif
      return SML_ERR_XLT_BUF_ERR;
    }
    pBufMgr->smlXltBufferP += size;
  }
  pBufMgr->smlXltWrittenBytes += size;

  return SML_ERR_OK;
}
