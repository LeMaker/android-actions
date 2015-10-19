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

#include "DmtJavaPluginManager.h"
#include "DmtJavaPluginNode.h"
#include "DmtJavaPluginCommon.h"
#include "DmtJavaMethod.h"

#include <android_runtime/AndroidRuntime.h>

#include <stdlib.h>

#define DECLARE_METHOD(method, methodName, methodParam)                                     \
    DmtJavaMethod method(mEnv, mJavaPluginManager, (methodName), (methodParam));            \
    if (!method.isFound())                                                                  \
    {                                                                                       \
        DmtJavaPlugin_Debug("Failed to get %s() method\n", (methodName));                   \
        return SYNCML_DM_FAIL;                                                              \
    }

#define DECLARE_STRING(obj, str)                                                            \
    DmtJString obj(mEnv, mEnv->NewStringUTF(str));                                          \
    if (!obj)                                                                               \
    {                                                                                       \
        DmtJavaPlugin_Debug("Failed to create jstring %s\n", (!str ? "" : str));            \
        return SYNCML_DM_FAIL;                                                              \
    }

#define CHECK_EXCEPTION(retcode)                                                            \
    if (mEnv->ExceptionCheck())                                                             \
    {                                                                                       \
        DmtJavaPlugin_Debug("An exception is thrown\n");                                    \
        (retcode) = SYNCML_DM_FAIL;                                                         \
    }

DmtJavaPluginManager::DmtJavaPluginManager(const char* pPath, DMStringMap& mapParameters)
    : mEnv(NULL), mJavaPluginManager(mEnv), mIsInitialized(false)
{
    if (!InitJNIEnv()                       ||
        !InitJavaPluginManager()            ||
        !InitJavaPlugin(pPath, mapParameters))
    {
        DmtJavaPlugin_Debug("Fail to init DmtJavaPluginManager!\n");
        return;
    }

    mIsInitialized = true;
}

DmtJavaPluginManager::~DmtJavaPluginManager()
{
    if (InitJNIEnv() && mEnv != NULL && mJavaPluginManager != NULL)
    {
        ReleasePluginManager();
    }
}

bool DmtJavaPluginManager::InitJNIEnv()
{
    JavaVM* jvm = android::AndroidRuntime::getJavaVM();
    if (jvm != NULL)
    {
        return jvm->GetEnv((void**)&mEnv, JNI_VERSION_1_6) == JNI_OK;
    }

    JavaVMOption options[1];
    memset(&options, 0, sizeof(options));

    options[0].optionString = "-Djava.class.path=/system/framework/com.android.omadm.plugin.jar:/system/framework/com.android.omadm.plugin.dev.jar";

    JavaVMInitArgs initArgs;
    memset(&initArgs, 0, sizeof(initArgs));

    initArgs.version  = JNI_VERSION_1_6;
    initArgs.options  = options;
    initArgs.nOptions = 1;
    initArgs.ignoreUnrecognized = JNI_TRUE;

    DmtJavaPlugin_Debug("Create new JVM\n");
    return JNI_CreateJavaVM(&jvm, &mEnv, &initArgs) == JNI_OK;
}

bool DmtJavaPluginManager::InitJavaPluginManager()
{
    DmtJavaMethod constructor(mEnv,"com/android/omadm/plugin/impl/DmtPluginManager", "<init>", "()V");
    if (!constructor.isFound())
    {
        return false;
    }

    DmtJObject localPluginManager(mEnv, mEnv->NewObject(constructor.getClass(), constructor.getMethodID()));
    if (!localPluginManager)
    {
        DmtJavaPlugin_Debug("Failed to create new object for DmtPluginManager!\n");
        return false;
    }

    mJavaPluginManager.assignValue(localPluginManager.getValue(), mEnv);

    return (mJavaPluginManager != NULL);
}

bool DmtJavaPluginManager::InitJavaPlugin(const char* pPath, DMStringMap& mapParameters)
{
    if (pPath == NULL)
    {
        DmtJavaPlugin_Debug("Path is null\n");
        return false;
    }

    DmtJavaMethod javaMethod(mEnv, mJavaPluginManager, "initJavaPlugin", "(Ljava/lang/String;[Ljava/lang/String;)Z");
    if (!javaMethod.isFound())
    {
        DmtJavaPlugin_Debug("Failed to get loadJavaPlugin()\n");
        return false;
    }

    DmtJString objPath(mEnv, mEnv->NewStringUTF(pPath));
    DmtJClass  classString(mEnv, mEnv->FindClass("java/lang/String"));
    DmtJObjectArray objStrArray(mEnv, mEnv->NewObjectArray(mapParameters.size() * 2, classString, NULL));
    if (!objPath || !classString || !objStrArray)
    {
        DmtJavaPlugin_Debug("Failed to create jstring or jobjectArray\n");
        return false;
    }

    int i = 0;
    for (DMStringMap::POS it = mapParameters.begin(); it != mapParameters.end(); it++)
    {
        DmtJString name(mEnv,  mEnv->NewStringUTF(mapParameters.get_key(it).c_str()));
        DmtJString value(mEnv, mEnv->NewStringUTF(mapParameters.get_value(it).c_str()));
        if (!name || !value)
        {
            DmtJavaPlugin_Debug("Failed to create string for name or value\n");
            return false;
        }
        mEnv->SetObjectArrayElement(objStrArray, i    , name);
        mEnv->SetObjectArrayElement(objStrArray, i + 1, value);

        i += 2;
    }

    jboolean isInitialized = mEnv->CallBooleanMethod(mJavaPluginManager, javaMethod,
                                                     objPath.getValue(), objStrArray.getValue());
    return (!mEnv->ExceptionCheck() && isInitialized);
}

void DmtJavaPluginManager::ReleasePluginManager()
{
    DmtJavaMethod javaMethod(mEnv, mJavaPluginManager, "release", "()V");
    if (!javaMethod.isFound()) {
        DmtJavaPlugin_Debug("Fail to get release() method\n");
        return;
    }

    mEnv->CallVoidMethod(mJavaPluginManager, javaMethod);
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginManager::GetDmtNodeValue(jint& type,
                                                             char*& pValue,
                                                             const DmtData& data)
{
    DMString dmStringValue;

    SYNCML_DM_RET_STATUS_T retcode = data.GetString(dmStringValue);
    if (retcode != SYNCML_DM_SUCCESS)
    {
        DmtJavaPlugin_Debug("Fail to get string value\n");
        return SYNCML_DM_FAIL;
    }

    const char* pStringValue = dmStringValue.c_str();

    size_t size = strlen(pStringValue) + 1;

    pValue = new char[size];
    if (!pValue) {
        DmtJavaPlugin_Debug("Fail to allocate memory\n");
        return SYNCML_DM_FAIL;
    }

    strcpy(pValue, pStringValue);

    switch (data.GetType())
    {
        case SYNCML_DM_DATAFORMAT_NULL:
            type = JAVA_SYNCML_DM_DATAFORMAT_NULL;
            break;

        case SYNCML_DM_DATAFORMAT_STRING:
            type = JAVA_SYNCML_DM_DATAFORMAT_STRING;
            break;

        case SYNCML_DM_DATAFORMAT_INT:
            type = JAVA_SYNCML_DM_DATAFORMAT_INT;
            break;

        case SYNCML_DM_DATAFORMAT_BOOL:
            type = JAVA_SYNCML_DM_DATAFORMAT_BOOL;
            break;

        case SYNCML_DM_DATAFORMAT_BIN:
            type = JAVA_SYNCML_DM_DATAFORMAT_BIN;
            break;

        case SYNCML_DM_DATAFORMAT_DATE:
            type = JAVA_SYNCML_DM_DATAFORMAT_DATE;
            break;

        case SYNCML_DM_DATAFORMAT_TIME:
            type = JAVA_SYNCML_DM_DATAFORMAT_TIME;
            break;

        case SYNCML_DM_DATAFORMAT_FLOAT:
            type = JAVA_SYNCML_DM_DATAFORMAT_FLOAT;
            break;

        default:
            DmtJavaPlugin_Debug("Unsupported DmtData type=%d!\n", data.GetType());
            delete[] pValue;
            pValue = NULL;
            return SYNCML_DM_FAIL;
    }

    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginManager::SetDmtNodeValue(const char* pType,
                                                             const char* pValue,
                                                             DmtData& data)
{
    if (pType == NULL)
    {
        DmtJavaPlugin_Debug("Failed to get value type\n");
        return SYNCML_DM_FAIL;
    }

    switch (atoi(pType))
    {
        case JAVA_SYNCML_DM_DATAFORMAT_NULL:
            data.SetString(NULL, SYNCML_DM_DATAFORMAT_NULL);
            break;

        case JAVA_SYNCML_DM_DATAFORMAT_STRING:
            data.SetString(pValue, SYNCML_DM_DATAFORMAT_STRING);
            break;

        case JAVA_SYNCML_DM_DATAFORMAT_INT:
            data.SetString(pValue, SYNCML_DM_DATAFORMAT_INT);
            break;

        case JAVA_SYNCML_DM_DATAFORMAT_BOOL:
            data.SetString(pValue, SYNCML_DM_DATAFORMAT_BOOL);
            break;

        case JAVA_SYNCML_DM_DATAFORMAT_BIN:
            data.SetBinary((UINT8*)pValue, strlen(pValue));
            break;

        case JAVA_SYNCML_DM_DATAFORMAT_DATE:
            data.SetString(pValue, SYNCML_DM_DATAFORMAT_DATE);
            break;

        case JAVA_SYNCML_DM_DATAFORMAT_TIME:
            data.SetString(pValue, SYNCML_DM_DATAFORMAT_TIME);
            break;

        case JAVA_SYNCML_DM_DATAFORMAT_FLOAT:
            data.SetString(pValue, SYNCML_DM_DATAFORMAT_FLOAT);
            break;

        case JAVA_SYNCML_DM_DATAFORMAT_NODE:
            {
                DMStringVector vec;
                if (pValue != NULL && *pValue != 0)
                {
                    const char* nodeNameBegin = pValue;
                    const char* nodeNameEnd = strchr(nodeNameBegin, '\n');
                    while (nodeNameEnd != NULL)
                    {
                        DMString dmNodeName(nodeNameBegin, nodeNameEnd - nodeNameBegin);
                        vec.push_back(dmNodeName);
                        nodeNameBegin = nodeNameEnd + 1;
                        nodeNameEnd = strchr(nodeNameBegin, '\n');
                    }
                }
                data.SetNodeValue(vec);
            }
            break;

        default:
            DmtJavaPlugin_Debug("Unsupported value type\n");
            return SYNCML_DM_FAIL;
    }
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginManager::SetServerID(PDmtTree pTree)
{
    if(!InitJNIEnv())
    {
        DmtJavaPlugin_Debug("Init JNI Env failed...\n");
        return SYNCML_DM_FAIL;
    }

    DECLARE_METHOD(javaMethod, "setServerID", "(Ljava/lang/String;)V");
    DECLARE_STRING(objServerID, pTree->GetPrincipal().getName().c_str());

    mEnv->CallVoidMethod(mJavaPluginManager, javaMethod, objServerID.getValue());
    return SYNCML_DM_SUCCESS;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginManager::SetNodeValue(const char* pPath, const DmtData& data)
{
    if(!InitJNIEnv())
    {
        DmtJavaPlugin_Debug("Init JNI Env failed...\n");
        return SYNCML_DM_FAIL;
    }

    DECLARE_METHOD(javaMethod, "setNodeValue", "(Ljava/lang/String;ILjava/lang/String;)I");

    jint type = 0;
    char* pValue = NULL;
    SYNCML_DM_RET_STATUS_T retcode = GetDmtNodeValue(type, pValue, data);
    if (retcode != SYNCML_DM_SUCCESS)
    {
        DmtJavaPlugin_Debug("Failed to get node value\n");
        return retcode;
    }

    DECLARE_STRING(objPath , pPath);
    DECLARE_STRING(objValue, (pValue == NULL ? "" : pValue));
    if (pValue == NULL) {
        delete[] pValue;
        pValue = NULL;
    }

    retcode = mEnv->CallIntMethod(mJavaPluginManager, javaMethod, objPath.getValue(), type, objValue.getValue());
    CHECK_EXCEPTION(retcode);
    return retcode;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginManager::GetNodeValue(const char* pPath, DmtData& data)
{
    if(!InitJNIEnv())
    {
        DmtJavaPlugin_Debug("Init JNI Env failed...\n");
        return SYNCML_DM_FAIL;
    }

    DECLARE_METHOD(javaMethod, "getNodeValue", "(Ljava/lang/String;)[Ljava/lang/String;");
    DECLARE_STRING(objPath, pPath);

    /* The getNodeValue return objStrArray with 2 elements.         */
    /* The first element is value type and the second is the value. */
    DmtJObjectArray objStrArray(mEnv);
    objStrArray = static_cast<jobjectArray>(mEnv->CallObjectMethod(mJavaPluginManager,
                                                                   javaMethod,
                                                                   (jstring)objPath));
    if (mEnv->ExceptionCheck() || !objStrArray)
    {
        DmtJavaPlugin_Debug("Got an error for getNodeValue()\n");
        return SYNCML_DM_FAIL;
    }

    if (mEnv->GetArrayLength(objStrArray) != 2)
    {
		if (mEnv->GetArrayLength(objStrArray) == 1) {
			DmtJObject objStatus(mEnv, mEnv->GetObjectArrayElement(objStrArray, 0));
			if (!objStatus)
				return SYNCML_DM_FAIL;
			const char* pStatus = mEnv->GetStringUTFChars((jstring)objStatus.getValue(), NULL);
			SYNCML_DM_RET_STATUS_T retcode = atoi(pStatus);
			return retcode;
		}
        DmtJavaPlugin_Debug("Array size is not 2!\n");
        return SYNCML_DM_FAIL;
    }

    DmtJObject objType(mEnv,  mEnv->GetObjectArrayElement(objStrArray, 0));
    DmtJObject objValue(mEnv, mEnv->GetObjectArrayElement(objStrArray, 1));
    if (!objType || !objValue)
    {
        DmtJavaPlugin_Debug("Failed to get value type or value object\n");
        return SYNCML_DM_FAIL;
    }

    const char* pType  = mEnv->GetStringUTFChars((jstring)objType.getValue(),  NULL);
    const char* pValue = mEnv->GetStringUTFChars((jstring)objValue.getValue(), NULL);

    return SetDmtNodeValue(pType, pValue, data);
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginManager::CreateInteriorNode(const char* pPath)
{
    if(!InitJNIEnv())
    {
        DmtJavaPlugin_Debug("Init JNI Env failed...\n");
        return SYNCML_DM_FAIL;
    }

    DECLARE_METHOD(javaMethod, "createInteriorNode", "(Ljava/lang/String;)I");
    DECLARE_STRING(objPath, pPath);

    SYNCML_DM_RET_STATUS_T retcode = mEnv->CallIntMethod(mJavaPluginManager, javaMethod, objPath.getValue());
    CHECK_EXCEPTION(retcode);
    return retcode;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginManager::CreateLeafNode(const char* pPath, const DmtData& data)
{
    if(!InitJNIEnv())
    {
        DmtJavaPlugin_Debug("Init JNI Env failed...\n");
        return SYNCML_DM_FAIL;
    }

    DECLARE_METHOD(javaMethod, "createLeafNode", "(Ljava/lang/String;ILjava/lang/String;)I");

    jint type = 0;
    char* pValue = NULL;
    SYNCML_DM_RET_STATUS_T retcode = GetDmtNodeValue(type, pValue, data);
    if (retcode != SYNCML_DM_SUCCESS)
    {
        DmtJavaPlugin_Debug("Failed to get node value\n");
        return retcode;
    }

    DECLARE_STRING(objPath,  pPath);
    DECLARE_STRING(objValue, (pValue == NULL ? "" : pValue));
    if (pValue != NULL) {
        delete[] pValue;
        pValue = NULL;
    }

    retcode = mEnv->CallIntMethod(mJavaPluginManager, javaMethod, objPath.getValue(), type, objValue.getValue());
    CHECK_EXCEPTION(retcode);
    return retcode;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginManager::RenameNode(const char* pPath, const char* pNewNodeName)
{
    if(!InitJNIEnv())
    {
        DmtJavaPlugin_Debug("Init JNI Env failed...\n");
        return SYNCML_DM_FAIL;
    }

    DECLARE_METHOD(javaMethod, "renameNode", "(Ljava/lang/String;Ljava/lang/String;)I");
    DECLARE_STRING(objPath, pPath);
    DECLARE_STRING(objNewNodeName, pNewNodeName);

    SYNCML_DM_RET_STATUS_T retcode = mEnv->CallIntMethod(mJavaPluginManager, javaMethod,
                                                         objPath.getValue(), objNewNodeName.getValue());
    CHECK_EXCEPTION(retcode);
    return retcode;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginManager::DeleteNode(const char* pPath)
{
    if(!InitJNIEnv())
    {
        DmtJavaPlugin_Debug("Init JNI Env failed...\n");
        return SYNCML_DM_FAIL;
    }

    DECLARE_METHOD(javaMethod, "deleteNode", "(Ljava/lang/String;)I");
    DECLARE_STRING(objPath, pPath);

    SYNCML_DM_RET_STATUS_T retcode = mEnv->CallIntMethod(mJavaPluginManager, javaMethod, objPath.getValue());
    CHECK_EXCEPTION(retcode);
    return retcode;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginManager::Commit()
{
    if(!InitJNIEnv())
    {
        DmtJavaPlugin_Debug("Init JNI Env failed...\n");
        return SYNCML_DM_FAIL;
    }

    DECLARE_METHOD(javaMethod, "commit", "()I");

    SYNCML_DM_RET_STATUS_T retcode = mEnv->CallIntMethod(mJavaPluginManager, javaMethod);
    CHECK_EXCEPTION(retcode);
    return retcode;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginManager::ExecuteNode(const char* pArgs,
                                                         const char* pCorrelator,
                                                         PDmtTree    pTree,
                                                         DMString&   results)
{
    results = "";

    if (pArgs == NULL || pTree == NULL) /*|| pCorrelator == NULL*/
    {
        DmtJavaPlugin_Debug("A parameter is null\n");
        return SYNCML_DM_FAIL;
    }

    if (!mIsInitialized)
    {
        DmtJavaPlugin_Debug("Plug-in is not initialized\n");
        return SYNCML_DM_FAIL;
    }

    SYNCML_DM_RET_STATUS_T retcode = SetServerID(pTree);
    if (retcode != SYNCML_DM_SUCCESS)
    {
        DmtJavaPlugin_Debug("Fail to set ServerID\n");
        return retcode;
    }

    if(!InitJNIEnv())
    {
        DmtJavaPlugin_Debug("Init JNI Env failed...\n");
        return SYNCML_DM_FAIL;
    }

    DECLARE_METHOD(javaMethod, "executeNode", "(Ljava/lang/String;Ljava/lang/String;)I");
    DECLARE_STRING(objArgs, pArgs);
    DECLARE_STRING(objCorrelator, (pCorrelator == NULL ? "" : pCorrelator) );

    retcode = mEnv->CallIntMethod(mJavaPluginManager, javaMethod, objArgs.getValue(), objCorrelator.getValue());
    CHECK_EXCEPTION(retcode);
    return retcode;
}

SYNCML_DM_RET_STATUS_T DmtJavaPluginManager::BuildPluginTree(PDmtJavaPluginTree pTree)
{
    if (pTree == NULL)
    {
        DmtJavaPlugin_Debug("Tree is NULL\n");
        return SYNCML_DM_FAIL;
    }

    if (!mIsInitialized)
    {
        DmtJavaPlugin_Debug("Plug-in is not initialized\n");
        return SYNCML_DM_FAIL;
    }

    if(!InitJNIEnv())
    {
        DmtJavaPlugin_Debug("Init JNI Env failed...\n");
        return SYNCML_DM_FAIL;
    }

    DECLARE_METHOD(javaMethod, "getNodes", "()[Ljava/lang/String;");

    DmtJObjectArray objStrArray(mEnv);
    objStrArray = (jobjectArray)(mEnv->CallObjectMethod(mJavaPluginManager, javaMethod));
    if (!objStrArray)
    {
        DmtJavaPlugin_Debug("Fail to get nodes\n");
        return SYNCML_DM_FAIL;
    }

    if ((mEnv->GetArrayLength(objStrArray) % 3) != 0)
    {
        DmtJavaPlugin_Debug("Array count is invalid!\n");
        return SYNCML_DM_FAIL;
    }

    for (int i = 0; i < mEnv->GetArrayLength(objStrArray); i += 3)
    {
        DmtJObject objPath(mEnv,  mEnv->GetObjectArrayElement(objStrArray, i));
        DmtJObject objType(mEnv,  mEnv->GetObjectArrayElement(objStrArray, i + 1));
        DmtJObject objValue(mEnv, mEnv->GetObjectArrayElement(objStrArray, i + 2));
        if (!objPath || !objType || !objValue)
        {
            DmtJavaPlugin_Debug("Failed to parse array, iteration:%d\n", i);
            return SYNCML_DM_FAIL;
        }

        const char* pPath  = mEnv->GetStringUTFChars((jstring)objPath.getValue(),  NULL);
        const char* pType  = mEnv->GetStringUTFChars((jstring)objType.getValue(),  NULL);
        const char* pValue = mEnv->GetStringUTFChars((jstring)objValue.getValue(), NULL);
        if (pPath == NULL || pType == NULL || pValue == NULL)
        {
            DmtJavaPlugin_Debug("Failed to convert to string, iteration:%d\n", i);
            return SYNCML_DM_FAIL;
        }

        DmtData data;

        SYNCML_DM_RET_STATUS_T retcode = SetDmtNodeValue(pType, pValue, data);
        if (retcode != SYNCML_DM_SUCCESS)
        {
            return retcode;
        }

        PDmtJavaPluginNode pNode = new DmtJavaPluginNode(pTree, pPath, data);

        pTree->SetNode(pPath, static_cast<PDmtNode>(pNode));
    }

    return SYNCML_DM_SUCCESS;
}
