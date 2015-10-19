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

#ifndef __DMTJAVAMETHOD_H__
#define __DMTJAVAMETHOD_H__

#include "jni.h"
#include "DmtJavaItem.h"

class DmtJavaMethod
{
public:
    DmtJavaMethod(JNIEnv*& env, jobject object, const char* methodName, const char* methodParam)
        : mEnv(env), mClass(env), mMethodID(NULL)
    {
        if (mEnv == NULL || object == NULL || methodName == NULL || methodParam == NULL)
        {
            DmtJavaPlugin_Debug("DmtJavaMethod::DmtJavaMethod -> invalid params\n");
            return;
        }

        mClass = mEnv->GetObjectClass(object);
        if (mClass) {
            mMethodID = mEnv->GetMethodID(mClass, methodName, methodParam);
        }
    };

    DmtJavaMethod(JNIEnv*& env, const char* classPath, const char* methodName, const char* methodParam)
        : mEnv(env), mClass(env), mMethodID(NULL)
    {
        if (mEnv == NULL || classPath == NULL || methodName == NULL || methodParam == NULL)
        {
            DmtJavaPlugin_Debug("DmtJavaMethod::DmtJavaMethod -> invalid params\n");
            return;
        }

        mClass = mEnv->FindClass(classPath);
        if (mClass) {
            mMethodID = mEnv->GetMethodID(mClass, methodName, methodParam);
        }
    };

    ~DmtJavaMethod() {};

    bool isFound() { return mMethodID != NULL; };

    jmethodID getMethodID() { return mMethodID; }
    jclass    getClass()    { return mClass;    }

    operator jmethodID() const { return mMethodID; }

private:
    JNIEnv*&  mEnv;
    DmtJClass mClass;
    jmethodID mMethodID;
};

#endif //__DMTJAVAMETHOD_H__
