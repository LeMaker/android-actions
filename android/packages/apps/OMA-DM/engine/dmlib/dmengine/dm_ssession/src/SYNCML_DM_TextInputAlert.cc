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

    File Name: SYNCML_DM_TextInputAlert.cc

    General Description: A class representing the alerts.

==================================================================================================*/

#include "SYNCML_DM_TextInputAlert.H"
#include "dmStringUtil.h"
#include "dmtoken.h"


SYNCML_DM_TextInputAlert::SYNCML_DM_TextInputAlert()  
{
  maxLength = 20;
  inputType = XPL_DM_ALERT_I_ALPHA;
  echoType = XPL_DM_ALERT_E_TEXT;
  response.action = XPL_DM_ALERT_RES_TIMEOUT;
}


void SYNCML_DM_TextInputAlert::processParameter(CPCHAR name, CPCHAR value)
{
    if ( DmStrcmp(name,SYNCML_DM_ALERT_OPTION_MAXDT) == 0 )
       setMaxDisplayTime(DmAtoi(value));
    else 
        if ( DmStrcmp(name,SYNCML_DM_ALERT_OPTION_DEFAULT_RESPONSE) == 0 ) 
             defaultResponse = value;
        else 
            if ( DmStrcmp(name,SYNCML_DM_ALERT_OPTION_MAX_LENGTH) == 0 )
                 maxLength = DmAtoi(value);
            else 
                if ( DmStrcmp(name,SYNCML_DM_ALERT_OPTION_INPUT_TYPE) == 0 )
                     inputType = convertInputType(value);
                else 
                     if ( DmStrcmp(name,SYNCML_DM_ALERT_OPTION_ECHO_TYPE) == 0 )
                            echoType = convertEchoType(value);
}


SYNCML_DM_RET_STATUS_T SYNCML_DM_TextInputAlert::show()
{

  SYNCML_DM_RET_STATUS_T ret_status;

  defaultResponse.replaceAll('+',' ');
  ret_status = XPL_DM_ShowTextInputAlert(maxDisplayTime, msg,defaultResponse,
                                    maxLength, inputType, echoType, &response ); 

  defaultResponse.replaceAll(' ','+');
  if ( ret_status == SYNCML_DM_SUCCESS )
  {
     response.response.replaceAll(' ','+');
  }   

  return ret_status;
}

XPL_DM_ALERT_RES_T SYNCML_DM_TextInputAlert::getAction() const
{
    return response.action;
}


SYNCML_DM_RET_STATUS_T 
SYNCML_DM_TextInputAlert::getDefaultResponse(DMStringVector & userResponse) const
{
    userResponse.push_back(defaultResponse);
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T 
SYNCML_DM_TextInputAlert::getResponse(DMStringVector & userResponse) const
{
    userResponse.push_back(response.response);
    return SYNCML_DM_SUCCESS;
}


XPL_DM_ALERT_INPUT_T  SYNCML_DM_TextInputAlert::convertInputType(CPCHAR inputType) {

  if (DmStrcmp(inputType,SYNCML_DM_ALERT_OPTION_IT_A) == 0) 
  {
      return XPL_DM_ALERT_I_ALPHA;
  }
  else
     if (DmStrcmp(inputType,SYNCML_DM_ALERT_OPTION_IT_N) == 0) 
     {
         return XPL_DM_ALERT_I_NUMERIC;
     } 
     else 
         if (DmStrcmp(inputType,SYNCML_DM_ALERT_OPTION_IT_D) == 0)
         {
            return XPL_DM_ALERT_I_DATE;
         } 
         else 
             if (DmStrcmp(inputType,SYNCML_DM_ALERT_OPTION_IT_T) == 0) 
             {
                return XPL_DM_ALERT_I_TIME;
             } 
             else 
                 if (DmStrcmp(inputType,SYNCML_DM_ALERT_OPTION_IT_P) == 0) 
                 {
                    return XPL_DM_ALERT_I_PHONE_NUM;
                 }
                 else
                     if (DmStrcmp(inputType,SYNCML_DM_ALERT_OPTION_IT_I) == 0)
                     {
                        return XPL_DM_ALERT_I_IP_ADDR;
                     }
   return XPL_DM_ALERT_I_ALPHA;
}

XPL_DM_ALERT_ECHO_T  SYNCML_DM_TextInputAlert::convertEchoType(CPCHAR echoType) {
    if (DmStrcmp(echoType,SYNCML_DM_ALERT_OPTION_ET_T) == 0)
    {
        return XPL_DM_ALERT_E_TEXT;
    } 
    else {
        if (DmStrcmp(echoType,SYNCML_DM_ALERT_OPTION_ET_P) == 0)
        {
            return XPL_DM_ALERT_E_PASSWD;
        } 
    }
    return XPL_DM_ALERT_E_TEXT;
}
