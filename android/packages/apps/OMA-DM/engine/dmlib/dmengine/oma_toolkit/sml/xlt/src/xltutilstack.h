/*************************************************************************/
/* module:         XLT Decoder Util                                      */
/* file:           XLTUtilStack.h                                        */
/* target system:  all                                                   */
/* target OS:      all                                                   */
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
 * Header file for a simple stack implementation used by the WBXML scanner
 * and the SyncML parser.
 */

/*************************************************************************/
/* Definitions                                                           */
/*************************************************************************/
#ifndef _XLT_UTIL_STACK_H
#define _XLT_UTIL_STACK_H

#include "xlttags.h"
#include <smldef.h>

/* type for stack elements */
typedef XltTagID_t XltUtilStackItem_t;

/**
 * XLTUtilStack interface
 *
 * Like the WBXML/XML scanner, this stack implementation tries to emulate
 * an object-oriented interface. It consist of one stack structure that
 * contains the public methods and attributes and another private stack
 * structure that is not visible to the users of the stack. A stack object
 * has the following public methods:
 *
 * FUNCTION: top
 *
 * Returns the top element of the stack.
 *
 * PRE-Condition:
 *                 the stack contains at least one element
 *
 * POST-Condition:
 *
 * IN:             the stack
 *
 * OUT:            the top element of the stack
 *
 * IN/OUT:
 *
 * RETURNS:        SML_ERR_WRONG_USAGE, if the stack is empty
 *                 SML_ERR_OK, else
 *
 * FUNCTION: pop
 *
 * Returns the top element and takes it off the stack.
 *
 * PRE-Condition:
*                 the stack contains at least one element
*
* POST-Condition:
*                 the top element of the stack is removed
*
* IN:
*
* OUT:            the top element of the stack
*
* IN/OUT:         the stack
*
* RETURNS:        SML_ERR_WRONG_USAGE, if the stack is empty
*                 SML_ERR_NOT_ENOUGH_SPACE, if memory reallocation failed
*                 SML_ERR_OK, else
*
* FUNCTION: push
*
* Put a new element on top of the stack.
*
* PRE-Condition:
*
* POST-Condition:
*                 popping the stack yields the same stack as before
*                 the push
*
* IN:             the new stack element
*
* OUT:
*
* IN/OUT:         the stack
*
* RETURNS:        SML_ERR_NOT_ENOUGH_SPACE, if memory reallocation failed
*                 SML_ERR_OK, else
*
* FUNCTION: destroy
*
* Free the memory used by the stack.
*
* PRE-Condition:
* POST-Condition:
*
* IN:
*
* OUT:
*
* IN/OUT:         the stack
*
* RETURNS:        SML_ERR_OK
*/
typedef struct XltUtilStack_s *XltUtilStackPtr_t, XltUtilStack_t;
struct XltUtilStack_s
{
        Ret_t (*top)(const XltUtilStackPtr_t, XltUtilStackItem_t *);
        Ret_t (*pop)(XltUtilStackPtr_t, XltUtilStackItem_t *);
        Ret_t (*push)(XltUtilStackPtr_t, const XltUtilStackItem_t);
        Ret_t (*destroy)(XltUtilStackPtr_t);
};

/**
 * FUNCTION: xltUtilCreateStack
 *
 * Creates a new stack. The size parameter indicates for how many elements
 * memory should be allocated initially. This does _not_ mean that you can
 * not push more than that many element onto the stack - in that case
 * memory for another size elements is allocated.
 *
 * PRE-Condition:
 * POST-Condition:
 *                 the stack pointer points to a new, empty stack
 *
 * IN:             size, the initial size of the stack
 *
 * OUT:            a new stack
 *
 * IN/OUT:
 *
 * RETURNS:        SML_ERR_NOT_ENOUGH_SPACE, if memory allocation failed
 *                 SML_ERR_OK, else
 */
Ret_t xltUtilCreateStack(XltUtilStackPtr_t *, const Long_t size);

#endif
