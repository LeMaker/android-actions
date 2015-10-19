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

#ifndef SYNCML_DM_ALERT_H
#define SYNCML_DM_ALERT_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: SYNCML_DM_Alert.H

    General Description: A class representing the alert.

==================================================================================================*/

#include "dmSessionDefs.h"
#include "xpl_dm_ServerAlert.h"

#define SYNCML_DM_ALERT_OPTION_MINDT "MINDT"                    // Minimum Display Time
#define SYNCML_DM_ALERT_OPTION_MAXDT "MAXDT"                    // Maximum Display Time
#define SYNCML_DM_ALERT_OPTION_DEFAULT_RESPONSE "DR"            // Default Response
#define SYNCML_DM_ALERT_OPTION_MAX_LENGTH "MAXLEN"                  // Maximum Length
#define SYNCML_DM_ALERT_OPTION_INPUT_TYPE "IT"                  // Input Type
#define SYNCML_DM_ALERT_OPTION_ECHO_TYPE "ET"                   // Echo Type

#define SYNCML_DM_ALERT_OPTION_IT_A "A"                         // Alphanumeric
#define SYNCML_DM_ALERT_OPTION_IT_N "N"                         // Numeric
#define SYNCML_DM_ALERT_OPTION_IT_D "D"                         // Date
#define SYNCML_DM_ALERT_OPTION_IT_T "T"                         // Time
#define SYNCML_DM_ALERT_OPTION_IT_P "P"                         // Phone
#define SYNCML_DM_ALERT_OPTION_IT_I "I"                         // IP Address

#define SYNCML_DM_ALERT_OPTION_ET_T "T"                         // Text
#define SYNCML_DM_ALERT_OPTION_ET_P "P"                         // Password

#define SYNCML_DM_ALERT_MAX_TIME 60                             // maximum display time in seconds


class SYNCML_DM_Alert {

  public: 

 
  SYNCML_DM_Alert();
  virtual ~SYNCML_DM_Alert() {};
 
  virtual void parse(SmlAlertPtr_t  pContent);

  virtual SYNCML_DM_RET_STATUS_T show() = 0;

  virtual  SYNCML_DM_RET_STATUS_T processResponse(DMStringVector & userResponse,
  	                                                                          SYNCML_DM_ALERT_RES_T * alertStatus ); 

  protected:

  DMString msg;              // the message to display
  INT32 minDisplayTime;    // minimum display time
  INT32 maxDisplayTime;       // maximum display time

  virtual XPL_DM_ALERT_RES_T getAction() const = 0;

  virtual SYNCML_DM_RET_STATUS_T getDefaultResponse(DMStringVector & userResponse) const = 0;
  
  virtual SYNCML_DM_RET_STATUS_T getResponse(DMStringVector & userResponse) const = 0;   
  
  void parseParameters(SmlItemListPtr_t p_alert_list_item);

  virtual void processParameter(CPCHAR name, CPCHAR value) = 0;

  void parseMessage(SmlItemListPtr_t p_alert_list_item);

  void setMinDisplayTime(INT32 minDisplayTime);

  void setMaxDisplayTime(INT32 maxDisplayTime);
};

#endif /* SYNCML_DM_ALERT_H */
