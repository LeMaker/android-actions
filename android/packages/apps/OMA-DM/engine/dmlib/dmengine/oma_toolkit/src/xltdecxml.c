/*************************************************************************/
/* module:         XML scanner                                           */
/* file:           XLTDecXml.c                                           */
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
 * The XML scanner/tokenizer. Used by the SyncML parser.
 */

#include <define.h>
#ifdef __SML_XML__
/*************************************************************************/
/* Definitions                                                           */
/*************************************************************************/
#include "xltdeccom.h"
#include "xlttags.h"

#include <libmem.h>
#include <libstr.h>

#include <smlerr.h>
#include "xpl_dm_Manager.h"

/**
 * Private Interface for the XML scanner.
 */
typedef struct xmlScannerPriv_s xmlScannerPriv_t, *xmlScannerPrivPtr_t;
struct xmlScannerPriv_s
{
    /* public */
    Ret_t (*nextTok)(XltDecScannerPtr_t);
    Ret_t (*destroy)(XltDecScannerPtr_t);
    Ret_t (*pushTok)(XltDecScannerPtr_t);
    void  (*setBuf)(XltDecScannerPtr_t pScanner, const MemPtr_t pBufStart, const MemPtr_t pBufEnd); 
    MemPtr_t (*getPos)(XltDecScannerPtr_t pScanner); 

    XltDecTokenPtr_t curtok;       /* current token */
    Long_t charset;                /* 0 */
    String_t charsetStr;           /* character set */
    Long_t pubID;                  /* 0 */
    String_t pubIDStr;             /* document public identifier */
	SmlPcdataExtension_t ext;      /* which is the actual open namespace ? */
	SmlPcdataExtension_t prev_ext; /* which is the previous open namespace ? */
	XltTagID_t ext_tag;            /* which tag started the actual namespace ? */
	XltTagID_t prev_ext_tag;       /* which tag started the previous open namespace ? */
	String_t   nsprefix;           /* prefix used for active namespace (if any) */
	Byte_t     nsprelen;           /* how long is the prefix ? (to save smlLibStrlen calls) */
    Flag_t finished;

    /* private */
    MemPtr_t pos;                  /* current position */
    MemPtr_t bufend;               /* end of buffer */
};

const ESCAPE_CHAR_TABLE_T escape_char_table[] =
{
   {'&',   (String_t)"amp;"},
   {'&',   (String_t)"amp"},
   {'<',   (String_t)"lt;"},
   {'<',   (String_t)"lt"},
   {'>',   (String_t)"gt;"},
   {'>',   (String_t)"gt"},
   {'\\',  (String_t)"apos;"},
   {'\\',  (String_t)"apos"},
   {'"',   (String_t)"quot;"},
   {'"',   (String_t)"quot"},
   {' ',   NULL}
};


/**
 * Public methods of the scanner interface.
 *
 * Description see XLTDecCom.h.
 */
static Ret_t _destroy(XltDecScannerPtr_t);
static Ret_t _nextTok(XltDecScannerPtr_t);
static Ret_t _pushTok(XltDecScannerPtr_t);
static void _setBuf(XltDecScannerPtr_t, const MemPtr_t, const MemPtr_t); 
static MemPtr_t _getPos(XltDecScannerPtr_t); 

/**
 * FUNCTION: readBytes
 *
 * Advance the current position pointer after checking whether the end of
 * the buffer has been reached. If the end of the buffer has been reached
 * the scanner's finished flag is set.
 *
 * PRE-Condition:
 * POST-Condition:
 *
 * IN:             bytes, read this many bytes
 *
 * IN/OUT:         pScanner, the scanner
 *
 * RETURNS:        1, if end of buffer has not been reached
 *                 0 otherwise
 */
static Boolean_t readBytes(xmlScannerPrivPtr_t pScanner, Long_t bytes);

/**
 * Skip whitespaces.
 */
static void skipS(xmlScannerPrivPtr_t pScanner);

static Ret_t xmlTag(xmlScannerPrivPtr_t pScanner, Byte_t endtag);
static Ret_t xmlName(xmlScannerPrivPtr_t pScanner, String_t *name);
static Ret_t xmlCharData(xmlScannerPrivPtr_t pScanner);
static Ret_t xmlProlog(xmlScannerPrivPtr_t pScanner);
static Ret_t xmlDocTypeDecl(xmlScannerPrivPtr_t pScanner);
static Ret_t xmlXMLDecl(xmlScannerPrivPtr_t pScanner);
static Ret_t xmlAttribute(xmlScannerPrivPtr_t pScanner, String_t *name, String_t *value);
static Ret_t xmlStringConst(xmlScannerPrivPtr_t pScanner, String_t *value);

static Ret_t xmlSkipPCDATA(xmlScannerPrivPtr_t pScanner);
static Ret_t xmlSkipComment(xmlScannerPrivPtr_t pScanner);
static Ret_t xmlSkipAttributes(xmlScannerPrivPtr_t pScanner);
static Ret_t xmlSkipPI(xmlScannerPrivPtr_t pScanner);
static Ret_t xmlCDATA(xmlScannerPrivPtr_t pScanner);
Boolean_t isPcdata(XltTagID_t tagid);

/*************************************************************************/
/* External Functions                                                    */
/*************************************************************************/


Ret_t
xltDecXmlInit(const MemPtr_t pBufEnd, MemPtr_t *ppBufStart, XltDecScannerPtr_t *ppScanner)
{
        xmlScannerPrivPtr_t pScanner;
        Ret_t rc;

        pScanner = (xmlScannerPrivPtr_t)smlLibMalloc(sizeof(xmlScannerPriv_t));
        if (pScanner == NULL) {
 			*ppScanner = NULL;
 			return SML_ERR_NOT_ENOUGH_SPACE;
 		}

        pScanner->finished = 0;
        pScanner->pos = *ppBufStart;
        pScanner->bufend = pBufEnd;
        pScanner->curtok = (XltDecTokenPtr_t)smlLibMalloc(sizeof(XltDecToken_t));
        if (pScanner->curtok == NULL) {
 			smlLibFree(pScanner);
 			*ppScanner = NULL;
 			return SML_ERR_NOT_ENOUGH_SPACE;
 		}
        
        pScanner->curtok->pcdata = NULL;
        pScanner->curtok->tagid = TN_UNDEF;
        pScanner->pubID = 0;
        pScanner->pubIDStr = NULL;
        pScanner->charset = 0;
        pScanner->charsetStr = NULL;
		pScanner->ext          = SML_EXT_UNDEFINED;
		pScanner->prev_ext     = (SmlPcdataExtension_t)255;
		pScanner->ext_tag      = TN_UNDEF;
		pScanner->prev_ext_tag = TN_UNDEF;
		pScanner->nsprelen     = 0;
		pScanner->nsprefix     = NULL;


        /* point public/private methods to the right implementation */
        pScanner->nextTok = _nextTok;
        pScanner->destroy = _destroy;
        pScanner->pushTok = _pushTok;
        pScanner->setBuf = _setBuf;
        pScanner->getPos = _getPos;

        if ((rc = xmlProlog(pScanner)) != SML_ERR_OK) {
                smlLibFree(pScanner->curtok);
                smlLibFree(pScanner);
                *ppScanner = NULL;
                return rc;
        }

        *ppScanner = (XltDecScannerPtr_t)pScanner;



        return SML_ERR_OK;
}

/**
 * FUNCTION: destroy
 * 
 * Free memory. Description see XltDecAll.h.
 */
static Ret_t
_destroy(XltDecScannerPtr_t pScanner)
{
        xmlScannerPrivPtr_t pScannerPriv;

        if (pScanner == NULL)
            return SML_ERR_OK;

        pScannerPriv = (xmlScannerPrivPtr_t)pScanner;
        smlLibFree(pScannerPriv->curtok);
        smlLibFree(pScannerPriv->charsetStr);
        smlLibFree(pScannerPriv->pubIDStr);
        smlLibFree(pScannerPriv);

        return SML_ERR_OK;
}

/**
 * FUNCTION: nextTok
 *
 * Get next token. Description see XltDecAll.h.
 */
static Ret_t
_nextTok(XltDecScannerPtr_t pScanner)
{
        xmlScannerPrivPtr_t pScannerPriv;
        Ret_t rc;

        pScannerPriv = (xmlScannerPrivPtr_t)pScanner;

        pScannerPriv->curtok->start = pScannerPriv->pos;

        skipS(pScannerPriv);

        /* skip unsupported elements until we find a supported one */
        rc = 0;


        while (!rc) {
            if (smlLibStrncmp((String_t)pScannerPriv->pos, "<!--", 4) == 0) {
                rc = xmlSkipComment(pScannerPriv);
            } else if (smlLibStrncmp((String_t)pScannerPriv->pos, "<?", 2) == 0) {
                rc = xmlSkipPI(pScannerPriv);
            } else if (smlLibStrncmp((String_t)pScannerPriv->pos, "</", 2) == 0) {
                rc = xmlTag(pScannerPriv, 1);
                break;
            } else if (smlLibStrncmp((String_t)pScannerPriv->pos, "<![CDATA[", 9) == 0) {
                rc = xmlCDATA(pScannerPriv);
                break;
            } else if ((isPcdata(pScannerPriv->curtok->tagid)) && (pScannerPriv->curtok->type != TOK_TAG_END)) {
                rc = xmlSkipPCDATA(pScannerPriv);
                break;
            } else if (smlLibStrncmp((String_t)pScannerPriv->pos, "<", 1) == 0) {
                rc = xmlTag(pScannerPriv, 0);
                break;
            } else {
                rc = xmlCharData(pScannerPriv);
                break;
            }
        }
        if (rc)
            return rc;

        return SML_ERR_OK;
}

/**
 * FUNCTION: pushTok
 *
 * Reset the scanner to the starting position of the current token within
 * the buffer. Description see XltDecAll.h.
 */
static Ret_t _pushTok(XltDecScannerPtr_t pScanner)
{
        xmlScannerPrivPtr_t pScannerPriv;       

        pScannerPriv = (xmlScannerPrivPtr_t)pScanner;
        pScannerPriv->pos = pScannerPriv->curtok->start;

        /* invalidate curtok */
        /* T.K. Possible Error. pScannerPriv->curtok is of type XltDecToken_t NOT ...Ptr_t */
        // OrigLine:
        // smlLibMemset(pScannerPriv->curtok, 0, sizeof(XltDecTokenPtr_t));
        pScannerPriv->curtok->type = (XltTokType_t)0;

        return SML_ERR_OK;
}

/**
 * FUNCTION: setBuf
 *
 * Set the working buffer of the scanner.
 */
static void
_setBuf(XltDecScannerPtr_t pScanner, const MemPtr_t pBufStart,
        const MemPtr_t pBufEnd)
{
    xmlScannerPrivPtr_t pScannerPriv = (xmlScannerPrivPtr_t)pScanner;
    pScannerPriv->pos = pBufStart;
    pScannerPriv->bufend = pBufEnd;
}

/**
 * FUNCTION: getPos
 *
 * Get the current position of the scanner within its working buffer.
 */
static MemPtr_t
_getPos(XltDecScannerPtr_t pScanner)
{
    return ((xmlScannerPrivPtr_t)pScanner)->pos;
}




/*************************************************************************/
/* Internal Functions                                                    */
/*************************************************************************/

/**
 * FUNCTION: readBytes
 *
 * Advance the position pointer. Description see above.
 */
static Boolean_t
readBytes(xmlScannerPrivPtr_t pScanner, Long_t bytes)
{
        if (pScanner->pos + bytes > pScanner->bufend) {
                pScanner->finished = 1;
                return 0;
        }
        pScanner->pos += bytes;
        return 1;
}

/**
 * FUNCTION: skipS
 *
 * Skip whitespace.
 */
static void skipS(xmlScannerPrivPtr_t pScanner)
{
    for (;;) {
        switch (*pScanner->pos) {
            case  9: /* tab stop */
            case 10: /* line feed */
            case 13: /* carriage return */
            case 32: /* space */
                // %%% luz: 2001-07-03: added exit from loop if no more bytes
                if (!readBytes(pScanner, 1)) return;
                break;
            default:
                return;
        }
    }
}

/**
 * FUNCTION: xmlProlog
 *
 * Scan the XML prolog (might be empty...).
 */
static Ret_t
xmlProlog(xmlScannerPrivPtr_t pScanner)
{
    Ret_t rc;

    skipS(pScanner);

    if (pScanner->pos + 5 > pScanner->bufend)
        return SML_ERR_OK;
    if (smlLibStrncmp((String_t)pScanner->pos, "<?xml", 5) == 0)
        if ((rc = xmlXMLDecl(pScanner)) != SML_ERR_OK)
            return rc;

    skipS(pScanner);

    while ((pScanner->pos + 4 <= pScanner->bufend) &&
           ((smlLibStrncmp((String_t)pScanner->pos, "<!--", 4) == 0) ||
            (smlLibStrncmp((String_t)pScanner->pos, "<?", 2) == 0))) {
        if (smlLibStrncmp((String_t)pScanner->pos, "<!--", 4) == 0)
            rc = xmlSkipComment(pScanner);
        else
            rc = xmlSkipPI(pScanner);
        if (rc != SML_ERR_OK)
            return rc;
        skipS(pScanner);
    }

    if ((pScanner->pos + 9 <= pScanner->bufend) &&
        (smlLibStrncmp((String_t)pScanner->pos, "<!DOCTYPE", 9) == 0))
        if ((rc = xmlDocTypeDecl(pScanner)) != SML_ERR_OK)
            return rc;

    skipS(pScanner);

    return SML_ERR_OK;
}

/**
 * FUNCTION: xmlDocTypeDecl
 *
 * Part of the Prolog scanning
 */
static Ret_t
xmlDocTypeDecl(xmlScannerPrivPtr_t pScanner)
{
    Ret_t rc;
    String_t name = NULL;
    String_t syslit = NULL;
    String_t publit = NULL;

    readBytes(pScanner, 9);
    skipS(pScanner);
    if ((rc = xmlName(pScanner, &name)) != SML_ERR_OK) {
        smlLibFree(name);
        return rc;
    }
    skipS(pScanner);

    /* parse ExternalID */
    if ((pScanner->pos + 6 <= pScanner->bufend) &&
        (smlLibStrncmp((String_t)pScanner->pos, "SYSTEM", 6) == 0)) {
        readBytes(pScanner, 6);
        skipS(pScanner);
        if ((rc = xmlStringConst(pScanner, &syslit)) != SML_ERR_OK) {
            smlLibFree(name);
            smlLibFree(syslit);
            return rc;
        }
    } else if ((pScanner->pos + 6 <= pScanner->bufend) &&
         (smlLibStrncmp((String_t)pScanner->pos, "PUBLIC", 6) == 0)) {
        readBytes(pScanner, 6);
        skipS(pScanner);
        if ((rc = xmlStringConst(pScanner, &publit)) != SML_ERR_OK) {
            smlLibFree(name);
            smlLibFree(publit);
            return rc;
        }
        skipS(pScanner);
        if ((rc = xmlStringConst(pScanner, &syslit)) != SML_ERR_OK) {
            smlLibFree(name);
            smlLibFree(syslit);
            smlLibFree(publit);
            return rc;
        }
    }

    smlLibFree(name); 
    smlLibFree(syslit); 
    smlLibFree(publit); 

    skipS(pScanner);

    if (*pScanner->pos != '>')
        return SML_ERR_XLT_INVAL_XML_DOC;
    readBytes(pScanner, 1);

    return SML_ERR_OK;
}

/**
 * FUNCTION: xmlXMLDecl
 *
 * Part of the Prolog scanning
 */
static Ret_t
xmlXMLDecl(xmlScannerPrivPtr_t pScanner)
{
    String_t name, value;
    Ret_t rc;

    readBytes(pScanner, 5);
    skipS(pScanner);
    
    /* mandatory version info */
    if ((rc = xmlAttribute(pScanner, &name, &value)) != SML_ERR_OK) {
        smlLibFree(name);
        smlLibFree(value);
        return rc;
    }
    if (smlLibStrcmp(name, "version") != 0) {
        smlLibFree(name);
        smlLibFree(value);
        return SML_ERR_XLT_INVAL_XML_DOC;
    }
    smlLibFree(name);
    smlLibFree(value);

    skipS(pScanner);

    /* optional attributes are encoding and standalone */
    while ((pScanner->pos + 2 <= pScanner->bufend) &&
        (smlLibStrncmp((String_t)pScanner->pos, "?>", 2) != 0)) {
        if ((rc = xmlAttribute(pScanner, &name, &value)) != SML_ERR_OK) {
            smlLibFree(name);
            smlLibFree(value);
            return rc;
        } 
        smlLibFree(name);
        smlLibFree(value);
        skipS(pScanner);
    }

    if (pScanner->pos + 2 > pScanner->bufend)
        return SML_ERR_XLT_END_OF_BUFFER;

    readBytes(pScanner, 2);

    return SML_ERR_OK;
}

/**
 * FUNCTION: xmlAttribute
 *
 * Handle Attributes //function can be used if attributes get necessary
 */
static Ret_t
xmlAttribute(xmlScannerPrivPtr_t pScanner, String_t *name, String_t *value)
{
    Ret_t rc;
    
    skipS(pScanner);

    if ((rc = xmlName(pScanner, name)) != SML_ERR_OK)
        return rc;

    skipS(pScanner);

		/* no attributes found, because this tag has none -> bail out */
		if (*pScanner->pos == '>') { 
			return SML_ERR_XLT_MISSING_CONT;
		}
		if (smlLibStrncmp((String_t)pScanner->pos, "/>", 2) == 0) {
			return SML_ERR_XLT_MISSING_CONT;
		}

    if (*pScanner->pos != '=') {
        smlLibFree(*name);
        *name = NULL;
        *value = NULL;
        return SML_ERR_XLT_INVAL_XML_DOC;
    }
    readBytes(pScanner, 1);

    skipS(pScanner);

    if ((rc = xmlStringConst(pScanner, value)) != SML_ERR_OK) {
        smlLibFree(*name);
        *name = NULL;
        *value = NULL;
        return rc;
    }
    
    return SML_ERR_OK;
}

/**
 * FUNCTION: xmlStringConst
 *
 * Handle Pcdata String Constants
 */
static Ret_t
xmlStringConst(xmlScannerPrivPtr_t pScanner, String_t *value)
{
    String_t end;
    int len;
    char del;

    if ((*pScanner->pos != '"') && (*pScanner->pos != '\'')) {
        *value = NULL;
        return SML_ERR_XLT_INVAL_XML_DOC;
    }
    del = *pScanner->pos;
    readBytes(pScanner, 1);
        
    if ((end = smlLibStrchr((String_t)pScanner->pos, del)) == NULL) {
        *value = NULL;
		
        return SML_ERR_XLT_END_OF_BUFFER;
    }
    len = end - (String_t)pScanner->pos;
    if ((*value = (String_t)smlLibMalloc(len + 1)) == NULL)
	{
       
		return SML_ERR_NOT_ENOUGH_SPACE;
	}
    smlLibMemset(*value, 0, len + 1);
    smlLibStrncpy(*value, (String_t)pScanner->pos, len);
    readBytes(pScanner, len + 1);

    return SML_ERR_OK;
}

/**
 * FUNCTION: xmlStringConst
 *
 * Handle escaped characters
 */
static void
xmlReadEscape(String_t buffer, MemPtr_t instring, int *instr_len)
{
int len= *instr_len;

	while (len !=0)
	{
	 if(*instring !=  '&')
	 {	 *buffer ++ = *instring++;
		 len--;
	 }
	 else
	 { 
		instring++;
		len--;
	 
	 	if(len>1)
	 	{

			Boolean_t	found =0;
			 unsigned int i=0;
			for (i = 0; escape_char_table[i].escape_str != NULL; i++) 
			{
			unsigned int str_len=smlLibStrlen( escape_char_table[i].escape_str);
			 if (smlLibStrncmp((String_t)instring, escape_char_table[i].escape_str, str_len) == 0)
			 {
			 *buffer++ =escape_char_table[i].token;
			 instring += str_len;
			 len -=str_len;
			 found = 1;
			*instr_len-= str_len;
			 break;
			 }
			}
			if(found==0)
			{
				*buffer++ = '&';
			}
        	}
		else 
			{
				*buffer++ = '&';
			}

 	}
 	}
}

/**
 * FUNCTION: xmlCharData
 *
 * Handle Pcdata character data content
 */
static Ret_t
xmlCharData(xmlScannerPrivPtr_t pScanner)
{
    SmlPcdataPtr_t pPCData;
    MemPtr_t begin;
    int len;


    pPCData = (SmlPcdataPtr_t)smlLibMalloc(sizeof(SmlPcdata_t));
    if (pPCData == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    pPCData->contentType = SML_PCDATA_UNDEFINED;
    pPCData->length = 0;
    pPCData->content = NULL;

    begin = pScanner->pos;
    
    if (pScanner->pos >= pScanner->bufend) {
        pPCData->content     = NULL;
        pPCData->contentType = SML_PCDATA_UNDEFINED;
        pPCData->extension   = SML_EXT_UNDEFINED;
        pPCData->length      = 0;
   	    pScanner->curtok->type = TOK_CONT;  
        pScanner->curtok->pcdata = pPCData; 
        smlLibFree(pPCData);
        return SML_ERR_XLT_END_OF_BUFFER;
    }

    while (*pScanner->pos != '<') /* && (*pScanner->pos != '&') */
    {
      if (pScanner->pos >= pScanner->bufend)
      {
        smlLibFree(pPCData);
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
      }
      if (!readBytes(pScanner, 1)) {
          smlLibFree(pPCData);
        return SML_ERR_XLT_END_OF_BUFFER;
      }
	  
    }
    len = pScanner->pos - begin;
    pPCData->content = smlLibMalloc(len + 1);
    if (pPCData->content == NULL){
        smlLibFree(pPCData);
		return SML_ERR_NOT_ENOUGH_SPACE;
	}
    smlLibMemset(pPCData->content, 0, len + 1);
    xmlReadEscape(pPCData->content, begin, &len);
    pPCData->contentType = SML_PCDATA_STRING;
    pPCData->length = len;

    pScanner->curtok->type = TOK_CONT;
    pScanner->curtok->pcdata = pPCData;

    return SML_ERR_OK;
}

/**
 * FUNCTION: xmlName
 *
 * Handle Name Elements
 */
static Ret_t
xmlName(xmlScannerPrivPtr_t pScanner, String_t *name)
{
    MemPtr_t begin;
    String_t tmp;
    int len;

    begin = pScanner->pos;
    while (((*pScanner->pos >= 'a') && (*pScanner->pos <= 'z')) ||
           ((*pScanner->pos >= 'A') && (*pScanner->pos <= 'Z')) ||
           ((*pScanner->pos >= '0') && (*pScanner->pos <= '9')) ||
           (*pScanner->pos == '.') || (*pScanner->pos == '-') ||
           (*pScanner->pos == '_') || (*pScanner->pos == ':'))
        if (!readBytes(pScanner, 1))
            return SML_ERR_XLT_END_OF_BUFFER;
        
    len = pScanner->pos - begin;
		/* T.K. bail out if len is zero without modifying name */
		if (len == 0) return SML_ERR_OK;


    tmp = (String_t)smlLibMalloc(len + 1);
    if (tmp == NULL) {
        *name = NULL;
        return SML_ERR_NOT_ENOUGH_SPACE;
    }
    smlLibMemset(tmp, 0, len + 1);
    smlLibStrncpy(tmp, (String_t)begin, len);
    *name = tmp;
    return SML_ERR_OK;
}

/**
 * FUNCTION: xmlTag
 *
 * Handle XML Tags
 */
static Ret_t
xmlTag(xmlScannerPrivPtr_t pScanner, Byte_t endtag)
{
    Ret_t rc;
    String_t name = NULL, attname=NULL, value = NULL, nsprefix = NULL;
	Byte_t nsprelen = 0;
    XltTagID_t tagid;
	SmlPcdataExtension_t ext;


    if (endtag) {
        if (!readBytes(pScanner, 2))
            return SML_ERR_XLT_END_OF_BUFFER;
    } else {
        if (!readBytes(pScanner, 1))
            return SML_ERR_XLT_END_OF_BUFFER;
    }

    if ((rc = xmlName(pScanner, &name)) != SML_ERR_OK)
    {
        if (rc != SML_ERR_NOT_ENOUGH_SPACE)
        {
            return SML_ERR_XLT_INVAL_XML_DOC;
        }
        else
        {
            return rc;
        }
        
    }

    //DP: case for corrupted xml file
    if ( !name )
       return SML_ERR_XLT_INVAL_XML_DOC;
      
	ext = pScanner->ext;
	if (!endtag) {
		/* Namespaces can only be defined on start, never on endtags
		 * but we have to make sure we close a namespace on the corrsponding endtag.
		 * Thats why we a) only open a namespace when it differs from the previous one, and
		 * b) record the tag_id that opend the namespace so we can close it when the 
		 * corresponding endtag is reached.
		 */
               
        if ((rc = xmlAttribute(pScanner, &attname, &value)) == SML_ERR_OK)
        {
			if (smlLibStrncmp(attname, "xmlns", 5) == 0) {
		/* Heureka we found a Namespace :-) */
				/* It's save to check attname[5] here, as it contains at least the terminating '\000' */
				if (attname[5] == ':') { /* we found a namespace prefixdefinition */
                    nsprelen = (Byte_t)smlLibStrlen(&attname[6]);
					nsprefix = smlLibMalloc(nsprelen+1);
                    if (nsprefix == NULL) {
                        smlLibFree(attname);
                        smlLibFree(value);
                        smlLibFree(name);
                        return SML_ERR_NOT_ENOUGH_SPACE;
                    }
					smlLibStrcpy(nsprefix,&attname[6]);						
				}
				ext = getExtByName(value);
				if (ext == 255) {
					smlLibFree(nsprefix); /* doesn't harm, even when empty */
					smlLibFree(attname);
					smlLibFree(value);
					smlLibFree(name);
					return  SML_ERR_XLT_INVALID_CODEPAGE;
				}
            else 
            {
			       if (smlLibStrncmp(value, "SYNCML:", 7) == 0)
                {
                   if ( smlLibStrcmp(XPL_DM_GetEnv(SYNCML_DM_VERSION), "1.2")==0)
                   {
   			          if (smlLibStrncmp(value+7, "SYNCML1.2", 9) != 0) 
                      {
                         smlLibFree(attname);
                         smlLibFree(value);
                         smlLibFree(name);
                         smlLibFree(nsprefix);
                         
   					       return  SML_ERR_XLT_INVALID_CODEPAGE;
                      }
                   }
                   else if (smlLibStrncmp(value+7, "SYNCML1.1", 9) != 0) 
                   {
                      if (smlLibStrncmp(value+7, "SYNCML1.1", 9) != 0) 
                      {
                         smlLibFree(attname);
                         smlLibFree(value);
                         smlLibFree(name);
                         smlLibFree(nsprefix);
   					       return  SML_ERR_XLT_INVALID_CODEPAGE;
                      }
                   }
                }
            }
			} else {
                if (rc == SML_ERR_NOT_ENOUGH_SPACE){
                    smlLibFree(attname);
                    smlLibFree(value);
                    smlLibFree(name);
                    return SML_ERR_NOT_ENOUGH_SPACE;
                }
                else{
				/* we found an unknown attribute -> bail out */
				smlLibFree(attname);
				smlLibFree(value);
				/* nsprefix is empty here so we save us a function call */
				smlLibFree(name);
				return SML_ERR_XLT_INVAL_XML_DOC;
			}
		}
            
        }
        else if ( rc != SML_ERR_XLT_MISSING_CONT)
        {
            /* xmlAttribute returns an SML_ERR_XLT_MISSING_CONT error when
             * no attribute was found. This is not an error, but everything else is.
             */
            smlLibFree(value);
            smlLibFree(name);
            return rc;
        }
        
        
	}


	if (pScanner->ext == ext) {
		/* no new Namespace found - lets proceed with the active one */

		/* first lets check wether a tag is in the right namespace, in case
		 * we are using namespaces with prefix notation ('mi:Format' instead of
		 * 'Format nsattr="..."). 
		 * If we are and the token is not in this namespace -> bail out
		 */
		if (pScanner->nsprelen > 0 && smlLibStrlen(name) > pScanner->nsprelen+1) {
			if (name[pScanner->nsprelen] != ':' || smlLibStrncmp(name,pScanner->nsprefix, pScanner->nsprelen) != 0) {
				smlLibFree(name);
				smlLibFree(attname);
				smlLibFree(value);
				smlLibFree(nsprefix);
				return SML_ERR_XLT_NO_MATCHING_CODEPAGE;
			}
		}
		/* Strip off namespace prefixes and ':' to find the tag.
		 * If no prefix is defined (pScanner->nsprelen == 0) take the whole tagname.
		 */
		if (pScanner->nsprelen > 0)
 			rc = getTagIDByStringAndExt(&name[0+pScanner->nsprelen+1], pScanner->ext, &tagid);
		else
			rc = getTagIDByStringAndExt(name, pScanner->ext, &tagid);
	} else {
		/* we have a new Namespace */
		if (nsprelen > 0 && smlLibStrlen(name) > nsprelen+1) {
			if (name[nsprelen] != ':' || smlLibStrncmp(name,nsprefix, nsprelen) != 0) {
				smlLibFree(name);
				smlLibFree(attname);
				smlLibFree(value);
				smlLibFree(nsprefix);
				return SML_ERR_XLT_NO_MATCHING_CODEPAGE;
			}
		}
		/* Strip off namespace prefixes and ':' to find the tag.
		 * If no prefix is defined (pScanner->nsprelen == 0) take the whole tagname.
		 */
		if (nsprelen > 0)
 			rc = getTagIDByStringAndExt(&name[nsprelen+1], ext, &tagid);
		else
			rc = getTagIDByStringAndExt(name, ext, &tagid);
	}
	/* free temporary buffers */
	smlLibFree(name);
	smlLibFree(attname);
	smlLibFree(value);

	if ((tagid == TN_UNDEF) || (rc != SML_ERR_OK)) {
		smlLibFree(nsprefix);
	return rc;
	}

	/* remember the old extension including the corresponding start tag if we found a new one */
	if (ext != pScanner->ext) { /* namespace changed */
		pScanner->prev_ext     = pScanner->ext;     /* remember the old ext */
		pScanner->prev_ext_tag = pScanner->ext_tag; /* and the corresponding start tag */
		pScanner->ext          = ext;
		pScanner->ext_tag      = tagid;
		smlLibFree(pScanner->nsprefix);
		pScanner->nsprefix    = nsprefix;
		pScanner->nsprelen    = nsprelen;
	}


	pScanner->curtok->tagid = tagid;
	pScanner->curtok->ext   = pScanner->ext;
    skipS(pScanner);

    if (endtag) {
        /* found end tag */
        if (smlLibStrncmp((String_t)pScanner->pos, ">", 1) != 0)
            return SML_ERR_XLT_INVAL_XML_DOC;
        pScanner->curtok->type = TOK_TAG_END;
        readBytes(pScanner, 1);
    	/* in case of an endtag we might need to close the current CP */
		if (tagid == pScanner->ext_tag) {
			pScanner->ext_tag = pScanner->prev_ext_tag;
			pScanner->ext     = pScanner->prev_ext;
			pScanner->prev_ext     = SML_EXT_UNDEFINED;
			pScanner->prev_ext_tag = TN_UNDEF;
			pScanner->nsprelen    = 0;
			smlLibFree(pScanner->nsprefix);
			pScanner->nsprefix    = NULL;
		}
    } else {
        /* Attributes are not supported in SyncML -> skip them*/
        if ((rc = xmlSkipAttributes(pScanner)) != SML_ERR_OK) return rc;

        if (smlLibStrncmp((String_t)pScanner->pos, "/>", 2) == 0) {
            /* found empty tag */
            pScanner->curtok->type = TOK_TAG_EMPTY;
            readBytes(pScanner, 2);
        } else if (smlLibStrncmp((String_t)pScanner->pos, ">", 1) == 0) {
            pScanner->curtok->type = TOK_TAG_START;
            readBytes(pScanner, 1);
        } else {
            return SML_ERR_XLT_INVAL_XML_DOC;
        }
    }

    return SML_ERR_OK;
}

/**
 * FUNCTION: xmlSkipPI
 *
 * Skip PI elements
 */
static Ret_t
xmlSkipPI(xmlScannerPrivPtr_t pScanner)
{
    if (pScanner) { /* Get rid of warning, this should not be called anyway */
 	}

    return SML_ERR_UNSPECIFIC;
}

/**
 * FUNCTION: xmlSkipComment
 *
 * Skip comments
 */
static Ret_t
xmlSkipComment(xmlScannerPrivPtr_t pScanner)
{
    readBytes(pScanner, 4);

    while ((pScanner->pos + 3 <= pScanner->bufend) &&
           (smlLibStrncmp((String_t)pScanner->pos, "-->", 3) != 0))
		   if (!readBytes(pScanner, 1))
           return SML_ERR_XLT_END_OF_BUFFER;
        

    if (pScanner->pos + 3 > pScanner->bufend)
        return SML_ERR_XLT_END_OF_BUFFER;

    if (!readBytes(pScanner, 3))
        return SML_ERR_XLT_END_OF_BUFFER;

    skipS(pScanner);

    return SML_ERR_OK;
}

/**
 * FUNCTION: xmlSkipAttributes
 *
 * Skip attributes -> they are not supported in SyncML
 */
static Ret_t
xmlSkipAttributes(xmlScannerPrivPtr_t pScanner)
{ 

    while ((pScanner->pos + 1 <= pScanner->bufend) &&
           (smlLibStrncmp((String_t)pScanner->pos, ">", 1)) && (smlLibStrncmp((String_t)pScanner->pos, "/>", 2)))
		   if (!readBytes(pScanner, 1))
           return SML_ERR_XLT_END_OF_BUFFER;

    
    if (pScanner->pos + 1 > pScanner->bufend)
        return SML_ERR_XLT_END_OF_BUFFER;

    return SML_ERR_OK;
}

/**
 * FUNCTION: xmlCDATA
 *
 * Handle a CDATA content
 */
static Ret_t
xmlCDATA(xmlScannerPrivPtr_t pScanner)
{
    SmlPcdataPtr_t pPCData;
    MemPtr_t begin;
    int len;
    
    readBytes(pScanner, 9);

    pPCData = (SmlPcdataPtr_t)smlLibMalloc(sizeof(SmlPcdata_t));
    if (pPCData == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    pPCData->contentType = SML_PCDATA_UNDEFINED;
    pPCData->length = 0;
    pPCData->content = NULL;

    begin = pScanner->pos;
    while (!((pScanner->pos[0] == ']') && (pScanner->pos[1] == ']') && (pScanner->pos[2] == '>')))
    {
        if (!readBytes(pScanner, 1))
        {
            smlLibFree(pPCData);
            return SML_ERR_XLT_END_OF_BUFFER;
        }
    }
		
    len = pScanner->pos - begin;
    pPCData->content = smlLibMalloc(len + 1);
    if (pPCData->content == NULL) {
		smlLibFree(pPCData);
 		return SML_ERR_NOT_ENOUGH_SPACE;
 	}

    smlLibMemset(pPCData->content, 0, len + 1);
    smlLibMemcpy(pPCData->content, begin, len);
    pPCData->contentType = SML_PCDATA_CDATA;
    pPCData->length = len;

    pScanner->curtok->type = TOK_CONT;
    pScanner->curtok->pcdata = pPCData;

    readBytes(pScanner, 3);

    return SML_ERR_OK;
}

/**
 * FUNCTION: xmlSkipPCDATA
 *
 * Read over a Pcdata content
 */
static Ret_t
xmlSkipPCDATA(xmlScannerPrivPtr_t pScanner)
{
    SmlPcdataPtr_t pPCData;
    MemPtr_t begin;
    int len;
    Ret_t rc;
    String_t _tagString = NULL;
    String_t _tagString2 = NULL;

		/* Check wether this PCData might contain a subdtd.
    ** We assume a Sub DTD starts with '<' as first char.
    ** If this char is present start further processing else
    ** take it as pure String data. If the scanning returns an
    ** error we reject the file, as '<' is not a valid char inside
    ** PCData elements.
		 */
    if (smlLibStrncmp((String_t)pScanner->pos, "<", 1) == 0) {
      rc = xmlTag(pScanner, 0);
        return rc;
    }

    _tagString = smlLibMalloc(XML_MAX_TAGLEN);
    if (_tagString == NULL) return SML_ERR_NOT_ENOUGH_SPACE;
    if ((rc = getTagString(pScanner->curtok->tagid, _tagString, pScanner->curtok->ext)) != SML_ERR_OK)
    {
        smlLibFree(_tagString);
        return rc;
    }
    
    _tagString2 = smlLibMalloc(smlLibStrlen(_tagString) + 4 + (pScanner->nsprelen +1));

    // build a end tag String to compate (e.g. </Meta>)
		// beware of possible namespace prefixes
    if (_tagString2 == NULL) 
    {
        smlLibFree(_tagString);
        return SML_ERR_NOT_ENOUGH_SPACE;
    }
    
    _tagString2 = smlLibStrcpy(_tagString2,"</");
		if (pScanner->nsprelen > 0) {
			_tagString2 = smlLibStrcat(_tagString2,pScanner->nsprefix);
			_tagString2 = smlLibStrcat(_tagString2,":");
		}
    _tagString2 = smlLibStrcat(_tagString2,_tagString);
    _tagString2 = smlLibStrcat(_tagString2,">");
    smlLibFree(_tagString);

    pPCData = (SmlPcdataPtr_t)smlLibMalloc(sizeof(SmlPcdata_t));
    
    
    
    if (pPCData == NULL)
    {
        smlLibFree(_tagString2);
        return SML_ERR_NOT_ENOUGH_SPACE;
    }
    pPCData->contentType = SML_PCDATA_UNDEFINED;
    pPCData->extension = SML_EXT_UNDEFINED;
    pPCData->length = 0;
    pPCData->content = NULL;

    begin = pScanner->pos;

    // read Pcdata content until end tag appears
    len = smlLibStrlen(_tagString2);
    while (smlLibStrncmp((String_t)pScanner->pos, _tagString2, len) != 0)
    {
      if ((pScanner->pos + len) >= pScanner->bufend)
      {
        smlLibFree(_tagString2);
        smlLibFree(pPCData);
        pScanner->curtok->pcdata = NULL;
        return SML_ERR_XLT_INVAL_SYNCML_DOC;
      }
        if (!readBytes(pScanner, 1))
      {
            smlLibFree(pPCData);
            return SML_ERR_XLT_END_OF_BUFFER;
      }
    }
    
    smlLibFree(_tagString2);
    
    len = pScanner->pos - begin;
    pPCData->content = smlLibMalloc(len + 1);
    if (pPCData->content == NULL) 
    {
        smlLibFree(pPCData);
        return SML_ERR_NOT_ENOUGH_SPACE;
    }
    
    
    smlLibMemset(pPCData->content, 0, len + 1);
    xmlReadEscape(pPCData->content, begin, &len);

    pPCData->contentType = SML_PCDATA_STRING;
    pPCData->length = len;

    pScanner->curtok->type = TOK_CONT;
    pScanner->curtok->pcdata = pPCData;


    return SML_ERR_OK;
}

/**
 * FUNCTION: isPcdata
 *
 * Check if the current tag id represents a Pcdata element
 */
Boolean_t isPcdata(XltTagID_t tagid)
{
  switch (tagid)
  {
    case TN_CMD:
    case TN_CMDID:
    case TN_CORRELATOR:      
    case TN_CMDREF:
    case TN_LANG:
    case TN_LOCNAME:      
    case TN_LOCURI:   
    case TN_MSGID:   
    case TN_MSGREF:   
    case TN_RESPURI:   
    case TN_SESSIONID:   
    case TN_SOURCEREF:   
    case TN_TARGETREF:   
    case TN_VERSION:   
    case TN_PROTO:   
    case TN_DATA:
    case TN_META:
	case TN_NUMBEROFCHANGES:
#ifdef __USE_METINF__
	case TN_METINF_EMI:
	case TN_METINF_FORMAT:
	case TN_METINF_FREEID:
	case TN_METINF_FREEMEM:
	case TN_METINF_LAST:
	case TN_METINF_MARK:
	case TN_METINF_MAXMSGSIZE:
	/* SCTSTK - 18/03/2002 S.H. 2002-04-05 : SyncML 1.1 */
	case TN_METINF_MAXOBJSIZE:
	case TN_METINF_NEXT:
	case TN_METINF_NEXTNONCE:		
	case TN_METINF_SIZE:
	case TN_METINF_TYPE:
	case TN_METINF_VERSION:
#endif
#ifdef __USE_DEVINF__
    case TN_DEVINF_MAN:
    case TN_DEVINF_MOD:
    case TN_DEVINF_OEM:
    case TN_DEVINF_FWV:
    case TN_DEVINF_SWV:
    case TN_DEVINF_HWV:
    case TN_DEVINF_DEVID:
    case TN_DEVINF_DEVTYP:
    case TN_DEVINF_MAXGUIDSIZE:
    case TN_DEVINF_SOURCEREF:
    case TN_DEVINF_DISPLAYNAME:
    case TN_DEVINF_CTTYPE:
    case TN_DEVINF_DATATYPE:
    case TN_DEVINF_SIZE:
    case TN_DEVINF_PROPNAME:
    case TN_DEVINF_VALENUM:
    case TN_DEVINF_PARAMNAME:
    case TN_DEVINF_SYNCTYPE:
    case TN_DEVINF_XNAM:
    case TN_DEVINF_XVAL:
    case TN_DEVINF_MAXMEM:
    case TN_DEVINF_MAXID:
    case TN_DEVINF_VERCT:
    case TN_DEVINF_VERDTD:
#endif
#ifdef __USE_DMTND__
    case TN_DMTND_AccessType: // TND->Node->DFProps
    case TN_DMTND_ACL: // TND->Node->RTProps
    case TN_DMTND_Add: // TND->Node->DFProps->AccessType
    case TN_DMTND_b64: // TND->Node->RTProps->Format
    case TN_DMTND_bin:  // TND->Node->RTProps->Format
    case TN_DMTND_bool:  // TND->Node->RTProps->Format
    case TN_DMTND_chr:  // TND->Node->RTProps->Format
    // case TN_DMTND_CaseSense: // TND->Node->DFProps
    case TN_DMTND_CIS: //  TND->Node->DFProps->CaseSense
    case TN_DMTND_Copy: // TND->Node->DFProps->AccessType 
    case TN_DMTND_CS: //  TND->Node->DFProps->CaseSense
    case TN_DMTND_date:  // TND->Node->RTProps->Format
    case TN_DMTND_DDFName:  // TND->Node->RTProps->Type
    case TN_DMTND_DefaultValue: // TND->Node->DFProps
    case TN_DMTND_Delete: // TND->Node->DFProps->AccessType 
    case TN_DMTND_Description:  // TND->Node->DFProps
    case TN_DMTND_DFFormat:  // TND->Node->DFProps 
    // case TN_DMTND_DFProperties: // TND->Node
    case TN_DMTND_DFTitle: // TND->Node->DFProps 
    case TN_DMTND_DFType: // TND->Node->DFProps 
    // case TN_DMTND_Dynamic:  // TND->Node->DFProps->Scope
    case TN_DMTND_Exec: // TND->Node->DFProps->AccessType 
    case TN_DMTND_float:  // TND->Node->RTProps->Format
    // case TN_DMTND_Format: // TND->Node->RTProps: 
    case TN_DMTND_Get: // TND->Node->DFProps->AccessType 
    case TN_DMTND_int:  // TND->Node->RTProps->Format
    case TN_DMTND_Man: // TND
    // case TN_DMTND_MgmtTree: 
    case TN_DMTND_MIME:  // TND->Node->RTProps->Type
    case TN_DMTND_Mod: // TND
    case TN_DMTND_Name: // TND->Node->RTProps 
    // case TN_DMTND_Node: // TND
    case TN_DMTND_node:  // TND->Node->RTProps->Format
    case TN_DMTND_NodeName: // TND->Node
    case TN_DMTND_null:  // TND->Node->RTProps->Format
    // case TN_DMTND_Occurrence: // TND->Node->DFProps
    case TN_DMTND_One: // TND->Node->DFProps->Occurrence
    case TN_DMTND_OneOrMore: // TND->Node->DFProps->Occurrence
    case TN_DMTND_OneOrN: // TND->Node->DFProps->Occurrence
    case TN_DMTND_Path: // TND->Node
    case TN_DMTND_Permanent: // TND->Node->DFProps->Scope
    case TN_DMTND_Replace: // TND->Node->DFProps->AccessType 
    // case TN_DMTND_RTProperties: // TND->Node 
    // case TN_DMTND_Scope: // TND->Node->DFProps
    case TN_DMTND_Size:  // TND->Node->RTProps
    case TN_DMTND_time: // TND->Node->RTProps->Format
    case TN_DMTND_Title: // TND->Node->RTProps
    case TN_DMTND_TStamp: // TND->Node->RTProps 
    // case TN_DMTND_Type: // TND->Node->RTProps->Type
    case TN_DMTND_Value: // TND
    case TN_DMTND_VerDTD: // TND
    case TN_DMTND_VerNo: // TND->Node->RTProps
    case TN_DMTND_xml: // TND->Node->RTProps->Format
    case TN_DMTND_ZeroOrMore: // TND->Node->DFProps->Occurrence
    case TN_DMTND_ZeroOrN:  // TND->Node->DFProps->Occurrence
    case TN_DMTND_ZeroOrOne:// TND->Node->DFProps->Occurrence
#endif
      return 1;
    default:
      return 0;
  }
}
#endif
