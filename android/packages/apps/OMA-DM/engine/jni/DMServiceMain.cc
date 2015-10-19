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

// don't strip logging from release builds
#define LOG_NDEBUG 0

#include <android_runtime/AndroidRuntime.h>
#include "utils/Log.h"
#include "DMServiceMain.h"
#include "dmt.hpp"
#include "DMTreeManager.h"
static jobject g_sessionObj;
int g_cancelSession;

//#define LOGV printf
//#define LOGD printf


#define SET_RET_STATUS_BUF \
        jResult = jenv->NewByteArray(5); char szResult[5]; memset(szResult, 0, 5); \
        ::snprintf(szResult, 5, "%4d", ret_status); \
        jenv->SetByteArrayRegion(jResult, 0, 5, (const jbyte*)szResult)

static void Dump(const char* buf, int size, boolean isBinary)
{
  if (!isBinary) {
    // just print the string
    char* szBuf = new char[size + 1];

    memcpy(szBuf, buf, size);
    szBuf[size] = 0;

    LOGE("The test script error text:\n\n%s\n\n", szBuf);
  } else {
    int nOffset = 0;

    while (size > 0) {
      int nLine = size > 16 ? 16 : size;

      char s[250];
      int pos = 0;

      pos += ::snprintf(s + pos, (250 - pos), "%04x:", nOffset);

      for (int i = 0; i < nLine; i++) {
        pos += ::snprintf(s + pos, (250 - pos), " %02x", (unsigned int)((unsigned char) buf[i]) );
      }
      for (int i = nLine; i < 16; i++) {
        pos += ::snprintf(s + pos, (250 - pos), "   ");
      }

      pos += ::snprintf(s + pos, (250 - pos), "  ");
      for (int i = 0; i < nLine; i++) {
        pos += ::snprintf(s + pos, (250 - pos), "%c", (buf[i] > 31 ? buf[i] : '.') );
      }

      LOGE("%s\n", s);
      buf += nLine;
      size -= nLine;
      nOffset += nLine;
    }
  }
}

/**
 * check the input string is a a valid UTF-8 string or not
 * 1 -- valid, 0 -- invalid
 */
static jint isUtf8Valid(const char* bytes) {
    while (*bytes != '\0') {
      uint8_t utf8 = *(bytes++);
      // Switch on the high four bits.
      switch (utf8 >> 4) {
      case 0x00:
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07:
        // Bit pattern 0xxx. No need for any extra bytes.
        break;
      case 0x08:
      case 0x09:
      case 0x0a:
      case 0x0b:
      case 0x0f:
        /*
         * Bit pattern 10xx or 1111, which are illegal start bytes.
         * Note: 1111 is valid for normal UTF-8, but not the
         * Modified UTF-8 used here.
         */
        return 0;
      case 0x0e:
        // Bit pattern 1110, so there are two additional bytes.
        utf8 = *(bytes++);
        if ((utf8 & 0xc0) != 0x80) {
          return 0;
        }
        // Fall through to take care of the final byte.
      case 0x0c:
      case 0x0d:
        // Bit pattern 110x, so there is one additional byte.
        utf8 = *(bytes++);
        if ((utf8 & 0xc0) != 0x80) {
          return 0;
        }
        break;
      }
    }
    return 1;
}


JNIEXPORT jint
initialize(JNIEnv* /*env*/, jobject /*jobj*/)
{
  LOGD("native initialize");
  if (!DmtTreeFactory::Initialize()) {
    LOGE("Failed to initialize DM\n");
    return static_cast<jint>(SYNCML_DM_FAIL);
  }

  return static_cast<jint>(SYNCML_DM_SUCCESS);
}

JNIEXPORT jint
destroy(JNIEnv* /*env*/, jobject /*jobj*/)
{
  LOGD("Enter destroy");
  if (DmtTreeFactory::Uninitialize() != SYNCML_DM_SUCCESS) {
    LOGE("Failed to uninitialize DM\n");
    return static_cast<jint>(SYNCML_DM_FAIL);
  }

  LOGD("Leave destroy");
  return static_cast<jint>(SYNCML_DM_SUCCESS);
}

JNIEXPORT jint
parsePkg0(JNIEnv* env, jclass, jbyteArray jPkg0, jobject jNotification)
{
    LOGD("Enter parsePkg0");
    jclass notifClass = env->GetObjectClass(jNotification);

    if (jPkg0 == NULL) {
        return static_cast<jint>(SYNCML_DM_FAIL);
    }

    jbyte* pkg0Buf = env->GetByteArrayElements(jPkg0, NULL);
    jsize pkg0Len = env->GetArrayLength(jPkg0);

    DmtNotification notif;
    DmtPrincipal p("localhost");

    SYNCML_DM_RET_STATUS_T ret = DmtTreeFactory::ProcessNotification(p, (UINT8*)pkg0Buf, (INT32)pkg0Len, notif);

    if(ret == SYNCML_DM_FAIL) {
	return static_cast<jint>(SYNCML_DM_FAIL);
    }

    jmethodID jSetServerID = env->GetMethodID( notifClass, "setServerID", "(Ljava/lang/String;)V");

    if(isUtf8Valid(notif.getServerID().c_str())) {
        jstring jServerID = env->NewStringUTF(notif.getServerID().c_str());
        env->CallVoidMethod(jNotification, jSetServerID, jServerID);
    } else {
        LOGE("Invalid Server ID, not legal UTF8");
        return static_cast<jint>(SYNCML_DM_FAIL);
    }


    jmethodID jSetSessionID = env->GetMethodID( notifClass, "setSessionID", "(I)V");
    env->CallVoidMethod(jNotification, jSetSessionID, (jint)notif.getSessionID());

    jmethodID jSetUIMode = env->GetMethodID( notifClass, "setUIMode", "(I)V");
    env->CallVoidMethod(jNotification, jSetUIMode, (jint)notif.getUIMode());

    jmethodID jSetInitiator = env->GetMethodID( notifClass, "setInitiator", "(I)V");
    env->CallVoidMethod(jNotification, jSetInitiator, (jint)notif.getInitiator());

    jmethodID jSetAuthFlag = env->GetMethodID( notifClass, "setAuthFlag", "(I)V");
    env->CallVoidMethod(jNotification, jSetAuthFlag, (jint)notif.getAuthFlag());

    env->ReleaseByteArrayElements(jPkg0, pkg0Buf, 0);

    LOGD("Leave parsePkg0, ret: %d", ret);
    return static_cast<jint>(ret);
}


JNIEXPORT jint JNICALL startFotaClientSession(JNIEnv* jenv, jclass,
        jstring jServerId, jstring jAlertStr, jobject jdmobj)
{
    LOGV("In native startFotaClientSession\n");

    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_FAIL;
    DMString serverID;

    g_sessionObj = jdmobj;

    const char* szDmServerId = jenv->GetStringUTFChars(jServerId, NULL);
    const char* szDMAlertStr = NULL;
    if (jAlertStr != NULL) {
        szDMAlertStr = jenv->GetStringUTFChars(jAlertStr, NULL);
    }

    DmtPrincipal principal(szDmServerId);

    DMString alertURI("./DevDetail/Ext/SystemUpdate");
    DMString strEmpty;
    DmtFirmAlert alert(alertURI, strEmpty, szDMAlertStr, "chr", strEmpty, strEmpty);
    DmtSessionProp prop(alert, true);

    g_cancelSession = 0;

    ret_status = DmtTreeFactory::StartServerSession(principal, prop);

    if (jAlertStr != NULL) {
        jenv->ReleaseStringUTFChars(jAlertStr, szDMAlertStr);
    }

    g_sessionObj = NULL;
    if (ret_status == SYNCML_DM_SUCCESS) {
        LOGV("Native startFotaClientSession return successfully\n");
        return static_cast<jint>(SYNCML_DM_SUCCESS);
    } else {
        LOGE("Native startFotaClientSession return error %d\n", ret_status);
        return static_cast<jint>(ret_status);
    }
}

JNIEXPORT jint JNICALL startClientSession(JNIEnv* jenv, jclass,
        jstring jServerId, jobject jdmobj)
{
    LOGV("In native startClientSession\n");

    SYNCML_DM_RET_STATUS_T ret_status = SYNCML_DM_FAIL;
    DMString serverID;

    g_sessionObj = jdmobj;
    const char* szDmServerId = jenv->GetStringUTFChars(jServerId, NULL);
    DmtPrincipal principal(szDmServerId);

    DmtSessionProp prop(true);

    g_cancelSession = 0;

    ret_status = DmtTreeFactory::StartServerSession(principal, prop);

    jenv->ReleaseStringUTFChars(jServerId, szDmServerId);

    g_sessionObj = NULL;
    if (ret_status == SYNCML_DM_SUCCESS) {
        LOGV("Native startClientSession return successfully\n");
    } else {
        LOGV("Native startClientSession return error %d\n", ret_status);
    }
    return static_cast<jint>(ret_status);
}

JNIEXPORT jint JNICALL startFotaServerSession(JNIEnv* jenv, jclass,
        jstring jServerId, jint sessionID, jobject jdmobj)
{
    LOGV("In native startFotaServerSession\n");

    g_sessionObj = jdmobj;

    const char* szDmServerId = jenv->GetStringUTFChars(jServerId, NULL);
    DmtPrincipal principal(szDmServerId);
    DmtSessionProp prop(static_cast<UINT16>(sessionID), true);

    g_cancelSession = 0;

    SYNCML_DM_RET_STATUS_T ret_status = DmtTreeFactory::StartServerSession(principal, prop);

    jenv->ReleaseStringUTFChars(jServerId, szDmServerId);

    g_sessionObj = NULL;

    if (ret_status == SYNCML_DM_SUCCESS) {
        LOGV("Native startFotaServerSession return successfully\n");
    } else {
        LOGV("Native startFotaServerSession return error %d\n", ret_status);
    }
    return static_cast<jint>(ret_status);
}

JNIEXPORT jint JNICALL startFotaNotifySession(JNIEnv* jenv, jclass,
        jstring result, jstring pkgURI, jstring alertType,
        jstring serverID, jstring correlator, jobject jdmobj)
{
    g_sessionObj = jdmobj;

    const char* szResult = jenv->GetStringUTFChars(result, NULL);
    const char* szPkgURI = jenv->GetStringUTFChars(pkgURI, NULL);
    const char* szAlertType = jenv->GetStringUTFChars(alertType, NULL);
    const char* szDmServerId = jenv->GetStringUTFChars(serverID, NULL);
    const char* szCorrelator = jenv->GetStringUTFChars(correlator, NULL);

    DmtPrincipal principal(szDmServerId);
    DmtFirmAlert alert(szPkgURI, szResult, szAlertType, "chr", NULL, szCorrelator);
    DmtSessionProp prop(alert, true);

    SYNCML_DM_RET_STATUS_T dm_result = SYNCML_DM_SUCCESS;
    g_cancelSession = 0;

    dm_result = DmtTreeFactory::StartServerSession(principal, prop);

    jenv->ReleaseStringUTFChars(result, szResult);
    jenv->ReleaseStringUTFChars(pkgURI, szPkgURI);
    jenv->ReleaseStringUTFChars(alertType, szAlertType);
    jenv->ReleaseStringUTFChars(serverID, szDmServerId);
    jenv->ReleaseStringUTFChars(correlator, szCorrelator);

    g_sessionObj = NULL;
    if (dm_result == SYNCML_DM_SUCCESS) {
        LOGV("Native startFotaNotifySession return successfully\n");
    } else {
        LOGV("Native startFotaNotifySession return error %d\n", dm_result);
    }
    return static_cast<jint>(dm_result);
}

jobject getNetConnector()
{
    JNIEnv* env = android::AndroidRuntime::getJNIEnv();

    jclass jdmSessionClz = env->GetObjectClass(g_sessionObj);
    jmethodID jgetNet = env->GetMethodID(jdmSessionClz,
            "getNetConnector",
            "()Lcom/android/omadm/service/DMHttpConnector;");
    return env->CallObjectMethod(g_sessionObj, jgetNet);
}

jobject getDMAlert(JNIEnv* env)
{
   LOGD(("DM Alert: enter getDMAlert()"));
   if (NULL == g_sessionObj) {
       LOGE(("DM Alert: g_sessionObj is NULL!"));
       return NULL;
   }

   jclass jdmSessionClz = env->GetObjectClass(g_sessionObj);
   if (NULL == jdmSessionClz) {
       LOGE(("DM Alert: env->GetObjectClass(g_sessionObj) failed!"));
       return NULL;
   }
   LOGD(("DM Alert: success env->GetObjectClass(...)"));

   jmethodID jdmGetDMAlert = env->GetMethodID(jdmSessionClz,
            "getDMAlert",
            "()Lcom/android/omadm/service/DMAlert;");
   if ( NULL == jdmGetDMAlert ) {
       LOGE(("DM Alert: env->GetMethodID(jdmSessionClz) failed!"));
       return NULL;
   }
   LOGD(("DM Alert: success env->GetMethodID(...)"));

   return env->CallObjectMethod(g_sessionObj, jdmGetDMAlert);
}

JNIEXPORT jint JNICALL cancelSession(JNIEnv*, jclass)
{
    g_cancelSession = 1;
    return static_cast<jint>(SYNCML_DM_SUCCESS);
}

JNIEXPORT jstring JNICALL parseBootstrapServerId(JNIEnv* jenv, jclass, jbyteArray jMsgBuf,
        jboolean isWbxml)
{
    jint retCode = 0;
    jstring jServerId = NULL;

    SYNCML_DM_RET_STATUS_T dm_ret_status;

    jbyte* jBuf = jenv->GetByteArrayElements(jMsgBuf, NULL);
    jsize jBufSize = jenv->GetArrayLength(jMsgBuf);

    DmtPrincipal principal("DM_BOOTSTRAP");
    DMString strServerId;
    dm_ret_status = DmtTreeFactory::Bootstrap(principal, (const UINT8*)jBuf, jBufSize, isWbxml,
            false, strServerId);

    LOGD("parseBootstrapServerId dm_ret_status: %d", dm_ret_status);

    if (dm_ret_status == SYNCML_DM_SUCCESS && !strServerId.empty()) {
        LOGD("parseBootstrapServerId returns strServerId: %s", strServerId.c_str());
        jServerId = jenv->NewStringUTF(strServerId.c_str());
    }

    return jServerId;
}

JNIEXPORT jint JNICALL processBootstrapScript(JNIEnv* jenv, jclass, jbyteArray jMsgBuf, jboolean isWbxml, jstring jServerId)
{
    SYNCML_DM_RET_STATUS_T dm_ret_status;
    const char* szDmServerId = jenv->GetStringUTFChars(jServerId, NULL);

    jbyte* jBuf = jenv->GetByteArrayElements(jMsgBuf, NULL);
    jsize jBufSize = jenv->GetArrayLength(jMsgBuf);

    DmtPrincipal principal("DM_BOOTSTRAP");
    DMString strServerId(szDmServerId);
    dm_ret_status = DmtTreeFactory::Bootstrap(
                    principal, (const UINT8*)jBuf, jBufSize, isWbxml, true, strServerId);

    LOGD("processBootstrapScript dm_ret_status: %d", static_cast<jint>(dm_ret_status));

    return static_cast<jint>(dm_ret_status);
}


JNIEXPORT jbyteArray JNICALL processScript(JNIEnv* jenv, jclass, jstring jServerId,
        jstring jFileName, jboolean jIsBinary, jint /*jRetCode*/, jobject jdmobj)
{
  LOGV("In native processScript\n");
  g_sessionObj = jdmobj;

  jbyteArray jResult = NULL;
  SYNCML_DM_RET_STATUS_T ret_status;

  const char* szDmServerId = jenv->GetStringUTFChars(jServerId, NULL);

  if (szDmServerId == NULL) {
    ret_status = SYNCML_DM_DEVICE_FULL;
    SET_RET_STATUS_BUF;
    return jResult;
  }

  const char* szFileName = jenv->GetStringUTFChars(jFileName, NULL);
  if (szFileName == NULL) {
    jenv->ReleaseStringUTFChars(jServerId, szDmServerId);
    ret_status = SYNCML_DM_DEVICE_FULL;
    SET_RET_STATUS_BUF;
    return jResult;
  }

  LOGV("native processScript reading file <%s>\n", szFileName);

  FILE *fd = fopen(szFileName, "r");
  if (!fd) {
    LOGV("native processScript can't open file %s", szFileName);
    ret_status = SYNCML_DM_FILE_NOT_FOUND;
    SET_RET_STATUS_BUF;
    return jResult;
  }

  // assume 100k is enough
  const int c_nSize = 100 * 1024;
  char* szBuf = new char[c_nSize];

  if (szBuf == NULL) {
    ret_status = SYNCML_DM_DEVICE_FULL;
    SET_RET_STATUS_BUF;
    return jResult;
  }

  int buf_size = fread(szBuf, 1, c_nSize, fd );
  LOGE("native processScript read %d bytes, jIsBinary=%d\n", buf_size, jIsBinary);


  if (buf_size > 0) {
    DmtPrincipal principal(szDmServerId);
    DMVector<UINT8> bResult;

    ret_status = DmtTreeFactory::ProcessScript(principal, (const UINT8*)szBuf, buf_size, jIsBinary, bResult);

    // copy bResult to jResult
    int resultSize = bResult.size();

    if (resultSize > 0) {
        //Dump((const char*)&bResult.front(), resultSize, jIsBinary);
        Dump((const char*)bResult.get_data(), resultSize, jIsBinary);

        jResult = jenv->NewByteArray(resultSize);

        //jenv->SetByteArrayRegion(jResult, 0, resultSize, (const jbyte*)&bResult.front());
        jenv->SetByteArrayRegion(jResult, 0, resultSize, (const jbyte*)bResult.get_data());
    }
    else {
        SET_RET_STATUS_BUF;
    }
  }
  else {
    // read 0 bytes from script file
    ret_status = SYNCML_DM_IO_FAILURE;
    SET_RET_STATUS_BUF;
  }

  // release memory allocated from GetStringUTFChars
  jenv->ReleaseStringUTFChars(jServerId, szDmServerId);
  jenv->ReleaseStringUTFChars(jFileName, szFileName);


  LOGV("Native processScript return code %d\n", static_cast<jint>(ret_status));
  g_sessionObj = NULL;

  return jResult;
}


static JNINativeMethod gMethods[] = {
    {"initialize", "()I", (void*)initialize},
    {"destroy", "()I", (void*)destroy},
    {"parsePkg0", "([BLcom/android/omadm/service/DMPkg0Notification;)I", (void*)parsePkg0},
    {"startFotaClientSession",
        "(Ljava/lang/String;Ljava/lang/String;Lcom/android/omadm/service/DMSession;)I",
        (void*)startFotaClientSession},
    {"startFotaServerSession", "(Ljava/lang/String;ILcom/android/omadm/service/DMSession;)I",
        (void*)startFotaServerSession},
    {"startClientSession", "(Ljava/lang/String;Lcom/android/omadm/service/DMSession;)I",
        (void*)startClientSession},

    {"startFotaNotifySession",
        "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lcom/android/omadm/service/DMSession;)I",
        (void*)startFotaNotifySession},

    {"cancelSession", "()I", (void*)cancelSession},

    {"processScript",
        "(Ljava/lang/String;Ljava/lang/String;ZILcom/android/omadm/service/DMSession;)[B",
        (void*)processScript},
};

int registerNatives(JNIEnv* env)
{
    jclass clazz = env->FindClass(javaDMEnginePackage);
    if (clazz == NULL)
        return JNI_FALSE;

    if (env->RegisterNatives(clazz, gMethods, sizeof(gMethods)/sizeof(gMethods[0])) < 0) {
        LOGE("registerNatives return ERROR");
        return JNI_FALSE;
    }

    registerDMTreeNatives(env);
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* /*vm*/, void* /*reserved*/)
{
    LOGD("In JNI_OnLoad");
    JNIEnv* env = android::AndroidRuntime::getJNIEnv();

    if (env == NULL) {
        LOGE("Get Environment Error");
        return -1;
    }

    return (registerNatives(env) == JNI_TRUE) ? JNI_VERSION_1_6 : -1;
}
