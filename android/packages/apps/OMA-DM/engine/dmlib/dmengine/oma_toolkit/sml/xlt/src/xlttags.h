/*************************************************************************/
/* module:          Definition of WBXML/XML tags for the en-/decoder     */
/* file:            XLTTags.h                                            */
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

/*************************************************************************/
/*  Definitions                                                          */
/*************************************************************************/

#ifndef _XLT_TAGS_H
#define _XLT_TAGS_H

#include <smldtd.h>
#include <smldef.h>

#define XML_MAX_TAGLEN 35

#ifdef _cplusplus
extern "C" {
#endif

// Tag IDs
typedef enum {
    TN_UNDEF = 0,

    TN_ADD,
    TN_ALERT,
    TN_ARCHIVE,
    TN_ATOMIC,
    TN_ATOMIC_END, /* 5 */
    TN_CHAL,
    TN_CMD,
    TN_CMDID,
    TN_CMDREF,
    TN_COPY,     /* 10 */
    TN_CRED,
    TN_DATA,
    TN_DELETE,
    TN_EXEC,
    TN_FINAL,    /* 15 */
    TN_GET,
    TN_ITEM,
    TN_LANG,
    TN_LOCNAME,
    TN_LOCURI,  /* 20 */
    TN_MAP,
    TN_MAPITEM,
    TN_META,
    TN_MSGID,
    TN_MSGREF,  /* 25 */
    TN_NORESP,
    TN_NORESULTS,
    TN_PUT,
    TN_REPLACE,
    TN_RESPURI, /* 30 */
    TN_RESULTS,
    TN_SEARCH,
    TN_SEQUENCE,
    TN_SEQUENCE_END,
    TN_SESSIONID, /* 35 */
    TN_SFTDEL,
    TN_SOURCE,
    TN_SOURCEREF,
    TN_STATUS,
    TN_SYNC, /* 40 */
    TN_SYNCBODY,
    TN_SYNCHDR,
    TN_SYNCML,
    TN_SYNC_END,
    TN_TARGET, /* 45 */
    TN_TARGETREF,
    TN_VERSION,
    TN_PROTO,
    TN_METINF_ANCHOR,
	TN_METINF_EMI, /* 50 */
	TN_METINF_FORMAT, 
	TN_METINF_FREEID,
	TN_METINF_FREEMEM,		
	TN_METINF_LAST,
	TN_METINF_MARK, /* 55 */
	TN_METINF_MAXMSGSIZE,
	TN_METINF_MEM,
	TN_METINF_METINF,
	TN_METINF_NEXT,
	TN_METINF_NEXTNONCE, /* 60 */
	TN_METINF_SHAREDMEM,
	TN_METINF_SIZE,
	TN_METINF_TYPE,
	TN_METINF_VERSION,
  TN_DEVINF_CTCAP,    /* 65 */
  TN_DEVINF_CTTYPE,
  TN_DEVINF_DATASTORE,
  TN_DEVINF_DATATYPE,
  TN_DEVINF_DEVID,
  TN_DEVINF_DEVINF, /* 70 */
  TN_DEVINF_DEVTYP,
  TN_DEVINF_DISPLAYNAME,
  TN_DEVINF_DSMEM,
  TN_DEVINF_EXT,
  TN_DEVINF_FWV, /* 75 */
  TN_DEVINF_HWV,
  TN_DEVINF_MAN,
  TN_DEVINF_MAXGUIDSIZE,
  TN_DEVINF_MAXID,
  TN_DEVINF_MAXMEM, /* 80 */
  TN_DEVINF_MOD,
  TN_DEVINF_OEM,
  TN_DEVINF_PARAMNAME,
  TN_DEVINF_PROPNAME,
  TN_DEVINF_RX, /* 85 */
  TN_DEVINF_RXPREF,
  TN_DEVINF_SHAREDMEM,
  TN_DEVINF_SIZE,
  TN_DEVINF_SOURCEREF,
  TN_DEVINF_SWV, /* 90 */
  TN_DEVINF_SYNCCAP,
  TN_DEVINF_SYNCTYPE,
  TN_DEVINF_TX,
  TN_DEVINF_TXPREF,
  TN_DEVINF_VALENUM, /* 95 */
  TN_DEVINF_VERCT,
  TN_DEVINF_VERDTD,
  TN_DEVINF_XNAM,
  TN_DEVINF_XVAL,
	/* SCTSTK - 18/03/2002, S.H. 2002-04-05 : SyncML 1.1 */
	TN_NUMBEROFCHANGES, /* 100 */
	TN_MOREDATA,
	TN_METINF_MAXOBJSIZE,
	TN_DEVINF_UTC,
	TN_DEVINF_NOFM,
	TN_DEVINF_LARGEOBJECT,
	TN_CORRELATOR,
   /* TNDS - 08/05/2008, Ken Chen(e50324) Motorola Inc., OMA-TS-DM_TND-V1_2_1-20080617-A */
   TN_DMTND_AccessType, /* 107 */
   TN_DMTND_ACL, 
   TN_DMTND_Add, 
   TN_DMTND_b64, /* 110 */
   TN_DMTND_bin, 
   TN_DMTND_bool, 
   TN_DMTND_chr, 
   TN_DMTND_CaseSense, 
   TN_DMTND_CIS, 
   TN_DMTND_Copy, 
   TN_DMTND_CS, 
   TN_DMTND_date, 
   TN_DMTND_DDFName, 
   TN_DMTND_DefaultValue, /* 120 */
   TN_DMTND_Delete, 
   TN_DMTND_Description, 
   TN_DMTND_DFFormat, 
   TN_DMTND_DFProperties,
   TN_DMTND_DFTitle, 
   TN_DMTND_DFType, 
   TN_DMTND_Dynamic, 
   TN_DMTND_Exec, 
   TN_DMTND_float, 
   TN_DMTND_Format, /* 130 */
   TN_DMTND_Get, 
   TN_DMTND_int, 
   TN_DMTND_Man, 
   TN_DMTND_MgmtTree, 
   TN_DMTND_MIME, 
   TN_DMTND_Mod, 
   TN_DMTND_Name, 
   TN_DMTND_Node, 
   TN_DMTND_node, 
   TN_DMTND_NodeName, /* 140 */ 
   TN_DMTND_null, 
   TN_DMTND_Occurrence, 
   TN_DMTND_One, 
   TN_DMTND_OneOrMore, 
   TN_DMTND_OneOrN, 
   TN_DMTND_Path, 
   TN_DMTND_Permanent, 
   TN_DMTND_Replace, 
   TN_DMTND_RTProperties, 
   TN_DMTND_Scope, /* 150 */
   TN_DMTND_Size,  
   TN_DMTND_time, 
   TN_DMTND_Title, 
   TN_DMTND_TStamp, 
   TN_DMTND_Type, 
   TN_DMTND_Value, 
   TN_DMTND_VerDTD, 
   TN_DMTND_VerNo, 
   TN_DMTND_xml, 
   TN_DMTND_ZeroOrMore, /* 160 */
   TN_DMTND_ZeroOrN,  
   TN_DMTND_ZeroOrOne
} XltTagID_t;


extern const char * const SyncMLNamespaces[SML_NUM_VERS];

typedef struct Dtd_s
{
	String_t	         name;
	SmlPcdataExtension_t ext;
} Dtd_t, *DtdPtr_t;


/**
 * FUNCTION: getTagString
 *
 * Returns a tag string which belongs to a tag ID. 
 * This function is needed for the XML encoding
 *
 * PRE-Condition:   valid tag ID, the tagSring has to be allocated 
 *
 * POST-Condition:  tag string is returned
 *
 * IN:              tagId, the ID for the tag 
 *
 * IN/OUT:          tagString, allocated string into which the XML 
 *                             tag string will be written
 * 
 * RETURN:          0, if OK
 */
Ret_t getTagString(XltTagID_t tagID, String_t tagString, SmlPcdataExtension_t ext);

#ifdef __SML_WBXML__
/**
 * FUNCTION: getTagByte
 *
 * Returns a WBXML byte which belongs to a tag ID in a defined codepage. 
 * This function is needed for the WBXML encoding
 *
 * PRE-Condition:   valid tag ID, valid code page
 *
 * POST-Condition:  tag byte is returned
 *
 * IN:              tagId, the ID for the tag 
 *                  cp, code page group for the tag 
 *                  tagByte, the byte representation of the tag
 * 
 * RETURN:          0, if OK
 */
Ret_t getTagByte(XltTagID_t tagID, SmlPcdataExtension_t ext, Byte_t *tagByte);
#endif

/**
 * FUNCTION: getCodePage
 *
 * Returns the code page which belongs to a certain PCDATA extension type. 
 *
 * PRE-Condition:   valid PCDATA extension type
 *
 * POST-Condition:  the code page is returned
 *
 * IN:              ext, the PCDATA extension type
 * 
 * RETURN:          the code page
 */
Byte_t     getCodePage(SmlPcdataExtension_t ext);

/**
 * FUNCTION: getTagIDByStringAndCodepage
 *
 * Returns the tag ID which belongs to a tag string in a certain codepage
 *
 * PRE-Condition:   valid tag string, valid code page
 *
 * POST-Condition:  tag id is returned
 *
 * IN:              tag, the string representation of the tag 
 *                  cp, code page group for the tag 
 *                  pTagID, the tag id of the tag
 * 
 * RETURN:          0, if OK
 */
Ret_t getTagIDByStringAndExt(String_t tag, SmlPcdataExtension_t ext, XltTagID_t *pTagID);

#ifdef __SML_WBXML__
/**
 * FUNCTION: getTagIDByByteAndCodepage
 *
 * Returns the tag ID which belongs to a tag byte in a certain codepage
 *
 * PRE-Condition:   valid tag byte, valid code page
 *
 * POST-Condition:  tag id is returned
 *
 * IN:              tag, the byte representation of the tag 
 *                  cp, code page group for the tag  
 *                  pTagID, the tag id of the tag
 * 
 * RETURN:          0, if OK
 */
Ret_t getTagIDByByteAndExt(Byte_t tag, SmlPcdataExtension_t ext, XltTagID_t *pTagID);
#endif


#ifdef __SML_XML__
/**
 * FUNCTION: getTagIDByStringAndNamespace
 *
 * Returns the tag ID which belongs to a tag string in a certain namespace
 *
 * PRE-Condition:   valid tag string, valid namespace
 *
 * POST-Condition:  tag id is returned
 *
 * IN:              tag, the string representation of the tag 
 *                  ns, namespace group for the tag  
 *                  pTagID, the tag id of the tag
 * 
 * RETURN:          0, if OK
 */
Ret_t getTagIDByStringAndNamespace(String_t tag, String_t ns, XltTagID_t *pTagID);
#endif

/**
 * FUNCTION: getCodePageById
 *
 * Returns the codepage which belongs to a certain tag ID
 *
 * PRE-Condition:   valid tag ID
 *
 * POST-Condition:  the code page is returned
 *
 * IN:              tagID, the ID of the tag 
 *                  pCp, the codepage of the tag
 *
 * RETURN:          0, if OK
 */
Ret_t getExtById(XltTagID_t tagID, SmlPcdataExtension_t *pExt);

// %%% luz:2003-07-31: added vers parameter
Ret_t getExtName(SmlPcdataExtension_t ext, String_t *name, SmlVersion_t vers);
Ret_t getTagIDByStringAndExt(String_t tag, SmlPcdataExtension_t ext, XltTagID_t *pTagID);
SmlPcdataExtension_t getExtByName(String_t ns);


#ifdef _cplusplus
}
#endif

#endif
