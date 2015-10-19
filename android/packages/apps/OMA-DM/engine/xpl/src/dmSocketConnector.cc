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

//--------------------------------------------------------------------------------------------------
//
//   Module Name: dmSocketConnector.cpp
//
//   General Description: DmBrwConnector socket implementation class implementation file.  This allows
//   DM to send and receive SYNCML data through HTTP protocol.  This implementation is platform independent.
//   It can be used in any UNIX or LINUX machine.  It can allow use under Windows provided that Windows
//   has the unix socket libraries.  This implementation also supports go through proxy server.
//
//   Proxy Usage:
//
//   setenv DM_PROXY_URL=123.567.28.167:1080            // please use IP address
//   setenv DM_PROXY_AUTH="Basic ZTExNjdadsfagewwer"
//   
//   Note: The sequences of letters after Basic is the B64 encoding of your 
//         userID:userPW.
//
//   Hint: Here is a web site to do B64 encoding:
//         http://www.ericphelps.com/scripting/samples/Decode.htm 

#include <stdlib.h>        // Needed for exit()
#include <string.h>        // Needed for strcpy() and strlen()
#include <sys/types.h>     // Needed for system defined identifiers.
#include <netinet/in.h>    // Needed for internet address structure.
#include <sys/socket.h>    // Needed for socket(), bind(), etc...
#include <arpa/inet.h>     // Needed for inet_ntoa()
#include <unistd.h>        // Needed for close()
#include <stdio.h>

#include "dmSocketConnector.h"

#define  BUF_SIZE   50000   // Buffer size

static int  g_nPrintfEnabled = getenv("DM_NOPRINTF") == NULL;

/*
 *  Creates a socket connector.
 *
 *  @return a socket connector.
 */
DmSocketConnector * DmBrwCreateConnector() {
    if ( g_nPrintfEnabled ) printf("\nCreate Socket Connector\n");
    DmSocketConnector * socketHandler = NULL;
    socketHandler = new DmSocketConnector();
    return socketHandler;
}

/*
 *  Destroy a socket connector.
 *
 *  @param the handler to a socket connector.
 *  @return success if no error, otherwise return fail
 */
SYNCML_DM_RET_STATUS_T DmBrwDestroyConnector(DmSocketConnector * browser_handler) {
    if ( g_nPrintfEnabled ) printf("Destroy Socket Connector\n");
    if (browser_handler != NULL) {
        delete browser_handler;
    }
    return SYNCML_DM_SUCCESS;
}

/*
 *  Prepair a socket connection by parsing the URL to get information such   
 *  as host, port, and path.  
 *
 *  @param url the URL to be parse
 *  @param ConRef the connection reference
 *  @param AddrType the type of a host address
 *  @return success if get all connection information, otherwise return fail
 */
SYNCML_DM_RET_STATUS_T DmSocketConnector::Open(CPCHAR url, CPCHAR ConRef, int AddrType) {
    if ( g_nPrintfEnabled ) printf("Open URL: %s\n", url);
    DMString strURI = url;
    DMString strAddrPort;
    DMString strURIPath;

    if (parseURL(strURI, strAddrPort, strURIPath)) {   
        if (parseAddrPort(strAddrPort, ipAddress, portNum)) {
            proxy_url = getenv("DM_PROXY_URL");
            if (proxy_url != NULL) {
                urlPath = url;
                DMString tmpURL = proxy_url;
                if (!parseAddrPort(tmpURL, socket_ipAddress, socket_portNum)) {
                    return SYNCML_DM_FAIL;
                }
            } else {
                urlPath += "/";
                urlPath += strURIPath;
                socket_ipAddress = ipAddress;
                socket_portNum = portNum;
            }
            return SYNCML_DM_SUCCESS;
        }
    }
    return SYNCML_DM_FAIL;
}

/*
 *  Dynamically change a socket connection by parsing a new URL to get information such   
 *  as host, port, and path.
 *
 *  @param url the URL to be parse
 *  @param ConRef the connection reference
 *  @param AddrType the type of a host address
 *  @return success if get all connection information, otherwise return fail
 */
SYNCML_DM_RET_STATUS_T DmSocketConnector::SetUrl(CPCHAR url, CPCHAR ConRef, int AddrType) {
    if ( g_nPrintfEnabled ) printf("Set URL: %s\n", url);
    DMString strURI = url;
    DMString strAddrPort;
    DMString strURIPath;

    if (parseURL(strURI, strAddrPort, strURIPath)) {
        if (proxy_url != NULL) {
            urlPath = url;
        } else {
            urlPath = "/";
            urlPath += strURIPath;
        }
    }
    return SYNCML_DM_SUCCESS;
}

/*
 *  Sets the HTTP request method such as GET and POST.
 *
 *  @param method the method to be set
 *  @return success if the method is supported, else return fail
 */
SYNCML_DM_RET_STATUS_T DmSocketConnector::SetRequestMethod(XPL_HTTP_METHOD_T method) {
    requestMethod = method;
    if ( method == XPL_HTTP_METHOD_POST ) {
        if ( g_nPrintfEnabled ) printf("Request Method: POST   CODE: %d\n\n", method);
        sentBuf = "POST ";
        sentBuf += urlPath;
        sentBuf += " HTTP/1.0\r\n";
        sentBuf += "Host: ";
        sentBuf += socket_ipAddress;
        sentBuf += "\r\n";
    } else if ( method == XPL_HTTP_METHOD_GET ) {
        if ( g_nPrintfEnabled ) printf("Request Method: GET    CODE: %d\n", method);
        sentBuf = "GET ";
        sentBuf += urlPath;
        sentBuf += " HTTP/1.1\r\n";
    } else {
      if ( g_nPrintfEnabled ) printf("Error: Request method not supported\n");
      return SYNCML_DM_FAIL;
    }
    return SYNCML_DM_SUCCESS;
}

/*
 *  Sets HTTP request property by appending to the sent buffer. 
 *  If DM_PROXY_AUTH enviornment variable was set, it will add 
 *  a proxy authorization properties in every new session.
 *
 *  @param header_start the name of a property
 *  @param value_start_prt the value of the property
 *  @return success after append to the sent buffer.
 */
SYNCML_DM_RET_STATUS_T DmSocketConnector::SetRequestProperty(CPCHAR props) {
    if ( g_nPrintfEnabled ) printf("Property header: %s\n", props);
    sentBuf += props;

    if (proxy_auth == NULL && proxy_enable_check) {
        proxy_auth = getenv("DM_PROXY_AUTH");       // proxy_auth=Basic userid:passwd
        proxy_enable_check = false;
    }

    if (proxy_auth != NULL && new_session) {
        if ( g_nPrintfEnabled ) printf ("Property header: Proxy-Authorization=%s\n", proxy_auth);
        sentBuf += "Proxy-Authorization";
        sentBuf += ": ";
        sentBuf += proxy_auth;
        sentBuf += "\r\n";
        new_session = false;
    }
    return SYNCML_DM_SUCCESS;
}

/*
 *  Send the data through socket and get the response back.
 *
 *  @data the data to be sent
 *  @size the size of the data to be sent
 *  @return success if no error when sending and getting response, else return fail
 */
SYNCML_DM_RET_STATUS_T DmSocketConnector::Send(CPCHAR data, UINT32 size) {
    if (doSend(data, size) == SYNCML_DM_SUCCESS) {
        return getResponse();
    }
    return SYNCML_DM_FAIL;
}

/*
 *  Return the length of the response data.
 *
 *  @return the response data length
 */
UINT32 DmSocketConnector::GetResponseLength() {
    if ( g_nPrintfEnabled ) printf("Get response data length: %d\n", responseLength);
    return responseLength;
}

/*
 *  Get the response data.
 *
 *  @param data the data to be fill with response data
 *  @param size the size of the data
 *  @return success if the data is filled, else return fail
 */
SYNCML_DM_RET_STATUS_T DmSocketConnector::GetResponse(char * data, UINT32 size) {
    char tmpBuf[50000];
    memcpy(data, responseData, responseLength);
    if ( requestMethod == XPL_HTTP_METHOD_POST && size < 50000 ) {
        memcpy(tmpBuf, responseData, responseLength);
        tmpBuf[responseLength]=0;
        if ( g_nPrintfEnabled ) printf("\nResponse Body: %s\n", tmpBuf);
    }
    return SYNCML_DM_SUCCESS;
}

/*
 *  Get the value of a HTTP header feild based on header field name.  
 *
 *  @param field the header field name
 *  @param value the value of the header field
 *  @return success if found the header field, else fail
 *
 */
SYNCML_DM_RET_STATUS_T DmSocketConnector::GetHeaderField(CPCHAR field, char ** value) {
    bool found = false;

    for (int i = responseHeaders.begin(); i < responseHeaders.end(); i++) {
        if (responseHeaders.get_key(i) == field) {
            found = true;
            break;
        }
    }

    if (found == true) {
       const DMString& s = responseHeaders.get(field).c_str();
       *value = (char*)DmtMemAlloc( s.length() + 1 );
       strcpy( *value, s );
        
       if ( g_nPrintfEnabled ) printf("\nGet HeaderField %s : %s\n", field, *value);
    } else {
        if ( g_nPrintfEnabled ) printf("INFO: Can not find %s\n", field);
        return SYNCML_DM_FAIL;
    }
    return SYNCML_DM_SUCCESS;
}

/*
 *  Return HTTP response code such as 200, 404, and etc.
 *
 *  @return the HTTP response code
 */
XPL_HTTP_CODE_T DmSocketConnector::GetResponseCode() {
    if ( g_nPrintfEnabled ) printf("\nGet Response Code: %s \n\n", responseCode.c_str());
    return atoi(responseCode.c_str());
}

/*
 *  Close the socket connection.
 *
 *  @return success if socket closed, else fail.
 */
SYNCML_DM_RET_STATUS_T DmSocketConnector::Close() {
    if ( g_nPrintfEnabled ) printf("Close Socket Connector\n");
    int ret = close(server_s);
    if (ret != 0) {
        if ( g_nPrintfEnabled ) printf("ERROR: Can not close the socket.\n");
	return SYNCML_DM_FAIL;
    }
    return SYNCML_DM_SUCCESS;
}

/*
 *  Close current session, but leave the connection open.
 *  
 *  @return success after closed the session
 */
SYNCML_DM_RET_STATUS_T DmSocketConnector::CloseReq() {
    if ( g_nPrintfEnabled ) printf("Close Socket Session\n");
    new_session = true;
    return SYNCML_DM_SUCCESS;
}

/*
 *  Giving string and delimiter, the function will store the section of the string until the delimiter
 *  in a string item.  This is similar to string tokenizer.
 * 
 *  Ex:  abc:123:xyz      abc,123,and xyz are tokens seperated by a delimiter ':'
 *
 *  @param strItem the first section of the string until the delimiter
 *  @param strReminder the rest of the string without the delimiter
 *  @param cDelim the delimiter
 *
 */
bool DmSocketConnector::DmStringParserGetItem( DMString& strItem, DMString& strReminder, char cDelim ) {
    if ( strReminder[0] == 0 ) {
        return false;
    }
    const char* s = strchr( strReminder, cDelim );
    int nPos = s ? s - strReminder : -1;
    if ( nPos < 0 ) {
        strItem = strReminder;
        strReminder = "";
    } else {
        strItem.assign( strReminder, nPos );
        strReminder = DMString(s+1);
    }
    return true;
}

/*
 *  Parse a HTTP response header.
 *  
 *  @param strBuffer  the received data buffer from server that may contain the entire HTTP header 
 *  @param dataBufSize  number of bytes contained in strBuffer
 *  @param strBufRemaining  the updated buffer containging HTTP body (i.e. strBuffer - HTTP header)
 *  @param lenRemaining number of bytes in strBufRemaining
 *  @return true if HTTP header marker "\r\n\r\n" is found in strBuffer, false otherwise
 *
 */
bool DmSocketConnector::DmParseHTTPHeader( char* strBuffer, int dataBufSize, char** strBufRemaining, int& lenRemaining) {
    // Let's get the response code first
    // If we do not see end of HTTP header, do not bother to parse
    char* entireHeaderEnd = strstr( strBuffer, "\r\n\r\n" );
    if ( !entireHeaderEnd )
        return false;

    // Let's get the response code by looking for first space in response
    char* pFirstSpace = strstr( strBuffer, " ");
    pFirstSpace++;
    char tmpBuf[10];
    strncpy(tmpBuf, pFirstSpace, 3);
    tmpBuf[3]=0;
    responseCode = tmpBuf;
    char* curPos = strBuffer;
    // skip one \r\n
    char *headerEnd = strstr(curPos, "\r\n");
    curPos = headerEnd + strlen("\r\n");
    headerEnd = strstr(curPos, "\r\n");

    // Found an HTTP Header, let's get the name and value pair
    while ( headerEnd != NULL ) {
        char* pColon = strchr(curPos, ':');
        char name[256];
        char value[256];
        strncpy(name, curPos, pColon-curPos);
        name[pColon-curPos]=0;
        strncpy(value, pColon+2, headerEnd-pColon-2);
        value[headerEnd-pColon-2]=0;
        responseHeaders.put(name, value);
        if ( headerEnd == entireHeaderEnd )
            break;
        curPos = headerEnd + strlen("\r\n");
        headerEnd = strstr(curPos, "\r\n");
    }
    *strBufRemaining = entireHeaderEnd + strlen("\r\n\r\n");
    lenRemaining = dataBufSize - (*strBufRemaining - strBuffer);
    return true;
}

/*
 *  Parse a HTTP response header.
 *  
 *  @param newData  pointer to the buffer containing data from server
 *  @param len  the size of the data in the buffer
 *  @return true if data is set successfully, false if memory can not be allocated 
 */
bool DmSocketConnector::SetResponseData(unsigned char* newData, int len) {
    if (len == 0) {
        return true;
    }

    if ( responseData == NULL ) {
        responseData = (unsigned char*)malloc(len);
        memcpy( responseData, newData, len);
    } else {
        unsigned char* newPtr = (unsigned char*)malloc(len + responseLength);
        memcpy((void*)newPtr, (void*)responseData, responseLength);
        memcpy((void*)(newPtr+responseLength), (void*)newData, len);
        free(responseData);
        responseData = newPtr;
    }
    return true;
}

/*
 *  Parse the URL into address:port and URL path.
 *
 *  @param strURI the URI to be parse
 *  @param strAddrPort the string to store the address:port
 *  @param strURIPath the string to store the path of the URI
 *
 *  @return true if URI was in right format, else false
 */
bool DmSocketConnector::parseURL(DMString strURI, DMString& strAddrPort, DMString& strURIPath) {
    int counter = 0;
    DMString tmpStr;

    while(DmStringParserGetItem(tmpStr, strURI, '/')) {
        if (counter == 0) {
            if (strcmp(tmpStr.c_str(), "http:") != 0) {
                return false;
            }
        } else if (counter == 1) {
            if (tmpStr.c_str()[0]!=0/*strcmp(tmpStr.c_str(), "") !=0*/) {
                return false;
            }
        } else if (counter == 2) {
            strAddrPort = tmpStr;
            strURIPath = strURI;
            return true;
        }
        counter++;
    }
    return false;
}

/*
 *  Parse the address:port into address and port.
 *
 *  @param strAddrPort the string that holds address:port
 *  @param strAddr the string that store address
 *  @param strPort the string that store port
 * 
 *  @return true after parsing
 */
bool DmSocketConnector::parseAddrPort(DMString strAddrPort, DMString& strAddr, DMString& strPort) {
    int j = 0;
    DMString tmpStr;
    while(DmStringParserGetItem(tmpStr, strAddrPort, ':')) {
        if (j == 0) {                                      
            strAddr = tmpStr;
        } else if (j == 1) {
            strPort = tmpStr;
        }
        j++;
    }
    return true;
}

/* 
 *  Prepair HTTP sent with sent data and sent it out through socket.
 *
 *  @param data the body of the request
 *  @param size the size of the request body
 *
 *  @return success if it sent all data through socket, else fail
 */
SYNCML_DM_RET_STATUS_T DmSocketConnector::doSend(CPCHAR data, UINT32 size) {
    unsigned int retcode;        // Return code

    if ( g_nPrintfEnabled ) printf("\n[Header: %d bytes]\n%s\n", strlen(sentBuf.c_str()), sentBuf.c_str());
    if ( g_nPrintfEnabled ) printf("[Data: %d bytes]\n%s\n\n", strlen(data), data);

    if (size != 0) {
        sentBuf += "Content-length: ";

        char size_str[10];
        sprintf(size_str, "%d", size);

        sentBuf += size_str;
        sentBuf += "\r\n\r\n";
    } else {
        sentBuf += "Host: ";
        sentBuf += socket_ipAddress;
        sentBuf += "\r\n\r\n";
    }

    server_s = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;                // Address family to use

    // Port num to use
    server_addr.sin_port = htons(atoi(socket_portNum.c_str()));
    // IP address to use
    server_addr.sin_addr.s_addr = inet_addr(socket_ipAddress);

    if ( g_nPrintfEnabled ) printf("Host: %s  Port: %s\n", socket_ipAddress.c_str(), socket_portNum.c_str());

    // Do a connect (connect() blocks)
    retcode = connect(server_s, (struct sockaddr *)&server_addr,
                      sizeof(server_addr));

    if (retcode != 0) {
        if ( g_nPrintfEnabled ) printf("ERROR: connect() failed \n");
        return SYNCML_DM_FAIL;
    }

    //memset(out_buf, 0, BUF_SIZE);
    //strcpy(out_buf, sentBuf.c_str());

    if ( g_nPrintfEnabled ) printf("Send Size: %d\n", size);
    if ( g_nPrintfEnabled ) printf("\nSend\n>>> >>> >>>\n%s%s<<< <<< <<<\n", sentBuf.c_str(), data);

    // Send a request to the server
    int ret = send(server_s, sentBuf.c_str(), strlen(sentBuf.c_str()), 0);
    ret = send(server_s, data, size, 0);
    
    if ( g_nPrintfEnabled ) printf("Send Size: %d\n", ret);

    if (ret != -1) {
      return SYNCML_DM_SUCCESS;
    }
    return SYNCML_DM_FAIL;
}

/*
 *  Get HTTP response by parsing the header information and body.
 *
 *  @return success if the response size is greater than zero, else fail
 */
SYNCML_DM_RET_STATUS_T DmSocketConnector::getResponse() {
    if ( g_nPrintfEnabled ) printf("\nGet Response\n");
    char in_buf[BUF_SIZE];   // Input buffer for response
    bool handleHeader = true;
    int retcode;
    int nBufUsed = 0;
    responseBody = "";
    responseLength = 0;

    if ( responseData != NULL ) {
        free(responseData);
        responseData = NULL;
    }

    retcode = recv(server_s, in_buf, BUF_SIZE, 0);
    if ( g_nPrintfEnabled ) printf("Size: %d\n", retcode);
    
    while ((retcode > 0) && (retcode != -1)) {
        int lenRemaining;
        bool bEndHeader;
        char* strRemaining;

        if ( handleHeader )  {
            bEndHeader = DmParseHTTPHeader( in_buf, retcode, &strRemaining, lenRemaining);
            if ( bEndHeader ) {
                handleHeader = false;
                SetResponseData((unsigned char*)strRemaining,lenRemaining);
                responseLength = lenRemaining;
                nBufUsed = 0;
            }
            else
              nBufUsed += retcode;
        } else {
            SetResponseData((unsigned char*)in_buf,retcode);
            responseLength += retcode;
        }
        retcode = recv(server_s, in_buf + nBufUsed, BUF_SIZE-nBufUsed, 0);
    }

    if ( responseLength > 0 ) {
        return SYNCML_DM_SUCCESS;
    } else {
        return SYNCML_DM_FAIL;
    }
}
