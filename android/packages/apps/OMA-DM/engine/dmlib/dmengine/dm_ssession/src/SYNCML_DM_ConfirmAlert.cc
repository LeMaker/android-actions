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

    File Name: SYNCML_DM_ConfirmAlert.cc

    General Description: A class representing the alerts.

==================================================================================================*/

#include "dmStringUtil.h"
#include "dmtoken.h"
#include "SYNCML_DM_ConfirmAlert.H"

SYNCML_DM_ConfirmAlert::SYNCML_DM_ConfirmAlert()  
{
  defaultResponse = XPL_DM_ALERT_RES_NONE;
  responseCode = XPL_DM_ALERT_RES_TIMEOUT;
}

void SYNCML_DM_ConfirmAlert::processParameter(CPCHAR name, CPCHAR value)
{
    if ( DmStrcmp(name,SYNCML_DM_ALERT_OPTION_MAXDT) == 0 )
       setMaxDisplayTime(DmAtoi(value));
    else 
         if ( DmStrcmp(name,SYNCML_DM_ALERT_OPTION_DEFAULT_RESPONSE) == 0 )
         {
             defaultResponse = (XPL_DM_ALERT_RES_T)DmAtoi(value);
             if (defaultResponse == 0)
                  defaultResponse = XPL_DM_ALERT_RES_NO;
             else
              if (defaultResponse == 1)
                 defaultResponse = XPL_DM_ALERT_RES_YES;
             XPL_LOG_DM_SESS_Debug (("SYNCML_DM_ConfirmAlert::processParamete : defaultResponse = %d \n", defaultResponse));
         }
}

SYNCML_DM_RET_STATUS_T SYNCML_DM_ConfirmAlert::show() 
{
  
  return XPL_DM_ShowConfirmAlert(maxDisplayTime, msg, defaultResponse, &responseCode);
}

XPL_DM_ALERT_RES_T SYNCML_DM_ConfirmAlert::getAction() const
{
    return responseCode;
}

SYNCML_DM_RET_STATUS_T 
SYNCML_DM_ConfirmAlert::getDefaultResponse(DMStringVector & userResponse) const
{
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T 
SYNCML_DM_ConfirmAlert::getResponse(DMStringVector & userResponse) const
{
    return SYNCML_DM_SUCCESS;
}
