#include <stdio.h>
#include "xpl_HTTP.h"
#include "dmConnection.h"
#include "dmStringUtil.h"

#ifdef __cplusplus
extern "C" {
#endif


static DmBrwConnector *stDmBrwConnector = NULL;

XPL_HTTP_HANDLE_T XPL_HTTP_Open(CPCHAR url, 
                                CPCHAR ConRef,
                                XPL_ADDR_TYPE_T addrType,
                                XPL_HTTP_RET_STATUS_T * result)    
{
    if(stDmBrwConnector == NULL)
    {
        stDmBrwConnector = DmBrwCreateConnector();
    
        if ( stDmBrwConnector == NULL )
        {
            if ( result )
                *result = XPL_HTTP_RET_FAIL;
            return XPL_HTTP_HANDLE_INVALID;
        }
    } 

    
    XPL_HTTP_RET_STATUS_T res;
    res = stDmBrwConnector->Open(url, ConRef, (UINT32)addrType);
    if ( res == XPL_HTTP_RET_SUCCESS )
    {
        if ( result )
           *result = res;
        return (XPL_HTTP_HANDLE_T)stDmBrwConnector;
    }    
    else
    {
        if ( result )
            *result = XPL_HTTP_RET_FAIL;
        return XPL_HTTP_HANDLE_INVALID;
    }
}


XPL_HTTP_RET_STATUS_T XPL_HTTP_SetUrl(XPL_HTTP_HANDLE_T handler,
                                      CPCHAR url,  
                                      CPCHAR ConRef,
                                      XPL_ADDR_TYPE_T addrType)
{
    DmBrwConnector * brwConnector = (DmBrwConnector*)handler;
    if ( brwConnector != stDmBrwConnector ) 
        return XPL_HTTP_RET_BADARGUMENT;

    return brwConnector->SetUrl(url, ConRef, (UINT32)addrType);
}

XPL_HTTP_RET_STATUS_T XPL_HTTP_SetRequestMethod(XPL_HTTP_HANDLE_T handler, XPL_HTTP_METHOD_T method)
{
    DmBrwConnector* brwConnector = (DmBrwConnector*)handler;
    if ( brwConnector != stDmBrwConnector ) 
        return XPL_HTTP_RET_BADARGUMENT;

    return brwConnector->SetRequestMethod(method);
}        


XPL_HTTP_RET_STATUS_T  
XPL_HTTP_SetRequestProperty(XPL_HTTP_HANDLE_T handler, CPCHAR buffer)
{
     DmBrwConnector* brwConnector = (DmBrwConnector*)handler;
    if ( brwConnector != stDmBrwConnector ) 
        return XPL_HTTP_RET_BADARGUMENT;

    return brwConnector->SetRequestProperty((CPCHAR)"header_start", buffer);
}

XPL_HTTP_RET_STATUS_T XPL_HTTP_Send(XPL_HTTP_HANDLE_T handler, CPCHAR data, UINT32 size)
{
    DmBrwConnector* brwConnector = (DmBrwConnector*)handler;
    if ( brwConnector != stDmBrwConnector ) 
        return XPL_HTTP_RET_BADARGUMENT;

    return brwConnector->Send(data,size);
}

UINT32 XPL_HTTP_GetResponseLength(XPL_HTTP_HANDLE_T handler)
{
    DmBrwConnector* brwConnector = (DmBrwConnector*)handler;
    if ( brwConnector != stDmBrwConnector ) 
        return XPL_HTTP_RET_BADARGUMENT;

    return brwConnector->GetResponseLength();
}

XPL_HTTP_RET_STATUS_T XPL_HTTP_GetResponse(XPL_HTTP_HANDLE_T handler, char * data, UINT32 size)
{
    DmBrwConnector* brwConnector = (DmBrwConnector*)handler;
    if ( brwConnector != stDmBrwConnector ) 
        return XPL_HTTP_RET_BADARGUMENT;

    return brwConnector->GetResponse(data,size);
}


XPL_HTTP_RET_STATUS_T XPL_HTTP_GetHeaderField(XPL_HTTP_HANDLE_T handler, CPCHAR field, char **value)
{
    DmBrwConnector* brwConnector = (DmBrwConnector*)handler;
    if ( brwConnector != stDmBrwConnector ) 
        return XPL_HTTP_RET_BADARGUMENT;

    DMString strValue;
    SYNCML_DM_RET_STATUS_T ret=brwConnector->GetHeaderField(field,strValue);
    DmStrcpy(*value, strValue.c_str()); 
    return (XPL_HTTP_RET_STATUS_T)ret;
}

XPL_HTTP_CODE_T XPL_HTTP_GetResponseCode(XPL_HTTP_HANDLE_T handler)
{
    DmBrwConnector* brwConnector = (DmBrwConnector*)handler;
    if ( brwConnector != stDmBrwConnector ) 
        return XPL_HTTP_RET_BADARGUMENT;

    return brwConnector->GetResponseCode();
}

XPL_HTTP_RET_STATUS_T XPL_HTTP_Close(XPL_HTTP_HANDLE_T handler)
{
    DmBrwConnector* brwConnector = (DmBrwConnector*)handler;
    if ( brwConnector != stDmBrwConnector ) 
        return XPL_HTTP_RET_BADARGUMENT;

    brwConnector->Close();

    DmBrwDestroyConnector(brwConnector);    
    stDmBrwConnector = NULL;
    return XPL_HTTP_RET_SUCCESS;
}

XPL_HTTP_RET_STATUS_T XPL_HTTP_CloseReq(XPL_HTTP_HANDLE_T handler)
{
    DmBrwConnector* brwConnector = (DmBrwConnector*)handler;
    if ( brwConnector != stDmBrwConnector ) 
        return XPL_HTTP_RET_BADARGUMENT;

    return brwConnector->CloseReq();
}


XPL_HTTP_RET_STATUS_T XPL_HTTP_DowloadByFile(XPL_HTTP_HANDLE_T handler, CPCHAR file_name)
{
    return XPL_HTTP_RET_NOT_SUPPORTED;

}


XPL_HTTP_RET_STATUS_T XPL_HTTP_DownloadByBuffer(XPL_HTTP_HANDLE_T handler, UINT8 * buffer, UINT32 size)
{
    return XPL_HTTP_RET_NOT_SUPPORTED;

}

XPL_HTTP_RET_STATUS_T 
XPL_HTTP_GetRespHeader(XPL_HTTP_HANDLE_T handler, char ** header)
{
    return XPL_HTTP_RET_NOT_SUPPORTED;
}

XPL_HTTP_RET_STATUS_T 
XPL_HTTP_GetType(XPL_HTTP_HANDLE_T handler, char ** content_type)
{
    return XPL_HTTP_RET_NOT_SUPPORTED;

}

#ifdef __cplusplus
}
#endif

