
#include "xpl_HTTP.h"
#include "dmSocketConnector.h"


/* Activetes HTTP transport and set specified URL to connect with server.
* Do not establish actual connection with server */
XPL_HTTP_HANDLE_T XPL_HTTP_Open(CPCHAR url, 
                                CPCHAR ConRef,
                                XPL_ADDR_TYPE_T addrType,
                                XPL_HTTP_RET_STATUS_T * result)
{
  DmSocketConnector * p = DmBrwCreateConnector();

  if ( !p ) {
    if ( result )
      *result = XPL_HTTP_RET_FAIL;
    return 0;
  }

  if ( p->Open(url, ConRef, addrType) != SYNCML_DM_SUCCESS ){
    delete p;
    
    if ( result )
      *result = XPL_HTTP_RET_FAIL;
    
    return 0;
  }
  
  if ( result )
    *result = XPL_HTTP_RET_SUCCESS;
    
  return (XPL_HTTP_HANDLE_T)p;
}

/* Sets method for URL request. */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_SetRequestMethod(XPL_HTTP_HANDLE_T handler, 
                          XPL_HTTP_METHOD_T method)
{
  DmSocketConnector * p = (DmSocketConnector*) handler;

  if ( !p )
    return XPL_HTTP_RET_BADARGUMENT;

  if ( p->SetRequestMethod(method) == SYNCML_DM_SUCCESS )
    return XPL_HTTP_RET_SUCCESS;

  return XPL_HTTP_RET_FAIL;
}


/* Sets a general request property. 
* This method is called before Send method */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_SetRequestProperty(XPL_HTTP_HANDLE_T handler,
                            CPCHAR buffer)
{
  DmSocketConnector * p = (DmSocketConnector*) handler;

  if ( !p )
    return XPL_HTTP_RET_BADARGUMENT;

  if ( p->SetRequestProperty(buffer) == SYNCML_DM_SUCCESS )
    return XPL_HTTP_RET_SUCCESS;

  return XPL_HTTP_RET_FAIL;
}

/* Sends data to the resource pointed to by the URL. 
* Size specifies size of content to be sent. */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_Send(XPL_HTTP_HANDLE_T handler, 
              CPCHAR buffer, 
              UINT32 size)
{
  DmSocketConnector * p = (DmSocketConnector*) handler;

  if ( !p )
    return XPL_HTTP_RET_BADARGUMENT;

  if ( p->Send(buffer, size) == SYNCML_DM_SUCCESS )
    return XPL_HTTP_RET_SUCCESS;

  return XPL_HTTP_RET_FAIL;
}

/* Returns the response length */
UINT32 XPL_HTTP_GetResponseLength(XPL_HTTP_HANDLE_T handler)
{
  DmSocketConnector * p = (DmSocketConnector*) handler;

  if ( !p )
    return 0;

  return p->GetResponseLength();
}

/* Downloads the contents from the resource. The downloaded content is 
* stored in a buffer. */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_GetResponse(XPL_HTTP_HANDLE_T handler, 
                     char * buffer, 
                     UINT32 size)
{
  DmSocketConnector * p = (DmSocketConnector*) handler;

  if ( !p )
    return XPL_HTTP_RET_BADARGUMENT;

  if ( p->GetResponse(buffer, size)== SYNCML_DM_SUCCESS )
    return XPL_HTTP_RET_SUCCESS;

  return XPL_HTTP_RET_FAIL;
}

/* Method returns the value of the header for given header field. */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_GetHeaderField(XPL_HTTP_HANDLE_T handler, 
                        CPCHAR field, 
                        char ** value)
{
  DmSocketConnector * p = (DmSocketConnector*) handler;

  if ( !p )
    return XPL_HTTP_RET_BADARGUMENT;

  if ( p->GetHeaderField(field, value)== SYNCML_DM_SUCCESS )
    return XPL_HTTP_RET_SUCCESS;

  return XPL_HTTP_RET_FAIL;
}

/* Method returns the complete response header as sent by server. */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_GetRespHeader(XPL_HTTP_HANDLE_T handler, 
                       char ** header)
{
  return XPL_HTTP_RET_FAIL;
}
                       
/* Method returns value of content-type in the response header. */
XPL_HTTP_RET_STATUS_T 
XPL_HTTP_GetType(XPL_HTTP_HANDLE_T handler, 
                 char ** content_type)
{
  return XPL_HTTP_RET_FAIL;
}

/* Gets status code from HTTP/WAP response message.  Returns -1, if no code be
* discerned from the response (i.e., the response is not valid HTTP/WAP) */
XPL_HTTP_CODE_T XPL_HTTP_GetResponseCode(XPL_HTTP_HANDLE_T handler)
{
  DmSocketConnector * p = (DmSocketConnector*) handler;

  if ( !p )
    return 0;

  return p->GetResponseCode();
}

/* Closes connection with server. */
XPL_HTTP_RET_STATUS_T XPL_HTTP_Close(XPL_HTTP_HANDLE_T handler)
{
  DmSocketConnector * p = (DmSocketConnector*) handler;

  if ( !p )
    return XPL_HTTP_RET_BADARGUMENT;

  p->Close();
  delete p;
    
  return XPL_HTTP_RET_SUCCESS;
}

/* Closes request transaction without closing connection with server. */
XPL_HTTP_RET_STATUS_T XPL_HTTP_CloseReq(XPL_HTTP_HANDLE_T handler)
{
  DmSocketConnector * p = (DmSocketConnector*) handler;

  if ( !p )
    return XPL_HTTP_RET_BADARGUMENT;

  p->CloseReq();
    
  return XPL_HTTP_RET_SUCCESS;
}

/* Sets URL for new request */ 
XPL_HTTP_RET_STATUS_T XPL_HTTP_SetUrl(XPL_HTTP_HANDLE_T handler ,
                                      CPCHAR url,
                                      CPCHAR ConRef,
                                      XPL_ADDR_TYPE_T addrType)
{
  DmSocketConnector * p = (DmSocketConnector*) handler;

  if ( !p )
    return XPL_HTTP_RET_BADARGUMENT;

  if ( p->SetUrl(url, ConRef, addrType)== SYNCML_DM_SUCCESS )
    return XPL_HTTP_RET_SUCCESS;

  return XPL_HTTP_RET_FAIL;
}


/* Dowloads content from the resource. The read content is stored in the file */ 
XPL_HTTP_RET_STATUS_T XPL_HTTP_DowloadByFile(XPL_HTTP_HANDLE_T handler, 
                                             CPCHAR file_name)
{
  return XPL_HTTP_RET_FAIL;
}

/* Dowloads content from the resource. The downloaded content is stored in a buffer */ 
XPL_HTTP_RET_STATUS_T XPL_HTTP_DownloadByBuffer(XPL_HTTP_HANDLE_T handler, 
                                                UINT8 * buffer, 
                                                UINT32 size)
{
  return XPL_HTTP_RET_FAIL;
}



