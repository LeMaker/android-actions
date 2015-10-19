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

#ifndef SYNCML_DM_CHOICE_ALERT_H
#define SYNCML_DM_CHOICE_ALERT_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: SYNCML_DM_ChoiceAlert.h

    General Description: A class representing the display alerts.

==================================================================================================*/

#include "SYNCML_DM_Alert.H"

class SYNCML_DM_ChoiceAlert : public SYNCML_DM_Alert
{

public:
  virtual void parse(SmlAlertPtr_t  pContent);
  virtual ~SYNCML_DM_ChoiceAlert(){}
  
protected:
  DMStringVector choices;         // choices

  void parseChoices(SmlItemListPtr_t p_alert_list_item);

  void addChoice(const char* choice);

};

#endif /* SYNCML_DM_CHOICE_ALERT_H */
