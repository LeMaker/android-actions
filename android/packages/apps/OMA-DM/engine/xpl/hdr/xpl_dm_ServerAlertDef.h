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
 *      The xpl_dm_ServerAlertDef.h header file contains defenition for server 
 *      alerts/prompts
 */

#ifndef XPL_DMSERVERALERTDEF_H
#define XPL_DMSERVERALERTDEF_H

#include "xpl_Types.h"

enum 
{
  XPL_DM_ALERT_RES_NONE,
  XPL_DM_ALERT_RES_NO,
  XPL_DM_ALERT_RES_YES,
  XPL_DM_ALERT_RES_CANCEL,
  XPL_DM_ALERT_RES_TIMEOUT,
};
typedef UINT8 XPL_DM_ALERT_RES_T;

enum 
{
  XPL_DM_ALERT_I_ALPHA,                                      // Alphanumeric
  XPL_DM_ALERT_I_NUMERIC,                                    // Numeric
  XPL_DM_ALERT_I_DATE,                                       // Date
  XPL_DM_ALERT_I_TIME,                                       // Time
  XPL_DM_ALERT_I_PHONE_NUM,                                  // Phone Number
  XPL_DM_ALERT_I_IP_ADDR,                                    // IP Address
};
typedef UINT8 XPL_DM_ALERT_INPUT_T;
  
enum 
{
  XPL_DM_ALERT_E_TEXT,                                       // Text
  XPL_DM_ALERT_E_PASSWD,                                     // Password
};
typedef UINT8 XPL_DM_ALERT_ECHO_T;

#endif
