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

    File Name: SYNCML_DM_Alert.cc

    General Description: A class representing the alerts.

==================================================================================================*/

#include "dmStringUtil.h"
#include "dm_uri_utils.h"
#include "SYNCML_DM_Alert.H"

SYNCML_DM_Alert::SYNCML_DM_Alert() {
   maxDisplayTime = -1;
   minDisplayTime = -1;
}

void SYNCML_DM_Alert::parse(SmlAlertPtr_t  pContent) {

  SmlItemListPtr_t     p_alert_list_item;

  if ( pContent == NULL )
    return; 
  
  p_alert_list_item = pContent->itemList;
    
  if ( p_alert_list_item != NULL )
  {
      parseParameters(p_alert_list_item);
      parseMessage(p_alert_list_item->next);
  }

} 

void SYNCML_DM_Alert::parseParameters(SmlItemListPtr_t p_alert_list_item) 
{
    SmlItemPtr_t         p_alert_item;
    SmlPcdataPtr_t       p_alert_data;

    if (p_alert_list_item == NULL)
        return;

    p_alert_item = p_alert_list_item->item;
    
    if (p_alert_item == NULL) 
        return;
    
    p_alert_data = p_alert_item->data;
    
    if ( p_alert_data == NULL )
        return;
    
    CPCHAR name;
    CPCHAR value;
    DMToken params(FALSE,(CPCHAR)p_alert_data->content,'&');
    CPCHAR pSegment = params.nextSegment();

    while ( pSegment != NULL ) 
    {
          {
                DMParser token(pSegment,'=');
                name = token.nextSegment();
                value = token.nextSegment();

                if ( value != NULL && name != NULL )
                	processParameter(name,value);
          }    
          pSegment = params.nextSegment();
     }
}



SYNCML_DM_RET_STATUS_T 
SYNCML_DM_Alert::processResponse(DMStringVector & userResponse, 
                                                       SYNCML_DM_ALERT_RES_T * alertStatus) 
{

    SYNCML_DM_RET_STATUS_T dm_stat = SYNCML_DM_SUCCESS;
    switch(getAction()) 
    {
         case XPL_DM_ALERT_RES_NO:
               *alertStatus =  SYNCML_DM_ALERT_NO;
               dm_stat = SYNCML_DM_NOT_MODIFIED; 
               break;
    
         case XPL_DM_ALERT_RES_YES:
                getResponse(userResponse);
                dm_stat = SYNCML_DM_SUCCESS; 
                break;

         case XPL_DM_ALERT_RES_CANCEL:
                *alertStatus =  SYNCML_DM_ALERT_CANCEL;
                dm_stat = SYNCML_DM_OPERATION_CANCELLED;       
                break;

         case XPL_DM_ALERT_RES_NONE:
         case XPL_DM_ALERT_RES_TIMEOUT:
                getDefaultResponse(userResponse);
                *alertStatus =  SYNCML_DM_ALERT_CANCEL;
                dm_stat = SYNCML_DM_REQUEST_TIMEOUT;
                break;

         default:
                break;
            
    }
    return dm_stat;

}
  

void SYNCML_DM_Alert::parseMessage(SmlItemListPtr_t p_alert_list_item) {
  SmlItemPtr_t       p_alert_item;
  SmlPcdataPtr_t    p_alert_data;

  if ( p_alert_list_item == NULL ) 
    return;
 
  p_alert_item = p_alert_list_item->item;

  if ( p_alert_item == NULL ) 
    return;
  
  p_alert_data = p_alert_item->data;       

  if (  p_alert_data == NULL )
    return;

  msg = (CPCHAR)p_alert_data->content;
}


void SYNCML_DM_Alert::setMaxDisplayTime(INT32 maxDisplayTime) 
{
    if (0 <= maxDisplayTime)
        this->maxDisplayTime = maxDisplayTime;
}

void SYNCML_DM_Alert::setMinDisplayTime(INT32 minDisplayTime) 
{
    if (0 <= minDisplayTime && minDisplayTime <= SYNCML_DM_ALERT_MAX_TIME)
        this->minDisplayTime = minDisplayTime;
}
