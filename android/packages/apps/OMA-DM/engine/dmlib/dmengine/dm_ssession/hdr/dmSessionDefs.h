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

#ifndef DM_UA_TYPEDEFS_H
#define DM_UA_TYPEDEFS_H

/*==================================================================================================

    Header Name: dm_ua_typedefs.h

    General Description: This contains DMUA data type definitions.

==================================================================================================*/

#include "syncml_dm_data_types.h"
#include "smldtd.h"
#include "dmbuffer.h"
#include "dm_tree_class.H"

/*==================================================================================================
                                 CONSTANTS
==================================================================================================*/

#define UINT16_TYPE_STR_SIZE_5    6       /* Array size for coverting a UINT16 number to a string */
#define UINT32_TYPE_STR_SIZE_10   11      /* Array size for coverting a UINT32 number to a string */

#define NUMBER_OF_WORKSPACE      2                          /* Number of workspace for a session */
#define WORKSPACE_NAME_LEN        30      /* String length of workspace name */
#define MAX_BIN_NONCE_LEN         50      /* Maximum length of a binary decoded Nonce */
#define MAX_BIN_VAL_LEN              50     /* Max length of a b64 decode bin array */

#define DEFAULT_MESSAGE_ID   "1"
#define DEFAULT_HDR_CMD_ID   "0"

#define SERVER_RESYNC_NONCE  "AAAAAA=="  // Motorola, <e50324>, <12/08/09>, <ikmap-2156> / Nonce Resynchronization
//#define SERVER_RESYNC_NONCE  "MTIzNDU="  // for Sprint HFA OMA DM

/*==================================================================================================
                                 ENUMS
==================================================================================================*/
/* These enums are used when handling the AppNotify messages.*/
enum
{
    SYNCML_DM_NODE_ADD,
    SYNCML_DM_NODE_DELETE,
    SYNCML_DM_NODE_RENAME,
    SYNCML_DM_NODE_INVALID
};
typedef UINT8 SYNCML_DM_NODE_CHANGE_T;

/* These enums are used for the various Challenge types.*/
enum
{
    SYNCML_DM_CHAL_UNDEFINED = -1,
    SYNCML_DM_CHAL_NONE = 0,
    SYNCML_DM_CHAL_BASIC = 1,
    SYNCML_DM_CHAL_MD5 = 2,
    SYNCML_DM_CHAL_HMAC = 3
};
typedef INT8 SYNCML_DM_CHAL_TYPE_T;

/* Type Defines */
enum {
    DM_COMMAND_INVALID = 0,
    DM_COMMAND_ADD,
    DM_COMMAND_ALERT,
    DM_COMMAND_ATOMIC_START,
    DM_COMMAND_ATOMIC_END,
    DM_COMMAND_COPY,
    DM_COMMAND_DELETE,
    DM_COMMAND_EXEC,
    DM_COMMAND_GET,
    DM_COMMAND_MAP,
    DM_COMMAND_PUT,
    DM_COMMAND_RESULTS,
    DM_COMMAND_SEARCH,
    DM_COMMAND_SEQUENCE_START,
    DM_COMMAND_SEQUENCE_END,
    DM_COMMAND_STATUS,
    DM_COMMAND_SYNC_START,
    DM_COMMAND_SYNC_END,
    DM_COMMAND_REPLACE,
    DM_COMMAND_HEADER,
    DM_COMMAND_BODY_END,
    DM_COMMAND_COUNT
};
typedef UINT8 DMCommandType;

/*==================================================================================================
                                 TYPEDEFS
==================================================================================================*/

/* This structure is used to hold credential information. */
class DMCredHeaders
{
public:
    inline void clear()
    {
      m_oAlgorithm.clear();
      m_oUserName.clear();
      m_oMac.clear();
    }

    inline BOOLEAN empty() const
    {
       return ( (m_oUserName.getSize() == 0 || m_oMac.getSize() == 0) ? TRUE : FALSE );
    }

    inline BOOLEAN isCorrect() const
    {
        if ( m_oUserName.getSize() != 0 )
        {
            if ( m_oMac.getSize() == 0 )
                return FALSE;
        }
        else
        {
            if ( m_oMac.getSize() != 0 )
                return FALSE;
        }
        return TRUE;
    }    
  
    DMBuffer m_oAlgorithm;
    DMBuffer m_oUserName;
    DMBuffer m_oMac;
};



typedef struct
{
   UINT8   *pChalType;
   UINT8   *pChalFormat;
   UINT8   *pChalNonce;
} DM_CHALLENGE_T;


/* Added for Multiple Messages Support */
struct SYNCML_DM_STATUS_DATA_T
{
    DMString      pCmdId;
    DMString      pCmdName;
    DMString      pSource;
    DMString      pTarget;
    SYNCML_DM_RET_STATUS_T status;
    DMStringVector    responses;  // alert requires additional parameters
    BOOLEAN       bValueSet;
    BOOLEAN       bInSequence;

  SYNCML_DM_STATUS_DATA_T() { status = 0; bValueSet = FALSE; bInSequence = FALSE;}
  SYNCML_DM_STATUS_DATA_T( const char* szCmd, const char* szCmdName, const char* szSource,
    const char* szTarget, SYNCML_DM_RET_STATUS_T st, DMStringVector* pAdditionalData = NULL ){
      pCmdId = szCmd;
      pCmdName = szCmdName;
      pSource = szSource;
      pTarget = szTarget;
      status = st;
      bValueSet = TRUE;
      bInSequence = FALSE;
      
      if ( pAdditionalData )
        responses = *pAdditionalData;
    }
};

struct SYNCML_DM_RESULT_VALUE {
  enum {Enum_Result_Get, Enum_Result_GetStruct, Enum_Result_GetStructData, Enum_Result_GetTnds, Enum_Result_Exec};

  SmlResultsPtr_t _pGetExecResult;
  SYNCML_DM_GET_ON_LIST_RET_DATA_T  _oGetStructPos;  // cached position for GetStruct
  UINT8  _type; // see enum above
  DMString _cmdRef;
  DMString _msgID;

  // methods
  inline SYNCML_DM_RESULT_VALUE()
    {
    _type = 0; _pGetExecResult = NULL;
    }
  inline SYNCML_DM_RESULT_VALUE( UINT8 type, SmlResultsPtr_t pRes, const SYNCML_DM_GET_ON_LIST_RET_DATA_T& oData, const char* strCmdRef, const char* strMsgID ){
    _type = type;
    _oGetStructPos = oData;
    _pGetExecResult = pRes;
    _cmdRef = strCmdRef;
    _msgID = strMsgID;
    }
};


class DMProcessScriptSession;
class SYNCML_DM_BuildPackage;

struct SYNCML_DM_USER_DATA_T
{
  enum {Enum_Alert_None, Enum_Alert_Cancel, Enum_Alert_No};
    
  // saved statuses
  DMVector<UINT32>  aStatuses;
  DMVector<SYNCML_DM_RESULT_VALUE> aResults;

  DMString              pMsgRef;
  SYNCML_DM_STATUS_DATA_T pAtomicStatus;  // atomic command state
  DMVector<SYNCML_DM_STATUS_DATA_T> oStatus;  // atomic statuses (not built)
  BOOLEAN               rollback;   // inside atomic and already rolled back
  BOOLEAN               sequenceFailed; 
  SYNCML_DM_ALERT_RES_T  alertState; // alert state in atomic/sequence/document - 
  BOOLEAN               bNonceGenerated;
  DMProcessScriptSession *pSessionMng;            
  SYNCML_DM_BuildPackage * pPkgBuilder;
  

  inline SYNCML_DM_USER_DATA_T() {
      rollback        = FALSE;
      sequenceFailed  = FALSE;            
      bNonceGenerated = FALSE;
      pMsgRef = NULL;
      alertState = SYNCML_DM_ALERT_NONE;
      pSessionMng = NULL;
      pPkgBuilder = NULL;
    }

  inline bool IsCommandSkipped() const 
  {
    return ( alertState == SYNCML_DM_ALERT_CANCEL ||
             ( ((pAtomicStatus.bValueSet || pAtomicStatus.bInSequence) &&
                (rollback || alertState == SYNCML_DM_ALERT_NO)) ||
                (pAtomicStatus.bInSequence && sequenceFailed)));
  }
  inline void EndAtomic() {
    pAtomicStatus.bValueSet = FALSE;
    rollback = FALSE;
    oStatus.clear();
    
    if ( alertState != SYNCML_DM_ALERT_CANCEL )
      alertState = SYNCML_DM_ALERT_NONE;
  }

  inline BOOLEAN IsSequence()
    {
      return pAtomicStatus.bInSequence;
    }
  
  inline void StartSequence()
    {
      pAtomicStatus.bInSequence = TRUE;
    }
  inline void EndSequence()
    {
      pAtomicStatus.bInSequence = FALSE;
      sequenceFailed = FALSE;  
      
      if ( alertState != Enum_Alert_Cancel )
        alertState = Enum_Alert_None;
    }
};

#endif /* DM_UA_TYPEDEFS_H */
