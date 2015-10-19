/*************************************************************************/
/* module:         SyncmML Decoder                                       */
/* file:           XLTDec.c                                              */
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
 * The SyncML parser.
 */

/*************************************************************************/
/* Definitions                                                           */
/*************************************************************************/
#include "xltdec.h"
#include "xltdeccom.h"
#include "xlttags.h"
#include "xltutilstack.h"
#include "xlttagtbl.h"
#include "xltmetinf.h"
#include "xltdevinf.h"
#include "xltdmtnd.h"

#include <smldef.h>
#include <smldtd.h>
#include <smlmetinfdtd.h>
#include <smldevinfdtd.h>
#include <smldmtnddtd.h>
#include <smlerr.h>

#include <libmem.h>
#include <libstr.h>
#include <mgrutil.h>

#ifdef __USE_EXTENSIONS__
/* prototype for function in xltdecwbxml.c */
void subdtdDecodeWbxml(XltDecoderPtr_t pDecoder,SmlPcdataPtr_t *ppPcdata);
#endif

/**
 * FUNCTION: concatPCData
 *
 * Tries to concatenate two Pcdata elements. Only works when the two
 * elements are of the same type (e.g. SML_PCDATA_STRING). Returns a
 * pointer to the new Pcdata element or NULL if concatenation failed.
 */
static SmlPcdataPtr_t concatPCData(SmlPcdataPtr_t pDat1, const SmlPcdataPtr_t pDat2);


/**
 * FUNCTION: appendXXXList
 *
 * These are auxiliary functions for building SyncML elements that contain
 * lists of certain other data structures (e.g. Items). They take an
 * existing list (e.g. of type ItemListPtr_t) and append an appropriate
 * element at the end. If the ListPtr points to NULL a new list is created.
 *
 * PRE-Condition:
 *                 The scanner's current token is the start tag (may be
 *                 empty) of the SyncML element to be appended to the list.
 *
 * POST-Condition:
 *                 The scanner's current token is the end tag (or empty
 *                 start tag) of the SyncML element that was added to the
 *                 list.
 *
 * IN/OUT:         pDecoder, the decoder
 *                 ppXXXList, NULL or an initialized list, to which element
 *                 will be appended
 *
 * RETURNS:        SML_ERR_OK, if an element was successfully appended
 *                 else error code
 */
static Ret_t appendItemList(XltDecoderPtr_t pDecoder, SmlItemListPtr_t *ppItemList);
static Ret_t appendSourceList(XltDecoderPtr_t pDecoder, SmlSourceListPtr_t *ppSourceList);
#ifdef MAPITEM_RECEIVE
  static Ret_t appendMapItemList(XltDecoderPtr_t pDecoder, SmlMapItemListPtr_t *ppMapItemList);
#endif
static Ret_t appendTargetRefList(XltDecoderPtr_t pDecoder, SmlTargetRefListPtr_t *ppTargetRefList);
static Ret_t appendSourceRefList(XltDecoderPtr_t pDecoder, SmlSourceRefListPtr_t *ppSourceRefList);

/* if the commands are not defined we let the functions point to NULL */
#ifndef RESULT_RECEIVE
#define buildResults NULL
#endif

#ifndef MAP_RECEIVE
#define buildMap NULL
#endif

#ifndef EXEC_RECEIVE
#define buildExec NULL
#endif

#if !defined(ATOM_RECEIVE) && !defined(SEQUENCE_RECEIVE)
#define buildAtomOrSeq NULL
#endif

#ifndef SEARCH_RECEIVE
#define buildSearch NULL
#endif


typedef struct PEBuilder_s
{
    XltTagID_t     tagid;
    SmlProtoElement_t type;
    Ret_t (*build)(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem);
} PEBuilder_t, *PEBuilderPtr_t;

PEBuilderPtr_t getPETable(void);

PEBuilderPtr_t getPETable(void)
{ 
  PEBuilderPtr_t _tmpPEPtr;
  PEBuilder_t PE[] = {
    { TN_ADD,       SML_PE_ADD,             buildGenericCmd },
    { TN_ALERT,     SML_PE_ALERT,           buildAlert      },
    { TN_ATOMIC,    SML_PE_ATOMIC_START,    buildAtomOrSeq  },
    { TN_COPY,      SML_PE_COPY,            buildGenericCmd },
    { TN_DELETE,    SML_PE_DELETE,          buildGenericCmd },
    { TN_EXEC,      SML_PE_EXEC,            buildExec       },
    { TN_GET,       SML_PE_GET,             buildPutOrGet   },
    { TN_MAP,       SML_PE_MAP,             buildMap        },
    { TN_PUT,       SML_PE_PUT,             buildPutOrGet   },
    { TN_RESULTS,   SML_PE_RESULTS,         buildResults    },
    { TN_SEARCH,    SML_PE_SEARCH,          buildSearch     },    
    { TN_SEQUENCE,  SML_PE_SEQUENCE_START,  buildAtomOrSeq  },
    { TN_STATUS,    SML_PE_STATUS,          buildStatus     },
    { TN_SYNC,      SML_PE_SYNC_START,      buildSync       },
    { TN_REPLACE,   SML_PE_REPLACE,         buildGenericCmd },
    { TN_UNDEF,     SML_PE_UNDEF,           0               }
  };
    
  _tmpPEPtr = smlLibMalloc(sizeof(PE));
  if (_tmpPEPtr == NULL) return NULL;
  smlLibMemcpy(_tmpPEPtr, &PE, sizeof(PE));
  return _tmpPEPtr; 
}

/*************************************************************************/
/* External Functions                                                    */
/*************************************************************************/
/**
 * Description see XLTDec.h header file.
 */
Ret_t
xltDecInit(const SmlEncoding_t enc,
        const MemPtr_t pBufEnd,
        MemPtr_t *ppBufPos,
        XltDecoderPtr_t *ppDecoder, 
        SmlSyncHdrPtr_t *ppSyncHdr)
{
    XltDecoderPtr_t pDecoder;
    Ret_t rc;


    /* create new decoder object */
    if ((pDecoder = (XltDecoderPtr_t)smlLibMalloc(sizeof(XltDecoder_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    pDecoder->finished = 0;
    pDecoder->final = 0;
    pDecoder->scanner = NULL;
    if ((rc = xltUtilCreateStack(&pDecoder->tagstack, 10)) != SML_ERR_OK) {
        xltDecTerminate(pDecoder);
        return rc;
    }

#ifdef __SML_WBXML__
    if (enc == SML_WBXML) 
    {
        rc = xltDecWbxmlInit(pBufEnd, ppBufPos, &pDecoder->scanner);
        if (rc == SML_ERR_OK) 
        {
            pDecoder->charset = pDecoder->scanner->charset;
            pDecoder->charsetStr = NULL;
        }
    } else
#endif

#ifdef __SML_XML__
    if (enc == SML_XML) 
    {

        rc = xltDecXmlInit(pBufEnd, ppBufPos, &pDecoder->scanner);
        if (rc == SML_ERR_OK) 
        {
            pDecoder->charset = 0;
            pDecoder->charsetStr = pDecoder->scanner->charsetStr;
        }
    } else
#endif

    {
        rc = SML_ERR_XLT_ENC_UNK;
    }

    if (rc != SML_ERR_OK) 
    {
        xltDecTerminate((XltDecoderPtr_t)pDecoder);
        return rc;
    }

    /* try to find SyncHdr element, first comes the SyncML tag... */
    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        xltDecTerminate((XltDecoderPtr_t)pDecoder);
        return rc;
    }
    if (!IS_START(pDecoder->scanner->curtok) ||
            (pDecoder->scanner->curtok->tagid != TN_SYNCML)) {
        smlFreePcdata(pDecoder->scanner->curtok->pcdata);
        xltDecTerminate((XltDecoderPtr_t)pDecoder);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    /* ... then the SyncHdr */
    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        xltDecTerminate((XltDecoderPtr_t)pDecoder);
        return rc;
    }
    if ((rc = buildSyncHdr(pDecoder, (VoidPtr_t)ppSyncHdr)) != SML_ERR_OK) {
        xltDecTerminate((XltDecoderPtr_t)pDecoder);
        return rc;
    }

    *ppBufPos = pDecoder->scanner->getPos(pDecoder->scanner);

#ifdef __DM_TND__
    pDecoder->smlEncoding = enc;
    pDecoder->tndsEncoding = pDecoder->smlEncoding;
#endif
    *ppDecoder = (XltDecoderPtr_t)pDecoder;

    return SML_ERR_OK;
}

/**
 * Description see XLTDec.h header file.
 */
Ret_t
xltDecNext(XltDecoderPtr_t pDecoder,
        const MemPtr_t pBufEnd,
        MemPtr_t *ppBufPos,
        SmlProtoElement_t *pe,
        VoidPtr_t *ppContent)
{
    XltDecoderPtr_t pDecPriv = (XltDecoderPtr_t)pDecoder;
    XltDecScannerPtr_t pScanner = pDecPriv->scanner;
    XltTagID_t tagid;
    Ret_t rc;
    int i;

    pScanner->setBuf(pScanner, *ppBufPos, pBufEnd);

    /* if we are still outside the SyncBody, look for SyncBody start tag */
    if ((rc = pDecPriv->tagstack->top(pDecPriv->tagstack, &tagid)) != SML_ERR_OK)
        return rc;
    if (tagid == TN_SYNCML) {
        if (((rc = nextToken(pDecPriv)) != SML_ERR_OK)) {
            return rc;
        }
        if (!((IS_START(pScanner->curtok)) &&
             (pScanner->curtok->tagid == TN_SYNCBODY))) {
            return SML_ERR_XLT_INVAL_PROTO_ELEM;
        }
    }

    if ((rc = nextToken(pDecPriv)) != SML_ERR_OK)
        return rc;

    /* if we find a SyncML protocol element build the corresponding
       data structure */
    if ((IS_START_OR_EMPTY(pScanner->curtok)) && (pScanner->curtok->tagid != TN_FINAL)) {

        PEBuilderPtr_t pPEs = getPETable();
        if (pPEs == NULL)
        {
          smlLibFree(pPEs);
          return SML_ERR_NOT_ENOUGH_SPACE;
        }
        i = 0;
        while (((pPEs+i)->tagid) != TN_UNDEF)
        { 
            if (((pPEs+i)->tagid) == pScanner->curtok->tagid) 
            {
                *pe = ((pPEs+i)->type);
                if ((rc = (pPEs+i)->build(pDecPriv, ppContent)) != SML_ERR_OK)
                {
                    smlLibFree(pPEs);
                    return rc;
                } 
                /* T.K. adjust the SML_PE_ for 'generic' structures */
                if (*pe == SML_PE_GENERIC) {
                    SmlGenericCmdPtr_t g = *ppContent;
                    switch ((int) ((pPEs+i)->tagid)) {
                    case TN_ADD    : g->elementType = SML_PE_ADD;     break;
                    case TN_COPY   : g->elementType = SML_PE_COPY;    break;
                    case TN_DELETE : g->elementType = SML_PE_DELETE;  break;
                    case TN_REPLACE: g->elementType = SML_PE_REPLACE; break;
                    }
                }
                break;
            }
            i++;
        }
        if (((pPEs+i)->tagid) == TN_UNDEF) 
        {
                *pe = SML_PE_UNDEF;
                *ppContent = NULL;
                smlLibFree(pPEs);
                return SML_ERR_XLT_INVAL_PROTO_ELEM;
        }
        smlLibFree(pPEs);
    } else {

        /* found end tag */
        switch (pScanner->curtok->tagid) {
            case TN_ATOMIC:
                *pe = SML_PE_ATOMIC_END;
                *ppContent = NULL;
                break;
            case TN_SEQUENCE:
                *pe = SML_PE_SEQUENCE_END;
                *ppContent = NULL;
                break;
            case TN_SYNC:
                *pe = SML_PE_SYNC_END;
                *ppContent = NULL;
                break;
            case TN_FINAL:
                *pe = SML_PE_FINAL;
                *ppContent = NULL;
                pDecPriv->final = 1;
                break;
            case TN_SYNCBODY:
                /* next comes the SyncML end tag, then we're done */
                if ((rc = nextToken(pDecPriv)) != SML_ERR_OK)
                    return rc;
                if ((pScanner->curtok->type == TOK_TAG_END) &&
                        (pScanner->curtok->tagid == TN_SYNCML)) {
                    *pe = SML_PE_UNDEF;
                    *ppContent = NULL;
                    pDecPriv->finished = 1;
                } else {
                    return SML_ERR_XLT_INVAL_SYNCML_DOC;
                }
                break;
            default: 
                return SML_ERR_XLT_INVAL_PROTO_ELEM;
        }
    }

    *ppBufPos = pScanner->getPos(pScanner);

    return SML_ERR_OK;
}

/**
 * Description see XLTDec.h header file.
 */
Ret_t
xltDecTerminate(XltDecoderPtr_t pDecoder)
{
    XltDecoderPtr_t pDecPriv;

    if (pDecoder == NULL)
        return SML_ERR_OK;

    pDecPriv = (XltDecoderPtr_t)pDecoder;
    if (pDecPriv->scanner != NULL)
        pDecPriv->scanner->destroy(pDecPriv->scanner);
    if (pDecPriv->tagstack != NULL)
        pDecPriv->tagstack->destroy(pDecPriv->tagstack);
    smlLibFree(pDecPriv);

    return SML_ERR_OK;
}


Ret_t xltDecReset(XltDecoderPtr_t pDecoder)
{
  return xltDecTerminate(pDecoder);
}

/**
 * Gets the next token from the scanner.
 * Checks if the current tag is an end tag and if so, whether the last
 * open start tag has the same tag id as the current end tag. An open start
 * tag is one which matching end tag has not been seen yet.
 * If the current tag is a start tag its tag ID will be pushed onto the
 * tag stack.
 * If the current tag is an empty tag or not a tag at all nothing will be
 * done.
 */
Ret_t
nextToken(XltDecoderPtr_t pDecoder)
{
    XltUtilStackPtr_t pTagStack;
    XltDecTokenPtr_t pToken;
    Ret_t rc;

#ifdef __USE_DMTND__
    // TODO
    if ( pDecoder->smlEncoding != pDecoder->tndsEncoding )
    {
        KCDBG("pDecoder->smlEncoding != pDecoder->tndsEncoding\n");
        return SML_ERR_OK;
    }
#endif

    if ((rc = pDecoder->scanner->nextTok(pDecoder->scanner)) != SML_ERR_OK)
        return rc;

    pToken = pDecoder->scanner->curtok;
    pTagStack = pDecoder->tagstack;

    if (IS_START(pToken)) {
        if (pTagStack->push(pTagStack, pToken->tagid))
            return SML_ERR_UNSPECIFIC;
    } else if (IS_END(pToken)) {
        XltTagID_t lastopen;
        if (pTagStack->pop(pTagStack, &lastopen))
            return SML_ERR_UNSPECIFIC;
        if (pToken->tagid != lastopen)
            return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }
    return SML_ERR_OK;
}

Ret_t discardToken(XltDecoderPtr_t pDecoder)

{
    Ret_t rc;
    XltTagID_t tmp;
    if ((rc = pDecoder->scanner->pushTok(pDecoder->scanner)) != SML_ERR_OK)
        return rc;
    if ((rc = pDecoder->tagstack->pop(pDecoder->tagstack, &tmp)) != SML_ERR_OK)
        return rc;
    return SML_ERR_OK;
}

/*************************************************************************/
/* Internal Functions                                                    */
/*************************************************************************/

static SmlPcdataPtr_t
concatPCData(SmlPcdataPtr_t pDat1, const SmlPcdataPtr_t pDat2)
{
    if (pDat1->contentType != pDat2->contentType)
        return NULL;

    switch (pDat1->contentType) {
        case SML_PCDATA_STRING:
            pDat1->content = (VoidPtr_t)smlLibStrcat(pDat1->content, pDat2->content);
            pDat1->length += pDat2->length;
            break;
        case SML_PCDATA_OPAQUE:
            if ((pDat1->content = smlLibRealloc(pDat1->content, pDat1->length + pDat2->length)) == NULL) 
                return NULL;
            smlLibMemmove(((Byte_t*)pDat1->content) + pDat1->length, pDat2->content, pDat2->length);
            pDat1->length += pDat2->length;
            break;
        default:
            return NULL;
    }
    return pDat1;
}

Ret_t
buildSyncHdr(XltDecoderPtr_t pDecoder, VoidPtr_t *ppSyncHdr)
{
    XltDecScannerPtr_t pScanner;
    SmlSyncHdrPtr_t pSyncHdr;
    Ret_t rc;
    Long_t sessionid = 0, msgid = 0, source = 0, target = 0, version = 0, proto = 0;

    /* shortcut to the scanner object */
    pScanner = pDecoder->scanner;

    /* if ppSyncHdr is not NULL we've already 
       found a SyncHdr before! */
    if (*ppSyncHdr != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    /* initialize new SmlSyncHdr */
    if ((pSyncHdr = (SmlSyncHdrPtr_t)smlLibMalloc(sizeof(SmlSyncHdr_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pSyncHdr, 0, sizeof(SmlSyncHdr_t));

    /* initialize the element type field */
    pSyncHdr->elementType = SML_PE_HEADER;

    /* empty SmlSyncHdr is possible */
    if (IS_EMPTY(pScanner->curtok)) {
        *ppSyncHdr = pSyncHdr;
        return SML_ERR_OK;
    }

    /* get next Token */
    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pSyncHdr);
        return rc;
    }

    /* parse child elements until we find a matching end tag */
    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_VERSION:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSyncHdr->version);
                version++;
                break;
            case TN_PROTO:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSyncHdr->proto);
                proto++;
                break;
            case TN_SESSIONID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSyncHdr->sessionID);
                sessionid++;
                break;
            case TN_MSGID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSyncHdr->msgID);
                msgid++;
                break;
            case TN_RESPURI:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSyncHdr->respURI);
                break;

                /* child tags */
            case TN_TARGET:
                rc = buildTargetOrSource(pDecoder, (VoidPtr_t)&pSyncHdr->target);
                target++;
                break;
            case TN_SOURCE:
                rc = buildTargetOrSource(pDecoder, (VoidPtr_t)&pSyncHdr->source);
                source++;
                break;
            case TN_CRED:
                rc = buildCred(pDecoder, (VoidPtr_t)&pSyncHdr->cred);
                break;
            case TN_META:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSyncHdr->meta);
                break;

                /* flags (empty tags) */
            case TN_NORESP:
                pSyncHdr->flags |= SmlNoResp_f;
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }

        /* decoding of child element went ok? */
        if (rc != SML_ERR_OK) {
            smlFreeSyncHdr(pSyncHdr);

            return rc;
        }

        /* get next token */
        if ((rc = nextToken(pDecoder)) != SML_ERR_OK) {
            smlFreeSyncHdr(pSyncHdr);
            return rc;
        }
    }

    if ((sessionid == 0) || (msgid == 0) || (target == 0) || (source == 0) || (version == 0) || (proto == 0))
    {
      smlFreeSyncHdr(pSyncHdr);
      return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppSyncHdr = pSyncHdr;

    return SML_ERR_OK;
}

Ret_t
buildSync(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlSyncPtr_t pSync;
    Ret_t rc;
    Long_t cmdid = 0;

    /* stop decoding the Sync when we find a SyncML command */
    Byte_t break_sync = 0;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    /* initialize a new Sync */
    if ((pSync = (SmlSyncPtr_t)smlLibMalloc(sizeof(SmlSync_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pSync, 0, sizeof(SmlSync_t));

    /* initialize the element type field */
    pSync->elementType = SML_PE_SYNC_START;

    if (IS_EMPTY(pScanner->curtok)) {

        smlLibFree(pSync);
        return SML_ERR_OK;
    }

    /* get next token */
    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pSync);
        return rc;
    }

    /* parse child elements until we find a matching end tag
       or until we find a TN_ADD, TN_ATOMIC, etc. start tag */
    while ((pScanner->curtok->type != TOK_TAG_END) && !break_sync) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_CMDID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSync->cmdID);
                cmdid++;
                break;
            case TN_META:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSync->meta);
                break;
            case TN_NUMBEROFCHANGES:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSync->noc);
                break;

                /* child tags */
            case TN_CRED:
                rc = buildCred(pDecoder, (VoidPtr_t)&pSync->cred);
                break;
            case TN_TARGET:
                rc = buildTargetOrSource(pDecoder, (VoidPtr_t)&pSync->target);
                break;
            case TN_SOURCE:
                rc = buildTargetOrSource(pDecoder, (VoidPtr_t)&pSync->source);
                break;

                /* flags */
            case TN_NORESP:
                pSync->flags |= SmlNoResp_f;
                break;

                /* quit if we find an Add, Atomic, etc.
                   element */
            case TN_ADD:
            case TN_ATOMIC:
            case TN_COPY:
            case TN_DELETE:
            case TN_SEQUENCE:
            case TN_REPLACE:
                break_sync = 1;
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeSync(pSync);
            return rc;
        }
        if (!break_sync) {
            /* get next token and continue as usual */
            if ((rc = nextToken(pDecoder)) != SML_ERR_OK) {
                smlFreeSync(pSync);
                return rc;
            }
        } else {
            /* we've found a SyncML command - we need to go
               back one token and correct the tagstack */
            if ((rc = discardToken(pDecoder)) != SML_ERR_OK) {
                smlFreeSync(pSync);
                return rc;
            }
        }
    }

    if (!break_sync)  {
      if ((pScanner->curtok->tagid) != TN_SYNC)
      {
        smlFreeSync(pSync);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
      }
      else
      {
		 if (pDecoder->tagstack->push(pDecoder->tagstack, pScanner->curtok->tagid))
         { 
            smlFreeSync(pSync);
            return SML_ERR_UNSPECIFIC;
         }
         if ((rc = pDecoder->scanner->pushTok(pDecoder->scanner)) != SML_ERR_OK)
         {
           smlFreeSync(pSync);
           return rc;
         }		  
      } 
    }

    *ppElem = pSync;

    return SML_ERR_OK;
}

#if (defined ATOMIC_RECEIVE || defined SEQUENCE_RECEIVE)  
Ret_t
buildAtomOrSeq(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlAtomicPtr_t pAoS;        /* SmlAtomicPtr_t and SequencePtr_t are pointer
                                to the same structure! */
    Ret_t rc;
    Byte_t break_aos = 0;    /* stop decoding the Atomic when we find a
                                SyncML command */
    Long_t cmdid = 0;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pAoS = (SmlAtomicPtr_t)smlLibMalloc(sizeof(SmlAtomic_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pAoS, 0, sizeof(SmlAtomic_t));

    /* initialize the element type field */
    pAoS->elementType = SML_PE_CMD_GROUP;

    if (IS_EMPTY(pScanner->curtok)) {
        smlLibFree(pAoS);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    /* get next token */
    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree (pAoS);
        return rc;
    }

    /* parse child elements until we find a matching end tag
       or until we find a TN_ADD, TN_ATOMIC, etc. start tag */
    while ((pScanner->curtok->type != TOK_TAG_END) && !break_aos) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_CMDID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pAoS->cmdID);
                cmdid++;
                break;
            case TN_META:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pAoS->meta);
                break;

                /* flags */
            case TN_NORESP:
                pAoS->flags |= SmlNoResp_f;
                break;

                /* quit if we find an Add, Atomic, etc.
                   element */
            case TN_ADD:
            case TN_REPLACE:
            case TN_DELETE:
            case TN_COPY:
            case TN_ATOMIC:
            case TN_MAP:
            case TN_SYNC:
			case TN_GET:
			case TN_ALERT:
			case TN_EXEC:
                break_aos = 1;
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeAtomic(pAoS);
            return rc;
        }
        if (!break_aos) {
            if ((rc = nextToken(pDecoder)) != SML_ERR_OK) {
                smlFreeAtomic(pAoS);
                return rc;
            }
        } else {
            /* we've found a SyncML command - we need to go
               back one token and correct the tagstack */
            if ((rc = discardToken(pDecoder)) != SML_ERR_OK) {
                smlFreeAtomic(pAoS);
                return rc;
            }
        }
    }

    if (!break_aos) {
        /* Atomic/Sequence must contain at least one SyncML command */
        smlFreeAtomic(pAoS);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    if (cmdid == 0)
    {
        smlFreeAtomic(pAoS);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppElem = pAoS;

    return SML_ERR_OK;
}
#endif

#ifdef EXEC_RECEIVE
Ret_t
buildExec(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlExecPtr_t pExec;
    Ret_t rc;
    Long_t items = 0, cmdid = 0;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pExec = (SmlExecPtr_t)smlLibMalloc(sizeof(SmlExec_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pExec, 0, sizeof(SmlExec_t));

    /* initialize the element type field */
    pExec->elementType = SML_PE_EXEC;

    if (IS_EMPTY(pScanner->curtok)) {
        smlLibFree(pExec);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pExec);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCData */
            case TN_CMDID: 
                rc = buildPCData(pDecoder, (VoidPtr_t)&pExec->cmdID);
                cmdid++;
                break;

            case TN_CORRELATOR: 
                rc = buildPCData(pDecoder, (VoidPtr_t)&pExec->correlator);
                break;

           case TN_META:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pExec->meta);
                break;

                /* child tags */
            case TN_CRED: 
                rc = buildCred(pDecoder, (VoidPtr_t)&pExec->cred);
                break;

            case TN_ITEM: 
                rc = buildItem(pDecoder, (VoidPtr_t)&pExec->item);
                items++;
                break;

                /* flags */
            case TN_NORESP:
                pExec->flags |= SmlNoResp_f;
                break;

            default:
                rc =  SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeExec(pExec);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeExec(pExec);
            return rc;
        }
    }

    if ((items == 0) || (cmdid == 0)) {
        smlFreeExec(pExec);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppElem = pExec;

    return SML_ERR_OK;
}
#endif

Ret_t
buildGenericCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlGenericCmdPtr_t pGenCmd;
    Ret_t rc;
    Long_t items = 0, cmdid = 0;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    /* initialize a new GenericCmd */
    if ((pGenCmd = (SmlGenericCmdPtr_t)smlLibMalloc(sizeof(SmlGenericCmd_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pGenCmd, 0, sizeof(SmlGenericCmd_t));

    /* initialize the element type field */
    pGenCmd->elementType = SML_PE_GENERIC;

    if (IS_EMPTY(pScanner->curtok)) {
        smlLibFree(pGenCmd);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pGenCmd);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_CMDID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pGenCmd->cmdID);
                cmdid++;
                break;
            case TN_META: 
                rc = buildPCData(pDecoder, (VoidPtr_t)&pGenCmd->meta);
                break;

                /* child tags */
            case TN_CRED: 
                rc = buildCred(pDecoder, (VoidPtr_t)&pGenCmd->cred);
                break;

                /* flags (empty tags) */
            case TN_NORESP:
                pGenCmd->flags |= SmlNoResp_f;
                break;
            case TN_ARCHIVE:
                pGenCmd->flags |= SmlArchive_f;
                break;
            case TN_SFTDEL:
                pGenCmd->flags |= SmlSftDel_f;
                break;

                /* Lists */
            case TN_ITEM:
                rc = appendItemList(pDecoder, &pGenCmd->itemList);
                items++;
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeGeneric(pGenCmd);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeGeneric(pGenCmd);
            return rc;
        }
    }

    if ((items == 0) || (cmdid == 0))
    {
        smlFreeGeneric(pGenCmd);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppElem = pGenCmd;

    return SML_ERR_OK;
}

Ret_t
buildAlert(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlAlertPtr_t pAlert;
    Ret_t rc;
    Long_t cmdid = 0;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pAlert = (SmlAlertPtr_t)smlLibMalloc(sizeof(SmlAlert_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pAlert, 0, sizeof(SmlAlert_t));

    /* initialize the element type field */
    pAlert->elementType = SML_PE_ALERT;

    if (IS_EMPTY(pScanner->curtok)) {
        smlLibFree(pAlert);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pAlert);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_CMDID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pAlert->cmdID);
                cmdid++;
                break;

            case TN_CORRELATOR: 
                rc = buildPCData(pDecoder, (VoidPtr_t)&pAlert->correlator);
                break;
                
            case TN_DATA: 
                rc = buildPCData(pDecoder, (VoidPtr_t)&pAlert->data);
                break;

                /* child tags */
            case TN_CRED: 
                rc = buildCred(pDecoder, (VoidPtr_t)&pAlert->cred);
                break;

                /* flags (empty tags) */
            case TN_NORESP:
                pAlert->flags |= SmlNoResp_f;
                break;

                /* Lists */
            case TN_ITEM:
                rc = appendItemList(pDecoder, &pAlert->itemList);
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeAlert(pAlert);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeAlert(pAlert);
            return rc;
        }
    }

    if (cmdid == 0)
    {
        smlFreeAlert(pAlert);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppElem = pAlert;

    return SML_ERR_OK;
}

#ifdef MAP_RECEIVE
Ret_t
buildMap(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlMapPtr_t pMap;
    Ret_t rc;
    Long_t target = 0, source = 0, mapitems = 0, cmdid = 0;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pMap = (SmlMapPtr_t)smlLibMalloc(sizeof(SmlMap_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pMap, 0, sizeof(SmlMap_t));

    /* initialize the element type field */
    pMap->elementType = SML_PE_MAP;

    /* Source is required */
    if (IS_EMPTY(pScanner->curtok)) {
        smlLibFree(pMap);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pMap);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_CMDID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pMap->cmdID);
                cmdid++;
                break;
            case TN_META: 
                rc = buildPCData(pDecoder, (VoidPtr_t)&pMap->meta);
                break;

                /* child tags */
            case TN_CRED: 
                rc = buildCred(pDecoder, (VoidPtr_t)&pMap->cred);
                break;
            case TN_SOURCE: 
                rc = buildTargetOrSource(pDecoder, (VoidPtr_t)&pMap->source);
                source++;
                break;
            case TN_TARGET: 
                rc = buildTargetOrSource(pDecoder, (VoidPtr_t)&pMap->target);
                target++;
                break;
#ifdef MAPITEM_RECEIVE
                /* Lists */
            case TN_MAPITEM:
                rc = appendMapItemList(pDecoder, &pMap->mapItemList);
                mapitems++;
                break;
#endif
            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeMap(pMap);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeMap(pMap);
            return rc;
        }
    }

    if ((source == 0) || (mapitems == 0) || (target == 0) || (cmdid == 0)) {
        smlFreeMap(pMap);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppElem = pMap;

    return SML_ERR_OK;
}
#endif

#ifdef SEARCH_RECEIVE
Ret_t
buildSearch(XltDecoderPtr_t pDecoder, VoidPtr_t *ppSearch)
{
    XltDecScannerPtr_t pScanner;
    SmlSearchPtr_t pSearch;
    Ret_t rc;
    Long_t source = 0, meta = 0, data = 0, cmdid = 0;

    pScanner = pDecoder->scanner;

    if (*ppSearch != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pSearch = (SmlSearchPtr_t)smlLibMalloc(sizeof(SmlSearch_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pSearch, 0, sizeof(SmlSearch_t));

    /* initialize the element type field */
    pSearch->elementType = SML_PE_SEARCH;

    /* Meta is required */
    if (IS_EMPTY(pScanner->curtok)) {
        smlLibFree(pSearch);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pSearch);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_CMDID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSearch->cmdID);
                cmdid++;
                break;
            case TN_LANG:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSearch->lang);
                break;
            case TN_META: 
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSearch->meta);
                meta++;
                break;
            case TN_DATA: 
                rc = buildPCData(pDecoder, (VoidPtr_t)&pSearch->data);
                data++;
                break;


                /* child tags */
            case TN_CRED: 
                rc = buildCred(pDecoder, (VoidPtr_t)&pSearch->cred);
                break;
            case TN_TARGET: 
                rc = buildTargetOrSource(pDecoder, (VoidPtr_t)&pSearch->target);
                break;

                /* flags */
            case TN_NORESP:
                pSearch->flags |= SmlNoResp_f;
                break;
            case TN_NORESULTS:
                pSearch->flags |= SmlNoResults_f;
                break;

                /* Lists */
            case TN_SOURCE:
                rc = appendSourceList(pDecoder, &pSearch->sourceList);
                source++;
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeSearch(pSearch);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeSearch(pSearch);
            return rc;
        }
    }

    if ((source == 0) || (meta == 0) || (data == 0) || (cmdid == 0)) {
        smlFreeSearch(pSearch);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppSearch = pSearch;

    return SML_ERR_OK;
}
#endif

Ret_t
buildPutOrGet(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlGetPtr_t pGet;
    Ret_t rc;
    Long_t items = 0, cmdid = 0;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pGet = (SmlGetPtr_t)smlLibMalloc(sizeof(SmlGet_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pGet, 0, sizeof(SmlGet_t));

    /* initialize the element type field */
    pGet->elementType = SML_PE_PUT_GET;

    if (IS_EMPTY(pScanner->curtok)) {
        smlLibFree(pGet);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pGet);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_CMDID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pGet->cmdID);
                cmdid++;
                break;
            case TN_LANG:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pGet->lang);
                break;
            case TN_META: 
                rc = buildPCData(pDecoder, (VoidPtr_t)&pGet->meta);
                break;

                /* child tags */
            case TN_CRED: 
                rc = buildCred(pDecoder, (VoidPtr_t)&pGet->cred);
                break;

                /* flags */
            case TN_NORESP:
                pGet->flags |= SmlNoResp_f;
                break;

                /* Lists */

            case TN_ITEM:
                rc = appendItemList(pDecoder, &pGet->itemList);
                items++;
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeGetPut(pGet);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeGetPut(pGet);
            return rc;
        }
    }

    if ((items == 0) || (cmdid == 0))
    {
        smlFreeGetPut(pGet);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppElem = pGet;

    return SML_ERR_OK;
}

Ret_t
buildTargetOrSource(XltDecoderPtr_t pDecoder, VoidPtr_t *ppTarget)
{
    XltDecScannerPtr_t pScanner;
    SmlTargetPtr_t pTarget;
    Long_t locuri = 0;
    Ret_t rc;

    pScanner = pDecoder->scanner;

    if (*ppTarget != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pTarget = (SmlTargetPtr_t)smlLibMalloc(sizeof(SmlTarget_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pTarget, 0, sizeof(SmlTarget_t));

    if (IS_EMPTY(pScanner->curtok)) {
        smlLibFree(pTarget);
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pTarget);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_LOCURI:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pTarget->locURI);
                locuri++;
                break;
            case TN_LOCNAME:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pTarget->locName);
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
	    if(pScanner->curtok->pcdata != NULL)
		    smlLibFree(pScanner->curtok->pcdata->content);
	    smlLibFree(pScanner->curtok->pcdata);
            smlFreeSourceTargetPtr(pTarget);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeSourceTargetPtr(pTarget);
            return rc;
        }
    }

    if (locuri == 0) 
    {
        smlFreeSourceTargetPtr(pTarget);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppTarget = pTarget;

    return SML_ERR_OK;
}

Ret_t
buildChal(XltDecoderPtr_t pDecoder, VoidPtr_t *ppChal)
{
    XltDecScannerPtr_t pScanner;
    SmlChalPtr_t pChal;
    Long_t meta = 0;
    Ret_t rc;

    pScanner = pDecoder->scanner;

    if (*ppChal != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pChal = (SmlChalPtr_t)smlLibMalloc(sizeof(SmlChal_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pChal, 0, sizeof(SmlChal_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppChal = pChal;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pChal);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_META:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pChal->meta);
                meta++;
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeChalPtr(pChal);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeChalPtr(pChal);
            return rc;
        }
    }

    if (meta == 0) 
    {
        smlFreeChalPtr(pChal);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppChal = pChal;

    return SML_ERR_OK;
}

Ret_t
buildCred(XltDecoderPtr_t pDecoder, VoidPtr_t *ppCred)
{
    XltDecScannerPtr_t pScanner;
    SmlCredPtr_t pCred;
    Ret_t rc;
    Long_t data = 0; 

    pScanner = pDecoder->scanner;

    if (*ppCred != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pCred = (SmlCredPtr_t)smlLibMalloc(sizeof(SmlCred_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pCred, 0, sizeof(SmlCred_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppCred = pCred;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pCred);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_DATA:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pCred->data);
                data++;
                break;
            case TN_META:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pCred->meta);
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeCredPtr(pCred);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeCredPtr(pCred);
            return rc;
        }
    }

    if (data == 0)
    {
      smlFreeCredPtr(pCred);
      return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppCred = pCred;

    return SML_ERR_OK;
}

Ret_t
buildItem(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlItemPtr_t pItem;
    Ret_t rc;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pItem = (SmlItemPtr_t)smlLibMalloc(sizeof(SmlItem_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pItem, 0, sizeof(SmlItem_t));

    /* Item might be empty */
    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pItem;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pItem);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_META:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pItem->meta);
                break;
            case TN_DATA:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pItem->data);
#ifdef __USE_EXTENSIONS__
#ifdef __SML_WBXML__
                if (pItem->data && pItem->data->contentType == SML_PCDATA_OPAQUE)
                    subdtdDecodeWbxml(pDecoder, (SmlPcdataPtr_t*)&pItem->data);
#endif
#endif
                break;
                /* child tags */
            case TN_TARGET:
                rc = buildTargetOrSource(pDecoder, (VoidPtr_t)&pItem->target);
                break;
            case TN_SOURCE:
                rc = buildTargetOrSource(pDecoder, (VoidPtr_t)&pItem->source);
                break;

            /* flags */
            case TN_MOREDATA:
                pItem->flags |= SmlMoreData_f;
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeItemPtr(pItem);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeItemPtr(pItem);
            return rc;
        }
    }

    *ppElem = pItem;

    return SML_ERR_OK;
}

#ifdef MAPITEM_RECEIVE
Ret_t
buildMapItem(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlMapItemPtr_t pMapItem;
    Long_t target = 0, source = 0;
    Ret_t rc;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    pScanner = pDecoder->scanner;

    if ((pMapItem = (SmlMapItemPtr_t)smlLibMalloc(sizeof(SmlMapItem_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pMapItem, 0, sizeof(SmlMapItem_t));

    if (IS_EMPTY(pScanner->curtok)) {
        smlLibFree(pMapItem);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pMapItem);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
            /* child tags */
            case TN_TARGET:
                rc = buildTargetOrSource(pDecoder, (VoidPtr_t)&pMapItem->target);
                target++;
                break;
            case TN_SOURCE:
                rc = buildTargetOrSource(pDecoder, (VoidPtr_t)&pMapItem->source);
                source++;
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeMapItemPtr(pMapItem);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeMapItemPtr(pMapItem);
            return rc;
        }
    }

    if ((target == 0) || (source == 0)) {
        smlFreeMapItemPtr(pMapItem);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppElem = pMapItem;

    return SML_ERR_OK;
}

#endif

Ret_t
buildStatus(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlStatusPtr_t pStatus;
    Ret_t rc;
    Long_t cmd = 0, data = 0, cmdid = 0;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    pScanner = pDecoder->scanner;

    if ((pStatus = (SmlStatusPtr_t)smlLibMalloc(sizeof(SmlStatus_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pStatus, 0, sizeof(SmlStatus_t));

    /* initialize the element type field */
    pStatus->elementType = SML_PE_STATUS;

    if (IS_EMPTY(pScanner->curtok)) {
        smlLibFree(pStatus);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pStatus);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCData elements */
            case TN_CMDID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pStatus->cmdID);
                cmdid++;
                break;
            case TN_MSGREF:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pStatus->msgRef);
                break;
            case TN_CMDREF:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pStatus->cmdRef);
                break;
            case TN_CMD:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pStatus->cmd);
                cmd++;
                break;
            case TN_DATA:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pStatus->data);
                data++;
                break;
            case TN_CHAL:
                rc = buildChal(pDecoder, (VoidPtr_t)&pStatus->chal);
                break;
            case TN_CRED:
                rc = buildCred(pDecoder, (VoidPtr_t)&pStatus->cred);
                break;

            /* Lists */
            case TN_ITEM:
                rc = appendItemList(pDecoder, (VoidPtr_t)&pStatus->itemList);
                break;
            case TN_TARGETREF:
                rc = appendTargetRefList(pDecoder, (VoidPtr_t)&pStatus->targetRefList);
                break;
            case TN_SOURCEREF:
                rc = appendSourceRefList(pDecoder, (VoidPtr_t)&pStatus->sourceRefList);
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeStatus(pStatus);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeStatus(pStatus);
            return rc;
        }
    }

    if ((cmd == 0) || (data == 0) || (cmdid == 0)) 
    {
        smlFreeStatus(pStatus);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppElem = pStatus;

    return SML_ERR_OK;
}

#ifdef RESULT_RECEIVE
Ret_t
buildResults(XltDecoderPtr_t pDecoder, VoidPtr_t *ppResults)
{
    XltDecScannerPtr_t pScanner;
    SmlResultsPtr_t pResults;
    Ret_t rc;
    Long_t cmdref = 0, items = 0, cmdid = 0;

    if (*ppResults != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    pScanner = pDecoder->scanner;

    if ((pResults = (SmlResultsPtr_t)smlLibMalloc(sizeof(SmlResults_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pResults, 0, sizeof(SmlResults_t));

    /* initialize the element type field */
    pResults->elementType = SML_PE_RESULTS;

    if (IS_EMPTY(pScanner->curtok)) {
        smlLibFree(pResults);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pResults);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {

            /* PCDATA elements */
            case TN_CMDID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pResults->cmdID);
                cmdid++;
                break;
            case TN_MSGREF:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pResults->msgRef);
                break;
            case TN_CMDREF:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pResults->cmdRef);
                cmdref++;
                break;
            case TN_META:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pResults->meta);
                break;
            case TN_TARGETREF:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pResults->targetRef);
                break;
            case TN_SOURCEREF:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pResults->sourceRef);
                break;

                /* Lists */
            case TN_ITEM:
                rc = appendItemList(pDecoder, &pResults->itemList);
                items++;
                break;

            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeResults(pResults);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeResults(pResults);
            return rc;
        }
    }

    if ((cmdref == 0) || (items == 0) || (cmdid == 0))
    {
        smlFreeResults(pResults);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
    }

    *ppResults = pResults;

    return SML_ERR_OK;
}

#endif

Ret_t
buildPCData(XltDecoderPtr_t pDecoder, VoidPtr_t *ppPCData)
{
    XltDecScannerPtr_t pScanner;
    SmlPcdataPtr_t pPCData = 0;
    SmlPcdataExtension_t ext;
    Ret_t rc;

    pScanner = pDecoder->scanner;

    if (*ppPCData != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if (IS_EMPTY(pScanner->curtok)) {
        if ((pPCData = (SmlPcdataPtr_t)smlLibMalloc(sizeof(SmlPcdata_t))) == NULL)
            return SML_ERR_NOT_ENOUGH_SPACE;

        smlLibMemset(pPCData, 0, sizeof(SmlPcdata_t));

        *ppPCData = pPCData;
        return SML_ERR_OK;
    } 

    pPCData = NULL;

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        if (rc == SML_ERR_XLT_INVAL_SYNCML_DOC) { /* leaks if dtd failed */
	        pPCData = pScanner->curtok->pcdata;
 		    *ppPCData = pPCData;
 		}

        return rc;
    }

    if (IS_CONTENT(pScanner->curtok)) {
        /* PCData element has a regular string or opaque content */
        while (pScanner->curtok->type == TOK_CONT) {
            if (pPCData == NULL)
                pPCData = pScanner->curtok->pcdata;
            else {
                pPCData = concatPCData(pPCData, pScanner->curtok->pcdata);
                smlLibFree(pScanner->curtok->pcdata->content);
                smlLibFree(pScanner->curtok->pcdata);
				
                if (pPCData == NULL)
                    return SML_ERR_XLT_INVAL_PCDATA;
            }
                
           if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
                *ppPCData = pPCData;
                return rc;
            }
        }
    } else if (IS_START_OR_EMPTY(pScanner->curtok)) {
        /* PCData element contains an XML dokument that is handled by an
           extension mechanism  */
        ext = pScanner->curtok->ext;
        if ((rc = discardToken(pDecoder)) != SML_ERR_OK) return rc;
        if ((pPCData = (SmlPcdataPtr_t)smlLibMalloc(sizeof(SmlPcdata_t))) == NULL)
            return SML_ERR_NOT_ENOUGH_SPACE;
        smlLibMemset(pPCData, 0, sizeof(SmlPcdata_t));
        pPCData->contentType = SML_PCDATA_EXTENSION;
        pPCData->extension = ext;
        switch (ext) {
#ifdef __USE_METINF__
				case SML_EXT_METINF:

                        if ((rc = buildMetInfMetInfCmd(pDecoder, (VoidPtr_t)&pPCData->content)) != SML_ERR_OK) {
                        smlLibFree(pPCData);
                        return rc;
                    }
                break;
#endif
#ifdef __USE_DEVINF__
				case SML_EXT_DEVINF:
                
				if ((rc = buildDevInfDevInfCmd(pDecoder, (VoidPtr_t)&pPCData->content)) != SML_ERR_OK) {
                           
					smlLibFree(pPCData);
                        return rc;
                    }

                /* the scanner must point to the closing PCDATA tag */
                if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) { 
                    smlLibFree(pPCData);
                    return rc;
                }
                break;
#endif
#ifdef __USE_DMTND__
            case SML_EXT_DMTND:
                
				if ((rc = buildDmTndCmd(pDecoder, (VoidPtr_t)&pPCData->content)) != SML_ERR_OK) {
                           
					smlLibFree(pPCData);
                    return rc;
                }

                /* the scanner must point to the closing PCDATA tag */
                if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) { 
                    smlLibFree(pPCData);
                    return rc;
                }
                break;
#endif
            default:
                smlFreePcdata(pPCData);
                return SML_ERR_XLT_INVAL_EXT;
        }

    } else if (IS_END(pScanner->curtok)) {
        /* PCData element is empty */
    } else {
        return SML_ERR_XLT_INVAL_PCDATA;
    }

        
    if (pScanner->curtok->type != TOK_TAG_END)
        return SML_ERR_XLT_INVAL_PCDATA;

    if (pPCData == NULL) {
        if ((pPCData = (SmlPcdataPtr_t)smlLibMalloc(sizeof(SmlPcdata_t))) == NULL)
            return SML_ERR_NOT_ENOUGH_SPACE;
        smlLibMemset(pPCData, 0, sizeof(SmlPcdata_t));
    }

    *ppPCData = pPCData;

    return SML_ERR_OK;
}

Ret_t
buildPCDataList(XltDecoderPtr_t pDecoder, VoidPtr_t *ppPCData)
{
		SmlPcdataListPtr_t pPCDataList = NULL, pPrev = NULL;
    
		pPCDataList = (SmlPcdataListPtr_t) *ppPCData;

		/* advance to the end of the list, and create ther an empty list element */
		while (pPCDataList != NULL) {
			pPrev = pPCDataList;
			pPCDataList = pPrev->next;
		}
		if ((pPCDataList = (SmlPcdataListPtr_t)smlLibMalloc(sizeof(SmlPcdataList_t))) == NULL)
          return SML_ERR_NOT_ENOUGH_SPACE;
        smlLibMemset(pPCDataList, 0, sizeof(SmlPcdataList_t));
		if (pPrev != NULL) /* we already had some entries in the list */
			pPrev->next = pPCDataList;
		else /* nope we created a new list */
			*ppPCData = pPCDataList;
		pPCDataList->data = NULL;
		/* at this point pPCDataList should point to an valid list element */
		return buildPCData(pDecoder, (VoidPtr_t)&pPCDataList->data);
}


static Ret_t
appendItemList(XltDecoderPtr_t pDecoder, SmlItemListPtr_t *ppItemList)
{
    SmlItemListPtr_t pNewItemList;
    SmlItemListPtr_t pItemList;
    Ret_t rc;

    pItemList = *ppItemList;
    if (pItemList != NULL)
        while (pItemList->next != NULL)
            pItemList = pItemList->next;

    if ((pNewItemList = (SmlItemListPtr_t)smlLibMalloc(sizeof(SmlItemList_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pNewItemList, 0, sizeof(SmlItemList_t));

    if ((rc = buildItem(pDecoder, (VoidPtr_t)&pNewItemList->item)) != SML_ERR_OK) {
        smlLibFree(pNewItemList);
        return rc;
    }

    if (pItemList == NULL)
        *ppItemList = pNewItemList;
    else
        pItemList->next = pNewItemList;

    return SML_ERR_OK;
}

static Ret_t
appendSourceList(XltDecoderPtr_t pDecoder, SmlSourceListPtr_t *ppSourceList)
{
    SmlSourceListPtr_t pNewSourceList;
    SmlSourceListPtr_t pSourceList;
    Ret_t rc;

    pSourceList = *ppSourceList;
    if (pSourceList != NULL)
        while (pSourceList->next != NULL)
            pSourceList = pSourceList->next;

    if ((pNewSourceList = (SmlSourceListPtr_t)smlLibMalloc(sizeof(SmlSourceList_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pNewSourceList, 0, sizeof(SmlSourceList_t));

    if ((rc = buildTargetOrSource(pDecoder, (VoidPtr_t)&pNewSourceList->source)) != SML_ERR_OK) {
        smlLibFree(pNewSourceList);
        return rc;
    }

    if (pSourceList == NULL)
        *ppSourceList = pNewSourceList;
    else
        pSourceList->next = pNewSourceList;

    return SML_ERR_OK;
}

#ifdef MAPITEM_RECEIVE

static Ret_t
appendMapItemList(XltDecoderPtr_t pDecoder, SmlMapItemListPtr_t *ppMapItemList)
{
    SmlMapItemListPtr_t pNewMapItemList;
    SmlMapItemListPtr_t pMapItemList;
    Ret_t rc;

    pMapItemList = *ppMapItemList;
    if (pMapItemList != NULL)
        while (pMapItemList->next != NULL)
            pMapItemList = pMapItemList->next;

    if ((pNewMapItemList = (SmlMapItemListPtr_t)smlLibMalloc(sizeof(SmlMapItemList_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pNewMapItemList, 0, sizeof(SmlMapItemList_t));

    if ((rc = buildMapItem(pDecoder, (VoidPtr_t)&pNewMapItemList->mapItem)) != SML_ERR_OK) {
        smlLibFree(pNewMapItemList);
        return rc;
    }

    if (pMapItemList == NULL)
        *ppMapItemList = pNewMapItemList;
    else
        pMapItemList->next = pNewMapItemList;

    return SML_ERR_OK;
}
#endif

static Ret_t
appendTargetRefList(XltDecoderPtr_t pDecoder, SmlTargetRefListPtr_t *ppTargetRefList)
{
    SmlTargetRefListPtr_t pNewTargetRefList;
    SmlTargetRefListPtr_t pTargetRefList;
    Ret_t rc;

    pTargetRefList = *ppTargetRefList;
    if (pTargetRefList != NULL)
        while (pTargetRefList->next != NULL)
            pTargetRefList = pTargetRefList->next;

    if ((pNewTargetRefList = (SmlTargetRefListPtr_t)smlLibMalloc(sizeof(SmlTargetRefList_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pNewTargetRefList, 0, sizeof(SmlTargetRefList_t));

    if ((rc = buildPCData(pDecoder, (VoidPtr_t)&pNewTargetRefList->targetRef)) != SML_ERR_OK) {
        smlFreePcdata(pNewTargetRefList->targetRef);
        smlLibFree(pNewTargetRefList);
        return rc;
    }

    if (pTargetRefList == NULL)
        *ppTargetRefList = pNewTargetRefList;
    else
        pTargetRefList->next = pNewTargetRefList;

    return SML_ERR_OK;
}

static Ret_t
appendSourceRefList(XltDecoderPtr_t pDecoder, SmlSourceRefListPtr_t *ppSourceRefList)
{
    SmlSourceRefListPtr_t pNewSourceRefList;
    SmlSourceRefListPtr_t pSourceRefList;
    Ret_t rc;

    pSourceRefList = *ppSourceRefList;
    if (pSourceRefList != NULL)
        while (pSourceRefList->next != NULL)
            pSourceRefList = pSourceRefList->next;

    if ((pNewSourceRefList = (SmlSourceRefListPtr_t)smlLibMalloc(sizeof(SmlSourceRefList_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pNewSourceRefList, 0, sizeof(SmlSourceRefList_t));

    if ((rc = buildPCData(pDecoder, (VoidPtr_t)&pNewSourceRefList->sourceRef)) != SML_ERR_OK) {
        smlFreePcdata(pNewSourceRefList->sourceRef);
        smlLibFree(pNewSourceRefList);
        return rc;
    }

    if (pSourceRefList == NULL)
        *ppSourceRefList = pNewSourceRefList;
    else
        pSourceRefList->next = pNewSourceRefList;

    return SML_ERR_OK;
}

