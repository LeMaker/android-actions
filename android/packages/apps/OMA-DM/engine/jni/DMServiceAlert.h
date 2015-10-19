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

#ifndef DMSERVICE_ALERT_H
#define DMSERVICE_ALERT_H

#ifndef __cplusplus
#error "This is a C++ header file; it requires C++ to compile."
#endif

/**
 * \file  DMServiceAlert.h
 * \brief Declaration of JNI implementation of DMServiceAlert.
 */

#include <DMServiceMain.h>
#include "xpl_dm_ServerAlert.h"

// DMServiceAlert Response Code
static const int DM_SERVICE_ALERT_RESP_FAIL     = -1;
static const int DM_SERVICE_ALERT_RESP_NONE     = 0;
static const int DM_SERVICE_ALERT_RESP_NO       = 1;
static const int DM_SERVICE_ALERT_RESP_YES      = 2;
static const int DM_SERVICE_ALERT_RESP_CANCEL   = 3;
static const int DM_SERVICE_ALERT_RESP_TIMEOUT  = 4;

#ifdef DM_SDMSERVICES
static const int DM_SERVICE_ALERT_MAX_MSG_SIZE  = 250;
#endif

// DMServiceAlert Input Type
static const int DM_SERVICE_ALERT_INPUT_ALPHA       = 0;
static const int DM_SERVICE_ALERT_INPUT_NUMERIC     = 1;
static const int DM_SERVICE_ALERT_INPUT_DATE        = 2;
static const int DM_SERVICE_ALERT_INPUT_TIME        = 3;
static const int DM_SERVICE_ALERT_INPUT_PHONE_NUM   = 4;
static const int DM_SERVICE_ALERT_INPUT_IP_ADDR     = 5;

// DMServiceAlert Echo Type
static const int DM_SERVICE_ALERT_ECHO_TEXT         = 0;
static const int DM_SERVICE_ALERT_ECHO_PASSWD       = 1;

// DMServiceAlert Icon Type
static const int DM_SERVICE_ALERT_ICON_GENERIC      = 0;
static const int DM_SERVICE_ALERT_ICON_PROGRESS     = 1;
static const int DM_SERVICE_ALERT_ICON_OK           = 2;
static const int DM_SERVICE_ALERT_ICON_ERROR        = 3;
static const int DM_SERVICE_ALERT_ICON_CONFIRM      = 4;
static const int DM_SERVICE_ALERT_ICON_ACTION       = 5;
static const int DM_SERVICE_ALERT_ICON_INFO         = 6;

// DMServiceAlert Title Type
static const int DM_SERVICE_ALERT_TITLE_NULL                = 0;
static const int PMF_RESOURCE_ID_TITLE_SYSTEM_UPDATE        = 1;
static const int PMF_RESOURCE_ID_TITLE_NEED_AUTHENTICATION  = 2;
static const int PMF_RESOURCE_ID_TITLE_UPDATE_COMPLETE      = 3;
static const int PMF_RESOURCE_ID_TITLE_SYSTEM_MESSAGE           = 4;
static const int PMF_RESOURCE_ID_TITLE_AUTHENTICATION_FAILED    = 5;
static const int PMF_RESOURCE_ID_TITLE_UPDATE_ERROR             = 6;
static const int PMF_RESOURCE_ID_TITLE_UPDATE_CANCELLED         = 7;
static const int PMF_RESOURCE_ID_TITLE_PROFILE_FOR_BROWSER      = 8;
static const int PMF_RESOURCE_ID_TITLE_PROFILE_FOR_MMS          = 9;
static const int PMF_RESOURCE_ID_TITLE_CONNECTION_FAILED        = 10;
static const int PMF_RESOURCE_ID_TITLE_CONNECTION_FAILURE       = 11;
static const int PMF_RESOURCE_ID_TITLE_SW_UPDATE                = 12;
static const int PMF_RESOURCE_ID_14674_TITLE_REGISTRATION       = 13;
static const int PMF_RESOURCE_ID_CONTEXT_SYSTEM_UPDATE_SEVERAL_MINUTES_NO_PROCEED = 14;
static const int PMF_RESOURCE_ID_CONTEXT_SYSTEM_UPDATE_NOTICE   = 15;
static const int PMF_RESOURCE_ID_CONTEXT_SYSTEM_UPDATE          = 16;
static const int PMF_RESOURCE_ID_CONTEXT_ENTER_PIN_CARRIER      = 17;
static const int PMF_RESOURCE_ID_CONTEXT_SYSTEM_UPDATE_IN_PROGRESS      = 18;
static const int PMF_RESOURCE_ID_CONTEXT_DO_YOU_WANT_TO_ACCEPT_UPDATE   = 19;
static const int PMF_RESOURCE_ID_CONNECTING_TO_UPDATE_SERVICE           = 20;
static const int PMF_RESOURCE_ID_CONTEXT_SYSTEM_UPDATE_COMPLETED        = 21;
static const int PMF_RESOURCE_ID_CONTEXT_CONNECTION_SUCCEEDED           = 22;
static const int PMF_RESOURCE_ID_CONTEXT_PIN_FAILED_TRY_AGAIN           = 23;
static const int PMF_RESOURCE_ID_CONTEXT_PIN_FAILED_CONTACT_CARRIER     = 24;
static const int PMF_RESOURCE_ID_CONTEXT_SYSTEM_UPDATE_FAILED_CONTACT_CARRIER = 25;
static const int PMF_RESOURCE_ID_CONTEXT_CONNECTION_FAILED              = 26;
static const int PMF_RESOURCE_ID_CONTEXT_SYSTEM_UPDATE_CANCELLED        = 27;
static const int PMF_RESOURCE_ID_CONTEXT_PLEASE_SELECT_BROWSER_PROFILE  = 28;
static const int PMF_RESOURCE_ID_CONTEXT_PLEASE_SELECT_MMS_PROFILE      = 29;
static const int PMF_RESOURCE_ID_CONTEXT_AUTH_FAILED_CONTACT_CARRIER    = 30;
static const int PMF_RESOURCE_ID_CONTEXT_NWNOT_AVAILABLE                = 31;
static const int PMF_RESOURCE_ID_CONTEXT_CONNECT_ERROR                  = 32;
static const int PMF_RESOURCE_ID_CONTEXT_SW_UPDATE                      = 33;
static const int PMF_RESOURCE_ID_CONTEXT_CONNECTING                     = 34;

class DMServiceAlert
{
public:

    /**
     * Constructor.
     */
    DMServiceAlert();

    /**
     * Destructor
     */
    ~DMServiceAlert();

    /**
     * Display a text message alert.
     *
     * \param[in] minDisplayTime minimum display time, in seconds.
     * \param[in] msg message to display
     * \return SYNCML_DM_STATUS_T::SUCCESS on success; otherwise returns error code.
     */
    SYNCML_DM_RET_STATUS_T showDisplayAlert(INT32 minDisplayTime, const DMString& msg,
            INT32 title = DM_SERVICE_ALERT_TITLE_NULL, INT32 icon = DM_SERVICE_ALERT_ICON_GENERIC);

    /**
     * Display a confirm alert message box; user can confirm or cancel the action.
     *
     * \param[in] maxDisplayTime maximum display time (for timeout), in seconds.
     * \param[in] msg message to display
     * \param[in] defaultResponse default user action when timeout
     * \param[out] response user's action will be returned here.
     * \return SYNCML_DM_STATUS_T::SUCCESS on success; otherwise returns error code.
     */
    SYNCML_DM_RET_STATUS_T showConfirmAlert(INT32 maxDisplayTime, const DMString& msg,
            XPL_DM_ALERT_RES_T defaultResponse, XPL_DM_ALERT_RES_T* response,
            INT32 title = DM_SERVICE_ALERT_TITLE_NULL, INT32 icon = DM_SERVICE_ALERT_ICON_GENERIC);

    /**
     * Display a text input message box for user to enter input.
     *
     * \param[in] maxDisplayTime maximum display time (for timeout), in seconds.
     * \param[in] msg message to display
     * \param[in] defaultResponse default user action when timeout
     * \param[in] maxLength length allowed in user input
     * \param[in] inputType data format as specified in DM_ALERT_INPUT_T
     * \param[in] echoType whether to echo user input (hidden for password)
     * \param[out] response hold user's response action and input data.
     * \return SYNCML_DM_STATUS_T::SUCCESS on success; otherwise returns error code.
     */
    SYNCML_DM_RET_STATUS_T showTextInputAlert(INT32 maxDisplayTime, const DMString& msg,
            const DMString& defaultResponse, INT32 maxLength, XPL_DM_ALERT_INPUT_T inputType,
            XPL_DM_ALERT_ECHO_T echoType, XPL_DM_ALERT_TEXTINPUT_RES_T* response,
            INT32 title = DM_SERVICE_ALERT_TITLE_NULL, INT32 icon = DM_SERVICE_ALERT_ICON_GENERIC);

    /**
     * Display a single choice message box for user to select one entry.
     *
     * \param[in] maxDisplayTime maximum display time (for timeout), in seconds.
     * \param[in] msg message to display
     * \param[in] choices a string vector to hold text for each choice
     * \param[in] defaultResponse default user action when timeout
     * \param[out] response hold user's response action and selected choice.
     * \return SYNCML_DM_STATUS_T::SUCCESS on success; otherwise returns error code.
     */
    SYNCML_DM_RET_STATUS_T showSingleChoiceAlert(INT32 maxDisplayTime, const DMString& msg,
            const DMStringVector& choices, INT32 defaultResponse,
            XPL_DM_ALERT_SCHOICE_RES_T* response, INT32 title = DM_SERVICE_ALERT_TITLE_NULL,
            INT32 icon = DM_SERVICE_ALERT_ICON_GENERIC);

    /**
     * Display a multiple choice message box for user to select zero to many entries.
     *
     * \param[in] maxDisplayTime maximum display time (for timeout), in seconds.
     * \param[in] msg message to display
     * \param[in] choices a string vector to hold text for each choice
     * \param[in] defaultResponses holds default responses in an array of string representation of
     * selected indexes (starting from 1)
     * \param[out] response hold user's response action and selected choice.
     * \return SYNCML_DM_STATUS_T::SUCCESS on success; otherwise returns error code.
     */
    SYNCML_DM_RET_STATUS_T showMultipleChoiceAlert(INT32 maxDisplayTime, const DMString& msg,
            const DMStringVector& choices, const DMStringVector& defaultResponses,
            XPL_DM_ALERT_MCHOICE_RES_T* response, INT32 title = DM_SERVICE_ALERT_TITLE_NULL,
            INT32 icon = DM_SERVICE_ALERT_ICON_GENERIC);

private:

    JNIEnv*    m_jDmEnv;
    jclass     m_jDmAlertCls;
    jobject    m_jDmAlertObj;
    jmethodID  m_jDmAlertMID;

    bool isJvmNull() {
        return (m_jDmEnv == NULL || m_jDmAlertCls == NULL || m_jDmAlertObj == NULL);
    }
};

#endif /* DMSERVICEALERT_H */
