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

#include "DMServiceAlert.h"
#include "xpl_dm_ServerAlert.h"

#include <android_runtime/AndroidRuntime.h>

/**
 * Constructor
 *
 **/
DMServiceAlert::DMServiceAlert() : m_jDmEnv(NULL), m_jDmAlertCls(NULL), m_jDmAlertObj(NULL)
{
   LOGD("DMServiceAlert: enter  DMServiceAlert::DMServiceAlert()");
   m_jDmEnv = android::AndroidRuntime::getJNIEnv();
   if ( NULL == m_jDmEnv ) {
      LOGE("DMServiceAlert: android::AndroidRuntime::getJNIEnv() failed!");
      return;
   }
   LOGD("DMServiceAlert: Get JNI Env success.");

   m_jDmAlertObj = getDMAlert(m_jDmEnv);
   if ( NULL == m_jDmAlertObj) {
      LOGE("DMServiceAlert: m_jDmEnv->CallObjectMethod failed!");
      return;
   }
   LOGD("DMServiceAlert: Get DmAlert Object success");

   m_jDmAlertCls = m_jDmEnv->GetObjectClass(m_jDmAlertObj);
   if ( NULL == m_jDmAlertCls ) {
      LOGE("DMServiceAlert: m_jDmEnv->GetObjectClass failed!");
      return;
   }
   LOGD("DMServiceAlert: Get DmAlert Class success.");
}

/**
 * De-Constructor
 *
 **/
DMServiceAlert::~DMServiceAlert()
{
   if ( !m_jDmEnv ) {
      return;
   }

   if ( m_jDmAlertCls ) {
      m_jDmEnv->DeleteLocalRef(m_jDmAlertCls);
   }

   if ( m_jDmAlertObj ) {
      m_jDmEnv->DeleteLocalRef(m_jDmAlertObj);
   }
}

/**
 * Display a text messages
 *
 * @param minDisplayTime minimum display time, in seconds.
 * @param msg messages to display
 * @return Upon successful completion, the SYNCML_DM_SUCCESS is returned, otherwise
 * SYNCML_DM_FAIL or other more specific error codes.
 **/
SYNCML_DM_RET_STATUS_T
DMServiceAlert::showDisplayAlert(INT32 minDisplayTime, const DMString& msg, INT32 title, INT32 icon)
{
   LOGD("DMServiceAlert: enter showDisplayAlert()");

   if (isJvmNull()) {
      LOGE("DMServiceAlert: JVM validation failed!");
      return SYNCML_DM_FAIL;
   }

   m_jDmAlertMID = m_jDmEnv->GetMethodID(m_jDmAlertCls,
                                         "showDisplayAlert",
                                         "(ILjava/lang/String;II)I");
   if (NULL == m_jDmAlertMID) {
      LOGE("DMServiceAlert: m_jDmEnv->GetMethodID(showDisplayAlert) failed");
      return SYNCML_DM_FAIL;
   }

   LOGD("DMServiceAlert: m_jDmEnv->GetMethodID(showDisplayAlert) success.");
   jint t = minDisplayTime;
   jstring m = m_jDmEnv->NewStringUTF(msg.c_str());
   jint tl = title;
   jint ic = icon;

   jint r = m_jDmEnv->CallIntMethod(m_jDmAlertObj, m_jDmAlertMID, t, m, tl, ic);
   m_jDmEnv->DeleteLocalRef(m);
   LOGD("DMServiceAlert: DisplayAlert result: %d", r);

   return (r != DM_SERVICE_ALERT_RESP_FAIL)
            ? SYNCML_DM_SUCCESS : SYNCML_DM_FAIL;
};

/**
 * Display a confirm alert message box, user can confirm or cancel the action
 *
 * @param maxDisplayTime maximum display time (for timeout), in seconds.
 * @param msg messages to display
 * @param defaultResponse default user action when timeout
 * @param responseCode user's action will be returned here.
 * @return Upon successful completion, the SYNCML_DM_SUCCESS is returned, otherwise
 * SYNCML_DM_FAIL or other more specific error codes.
 **/
SYNCML_DM_RET_STATUS_T
DMServiceAlert::showConfirmAlert(INT32 maxDisplayTime, const DMString& msg,
                                  XPL_DM_ALERT_RES_T defaultResponse,
                                  XPL_DM_ALERT_RES_T *response,
                                  INT32 title,
                                  INT32 icon)
{
   LOGD("DMServiceAlert: enter showConfirmAlert()");

   if (isJvmNull()) {
      LOGE("DMServiceAlert: JVM validation failed!");
      return SYNCML_DM_FAIL;
   }

 #ifdef DM_SDMSERVICES
   if (strlen(msg) > DM_SERVICE_ALERT_MAX_MSG_SIZE) {
        LOGD("DMServiceAlert: showConfirmAlert MAX UI ALERT STRING SIZE is 250");
        return SYNCML_DM_BAD_REQUEST;
   }

   for(int i=0; i<strlen(msg); i++){
        if( msg[i]=='$' || msg[i]=='%' || msg[i]=='^' || msg[i]=='&' || msg[i]=='_' ) {
            LOGD("DMServiceAlert: Confirm Alert message contains unacceptable character");
            return SYNCML_DM_BAD_REQUEST;
        }
   }
 #endif

   m_jDmAlertMID = m_jDmEnv->GetMethodID(m_jDmAlertCls,
                                         "showConfirmAlert",
                                         "(ILjava/lang/String;II)I");
   if ( NULL == m_jDmAlertMID ) {
      LOGE("DMServiceAlert: m_jDmEnv->GetMethodID(showConfirmAlert) failed");
      return SYNCML_DM_FAIL;
   }

   LOGD("DMServiceAlert: m_jDmEnv->GetMethodID(showConfirmAlert) success.");
   jint t = maxDisplayTime;
   jstring m = m_jDmEnv->NewStringUTF(msg.c_str());
   jint tl = title;
   jint ic = icon;

   jint r = m_jDmEnv->CallIntMethod(m_jDmAlertObj, m_jDmAlertMID, t, m, tl, ic);
   m_jDmEnv->DeleteLocalRef(m);
   LOGD("DMServiceAlert: ConfirmAlert result: %d", r);

   if (r == DM_SERVICE_ALERT_RESP_TIMEOUT) {
      *response = defaultResponse;
   }
   else {
      *response = (XPL_DM_ALERT_RES_T) r;
   }

   return (r != DM_SERVICE_ALERT_RESP_FAIL)
            ? SYNCML_DM_SUCCESS : SYNCML_DM_FAIL;
}

/**
 * Display a text input message box for user to enter input.
 *
 * @param maxDisplayTime maximum display time (for timeout), in seconds.
 * @param msg messages to display
 * @param defaultResponse default user action when timeout
 * @param maxLength length allowed in user input
 * @param inputType data format as specified in DM_ALERT_INPUT_T
 * @param echoType whether to echo user input (hidden for password ) as specified in DM_ALERT_ECHO_T
 * @param response hold user's response action and input data.
 * @return Upon successful completion, the SYNCML_DM_SUCCESS is returned, otherwise
 * SYNCML_DM_FAIL or other more specific error codes.
 **/
SYNCML_DM_RET_STATUS_T
DMServiceAlert::showTextInputAlert( INT32 maxDisplayTime,
                                    const DMString& msg,
                                    const DMString& defaultResponse,
                                    INT32 maxLength,
                                    XPL_DM_ALERT_INPUT_T inputType,
                                    XPL_DM_ALERT_ECHO_T echoType,
                                    XPL_DM_ALERT_TEXTINPUT_RES_T * response,
                                    INT32 title,
                                    INT32 icon)
{
   LOGD("DMServiceAlert: enter showTextInputAlert()");

   if (isJvmNull()) {
      LOGE("DMServiceAlert: JVM validation failed!");
      return SYNCML_DM_FAIL;
   }

   m_jDmAlertMID = m_jDmEnv->GetMethodID(m_jDmAlertCls,
                                         "showTextInputAlert",
                                         "(ILjava/lang/String;Ljava/lang/String;IIIII)Ljava/lang/String;");
   if ( NULL == m_jDmAlertMID ) {
      LOGE("DMServiceAlert: m_jDmEnv->GetMethodID(showTextInputAlert) failed");
      return SYNCML_DM_FAIL;
   }

   LOGD("DMServiceAlert: m_jDmEnv->GetMethodID(showTextInputAlert) success.");
   jint t = maxDisplayTime;
   jstring m = m_jDmEnv->NewStringUTF(msg.c_str());
   jstring d = m_jDmEnv->NewStringUTF(defaultResponse.c_str());
   jint l = maxLength;
   jint i = (int)inputType;
   jint e = (int)echoType;
   jint tl = title;
   jint ic = icon;

   jstring r = (jstring)m_jDmEnv->CallObjectMethod(m_jDmAlertObj, m_jDmAlertMID, t, m, d, l, i, e, tl, ic);
   m_jDmEnv->DeleteLocalRef(m);

   const char *rlt = m_jDmEnv->GetStringUTFChars(r, 0);
   if ( NULL == rlt || '-' == rlt[0] ) {
      LOGE("DMServiceAlert: TextInputAlert return NULL or FAIL !");
      return SYNCML_DM_FAIL;
   }

   LOGD("DMServiceAlert: TextInputAlert result: %s", rlt);

   int a = rlt[0] - '0';
   if ( a == DM_SERVICE_ALERT_RESP_FAIL ) {
      LOGE("DMServiceAlert: TextInputAlert return fail!");
      return SYNCML_DM_FAIL;
   }

   response->action = (XPL_DM_ALERT_RES_T) a;
   if (a == DM_SERVICE_ALERT_RESP_TIMEOUT) {
      response->response = defaultResponse;
   }
   else {
      response->response = DMString(rlt+2);
   }

   return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T
DMServiceAlert::showSingleChoiceAlert(INT32 maxDisplayTime, const DMString& msg,
        const DMStringVector& choices, INT32 defaultResponse,
        XPL_DM_ALERT_SCHOICE_RES_T* response, INT32 title, INT32 icon)
{
   LOGD("DMServiceAlert: enter showSingleChoiceAlert()");

   if (isJvmNull()) {
      LOGE("DMServiceAlert: JVM validation failed!");
      return SYNCML_DM_FAIL;
   }

   m_jDmAlertMID = m_jDmEnv->GetMethodID(m_jDmAlertCls,
                                         "showSingleChoiceAlert",
                                         "(ILjava/lang/String;[Ljava/lang/String;III)Ljava/lang/String;");
   if ( NULL == m_jDmAlertMID ) {
      LOGE("DMServiceAlert: m_jDmEnv->GetMethodID(showSingleChoiceAlert) failed");
      return SYNCML_DM_FAIL;
   }

   LOGD("DMServiceAlert: m_jDmEnv->GetMethodID(showSingleChoiceAlert) success.");
   jint t = maxDisplayTime;
   jstring m = m_jDmEnv->NewStringUTF(msg.c_str());
   int size = choices.size();
   jobjectArray c = m_jDmEnv->NewObjectArray(size, m_jDmEnv->FindClass("java/lang/String"), NULL);
   for ( int i = 0; i < size; i ++ ) {
      m_jDmEnv->SetObjectArrayElement(c, i, m_jDmEnv->NewStringUTF(choices[i].c_str()));
   }
   jint d = defaultResponse - 1;
   jint tl = title;
   jint ic = icon;

   jstring r = (jstring)m_jDmEnv->CallObjectMethod(m_jDmAlertObj, m_jDmAlertMID, t, m, c, d, tl, ic);
    // FIXME: delete local refs to object array strings
   m_jDmEnv->DeleteLocalRef(m);

   const char *rlt = m_jDmEnv->GetStringUTFChars(r, 0);
   if ( NULL == rlt || '-' == rlt[0] ) {
      LOGE("DMServiceAlert: TextInputAlert return NULL or FAIL !");
      return SYNCML_DM_FAIL;
   }

   LOGD("DMServiceAlert: SingleChoiceAlert result: %s", rlt);

   int a = rlt[0] - '0';
   if (a == DM_SERVICE_ALERT_RESP_FAIL) {
      LOGE("DMServiceAlert: SingleChoiceAlert return fail!");
      return SYNCML_DM_FAIL;
   }

   response->action = (XPL_DM_ALERT_RES_T)a;
   if (a == DM_SERVICE_ALERT_RESP_TIMEOUT) {
      response->response = defaultResponse;
   }
   else {
      response->response = atoi(rlt+2) + 1;
   }

   return SYNCML_DM_SUCCESS;
}


SYNCML_DM_RET_STATUS_T
DMServiceAlert::showMultipleChoiceAlert(INT32 maxDisplayTime, const DMString& msg,
        const DMStringVector& choices, const DMStringVector& defaultResponses,
        XPL_DM_ALERT_MCHOICE_RES_T* response, INT32 title, INT32 icon)
{
   LOGD("DMServiceAlert: enter showMultipleChoiceAlert()");

   if (isJvmNull()) {
      LOGE("DMServiceAlert: JVM validation failed!");
      return SYNCML_DM_FAIL;
   }

   m_jDmAlertMID = m_jDmEnv->GetMethodID(m_jDmAlertCls,
                                         "showMultipleChoiceAlert",
                                         "(ILjava/lang/String;[Ljava/lang/String;[ZII)I");
   if ( NULL == m_jDmAlertMID ) {
      LOGE("DMServiceAlert: m_jDmEnv->GetMethodID(showMultipleChoiceAlert) failed");
      return SYNCML_DM_FAIL;
   }

   LOGD("DMServiceAlert: m_jDmEnv->GetMethodID(showMultipleChoiceAlert) success.");
   jint t = maxDisplayTime;
   jstring m = m_jDmEnv->NewStringUTF(msg.c_str());

   int size = choices.size();
   LOGD("DMServiceAlert: showMultipleChoiceAlert choices size: %d", size);
   jclass s = m_jDmEnv->FindClass("java/lang/String");
   jobjectArray c = m_jDmEnv->NewObjectArray(size, s, NULL);
   for (int i = 0; i < size; ++i) {
      m_jDmEnv->SetObjectArrayElement(c, i, m_jDmEnv->NewStringUTF(choices[i].c_str()));
   }

   jbooleanArray d = m_jDmEnv->NewBooleanArray(size);
   jboolean ds[size];
   int dsize = defaultResponses.size();
   LOGD("DMServiceAlert: showMultipleChoiceAlert default choices size: %d", dsize);
   for (int i = 0; i < size; ++i) {
      ds[i] = false;
   }
   int idx = 0;
   for (int i = 0; i < dsize; ++i) {
      idx = atoi(defaultResponses[i].c_str()) - 1;
      if ( idx < size ) {
         ds[idx] = true;
      }
   }
   m_jDmEnv->SetBooleanArrayRegion(d, (jsize)0, (jsize)size, (const jboolean *)ds);

   jint tl = title;
   jint ic = icon;

   jint r = m_jDmEnv->CallIntMethod(m_jDmAlertObj, m_jDmAlertMID, t, m, c, d, tl, ic);
    // FIXME: delete local refs to object array strings
   m_jDmEnv->DeleteLocalRef(m);

   if ( r == DM_SERVICE_ALERT_RESP_FAIL ) {
      LOGE("DMServiceAlert: m_jDmEnv->CallIntMethod(showMultipleChoiceAlert) failed");
      return SYNCML_DM_FAIL;
   }
   LOGD("DMServiceAlert: MultiChoiceAlert result: %d", r);

   response->action = (XPL_DM_ALERT_RES_T) r;
   if (r == DM_SERVICE_ALERT_RESP_TIMEOUT) {
      for ( int i = 0; i < dsize; i++ ) {
         response->responses.push_back(defaultResponses[i].c_str());
      }
   }
   else {

      char idx[4]; // Max 3 digits index by default;
      for ( int i = 0; i < size; i++) {
         m_jDmEnv->GetBooleanArrayRegion(d, (jsize)0, (jsize)size, (jboolean *)ds);
         LOGD("DMServiceAlert: showMultipleChoiceAlert %d response: %d", i, ds[i]);
         if ( ds[i] ) {
            snprintf(idx, 4, "%d", i + 1);
            response->responses.push_back(idx);
         }
      }
   }

   return SYNCML_DM_SUCCESS;
}
