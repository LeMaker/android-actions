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

#ifndef DMSOCKETCONNECTOR_H
#define DMSOCKETCONNECTOR_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

#include <sys/types.h>     // Needed for system defined identifiers.
#include <netinet/in.h>    // Needed for internet address structure.
#include <sys/socket.h>    // Needed for socket(), bind(), etc...

#include "dmvector.h"
#include "dmt.hpp"
#include "xpl_HTTP.h"

class DmSocketConnector 
{
 public :	
  DmSocketConnector() {
    proxy_auth = NULL;
    proxy_enable_check = true;  
    new_session = true;
    portNum = "80";
    socket_portNum = "80";
    responseData = NULL;    
    proxy_url = NULL;
  }

  ~DmSocketConnector() {
    if ( responseData != NULL ) {
       free(responseData);
       responseData = NULL;
    }
  }

	
  SYNCML_DM_RET_STATUS_T Open(CPCHAR url, CPCHAR ConRef, int AddrType);

  SYNCML_DM_RET_STATUS_T SetRequestMethod(XPL_HTTP_METHOD_T method);

  SYNCML_DM_RET_STATUS_T  SetRequestProperty(CPCHAR props);
  
  SYNCML_DM_RET_STATUS_T Send(CPCHAR data, UINT32 size);
  
  UINT32 GetResponseLength();
  
  SYNCML_DM_RET_STATUS_T GetResponse(char * data, UINT32 size); 
  
  SYNCML_DM_RET_STATUS_T GetHeaderField(CPCHAR field, char ** value); 
  
  XPL_HTTP_CODE_T GetResponseCode();
  
  SYNCML_DM_RET_STATUS_T Close();
  SYNCML_DM_RET_STATUS_T CloseReq();
  SYNCML_DM_RET_STATUS_T SetUrl(CPCHAR url, CPCHAR ConRef, int AddrType);

  bool DmStringParserGetItem( DMString& strItem, DMString& strReminder, char cDelim );  

  bool DmParseHTTPHeader( char* strBuffer, int dataBufSize, char** strRemaining, int& lenRemaining);
  bool SetResponseData(unsigned char* dataReceived, int len);

  bool parseURL(DMString strURI, DMString& strAddrPort, DMString& strURIPath);
  bool parseAddrPort(DMString strAddrPort, DMString& strAddr, DMString& strPort);
  SYNCML_DM_RET_STATUS_T doSend(CPCHAR data, UINT32 size);
  SYNCML_DM_RET_STATUS_T getResponse(); 

 private:
  unsigned int         server_s;             // Server socket descriptor
  struct sockaddr_in   server_addr;          // Server Internet address
  DMString             sentBuf;              // Sent buffer
  DMString             urlPath;              // The path part of a URL
  DMString             portNum;
  DMString             ipAddress;
  DMString             responseCode;
  DMString             responseBody;
  DMMap<DMString, DMString>   responseHeaders;  
  UINT32               responseLength;
  const char *               proxy_auth;
  bool                 proxy_enable_check;
  bool                 new_session;
  DMString             socket_ipAddress;
  DMString             socket_portNum;  
  XPL_HTTP_METHOD_T      requestMethod;
  unsigned char*       responseData;
  const char *         proxy_url;

};

DmSocketConnector * DmBrwCreateConnector();
SYNCML_DM_RET_STATUS_T DmBrwDestroyConnector(DmSocketConnector * browser_handler);

#endif
