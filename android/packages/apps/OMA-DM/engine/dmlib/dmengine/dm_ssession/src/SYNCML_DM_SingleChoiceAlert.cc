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

    File Name: SYNCML_DM_SingleChoiceAlert.cc

    General Description: A class representing the alerts.

==================================================================================================*/

#include "dmStringUtil.h"
#include "dmtoken.h"
#include "SYNCML_DM_SingleChoiceAlert.H"

SYNCML_DM_SingleChoiceAlert::SYNCML_DM_SingleChoiceAlert() 
{
  defaultResponse = -1;
  response.action = XPL_DM_ALERT_RES_TIMEOUT;
}


void SYNCML_DM_SingleChoiceAlert::processParameter(CPCHAR name, CPCHAR value)
{

    if ( DmStrcmp(name,SYNCML_DM_ALERT_OPTION_MAXDT) == 0 )
       setMaxDisplayTime(DmAtoi(value));
    else 
         if ( DmStrcmp(name,SYNCML_DM_ALERT_OPTION_DEFAULT_RESPONSE) == 0 ) 
             defaultResponse = DmAtoi(value);
}


SYNCML_DM_RET_STATUS_T SYNCML_DM_SingleChoiceAlert::show()
{
  return XPL_DM_ShowSingleChoiceAlert(maxDisplayTime, msg, choices, defaultResponse, &response);
}

XPL_DM_ALERT_RES_T SYNCML_DM_SingleChoiceAlert::getAction() const
{
    return response.action;
}

SYNCML_DM_RET_STATUS_T 
SYNCML_DM_SingleChoiceAlert::getDefaultResponse(DMStringVector & userResponse) const
{
     if (defaultResponse != -1) 
     {
           char selection[UINT32_TYPE_STR_SIZE_10];
            DmSprintf((char *)selection, "%d", defaultResponse );
            userResponse.push_back(selection);
     }
     return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T 
SYNCML_DM_SingleChoiceAlert::getResponse(DMStringVector & userResponse) const
{
    char selection[UINT32_TYPE_STR_SIZE_10];
    DmSprintf((char *)selection, "%d", response.response );
    userResponse.push_back(selection);    
    return SYNCML_DM_SUCCESS;
}
