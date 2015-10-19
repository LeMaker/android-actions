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

/*
 *  DESCRIPTION:
 *      The xpl_HTTP.h header file contains constants and function prototypes
 *      for making HTTP connections.
 */

#ifndef XPL_HTTP_H
#define XPL_HTTP_H

#include "xpl_Port.h"

#ifdef __cplusplus
extern "C" {
#endif

/************** CONSTANTS ****************************************************/

#define XPL_HTTP_HANDLE_INVALID (0)


/************** STRUCTURES, ENUMS, AND TYPEDEFS ******************************/

typedef INT32 XPL_HTTP_HANDLE_T;

enum {
    XPL_HTTP_BAD_REQUEST_400 = 400,
    XPL_HTTP_UNAUTHORIZED_401 = 401,  
    XPL_HTTP_FORBIDDEN_403 = 403,  
    XPL_HTTP_NOT_FOUND_404 = 404,  
    XPL_HTTP_BAD_METHOD_405 = 405,
    XPL_HTTP_NOT_ACCEPTABLE_405 = 406,
    XPL_HTTP_REQUEST_TIMEOUT_408 = 408,
    XPL_HTTP_REQUEST_CONFLICT_409 = 409,
    XPL_HTTP_REQUEST_CONTENT_TOO_LARGE_413 = 413,
    XPL_HTTP_REQUEST_URI_TOO_LARGE_414 = 414,
    XPL_HTTP_UNSUPPORTED_TYPE_415 = 415,
    XPL_HTTP_INTERNAL_SERVER_ERROR_500 =  500,
    XPL_HTTP_NOT_IMPLEMENTED_501 =  501,
    XPL_HTTP_BAD_GATEWAY_502 =  502, 
    XPL_HTTP_SERVICE_UNAVAILABLE_503 = 503,
    XPL_HTTP_GATEWAY_TIMEOUT_504 = 504,
    XPL_HTTP_OK_200 = 200
};    
typedef INT32 XPL_HTTP_CODE_T;

enum {
    XPL_HTTP_METHOD_POST = 0,
    XPL_HTTP_METHOD_GET = 1,
    XPL_HTTP_METHOD_HEAD = 2,
};    
typedef UINT8 XPL_HTTP_METHOD_T;


enum{
    XPL_ADDR_DEFAULT = 0,
    XPL_ADDR_HTTP = 1,
    XPL_ADDR_WSP = 2,
};
typedef UINT16 XPL_ADDR_TYPE_T;


enum
{
   XPL_HTTP_RET_SUCCESS = 0,        
   XPL_HTTP_RET_FAIL = 1,
   XPL_HTTP_RET_CANCELLED = 2,
   XPL_HTTP_RET_BADARGUMENT = 3,
   XPL_HTTP_RET_NOT_SUPPORTED = 4,
   XPL_HTTP_RET_NW_NOT_AVAILABLE = 5,
   XPL_HTTP_RET_NO_CONNECT = 6
   
};
typedef UINT8  XPL_HTTP_RET_STATUS_T;

/*=============================================================================
                                     FUNCTION PROTOTYPES
===============================================================================*/

/* Activetes HTTP transport and set specified URL to connect with server.
* Do not establish actual connection with server */
XPL_HTTP_HANDLE_T XPL_HTTP_Open(CPCHAR url, 
                                CPCHAR ConRef,
                                XPL_ADDR_TYPE_T addrType,
                                XPL_HTTP_RET_STATUS_T * result); 

/* Sets method for URL request. */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_SetRequestMethod(XPL_HTTP_HANDLE_T handler, 
                          XPL_HTTP_METHOD_T method);


/* Sets a general request property. 
* This method is called before Send method */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_SetRequestProperty(XPL_HTTP_HANDLE_T handler,
                            CPCHAR buffer);

/* Sends data to the resource pointed to by the URL. 
* Size specifies size of content to be sent. */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_Send(XPL_HTTP_HANDLE_T handler, 
              CPCHAR buffer, 
              UINT32 size);

/* Returns the response length */
UINT32 XPL_HTTP_GetResponseLength(XPL_HTTP_HANDLE_T handler);

/* Downloads the contents from the resource. The downloaded content is 
* stored in a buffer. */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_GetResponse(XPL_HTTP_HANDLE_T handler, 
                     char * buffer, 
                     UINT32 size);

/* Method returns the value of the header for given header field. */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_GetHeaderField(XPL_HTTP_HANDLE_T handler, 
                        CPCHAR field, 
                        char ** value);

/* Method returns the complete response header as sent by server. */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_GetRespHeader(XPL_HTTP_HANDLE_T handler, 
                       char ** header); 
                       
/* Method returns value of content-type in the response header. */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_GetType(XPL_HTTP_HANDLE_T handler, 
                 char ** content_type); 

/* Gets status code from HTTP/WAP response message.  Returns -1 if no code was
 * received from the response (i.e., the response is not valid HTTP/WAP) */
XPL_HTTP_CODE_T XPL_HTTP_GetResponseCode(XPL_HTTP_HANDLE_T handler);

/* Closes connection with server. */
XPL_HTTP_RET_STATUS_T XPL_HTTP_Close(XPL_HTTP_HANDLE_T handler);

/* Closes request transaction without closing connection with server. */
XPL_HTTP_RET_STATUS_T XPL_HTTP_CloseReq(XPL_HTTP_HANDLE_T handler);

/* Sets URL for new request */ 
XPL_HTTP_RET_STATUS_T XPL_HTTP_SetUrl(XPL_HTTP_HANDLE_T handler ,
                                      CPCHAR url,
                                      CPCHAR ConRef,
                                      XPL_ADDR_TYPE_T addrType);


/* Dowloads content from the resource. The read content is stored in the file */ 
XPL_HTTP_RET_STATUS_T XPL_HTTP_DowloadByFile(XPL_HTTP_HANDLE_T handler, 
                                             CPCHAR file_name);

/* Dowloads content from the resource. The downloaded content is stored in a buffer */ 
XPL_HTTP_RET_STATUS_T XPL_HTTP_DownloadByBuffer(XPL_HTTP_HANDLE_T handler, 
                                                UINT8 * buffer, 
                                                UINT32 size);

#ifdef __cplusplus
}
#endif

#endif /* XPL_HTTP_H */
