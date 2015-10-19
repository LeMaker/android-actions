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

#ifndef SYNCML_DM_TEXTINPUT_ALERT_H
#define SYNCML_DM_TEXTINPUT_ALERT_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/*==================================================================================================

    Header Name: SYNCML_DM_TextInputAlert.h

    General Description: A class representing the display alerts.

==================================================================================================*/

#include "SYNCML_DM_Alert.H"

class SYNCML_DM_TextInputAlert : public SYNCML_DM_Alert
{

public:

  SYNCML_DM_TextInputAlert();
  virtual ~SYNCML_DM_TextInputAlert(){}

  SYNCML_DM_RET_STATUS_T show();

private:
  DMString defaultResponse;         // the default response
  INT32 maxLength;                  // maximum length for user input text
  XPL_DM_ALERT_INPUT_T inputType;          // alert input type
  XPL_DM_ALERT_ECHO_T echoType;            // alert echo type
  XPL_DM_ALERT_TEXTINPUT_RES_T response; 

  virtual XPL_DM_ALERT_RES_T getAction() const;

  virtual SYNCML_DM_RET_STATUS_T getDefaultResponse(DMStringVector & userResponse) const;
  
  virtual SYNCML_DM_RET_STATUS_T getResponse(DMStringVector & userResponse) const;   

  XPL_DM_ALERT_INPUT_T convertInputType(CPCHAR inputType);

  XPL_DM_ALERT_ECHO_T convertEchoType(CPCHAR echoType);

  virtual void processParameter(CPCHAR name, CPCHAR value);
};

#endif /* SYNCML_DM_TEXTINPUT_ALERT_H */
