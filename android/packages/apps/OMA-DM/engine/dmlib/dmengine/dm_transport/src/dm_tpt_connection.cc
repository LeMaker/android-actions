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

//-----------------------------------------------------------------------
//                                                                               
//   Module Name: dm_tpt_connection.cc
//
//   General Description: This file contains the Implementation of
//                        SYNCML_DM_OTAConnection class
//
//------------------------------------------------------------------------

#include "dm_tpt_connection.H"
#include "dm_tree_util.h"
#include "xpl_Logger.h"
#include "xpl_File.h"

//------------------------------------------------------------------------
//                                       LOCAL MACROS
//------------------------------------------------------------------------

// Size of MD5 string
#define DMTPT_MD5_STRING_SIZE           3
 
// Maximum size of the MAC string size
#define DMTPT_MAX_MAC_SIZE              100

// Maximiun size of the Algorithm string
#define DMTPT_MAX_ALGO_SIZE              10

// Maximiun value of the Request ID.
#define DMTPT_MAX_REQ_ID_VALUE          30000

// Language to be used
#define DMTPT_HTTP_LANGUAGE             "en"

// Name of the DM Client
#define DMTPT_CLIENT_NAME        "Motorola SyncML DM Client ["DMTPT_HTTP_LANGUAGE"] "

// Version of the HTTP protocol used
#define DMTPT_HTTP_VERSION              "HTTP/1.0"

#define DMTPT_HTTP_MIME_TYPES           "application/vnd.syncml.dm+wbxml"

// Specifying the Cache-control Http header field option
#define DMTPT_HTTP_CACHECONTROL         "private"

// Defining location string
// WSP module converts headers to lower-case
#define DMTPT_FIELD_LOCATION            "location:"

// Defining location string
// HTTP stack maintains headers in mixed-case
#define DMTPT_FIELD_LOCATION_HTTP              "Location:"

// Size of the Location string
#define DMTPT_LOCATION_SIZE               9

// User Agent Header Field
#define DMTPT_USER_AGENT_HF "User-Agent: "   
#define DMTPT_USER_AGENT_HF_LEN   12

//Accept Haeder field
#define DMTPT_ACCEPT_HF    "Accept: "  
#define DMTPT_ACCEPT_HF_LEN   8

//Accept language header field
#define DMTPT_ACCEPT_LANG_HF "Accept-Language: "  
#define DMTPT_ACCEPT_LANG_HF_LEN  17

//Accept Charset header 
#define DMTPT_ACCEPT_CHARSET   "Accept-Charset: utf-8"
#define DMTPT_ACCEPT_CHARSET_LEN    21

//Cache control header field
#define DMTPT_CACHE_CONTROL_HF  "Cache-Control: " 
#define DMTPT_CACHE_CONTROL_HF_LEN  15

//Content type header field
#define DMTPT_CONT_TYPE_HF  "Content-type: "  
#define DMTPT_CONT_TYPE_HF_LEN   14


//HMAC Algo
#define DMTPT_HMAC_ALGO_HF  "x-syncml-hmac: algorithm="
#define DMTPT_HMAC_ALGO_HF_LEN   25

//HMAC uname 
#define DMTPT_HMAC_UNAME_HF  ", username="
#define DMTPT_HMAC_UNAME_HF_LEN   11

//HMAC mac
#define DMTPT_HMAC_MAC_HF   ", mac="
#define  DMTPT_HMAC_MAC_HF_LEN    6

//Carriage return line feed string
#define DMTPT_CR_LF  "\r\n"
#define DMTPT_CR_LF_LEN   2

// two double quotes " "
#define DMTPT_TWO_DOUBLEQUOTES   2


//------------------------------------------------------------------------
//                                     CLASS IMPLEMENTATION
//------------------------------------------------------------------------

SYNCML_DM_OTAConnection::SYNCML_DM_OTAConnection()
{
    bNumRetries = 0;
    m_hConnection = 0;
    psSendSyncMLDoc = NULL;
    psRecvSyncMLDoc = NULL;
}

SYNCML_DM_OTAConnection::~SYNCML_DM_OTAConnection()
{
    if ( m_hConnection )
        XPL_HTTP_Close(m_hConnection);
}

//========================================================================
//
// DESCRIPTION: Init routine
//
// ARGUMENTS PASSED:
//          INPUT : UINT32 dwMaxAcptSize - Maximum size of the message
//                  UINT8 bWebSessIndex - Web session index to be used
//                  UINT16 wWapGatePortno - WAP Gateway port number
//                  SYNCML_DM_ADDR_TYPE_T AddressType - Address type 
//                                    provided by the DMUA.
//                                    
//             
//
//          OUTPUT:
//               None
//
// RETURN VALUE: None
//              
//
// IMPORTANT NOTES: The constructor gets called when the SYNCML_DM_OTATransport 
//                  creates a new SYNCML_DM_OTAConnection object.
//===========================================================================

SYNCML_DM_RET_STATUS_T SYNCML_DM_OTAConnection::Init(UINT32 dwMaxAcptSize, 
                             XPL_ADDR_TYPE_T AddressType,
                             CPCHAR ConRef) 
{
    
    // Initialize the member variables
    dwMaxAcceptSize = dwMaxAcptSize;
    AddrType = AddressType;
 
    // Copy the new ConRef to the pConRef member variable.
    if(ConRef != NULL && ConRef[0] != 0)
    {
        m_szConRef = ConRef;
        if( m_szConRef.GetBuffer() ==NULL )
        {
            XPL_LOG_DM_SESS_Error(("SYNCML_DM_OTAConnection::Init : unable allocate memory\n"));
            return SYNCML_DM_DEVICE_FULL;
        }
    }

#ifdef DM_DUMP_SYNCML_PACKAGE
    dump_path = XPL_DM_GetEnv(SYNCML_DM_DUMP_SESSION_PACKAGE_PATH);

    if (dump_path == NULL) 
    {
       dump_path="/tmp";
    }   
    package_counter = 1;
   
    HTTP_HEADER_SERVER = "Server";
    HTTP_HEADER_DATE = "Date";
    HTTP_HEADER_ACCEPT_RANGES = "Accept-Ranges";
    HTTP_HEADER_CACHE_CONTROL = "Cache-Control";
    HTTP_HEADER_CONNECTION = "Connection";
    HTTP_HEADER_CONTENT_TYPE = "Content-Type";
    HTTP_HEADER_X_SYNCML_HMAC = "x-syncml-hmac";

    bodyFileName = "/package";
    bodyFileExt = ".xml";
  
    hdrFileName = "/header";
    hdrFileExt = ".txt";
#endif  

    return SYNCML_DM_SUCCESS;
}

//==============================================================================
// FUNCTION: SYNCML_DM_OTAConnection::Send
//
// DESCRIPTION: This function sends the data to the Server
//
// ARGUMENTS PASSED:
//          INPUT : psSendSyncMLDocument - Pointer to the SyncML DM document 
//                                         to be sent
//                psRecvSyncMLDocument - Pointer to the memory, where
//                                        the received document will be stored.  
//                pbContType -           Pointer to the content type of the
//                                         SyncML DM document that will be sent.
//                   psCredHdr          - Pointer to the HMAC Credential header
//
//             
//
//          OUTPUT:
//                 
//
// RETURN VALUE: SYNCML_DM_RET_STATUS_T
//               SYNCML_DM_SUCCESS - If data is sent successfully to the Fetch Agent
//
//               SYNCML_DM_FAIL  - There were some problems either with the data 
//                                  or while sending
//
//               SYNCML_DM_SEC_CONN_NOT_AVAILABLE - Secure connection is required, 
//                                                   but it is not available.
// 
//              
//
// IMPORTANT NOTES: DMUA calls this function to send the data to the Server.
//==============================================================================

SYNCML_DM_RET_STATUS_T SYNCML_DM_OTAConnection::Send(
       const SYNCML_DM_INDIRECT_BUFFER_T *psSendSyncMLDocument, 
        SYNCML_DM_INDIRECT_BUFFER_T *psRecvSyncMLDocument,  
        const UINT8 *pbContType, 
        const DMCredHeaders * psCredHdr)
{

    XPL_LOG_DM_SESS_Debug(("Enter SYNCML_DM_OTAConnection::Send \n"));
    
    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_SUCCESS;

    m_pCredHeaders = (DMCredHeaders*)psCredHdr;
    
    // Check whether the input parameters are valid
    
    if ((psSendSyncMLDocument EQ NULL) OR
        (psSendSyncMLDocument->pData EQ NULL) OR
            (psSendSyncMLDocument->dataSize EQ 0 ))
    {
        return SYNCML_DM_FAIL;
    }
    
    if ((psRecvSyncMLDocument EQ NULL)  OR
        (psRecvSyncMLDocument->pData EQ NULL))
    {
        return SYNCML_DM_FAIL;
    }
    
    if ((pbContType EQ NULL) OR (pbContType[0] EQ '\0'))
    {
        return SYNCML_DM_FAIL;
    }
    
    // Check whether psCredHdr is valid
    if ( psCredHdr->isCorrect() == FALSE )
        return SYNCML_DM_FAIL;
    
    
    psSendSyncMLDoc = (SYNCML_DM_INDIRECT_BUFFER_T*)psSendSyncMLDocument;
    
    psRecvSyncMLDoc = psRecvSyncMLDocument;
    
    //Call PrepareRequestHeaders method of this class with pbConType,
    //psCredHdr parameters to prepare the HTTP headers.
    ret_status = PrepareRequestHeaders(pbContType);
    
    if (ret_status == SYNCML_DM_SUCCESS )
         ret_status = SendInitialChunk();
    
    return ret_status;
    
}


//==============================================================================
// FUNCTION: SYNCML_DM_Connection::SetURI
//
// DESCRIPTION: This method sets the URL of the Server
//
// ARGUMENTS PASSED:
//          INPUT : pbUrl - URL of the Server
//             
//
//          OUTPUT: None
//
// RETURN VALUE:   SYNCML_DM_RET_STATUS_T
//                 SYNCML_DM_SUCCESS - Whether the new URL has been set 
//                                      successfully
//                 SYNCML_DM_FAIL   - Setting the new URL met with failure

//              
//
// IMPORTANT NOTES: This method is called by the DMUA and the
//                  SYNCML_DM_OTATransport to set the new URL of the Server.
//==============================================================================

SYNCML_DM_RET_STATUS_T SYNCML_DM_Connection::SetURI(CPCHAR szURL) 
{
    if (szURL EQ NULL)
        return SYNCML_DM_FAIL;
    
    m_szURL = szURL;
    if ( m_szURL.GetBuffer() == NULL )
    {
        XPL_LOG_DM_SESS_Error(("SYNCML_DM_OTAConnection::SetURI : unable allocate memory\n"));
        return SYNCML_DM_DEVICE_FULL; 
    }
    return SYNCML_DM_SUCCESS;
    
}


//==============================================================================
// FUNCTION: SYNCML_DM_OTAConnection::PrepareRequestHeaders
//
// DESCRIPTION: This method prepares the Request Headers
//
// ARGUMENTS PASSED:
//          INPUT :  pbContentTypetoSend -Pointer to the Content type of the 
//                                        document to be sent
//                   psCredHeaderstoSend - Pointer to the Credential headers to
//                                         be sent.
//             
//
//          OUTPUT:
//               None
//
// RETURN VALUE: BOOLEAN - TRUE - Successfully prepared the HTTP headers
//                         FALSE - HTTP headers preparation failed.
//              
//
// IMPORTANT NOTES: The SYNCML_DM_OTAConnection class this method.
//==============================================================================
SYNCML_DM_RET_STATUS_T SYNCML_DM_OTAConnection::PrepareRequestHeaders(const UINT8 *pbContentTypetoSend)
{
    char *pBuffer = NULL;
    UINT32 size = 0;

    XPL_LOG_DM_SESS_Debug(("Enter SYNCML_DM_OTAConnection::PrepareRequestHeaders \n"));

    // Free the old HTTP headers
    m_oHttpHdr.clear();
    
    // Compute the HTTP header string length
    size = ComputeHTTPHeaderLength(pbContentTypetoSend);
    
    // Allocate memory for the new string
    m_oHttpHdr.allocate(size);
    pBuffer = (char*)m_oHttpHdr.getBuffer();
    if ( pBuffer == NULL )
        return SYNCML_DM_DEVICE_FULL;
   
    
    // Prepare General HTTP header fields
    size = DmSprintf(pBuffer,DMTPT_CACHE_CONTROL_HF \
                             DMTPT_HTTP_CACHECONTROL \
                             DMTPT_CR_LF);
    
    
    // Prepare Request HTTP header fields
    size += DmSprintf(pBuffer + size,DMTPT_USER_AGENT_HF \
                                     DMTPT_CLIENT_NAME \
                                     DMTPT_CR_LF \
                                     DMTPT_ACCEPT_HF \
                                     DMTPT_HTTP_MIME_TYPES \
                                     DMTPT_CR_LF \
                                     DMTPT_ACCEPT_LANG_HF \
                                     DMTPT_HTTP_LANGUAGE \
                                     DMTPT_CR_LF );
    
    // Prepare Entity HTTP header fields
    size += DmSprintf(pBuffer + size,DMTPT_CONT_TYPE_HF "%s" DMTPT_CR_LF,pbContentTypetoSend);

    if ( m_pCredHeaders->empty() == FALSE ) 
    {
        UINT8 bTempAlgo[DMTPT_MAX_ALGO_SIZE+1] = {0};
        if ( m_pCredHeaders->m_oAlgorithm.getSize() == 0 )
            DmStrcpy((char*)bTempAlgo,"MD5");
        else
            m_pCredHeaders->m_oAlgorithm.copyTo((char*)bTempAlgo);

         size += DmSprintf(pBuffer + size,DMTPT_HMAC_ALGO_HF "%s" \
                                          DMTPT_HMAC_UNAME_HF "\"%s\"" \
                                          DMTPT_HMAC_MAC_HF "%s" DMTPT_CR_LF,
                                          bTempAlgo,
                                         (CPCHAR)m_pCredHeaders->m_oUserName.getBuffer(),
                                         (CPCHAR)m_pCredHeaders->m_oMac.getBuffer());
        
    }    
    m_oHttpHdr.setSize(size);
    return SYNCML_DM_SUCCESS;
}

//==============================================================================
// FUNCTION: SYNCML_DM_OTAConnection::ComputeHTTPHeaderLength
//
// DESCRIPTION: This method computes the length of the HTTP header fields
//
// ARGUMENTS PASSED:
//          INPUT : pbContentTypetoSend -Pointer to the Content type of the 
//                                        document to be sent
//                   psCredHeaderstoSend - Pointer to the Credential headers to
//                                         be sent.
//             
//
//          OUTPUT:
//               None
//
// RETURN VALUE: Length of the HTTP Header fields
//              
//
// IMPORTANT NOTES: This class calls this method. We support ATMOST 64K chunk of
//                  data to be sent to the Server. That's why the return type is
//                  UINT16.
//==============================================================================

UINT16  SYNCML_DM_OTAConnection::ComputeHTTPHeaderLength(const UINT8 *pbContentTypetoSend)
{
    
    //Find the length of the following individual header fields to compute 
    // the total length of the HTTP header: Cache-Control, User-Agent,
    //Accept, Accept-Language, Accept-Charset, Content-Type, Content-Length,
    //x-syncml-hmac.
    
    UINT16 wTotalLength =0;
    UINT8  bCrLfLen =0;

    bCrLfLen = DmStrlen(DMTPT_CR_LF) ;
    
    wTotalLength = DMTPT_USER_AGENT_HF_LEN + DmStrlen(DMTPT_CLIENT_NAME) +
                       bCrLfLen  +
                       DMTPT_ACCEPT_HF_LEN + DmStrlen(DMTPT_HTTP_MIME_TYPES) +
                       bCrLfLen  +
                       DMTPT_ACCEPT_LANG_HF_LEN + DmStrlen(DMTPT_HTTP_LANGUAGE) +
                       bCrLfLen +
                       DMTPT_ACCEPT_CHARSET_LEN + bCrLfLen  +
                       DMTPT_CACHE_CONTROL_HF_LEN + DmStrlen(DMTPT_HTTP_CACHECONTROL)+
                       bCrLfLen ;

    wTotalLength += DMTPT_CONT_TYPE_HF_LEN + DmStrlen((CPCHAR)pbContentTypetoSend) + bCrLfLen ;
                
    if ( m_pCredHeaders->empty() == FALSE ) 
    {

        wTotalLength += DMTPT_HMAC_ALGO_HF_LEN +  DMTPT_HMAC_UNAME_HF_LEN +
                                m_pCredHeaders->m_oUserName.getSize() +
                                DMTPT_HMAC_MAC_HF_LEN +
                                m_pCredHeaders->m_oMac.getSize() +
                                DMTPT_TWO_DOUBLEQUOTES +
                                bCrLfLen;
        
        if ( m_pCredHeaders->m_oAlgorithm.getSize() == 0)        
            wTotalLength += DMTPT_MD5_STRING_SIZE;
        else
            wTotalLength += m_pCredHeaders->m_oAlgorithm.getSize();
    }
    
    return wTotalLength;
    
}


//==============================================================================
// FUNCTION: SYNCML_DM_OTAConnection::ProcessCredHeaders
//
// DESCRIPTION: This method extracts the Credential headers from the 
//               Response headers
//
// ARGUMENTS PASSED:
//          INPUT : Pointer to the Response header
//             
//
//          OUTPUT: None
//
// RETURN VALUE:    BOOLEAN
//                  TRUE - If handled successfully
//                  FALSE - If there is some failure

//              
//
// IMPORTANT NOTES: The HandleOTARedirect method calls this method.
//==============================================================================
SYNCML_DM_RET_STATUS_T SYNCML_DM_OTAConnection::ProcessCredHeaders(CPCHAR pbOrigHmacStr)
{
    
    UINT8 *pbHmacString = NULL;
    UINT8 *pbInitialHmacString = NULL;
    UINT8 *pbParam = NULL;
    UINT8 *pbValue = NULL;
    char  *pbAlgo = NULL;
    char  *pbUname = NULL;
    char  *pbMAC = NULL;
    
    XPL_LOG_DM_SESS_Debug(("Enter SYNCML_DM_OTAConnection::ProcessCredHeaders\n"));
    if(pbOrigHmacStr EQ NULL)
        return TRUE;
  
    //Trim the blank space and tabs
    pbHmacString = DMTPTTrimString((UINT8*)pbOrigHmacStr);
    if (pbHmacString EQ NULL)
    {
        return SYNCML_DM_FAIL;
    }
        
    pbInitialHmacString = pbHmacString;
        
    pbHmacString = (UINT8*)DmStrstr((CPCHAR)pbHmacString, "algorithm");
        
    if (pbHmacString EQ NULL)
        pbHmacString = (UINT8*)DmStrstr((CPCHAR)pbInitialHmacString,"username");
            
        //Extract the algorithm, Username and mac from 
        //the x-syncml-hmac header
     while (pbHmacString NE NULL)
     {
         pbHmacString = DM_TPT_splitParamValue(pbHmacString,&pbParam,&pbValue);
            
         if ((pbParam NE NULL) AND (pbParam [0] NE '\0'))
         {
             if (!DmStrcmp ((CPCHAR)pbParam, "algorithm"))
             {
                 pbAlgo = (char*)pbValue;
             }
             else 
                if (!DmStrcmp ((CPCHAR)pbParam, "username"))
                {
                    pbUname = (char*)pbValue;
                }
                else 
                    if (!DmStrcmp ((CPCHAR)pbParam, "mac"))
                    {
                        pbMAC = (char*)pbValue;
                    }
         }
    }
        
        // Allocate memory to hold username, mac, algorithm
    if (pbUname == NULL || pbMAC == NULL)
    {
        DmFreeMem(pbInitialHmacString);
        return SYNCML_DM_FAIL;
    } 
        
    if (pbAlgo NE NULL)
        m_pCredHeaders->m_oAlgorithm.assign(pbAlgo); 
    else
        m_pCredHeaders->m_oAlgorithm.assign("MD5"); 
            
    if ( m_pCredHeaders->m_oAlgorithm.getBuffer() == NULL )
    {
        DmFreeMem(pbInitialHmacString);
        return SYNCML_DM_DEVICE_FULL;
    }

    m_pCredHeaders->m_oUserName.assign(pbUname);
    
    if ( m_pCredHeaders->m_oUserName.getBuffer() == NULL )
    {
        DmFreeMem(pbInitialHmacString);
        return SYNCML_DM_DEVICE_FULL;
    }

    m_pCredHeaders->m_oMac.assign(pbMAC);
    
    if ( m_pCredHeaders->m_oMac.getBuffer() == NULL )
    {
        DmFreeMem(pbInitialHmacString);
        return SYNCML_DM_DEVICE_FULL;
    }
        
    DmFreeMem(pbInitialHmacString);
    XPL_LOG_DM_SESS_Debug(("Leave SYNCML_DM_OTAConnection::ProcessCredHeaders\n"));
    return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T SYNCML_DM_OTAConnection::ConvertXPLCode(XPL_HTTP_RET_STATUS_T http_result)
{
    SYNCML_DM_RET_STATUS_T return_result;
    switch ( http_result )
    {
        case XPL_HTTP_RET_NW_NOT_AVAILABLE:
            return_result = SYNCML_DM_SESSION_NW_NOT_AVAILABLE; 
            break;
            
        case XPL_HTTP_RET_NO_CONNECT:
            return_result = SYNCML_DM_SESSION_NO_CONNECT; 
            break;
            
        default :
            return_result = SYNCML_DM_FAIL;
            break;
    }
    return return_result;
}      


/*******************************************************************************
*
*    Function      : FA_issue_urlRequest()
*    Parameters    : user id, request id, URL, method type, headers,
*                    data buffer(post), content preferences, 
*                    progress indicator.
*    Return Value  : Boolean (success or failure)
*    Description   : This api checks for the validity of the parameters received.
*                    Additional check for data buffer is made for Put and Post
*                    requests. Finally a message FA_GET_URL is constructed and 
*                    posted to the Fetch Module.
*
********************************************************************************/

SYNCML_DM_RET_STATUS_T SYNCML_DM_OTAConnection::IssueURLRequest(XPL_HTTP_CODE_T *ret_code)
{
    char * value = NULL;
    UINT32 receivedLength = 0;
    SYNCML_DM_RET_STATUS_T return_result = SYNCML_DM_SUCCESS;
    XPL_HTTP_RET_STATUS_T http_result = XPL_HTTP_OK_200;

    XPL_LOG_DM_SESS_Debug(("Enter SYNCML_DM_OTAConnection::IssueURLRequest \n"));

#ifdef DM_DUMP_SYNCML_PACKAGE
 
    XPL_FS_HANDLE_T     hBodyFile   = XPL_FS_HANDLE_INVALID;
    XPL_FS_HANDLE_T     hHeaderFile = XPL_FS_HANDLE_INVALID;
    XPL_FS_RET_STATUS_T nStatus     = XPL_FS_RET_FAIL;
    char                szName [512];
    
    DmSprintf( szName, "%s%s%d_request%s", dump_path.c_str(), hdrFileName.c_str(),
                 package_counter, hdrFileExt.c_str() );

    hHeaderFile = XPL_FS_Open(  szName,
                                    XPL_FS_FILE_WRITE, 
                                    &nStatus );
#endif

    // Use open method for the first time
    if (m_hConnection == 0)
        m_hConnection = XPL_HTTP_Open((CPCHAR)m_szURL, (CPCHAR)m_szConRef, AddrType,&http_result);
    else
        http_result = XPL_HTTP_SetUrl(m_hConnection,(CPCHAR)m_szURL, (CPCHAR)m_szConRef, AddrType);
    
    if ( http_result != XPL_HTTP_RET_SUCCESS )
        return SYNCML_DM_FAIL;
   

    http_result = XPL_HTTP_SetRequestMethod(m_hConnection,XPL_HTTP_METHOD_POST);
    if ( http_result !=  XPL_HTTP_RET_SUCCESS)
    {
        return_result = ConvertXPLCode(http_result);
        goto GetResponseCode;
    }

    http_result = XPL_HTTP_SetRequestProperty(m_hConnection,(CPCHAR)m_oHttpHdr.getBuffer());
    if ( http_result !=  XPL_HTTP_RET_SUCCESS)
    {
        return_result = ConvertXPLCode(http_result);
        goto GetResponseCode;
    }


#ifdef DM_DUMP_SYNCML_PACKAGE
    // capture header info
    if (  XPL_FS_HANDLE_INVALID != hHeaderFile ) 
    {
        XPL_FS_Write( hHeaderFile,
                      m_oHttpHdr.getBuffer(),
                      m_oHttpHdr.getSize(),
                      &nStatus );

        XPL_FS_Close( hHeaderFile );
        hHeaderFile = XPL_FS_HANDLE_INVALID;
    }
  
    DmSprintf( szName, "%s%s%d_request%s", dump_path.c_str(), bodyFileName.c_str(),
             package_counter++, bodyFileExt.c_str() );
  
    hBodyFile = XPL_FS_Open(  szName,
                              XPL_FS_FILE_WRITE, 
                              &nStatus );

    if ( XPL_FS_HANDLE_INVALID != hBodyFile )
    {
        XPL_FS_Write( hBodyFile,
                      psSendSyncMLDoc->pData,
                      psSendSyncMLDoc->dataSize,
                      &nStatus );

        XPL_FS_Close( hBodyFile );
        hBodyFile = XPL_FS_HANDLE_INVALID;
    }
#endif  
    
    // Upload buffer
    http_result = XPL_HTTP_Send(m_hConnection,(CPCHAR)psSendSyncMLDoc->pData,psSendSyncMLDoc->dataSize);
    if ( http_result !=  XPL_HTTP_RET_SUCCESS)
    {
        return_result = ConvertXPLCode(http_result);
        goto GetResponseCode;
    }
   
    receivedLength = XPL_HTTP_GetResponseLength(m_hConnection);

    if (receivedLength EQ 0) 
    {
        receivedLength = dwMaxAcceptSize;
    }

    if (receivedLength GT dwMaxAcceptSize)
    {
        return_result = SYNCML_DM_FAIL;
        goto GetResponseCode;    
    }

    psRecvSyncMLDoc->dataSize  = receivedLength;

    http_result = XPL_HTTP_GetResponse(m_hConnection,(char *)psRecvSyncMLDoc->pData,(INT32)receivedLength);
    if ( http_result !=  XPL_HTTP_RET_SUCCESS)
    {
        return_result = ConvertXPLCode(http_result);
        goto GetResponseCode;
    }

#ifdef DM_DUMP_SYNCML_PACKAGE
    // open a file to capture response header info
    value=(char*)DmAllocMem(512);
    DmSprintf( szName, "%s%s%d_response%s", dump_path.c_str(), hdrFileName.c_str(), package_counter, hdrFileExt.c_str() );
  
    hHeaderFile = XPL_FS_Open(  szName,
                                XPL_FS_FILE_WRITE, 
                                &nStatus );
  
   // capture header info
    if (  XPL_FS_HANDLE_INVALID != hHeaderFile ) 
    {
        const char           szCRLF[] = "\r\n";
        const XPL_FS_COUNT_T nCRLFlen = DmStrlen(szCRLF);
          
        if((XPL_HTTP_GetHeaderField(m_hConnection,HTTP_HEADER_SERVER, &value)) EQ XPL_HTTP_RET_SUCCESS) 
        {        
          XPL_FS_Write( hHeaderFile,
                        value,
                        DmStrlen(value),
                        &nStatus );

          XPL_FS_Write( hHeaderFile,
                        (void*)szCRLF,
                        nCRLFlen,
                        &nStatus );
        }

        if((XPL_HTTP_GetHeaderField(m_hConnection,HTTP_HEADER_DATE, &value)) EQ XPL_HTTP_RET_SUCCESS) 
        {
          XPL_FS_Write( hHeaderFile,
                        value,
                        DmStrlen(value),
                        &nStatus );

          XPL_FS_Write( hHeaderFile,
                        (void*)szCRLF,
                        nCRLFlen,
                        &nStatus );
        }

        if((XPL_HTTP_GetHeaderField(m_hConnection,HTTP_HEADER_ACCEPT_RANGES, &value)) EQ XPL_HTTP_RET_SUCCESS) 
        {
          XPL_FS_Write( hHeaderFile,
                        value,
                        DmStrlen(value),
                        &nStatus );

          XPL_FS_Write( hHeaderFile,
                        (void*)szCRLF,
                        nCRLFlen,
                        &nStatus );
        }

        if((XPL_HTTP_GetHeaderField(m_hConnection,HTTP_HEADER_CACHE_CONTROL, &value)) EQ XPL_HTTP_RET_SUCCESS) 
        {
          XPL_FS_Write( hHeaderFile,
                        value,
                        DmStrlen(value),
                        &nStatus );

          XPL_FS_Write( hHeaderFile,
                        (void*)szCRLF,
                        nCRLFlen,
                        &nStatus );
        }

        if((XPL_HTTP_GetHeaderField(m_hConnection,HTTP_HEADER_CONNECTION, &value)) EQ XPL_HTTP_RET_SUCCESS) 
        {
          XPL_FS_Write( hHeaderFile,
                        value,
                        DmStrlen(value),
                        &nStatus );

          XPL_FS_Write( hHeaderFile,
                        (void*)szCRLF,
                        nCRLFlen,
                        &nStatus );
        }

        if((XPL_HTTP_GetHeaderField(m_hConnection,HTTP_HEADER_CONTENT_TYPE, &value)) EQ XPL_HTTP_RET_SUCCESS) 
        {
          XPL_FS_Write( hHeaderFile,
                        value,
                        DmStrlen(value),
                        &nStatus );

          XPL_FS_Write( hHeaderFile,
                        (void*)szCRLF,
                        nCRLFlen,
                        &nStatus );
        }

        if((XPL_HTTP_GetHeaderField(m_hConnection,HTTP_HEADER_X_SYNCML_HMAC, &value)) EQ XPL_HTTP_RET_SUCCESS) 
        {
          XPL_FS_Write( hHeaderFile,
                        value,
                        DmStrlen(value),
                        &nStatus );

          XPL_FS_Write( hHeaderFile,
                        (void*)szCRLF,
                        nCRLFlen,
                        &nStatus );
        }

        XPL_FS_Close( hHeaderFile );
        hHeaderFile = XPL_FS_HANDLE_INVALID;
    }

    DmSprintf( szName, "%s%s%d_response%s", dump_path.c_str(), bodyFileName.c_str(), package_counter++, bodyFileExt.c_str() );
  
    hBodyFile = XPL_FS_Open(  szName,
                              XPL_FS_FILE_WRITE, 
                              &nStatus );

    if ( XPL_FS_HANDLE_INVALID != hBodyFile )
    {
        XPL_FS_Write( hBodyFile,
                      psRecvSyncMLDoc->pData,
                      receivedLength,
                      &nStatus );

        XPL_FS_Close( hBodyFile );
        hBodyFile = XPL_FS_HANDLE_INVALID;
    }

    DmFreeMem(value);
    value = NULL;
#endif  

    // Clean the Previously received Credential headers
    m_pCredHeaders->clear();
    value=(char*)DmAllocMem(512); 
    http_result = XPL_HTTP_GetHeaderField(m_hConnection,"x-syncml-hmac", &value);
    if ( http_result ==  XPL_HTTP_RET_SUCCESS)
    {
       return_result = ProcessCredHeaders(value);
       if ( return_result != SYNCML_DM_SUCCESS )
       {
            DmFreeMem(value);
            XPL_HTTP_CloseReq(m_hConnection);
            return SYNCML_DM_FAIL;
       }
    }
    else
       return_result = ConvertXPLCode(http_result);
    DmFreeMem(value);
    
 GetResponseCode:

    *ret_code = XPL_HTTP_GetResponseCode(m_hConnection);
    XPL_HTTP_CloseReq(m_hConnection);

    XPL_LOG_DM_SESS_Debug(("Exit from IssueURLRequest\n", return_result));
    
    return SYNCML_DM_SUCCESS;
}

//==============================================================================
// FUNCTION: SYNCML_DM_OTAConnection::SendInitialChunk
//
// DESCRIPTION: This method sends the initial chunk of data to the server.
//
// ARGUMENTS PASSED:
//          INPUT : None
//             
//
//          OUTPUT:
//               None
//
// RETURN VALUE:  BOOLEAN -
//                 TRUE - Whether the data is successfully posted to the 
//                        Fetch agent
//                  FALSE - posting the data to the Fetch Agent met with failure.
//              
//
// IMPORTANT NOTES: Send method calls this method.
//==============================================================================
SYNCML_DM_RET_STATUS_T SYNCML_DM_OTAConnection::SendInitialChunk()
{
    

    SYNCML_DM_RET_STATUS_T return_result;
    int  wNumRetries = 0;
    XPL_HTTP_CODE_T ret_code = XPL_HTTP_OK_200;
    
    XPL_LOG_DM_SESS_Debug(("Enter SYNCML_DM_OTAConnection::SendInitialChunk \n"));

    if (m_szURL == NULL)
       return SYNCML_DM_FAIL;

    while (wNumRetries LT DMTPT_MAX_RETRIES)
    {
        return_result = IssueURLRequest(&ret_code);
        if(return_result != SYNCML_DM_SUCCESS)
            return return_result;

        switch(ret_code)
        {
            case XPL_HTTP_REQUEST_TIMEOUT_408:
            case XPL_HTTP_INTERNAL_SERVER_ERROR_500:
            case XPL_HTTP_BAD_GATEWAY_502:
            case XPL_HTTP_SERVICE_UNAVAILABLE_503:
            case XPL_HTTP_GATEWAY_TIMEOUT_504:
                wNumRetries++;
                break;
        
            case XPL_HTTP_OK_200:
              return SYNCML_DM_SUCCESS;
        
            default:
              return SYNCML_DM_FAIL;
        }        
    }

    XPL_LOG_DM_SESS_Debug(("OTAConnection send failed due failure in SendInitialChunk\n"));
    return SYNCML_DM_FAIL;
}
