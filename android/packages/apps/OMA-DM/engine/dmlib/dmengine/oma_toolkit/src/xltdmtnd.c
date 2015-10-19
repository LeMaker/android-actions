/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*--------------------------------------------------------------------------------------------------

   Header Name: xltdmtnd.c

   General Description: XLT DM TND related functions 

--------------------------------------------------------------------------------------------------*/

#include "define.h"

#ifdef __USE_DMTND__
#include "smldmtnddtd.h"

#include "xlttags.h"
#include "xltdmtnd.h"
#include "xlttagtbl.h"
#include "xltenc.h"
#include "xltencwbxml.h"

#include <define.h>
#include <libstr.h>
#include <smlerr.h>
#include <libmem.h>
#include <libutil.h>

/* extern */
extern SML_API void smlFreeDmTndFormat(SmlDmTndFormatPtr_t data);
extern SML_API void smlFreeDmTndType(SmlDmTndTypePtr_t data);
extern SML_API void smlFreeDmTndDFElement(SmlDmTndDFElementPtr_t data);
extern SML_API void smlFreeDmTndRTProps(SmlDmTndRTPropsPtr_t data);
extern SML_API void smlFreeDmTndDFProps(SmlDmTndDFPropsPtr_t data);
extern SML_API void smlFreeDmTndNode(SmlDmTndNodePtr_t data);
extern SML_API void smlFreeDmTndNodeList(SmlDmTndNodeListPtr_t data);
extern SML_API void smlFreeDmTnd(SmlDmTndPtr_t data);

Ret_t buildDmTndNodeCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem);

/* decoder callbacks */
Ret_t buildDmTndFormatCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlDmTndFormatPtr_t pDmTndFormat;
    Ret_t rc;

    KCDBG("buildDmTndFormatCmd: Enter");    
 
    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pDmTndFormat = (SmlDmTndFormatPtr_t)smlLibMalloc(sizeof(SmlDmTndFormat_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pDmTndFormat, 0, sizeof(SmlDmTndFormat_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pDmTndFormat;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlFreeDmTndFormat(pDmTndFormat);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        VoidPtr_t pContent = smlLibMalloc(6);
        if ( pContent == NULL )
        {
            smlFreeDmTndFormat(pDmTndFormat);
            return SML_ERR_NOT_ENOUGH_SPACE;
        }
        smlLibMemset(pContent, 0, 6);

        switch (pScanner->curtok->tagid) {
          case TN_DMTND_b64:
             KCDBG("buildDmTndFormatCmd: b64");    
             smlLibStrcpy(pContent, "b64");
             break;
          case TN_DMTND_bin:
             KCDBG("buildDmTndFormatCmd: bin");    
             smlLibStrcpy(pContent, "bin");
             break;
          case TN_DMTND_bool:
             KCDBG("buildDmTndFormatCmd: bool");    
             smlLibStrcpy(pContent, "bool");
             break; 
          case TN_DMTND_chr:
             KCDBG("buildDmTndFormatCmd: chr");    
             smlLibStrcpy(pContent, "chr");
             break;
          case TN_DMTND_int:
             KCDBG("buildDmTndFormatCmd: int");    
             smlLibStrcpy(pContent, "int");
             break;
          case TN_DMTND_node:
             KCDBG("buildDmTndFormatCmd: node");    
             smlLibStrcpy(pContent, "node");
             break;
          case TN_DMTND_null:
             KCDBG("buildDmTndFormatCmd: null");    
             smlLibStrcpy(pContent, "null");
             break;
          case TN_DMTND_xml:
             KCDBG("buildDmTndFormatCmd: xml");    
             smlLibStrcpy(pContent, "xml");
             break;
          case TN_DMTND_date:
             KCDBG("buildDmTndFormatCmd: date");    
             smlLibStrcpy(pContent, "date");
             break;
          case TN_DMTND_time:
             KCDBG("buildDmTndFormatCmd: time");    
             smlLibStrcpy(pContent, "time");
             break;
          case TN_DMTND_float:
             KCDBG("buildDmTndFormatCmd: float");    
             smlLibStrcpy(pContent, "float");
            break;
          default:
              KCDBG(">>>>>  Unkown token: %d <<<<<", pScanner->curtok->tagid);
              rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }

        if (rc != SML_ERR_OK) {
            smlLibFree(pContent);
            smlFreeDmTndFormat(pDmTndFormat);
            return rc;
        }

        rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndFormat->value);
        if (rc != SML_ERR_OK) {
            smlLibFree(pContent);
            smlFreeDmTndFormat(pDmTndFormat);
            return rc;
        }
        pDmTndFormat->value->content = pContent;
        pDmTndFormat->value->length = smlLibStrlen(pContent);

        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlLibFree(pContent);
            smlFreeDmTndFormat(pDmTndFormat);
            return rc;
        }
        if (pScanner->curtok->type != TOK_TAG_END) {
            smlLibFree(pContent);
            smlFreeDmTndFormat(pDmTndFormat);
            return SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
    }

    *ppElem = pDmTndFormat;

    return SML_ERR_OK;
}

/* decoder callbacks */
Ret_t buildDmTndTypeCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner = NULL;
    SmlDmTndTypePtr_t pDmTndType = NULL;
    SmlPcdataPtr_t pPCData = NULL;
    Ret_t rc = SML_ERR_OK;

    KCDBG("buildDmTndTypeCmd: Enter");    
  
    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pDmTndType = (SmlDmTndTypePtr_t)smlLibMalloc(sizeof(SmlDmTndType_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pDmTndType, 0, sizeof(SmlDmTndType_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pDmTndType;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlFreeDmTndType(pDmTndType);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        if (IS_CONTENT(pScanner->curtok)) {
            // detected PCDATA instead of <MIME> or <DDFName> tags
            // need to copy it to pDmTndType->mime
            if ((pPCData = (SmlPcdataPtr_t)smlLibMalloc(sizeof(SmlPcdata_t))) != NULL) {
                smlLibMemset(pPCData, 0, sizeof(SmlPcdata_t));
                if ((pPCData->content = (VoidPtr_t)smlLibMalloc(pScanner->curtok->pcdata->length)) != NULL) {
                    smlLibMemmove(((Byte_t*)pPCData->content), pScanner->curtok->pcdata->content, pScanner->curtok->pcdata->length);
                    pPCData->length = pScanner->curtok->pcdata->length;
                    pDmTndType->mime = pPCData;
                } else {
                    smlLibFree(pPCData);
                    pPCData = NULL;
                    rc = SML_ERR_NOT_ENOUGH_SPACE;
                }
            } else {
                rc = SML_ERR_NOT_ENOUGH_SPACE;
            }
            if (rc == SML_ERR_OK) {
                KCDBG("buildDmTndTypeCmd:        %s", pDmTndType->mime->content);
            }
        } else {
            switch (pScanner->curtok->tagid) {
                case TN_DMTND_MIME:
                    KCDBG("buildDmTndTypeCmd:    MIME");    
                    rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndType->mime);
                    if (rc == SML_ERR_OK) {
                        KCDBG("buildDmTndTypeCmd:        %s", pDmTndType->mime->content);
                    }
                    break;
                case TN_DMTND_DDFName:
                    KCDBG("buildDmTndTypeCmd:    DDFName");    
                    rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndType->ddfname);
                    if (rc == SML_ERR_OK) {
                        KCDBG("buildDmTndTypeCmd:        %s", pDmTndType->ddfname->content);
                    }
                    break;
                default:
                    KCDBG("buildDmTndTypeCmd: Error: Unkown token: %d", pScanner->curtok->tagid);
                    rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
            }
        }
        if (rc != SML_ERR_OK) {
            smlFreeDmTndType(pDmTndType);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeDmTndType(pDmTndType);
            return rc;
        }
        if (pScanner->curtok->type != TOK_TAG_END) {
            smlFreeDmTndType(pDmTndType);
            return SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
    }

    *ppElem = pDmTndType;

    return SML_ERR_OK;
}

/* decoder callbacks */
Ret_t buildDmTndDFElementCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlDmTndDFElementPtr_t pDmTndDFElement;
    Ret_t rc;

    KCDBG("buildDmTndDFElementCmd: Enter");    
  
    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pDmTndDFElement = (SmlDmTndDFElementPtr_t)smlLibMalloc(sizeof(SmlDmTndDFElement_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pDmTndDFElement, 0, sizeof(SmlDmTndDFElement_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pDmTndDFElement;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlFreeDmTndDFElement(pDmTndDFElement);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        VoidPtr_t pContent = smlLibMalloc(12);
        if ( pContent == NULL )
        {
            smlFreeDmTndDFElement(pDmTndDFElement);
            return SML_ERR_NOT_ENOUGH_SPACE;
        }
        smlLibMemset(pContent, 0, 12);

        switch (pScanner->curtok->tagid) {
          /* AccessType Elements */
          case TN_DMTND_Add:
             smlLibStrcpy(pContent, "Add");
             break;
          case TN_DMTND_Copy:
             smlLibStrcpy(pContent, "Copy");
             break;
          case TN_DMTND_Delete:
             smlLibStrcpy(pContent, "Delete");
             break;
          case TN_DMTND_Exec:
             smlLibStrcpy(pContent, "Exec");
             break;
          case TN_DMTND_Get:
             smlLibStrcpy(pContent, "Get");
             break;
          case TN_DMTND_Replace:
             smlLibStrcpy(pContent, "Replace");
             break;
          /* Occurrence Elements */
          case TN_DMTND_One:
             smlLibStrcpy(pContent, "One");
             break;
          case TN_DMTND_ZeroOrOne:
             smlLibStrcpy(pContent, "ZeroOrOne");
             break;
          case TN_DMTND_ZeroOrMore:
             smlLibStrcpy(pContent, "ZeroOrMore");
             break;
          case TN_DMTND_OneOrMore:
             smlLibStrcpy(pContent, "OneOrMore");
             break;
          case TN_DMTND_ZeroOrN:
             smlLibStrcpy(pContent, "ZeroOrN");
             break;
          case TN_DMTND_OneOrN:
             smlLibStrcpy(pContent, "OneOrN");
             break;
          /* Scope Elements */
          case TN_DMTND_Permanent:
             smlLibStrcpy(pContent, "Permanent");
             break;
          case TN_DMTND_Dynamic:
             smlLibStrcpy(pContent, "Dynamic");
             break;
          /* CaseSense Elements */
          case TN_DMTND_CS:
             smlLibStrcpy(pContent, "CS");
             break;
          case TN_DMTND_CIS:
             smlLibStrcpy(pContent, "CIS");
             break;
          default:
              KCDBG(">>>>> buildDmTndDFElementCmd: Unkown token: %d <<<<<", pScanner->curtok->tagid);
              rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeDmTndDFElement(pDmTndDFElement);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeDmTndDFElement(pDmTndDFElement);
            return rc;
        }
        if (pScanner->curtok->type != TOK_TAG_END) {
            smlFreeDmTndDFElement(pDmTndDFElement);
            return SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
    }

    *ppElem = pDmTndDFElement;

    return SML_ERR_OK;
}

/* decoder callbacks */
Ret_t buildDmTndRTPropsCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlDmTndRTPropsPtr_t pDmTndRTProps;
    Ret_t rc;

    KCDBG("buildDmTndRTPropsCmd: Enter");    
  
    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pDmTndRTProps = (SmlDmTndRTPropsPtr_t)smlLibMalloc(sizeof(SmlDmTndRTProps_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pDmTndRTProps, 0, sizeof(SmlDmTndRTProps_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pDmTndRTProps;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlFreeDmTndRTProps(pDmTndRTProps);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
          case TN_DMTND_ACL:
            KCDBG("buildDmTndRTPropsCmd: ACL");    
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndRTProps->acl);
            if (rc == SML_ERR_OK) {
                KCDBG("buildDmTndRTPropsCmd:    ACL = %s", pDmTndRTProps->acl->content);
            }
            break;
          case TN_DMTND_Format:
            KCDBG("buildDmTndRTPropsCmd: Format");    
            rc = buildDmTndFormatCmd(pDecoder, (VoidPtr_t)&pDmTndRTProps->format);
            break;
          case TN_DMTND_Name:
            KCDBG("buildDmTndRTPropsCmd: Name");    
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndRTProps->name);
            if (rc == SML_ERR_OK) {
              KCDBG("buildDmTndRTPropsCmd:    Name = %s", pDmTndRTProps->name->content);
            }
            break;
          case TN_DMTND_Size:
            KCDBG("buildDmTndRTPropsCmd: Size");    
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndRTProps->size);
            if (rc == SML_ERR_OK) {
                KCDBG("buildDmTndRTPropsCmd:    Size = %s", pDmTndRTProps->size->content);
            }
            break;
          case TN_DMTND_Title:
            KCDBG("buildDmTndRTPropsCmd: Title");    
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndRTProps->title);
            if (rc == SML_ERR_OK) {
                KCDBG("buildDmTndRTPropsCmd:    Title = %s", pDmTndRTProps->title->content);
            }
            break;
          case TN_DMTND_TStamp:
            KCDBG("buildDmTndRTPropsCmd: TStampt");    
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndRTProps->tstamp);
            if (rc == SML_ERR_OK) {
                KCDBG("buildDmTndRTPropsCmd:    TStampt = %s", pDmTndRTProps->tstamp->content);
            }
            break;
          case TN_DMTND_Type:
            KCDBG("buildDmTndRTPropsCmd: Type");    
            rc = buildDmTndTypeCmd(pDecoder, (VoidPtr_t)&pDmTndRTProps->type);
            break;
          case TN_DMTND_VerNo:
            KCDBG("buildDmTndRTPropsCmd: VerNo");    
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndRTProps->verno);
            if (rc == SML_ERR_OK) {
                KCDBG("buildDmTndRTPropsCmd:    VerNo = %s", pDmTndRTProps->verno->content);
            }
            break;
          default:
              KCDBG("buildDmTndRTPropsCmd: Error: Unkown token: %d", pScanner->curtok->tagid);
              rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeDmTndRTProps(pDmTndRTProps);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeDmTndRTProps(pDmTndRTProps);
            return rc;
        }
    }

    *ppElem = pDmTndRTProps;

    return SML_ERR_OK;
}

/* decoder callbacks */
Ret_t buildDmTndDFPropsCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlDmTndDFPropsPtr_t pDmTndDFProps;
    Ret_t rc = SML_ERR_OK;

    KCDBG("buildDmTndDFPropsCmd: Enter");    
  
    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pDmTndDFProps = (SmlDmTndDFPropsPtr_t)smlLibMalloc(sizeof(SmlDmTndDFProps_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pDmTndDFProps, 0, sizeof(SmlDmTndDFProps_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pDmTndDFProps;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlFreeDmTndDFProps(pDmTndDFProps);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
          case TN_DMTND_AccessType:
            KCDBG("buildDmTndDFPropsCmd: AccessType");
            rc = buildDmTndDFElementCmd(pDecoder, (VoidPtr_t)&pDmTndDFProps->accesstype);
            break;
          case TN_DMTND_DefaultValue:
            KCDBG("buildDmTndDFPropsCmd: DefaultValue");
            rc = buildDmTndFormatCmd(pDecoder, (VoidPtr_t)&pDmTndDFProps->defaultvalue);
            break;
          case TN_DMTND_Description:
            KCDBG("buildDmTndDFPropsCmd: Description");
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndDFProps->description);
            break;
          case TN_DMTND_DFFormat:
            KCDBG("buildDmTndDFPropsCmd: DFFormat");
            rc = buildDmTndFormatCmd(pDecoder, (VoidPtr_t)&pDmTndDFProps->dfformat);
            break;
          case TN_DMTND_Occurrence:
            KCDBG("buildDmTndDFPropsCmd: Occurrence");
            rc = buildDmTndDFElementCmd(pDecoder, (VoidPtr_t)&pDmTndDFProps->occurrence);
            break;
          case TN_DMTND_Scope:
            KCDBG("buildDmTndDFPropsCmd: Scope");
            rc = buildDmTndDFElementCmd(pDecoder, (VoidPtr_t)&pDmTndDFProps->scope);
            break;
          case TN_DMTND_DFTitle:
            KCDBG("buildDmTndDFPropsCmd: DFTitle");
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndDFProps->dftitle);
            break;
          case TN_DMTND_DFType:
            KCDBG("buildDmTndDFPropsCmd: DFType");
            rc = buildDmTndTypeCmd(pDecoder, (VoidPtr_t)&pDmTndDFProps->dftype);
            break;
          case TN_DMTND_CaseSense:
            KCDBG("buildDmTndDFPropsCmd: CaseSense");
            rc = buildDmTndDFElementCmd(pDecoder, (VoidPtr_t)&pDmTndDFProps->casesense);
            break;
          default:
              KCDBG("buildDmTndDFPropsCmd: Error: Unkown token: %d", pScanner->curtok->tagid);
              rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeDmTndDFProps(pDmTndDFProps);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeDmTndDFProps(pDmTndDFProps);
            return rc;
        }
    }

    *ppElem = pDmTndDFProps;

    return SML_ERR_OK;
}

Ret_t buildDmTndNodeList(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) 
{
    SmlDmTndNodeListPtr_t pElem = NULL, pPrev = NULL;

    KCDBG("buildDmTndNodeList: Enter");    
    
    pElem = (SmlDmTndNodeListPtr_t) *ppElem;
    /* advance to the end of the list, and create ther an empty list element */
    while (pElem != NULL) {
        pPrev = pElem;
        pElem = pPrev->next;
    }
    if ((pElem = (SmlDmTndNodeListPtr_t)smlLibMalloc(sizeof(SmlDmTndNodeListPtr_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pElem, 0, sizeof(SmlDmTndNodeListPtr_t));
    pElem->next = NULL;
    if (pPrev != NULL) /* we already had some entries in the list */
        pPrev->next = pElem;
    else /* nope we created a new list */
        *ppElem = pElem;
    pElem->node = NULL;
    /* at this point pElem should point to an valid list element */
    return buildDmTndNodeCmd(pDecoder, (VoidPtr_t)&pElem->node);
}

/* decoder callbacks */
Ret_t buildDmTndNodeCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlDmTndNodePtr_t pDmTndNode;
    Ret_t rc;

    KCDBG("buildDmTndNodeCmd: Enter");    
    
    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pDmTndNode = (SmlDmTndNodePtr_t)smlLibMalloc(sizeof(SmlDmTndNode_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pDmTndNode, 0, sizeof(SmlDmTndNode_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pDmTndNode;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlFreeDmTndNode(pDmTndNode);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
          case TN_DMTND_NodeName:
            KCDBG("buildDmTndNodeCmd: NodeName");
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndNode->nodename);
            if (rc == SML_ERR_OK) {
                KCDBG("buildDmTndNodeCmd:    Node name =  %s", pDmTndNode->nodename->content);
            }
            break;
          case TN_DMTND_Path:
            KCDBG("buildDmTndNodeCmd: Path\n");
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndNode->path);
            if (rc == SML_ERR_OK) {
                KCDBG("buildDmTndNodeCmd:    path =  %s", pDmTndNode->path->content);
            }
            break;
          case TN_DMTND_RTProperties:
            KCDBG("buildDmTndNodeCmd: RTProperties");
            rc = buildDmTndRTPropsCmd(pDecoder, (VoidPtr_t)&pDmTndNode->rtprops);
            break;
          case TN_DMTND_DFProperties:
            KCDBG("buildDmTndNodeCmd: DFProperties");
            rc = buildDmTndDFPropsCmd(pDecoder, (VoidPtr_t)&pDmTndNode->dfprops);
            break;
          case TN_DMTND_Value:
            KCDBG("buildDmTndNodeCmd: Value");
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTndNode->value);
            if (rc == SML_ERR_OK) {
                KCDBG("buildDmTndNodeCmd:    Value = %s", pDmTndNode->value->content);
            }
            break;
          case TN_DMTND_Node:            
            KCDBG("buildDmTndNodeCmd: Node");
            rc = buildDmTndNodeList(pDecoder, (VoidPtr_t)&pDmTndNode->nodelist);
            break;
          default:
              KCDBG("buildDmTndNodeCmd: Unkown token: %d", pScanner->curtok->tagid);
              rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            if ( NULL != pDmTndNode->nodename && NULL != pDmTndNode->nodename->content )
            {
              KCDBG("buildDmTndNodeCmd: Error: Node %s corrupted", (char *)pDmTndNode->nodename->content);
            }
            smlFreeDmTndNode(pDmTndNode);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeDmTndNode(pDmTndNode);
            return rc;
        }
    }

    *ppElem = pDmTndNode;

    return SML_ERR_OK;
}

/* decoder callbacks */
Ret_t buildDmTndCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem)
{
    XltDecScannerPtr_t pScanner;
    SmlDmTndPtr_t pDmTnd;
    Ret_t rc;

    KCDBG("buildDmTndCmd: Enter");
      
    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pDmTnd = (SmlDmTndPtr_t)smlLibMalloc(sizeof(SmlDmTnd_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pDmTnd, 0, sizeof(SmlDmTnd_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pDmTnd;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlFreeDmTnd(pDmTnd);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
          case TN_DMTND_MgmtTree: 
            KCDBG("buildDmTndCmd: MgmtTree tag");
            break;
          case TN_DMTND_VerDTD:
            KCDBG("buildDmTndCmd: VerDTD tag");
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTnd->verdtd);
            break;
          case TN_DMTND_Man:
            KCDBG("buildDmTndCmd: Man tag");
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTnd->man);
            break;
          case TN_DMTND_Mod:
            KCDBG("buildDmTndCmd: Mod tag");
            rc = buildPCData(pDecoder, (VoidPtr_t)&pDmTnd->mod);
            break;
          case TN_DMTND_Node:
            KCDBG("buildDmTndCmd: Node tag");
            rc = buildDmTndNodeList(pDecoder, (VoidPtr_t)&pDmTnd->nodelist);
            break;
          default:
              KCDBG(">>>>> buildDmTndDFPropsCmd: Unkown token: %d <<<<<", pScanner->curtok->tagid);
              rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlFreeDmTnd(pDmTnd);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlFreeDmTnd(pDmTnd);
            return rc;
        }
    }

    *ppElem = pDmTnd;

    return SML_ERR_OK;
}

/* see xltenc.c:XltEncBlock for description of parameters */
Ret_t dmtndEncBlock(XltTagID_t tagId, XltRO_t reqOptFlag, const VoidPtr_t pContent, SmlEncoding_t enc, BufferMgmtPtr_t pBufMgr, SmlPcdataExtension_t attFlag) 
{
    if ((reqOptFlag == REQUIRED) && (pContent == NULL))
    {
        KCDBG(">>>>> TNDS required tag: %d missed ! <<<<<", tagId);
        return SML_ERR_XLT_MISSING_CONT;
    }
    //Check if pContent of a optional field is missing -> if yes we are done
    else if (pContent == NULL)
        return SML_ERR_OK;
  
    Ret_t _err;
    SmlDmTndNodeListPtr_t nodeList = NULL;

    //Generate the commands -> see DTD
    switch (tagId) 
    {
        case TN_DMTND_MgmtTree:
            if ((_err = xltGenerateTag(TN_DMTND_MgmtTree, TT_BEG, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_VerDTD, REQUIRED, ((SmlDmTndPtr_t) pContent)->verdtd, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_Man, OPTIONAL, ((SmlDmTndPtr_t) pContent)->man, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_Mod, OPTIONAL, ((SmlDmTndPtr_t) pContent)->mod, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            nodeList = ((SmlDmTndPtr_t)pContent)->nodelist;
            while (nodeList != NULL) 
            {        
               if ((_err = dmtndEncBlock(TN_DMTND_Node, OPTIONAL, nodeList->node, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
               nodeList = nodeList->next;
            };
            if ((_err = xltGenerateTag(TN_DMTND_MgmtTree, TT_END, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            break;
        case TN_DMTND_Node:
            if ((_err = xltGenerateTag(TN_DMTND_Node, TT_BEG, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_NodeName, REQUIRED, ((SmlDmTndNodePtr_t) pContent)->nodename, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_Path, OPTIONAL, ((SmlDmTndNodePtr_t) pContent)->path, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_RTProperties, OPTIONAL, ((SmlDmTndNodePtr_t) pContent)->rtprops, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_Value, OPTIONAL, ((SmlDmTndNodePtr_t) pContent)->value, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            nodeList = ((SmlDmTndNodePtr_t)pContent)->nodelist;
            while (nodeList != NULL) 
            {        
               if ((_err = dmtndEncBlock(TN_DMTND_Node, OPTIONAL, nodeList->node, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
               nodeList = nodeList->next;
            };
            if ((_err = xltGenerateTag(TN_DMTND_Node, TT_END, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            break;
        case TN_DMTND_RTProperties:
            if ((_err = xltGenerateTag(TN_DMTND_RTProperties, TT_BEG, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_ACL, OPTIONAL, ((SmlDmTndRTPropsPtr_t) pContent)->acl, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_Format, OPTIONAL, ((SmlDmTndRTPropsPtr_t) pContent)->format, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_Name, OPTIONAL, ((SmlDmTndRTPropsPtr_t) pContent)->name, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_Size, OPTIONAL, ((SmlDmTndRTPropsPtr_t) pContent)->size, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_Title, OPTIONAL, ((SmlDmTndRTPropsPtr_t) pContent)->title, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_TStamp, OPTIONAL, ((SmlDmTndRTPropsPtr_t) pContent)->tstamp, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_Type, REQUIRED, ((SmlDmTndRTPropsPtr_t) pContent)->type, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = dmtndEncBlock(TN_DMTND_VerNo, OPTIONAL, ((SmlDmTndRTPropsPtr_t) pContent)->verno, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ((_err = xltGenerateTag(TN_DMTND_RTProperties, TT_END, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            break;
        case TN_DMTND_Format:
            if ( NULL == pContent || NULL == ((SmlDmTndFormatPtr_t) pContent)->value || NULL == ((SmlDmTndFormatPtr_t) pContent)->value->content )
                break;
            if ((_err = xltGenerateTag(TN_DMTND_Format, TT_BEG, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ( !strcmp("b64", (char *)(((SmlDmTndFormatPtr_t) pContent)->value->content)) )
            {
               if ((_err = xltGenerateTag(TN_DMTND_b64, TT_ALL, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            else if ( !strcmp("bin", (char *)(((SmlDmTndFormatPtr_t) pContent)->value->content)) )
            {
               if ((_err = xltGenerateTag(TN_DMTND_bin, TT_ALL, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            else if ( !strcmp("bool", (char *)(((SmlDmTndFormatPtr_t) pContent)->value->content)) )
            {
               if ((_err = xltGenerateTag(TN_DMTND_bool, TT_ALL, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            else if ( !strcmp("chr", (char *)(((SmlDmTndFormatPtr_t) pContent)->value->content)) )
            {
               if ((_err = xltGenerateTag(TN_DMTND_chr, TT_ALL, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            else if ( !strcmp("int", (char *)(((SmlDmTndFormatPtr_t) pContent)->value->content)) )
            {
               if ((_err = xltGenerateTag(TN_DMTND_int, TT_ALL, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            else if ( !strcmp("node", (char *)(((SmlDmTndFormatPtr_t) pContent)->value->content)) )
            {
               if ((_err = xltGenerateTag(TN_DMTND_node, TT_ALL, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            else if ( !strcmp("null", (char *)(((SmlDmTndFormatPtr_t) pContent)->value->content)) )
            {
               if ((_err = xltGenerateTag(TN_DMTND_null, TT_ALL, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            else if ( !strcmp("xml", (char *)(((SmlDmTndFormatPtr_t) pContent)->value->content)) )
            {
               if ((_err = xltGenerateTag(TN_DMTND_xml, TT_ALL, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            else if ( !strcmp("date", (char *)(((SmlDmTndFormatPtr_t) pContent)->value->content)) )
            {
               if ((_err = xltGenerateTag(TN_DMTND_date, TT_ALL, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            else if ( !strcmp("time", (char *)(((SmlDmTndFormatPtr_t) pContent)->value->content)) )
            {
               if ((_err = xltGenerateTag(TN_DMTND_time, TT_ALL, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            else if ( !strcmp("float", (char *)(((SmlDmTndFormatPtr_t) pContent)->value->content)) )
            {
               if ((_err = xltGenerateTag(TN_DMTND_float, TT_ALL, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            else 
            {
              KCDBG(">>>>>  Unkown format: %s <<<<<\n",  (char *)(((SmlDmTndFormatPtr_t)pContent)->value->content));
              return SML_ERR_XLT_INVAL_SYNCML_DOC;
            }
            if ((_err = xltGenerateTag(TN_DMTND_Format, TT_END, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            break;
        case TN_DMTND_Type:
            if ((_err = xltGenerateTag(TN_DMTND_Type, TT_BEG, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            if ( ((SmlDmTndTypePtr_t) pContent)->mime ) 
            {
              if ((_err = dmtndEncBlock(TN_DMTND_MIME, OPTIONAL, ((SmlDmTndTypePtr_t) pContent)->mime, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            else if ( ((SmlDmTndTypePtr_t) pContent)->ddfname ) 
            {
              if ((_err = dmtndEncBlock(TN_DMTND_DDFName, OPTIONAL, ((SmlDmTndTypePtr_t) pContent)->ddfname, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            }
            if ((_err = xltGenerateTag(TN_DMTND_Type, TT_END, enc, pBufMgr, SML_EXT_DMTND)) != SML_ERR_OK) return _err;
            break;
         default: // all leaf nodes (PCDATA#)
            return xltEncPcdata(tagId, reqOptFlag, pContent, enc, pBufMgr, attFlag);
    } /* eof switch tagid */
    return SML_ERR_OK;
}

#endif /* __USE_DMTND__ */
