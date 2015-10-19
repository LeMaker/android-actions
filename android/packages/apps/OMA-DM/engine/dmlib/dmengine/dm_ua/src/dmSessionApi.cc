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

#ifndef DM_NO_SESSION_DLL
/*==================================================================================================

    Source Name: dmSessionAPI.cc

    General Description: Implementation of External interfaces to Server Session.

==================================================================================================*/

#include "dmSessionFactory.h"
#include "xpl_dm_Manager.h"
#include "xpl_Lib.h"
#include "xpl_Logger.h"

#ifdef __cplusplus
extern "C" {
#endif

SYNCML_DM_RET_STATUS_T  
DmProcessServerData(CPCHAR szPrincipal, 
                                 const DmtSessionProp&  session)
{
    SYNCML_DM_RET_STATUS_T ret_status;

#ifndef DM_NO_SESSION_LIB
    const char * lib_name = XPL_DM_GetEnv(SYNCML_DM_SESSION_LIB);
    /* Loads dynamic library */
    XPL_DL_HANDLE_T lib_handle = XPL_DL_Load(lib_name);

    if ( lib_handle == NULL )
        return SYNCML_DM_FAIL;

    XPL_DL_HANDLE_T pFunc = XPL_DL_GetFunction(lib_handle, "DmProcessServerDataInternal");

    if ( pFunc == NULL )
    {
       XPL_DL_Unload(lib_handle);
       return SYNCML_DM_FAIL;
    }

    ret_status = ((SYNCML_DM_RET_STATUS_T (*)(CPCHAR ,const DmtSessionProp&))(pFunc))(szPrincipal,session);
    
    XPL_DL_Unload(lib_handle);
#else
    ret_status = DmProcessServerDataInternal(szPrincipal,session);
    XPL_LOG_DM_SESS_Debug(("Returning from DmProcessServerDataInternal status=%d\n", ret_status));
#endif

    return ret_status;

}

SYNCML_DM_RET_STATUS_T  
DmProcessScriptData(const UINT8 * docInputBuffer,
                                 UINT32 inDocSize, 
                                 BOOLEAN isWBXML,
                                 DMBuffer & oResult)
{
    SYNCML_DM_RET_STATUS_T ret_status;

#ifndef DM_NO_SESSION_LIB
    const char * lib_name = XPL_DM_GetEnv(SYNCML_DM_SESSION_LIB);
    /* Loads dynamic library */
    XPL_DL_HANDLE_T lib_handle = XPL_DL_Load(lib_name);

    if ( lib_handle == NULL )
        return SYNCML_DM_FAIL;

    XPL_DL_HANDLE_T pFunc = XPL_DL_GetFunction(lib_handle, "DmProcessScriptDataInternal");

    if ( pFunc == NULL )
    {
       XPL_DL_Unload(lib_handle);
       return SYNCML_DM_FAIL;
    }

    ret_status = ((SYNCML_DM_RET_STATUS_T (*)(const UINT8 * , UINT32, BOOLEAN, DMBuffer &))(pFunc))
                    (docInputBuffer,inDocSize,isWBXML,oResult);
    
    XPL_DL_Unload(lib_handle);
#else
    ret_status = DmProcessScriptDataInternal(docInputBuffer,inDocSize,isWBXML,oResult);
#endif

    return ret_status;


}

SYNCML_DM_RET_STATUS_T 
DmBootstrap(const UINT8 * docInputBuffer,
                    UINT32 inDocSize, 
                    BOOLEAN isWBXML,
                    BOOLEAN isProcess,
                    DMString & serverID)
{

    SYNCML_DM_RET_STATUS_T ret_status;

#ifndef DM_NO_SESSION_LIB
    const char * lib_name = XPL_DM_GetEnv(SYNCML_DM_SESSION_LIB);

    XPL_DL_HANDLE_T lib_handle = XPL_DL_Load(lib_name);

    if ( lib_handle == NULL )
        return SYNCML_DM_FAIL;

    XPL_DL_HANDLE_T pFunc = XPL_DL_GetFunction(lib_handle, "DmBootstrapInternal");

    if ( pFunc == NULL )
    {
       XPL_DL_Unload(lib_handle);
       return SYNCML_DM_FAIL;
    }

    ret_status = ((SYNCML_DM_RET_STATUS_T (*)(const UINT8 * , UINT32, BOOLEAN, BOOLEAN, DMString &))(pFunc))
                    (docInputBuffer,inDocSize,isWBXML,isProcess, serverID);
    
    XPL_DL_Unload(lib_handle);
#else
    ret_status = DmBootstrapInternal(docInputBuffer,inDocSize,isWBXML,isProcess, serverID);
#endif
    return ret_status;

}

SYNCML_DM_RET_STATUS_T 
DmAuthenticateServer(SYNCML_DM_AuthContext_T& AuthContext )
{
    SYNCML_DM_RET_STATUS_T ret_status;

#ifndef DM_NO_SESSION_LIB
    const char * lib_name = XPL_DM_GetEnv(SYNCML_DM_SESSION_LIB);

    XPL_DL_HANDLE_T lib_handle = XPL_DL_Load(lib_name); 
    if ( lib_handle == NULL )
        return SYNCML_DM_FAIL; 
    
    XPL_DL_HANDLE_T pFunc = XPL_DL_GetFunction(lib_handle, "DmAuthenticateServerInternal"); 
    if ( pFunc == NULL )
    {
        XPL_DL_Unload(lib_handle);
        return SYNCML_DM_FAIL;
    } 
    ret_status = ((SYNCML_DM_RET_STATUS_T (*)(SYNCML_DM_AuthContext_T& ))(pFunc))(AuthContext);
    
    XPL_DL_Unload(lib_handle);
#else
    ret_status = DmAuthenticateServerInternal(AuthContext);
#endif

    return ret_status; 
}

#ifdef __cplusplus
}
#endif

#endif
