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
 *      The xpl_Prompt.h header file defines prompt IDs used across E2E 
 *      components.
 */

#ifndef XPL_PROMPT_H
#define XPL_PROMPT_H

/************** HEADER FILE INCLUDES *****************************************/

#include "xpl_Types.h"

/************** STRUCTURES, ENUMS, AND TYPEDEFS ******************************/

enum {
  XPL_PROMPT_DM_ALERT_DISPLAY = 0,     /* DisplayAlert */
  XPL_PROMPT_DM_ALERT_CONFIRM,         /* ConfirmAlert */
  XPL_PROMPT_DM_ALERT_TEXT_INPUT,      /* TextInputAlert */
  XPL_PROMPT_DM_ALERT_SINGLE_CHOICE,   /* SingleChoiceAlert */
  XPL_PROMPT_DM_ALERT_MULTIPLE_CHOICE, /* MultipleChoiceAlert */

  /* all new prompt IDs should be added before XPL_PROMPT_LAST */
  XPL_PROMPT_LAST
};
typedef UINT32 XPL_PROMPT_ID_T;

#endif /* XPL_PROMPT_H */
