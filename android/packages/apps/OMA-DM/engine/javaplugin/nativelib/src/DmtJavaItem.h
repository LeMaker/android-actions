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

#ifndef __DMTJAVAITEM_H__
#define __DMTJAVAITEM_H__

#include "jni.h"

#include <stdlib.h>

#if defined (__cplusplus)

template <class T> class DmtJavaItem
{
public:
    DmtJavaItem(JNIEnv*& env) : mEnv(env), mItem(NULL), mIsGlobalRef(false) {};
    DmtJavaItem(JNIEnv*& env, T item) : mEnv(env), mItem(item), mIsGlobalRef(false) {};
    ~DmtJavaItem()
    {
        if (mEnv != NULL && mItem != NULL)
        {
            if (mIsGlobalRef)
            {
                mEnv->DeleteGlobalRef(mItem);
            }
            else
            {
                mEnv->DeleteLocalRef(mItem);
            }
        }
    }

    void assignValue(jobject obj, JNIEnv*& env)
    {
        mEnv  = env;
        mIsGlobalRef = true;
        mItem = mEnv->NewGlobalRef(obj);
    }

    T getValue() { return mItem; }

    DmtJavaItem& operator=(const T& item)
    {
        mItem = item;
        return *this;
    }

    operator T() const { return mItem; }

private:
    JNIEnv*& mEnv;
    T        mItem;
    bool     mIsGlobalRef;
};

typedef DmtJavaItem<jobject>      DmtJObject;
typedef DmtJavaItem<jclass>       DmtJClass;
typedef DmtJavaItem<jstring>      DmtJString;
typedef DmtJavaItem<jobjectArray> DmtJObjectArray;

#endif // __cplusplus

#endif // __DMTJAVAITEM_H__
