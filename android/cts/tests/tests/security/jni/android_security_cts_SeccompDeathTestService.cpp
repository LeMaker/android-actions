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

#include <jni.h>
#include <signal.h>
#include <unistd.h>

void android_security_cts_SeccompDeathTestService_testSigSysSelf(JNIEnv* env, jobject thiz)
{
    kill(getpid(), SIGSYS);
}

static JNINativeMethod methods[] = {
    { "testSigSysSelf", "()V",
        (void *)android_security_cts_SeccompDeathTestService_testSigSysSelf }
};

int register_android_security_cts_SeccompDeathTestService(JNIEnv* env) {
    jclass clazz = env->FindClass("android/security/cts/SeccompDeathTestService");
    return env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(JNINativeMethod));
}
