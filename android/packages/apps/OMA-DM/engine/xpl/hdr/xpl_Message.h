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

#ifndef XPL_MESSAGE_H
#define XPL_MESSAGE_H

#include "xpl_Port.h"

#ifdef __cplusplus
extern "C" {
#endif

/************** CONSTANTS ****************************************************/

#define XPL_DM_TASK_MESSAGE_CATEGORY 0

/************** STRUCTURES, ENUMS, AND TYPEDEFS ******************************/

enum
{
     XPL_MSG_RET_SUCCESS  = 0,     /* operation successfully completed */
     XPL_MSG_RET_FAIL = 1,         /* operation failed */
     XPL_MSG_RET_BADARGUMENT = 2,  /* bad argument */
};
typedef UINT8  XPL_MSG_RET_STATUS_T;


/*=================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================*/

/* Sends asyncronyous message/event. Callback may be incorporated into message body if required */
XPL_MSG_RET_STATUS_T XPL_MSG_Send(XPL_PORT_T port,
                                UINT32 msgtype, 
                                void *data,
                                UINT32 size); 



#ifdef __cplusplus
}
#endif

#endif /* XPL_MESSAGE_H */
