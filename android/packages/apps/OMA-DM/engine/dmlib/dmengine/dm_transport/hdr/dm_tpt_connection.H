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

#ifndef _DM_TPT_CONNECTION_H
#define _DM_TPT_CONNECTION_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

//-----------------------------------------------------------------------
//
//   Header Name: dm_tpt_connection.H
//
//   General Description: This file contains the class declaration for
//                SYNCML_DM_Connection and SYNCML_DM_OTAConnection.
//
//-----------------------------------------------------------------------

#include "syncml_dm_data_types.h"
#include "dm_tpt_utils.h"
#include "dmSessionDefs.h"
#include "dmMemory.h"
#include "xpl_HTTP.h"


//-----------------------------------------------------------------------
//                           CONSTANTS
//-----------------------------------------------------------------------

// define for initial chunk size for HTTP transport
#define DMTPT_HTTP_CHUNK_SIZE 10240

// define for initial chunk size for WSP transport
#define DMTPT_WSP_CHUNK_SIZE  8192

// define for maximum number of retries
#define DMTPT_MAX_RETRIES 5

//-----------------------------------------------------------------------
//                            MACROS
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
//                            ENUMS
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//                  STRUCTURES AND OTHER TYPEDEFS
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
//                   GLOBAL VARIABLE DECLARATIONS
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//                        FUNCTION PROTOTYPES
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
//                            CLASS
//-----------------------------------------------------------------------


#if (!defined (PLATFORM_ANDROID) || defined(DM_NATIVE_HTTP))
// Abstract class that provides the interfaces to the DMUA for Sending/
// Receiving documents

class SYNCML_DM_Connection
{
protected:

    // AddrType stores stores the format and interpretation of the
    // DMAcc/x/AddrType node value
    XPL_ADDR_TYPE_T AddrType;

    // URL of the SyncML DM Server
    DMString m_szURL;

public:

    virtual SYNCML_DM_RET_STATUS_T Init(UINT32 dwMaxAcptSize,
                                        XPL_ADDR_TYPE_T AddressType,
                                        CPCHAR ConRef) = 0;

    // Interface for Send method
    virtual SYNCML_DM_RET_STATUS_T Send(
            const SYNCML_DM_INDIRECT_BUFFER_T *psSendDoc,
            SYNCML_DM_INDIRECT_BUFFER_T *psRecvDoc,
            const UINT8 *pbContentType,
            const DMCredHeaders * psCredHeaders) = 0;

    // Interface for SetURI method
    virtual SYNCML_DM_RET_STATUS_T SetURI(CPCHAR szURL);

    inline void* operator new(size_t sz)
    {
        return (DmAllocMem(sz));
    }

    inline void operator delete(void* buf)
    {
        DmFreeMem(buf);
    }
};


class SYNCML_DM_OTAConnection : public SYNCML_DM_Connection
{
public:

    SYNCML_DM_OTAConnection();

    virtual ~SYNCML_DM_OTAConnection();

    virtual SYNCML_DM_RET_STATUS_T Init(UINT32 dwMaxAcptSize,
                                        XPL_ADDR_TYPE_T AddressType,
                                        CPCHAR ConRef);

    // Method to send the SyncML document to the SyncML Server
    virtual SYNCML_DM_RET_STATUS_T Send(
            const SYNCML_DM_INDIRECT_BUFFER_T *psSendDoc,
            SYNCML_DM_INDIRECT_BUFFER_T *psRecvDoc,
            const UINT8 *pbContentType,
            const DMCredHeaders * psCredHeaders);

private:

    XPL_HTTP_HANDLE_T m_hConnection;

    // Number of retries that are attempted
    UINT8 bNumRetries;

    // Maximum size of the Message
    MAX_MSG_SIZE_T dwMaxAcceptSize;

    // HTTP header to be sent to the Server
    DMBuffer m_oHttpHdr;

    // SyncML document to be sent to the Server
    SYNCML_DM_INDIRECT_BUFFER_T *psSendSyncMLDoc;

    // SyncML document to be received from the Server
    SYNCML_DM_INDIRECT_BUFFER_T *psRecvSyncMLDoc;

    // Credential headers sent and received from the Server
    DMCredHeaders * m_pCredHeaders;

    DMString m_szConRef;

#ifdef DM_DUMP_SYNCML_PACKAGE

    // The path where the dump package file stores
    DMString dump_path;

    int package_counter;

    char * HTTP_HEADER_SERVER;
    char * HTTP_HEADER_DATE;
    char * HTTP_HEADER_ACCEPT_RANGES;
    char * HTTP_HEADER_CACHE_CONTROL;
    char * HTTP_HEADER_CONNECTION;
    char * HTTP_HEADER_CONTENT_TYPE;
    char * HTTP_HEADER_X_SYNCML_HMAC;

    DMString bodyFileName;
    DMString bodyFileExt;

    DMString hdrFileName;
    DMString hdrFileExt;

#endif

    // Method that prepares request headers
    SYNCML_DM_RET_STATUS_T PrepareRequestHeaders(const UINT8 *pbContentTypetoSend);

    // Method that computes the HTTP header fields length
    UINT16  ComputeHTTPHeaderLength(const UINT8 *pbContentTypetoSend);

    // Method that Sends the Inital chunk of the SyncML document
    SYNCML_DM_RET_STATUS_T SendInitialChunk(void);

    SYNCML_DM_RET_STATUS_T  ProcessCredHeaders(CPCHAR pbOrigHmacStr);

    SYNCML_DM_RET_STATUS_T IssueURLRequest(XPL_HTTP_CODE_T *ret_code);

    SYNCML_DM_RET_STATUS_T ConvertXPLCode(XPL_HTTP_RET_STATUS_T http_result);
};

#else //under android platform

#include "JNIHelp.h"
#include "jni.h"

class SYNCML_DM_OTAConnection
{
    public:

        SYNCML_DM_OTAConnection();

        ~SYNCML_DM_OTAConnection();

        SYNCML_DM_RET_STATUS_T Init(UINT32 dwMaxAcptSize,
                XPL_ADDR_TYPE_T AddressType,
                CPCHAR ConRef);

        // Method to send the SyncML document to the SyncML Server
        SYNCML_DM_RET_STATUS_T Send(
                const SYNCML_DM_INDIRECT_BUFFER_T *psSendDoc,
                SYNCML_DM_INDIRECT_BUFFER_T *psRecvDoc,
                const UINT8 *pbContentType,
                const DMCredHeaders * psCredHeaders);

        SYNCML_DM_RET_STATUS_T SetURI(CPCHAR szURL);

    private:

        SYNCML_DM_RET_STATUS_T  ProcessCredHeaders(CPCHAR pbOrigHmacStr);
        UINT32 m_maxAcptSize;

        DMString m_szURL;

        jobject m_jNetConnObj;

        jmethodID m_jSendRequest;
        jmethodID m_jGetRespLength;
        jmethodID m_jGetRespData;
        jmethodID m_jSetContentType;
        jmethodID m_jclose;
        jmethodID m_jEnbleApnByName;

        // Credential headers sent and received from the Server
        DMCredHeaders * m_pCredHeaders;
};
#endif

//---------------------------------------------------------------------
#endif  /* _DM_TPT_CONNECTION_H */
