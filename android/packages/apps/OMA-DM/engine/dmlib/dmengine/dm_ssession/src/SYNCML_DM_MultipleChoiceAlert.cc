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

    File Name: SYNCML_DM_MultipleChoiceAlert.cc

    General Description: A class representing the alerts.

==================================================================================================*/

#include "dmStringUtil.h"
#include "dm_uri_utils.h"
#include "dmtoken.h"
#include "SYNCML_DM_MultipleChoiceAlert.H"

SYNCML_DM_MultipleChoiceAlert::SYNCML_DM_MultipleChoiceAlert()
{
    response.action = XPL_DM_ALERT_RES_TIMEOUT;
}

void SYNCML_DM_MultipleChoiceAlert::processParameter(CPCHAR name, CPCHAR value)
{

    if ( DmStrcmp(name,SYNCML_DM_ALERT_OPTION_MAXDT) == 0 )
        setMaxDisplayTime(DmAtoi(value));
    else 
         if ( DmStrcmp(name,SYNCML_DM_ALERT_OPTION_DEFAULT_RESPONSE) == 0 )
              parseDefaultResponses(value);
}

SYNCML_DM_RET_STATUS_T SYNCML_DM_MultipleChoiceAlert::show() 
{

  return XPL_DM_ShowMultipleChoiceAlert(maxDisplayTime, msg, choices, defaultResponses, &response); 
  
}

void SYNCML_DM_MultipleChoiceAlert::parseDefaultResponses(CPCHAR defaultResponses) 
{
      DMToken token(FALSE,defaultResponses,',');
      CPCHAR pSegment = token.nextSegment();
      while( pSegment != NULL ) 
      {
         if ( NULL == strchr(pSegment, '-') ) {
            this->defaultResponses.push_back(pSegment);
         }
         else {
            DMToken dtoken(FALSE,defaultResponses,'-');
            CPCHAR pdSegment = dtoken.nextSegment();

            int first = atoi(pdSegment);

            pdSegment = dtoken.nextSegment();
            if ( NULL == pdSegment ) {
               pSegment = token.nextSegment();
               continue;
            }

            int last = atoi(pdSegment);

            pdSegment = dtoken.nextSegment();
            if ( NULL != pdSegment ) {
               pSegment = token.nextSegment();
               continue;
            }

            char idx[4]; // Max 3 digits index by default;
            if((first >=0) && (last >= 0))
            {
	        for ( first; first <= last; first++ ) {
	           snprintf(idx, 4, "%d", first);
	           this->defaultResponses.push_back(idx);
	        }
            }
         }
         pSegment = token.nextSegment();
      }
}

XPL_DM_ALERT_RES_T SYNCML_DM_MultipleChoiceAlert::getAction() const
{
   return response.action;  
}


SYNCML_DM_RET_STATUS_T 
SYNCML_DM_MultipleChoiceAlert::getDefaultResponse(DMStringVector & userResponse) const
{
    userResponse = defaultResponses;
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T 
SYNCML_DM_MultipleChoiceAlert::getResponse(DMStringVector & userResponse) const
{
    userResponse = response.responses;
    return SYNCML_DM_SUCCESS;
}
