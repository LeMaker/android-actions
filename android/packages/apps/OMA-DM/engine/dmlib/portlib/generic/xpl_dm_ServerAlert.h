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

/*
 *  DESCRIPTION:
 *      The xpl_dm_ServerAlert.h header file contains definition and function prototypes
 *      for server alerts/prompts
 */

#ifndef XPL_DMSERVERALERT_H
#define XPL_DMSERVERALERT_H

#include "xpl_dm_ServerAlertDef.h"
#include "dmtError.h"
#include "dmstring.h"
#include "dmvector.h"

typedef struct
{
   XPL_DM_ALERT_RES_T action; 
   DMString response;
} XPL_DM_ALERT_TEXTINPUT_RES_T;

typedef struct
{
   XPL_DM_ALERT_RES_T action; 
   INT32 response;
} XPL_DM_ALERT_SCHOICE_RES_T;

typedef struct
{
   XPL_DM_ALERT_RES_T action; 
   DMStringVector responses;
} XPL_DM_ALERT_MCHOICE_RES_T;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Display a confirm alert message box, user can confirm or cancel the action
 * 
 * @param maxDisplayTime maximum display time (for timeout), in seconds.
 * @param msg messages to display
 * @param defaultResponse default user action when timeout
 * @param responseCode user's action will be returned here.
 * @return Upon successful completion, the SYNCML_DM_SUCCESS is returned, otherwise 
 *  SYNCML_DM_FAIL or other more specific error codes.
 **/

SYNCML_DM_RET_STATUS_T XPL_DM_ShowConfirmAlert(INT32 maxDisplayTime, 
                                          CPCHAR msg, 
                                          XPL_DM_ALERT_RES_T defaultResponse, 
                                          XPL_DM_ALERT_RES_T * responseCode);

/**
 * Display a text messages
 * 
 * @param minDisplayTime minimum display time, in seconds.
 * @param msg messages to display
 * @return Upon successful completion, the SYNCML_DM_SUCCESS is returned, otherwise 
 *  SYNCML_DM_FAIL or other more specific error codes.
 **/
SYNCML_DM_RET_STATUS_T XPL_DM_ShowDisplayAlert(INT32 minDisplayTime, 
                                          CPCHAR msg);


/**
 * Display a text input message box for user to enter input.
 * 
 * @param maxDisplayTime maximum display time (for timeout), in seconds.
 * @param msg messages to display
 * @param defaultResponse default user action when timeout
 * @param maxLength length allowed in user input
 * @param inputType data format as specified in DM_ALERT_INPUT_T
 * @param echoType whether to echo user input (hidden for password ) as specified in DM_ALERT_ECHO_T
 * @param response hold user's response action and input data.
 * @return Upon successful completion, the SYNCML_DM_SUCCESS is returned, otherwise 
 *  SYNCML_DM_FAIL or other more specific error codes.
 **/
SYNCML_DM_RET_STATUS_T XPL_DM_ShowTextInputAlert(INT32 maxDisplayTime, 
                                            CPCHAR msg, 
                                            CPCHAR defaultResponse,
                                            INT32 maxLength,
                                            XPL_DM_ALERT_INPUT_T inputType, 
                                            XPL_DM_ALERT_ECHO_T echoType, 
                                            XPL_DM_ALERT_TEXTINPUT_RES_T * response ); 



/**
 * Display a single choice message box for user to pick up one entry.
 * 
 * @param maxDisplayTime maximum display time (for timeout), in seconds.
 * @param msg messages to display
 * @param choices a string vector to hold text for each choice
 * @param defaultResponse default user action when timeout
 * @param response hold user's response action and selected choice.
 * @return Upon successful completion, the SYNCML_DM_SUCCESS is returned, otherwise 
 *  SYNCML_DM_FAIL or other more specific error codes.
 **/
SYNCML_DM_RET_STATUS_T  XPL_DM_ShowSingleChoiceAlert(INT32 maxDisplayTime, 
                                          CPCHAR msg,
                                          DMStringVector & choices,
                                          INT32 defaultResponse,
                                          XPL_DM_ALERT_SCHOICE_RES_T * response ); 



/**
 * Display a multiple choice message box for user to pick up zero to many entry.
 * 
 * @param maxDisplayTime maximum display time (for timeout), in seconds.
 * @param msg messages to display
 * @param choices a string vector to hold text for each choice
 * @param defaultResponse default user action when timeout
 * @param defaultResponses holds default response in an array of string representation of 
 *        selected indexes (starting from 1)
 * @param response hold user's response action and selected choice.
 * @return Upon successful completion, the SYNCML_DM_SUCCESS is returned, otherwise 
 *  SYNCML_DM_FAIL or other more specific error codes.
 **/
SYNCML_DM_RET_STATUS_T XPL_DM_ShowMultipleChoiceAlert(INT32 maxDisplayTime,
                                                 CPCHAR msg, 
                                                 DMStringVector & choices, 
                                                 DMStringVector & defaultResponses,
                                                 XPL_DM_ALERT_MCHOICE_RES_T * response); 


#ifdef __cplusplus
}
#endif

#endif
