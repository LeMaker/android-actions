/*************************************************************************/
/* module:         Interface for the XLT Decoder component.              */
/* file:           XLTDec.h                                              */
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
 * Interface for the WBXML and XML decoder component.
 */

/*************************************************************************/
/* Definitions                                                           */
/*************************************************************************/
#ifndef _XLT_DEC_H
#define _XLT_DEC_H

#include <smldef.h>
#include <smldtd.h>
#include <smlerr.h>

#include "xltdeccom.h"
#include "xltutilstack.h"

#ifdef _cplusplus
extern "C" {
#endif

/**
 * The XLT Decoder Interface consists of a single XltDecoder "object"
 * (struct) and an creation/initialization function. The XltDecoder
 * object contains all "public" methods and data structures. The first
 * parameter for any public method is the object of which the method is
 * called.
 * 
 * The decoder's public methods/attributes are:
 * 
 * ATTRIBUTE: charset
 *
 * Character set used in the document - this is the MIBEnum value assigned
 * by the IANA for the character encoding, e.g. "3" for US-ASCII.
 *
 * ATTRIBUTE: charsetStr
 * 
 * Name of the character set, e.g. "US-ASCII" - valid only when charset == 0.
 *
 * ATTRIBUTE: finished
 *
 * Indicates whether the decoder has reached the end of the buffer during
 * the last call to xltDecNext.
 *
 * ATTRIBUTE: scanner
 *
 * Pointer to the scanner status object used by this decoder. The scanner
 * will be created during the initialization of the decoder as either a XML
 * or WBXML scanner.
 *
 * 
 * ATTRIBUTE: tagstack
 *
 * The decoder uses an internal stack to check that for every start tag
 * there is a corresponding end tag.

 */
typedef struct XltDecoder_s
{
    Long_t charset;
    String_t charsetStr;
    Flag_t finished;
    Boolean_t final;
    XltDecScannerPtr_t scanner;
    XltUtilStackPtr_t tagstack;
#ifdef __USE_DMTND__
    SmlEncoding_t smlEncoding;
    SmlEncoding_t tndsEncoding;
#endif

} XltDecoder_t, *XltDecoderPtr_t;

/**
 * FUNCTION: xltDecInit
 *
 * Initializes a new decoder object. This function allocates memory for the
 * decoder structure which has to be freed by a call to the decoder's
 * terminate method when the decoder is not needed anymore.  As part of the
 * initialization the decoder begins decoding the SyncML document to find
 * the SyncHdr element.
 *
 * PRE-Condition:
 *                 ppDecoder is NULL
 *                 ppBufPos
 *
 * POST-Condition:
 *                 ppDecoder points to an initialized decoder status object
 *
 * IN:             enc, the document encoding (WBXML or XML)
 *                 pBufEnd, pointer to the end of the buffer which contains
 *                     the document
 *
 * IN/OUT:         ppBufPos, pointer to the current position within the
 *                     buffer
 *
 * OUT:            ppDecoder, the decoder status object
 *                 ppSyncHdr, the SyncHdr element
 *
 * RETURNS:        SML_ERR_OK, if the decoder could be created and the
 *                             SmlSyncHdr was found
 *                             else error code
 */
Ret_t xltDecInit(const SmlEncoding_t enc,
        const MemPtr_t pBufEnd,
        MemPtr_t *ppBufPos,
        XltDecoderPtr_t *ppDecoder,
        SmlSyncHdrPtr_t *ppSyncHdr);

/**
 * FUNCTION: xltDecNext
 * 
 * Decodes the next protocol element of the given SyncML document. This
 * function creates the data structures detailed in the SMLDtd header file.
 * It is the responsibility of the SyncML client application to free the
 * allocated memory after it is done processing the data.
 * This function sets the decoder's finished flag if no protocol element was
 * found. In that case pPE is set to SML_PE_UNDEF and pContent is NULL.
 *
 * PRE-Condition:
 *                 pDecoder points to a decoder status object initialized
 *                 by xltDecInit
 * 
 * POST-Condition:
 *                 pPE and pContent describe the next valid protocol
 *                 element within the SyncML document OR
 *                 the finished flag of the decoder status object is set
 * 
 * IN:             pBufEnd, pointer to the end of the buffer
 *
 * IN/OUT:         pDecoder, the decoder status object
 *                 ppBufPos, pointer to the current position within the
 *                     buffer before and after the call to xltDecNext
 *
 * OUT:            pPE, the type of the protocol element (e.g. SML_PE_ADD)
 *                 pContent, the data structure for the p.e. cast
 *                     (e.g. AddPtr_t) to a void pointer
 * 
 * RETURN:         SML_ERR_OK, if a valid protocol element was found or if
 *                 decoder reached the end of the buffer
 *                 else error code showing where the parsing failed
 */
Ret_t xltDecNext(XltDecoderPtr_t pDecoder,
        const MemPtr_t pBufEnd,
        MemPtr_t *ppBufPos,
        SmlProtoElement_t *pPE,
        VoidPtr_t *pContent);

/**
 * FUNCTION: xltDecTerminate
 *
 * Frees the memory allocated by the decoder.
 *
 * PRE-Condition:
 *                 pDecoder points to a decoder status object initialized
 *                 by xltDecInit
 *
 * POST-Condition:
 *                 all memory allocated by the decoder status object is
 *                 freed
 *
 * IN:             pDecoder, the decoder
 *
 * RETURN:         SML_ERR_OK, if the memory could be freed
 *                 else error code
 */
Ret_t xltDecTerminate(XltDecoderPtr_t pDecoder);


Ret_t xltDecReset(XltDecoderPtr_t pDecoder);

/* T.K. moved here from xltdec.c for use in sub-DTD parsing */
#define IS_START(tok) ((tok)->type == TOK_TAG_START)
#define IS_END(tok) ((tok)->type == TOK_TAG_END)
#define IS_EMPTY(tok) ((tok)->type == TOK_TAG_EMPTY)
#define IS_TAG(tok) (IS_START(tok) || IS_EMPTY(tok) || IS_END(tok))
#define IS_START_OR_EMPTY(tok) (IS_START(tok) || IS_EMPTY(tok))
#define IS_CONTENT(tok) ((tok)->type == TOK_CONT)
/**
 * nextToken and discardToken are just wrappers around the scanner's
 * nextTok and pushTok methods that do some error checking.
 */
Ret_t nextToken(XltDecoderPtr_t pDecoder);
Ret_t discardToken(XltDecoderPtr_t pDecoder);
/* eof xltdec.c stuff */

#ifdef __SML_WBXML__
/*Added by w21034 begin*/
Ret_t wbxml2xmlInternal(unsigned char *bufIn, int bufInLen, unsigned char *bufOut, int * bufOutLen);
/*Added by w21034 end*/
#endif


#ifdef _cplusplus
}
#endif

#endif
