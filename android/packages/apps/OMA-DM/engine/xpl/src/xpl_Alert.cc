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

#include "xpl_dm_ServerAlert.h"
#include <DMServiceAlert.h>

SYNCML_DM_RET_STATUS_T XPL_DM_ShowDisplayAlert(INT32 minDisplayTime, CPCHAR msg)
{
   DMServiceAlert alert;
   return alert.showDisplayAlert(minDisplayTime, msg);
}

SYNCML_DM_RET_STATUS_T XPL_DM_ShowConfirmAlert(INT32 maxDisplayTime, CPCHAR msg, XPL_DM_ALERT_RES_T defaultResponse, XPL_DM_ALERT_RES_T * responseCode)
{
   DMServiceAlert alert;
   return alert.showConfirmAlert(maxDisplayTime, msg, defaultResponse, responseCode);
}

SYNCML_DM_RET_STATUS_T XPL_DM_ShowTextInputAlert(INT32 maxDisplayTime, 
                                            CPCHAR msg, 
                                            CPCHAR defaultResponse,
                                            INT32 maxLength, 
                                            XPL_DM_ALERT_INPUT_T inputType, 
                                            XPL_DM_ALERT_ECHO_T echoType,
                                            XPL_DM_ALERT_TEXTINPUT_RES_T * userResponse )
{
   DMServiceAlert alert;
   return alert.showTextInputAlert(maxDisplayTime,
                               msg,
                               defaultResponse,
                               maxLength,
                               inputType,
                               echoType,
                               userResponse );
}


SYNCML_DM_RET_STATUS_T  XPL_DM_ShowSingleChoiceAlert(INT32 maxDisplayTime, 
                                                CPCHAR msg,
                                                DMStringVector & choices,
                                                INT32 defaultResponse,
                                                XPL_DM_ALERT_SCHOICE_RES_T * userResponse ) 
{
   DMServiceAlert alert;
   return  alert.showSingleChoiceAlert(maxDisplayTime,
                                   msg,
                                   choices,
                                   defaultResponse,
                                   userResponse);
}


SYNCML_DM_RET_STATUS_T XPL_DM_ShowMultipleChoiceAlert(INT32 maxDisplayTime,
                                                       CPCHAR msg, 
                                                       DMStringVector & choices, 
                                    		       DMStringVector & defaultResponses,
                                    		       XPL_DM_ALERT_MCHOICE_RES_T * userResponse) 
{
   DMServiceAlert alert;
   return alert.showMultipleChoiceAlert(maxDisplayTime,
                                    msg,
                                    choices,
                                    defaultResponses,
                                    userResponse);
}
