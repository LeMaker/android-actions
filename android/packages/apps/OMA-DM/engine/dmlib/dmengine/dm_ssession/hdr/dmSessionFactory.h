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

#ifndef DMSESSIONFACTORY_H
#define DMSESSIONFACTORY_H

/*==================================================================================================

    Header Name: dmSessionFactory.h 

    General Description: Declaration of Internal interfaces to Server Session.

==================================================================================================*/

#include "dmtSessionProp.hpp"
#include "dmbuffer.h"
#include "syncml_dm_data_types.h"

#ifdef __cplusplus
extern "C" {
#endif


SYNCML_DM_RET_STATUS_T  DmProcessServerDataInternal(CPCHAR szPrincipal, const DmtSessionProp&  session);

SYNCML_DM_RET_STATUS_T  DmProcessScriptDataInternal(const UINT8 * docInputBuffer,
                                                    UINT32 inDocSize, 
                                                    BOOLEAN isWBXML,
                                                    DMBuffer & oResult);

SYNCML_DM_RET_STATUS_T  DmBootstrapInternal(const UINT8 * docInputBuffer,
                                                UINT32 inDocSize, 
                                                BOOLEAN isWBXML,
                                                BOOLEAN isProcess,
                                                DMString & serverID);


SYNCML_DM_RET_STATUS_T DmAuthenticateServerInternal(SYNCML_DM_AuthContext_T& AuthContext );


#ifndef DM_NO_SESSION_DLL
SYNCML_DM_RET_STATUS_T  DmProcessServerData(CPCHAR szPrincipal, const DmtSessionProp&  session);

SYNCML_DM_RET_STATUS_T  DmProcessScriptData(const UINT8 * docInputBuffer,
                                             UINT32 inDocSize, 
                                             BOOLEAN isWBXML,
                                             DMBuffer & oResult);

SYNCML_DM_RET_STATUS_T  DmBootstrap(const UINT8 * docInputBuffer,
                                        UINT32 inDocSize, 
                                        BOOLEAN isWBXML,
                                        BOOLEAN isProcess,
                                        DMString & serverID);


SYNCML_DM_RET_STATUS_T DmAuthenticateServer(SYNCML_DM_AuthContext_T& AuthContext );
#else
#define  DmProcessServerData(szPrincipal, isWBXML) DmProcessServerDataInternal(szPrincipal, isWBXML)

#define  DmProcessScriptData(docInputBuffer,inDocSize,isWBXML, oResult) \
             DmProcessScriptDataInternal(docInputBuffer,inDocSize,isWBXML, oResult) 

#define  DmBootstrap(docInputBuffer,inDocSize,isWBXML, isProcess, serverID) \
             DmBootstrapInternal(docInputBuffer,inDocSize,isWBXML, isProcess, serverID) 

#define DmAuthenticateServer(AuthContext) \
             DmAuthenticateServerInternal(AuthContext)
#endif


#ifdef __cplusplus
}
#endif

#endif /* DMSESSION_H */
