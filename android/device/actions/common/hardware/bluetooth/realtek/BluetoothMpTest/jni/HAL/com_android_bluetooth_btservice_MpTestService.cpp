/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "BluetoothMpTestJni"
#include "JNIHelp.h"
#include "jni.h"

#include "hardware/bluetoothmp.h"
#include "hardware/hardware.h"

#include "utils/Log.h"
#include "utils/misc.h"
#include "cutils/properties.h"
#include "android_runtime/AndroidRuntime.h"
//#include "android_runtime/Log.h"

#include <string.h>
#include <pthread.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <cutils/properties.h>



namespace android {


static jmethodID method_stateChangeCallback;
static jmethodID method_dut_mode_recv;

static const bt_interface_t *sBluetoothInterface = NULL;

static JNIEnv *callbackEnv = NULL;

static jobject sJniCallbacksObj;

const bt_interface_t* getBluetoothInterface() {
    return sBluetoothInterface;
}

JNIEnv* getCallbackEnv() {
   return callbackEnv;
}

void checkAndClearExceptionFromCallback(JNIEnv* env,
                                               const char* methodName) {
    if (env->ExceptionCheck()) {
        ALOGE("An exception was thrown by callback '%s'.", methodName);
//        LOGE_EX(env);
        env->ExceptionClear();
    }
}

static bool checkCallbackThread() {
    JNIEnv* env = AndroidRuntime::getJNIEnv();
    if (callbackEnv != env || callbackEnv == NULL) {
        ALOGE("Callback env check fail: env: %p, callback: %p", env, callbackEnv);
        return false;
    }
    return true;
}

static void adapter_state_change_callback(bt_state_t status) {
    if (!checkCallbackThread()) {
       ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
       return;
    }
    ALOGV("%s: Status is: %d", __FUNCTION__, status);

    callbackEnv->CallVoidMethod(sJniCallbacksObj, method_stateChangeCallback, (jint)status);

    checkAndClearExceptionFromCallback(callbackEnv, __FUNCTION__);
}

static void dut_mode_recv(uint8_t evtcode, char *buf)
{
    jstring  dataBuffer = NULL;
    jclass  strClass = NULL;
    jmethodID ctorID;
    jbyteArray  byteBuffer;
    jstring    encode = NULL;
    int strLen = 0;
    ALOGE("dut_mode_recv1: %s", buf);
    if (!checkCallbackThread()) {
        ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
        return;
    }

    strClass = callbackEnv->FindClass("java/lang/String"); 
    ctorID = callbackEnv->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V"); 
    encode    =callbackEnv->NewStringUTF("utf-8");
    if (encode== NULL) goto Fail;
    
    strLen = strlen(buf);
    
    byteBuffer = callbackEnv->NewByteArray(strLen); 
    if (byteBuffer== NULL) goto Fail;
        
    callbackEnv->SetByteArrayRegion(byteBuffer, 0, strLen, (jbyte*)buf); 
    dataBuffer = (jstring)callbackEnv->NewObject(strClass, ctorID, byteBuffer, encode);

    callbackEnv->CallVoidMethod(sJniCallbacksObj, method_dut_mode_recv, (jint)evtcode, dataBuffer);

    checkAndClearExceptionFromCallback(callbackEnv, __FUNCTION__);

    Fail:
    if (byteBuffer) callbackEnv->DeleteLocalRef(byteBuffer);
    if (dataBuffer) callbackEnv->DeleteLocalRef(dataBuffer);
}


static void callback_thread_event(bt_cb_thread_evt event) {
    JavaVM* vm = AndroidRuntime::getJavaVM();
    if (event  == ASSOCIATE_JVM) {
        JavaVMAttachArgs args;
        char name[] = "BT MP test Service Callback Thread";
        //TODO(BT)
        args.version = JNI_VERSION_1_6;
        args.name = name;
        args.group = NULL;
        vm->AttachCurrentThread(&callbackEnv, &args);
        ALOGV("Callback thread attached: %p", callbackEnv);
    } else if (event == DISASSOCIATE_JVM) {
        if (!checkCallbackThread()) {
            ALOGE("Callback: '%s' is not called on the correct thread", __FUNCTION__);
            return;
        }
        vm->DetachCurrentThread();
    }
}

bt_callbacks_t sBluetoothCallbacks = {
    sizeof(sBluetoothCallbacks),
    adapter_state_change_callback,
    callback_thread_event, /* thread_evt_cb */
    dut_mode_recv, /*dut_mode_recv_cb */
};

static void classInitNative(JNIEnv* env, jclass clazz) {
    int err;
    hw_module_t* module;
    ALOGE("%s:",__FUNCTION__);

    jclass jniCallbackClass =
        env->FindClass("com/android/bluetooth/btservice/MpTestService");

    method_stateChangeCallback = env->GetMethodID(jniCallbackClass, "stateChangeCallback", "(I)V");
    method_dut_mode_recv = env->GetMethodID(jniCallbackClass, "dut_mode_recv", "(BLjava/lang/String;)V");



    err = hw_get_module("bluetoothmp", (hw_module_t const**)&module);

    if (err == 0) {
        hw_device_t* abstraction;
        err = module->methods->open(module, "bluetoothmp", &abstraction);
        if (err == 0) {
            bluetooth_module_t* btStack = (bluetooth_module_t *)abstraction;
            sBluetoothInterface = btStack->get_bluetooth_interface();
        } else {
           ALOGE("Error while opening Bluetooth library");
        }
    } else {
        ALOGE("No Bluetooth Library found");
    }
}

static bool initNative(JNIEnv* env, jobject obj) {
    ALOGE("%s:",__FUNCTION__);

    char propBuf[PROPERTY_VALUE_MAX];
    propBuf[0] = '1';
    if(property_set("rt.bt.mp.mode", propBuf) < 0){
        ALOGE("property_set rt.bt.mp.mode fail ");
    }

    if (sJniCallbacksObj != NULL) {
         env->DeleteGlobalRef(sJniCallbacksObj);
         sJniCallbacksObj = NULL;
    }
    sJniCallbacksObj = env->NewGlobalRef(obj);

    if (sBluetoothInterface) {
        int ret = sBluetoothInterface->init(&sBluetoothCallbacks);
        if (ret != BT_STATUS_SUCCESS) {
            ALOGE("Error while setting the callbacks \n");
            sBluetoothInterface = NULL;
            return JNI_FALSE;
        }

        return JNI_TRUE;
    }
    return JNI_FALSE;
}

static bool cleanupNative(JNIEnv *env, jobject obj) {
    ALOGE("%s:",__FUNCTION__);

    char propBuf[PROPERTY_VALUE_MAX];
    propBuf[0] = '0';
    if(property_set("rt.bt.mp.mode", propBuf) < 0)
    {
        ALOGE("property_set rt.bt.mp.mode fail ");
    }

    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    sBluetoothInterface->cleanup();
    ALOGI("%s: return from cleanup",__FUNCTION__);

    if (sJniCallbacksObj != NULL) {
         env->DeleteGlobalRef(sJniCallbacksObj);
         sJniCallbacksObj = NULL;
    }

    return JNI_TRUE;
}

static jboolean enableNative(JNIEnv* env, jobject obj) {
    ALOGE("%s:",__FUNCTION__);

    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    int ret = sBluetoothInterface->enable();
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static jboolean disableNative(JNIEnv* env, jobject obj) {
    ALOGE("%s:",__FUNCTION__);

    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    int ret = sBluetoothInterface->disable();
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}


static jint hciSendNative(JNIEnv* env, jobject obj, jint opcode, jstring data)
{
    ALOGE("%s:",__FUNCTION__);

    jint bytesSend = 0;
    if (!sBluetoothInterface) return bytesSend;

    const char *input_data = NULL;
    jsize len = 0;
    if(data != NULL)
    {
        input_data = env->GetStringUTFChars(data, NULL);
    }

    bytesSend = sBluetoothInterface->hal_mp_op_send(opcode, (char*)input_data, len);

    if(input_data != NULL)
    {
        env->ReleaseStringUTFChars(data, input_data);
    }

    return bytesSend;
}

static jboolean dutModeConfigureNative(JNIEnv *env, jobject obj, jint type) {
    ALOGE("%s:",__FUNCTION__);

    jboolean result = JNI_FALSE;
    if (!sBluetoothInterface) return result;

    int ret = sBluetoothInterface->dut_mode_configure( type);
    result = (ret == BT_STATUS_SUCCESS) ? JNI_TRUE : JNI_FALSE;

    return result;
}

static JNINativeMethod sMethods[] = {
     /* name, signature, funcPtr */
    {"classInitNative", "()V", (void *) classInitNative},
    {"initNative", "()Z", (void *) initNative},
    {"cleanupNative", "()V", (void*) cleanupNative},
    {"enableNative", "()Z",  (void*) enableNative},
    {"disableNative", "()Z",  (void*) disableNative},
    {"hciSendNative", "(ILjava/lang/String;)I", (void*)hciSendNative},
    {"dutModeConfigureNative", "(I)Z", (void*) dutModeConfigureNative}

};

int register_com_android_bluetooth_btservice_MpTestService(JNIEnv* env)
{
    return jniRegisterNativeMethods(env, "com/android/bluetooth/btservice/MpTestService",
                                    sMethods, NELEM(sMethods));
}

} /* namespace android */


/*
 * JNI Initialization
 */
jint JNI_OnLoad(JavaVM *jvm, void *reserved)
{
   JNIEnv *e;
   int status;

   ALOGE("Bluetooth MpTest Service : loading JNI\n");

   // Check JNI version
   if(jvm->GetEnv((void **)&e, JNI_VERSION_1_6)) {
       ALOGE("JNI version mismatch error");
      return JNI_ERR;
   }

   if ((status = android::register_com_android_bluetooth_btservice_MpTestService(e)) < 0) {
       ALOGE("jni mptest service registration failure, status: %d", status);
      return JNI_ERR;
   }

   return JNI_VERSION_1_6;
}
