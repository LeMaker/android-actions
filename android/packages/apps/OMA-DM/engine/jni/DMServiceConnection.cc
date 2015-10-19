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

#ifdef PLATFORM_ANDROID

#include "dm_tpt_connection.H"
#include "dm_tpt_utils.h"
#include "DMServiceMain.h"
#include <android_runtime/AndroidRuntime.h>

SYNCML_DM_OTAConnection::SYNCML_DM_OTAConnection() : m_maxAcptSize(0), m_szURL()
{
    JNIEnv* jEnv = NULL;

    if (android::AndroidRuntime::getJavaVM()) {
        jEnv = android::AndroidRuntime::getJNIEnv();
        m_jNetConnObj = getNetConnector();
    } else {
        return;
    }

    jclass jNetConnCls = jEnv->GetObjectClass(m_jNetConnObj);
    if (jNetConnCls == NULL) {
        LOGD(("FindClass return Error"));
        goto end;
    }

    m_jSendRequest = jEnv->GetMethodID(jNetConnCls, "sendRequest",
            "(Ljava/lang/String;[BLjava/lang/String;)I");
    if (m_jSendRequest == NULL) {
        LOGD(("GetMethod 'sendRequest' return Error"));
        goto end;
    }

    m_jGetRespLength = jEnv->GetMethodID(jNetConnCls, "getResponseLength", "()J");
    if (m_jGetRespLength == NULL) {
        LOGD(("GetMethod 'getResponseLength' return Error"));
        goto end;
    }

    m_jGetRespData = jEnv->GetMethodID(jNetConnCls, "getResponseData", "()[B");
    if (m_jGetRespData == NULL) {
        LOGD(("GetMethod 'getResponseData' return Error"));
        goto end;
    }

    m_jSetContentType = jEnv->GetMethodID(jNetConnCls, "setContentType", "(Ljava/lang/String;)V");
    if (m_jSetContentType == NULL) {
        LOGD(("GetMethod 'setContentType' return Error"));
        goto end;
    }

    m_jEnbleApnByName = jEnv->GetMethodID(jNetConnCls, "enableApnByName", "(Ljava/lang/String;)V");
    if (m_jEnbleApnByName == NULL) {
        LOGD(("GetMethod 'enableApnByName' return Error"));
        goto end;
    }

    LOGD("constructed successfully");
end:
    return;
}

SYNCML_DM_OTAConnection::~SYNCML_DM_OTAConnection()
{
    LOGD("~SYNCML_DM_OTAConnection()");
}

SYNCML_DM_RET_STATUS_T
SYNCML_DM_OTAConnection::Init(UINT32 dwMaxAcptSize, XPL_ADDR_TYPE_T AddressType,
        CPCHAR ConRef)
{
    LOGD("dwMaxAcptSize=%d, AddressType=%d", dwMaxAcptSize, AddressType);

    if (ConRef != NULL) {
        LOGD("ConRef=%s", ConRef);
        JNIEnv* jEnv = android::AndroidRuntime::getJNIEnv();
        jstring jConRef = jEnv->NewStringUTF(ConRef);
        jEnv->CallVoidMethod(m_jNetConnObj, m_jEnbleApnByName, jConRef);
    }

    m_maxAcptSize = dwMaxAcptSize;

    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T
SYNCML_DM_OTAConnection::Send(const SYNCML_DM_INDIRECT_BUFFER_T* psSendSyncMLDocument,
        SYNCML_DM_INDIRECT_BUFFER_T* psRecvSyncMLDocument, const UINT8* pbContType,
        const DMCredHeaders* psCredHdr)
{
    LOGD("Send=%d", psSendSyncMLDocument->dataSize);

    if ((psSendSyncMLDocument == NULL) ||
            (psSendSyncMLDocument->pData == NULL) ||
            (psSendSyncMLDocument->dataSize == 0 ))
    {
        return SYNCML_DM_FAIL;
    }

    if ((psRecvSyncMLDocument == NULL) ||
            (psRecvSyncMLDocument->pData == NULL))
    {
        return SYNCML_DM_FAIL;
    }

    if ((pbContType == NULL) || (pbContType[0] == '\0'))
    {
        return SYNCML_DM_FAIL;
    }

    // Check whether psCredHdr is valid
    if (!psCredHdr->isCorrect())
        return SYNCML_DM_FAIL;

    JNIEnv* jEnv = android::AndroidRuntime::getJNIEnv();

    jstring jContentType = jEnv->NewStringUTF((const char*)pbContType);
    jEnv->CallVoidMethod(m_jNetConnObj, m_jSetContentType, jContentType);

    CPCHAR strUrl = m_szURL.c_str();
    LOGD("url=%s", strUrl);
    jstring jurl = jEnv->NewStringUTF(strUrl);
    jbyteArray jDataArray = jEnv->NewByteArray(psSendSyncMLDocument->dataSize);
    jEnv->SetByteArrayRegion(jDataArray,
            0, psSendSyncMLDocument->dataSize,
            (const jbyte*)psSendSyncMLDocument->pData);

    int wNumRetries = 0;

    jstring jstrMac = NULL;
    if (psCredHdr->empty() == FALSE) {
        DMString strHMAC;
        strHMAC += "algorithm=MD5,username=\"";
        strHMAC += reinterpret_cast<CPCHAR>(psCredHdr->m_oUserName.getBuffer());
        strHMAC += "\",mac=";
        strHMAC += reinterpret_cast<CPCHAR>(psCredHdr->m_oMac.getBuffer());
        LOGD("mac length=%d", psCredHdr->m_oMac.getSize());
        //LOGD("mac value in hex:%x", psCredHdr->m_oMac.getBuffer());
        LOGD("hmac value=%s", strHMAC.c_str());
        jstrMac = jEnv->NewStringUTF(strHMAC.c_str());
    }

    SYNCML_DM_RET_STATUS_T jResult = SYNCML_DM_FAIL;

    while (wNumRetries < DMTPT_MAX_RETRIES)
    {
        jResult = static_cast<SYNCML_DM_RET_STATUS_T>(jEnv->CallIntMethod(
                m_jNetConnObj, m_jSendRequest, jurl, jDataArray, jstrMac /*hmac*/));

        LOGD("Send result=%d", static_cast<int>(jResult));

        // retry for timeout or general connection errors
        if (jResult == SYNCML_DM_SOCKET_TIMEOUT
                || jResult == SYNCML_DM_SOCKET_CONNECT_ERR
                || jResult == SYNCML_DM_UNKNOWN_HOST
                || jResult == SYNCML_DM_NO_HTTP_RESPONSE
                || jResult == SYNCML_DM_REQUEST_TIMEOUT
                || jResult == SYNCML_DM_INTERRUPTED
                || jResult == SYNCML_DM_SERVICE_UNAVAILABLE
                || jResult == SYNCML_DM_GATEWAY_TIMEOUT)
        {
            wNumRetries++;

            // FIXME: thread blocks here on sleep for 10 seconds
            ::sleep(15);  // sleep a little bit before trying again
            continue;
        }

        if (static_cast<int>(jResult) == 200) {
            jlong jResponseLen = jEnv->CallLongMethod(m_jNetConnObj, m_jGetRespLength);
            LOGD("response length=%lld", jResponseLen);
            if(jResponseLen > 0 && jResponseLen <= m_maxAcptSize){
                jbyteArray jData = (jbyteArray)jEnv->CallObjectMethod(m_jNetConnObj, m_jGetRespData);
                jEnv->GetByteArrayRegion(jData, 0, jResponseLen, (jbyte*)psRecvSyncMLDocument->pData);
                psRecvSyncMLDocument->dataSize = jResponseLen;
                //Get header:x-syncml-hmac
                m_pCredHeaders = (DMCredHeaders*)psCredHdr;
                m_pCredHeaders->clear();
                jstring jstrHMAC = jEnv->NewStringUTF("x-syncml-hmac");
                jobject jobjHMACValue = NULL;
                jclass jNetConnCls = jEnv->GetObjectClass(m_jNetConnObj);
                jmethodID jmethodGetHeader = jEnv->GetMethodID(jNetConnCls, "getResponseHeader","(Ljava/lang/String;)Ljava/lang/String;");
                jobjHMACValue = jEnv->CallObjectMethod(m_jNetConnObj, jmethodGetHeader, jstrHMAC);
                if(jobjHMACValue != NULL)
                {
                    LOGD("Get hmac header successfully!");
                    const char * strHMACValue = jEnv->GetStringUTFChars(static_cast<jstring>(jobjHMACValue), NULL);
                    LOGD("hmac value=%s", strHMACValue);
                    if(strHMACValue != NULL && strlen(strHMACValue) > 0)
                    {
                        ProcessCredHeaders(strHMACValue);
                    }
                    LOGD("Finish process hmac header!");
                    LOGD("m_pCredHeaders: algorithm:%s", ((CPCHAR)m_pCredHeaders->m_oAlgorithm.getBuffer()));
                    LOGD("m_pCredHeaders: username:%s", ((CPCHAR)m_pCredHeaders->m_oUserName.getBuffer()));
                    LOGD("m_oRecvCredHeaders: mac:%s", ((CPCHAR)m_pCredHeaders->m_oMac.getBuffer()));

                    jEnv->ReleaseStringUTFChars(static_cast<jstring>(jobjHMACValue), strHMACValue);
                }
                LOGD("Return OK");
                return SYNCML_DM_SUCCESS;
            }
            LOGD("Too much data was received!");
            return SYNCML_DM_FAIL;
         } else {
            LOGD("Not retryable network error!");
            break;
        }
    }

    LOGD("Server or Net issue. return code=%d", static_cast<int>(jResult));
    return jResult;
}

SYNCML_DM_RET_STATUS_T SYNCML_DM_OTAConnection::SetURI(CPCHAR szURL)
{
    LOGD("szURL=%s", szURL);
    m_szURL = szURL;

    return SYNCML_DM_SUCCESS;
}

//==============================================================================
// FUNCTION: SYNCML_DM_OTAConnection::ProcessCredHeaders
//
// DESCRIPTION: This method extracts the Credential headers from the
//               Response headers
//
// ARGUMENTS PASSED:    the HMAC string
// RETURN VALUE:
//                  SYNCML_DM_SUCCESS on success
//                  SYNCML_DM_FAIL on any failure
//
// IMPORTANT NOTES: The HandleOTARedirect method calls this method.
//==============================================================================
SYNCML_DM_RET_STATUS_T
SYNCML_DM_OTAConnection::ProcessCredHeaders(CPCHAR origHmacStr)
{
    LOGD(("Enter SYNCML_DM_OTAConnection::ProcessCredHeaders"));

    if (origHmacStr == NULL)
        return SYNCML_DM_FAIL;

    // Trim the blank space and tabs
    DMString hmacString;
    size_t origHmacStrLen = ::strlen(origHmacStr);
    for (size_t i = 0; i < origHmacStrLen; ++i)
    {
        char c = origHmacStr[i];
        if (c != ' ' && c != '\t')
            hmacString += c;
    }

    // make R/W copy of hmac string and clear the C++ string.
    // TODO: convert this logic to use C++ strings.
    char* initialHmacString = new char[hmacString.length() + 1];
    memcpy(initialHmacString, hmacString.c_str(), (hmacString.length() + 1));
    hmacString.clear();

    UINT8* pbParam = NULL;
    UINT8* pbValue = NULL;
    char*  pbAlgo  = NULL;
    char*  pbUname = NULL;
    char*  pbMAC   = NULL;

    UINT8* pbHmacString = reinterpret_cast<UINT8*>(DmStrstr(initialHmacString, "algorithm"));

    if (pbHmacString == NULL)
        pbHmacString = reinterpret_cast<UINT8*>(DmStrstr(initialHmacString, "username"));

    // Extract the algorithm, Username and mac from the x-syncml-hmac header
    while (pbHmacString != NULL)
    {
        pbHmacString = DM_TPT_splitParamValue(pbHmacString, &pbParam, &pbValue);

        if ((pbParam != NULL) && (pbParam[0] != '\0'))
        {
            if (!strcmp((CPCHAR)pbParam, "algorithm"))
            {
                pbAlgo = reinterpret_cast<char*>(pbValue);
            }
            else if (!strcmp((CPCHAR)pbParam, "username"))
            {
                pbUname = reinterpret_cast<char*>(pbValue);
            }
            else if (!strcmp((CPCHAR)pbParam, "mac"))
            {
                pbMAC = reinterpret_cast<char*>(pbValue);
            }
        }
    }

    // Allocate memory to hold username, mac, algorithm
    if (pbUname == NULL || pbMAC == NULL)
    {
        delete[] initialHmacString;
        return SYNCML_DM_FAIL;
    }

    m_pCredHeaders->m_oAlgorithm.assign((pbAlgo != NULL) ? pbAlgo : "MD5");
    m_pCredHeaders->m_oUserName.assign(pbUname);
    m_pCredHeaders->m_oMac.assign(pbMAC);

    delete[] initialHmacString;

    LOGD(("Leave SYNCML_DM_OTAConnection::ProcessCredHeaders"));
    return SYNCML_DM_SUCCESS;
}

#endif
