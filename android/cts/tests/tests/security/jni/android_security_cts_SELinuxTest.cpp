/*
 * Copyright (C) 2013 The Android Open Source Project
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
#include <selinux/selinux.h>
#include <JNIHelp.h>
#include <ScopedUtfChars.h>

/*
 * Function: checkSELinuxAccess
 * Purpose: Check permissions between two security contexts.
 * Parameters: subjectContextStr: subject security context as a string
 *             objectContextStr: object security context as a string
 *             objectClassStr: object's security class name as a string
 *             permissionStr: permission name as a string
 * Returns: boolean: (true) if permission was granted, (false) otherwise
 * Exceptions: NullPointerException if any argument is NULL
 */
static jboolean android_security_cts_SELinuxTest_checkSELinuxAccess(JNIEnv *env, jobject, jstring subjectContextStr,
        jstring objectContextStr, jstring objectClassStr, jstring permissionStr, jstring auxStr) {
    if (subjectContextStr == NULL || objectContextStr == NULL || objectClassStr == NULL
            || permissionStr == NULL || auxStr == NULL) {
        jniThrowNullPointerException(env, NULL);
        return false;
    }

    ScopedUtfChars subjectContext(env, subjectContextStr);
    ScopedUtfChars objectContext(env, objectContextStr);
    ScopedUtfChars objectClass(env, objectClassStr);
    ScopedUtfChars permission(env, permissionStr);
    ScopedUtfChars aux(env, auxStr);

    char *tmp1 = const_cast<char *>(subjectContext.c_str());
    char *tmp2 = const_cast<char *>(objectContext.c_str());
    char *tmp3 = const_cast<char *>(aux.c_str());
    int accessGranted = selinux_check_access(tmp1, tmp2, objectClass.c_str(), permission.c_str(), tmp3);
    return (accessGranted == 0) ? true : false;
}

static jboolean android_security_cts_SELinuxTest_checkSELinuxContext(JNIEnv *env, jobject, jstring contextStr) {
    if (contextStr == NULL) {
        jniThrowNullPointerException(env, NULL);
        return false;
    }

    ScopedUtfChars context(env, contextStr);

    char *tmp = const_cast<char *>(context.c_str());
    int validContext = security_check_context(tmp);
    return (validContext == 0) ? true : false;
}


static JNINativeMethod gMethods[] = {
    {  "checkSELinuxAccess", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z",
            (void *) android_security_cts_SELinuxTest_checkSELinuxAccess },
    {  "checkSELinuxContext", "(Ljava/lang/String;)Z",
            (void *) android_security_cts_SELinuxTest_checkSELinuxContext },
};

int register_android_security_cts_SELinuxTest(JNIEnv* env)
{
    jclass clazz = env->FindClass("android/security/cts/SELinuxTest");
    return env->RegisterNatives(clazz, gMethods,
            sizeof(gMethods) / sizeof(JNINativeMethod));
}
