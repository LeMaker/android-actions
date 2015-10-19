/*************************************************************************/
/* module:          Communication Services, base64 encoding/decoding fns.*/
/* file:            /src/xpt/all/xpt-b64.h                               */
/* target system:   all                                                  */
/* target OS:       all                                                  */
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
 * function prototypes and return codes
 * for base64 encoding/ decoding functions.
 *
 */

#ifndef XPT_B64_H
#define XPT_B64_H

#include "xpttypes.h"

/**
 * FUNCTION: base64GetSize
 *
 *  Precalculates the size of an encoded document with the given size
 *
 * PRE-Condition:
 *  The function is called to get the size of the document that
 *  will be encoded with the base64Encode() service.
 *
 * POST-Condition:
 *
 * IN: cbRealDataSize, the size of the non-encoded document.
 *
 * RETURN: BufferSize_t, the size of the encoded document that will be
 *     generated using the base64Encode() service.
 *
 */

BufferSize_t base64GetSize (BufferSize_t cbRealDataSize);

/**
 * FUNCTION: base64Encode
 *
 *  Encodes a chunk of data according to the base64 encoding rules
 *
 * PRE-Condition:
 *  A chunk of data os copied to the source data buffer pbData, and the
 *  length of the data chunk is specified in *pcbDataLength;
 *
 * POST-Condition:
 *  A block of encoded data is available in the specified target buffer.
 *  The length of the encoded data is returned by the function.
 *
 *
 * IN: pbTarget, pointer to an allocated chunk of memory that receives the
 *               encoded data block.
 *     cbTargetSize, size of the data buffer above.
 *     bLast, flag that indicates if the specified block is the last
 *            part of the document. If the value is 0, the funciton expects
 *            that other blocks will follow, a value of 1 indicates that
 *            the data block that is presented in the input buffer is the
 *            last data block to be encoded.
 *     pbSaveBytes, pointer to a block of at least 3 Bytes. When this function
 *            is invoked the first time, the first byte of this buffer MUST
 *            be set to 0.
 * IN/OUT:
 *     pbData, pointer to a data block that contains the clear data that
 *             are to be encoded. On return, the remaining piece of the
 *             input data block that could not be encoded is copied to
 *             the memory that pbData points at.
 *     pcbDataLength, pointer to a variable that denotes the length of
 *             the data block that is to be encoded, The function updates
 *             this value with the size of the data block that could not
 *             be processed. If all data were able to be encoded, the
 *             value will be 0.
 *     pcbOffset, pointer to a variable that is internally used by the
 *             function. before the first call of base64encode() for a
 *             certain document is made, the variable that pcbOffset points
 *             at must be set to 0. The variable will be updated by the
 *             function, and should not be modified by the caller.
 * RETURN: BufferSize_t, the size of the data block that are available in
 *     pbTarget.
 *
 */

BufferSize_t base64Encode (DataBuffer_t pbTarget,
                     BufferSize_t cbTargetSize,
                     DataBuffer_t pbData,
                     BufferSize_t *pcbDataLength,
                     BufferSize_t *pcbOffset,
                     unsigned int bLast,
                     unsigned char *pbSavebytes);
/**
 * FUNCTION: base64Decode
 *
 *  Decodes a chunk of data according to the base64 encoding rules
 *
 * PRE-Condition:
 *  A chunk of data os copied to the source data buffer pbData, and the
 *  length of the data chunk is specified in *pcbDataLength;
 *
 * POST-Condition:
 *  A block of decoded data is available in the specified target buffer.
 *  The length of the decoded data is returned by the function.
 *
 *
 * IN: pbTarget, pointer to an allocated chunk of memory that receives the
 *               decoded data block.
 *     cbTargetSize, size of the data buffer above.
 * IN/OUT:
 *     pbData, pointer to a data block that contains the clear data that
 *             are to be decoded. On return, the remaining piece of the
 *             input data block that could not be decoded is copied to
 *             the memory that pbData points at.
 *     pcbDataLength, pointer to a variable that denotes the length of
 *             the data block that is to be decoded, The function updates
 *             this value with the size of the data block that could not
 *             be processed. If all data were able to be decoded, the
 *             value will be 0.
 * RETURN: BufferSize_t, the size of the data block that are available in
 *             pbTarget. If some invalid data were detected in the input
 *             data buffer, or if cbTargetSize is less than 3,
 *             the function returns 0. The caller should treat this as an
 *             error condition.
 *
 */

BufferSize_t base64Decode (DataBuffer_t pbTarget,
                     BufferSize_t cbTargetSize,
                     DataBuffer_t pbData,
                     BufferSize_t *pcbDataLength);


#endif
