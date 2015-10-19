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

#ifndef XPL_PORT_H
#define XPL_PORT_H

/************** HEADER FILE INCLUDES *****************************************/

#include "xpl_Types.h"

/************** STRUCTURES, ENUMS, AND TYPEDEFS ******************************/

enum {
  XPL_PORT_DM = 0,     /* Device management APPs */
  XPL_PORT_DM_TASK     /* Device management task port is used for async API messaging only */
};
typedef UINT8 XPL_PORT_T;
  
enum {
  XPL_LOG_PORT_DM_FIRST = 0,
  XPL_LOG_PORT_DM_API   = XPL_LOG_PORT_DM_FIRST,  /* Log DM API */
  XPL_LOG_PORT_DM_TMN,                            /* Log Tree management */
  XPL_LOG_PORT_DM_PLG,                            /* Log Plugin */
  XPL_LOG_PORT_DM_SESS,                           /* Log Session */
  XPL_LOG_PORT_DM_CONN,                           /* Log Connectivity */
  XPL_LOG_PORT_DM_XPL,                            /* Log Portlib */
  XPL_LOG_PORT_DM_CP,                             /* Client provisioning */

  /* DM: Insert new log ports here */
  
  XPL_LOG_PORT_DM_LAST

};

typedef UINT8 XPL_LOGS_PORT_T;

#endif /* XPL_PORT_H */
