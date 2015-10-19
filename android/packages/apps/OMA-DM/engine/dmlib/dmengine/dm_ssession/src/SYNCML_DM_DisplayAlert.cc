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

/*==================================================================================================

    File Name: SYNCML_DM_DisplayAlert.cc

    General Description: A class representing the alerts.

==================================================================================================*/

#include "dmStringUtil.h"
#include "dm_uri_utils.h"
#include "dmtoken.h"
#include "SYNCML_DM_DisplayAlert.H"

void SYNCML_DM_DisplayAlert::processParameter(CPCHAR name, CPCHAR value)
{
    if (DmStrcmp(name,SYNCML_DM_ALERT_OPTION_MINDT) == 0)
        setMinDisplayTime(DmAtoi(value));
}

SYNCML_DM_RET_STATUS_T SYNCML_DM_DisplayAlert::show()
{
    return XPL_DM_ShowDisplayAlert(minDisplayTime,msg);  
}


XPL_DM_ALERT_RES_T SYNCML_DM_DisplayAlert::getAction() const 
{
    return XPL_DM_ALERT_RES_YES;
}

SYNCML_DM_RET_STATUS_T 
SYNCML_DM_DisplayAlert::getDefaultResponse(DMStringVector & userResponse) const
{
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T 
SYNCML_DM_DisplayAlert::getResponse(DMStringVector & userResponse) const
{
    return SYNCML_DM_SUCCESS;
}

