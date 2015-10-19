/*************************************************************************/
/* module:          DeviceInf DTD related functions for the en-/decoder  */
/* file:            xltdevinf.c                                          */
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

#include "define.h"
#ifdef __USE_DEVINF__

#include "smldevinfdtd.h"
#include "xlttags.h"
#include "xltdevinf.h"
#include "xlttagtbl.h"
#include "xltenc.h"
#include "xltencwbxml.h"

#include <libstr.h>
#include <smlerr.h>
#include <smldtd.h>
#include <libmem.h>
#include <libutil.h>

/* decoder callbacks */
Ret_t buildDevInfDevInfContent(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem);
Ret_t buildDevInfCtcap(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem);


Ret_t buildDevInfDevInfCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) {
    KCDBG("buildDevInfDevInfCmd: Enter\n");
    
    XltDecScannerPtr_t pScanner;
    SmlDevInfDevInfPtr_t pElem = NULL;
    Ret_t rc;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pElem;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pElem);
        return rc;
    }
    
    switch (pScanner->curtok->tagid) {
        case TN_DEVINF_DEVINF:
            rc = buildDevInfDevInfContent(pDecoder, (VoidPtr_t)&pElem);
            break;
        default:
            rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
    }
    if (rc != SML_ERR_OK) {
        smlLibFree(pElem);
        return rc;
    }
    *ppElem = pElem;

    KCDBG("buildDevInfDevInfCmd: Leave\n");
    
    return SML_ERR_OK;
}

Ret_t buildDevInfDevInfContent(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) {
    XltDecScannerPtr_t pScanner;
    SmlDevInfDevInfPtr_t pElem;
    Ret_t rc;
	/* Modified by Tomy to allow <UTC></UTC>, <SupportNumberOfChanges></SupportNumberOfChanges> and <SupportLargeObjs></SupportLargeObjs> */
	SmlPcdataPtr_t tmp_ptr;
	/* End modified by Tomy */

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pElem = (SmlDevInfDevInfPtr_t)smlLibMalloc(sizeof(SmlDevInfDevInf_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pElem, 0, sizeof(SmlDevInfDevInf_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pElem;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pElem);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
            case TN_DEVINF_VERDTD:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->verdtd);
                break;
            case TN_DEVINF_MAN:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->man);
                break;
            case TN_DEVINF_MOD:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->mod);
                break;
            case TN_DEVINF_OEM:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->oem);
                break;
            case TN_DEVINF_FWV:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->fwv);
                break;
            case TN_DEVINF_SWV:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->swv);
                break;
            case TN_DEVINF_HWV:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->hwv);
                break;
            case TN_DEVINF_DEVID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->devid);
                break;
            case TN_DEVINF_DEVTYP:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->devtyp);
                break;
            case TN_DEVINF_DATASTORE:
                rc = buildDevInfDataStoreList(pDecoder, (VoidPtr_t)&pElem->datastore);
                break;
            case TN_DEVINF_CTCAP:
                rc = buildDevInfCtcap(pDecoder, (VoidPtr_t)&pElem->ctcap);
                break;
            case TN_DEVINF_EXT:
                rc = buildDevInfExtList(pDecoder, (VoidPtr_t)&pElem->ext);
                break;
            /* SCTSTK - 18/03/2002 S.H. 2002-04-05 : SyncML 1.1 */
            case TN_DEVINF_UTC:
                pElem->flags |= SmlDevInfUTC_f;
				/* Modified by Tomy to allow <UTC></UTC> */
				tmp_ptr = NULL;
				rc = buildPCData(pDecoder, (VoidPtr_t)&tmp_ptr);
				if (tmp_ptr->contentType != SML_PCDATA_UNDEFINED && tmp_ptr->extension != SML_EXT_UNDEFINED && tmp_ptr->length != 0 && tmp_ptr->content != NULL) rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
				/* End modified by Tomy */
                break;
            case TN_DEVINF_NOFM:
                pElem->flags |= SmlDevInfNOfM_f;
				/* Modified by Tomy to allow <SupportNumberOfChanges></SupportNumberOfChanges> */
				tmp_ptr = NULL;
				rc = buildPCData(pDecoder, (VoidPtr_t)&tmp_ptr);
				if (tmp_ptr->contentType != SML_PCDATA_UNDEFINED && tmp_ptr->extension != SML_EXT_UNDEFINED && tmp_ptr->length != 0 && tmp_ptr->content != NULL) rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
				/* End modified by Tomy */
               break;
            case TN_DEVINF_LARGEOBJECT:
                pElem->flags |= SmlDevInfLargeObject_f;
				/* Modified by Tomy to allow <SupportLargeObjs></SupportLargeObjs> */
				tmp_ptr = NULL;
				rc = buildPCData(pDecoder, (VoidPtr_t)&tmp_ptr);
				if (tmp_ptr->contentType != SML_PCDATA_UNDEFINED && tmp_ptr->extension != SML_EXT_UNDEFINED && tmp_ptr->length != 0 && tmp_ptr->content != NULL) rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
				/* End modified by Tomy */
               break;
            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlLibFree(pElem);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlLibFree(pElem);
            return rc;
        }
    }
    *ppElem = pElem;

    return SML_ERR_OK;
}

Ret_t buildDevInfDataStoreCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) {
    XltDecScannerPtr_t pScanner;
    SmlDevInfDatastorePtr_t pElem;
    Ret_t rc;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pElem = (SmlDevInfDatastorePtr_t)smlLibMalloc(sizeof(SmlDevInfDatastore_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pElem, 0, sizeof(SmlDevInfDatastore_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pElem;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pElem);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
		    /* PCDATA elements */
            case TN_DEVINF_SOURCEREF:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->sourceref);
                break;
            case TN_DEVINF_DISPLAYNAME:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->displayname);
                break;
            case TN_DEVINF_MAXGUIDSIZE:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->maxguidsize);
                break;
            case TN_DEVINF_RXPREF:
                rc = buildDevInfXmitCmd(pDecoder, (VoidPtr_t)&pElem->rxpref);
                break;
            case TN_DEVINF_TXPREF:
                rc = buildDevInfXmitCmd(pDecoder, (VoidPtr_t)&pElem->txpref);
                break;
            case TN_DEVINF_RX:
                rc = buildDevInfXmitList(pDecoder, (VoidPtr_t)&pElem->rx);
                break;
            case TN_DEVINF_TX:
                rc = buildDevInfXmitList(pDecoder, (VoidPtr_t)&pElem->tx);
                break;
            case TN_DEVINF_DSMEM:
                rc = buildDevInfDSMemCmd(pDecoder, (VoidPtr_t)&pElem->dsmem);
                break;
            case TN_DEVINF_SYNCCAP:
                rc = buildDevInfSyncCapCmd(pDecoder, (VoidPtr_t)&pElem->synccap);
                break;
            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlLibFree(pElem);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlLibFree(pElem);
            return rc;
        }
    }
    *ppElem = pElem;

    return SML_ERR_OK;
}

Ret_t buildDevInfXmitCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) {
    XltDecScannerPtr_t pScanner;
    SmlDevInfXmitPtr_t pXmit;
    Ret_t rc;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pXmit = (SmlDevInfXmitPtr_t)smlLibMalloc(sizeof(SmlDevInfXmit_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pXmit, 0, sizeof(SmlDevInfXmit_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pXmit;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pXmit);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
		    /* PCDATA elements */
            case TN_DEVINF_CTTYPE:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pXmit->cttype);
                break;
            case TN_DEVINF_VERCT:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pXmit->verct);
                break;
            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlLibFree(pXmit);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlLibFree(pXmit);
            return rc;
        }
    }
    *ppElem = pXmit;

    return SML_ERR_OK;
}

Ret_t buildDevInfXmitList(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) {
    SmlDevInfXmitListPtr_t pElem = NULL, pPrev = NULL;

    pElem = (SmlDevInfXmitListPtr_t) *ppElem;

    /* advance to the end of the list, and create ther an empty list element */
    while (pElem != NULL) {
	    pPrev = pElem;
	    pElem = pPrev->next;
    }
    if ((pElem = (SmlDevInfXmitListPtr_t)smlLibMalloc(sizeof(SmlDevInfXmitList_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pElem, 0, sizeof(SmlDevInfXmitList_t));
    if (pPrev != NULL) /* we already had some entries in the list */
	    pPrev->next = pElem;
    else /* nope we created a new list */
	    *ppElem = pElem;
    pElem->data = NULL;
    /* at this point pElem should point to an valid list element */
    return buildDevInfXmitCmd(pDecoder, (VoidPtr_t)&pElem->data);
}

Ret_t buildDevInfDataStoreList(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) {
    SmlDevInfDatastoreListPtr_t pElem = NULL, pPrev = NULL;

    pElem = (SmlDevInfDatastoreListPtr_t) *ppElem;

    /* advance to the end of the list, and create ther an empty list element */
    while (pElem != NULL) {
	    pPrev = pElem;
	    pElem = pPrev->next;
    }
    if ((pElem = (SmlDevInfDatastoreListPtr_t)smlLibMalloc(sizeof(SmlDevInfDatastoreList_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pElem, 0, sizeof(SmlDevInfDatastoreList_t));
    if (pPrev != NULL) /* we already had some entries in the list */
	    pPrev->next = pElem;
    else /* nope we created a new list */
	    *ppElem = pElem;
    pElem->data = NULL;
    /* at this point pElem should point to an valid list element */
    return buildDevInfDataStoreCmd(pDecoder, (VoidPtr_t)&pElem->data);
}

Ret_t buildDevInfExtList(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) {
    SmlDevInfExtListPtr_t pElem = NULL, pPrev = NULL;

    pElem = (SmlDevInfExtListPtr_t) *ppElem;

    /* advance to the end of the list, and create ther an empty list element */
    while (pElem != NULL) {
	    pPrev = pElem;
	    pElem = pPrev->next;
    }
    if ((pElem = (SmlDevInfExtListPtr_t)smlLibMalloc(sizeof(SmlDevInfExtList_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pElem, 0, sizeof(SmlDevInfExtList_t));
    if (pPrev != NULL) /* we already had some entries in the list */
	    pPrev->next = pElem;
    else /* nope we created a new list */
	    *ppElem = pElem;
    pElem->data = NULL;
    /* at this point pElem should point to an valid list element */
    return buildDevInfExtCmd(pDecoder, (VoidPtr_t)&pElem->data);
}


Ret_t buildDevInfCtcap(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) {
    SmlDevInfCtcapListPtr_t       pCtcap        = NULL, pPrev = NULL;
    SmlDevInfCTDataPropListPtr_t  pOldProp      = NULL, pProp = NULL;
    SmlDevInfCTDataListPtr_t      pOldParam     = NULL, pParam = NULL;
    SmlDevInfCtcapListPtr_t       pElem         = NULL;
    XltDecScannerPtr_t            pScanner;
    Ret_t rc;

    pElem = (SmlDevInfCtcapListPtr_t) *ppElem;
    pScanner = pDecoder->scanner;

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pElem;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pElem);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
        case TN_DEVINF_CTTYPE:        
            pCtcap = (SmlDevInfCtcapListPtr_t) *ppElem;
            /* advance to the end of the list, and create ther an empty list element */
            while (pCtcap != NULL) {
	            pPrev = pCtcap;
	            pCtcap = pPrev->next;
            }
            if ((pCtcap = (SmlDevInfCtcapListPtr_t)smlLibMalloc(sizeof(SmlDevInfCtcapList_t))) == NULL)
                return SML_ERR_NOT_ENOUGH_SPACE;
            smlLibMemset(pCtcap, 0, sizeof(SmlDevInfCtcapList_t));
            if (pPrev != NULL) /* we already had some entries in the list */
	            pPrev->next = pCtcap;
            else /* nope we created a new list */
	            *ppElem = pCtcap;
            pCtcap->data = (SmlDevInfCTCapPtr_t)smlLibMalloc(sizeof(SmlDevInfCTCap_t));
            if (pCtcap->data == NULL) return SML_ERR_NOT_ENOUGH_SPACE;
            smlLibMemset(pCtcap->data, 0, sizeof(SmlDevInfCTCap_t));
            rc = buildPCData(pDecoder, (VoidPtr_t)&pCtcap->data->cttype);
            break;
        case TN_DEVINF_PROPNAME:
            pCtcap = (SmlDevInfCtcapListPtr_t) *ppElem;
            if (pCtcap == NULL) return SML_ERR_XLT_INVAL_SYNCML_DOC;
            while (pCtcap->next != NULL) {
                pPrev = pCtcap;
                pCtcap = pCtcap->next;
            }
            /* here we are at the latest defined DevInfCTCapPtr_t */
            /* now we need to create a new DevInfCTDataPtr_t element, tostore the properties name */
            pOldProp = NULL;
            pProp    = pCtcap->data->prop;
            while (pProp != NULL) {
                pOldProp = pProp;
                pProp = pProp->next;
            }
            pProp = (SmlDevInfCTDataPropListPtr_t) smlLibMalloc(sizeof(SmlDevInfCTDataPropList_t));
            if (pProp == NULL) return SML_ERR_NOT_ENOUGH_SPACE;
            smlLibMemset(pProp, 0, sizeof(SmlDevInfCTDataPropList_t));
            if (pOldProp != NULL)
                pOldProp->next = pProp;
            else 
                pCtcap->data->prop = pProp;
            pProp->data = (SmlDevInfCTDataPropPtr_t)smlLibMalloc(sizeof(SmlDevInfCTDataProp_t));
            if (pProp->data == NULL) return SML_ERR_NOT_ENOUGH_SPACE;
            smlLibMemset(pProp->data, 0, sizeof(SmlDevInfCTDataProp_t));
            pProp->data->prop = (SmlDevInfCTDataPtr_t)smlLibMalloc(sizeof(SmlDevInfCTData_t));
            if (pProp->data->prop == NULL) return SML_ERR_NOT_ENOUGH_SPACE;
            smlLibMemset(pProp->data->prop, 0, sizeof(SmlDevInfCTData_t));
            rc = buildPCData(pDecoder, (VoidPtr_t)&pProp->data->prop->name);
            break;
        case TN_DEVINF_PARAMNAME:
            pCtcap = (SmlDevInfCtcapListPtr_t) *ppElem;
            if (pCtcap == NULL) return SML_ERR_XLT_INVAL_SYNCML_DOC;
            while (pCtcap->next != NULL) {
                pPrev = pCtcap;
                pCtcap = pCtcap->next;
            }
            /* here we are at the latest defined DevInfCTCapPtr_t */
            pProp    = pCtcap->data->prop;
            if (pProp == NULL) return SML_ERR_XLT_INVAL_SYNCML_DOC;
            while (pProp->next != NULL) {
                pProp = pProp->next;
            }
            /* here we are at the latest defined PropList Element in the latest defined CTCap element */
            /* now lets insert a new Param element into this property */
            pOldParam = NULL;
            pParam = pProp->data->param;
            while (pParam != NULL) {
                pOldParam = pParam;
                pParam    = pParam->next;
            }
            pParam = (SmlDevInfCTDataListPtr_t)smlLibMalloc(sizeof(SmlDevInfCTDataList_t));
            if (pParam == NULL) return SML_ERR_NOT_ENOUGH_SPACE;
            smlLibMemset(pParam, 0, sizeof(SmlDevInfCTDataList_t));
            if (pOldParam != NULL)
                pOldParam->next = pParam;
            else
                pProp->data->param = pParam;
            pParam->data = (SmlDevInfCTDataPtr_t)smlLibMalloc(sizeof(SmlDevInfCTData_t));
            if (pParam->data == NULL) return SML_ERR_NOT_ENOUGH_SPACE;
            smlLibMemset(pParam->data, 0, sizeof(SmlDevInfCTData_t));
            rc = buildPCData(pDecoder, (VoidPtr_t)&pParam->data->name);
            break;
        case TN_DEVINF_DISPLAYNAME:
        case TN_DEVINF_VALENUM:
        case TN_DEVINF_DATATYPE:
        case TN_DEVINF_SIZE:
            /* The code for the above 4 is basically the same.
             * The hardpart is finding the right SmlDevInfCTDataPtr_t
             * struct, as it can be either within a Property or an Parameter.
             */
            pCtcap = (SmlDevInfCtcapListPtr_t) *ppElem;
            if (pCtcap == NULL) return SML_ERR_XLT_INVAL_SYNCML_DOC;
            while (pCtcap->next != NULL) {
                pCtcap = pCtcap->next;
            }
            /* here we are at the latest defined DevInfCTCapPtr_t */
            pProp    = pCtcap->data->prop;
            if (pProp == NULL) return SML_ERR_XLT_INVAL_SYNCML_DOC;
            while (pProp->next != NULL) {
                pProp = pProp->next;
            }

            if (pProp->data == NULL) return SML_ERR_XLT_INVAL_SYNCML_DOC;
            if (pProp->data->prop == NULL) return SML_ERR_XLT_INVAL_SYNCML_DOC;
            if (pProp->data->param == NULL) {
                /* No Param's yet so we have Property fields to fill */
                switch(pScanner->curtok->tagid) {
                case TN_DEVINF_DISPLAYNAME:
                    rc = buildPCData(pDecoder, (VoidPtr_t)&pProp->data->prop->dname);
                    break;
                case TN_DEVINF_VALENUM:
                    rc = buildPCDataList(pDecoder, (VoidPtr_t)&pProp->data->prop->valenum);
                    break;
                case TN_DEVINF_DATATYPE:
                    rc = buildPCData(pDecoder, (VoidPtr_t)&pProp->data->prop->datatype);
                    break;
                case TN_DEVINF_SIZE:
                    rc = buildPCData(pDecoder, (VoidPtr_t)&pProp->data->prop->size);
                    break;
                default:
                    break;
                }
            } else {
                pParam = pProp->data->param;
                while (pParam->next != NULL) {
                    pParam = pParam->next;
                }
                if (pParam->data == NULL) return SML_ERR_XLT_INVAL_SYNCML_DOC;
                switch(pScanner->curtok->tagid) {
                case TN_DEVINF_DISPLAYNAME:
                    rc = buildPCData(pDecoder, (VoidPtr_t)&pParam->data->dname);
                    break;
                case TN_DEVINF_VALENUM:
                    rc = buildPCDataList(pDecoder, (VoidPtr_t)&pParam->data->valenum);
                    break;
                case TN_DEVINF_DATATYPE:
                    rc = buildPCData(pDecoder, (VoidPtr_t)&pParam->data->datatype);
                    break;
                case TN_DEVINF_SIZE:
                    rc = buildPCData(pDecoder, (VoidPtr_t)&pParam->data->size);
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlLibFree(pElem);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlLibFree(pElem);
            return rc;
        }
    } /* eof while */
    pElem = *ppElem;
    return SML_ERR_OK;
}

Ret_t buildDevInfDSMemCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) {
    XltDecScannerPtr_t pScanner;
    SmlDevInfDSMemPtr_t pElem;
    Ret_t rc;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pElem = (SmlDevInfDSMemPtr_t)smlLibMalloc(sizeof(SmlDevInfDSMem_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pElem, 0, sizeof(SmlDevInfDSMem_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pElem;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pElem);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
    		    /* PCDATA elements */
            case TN_DEVINF_SHAREDMEM:
                // %%% luz:2003-04-28: made work as a flag
                pElem->flags |= SmlDevInfSharedMem_f;
                // rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->shared);
                break;
            case TN_DEVINF_MAXMEM:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->maxmem);
                break;
            case TN_DEVINF_MAXID:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->maxid);
                break;
            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlLibFree(pElem);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlLibFree(pElem);
            return rc;
        }
    }
    *ppElem = pElem;

    return SML_ERR_OK;
}

Ret_t buildDevInfCTCapCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) {
    XltDecScannerPtr_t pScanner;
    SmlDevInfCTCapPtr_t pElem;
    Ret_t rc;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pElem = (SmlDevInfCTCapPtr_t)smlLibMalloc(sizeof(SmlDevInfCTCap_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pElem, 0, sizeof(SmlDevInfCTCap_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pElem;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pElem);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
            case TN_DEVINF_CTTYPE:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->cttype);
                break;
            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlLibFree(pElem);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlLibFree(pElem);
            return rc;
        }
    }
    *ppElem = pElem;

    return SML_ERR_OK;
}

Ret_t buildDevInfSyncCapCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) {
    XltDecScannerPtr_t pScanner;
    SmlDevInfSyncCapPtr_t pElem;
    Ret_t rc;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pElem = (SmlDevInfSyncCapPtr_t)smlLibMalloc(sizeof(SmlDevInfSyncCap_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pElem, 0, sizeof(SmlDevInfSyncCap_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pElem;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pElem);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
            case TN_DEVINF_SYNCTYPE:
                rc = buildPCDataList(pDecoder, (VoidPtr_t)&pElem->synctype);
                break;
            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlLibFree(pElem);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlLibFree(pElem);
            return rc;
        }
    }
    *ppElem = pElem;

    return SML_ERR_OK;
}

Ret_t buildDevInfExtCmd(XltDecoderPtr_t pDecoder, VoidPtr_t *ppElem) {
    XltDecScannerPtr_t pScanner;
    SmlDevInfExtPtr_t pElem;
    Ret_t rc;

    pScanner = pDecoder->scanner;

    if (*ppElem != NULL)
        return SML_ERR_XLT_INVAL_SYNCML_DOC;

    if ((pElem = (SmlDevInfExtPtr_t)smlLibMalloc(sizeof(SmlDevInfExt_t))) == NULL)
        return SML_ERR_NOT_ENOUGH_SPACE;
    smlLibMemset(pElem, 0, sizeof(SmlDevInfExt_t));

    if (IS_EMPTY(pScanner->curtok)) {
        *ppElem = pElem;
        return SML_ERR_OK;
    }

    if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
        smlLibFree(pElem);
        return rc;
    }

    while (pScanner->curtok->type != TOK_TAG_END) {
        switch (pScanner->curtok->tagid) {
            case TN_DEVINF_XNAM:
                rc = buildPCData(pDecoder, (VoidPtr_t)&pElem->xnam);
                break;
            case TN_DEVINF_XVAL:
                rc = buildPCDataList(pDecoder, (VoidPtr_t)&pElem->xval);
                break;
            default:
                rc = SML_ERR_XLT_INVAL_SYNCML_DOC;
        }
        if (rc != SML_ERR_OK) {
            smlLibFree(pElem);
            return rc;
        }
        if (((rc = nextToken(pDecoder)) != SML_ERR_OK)) {
            smlLibFree(pElem);
            return rc;
        }
    }
    *ppElem = pElem;

    return SML_ERR_OK;
}


/* see xltenc.c:XltEncBlock for description of parameters */
Ret_t devinfEncBlock(XltTagID_t tagId, XltRO_t reqOptFlag, const VoidPtr_t pContent, SmlEncoding_t enc, BufferMgmtPtr_t pBufMgr, SmlPcdataExtension_t attFlag) {
	//Return variable
	Ret_t _err;
	SmlPcdataListPtr_t           pList     = NULL, p2List = NULL;
    SmlDevInfDatastoreListPtr_t  dsList    = NULL;
    SmlDevInfCtcapListPtr_t      ctList    = NULL;
    SmlDevInfExtListPtr_t        exList    = NULL;
    SmlDevInfXmitListPtr_t       xmList    = NULL;
    SmlDevInfCTDataPropListPtr_t propList  = NULL;
    SmlDevInfCTDataListPtr_t     paramList = NULL;

    //Check if pContent of a required field is missing
	if ((reqOptFlag == REQUIRED) && (pContent == NULL))
		return SML_ERR_XLT_MISSING_CONT;
	//Check if pContent of a optional field is missing -> if yes we are done
	else if (pContent == NULL)
		return SML_ERR_OK;
  
	//Generate the commands -> see DTD
	switch (tagId) {
  	case TN_DEVINF_EXT:
  		if ((_err = xltGenerateTag(TN_DEVINF_EXT, TT_BEG, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
  		if ((_err = devinfEncBlock(TN_DEVINF_XNAM, REQUIRED, ((SmlDevInfExtPtr_t) pContent)->xnam, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
  		pList = ((SmlDevInfExtPtr_t)pContent)->xval;
          while (pList != NULL) {        
              if ((_err = devinfEncBlock(TN_DEVINF_XVAL, OPTIONAL, pList->data, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
              pList = pList->next;
          };
  		if ((_err = xltGenerateTag(TN_DEVINF_EXT, TT_END, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
  		break;
  	case TN_DEVINF_SYNCCAP:
  		if ((_err = xltGenerateTag(TN_DEVINF_SYNCCAP, TT_BEG, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
  		pList = ((SmlDevInfSyncCapPtr_t)pContent)->synctype;
          while (pList != NULL) {        
              if ((_err = devinfEncBlock(TN_DEVINF_SYNCTYPE, OPTIONAL, pList->data, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
              pList = pList->next;
          };
  		if ((_err = xltGenerateTag(TN_DEVINF_SYNCCAP, TT_END, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
  		break;
  	case TN_DEVINF_SHAREDMEM:                  
      //set the flag in the (WB)XML document if the flag is in the pContent
      if ((*((Flag_t *) pContent)) & (SmlDevInfSharedMem_f))
          if ((_err = xltGenerateTag(tagId, TT_ALL, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
      break;
    // %%% luz:2003-04-28 added missing 1.1 devinf tags here
	  case TN_DEVINF_UTC:
	    //set the flag in the (WB)XML document if the flag is in the pContent
      if ((*((Flag_t *) pContent)) & (SmlDevInfUTC_f)) {
        if ((_err = xltGenerateTag(tagId, TT_ALL, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
      }
      break;
	  case TN_DEVINF_NOFM:
	    //set the flag in the (WB)XML document if the flag is in the pContent
      if ((*((Flag_t *) pContent)) & (SmlDevInfNOfM_f)) {
        if ((_err = xltGenerateTag(tagId, TT_ALL, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
      }
      break;
	  case TN_DEVINF_LARGEOBJECT:
	    //set the flag in the (WB)XML document if the flag is in the pContent
      if ((*((Flag_t *) pContent)) & (SmlDevInfLargeObject_f)) {
        if ((_err = xltGenerateTag(tagId, TT_ALL, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
      }
      break;
    // %%% end luz
    case TN_DEVINF_CTCAP:
		if ((_err = xltGenerateTag(TN_DEVINF_CTCAP, TT_BEG, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
        ctList = ((SmlDevInfCtcapListPtr_t)pContent);
        if (ctList == NULL) return SML_ERR_XLT_INVAL_INPUT_DATA;
        while (ctList != NULL) { 
            if (ctList->data == NULL) return SML_ERR_XLT_INVAL_INPUT_DATA;
            if ((_err = devinfEncBlock(TN_DEVINF_CTTYPE, OPTIONAL, ctList->data->cttype, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
            /* now the propList */
            // %%% luz 2002-11-27: made property list optional (e.g. text/message of P800 has none)
            propList = ctList->data->prop;
            // %%% original: if (propList == NULL) return SML_ERR_XLT_INVAL_INPUT_DATA;
            while (propList != NULL) {
                if (propList->data == NULL) return SML_ERR_XLT_INVAL_INPUT_DATA;
                if (propList->data->prop == NULL) return SML_ERR_XLT_INVAL_INPUT_DATA;
                /* -- Propname */
                if ((_err = devinfEncBlock(TN_DEVINF_PROPNAME, REQUIRED, propList->data->prop->name, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
                /* -- (ValEnum+ | (Datatype, Size?))? */
                //if (propList->data->prop->valenum == NULL && propList->data->prop->datatype == NULL) return SML_ERR_XLT_INVAL_INPUT_DATA;
                if (propList->data->prop->valenum != NULL && propList->data->prop->datatype != NULL) return SML_ERR_XLT_INVAL_INPUT_DATA;
                if (propList->data->prop->valenum != NULL) {
                    // ValEnum+
                    pList = propList->data->prop->valenum;
                    while (pList != NULL) {        
                        if ((_err = devinfEncBlock(TN_DEVINF_VALENUM, REQUIRED, pList->data, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
                        pList = pList->next;
                    };
                } else if (propList->data->prop->datatype != NULL) {
                    // Datatype, Size?
                    if ((_err = devinfEncBlock(TN_DEVINF_DATATYPE, REQUIRED, propList->data->prop->datatype, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
                    if ((_err = devinfEncBlock(TN_DEVINF_SIZE,     OPTIONAL, propList->data->prop->size,     enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
                }
                /* -- DisplayName ? */
                if ((_err = devinfEncBlock(TN_DEVINF_DISPLAYNAME, OPTIONAL, propList->data->prop->dname, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
                /* -- now the paramList */
                paramList = propList->data->param;
                while (paramList != NULL) {
                    if ((_err = devinfEncBlock(TN_DEVINF_PARAMNAME, REQUIRED, paramList->data->name, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
                    /* -- (ValEnum+ | (Datatype, Size?))? */
                    //if (paramList->data->valenum == NULL && paramList->data->datatype == NULL) return SML_ERR_XLT_INVAL_INPUT_DATA;
                    if (paramList->data->valenum != NULL && paramList->data->datatype != NULL) return SML_ERR_XLT_INVAL_INPUT_DATA;
                    if (paramList->data->valenum != NULL) {
                        // ValEnum+
                        p2List = paramList->data->valenum;
                        while (p2List != NULL) {        
                            if ((_err = devinfEncBlock(TN_DEVINF_VALENUM, REQUIRED, p2List->data, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
                            p2List = p2List->next;
                        };
                    } else if (paramList->data->datatype != NULL) {
                        // Datatype, Size?
                        if ((_err = devinfEncBlock(TN_DEVINF_DATATYPE, REQUIRED, paramList->data->datatype, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
                        if ((_err = devinfEncBlock(TN_DEVINF_SIZE,     OPTIONAL, paramList->data->size,     enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
                    }
                    /* -- DisplayName ? */
                    if ((_err = devinfEncBlock(TN_DEVINF_DISPLAYNAME, OPTIONAL, paramList->data->dname, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
                    paramList = paramList->next;
                }
                propList = propList->next;
            }
            /* eof propList */
            ctList = ctList->next;
        };        

        if ((_err = xltGenerateTag(TN_DEVINF_CTCAP, TT_END, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		break;
    case TN_DEVINF_DSMEM:
		if ((_err = xltGenerateTag(TN_DEVINF_DSMEM, TT_BEG, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_SHAREDMEM, OPTIONAL, &(((SmlDevInfDSMemPtr_t) pContent)->flags), enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_MAXMEM,    OPTIONAL, ((SmlDevInfDSMemPtr_t) pContent)->maxmem, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_MAXID,     OPTIONAL, ((SmlDevInfDSMemPtr_t) pContent)->maxid,  enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = xltGenerateTag(TN_DEVINF_DSMEM, TT_END, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		break;
        // special case, the following 4 have the same structure, only the tag name differs
    case TN_DEVINF_RX:
    case TN_DEVINF_TX:
    case TN_DEVINF_RXPREF:
    case TN_DEVINF_TXPREF:
		if ((_err = xltGenerateTag(tagId, TT_BEG, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_CTTYPE, REQUIRED, ((SmlDevInfXmitPtr_t) pContent)->cttype, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_VERCT,  REQUIRED, ((SmlDevInfXmitPtr_t) pContent)->verct,  enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = xltGenerateTag(tagId, TT_END, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		break;
    case TN_DEVINF_DATASTORE:
		if ((_err = xltGenerateTag(TN_DEVINF_DATASTORE, TT_BEG, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_SOURCEREF,    REQUIRED, ((SmlDevInfDatastorePtr_t) pContent)->sourceref,    enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_DISPLAYNAME,  OPTIONAL, ((SmlDevInfDatastorePtr_t) pContent)->displayname,  enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_MAXGUIDSIZE,  OPTIONAL, ((SmlDevInfDatastorePtr_t) pContent)->maxguidsize,  enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_RXPREF,       REQUIRED, ((SmlDevInfDatastorePtr_t) pContent)->rxpref,       enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		xmList = ((SmlDevInfDatastorePtr_t)pContent)->rx;
        while (xmList != NULL) {        
            if ((_err = devinfEncBlock(TN_DEVINF_RX, OPTIONAL, xmList->data, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
            xmList = xmList->next;
        };
		if ((_err = devinfEncBlock(TN_DEVINF_TXPREF,       REQUIRED, ((SmlDevInfDatastorePtr_t) pContent)->txpref,       enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		xmList = ((SmlDevInfDatastorePtr_t)pContent)->tx;
        while (xmList != NULL) {        
            if ((_err = devinfEncBlock(TN_DEVINF_TX, OPTIONAL, xmList->data, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
            xmList = xmList->next;
        };
		if ((_err = devinfEncBlock(TN_DEVINF_DSMEM,        OPTIONAL, ((SmlDevInfDatastorePtr_t) pContent)->dsmem,        enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_SYNCCAP,      REQUIRED, ((SmlDevInfDatastorePtr_t) pContent)->synccap,      enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = xltGenerateTag(TN_DEVINF_DATASTORE, TT_END, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		break;
    case TN_DEVINF_DEVINF:
		if ((_err = xltGenerateTag(TN_DEVINF_DEVINF, TT_BEG, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_VERDTD,  REQUIRED, ((SmlDevInfDevInfPtr_t) pContent)->verdtd,  enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_MAN,     OPTIONAL, ((SmlDevInfDevInfPtr_t) pContent)->man,     enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_MOD,     OPTIONAL, ((SmlDevInfDevInfPtr_t) pContent)->mod,     enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_OEM,     OPTIONAL, ((SmlDevInfDevInfPtr_t) pContent)->oem,     enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_FWV,     OPTIONAL, ((SmlDevInfDevInfPtr_t) pContent)->fwv,     enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_SWV,     OPTIONAL, ((SmlDevInfDevInfPtr_t) pContent)->swv,     enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_HWV,     OPTIONAL, ((SmlDevInfDevInfPtr_t) pContent)->hwv,     enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_DEVID,   REQUIRED, ((SmlDevInfDevInfPtr_t) pContent)->devid,   enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_DEVTYP,  REQUIRED, ((SmlDevInfDevInfPtr_t) pContent)->devtyp,  enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		// %%% luz:2003-04-28 added missing SyncML 1.1 devinf tags		
		if ((_err = devinfEncBlock(TN_DEVINF_UTC,     OPTIONAL, &(((SmlDevInfDevInfPtr_t) pContent)->flags),     enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_NOFM,    OPTIONAL, &(((SmlDevInfDevInfPtr_t) pContent)->flags),     enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		if ((_err = devinfEncBlock(TN_DEVINF_LARGEOBJECT, OPTIONAL, &(((SmlDevInfDevInfPtr_t) pContent)->flags), enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
    // %%% end luz		
		
		dsList = ((SmlDevInfDevInfPtr_t)pContent)->datastore;
        if (dsList == NULL) return SML_ERR_XLT_MISSING_CONT;
        if ((_err = devinfEncBlock(TN_DEVINF_DATASTORE, REQUIRED, dsList->data, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
        dsList = dsList->next;
        while (dsList != NULL) {        
            if ((_err = devinfEncBlock(TN_DEVINF_DATASTORE, OPTIONAL, dsList->data, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
            dsList = dsList->next;
        };  
        if ((_err = devinfEncBlock(TN_DEVINF_CTCAP, OPTIONAL, ((SmlDevInfDevInfPtr_t)pContent)->ctcap, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		exList = ((SmlDevInfDevInfPtr_t)pContent)->ext;
        while (exList != NULL) {        
            if ((_err = devinfEncBlock(TN_DEVINF_EXT, OPTIONAL, exList->data, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
            exList = exList->next;
        };


		if ((_err = xltGenerateTag(TN_DEVINF_DEVINF, TT_END, enc, pBufMgr, SML_EXT_DEVINF)) != SML_ERR_OK) return _err;
		break;

	default: { // all leaf nodes (PCDATA#)
        return xltEncPcdata(tagId, reqOptFlag, pContent, enc, pBufMgr, attFlag);
	 } //* eof default statement from switch tagid 
	} // eof switch tagid 
	return SML_ERR_OK;
}
#endif /* __USE_DEVINF__ */
