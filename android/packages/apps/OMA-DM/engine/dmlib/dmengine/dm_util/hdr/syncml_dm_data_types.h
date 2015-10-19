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

#ifndef SYNCML_DM_DATA_TYPES_H
#define SYNCML_DM_DATA_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================

    Header Name: syncml_dm_data_types.h

    General Description: This contains declaration of DM engine general enums and data structures.

==================================================================================================*/

#include "dmtError.h"
#include "dmMemory.h"
#include "dmtDefs.h"

/*==================================================================================================
                                            CONSTANTS
==================================================================================================*/

#define DM_MD5_DIGEST_LENGTH 16      /* For package 0 data */

#define SYNCML_DM_MAX_SERVER_ALLOWED    3  /* for Current implementation */

#define SYNCML_DM_URI_MAX_DEPTH         20   //Max allowed levels

/* The DDF file for Motorola SyncML DM devices defines MaxTotLen = 127, and MaxSegLen = 32.
   MaxTotLen is limited to 127 due to persistent storage implementation. It serializes the tree
   structure into a flat file using WBXML format; opaque data values are stored with a multi-byte
   integer encoded length. The implementation of the mb_uint_32 format is limited to encode/decode
   up to the value 127. Anywhere memory is allocated for the Max URI length should use
   SYNCML_DM_URI_MAX_TOTAL_LENGTH+1 to account for a null string terminator, since the code works
   with the URI as a string. In the same manner, SYNCML_DM_URI_MAX_SEGMENT_LENGTH+1 should be used
   to account for its string terminator. Note! The SyncML specs state that by default, characters
   are encoded in UTF-8. The MaxTotLen and MaxSegLen values indicate the number of characters the
   device allows, which may be LESS than the number of bytes it actually takes to UTF-8 encode the
   string. In theory, the size in bytes of the memory to hold these strings should be much larger,
   to allow the UTF-8 encoded data to fit (as much as 2 or 3 times the size, worst case). At the
   moment, the byte-size and character-length for these limits are coded to be  the same, which
   always works if the characters are ASCII.  */

#define SYNCML_DM_URI_MAX_TOTAL_LENGTH    255 // will use too many stack spaces
#define SYNCML_DM_URI_MAX_SEGMENT_LENGTH   120

#define SYNCML_DM_MAX_TITLE_LENGTH 255
#define SYNCML_DM_MAX_TSTAMP_LENGTH 17
#define SYNCML_DM_STR_MAX_LEN       120   /* Max length of a string leaf node data value. */
#define SYNCML_DM_MAX_OBJ_SIZE 262080  // This is the maximum FOTA obj size required by TRS.

#define DEFAULT_DM_SMLTK_WORKSPACE_SIZE (120*1024)

#define DM_MSG_OVERHEAD         600   /* SyncML DM document overhead (none data part). */
extern INT32 g_iDMWorkspaceSize;

#define DM_FSTAB_FILENAME             "fstab"
#define DM_ROOT_MDF_FILENAME          "root.bmdf"
#define DM_PLUGINS_INI_FILENAME       "sysplugins.ini"
#define DM_PLIGINS_INI_FILEEXTENSION  "ini"
#define DM_MDF_FILEEXTENSION          "bmdf"
#define DM_DEFAULT_ACL_FILENAME       "acl.dat"
#define DM_DEFAULT_EVENT_FILENAME     "event.dat"
#define DM_DATASUBFOLDER              ""

#define DM_MAX_CONFIG_LINE 400


/*==================================================================================================
                                GLOBAL VARIABLES DECLARATION
==================================================================================================*/
/* These strings are used throughout the DM engine code. They are externed here to keep them
 * in one ROM location.  Please see the actual value definitions in "syncml_dm_main.cc".
 */

#define SYNCML_CONTENT_TYPE_DM_WBXML  "application/vnd.syncml.dm+wbxml"
#define SYNCML_CONTENT_TYPE_DM_XML    "application/vnd.syncml.dm+xml"
#define SYNCML_CONTENT_TYPE_DM_TNDS_WBXML  "application/vnd.syncml.dmtnds+wbxml"
#define SYNCML_CONTENT_TYPE_DM_TNDS_XML    "application/vnd.syncml.dmtnds+xml"
#define SYNCML_DM_PROTOCOL_VERSION_1_2    "DM/1.2"
#define SYNCML_DM_PROTOCOL_VERSION_1_1    "DM/1.1"


#define RECV_WORKSPACE_NAME   "Received SyncML Document"
#define SEND_WORKSPACE_NAME   "Send SyncML Document"
#define CACHE_WORKSPACE_NAME  "SyncML Document Cache"

#define DM_DEV_INFO_URI_PAK1  "./DevInfo?list=Struct"

#define RECV_WORKSPACE_NAME   "Received SyncML Document"
#define SEND_WORKSPACE_NAME   "Send SyncML Document"
#define CACHE_WORKSPACE_NAME  "SyncML Document Cache"

#define DM_ROOT_URI   "."
#define DM_STR_SLASH  "/"
#define DM_INBOX      "./Inbox"
#define DM_DMACC_1_1_URI  "./SyncML/DMAcc"
#define DM_DMACC_1_2_URI  "./DMAcc"
#define DM_DMCON_URI      "./SyncML/Con"

#define DM_APPID      "AppID"
#define DM_SERVERID_1_2 "ServerID"
#define DM_SERVERID_1_1 "ServerId"

#define MNG_OBJID_DMACC1 "urn:oma:mo:oma-dm-dmacc:1.0"
#define MNG_OBJID_DMACC2 "org.openmobilealliance/1.0/w7"

#define DM_NAME       "Name"
#define DM_PREFCONREF "PrefConRef"
#define DM_CONREF     "ConRef"
#define DM_TOCONREF   "ToConRef"
#define DM_APPADDR    "AppAddr"
#define DM_ADDR       "Addr"
#define DM_ADDRTYPE   "AddrType"
#define DM_PORT       "Port"
#define DM_PORTNBR    "PortNbr"
#define DM_USERNAME "UserName"
#define DM_AUTHPREF "AuthPref"
#define DM_CLIENTNONCE  "ClientNonce"
#define DM_SERVERNONCE  "ServerNonce"
#define DM_CLIENTPW "ClientPW"
#define DM_SERVERPW "ServerPW"

// OMA DM 1.2 changes
#define DM_AAUTHPREF   "AAuthPref"
#define DM_APPAUTH     "AppAuth"
#define DM_AAUTHLEVEL  "AAuthLevel"
#define DM_AAUTHTYPE   "AAuthType"
#define DM_AAUTHNAME   "AAuthName"
#define DM_AAUTHSECRET "AAuthSecret"
#define DM_AAUTHDATA   "AAuthData"
#define DM_EXT                  "Ext"
#define DM_LASTCLIENTAUTHTYPE "LastClientAuthType"

// Application service
#define DM_APPID_VALUE        "w7"

#define DM_AUTHLEVEL_CLCRED  "CLCRED"
#define DM_AUTHLEVEL_SRVCRED "SRVCRED"
#define DM_AUTHLEVEL_OBEX    "OBEX"
#define DM_AUTHLEVEL_HTTP    "HTTP"

#define DM_AUTHTYPE_HTTPBASIC     "HTTP-BASIC"
#define DM_AUTHTYPE_HTTPDIGEST    "HTTP-DIGEST"
#define DM_AUTHTYPE_BASIC         "BASIC"
#define DM_AUTHTYPE_DIGEST        "DIGEST"
#define DM_AUTHTYPE_HMAC          "HMAC"
#define DM_AUTHTYPE_TRANPORT      "TRANSPORT"
// end of OMA DM 1.2 changes

#define DM_DEV_INFO_URI_PAK1      "./DevInfo?list=Struct"
#define DM_DEV_INFO_DEVID_URI     "./DevInfo/DevId"
#define DM_DEV_INFO_MOD_URI       "./DevInfo/Mod"
#define DM_DEV_INFO_MAN_URI       "./DevInfo/Man"

#define SYNCML_AUTH_MAC     "syncml:auth-MAC"
#define SYNCML_AUTH_MD5     "syncml:auth-md5"
#define SYNCML_AUTH_BASIC   "syncml:auth-basic"
#define SYNCML_B64          "b64"
#define SYNCML_MAC_ALG      "MD5"

#define SYNCML_SYNCHDR  "SyncHdr"
#define SYNCML_REPLACE  "Replace"
#define SYNCML_ALERT    "Alert"

/*==================================================================================================
                                            ENUMS
==================================================================================================*/
/* Need to find the right global .h file for these defines */

typedef UINT32 MAX_MSG_SIZE_T;

/* These enum values are used to indicate the node scope for DMTNM_NODE_SCOPE */
enum
{
    DMTNM_NODE_PERMANENT = 1,
    DMTNM_NODE_DYNAMIC   = 2
};
typedef UINT8 DMTNM_NODE_SCOPE;

enum
{
    DMTNM_NODE_INTERIOR = 1,
    DMTNM_NODE_LEAF     = 2
};
typedef UINT8 DMTNM_NODE_TYPE;

/* These enum values are used to indicate command type for SYNCML_DM_COMMAND_T.
 * DON'T CHANGE THE ORDER OF THIS ENUM, TEHY ARE USED BY FEATURE DATABASES. */
enum
{
    SYNCML_DM_NO_COMMAND,
    SYNCML_DM_ADD,
    SYNCML_DM_DELETE,
    SYNCML_DM_REPLACE,
    SYNCML_DM_GET,
    SYNCML_DM_RENAME,
    SYNCML_DM_EXEC,
    SYNCML_DM_COPY,
    SYNCML_DM_ALERT,
    SYNCML_DM_HEADER,
    SYNCML_DM_STATUS,
    SYNCML_DM_ATOMIC,
    SYNCML_DM_SEQUENCE,
    SYNCML_DM_MAX_CMD,
    SYNCML_DM_ADD_CHILD,
    SYNCML_DM_RELEASE,
    SYNCML_DM_ROLLBACK,
    SYNCML_DM_COMMIT
};
typedef UINT8 SYNCML_DM_COMMAND_T;

/* These enum values are used to indicate the format of the data for SYNCML_DM_FORMAT_T */
enum
{
    SYNCML_DM_FORMAT_BIN     = SYNCML_DM_DATAFORMAT_BIN,
    SYNCML_DM_FORMAT_BOOL    = SYNCML_DM_DATAFORMAT_BOOL,
    SYNCML_DM_FORMAT_B64     = SYNCML_DM_DATAFORMAT_B64,
    SYNCML_DM_FORMAT_CHR     = SYNCML_DM_DATAFORMAT_STRING,
    SYNCML_DM_FORMAT_INT     = SYNCML_DM_DATAFORMAT_INT,
    SYNCML_DM_FORMAT_NODE    = SYNCML_DM_DATAFORMAT_NODE,
    SYNCML_DM_FORMAT_NULL    = SYNCML_DM_DATAFORMAT_NULL,
    SYNCML_DM_FORMAT_XML     = SYNCML_DM_DATAFORMAT_XML,
    SYNCML_DM_FORMAT_FLOAT   = SYNCML_DM_DATAFORMAT_FLOAT,
    SYNCML_DM_FORMAT_DATE    = SYNCML_DM_DATAFORMAT_DATE,
    SYNCML_DM_FORMAT_TIME    = SYNCML_DM_DATAFORMAT_TIME,
    SYNCML_DM_FORMAT_TEST    = 9,
    SYNCML_DM_FORMAT_INVALID = 21,
    SYNCML_DM_FORMAT_NODE_PDATA = 22 /* special type for serialization only */
};
typedef UINT8 SYNCML_DM_FORMAT_T;


/* These values came from the DM spec, are used for SYNCML_DM_ALERT_CODE. */
enum
{
  DM_ALERT_DISPLAY               = 1100,
  DM_ALERT_CONTINUE_OR_ABORT     = 1101,
  DM_ALERT_TEXT_INPUT            = 1102,
  DM_ALERT_SINGLE_CHOICE         = 1103,
  DM_ALERT_MULTIPLE_CHOICE       = 1104,
  DM_ALERT_SERVER_INITIATED_MGMT = 1200,
  DM_ALERT_CLIENT_INITIATED_MGMT = 1201,
  DM_ALERT_NEXT_MESSAGE          = 1222,
  DM_ALERT_SESSION_ABORT         = 1223,
  DM_ALERT_END_OF_DATA_NOT_RECEIVED = 1225,
  DM_ALERT_FIRMWARE_UPDATE       = 1226
};
typedef UINT16 SYNCML_DM_ALERT_CODE;


/* These enum values are used to indicate the state of Security for the DM User Agent
 * SYNCML_DM_SEC_STATE_FLAG_T  */
/* NOTE: DO NOT change numeric values, since bitwise operations used */
enum {
   DM_CLIENT_NO_SERVER_NO_AUTH = 0, /* Neither client nor server are authenticated.*/
   DM_CLIENT_NO_SERVER_Y_AUTH  = 1, /* Server Authenticated, Client not authenticated.*/
   DM_CLIENT_Y_SERVER_NO_AUTH  = 2, /* Client Authenticated, Server not authenticated.*/
   DM_BOTH_CLIENT_SERVER_AUTH = 3   /* Both Client and Server Authenticated.*/
};
typedef UINT8 SYNCML_DM_SEC_STATE_FLAG_T;


/* These enum values are used to indicate the URI checking result for SYNCML_DM_URI_RESULT_T */
enum
{
    SYNCML_DM_COMMAND_ON_NODE,             /* means URI doesnt have any special case */
    SYNCML_DM_COMMAND_ON_ACL_PROPERTY,    /* ?prop=ACL or ?prop=Name , Format etc */
    SYNCML_DM_COMMAND_ON_FORMAT_PROPERTY,
    SYNCML_DM_COMMAND_ON_NAME_PROPERTY,
    SYNCML_DM_COMMAND_ON_SIZE_PROPERTY,
    SYNCML_DM_COMMAND_ON_ESN_PROPERTY,
    SYNCML_DM_COMMAND_ON_TYPE_PROPERTY,
    SYNCML_DM_COMMAND_ON_TITLE_PROPERTY,
    SYNCML_DM_COMMAND_ON_TSTAMP_PROPERTY,
    SYNCML_DM_COMMAND_ON_VERNO_PROPERTY,
    SYNCML_DM_COMMAND_ON_UNKNOWN_PROPERTY,
    SYNCML_DM_COMMAND_LIST_STRUCT,    /* ?list=Struct found in URI */
    SYNCML_DM_COMMAND_LIST_STRUCTDATA,  /* ?list=StructData found in URI */
    SYNCML_DM_COMMAND_LIST_TNDS,  /* ?list=TNDS found in URI */
    SYNCML_DM_COMMAND_URI_TOO_LONG,     /* URI is too long */
    SYNCML_DM_COMMAND_INVALID_URI       /* URI was invalid */
};
typedef UINT8 SYNCML_DM_URI_RESULT_T;


enum
{
   SYNCML_DM_ALERT_NONE,
   SYNCML_DM_ALERT_CANCEL,
   SYNCML_DM_ALERT_NO
};
typedef UINT8 SYNCML_DM_ALERT_RES_T;


/*==================================================================================================
           Structures
==================================================================================================*/

/* This buffer is used to hold syncml document data */
typedef struct
{
    UINT32  dataSize;
    UINT8  *pData;
} SYNCML_DM_INDIRECT_BUFFER_T;


typedef struct
{
    BOOLEAN _AuthFlag;
    CPCHAR  _pServerId;
    UINT8   *_md5Digest;
    UINT8   *_pTrigger;
    UINT8   _triggerLen;
} SYNCML_DM_AuthContext_T;

/*================================================================================================*/
#ifdef __cplusplus
}
#endif

#endif /* SYNCML_DM_DATA_TYPES_H */
