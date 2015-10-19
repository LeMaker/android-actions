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
 *      The xpl_Notifications.h header file contains constants and function prototypes
 *      for dm specific notification functions      
 */

#ifndef XPL_DM_NOTIFICATIONS_H
#define XPL_DM_NOTIFICATIONS_H

#include "dmtDefs.h"
#include "dmtEventData.hpp"

#ifdef __cplusplus
extern "C" {
#endif


void  XPL_DM_NotifyTreeUpdate(CPCHAR szTopic, 
                                                CPCHAR szPath,
                                                SYNCML_DM_EVENT_TYPE_T nType,
                                                UINT8 * pData,
                                                UINT32 size);

void  XPL_DM_NotifySessionProgress(BOOLEAN bIsStarted);


#ifdef __cplusplus
}
#endif

#endif /* XPL_NOTIFICATIONS_H */
