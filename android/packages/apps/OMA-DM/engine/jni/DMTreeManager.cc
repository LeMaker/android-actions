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

#include <android_runtime/AndroidRuntime.h>
#include "utils/Log.h"
#include "DMServiceMain.h"
#include "dmt.hpp"
#include <stdarg.h>
#include <dmMemory.h>

extern "C" {
#include "xltdec.h"
}

static const int RESULT_BUF_SIZE = 8192; /*2048*/

// FIXME: get rid of these static variables!
static PDmtTree ptrTree = NULL;
static DMString s_strRootPath;
static DmtPrincipal principal("localhost");
static bool bShowTimestamp = false;

static PDmtTree GetTree();
static SYNCML_DM_RET_STATUS_T PrintNode(PDmtNode ptrNode);
static void DumpSubTree(PDmtNode ptrNode);

static char resultBuf[RESULT_BUF_SIZE];
static void strcatEx(const char * format, ...)
{
    if (!format) {
        return;
    }

    int len = strlen(resultBuf);
    if (len < RESULT_BUF_SIZE - 1) {
        va_list args;
        va_start(args, format);
        int ret = vsnprintf(&resultBuf[len], RESULT_BUF_SIZE - len - 1, format, args);
        if (ret == -1) {
            resultBuf[RESULT_BUF_SIZE - 1] = 0x0;
        }
        va_end(args);
    }
}

static PDmtNode GetNode(const DMString& strNodeName)
{
    PDmtNode ptrNode;
    GetTree();

    if (ptrTree) {
        if (ptrTree->GetNode(strNodeName, ptrNode) != SYNCML_DM_SUCCESS) {
            strcatEx("can't get node %s", strNodeName.c_str());
        }
    }

    return ptrNode;
}

JNIEXPORT jstring JNICALL setStringNode(JNIEnv* jenv, jclass, jstring nodePath, jstring value)
{
    resultBuf[0] = 0x0;

    const char* szNodePath = jenv->GetStringUTFChars(nodePath, NULL);
    const char* szValue = jenv->GetStringUTFChars(value, NULL);

    DMString strNodePath(szNodePath);
    DMString strValue(szValue);

    jenv->ReleaseStringUTFChars(nodePath, szNodePath);
    jenv->ReleaseStringUTFChars(value, szValue);

    PDmtNode ptrNode = GetNode(strNodePath);
    if (!ptrNode) {
        goto end;
    }

    if (ptrNode->SetStringValue(strValue) == SYNCML_DM_SUCCESS) {
        strcatEx("set value of node %s to %s successfully\n", strNodePath.c_str(), strValue.c_str());
        PrintNode(ptrNode);
    } else {
        strcatEx("can't set value of node %s to %s", strNodePath.c_str(), strValue.c_str());
    }

end:
    jstring ret = jenv->NewStringUTF(resultBuf);
    return ret;
}

JNIEXPORT jstring JNICALL getNodeInfo(JNIEnv* jenv, jclass, jstring jszNode)
{
    resultBuf[0] = 0x0;

    const char* szNode = jenv->GetStringUTFChars(jszNode, NULL);
    DMString strNode(szNode);
    jenv->ReleaseStringUTFChars(jszNode, szNode);

    PDmtNode ptrNode = GetNode(strNode);
    if (ptrNode) {
        PrintNode(ptrNode);
    }

    jstring ret = jenv->NewStringUTF(resultBuf);
    return ret;
}

JNIEXPORT jint JNICALL getNodeType(JNIEnv* jenv, jclass, jstring jszNode)
{
    const char* szNode = jenv->GetStringUTFChars(jszNode, NULL);
    DMString strNode(szNode);
    jenv->ReleaseStringUTFChars(jszNode, szNode);

    PDmtNode ptrNode = GetNode(strNode);
    if (ptrNode && !ptrNode->IsExternalStorageNode())
    {
        DmtData oData;
        LOGD("Enter get value...\n");
        SYNCML_DM_RET_STATUS_T ret = ptrNode->GetValue(oData);
        if (ret != SYNCML_DM_SUCCESS) {
            LOGD("Value is null");
            return 0;   // return NULL type on error
        }
        return oData.GetType();
    }

    return 0;   // return NULL type on error
}

JNIEXPORT jstring JNICALL getNodeValue(JNIEnv* jenv, jclass, jstring jszNode)
{
    const char* szNode = jenv->GetStringUTFChars(jszNode, NULL);
    DMString strNode(szNode);
    jenv->ReleaseStringUTFChars(jszNode, szNode);

    PDmtNode ptrNode = GetNode(strNode);
    if (ptrNode && !ptrNode->IsExternalStorageNode())
    {
        LOGD("Enter get value...\n");
        DmtData oData;
        DMString value;

        if (!ptrNode->IsLeaf()) {
            SYNCML_DM_RET_STATUS_T ret = ptrNode->GetValue(oData);
            if (ret != SYNCML_DM_SUCCESS) {
                LOGE("can't get child nodes");
                return NULL;    // return NULL reference on error
            }
            DMStringVector aChildren;
            ret = oData.GetNodeValue(aChildren);
            if (ret != SYNCML_DM_SUCCESS) {
                LOGE("oData.getNodeValue() failed");
                return NULL;    // return NULL reference on error
            }
            UINT32 childLength = aChildren.size();
            for (UINT32 i = 0; i < childLength; ++i) {
                if (i != 0) {
                    value += '|';
                }
                value += aChildren[i];
            }
        }
        else
        {
            SYNCML_DM_RET_STATUS_T ret = ptrNode->GetValue(oData);
            if (ret != SYNCML_DM_SUCCESS) {
                LOGE("Value is null");
                return NULL;   // return NULL reference on error
            }
            if (oData.GetString(value) != SYNCML_DM_SUCCESS) {
                LOGE("oData.GetString() failed");
                return NULL;   // return NULL reference on error
            }
        }

        return jenv->NewStringUTF(value.c_str());
    }

    return NULL;    // return NULL reference on error
}

JNIEXPORT jstring JNICALL executePlugin(JNIEnv* jenv, jclass, jstring jszNode, jstring jszData)
{
    resultBuf[0] = 0x0;

    const char* szNode = jenv->GetStringUTFChars(jszNode, NULL);
    DMString strNode(szNode);
    jenv->ReleaseStringUTFChars(jszNode, szNode);

    const char* szData = jenv->GetStringUTFChars(jszData, NULL);
    DMString strData(szData);
    jenv->ReleaseStringUTFChars(jszData, szData);

    PDmtNode ptrNode = GetNode(strNode);
    if (ptrNode) {
        DMString strResult;
        if (ptrNode->Execute(strData, strResult) == SYNCML_DM_SUCCESS) {
            strcatEx("execute node %s successfully, result=%s\n", strNode.c_str(), strResult.c_str());
        } else {
            strcatEx("can't execute node %s", strNode.c_str());
        }
    }

    jstring ret = jenv->NewStringUTF(resultBuf);
    return ret;
}

JNIEXPORT jstring JNICALL dumpTree(JNIEnv *jenv, jclass, jstring jszNode)
{
    resultBuf[0] = 0x0;

    const char* szNode = jenv->GetStringUTFChars(jszNode, NULL);
    DMString strNode(szNode);
    jenv->ReleaseStringUTFChars(jszNode, szNode);

    PDmtNode ptrNode = GetNode(strNode);
    if (ptrNode) {
        DumpSubTree(ptrNode);
    }

    jstring ret = jenv->NewStringUTF(resultBuf);
    return ret;
}

JNIEXPORT jint JNICALL createInterior(JNIEnv *jenv, jclass, jstring jszNode)
{
    GetTree();
    if (!ptrTree) {
        return static_cast<jint>(SYNCML_DM_FAIL);
    }

    const char* szNode = jenv->GetStringUTFChars(jszNode, NULL);
    DMString strNode(szNode);
    jenv->ReleaseStringUTFChars(jszNode, szNode);

    PDmtNode ptrNode;
    SYNCML_DM_RET_STATUS_T ret = ptrTree->CreateInteriorNode(strNode, ptrNode);
    if (ret == SYNCML_DM_SUCCESS) {
        LOGI("node %s created successfully\n", strNode.c_str());
    } else {
        LOGE("can't create node %s", strNode.c_str());
    }
    return static_cast<jint>(ret);
}

JNIEXPORT jint JNICALL createLeaf(JNIEnv *jenv, jclass, jstring jszNode, jstring jszData)
{
    if (jszNode == NULL) {
        return static_cast<jint>(SYNCML_DM_FAIL);
    }

    GetTree();
    if (!ptrTree) {
        return static_cast<jint>(SYNCML_DM_FAIL);
    }

    const char* szNode = jenv->GetStringUTFChars(jszNode, NULL);
    const char* szData = NULL;

    if (jszData != NULL) {
        szData = jenv->GetStringUTFChars(jszData, NULL);
    }

    PDmtNode ptrNode;
    SYNCML_DM_RET_STATUS_T ret = ptrTree->CreateLeafNode(szNode, ptrNode, DmtData(szData));
    if (ret == SYNCML_DM_SUCCESS) {
        LOGI("node %s (%s) created successfully\n", szNode, szData);
    } else {
        LOGE("can't create node %s", szNode);
    }

    jenv->ReleaseStringUTFChars(jszNode, szNode);
    jenv->ReleaseStringUTFChars(jszData, szData);

    return static_cast<jint>(ret);
}


JNIEXPORT jint JNICALL createLeafByte(JNIEnv *jenv, jclass clz, jstring jszNode,
        jbyteArray bDataArray)
{
    const char* szNode = jenv->GetStringUTFChars(jszNode, NULL);
    jbyte* jData = (jbyte*)jenv->GetByteArrayElements(bDataArray, NULL);
    jsize arraySize = jenv->GetArrayLength(bDataArray);

    char* pData = (char*)DmAllocMem(arraySize+1);
    memcpy(pData, jData, arraySize);
    pData[arraySize] = '\0';

    PDmtNode ptrNode;
    GetTree();

    jenv->ReleaseByteArrayElements(bDataArray, jData, 0);

    if ( ptrTree == NULL ) {
        DmFreeMem(pData);
        return SYNCML_DM_FAIL;
    }

    LOGI("NodePath=%s,Byte Data=0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X\n", szNode, pData[0], pData[1],
            pData[2], pData[3], pData[4], pData[5], pData[6]);

    DMString strNode(szNode);

    //PDmtNode ptrNode;
    SYNCML_DM_RET_STATUS_T ret = ptrTree->CreateLeafNode(szNode, ptrNode, DmtData( pData ));
    if (ret == SYNCML_DM_SUCCESS) {
        LOGI("node %s created successfully\n", strNode.c_str());
    } else {
        LOGE("can't create node %s", strNode.c_str());
    }
    return static_cast<jint>(ret);
}

JNIEXPORT jint JNICALL deleteNode(JNIEnv *jenv, jclass, jstring jszNode)
{
    GetTree();
    if (!ptrTree) {
        return static_cast<jint>(SYNCML_DM_FAIL);
    }

    const char* szNode = jenv->GetStringUTFChars(jszNode, NULL);
    DMString strNode(szNode);
    jenv->ReleaseStringUTFChars(jszNode, szNode);

    SYNCML_DM_RET_STATUS_T ret = ptrTree->DeleteNode(strNode);
    if (ret == SYNCML_DM_SUCCESS) {
        LOGI("node %s deleted successfully\n", strNode.c_str());
    } else {
        LOGE("can't delete node %s", strNode.c_str());
    }
    return static_cast<jint>(ret);
}

static PDmtTree GetTree()
{
    if (ptrTree) return ptrTree;

    if (DmtTreeFactory::GetSubtree(principal, s_strRootPath, ptrTree) != SYNCML_DM_SUCCESS) {
        strcatEx("Can't get tree '%s'.", s_strRootPath.c_str());
    }

    return ptrTree;
}

static void DumpSubTree(PDmtNode ptrNode)
{
    SYNCML_DM_RET_STATUS_T ret = PrintNode(ptrNode);
    strcatEx("\n");
    if (ret != SYNCML_DM_SUCCESS) return;

    if (!ptrNode->IsLeaf()) {
        DMVector<PDmtNode> aChildren;
        ret = ptrNode->GetChildNodes(aChildren);
        if (ret != SYNCML_DM_SUCCESS) {
            DMString path;
            ptrNode->GetPath(path);
            strcatEx("can't get child nodes of %s", path.c_str());
            return;
        }
        UINT32 childLength = aChildren.size();
        for (UINT32 i = 0; i < childLength; ++i) {
            DumpSubTree(aChildren[i]);
        }
    }
}

static SYNCML_DM_RET_STATUS_T PrintNode(PDmtNode ptrNode)
{
    LOGD("Enter PrintNode\n");
    DmtAttributes oAttr;
    DMString path;

    SYNCML_DM_RET_STATUS_T ret = ptrNode->GetPath(path);
    if (ret != SYNCML_DM_SUCCESS)
    {
        strcatEx("can't get attributes of node %d", ret);
    }

    LOGD("Get attributes\n");
    if ((ret = ptrNode->GetAttributes(oAttr)) != ptrNode->GetPath(path)) {
        strcatEx("can't get attributes of node %s", path.c_str());
        return ret;
    }

    LOGD("Check storage mode...\n");
    DmtData oData;
    if (!ptrNode->IsExternalStorageNode())
    {
        LOGD("Enter get value...\n");
        SYNCML_DM_RET_STATUS_T ret1 = ptrNode->GetValue(oData);
        if (ret1 != SYNCML_DM_SUCCESS) {
            LOGD("Value is null");
            strcatEx("can't get value of node %s", path.c_str());
            return ret1;
        }
    }

    LOGD("Compose string begin...\n");
    strcatEx("path=%s\n", (const char*)path.c_str());
    strcatEx("isLeaf=%s\n", (ptrNode->IsLeaf()?"true":"false") );
    strcatEx("name=%s\n", (const char*)oAttr.GetName().c_str() );
    strcatEx("format=%s\n", (const char*)oAttr.GetFormat().c_str() );
    strcatEx("type=%s\n", (const char*)oAttr.GetType().c_str() );
    strcatEx("title=%s\n", (const char*)oAttr.GetTitle().c_str() );
    strcatEx("acl=%s\n", (const char*)oAttr.GetAcl().toString().c_str() );
    strcatEx("size=%d\n", oAttr.GetSize());
    if (bShowTimestamp) {
        time_t timestamp = (time_t)(oAttr.GetTimestamp()/1000L);
        if (timestamp == 0) {
            strcatEx("timestamp=(Unknown)\n");
        } else {
            char timestampbuf[27];
            ctime_r(&timestamp, timestampbuf);
            strcatEx("timestamp=%s", timestampbuf);
        }
    }

    strcatEx("version=%d\n", oAttr.GetVersion() );
    if ( !ptrNode->IsLeaf() ) {
        DMStringVector aChildren;
        oData.GetNodeValue(aChildren);
        strcatEx("children:");
        if ( aChildren.size() == 0 ) {
            strcatEx("null");
        }
        UINT32 childLength = aChildren.size();
        for (UINT32 i = 0; i < childLength; ++i) {
            const DMString& child = aChildren[i];
            strcatEx("%s/", child.c_str());
        }
        strcatEx("\n");
    } else {
        if (ptrNode->IsExternalStorageNode())
        {
            strcatEx("value=\n");
            strcatEx("It is an ESN node, not supported now");
            //displayESN(ptrNode);
        }
        else {
            if (oAttr.GetFormat() == "bin") {
                strcatEx("Binary value: [");
                const DMVector<UINT8>& val = oData.GetBinaryValue();
                UINT32 valLength = val.size();
                for (UINT32 i = 0; i < valLength; ++i) {
                    UINT8 byte = val[i];
                    strcatEx("%02x ", byte);
                }
                strcatEx("]\n");
            }
            else
            {
                DMString s;
                oData.GetString(s);
                strcatEx("value=%s\n", s.c_str());
            }
        }
    }
    return SYNCML_DM_SUCCESS;
}

short wbxml2xml(unsigned char *bufIn, int bufInLen, unsigned char *bufOut, int * bufOutLen)
{
    short ret = 0;
#ifdef __SML_WBXML__
    ret = wbxml2xmlInternal(bufIn, bufInLen, bufOut,bufOutLen);
#endif
    return ret;
}

JNIEXPORT jbyteArray JNICALL ConvertWbxml2Xml(JNIEnv* env, jclass, jbyteArray bArray)
{
    unsigned char* xmlBuf = NULL;
    int xmlLen = 0;

    jbyte* wbxmlBuf = env->GetByteArrayElements(bArray, NULL);
    jsize  wbxmlLen = env->GetArrayLength(bArray);
    LOGD("ConvertWbxml2Xml: wbxml length = %d\n", wbxmlLen);

    if (wbxmlBuf == NULL || wbxmlLen <= 0)
    {
        LOGD("ConvertWbxml2Xml: nothing to convert\n");
        return NULL;
    }

    xmlLen = wbxmlLen * 6;
    xmlBuf = new unsigned char[xmlLen];
    if (xmlBuf == NULL)
    {
        LOGE("ConvertWbxml2Xml: failed to allocate memory\n");
        return NULL;
    }
    LOGD("ConvertWbxml2Xml: allocated xml length = %d\n", xmlLen);

#ifdef __SML_WBXML__
    short ret = wbxml2xmlInternal((unsigned char*)wbxmlBuf, wbxmlLen, xmlBuf, &xmlLen);
#else
    short ret = -1;
#endif

    if (ret != 0) {
        LOGE("ConvertWbxml2Xml: wbxml2xml failed: %d\n", ret);
        delete [] xmlBuf;
        return NULL;
    }

    jbyteArray jb = env->NewByteArray(xmlLen);
    env->SetByteArrayRegion(jb, 0, xmlLen, (jbyte*)xmlBuf);
    LOGD("ConvertWbxml2Xml: result xml length = %d\n", xmlLen);
    delete [] xmlBuf;
    return jb;
}

static JNINativeMethod gMethods[] = {
    {"setStringNode",
        "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
        (void*)setStringNode},
    {"getNodeInfo",
        "(Ljava/lang/String;)Ljava/lang/String;",
        (void*)getNodeInfo},
    {"executePlugin",
        "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
        (void*)executePlugin},
    {"dumpTree",
        "(Ljava/lang/String;)Ljava/lang/String;",
        (void*)dumpTree},
    {"createInterior",
        "(Ljava/lang/String;)I",
        (void*)createInterior},
    {"createLeaf",
        "(Ljava/lang/String;Ljava/lang/String;)I",
        (void*)createLeaf},
    {"createLeaf",
        "(Ljava/lang/String;[B)I",
        (void*)createLeafByte},
    {"deleteNode",
        "(Ljava/lang/String;)I",
        (void*)deleteNode},
    {"nativeWbxmlToXml",
        "([B)[B",
        (void*)ConvertWbxml2Xml},
    {"getNodeType",
        "(Ljava/lang/String;)I",
        (void*)getNodeType},
    {"getNodeValue",
        "(Ljava/lang/String;)Ljava/lang/String;",
        (void*)getNodeValue},
};

int registerDMTreeNatives(JNIEnv *env)
{
    jclass clazz = env->FindClass(javaDMEnginePackage);
    if (clazz == NULL)
        return JNI_FALSE;

    if (env->RegisterNatives(clazz, gMethods, sizeof(gMethods)/sizeof(gMethods[0])) < 0)
    {
        LOGE("registerDMTreeNatives return ERROR");
        return JNI_FALSE;
    }

    return JNI_TRUE;
}
