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

    File Name: SYNCML_DM_ChoiceAlert.cc

    General Description: A class representing the alerts.

==================================================================================================*/

#include "dmStringUtil.h"
#include "SYNCML_DM_ChoiceAlert.H"

void SYNCML_DM_ChoiceAlert::parse(SmlAlertPtr_t  pContent)
{

  SmlItemListPtr_t  p_alert_list_item;

  if ( pContent == NULL )
    return;
  
  SYNCML_DM_Alert::parse(pContent); 

  p_alert_list_item = pContent->itemList;
    
  if (p_alert_list_item != NULL && p_alert_list_item->next != NULL ) 
      parseChoices(p_alert_list_item->next->next);
} 


void SYNCML_DM_ChoiceAlert::parseChoices(SmlItemListPtr_t p_alert_list_item) 
{
  SmlItemPtr_t         p_alert_item;
  SmlPcdataPtr_t       p_alert_data;

  while (p_alert_list_item != NULL) 
  {
    p_alert_item = p_alert_list_item->item;
    p_alert_data = p_alert_item->data;

    addChoice((CPCHAR)p_alert_data->content);
    p_alert_list_item = p_alert_list_item->next;
  }
}

void SYNCML_DM_ChoiceAlert::addChoice(const char* choice) 
{
    choices.push_back(choice);
}
