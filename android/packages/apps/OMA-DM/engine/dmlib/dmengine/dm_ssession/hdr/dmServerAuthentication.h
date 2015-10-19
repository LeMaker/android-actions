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

#ifndef DMSERVERAUTHENTICATION_H
#define DMSERVERAUTHENTICATION_H

/*==================================================================================================

    Header Name: dmServerAuthentication.h

    General Description: Declaration of DMServerAuthentication class.

==================================================================================================*/

#include "syncml_dm_data_types.h"
#include "dmbuffer.h"

/*==================================================================================================
                                 CLASSES DECLARATION
==================================================================================================*/
class DMServerAuthentication
{
public:
    SYNCML_DM_RET_STATUS_T AuthenticateServer(SYNCML_DM_AuthContext_T& AuthContext);

private:
    void CheckCredentials(SYNCML_DM_AuthContext_T& AuthContext,const DMString& password, const DMBuffer& data, BOOLEAN bDecodeNonce);

    DMString GetPreferredProfilePath(const DMString& strAccName, const DMMap<DMString,UINT32>& dmAuthProfiles);

    SYNCML_DM_RET_STATUS_T TryProfile_1_1(const DMString& strAccName, const DMString& strProfilePath, SYNCML_DM_AuthContext_T& AuthContext);

    SYNCML_DM_RET_STATUS_T TryProfile_1_2(const DMString& strAccName, const DMString& strProfilePath, SYNCML_DM_AuthContext_T& AuthContext);
};
#endif
