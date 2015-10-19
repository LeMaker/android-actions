/*
 * Copyright 2008, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "wifi"

#include "jni.h"
#include <ScopedUtfChars.h>
#include <utils/misc.h>
#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <utils/String16.h>
#include <ctype.h>

#include "wifi.h"
#include "wifi_hal.h"
#include "jni_helper.h"

#define REPLY_BUF_SIZE 4096 // wpa_supplicant's maximum size.
#define EVENT_BUF_SIZE 2048

namespace android {

static jint DBG = false;

static bool doCommand(JNIEnv* env, jstring javaCommand,
                      char* reply, size_t reply_len) {
    ScopedUtfChars command(env, javaCommand);
    if (command.c_str() == NULL) {
        return false; // ScopedUtfChars already threw on error.
    }

    if (DBG) {
        ALOGD("doCommand: %s", command.c_str());
    }

    --reply_len; // Ensure we have room to add NUL termination.
    if (::wifi_command(command.c_str(), reply, &reply_len) != 0) {
        return false;
    }

    // Strip off trailing newline.
    if (reply_len > 0 && reply[reply_len-1] == '\n') {
        reply[reply_len-1] = '\0';
    } else {
        reply[reply_len] = '\0';
    }
    return true;
}

static jint doIntCommand(JNIEnv* env, jstring javaCommand) {
    char reply[REPLY_BUF_SIZE];
    if (!doCommand(env, javaCommand, reply, sizeof(reply))) {
        return -1;
    }
    return static_cast<jint>(atoi(reply));
}

static jboolean doBooleanCommand(JNIEnv* env, jstring javaCommand) {
    char reply[REPLY_BUF_SIZE];
    if (!doCommand(env, javaCommand, reply, sizeof(reply))) {
        return JNI_FALSE;
    }
    return (strcmp(reply, "OK") == 0);
}

// Send a command to the supplicant, and return the reply as a String.
static jstring doStringCommand(JNIEnv* env, jstring javaCommand) {
    char reply[REPLY_BUF_SIZE];
    if (!doCommand(env, javaCommand, reply, sizeof(reply))) {
        return NULL;
    }
    return env->NewStringUTF(reply);
}

static jboolean android_net_wifi_isDriverLoaded(JNIEnv* env, jobject)
{
    return (::is_wifi_driver_loaded() == 1);
}

static jboolean android_net_wifi_loadDriver(JNIEnv* env, jobject)
{
    return (::wifi_load_driver() == 0);
}

static jboolean android_net_wifi_unloadDriver(JNIEnv* env, jobject)
{
    return (::wifi_unload_driver() == 0);
}

static jboolean android_net_wifi_startSupplicant(JNIEnv* env, jobject, jboolean p2pSupported)
{
    return (::wifi_start_supplicant(p2pSupported) == 0);
}

static jboolean android_net_wifi_killSupplicant(JNIEnv* env, jobject, jboolean p2pSupported)
{
    return (::wifi_stop_supplicant(p2pSupported) == 0);
}

static jboolean android_net_wifi_connectToSupplicant(JNIEnv* env, jobject)
{
    return (::wifi_connect_to_supplicant() == 0);
}

static void android_net_wifi_closeSupplicantConnection(JNIEnv* env, jobject)
{
    ::wifi_close_supplicant_connection();
}

static jstring android_net_wifi_waitForEvent(JNIEnv* env, jobject)
{
    char buf[EVENT_BUF_SIZE];
    int nread = ::wifi_wait_for_event(buf, sizeof buf);
    if (nread > 0) {
        return env->NewStringUTF(buf);
    } else {
        return NULL;
    }
}

static jboolean android_net_wifi_doBooleanCommand(JNIEnv* env, jobject, jstring javaCommand) {
    return doBooleanCommand(env, javaCommand);
}

static jint android_net_wifi_doIntCommand(JNIEnv* env, jobject, jstring javaCommand) {
    return doIntCommand(env, javaCommand);
}

static jstring android_net_wifi_doStringCommand(JNIEnv* env, jobject, jstring javaCommand) {
    return doStringCommand(env,javaCommand);
}

/* wifi_hal <==> WifiNative bridge */

static jclass mCls;                             /* saved WifiNative object */
static JavaVM *mVM;                             /* saved JVM pointer */

static const char *WifiHandleVarName = "sWifiHalHandle";
static const char *WifiIfaceHandleVarName = "sWifiIfaceHandles";
static jmethodID OnScanResultsMethodID;

static JNIEnv *getEnv() {
    JNIEnv *env = NULL;
    mVM->AttachCurrentThread(&env, NULL);
    return env;
}

static wifi_handle getWifiHandle(JNIEnv *env, jclass cls) {
    return (wifi_handle) getStaticLongField(env, cls, WifiHandleVarName);
}

static wifi_interface_handle getIfaceHandle(JNIEnv *env, jclass cls, jint index) {
    return (wifi_interface_handle) getStaticLongArrayField(env, cls, WifiIfaceHandleVarName, index);
}

static jobject createScanResult(JNIEnv *env, wifi_scan_result *result) {

    // ALOGD("creating scan result");

    jobject scanResult = createObject(env, "android/net/wifi/ScanResult");
    if (scanResult == NULL) {
        ALOGE("Error in creating scan result");
        return NULL;
    }

    // ALOGD("setting SSID to %s", result.ssid);
    setStringField(env, scanResult, "SSID", result->ssid);

    char bssid[32];
    sprintf(bssid, "%02x:%02x:%02x:%02x:%02x:%02x", result->bssid[0], result->bssid[1],
        result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5]);

    setStringField(env, scanResult, "BSSID", bssid);

    setIntField(env, scanResult, "level", result->rssi);
    setIntField(env, scanResult, "frequency", result->channel);
    setLongField(env, scanResult, "timestamp", result->ts);

    return scanResult;
}

static jboolean android_net_wifi_startHal(JNIEnv* env, jclass cls) {
    wifi_handle halHandle = getWifiHandle(env, cls);

    if (halHandle == NULL) {
        wifi_error res = wifi_initialize(&halHandle);
        if (res == WIFI_SUCCESS) {
            setStaticLongField(env, cls, WifiHandleVarName, (jlong)halHandle);
            ALOGD("Did set static halHandle = %p", halHandle);
        }
        env->GetJavaVM(&mVM);
        mCls = (jclass) env->NewGlobalRef(cls);
        ALOGD("halHandle = %p, mVM = %p, mCls = %p", halHandle, mVM, mCls);
        return res == WIFI_SUCCESS;
    } else {
        return true;
    }
}

void android_net_wifi_hal_cleaned_up_handler(wifi_handle handle) {
    ALOGD("In wifi cleaned up handler");

    JNIEnv * env = getEnv();
    setStaticLongField(env, mCls, WifiHandleVarName, 0);
    env->DeleteGlobalRef(mCls);
    mCls = NULL;
    mVM  = NULL;
}

static void android_net_wifi_stopHal(JNIEnv* env, jclass cls) {
    ALOGD("In wifi stop Hal");
    wifi_handle halHandle = getWifiHandle(env, cls);
    wifi_cleanup(halHandle, android_net_wifi_hal_cleaned_up_handler);
}

static void android_net_wifi_waitForHalEvents(JNIEnv* env, jclass cls) {

    ALOGD("waitForHalEvents called, vm = %p, obj = %p, env = %p", mVM, mCls, env);

    wifi_handle halHandle = getWifiHandle(env, cls);
    wifi_event_loop(halHandle);
}

static int android_net_wifi_getInterfaces(JNIEnv *env, jclass cls) {
    int n = 0;
    wifi_handle halHandle = getWifiHandle(env, cls);
    wifi_interface_handle *ifaceHandles = NULL;
    int result = wifi_get_ifaces(halHandle, &n, &ifaceHandles);
    if (result < 0) {
        return result;
    }

    if (n < 0) {
        THROW(env, "android_net_wifi_getInterfaces no interfaces");
        return 0;
    }

    if (ifaceHandles == NULL) {
       THROW(env, "android_net_wifi_getInterfaces null interface array");
       return 0;
    }

    if (n > 8) {
        THROW(env, "Too many interfaces");
        return 0;
    }

    jlongArray array = (env)->NewLongArray(n);
    if (array == NULL) {
        THROW(env, "Error in accessing array");
        return 0;
    }

    jlong elems[8];
    for (int i = 0; i < n; i++) {
        elems[i] = reinterpret_cast<jlong>(ifaceHandles[i]);
    }
    env->SetLongArrayRegion(array, 0, n, elems);
    setStaticLongArrayField(env, cls, WifiIfaceHandleVarName, array);

    return (result < 0) ? result : n;
}

static jstring android_net_wifi_getInterfaceName(JNIEnv *env, jclass cls, jint i) {
    char buf[EVENT_BUF_SIZE];

    jlong value = getStaticLongArrayField(env, cls, WifiIfaceHandleVarName, i);
    wifi_interface_handle handle = (wifi_interface_handle) value;
    int result = ::wifi_get_iface_name(handle, buf, sizeof(buf));
    if (result < 0) {
        return NULL;
    } else {
        return env->NewStringUTF(buf);
    }
}


static void onScanResultsAvailable(wifi_request_id id, unsigned num_results) {

    JNIEnv *env = NULL;
    mVM->AttachCurrentThread(&env, NULL);

    ALOGD("onScanResultsAvailable called, vm = %p, obj = %p, env = %p", mVM, mCls, env);

    reportEvent(env, mCls, "onScanResultsAvailable", "(I)V", id);
}

static void onScanEvent(wifi_scan_event event, unsigned status) {
    JNIEnv *env = NULL;
    mVM->AttachCurrentThread(&env, NULL);

    ALOGD("onScanStatus called, vm = %p, obj = %p, env = %p", mVM, mCls, env);

    reportEvent(env, mCls, "onScanStatus", "(I)V", status);
}

static void onFullScanResult(wifi_request_id id, wifi_scan_result *result) {

    JNIEnv *env = NULL;
    mVM->AttachCurrentThread(&env, NULL);

    ALOGD("onFullScanResult called, vm = %p, obj = %p, env = %p", mVM, mCls, env);

    jobject scanResult = createScanResult(env, result);

    ALOGD("Creating a byte array of length %d", result->ie_length);

    jbyteArray elements = env->NewByteArray(result->ie_length);
    if (elements == NULL) {
        ALOGE("Error in allocating array");
        return;
    }

    ALOGE("Setting byte array");

    jbyte *bytes = (jbyte *)&(result->ie_data[0]);
    env->SetByteArrayRegion(elements, 0, result->ie_length, bytes);

    ALOGE("Returning result");

    reportEvent(env, mCls, "onFullScanResult", "(ILandroid/net/wifi/ScanResult;[B)V", id,
            scanResult, elements);

    env->DeleteLocalRef(scanResult);
    env->DeleteLocalRef(elements);
}

static jboolean android_net_wifi_startScan(
        JNIEnv *env, jclass cls, jint iface, jint id, jobject settings) {

    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    ALOGD("starting scan on interface[%d] = %p", iface, handle);

    wifi_scan_cmd_params params;
    memset(&params, 0, sizeof(params));

    params.base_period = getIntField(env, settings, "base_period_ms");
    params.max_ap_per_scan = getIntField(env, settings, "max_ap_per_scan");
    params.report_threshold = getIntField(env, settings, "report_threshold");

    ALOGD("Initialized common fields %d, %d, %d", params.base_period,
            params.max_ap_per_scan, params.report_threshold);

    const char *bucket_array_type = "[Lcom/android/server/wifi/WifiNative$BucketSettings;";
    const char *channel_array_type = "[Lcom/android/server/wifi/WifiNative$ChannelSettings;";

    jobjectArray buckets = (jobjectArray)getObjectField(env, settings, "buckets", bucket_array_type);
    params.num_buckets = getIntField(env, settings, "num_buckets");

    ALOGD("Initialized num_buckets to %d", params.num_buckets);

    for (int i = 0; i < params.num_buckets; i++) {
        jobject bucket = getObjectArrayField(env, settings, "buckets", bucket_array_type, i);

        params.buckets[i].bucket = getIntField(env, bucket, "bucket");
        params.buckets[i].band = (wifi_band) getIntField(env, bucket, "band");
        params.buckets[i].period = getIntField(env, bucket, "period_ms");

        ALOGD("Initialized common bucket fields %d:%d:%d", params.buckets[i].bucket,
                params.buckets[i].band, params.buckets[i].period);

        int report_events = getIntField(env, bucket, "report_events");
        params.buckets[i].report_events = report_events;

        ALOGD("Initialized report events to %d", params.buckets[i].report_events);

        jobjectArray channels = (jobjectArray)getObjectField(
                env, bucket, "channels", channel_array_type);

        params.buckets[i].num_channels = getIntField(env, bucket, "num_channels");
        ALOGD("Initialized num_channels to %d", params.buckets[i].num_channels);

        for (int j = 0; j < params.buckets[i].num_channels; j++) {
            jobject channel = getObjectArrayField(env, bucket, "channels", channel_array_type, j);

            params.buckets[i].channels[j].channel = getIntField(env, channel, "frequency");
            params.buckets[i].channels[j].dwellTimeMs = getIntField(env, channel, "dwell_time_ms");

            bool passive = getBoolField(env, channel, "passive");
            params.buckets[i].channels[j].passive = (passive ? 1 : 0);

            ALOGD("Initialized channel %d", params.buckets[i].channels[j].channel);
        }
    }

    ALOGD("Initialized all fields");

    wifi_scan_result_handler handler;
    memset(&handler, 0, sizeof(handler));
    handler.on_scan_results_available = &onScanResultsAvailable;
    handler.on_full_scan_result = &onFullScanResult;
    handler.on_scan_event = &onScanEvent;

    return wifi_start_gscan(id, handle, params, handler) == WIFI_SUCCESS;
}

static jboolean android_net_wifi_stopScan(JNIEnv *env, jclass cls, jint iface, jint id) {
    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    ALOGD("stopping scan on interface[%d] = %p", iface, handle);

    return wifi_stop_gscan(id, handle)  == WIFI_SUCCESS;
}

static jobject android_net_wifi_getScanResults(
        JNIEnv *env, jclass cls, jint iface, jboolean flush)  {
    
    wifi_scan_result results[256];
    int num_results = 256;
    
    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    ALOGD("getting scan results on interface[%d] = %p", iface, handle);
    
    int result = wifi_get_cached_gscan_results(handle, 1, num_results, results, &num_results);
    if (result == WIFI_SUCCESS) {
        jclass clsScanResult = (env)->FindClass("android/net/wifi/ScanResult");
        if (clsScanResult == NULL) {
            ALOGE("Error in accessing class");
            return NULL;
        }

        jobjectArray scanResults = env->NewObjectArray(num_results, clsScanResult, NULL);
        if (scanResults == NULL) {
            ALOGE("Error in allocating array");
            return NULL;
        }

        for (int i = 0; i < num_results; i++) {

            jobject scanResult = createObject(env, "android/net/wifi/ScanResult");
            if (scanResult == NULL) {
                ALOGE("Error in creating scan result");
                return NULL;
            }

            setStringField(env, scanResult, "SSID", results[i].ssid);

            char bssid[32];
            sprintf(bssid, "%02x:%02x:%02x:%02x:%02x:%02x", results[i].bssid[0],
                    results[i].bssid[1], results[i].bssid[2], results[i].bssid[3],
                    results[i].bssid[4], results[i].bssid[5]);

            setStringField(env, scanResult, "BSSID", bssid);

            setIntField(env, scanResult, "level", results[i].rssi);
            setIntField(env, scanResult, "frequency", results[i].channel);
            setLongField(env, scanResult, "timestamp", results[i].ts);

            env->SetObjectArrayElement(scanResults, i, scanResult);
            env->DeleteLocalRef(scanResult);
        }

        return scanResults;
    } else {
        return NULL;
    }
}


static jboolean android_net_wifi_getScanCapabilities(
        JNIEnv *env, jclass cls, jint iface, jobject capabilities) {

    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    ALOGD("getting scan capabilities on interface[%d] = %p", iface, handle);

    wifi_gscan_capabilities c;
    memset(&c, 0, sizeof(c));
    int result = wifi_get_gscan_capabilities(handle, &c);
    if (result != WIFI_SUCCESS) {
        ALOGD("failed to get capabilities : %d", result);
        return JNI_FALSE;
    }

    setIntField(env, capabilities, "max_scan_cache_size", c.max_scan_cache_size);
    setIntField(env, capabilities, "max_scan_buckets", c.max_scan_buckets);
    setIntField(env, capabilities, "max_ap_cache_per_scan", c.max_ap_cache_per_scan);
    setIntField(env, capabilities, "max_rssi_sample_size", c.max_rssi_sample_size);
    setIntField(env, capabilities, "max_scan_reporting_threshold", c.max_scan_reporting_threshold);
    setIntField(env, capabilities, "max_hotlist_aps", c.max_hotlist_aps);
    setIntField(env, capabilities, "max_significant_wifi_change_aps",
                c.max_significant_wifi_change_aps);

    return JNI_TRUE;
}


static byte parseHexChar(char ch) {
    if (isdigit(ch))
        return ch - '0';
    else if ('A' <= ch && ch <= 'F')
        return ch - 'A' + 10;
    else if ('a' <= ch && ch <= 'f')
        return ch - 'a' + 10;
    else {
        ALOGE("invalid character in bssid %c", ch);
        return 0;
    }
}

static byte parseHexByte(const char * &str) {
    byte b = parseHexChar(str[0]);
    if (str[1] == ':' || str[1] == '\0') {
        str += 2;
        return b;
    } else {
        b = b << 4 | parseHexChar(str[1]);
        str += 3;
        return b;
    }
}

static void parseMacAddress(const char *str, mac_addr addr) {
    addr[0] = parseHexByte(str);
    addr[1] = parseHexByte(str);
    addr[2] = parseHexByte(str);
    addr[3] = parseHexByte(str);
    addr[4] = parseHexByte(str);
    addr[5] = parseHexByte(str);
}

static bool parseMacAddress(JNIEnv *env, jobject obj, mac_addr addr) {
    jstring macAddrString = (jstring) getObjectField(
            env, obj, "bssid", "Ljava/lang/String;");

    if (macAddrString == NULL) {
        ALOGE("Error getting bssid field");
        return false;
    }

    const char *bssid = env->GetStringUTFChars(macAddrString, NULL);
    if (bssid == NULL) {
        ALOGE("Error getting bssid");
        return false;
    }

    parseMacAddress(bssid, addr);
    return true;
}

static void onHotlistApFound(wifi_request_id id,
        unsigned num_results, wifi_scan_result *results) {

    JNIEnv *env = NULL;
    mVM->AttachCurrentThread(&env, NULL);

    ALOGD("onHotlistApFound called, vm = %p, obj = %p, env = %p, num_results = %d",
            mVM, mCls, env, num_results);

    jclass clsScanResult = (env)->FindClass("android/net/wifi/ScanResult");
    if (clsScanResult == NULL) {
        ALOGE("Error in accessing class");
        return;
    }

    jobjectArray scanResults = env->NewObjectArray(num_results, clsScanResult, NULL);
    if (scanResults == NULL) {
        ALOGE("Error in allocating array");
        return;
    }

    for (unsigned i = 0; i < num_results; i++) {

        jobject scanResult = createObject(env, "android/net/wifi/ScanResult");
        if (scanResult == NULL) {
            ALOGE("Error in creating scan result");
            return;
        }

        setStringField(env, scanResult, "SSID", results[i].ssid);

        char bssid[32];
        sprintf(bssid, "%02x:%02x:%02x:%02x:%02x:%02x", results[i].bssid[0], results[i].bssid[1],
            results[i].bssid[2], results[i].bssid[3], results[i].bssid[4], results[i].bssid[5]);

        setStringField(env, scanResult, "BSSID", bssid);

        setIntField(env, scanResult, "level", results[i].rssi);
        setIntField(env, scanResult, "frequency", results[i].channel);
        setLongField(env, scanResult, "timestamp", results[i].ts);

        env->SetObjectArrayElement(scanResults, i, scanResult);

        ALOGD("Found AP %32s %s", results[i].ssid, bssid);
    }

    reportEvent(env, mCls, "onHotlistApFound", "(I[Landroid/net/wifi/ScanResult;)V",
        id, scanResults);
}

static jboolean android_net_wifi_setHotlist(
        JNIEnv *env, jclass cls, jint iface, jint id, jobject ap)  {

    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    ALOGD("setting hotlist on interface[%d] = %p", iface, handle);

    wifi_bssid_hotlist_params params;
    memset(&params, 0, sizeof(params));

    jobjectArray array = (jobjectArray) getObjectField(env, ap,
            "bssidInfos", "[Landroid/net/wifi/WifiScanner$BssidInfo;");
    params.num_ap = env->GetArrayLength(array);

    if (params.num_ap == 0) {
        ALOGE("Error in accesing array");
        return false;
    }

    for (int i = 0; i < params.num_ap; i++) {
        jobject objAp = env->GetObjectArrayElement(array, i);

        jstring macAddrString = (jstring) getObjectField(
                env, objAp, "bssid", "Ljava/lang/String;");
        if (macAddrString == NULL) {
            ALOGE("Error getting bssid field");
            return false;
        }

        const char *bssid = env->GetStringUTFChars(macAddrString, NULL);
        if (bssid == NULL) {
            ALOGE("Error getting bssid");
            return false;
        }
        parseMacAddress(bssid, params.ap[i].bssid);

        mac_addr addr;
        memcpy(addr, params.ap[i].bssid, sizeof(mac_addr));

        char bssidOut[32];
        sprintf(bssidOut, "%0x:%0x:%0x:%0x:%0x:%0x", addr[0], addr[1],
            addr[2], addr[3], addr[4], addr[5]);

        ALOGD("Added bssid %s", bssidOut);

        params.ap[i].low = getIntField(env, objAp, "low");
        params.ap[i].high = getIntField(env, objAp, "high");
    }

    wifi_hotlist_ap_found_handler handler;
    memset(&handler, 0, sizeof(handler));

    handler.on_hotlist_ap_found = &onHotlistApFound;
    return wifi_set_bssid_hotlist(id, handle, params, handler) == WIFI_SUCCESS;
}

static jboolean android_net_wifi_resetHotlist(
        JNIEnv *env, jclass cls, jint iface, jint id)  {

    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    ALOGD("resetting hotlist on interface[%d] = %p", iface, handle);

    return wifi_reset_bssid_hotlist(id, handle) == WIFI_SUCCESS;
}

void onSignificantWifiChange(wifi_request_id id,
        unsigned num_results, wifi_significant_change_result **results) {
    JNIEnv *env = NULL;
    mVM->AttachCurrentThread(&env, NULL);

    ALOGD("onSignificantWifiChange called, vm = %p, obj = %p, env = %p", mVM, mCls, env);

    jclass clsScanResult = (env)->FindClass("android/net/wifi/ScanResult");
    if (clsScanResult == NULL) {
        ALOGE("Error in accessing class");
        return;
    }

    jobjectArray scanResults = env->NewObjectArray(num_results, clsScanResult, NULL);
    if (scanResults == NULL) {
        ALOGE("Error in allocating array");
        return;
    }

    for (unsigned i = 0; i < num_results; i++) {

        wifi_significant_change_result result = *(results[i]);

        jobject scanResult = createObject(env, "android/net/wifi/ScanResult");
        if (scanResult == NULL) {
            ALOGE("Error in creating scan result");
            return;
        }

        // setStringField(env, scanResult, "SSID", results[i].ssid);

        char bssid[32];
        sprintf(bssid, "%02x:%02x:%02x:%02x:%02x:%02x", result.bssid[0], result.bssid[1],
            result.bssid[2], result.bssid[3], result.bssid[4], result.bssid[5]);

        setStringField(env, scanResult, "BSSID", bssid);

        setIntField(env, scanResult, "level", result.rssi[0]);
        setIntField(env, scanResult, "frequency", result.channel);
        // setLongField(env, scanResult, "timestamp", result.ts);

        env->SetObjectArrayElement(scanResults, i, scanResult);
    }

    reportEvent(env, mCls, "onSignificantWifiChange", "(I[Landroid/net/wifi/ScanResult;)V",
        id, scanResults);

}

static jboolean android_net_wifi_trackSignificantWifiChange(
        JNIEnv *env, jclass cls, jint iface, jint id, jobject settings)  {

    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    ALOGD("tracking significant wifi change on interface[%d] = %p", iface, handle);

    wifi_significant_change_params params;
    memset(&params, 0, sizeof(params));

    params.rssi_sample_size = getIntField(env, settings, "rssiSampleSize");
    params.lost_ap_sample_size = getIntField(env, settings, "lostApSampleSize");
    params.min_breaching = getIntField(env, settings, "minApsBreachingThreshold");

    const char *bssid_info_array_type = "[Landroid/net/wifi/WifiScanner$BssidInfo;";
    jobjectArray bssids = (jobjectArray)getObjectField(
                env, settings, "bssidInfos", bssid_info_array_type);
    params.num_ap = env->GetArrayLength(bssids);

    if (params.num_ap == 0) {
        ALOGE("Error in accessing array");
        return false;
    }

    ALOGD("Initialized common fields %d, %d, %d, %d", params.rssi_sample_size,
            params.lost_ap_sample_size, params.min_breaching, params.num_ap);

    for (int i = 0; i < params.num_ap; i++) {
        jobject objAp = env->GetObjectArrayElement(bssids, i);

        jstring macAddrString = (jstring) getObjectField(
                env, objAp, "bssid", "Ljava/lang/String;");
        if (macAddrString == NULL) {
            ALOGE("Error getting bssid field");
            return false;
        }

        const char *bssid = env->GetStringUTFChars(macAddrString, NULL);
        if (bssid == NULL) {
            ALOGE("Error getting bssid");
            return false;
        }

        mac_addr addr;
        parseMacAddress(bssid, addr);
        memcpy(params.ap[i].bssid, addr, sizeof(mac_addr));

        char bssidOut[32];
        sprintf(bssidOut, "%02x:%02x:%02x:%02x:%02x:%02x", addr[0], addr[1],
            addr[2], addr[3], addr[4], addr[5]);

        params.ap[i].low = getIntField(env, objAp, "low");
        params.ap[i].high = getIntField(env, objAp, "high");

        ALOGD("Added bssid %s, [%04d, %04d]", bssidOut, params.ap[i].low, params.ap[i].high);
    }

    ALOGD("Added %d bssids", params.num_ap);

    wifi_significant_change_handler handler;
    memset(&handler, 0, sizeof(handler));

    handler.on_significant_change = &onSignificantWifiChange;
    return wifi_set_significant_change_handler(id, handle, params, handler) == WIFI_SUCCESS;
}

static jboolean android_net_wifi_untrackSignificantWifiChange(
        JNIEnv *env, jclass cls, jint iface, jint id)  {

    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    ALOGD("resetting significant wifi change on interface[%d] = %p", iface, handle);

    return wifi_reset_significant_change_handler(id, handle) == WIFI_SUCCESS;
}

wifi_iface_stat link_stat;
wifi_radio_stat radio_stat; // L release has support for only one radio

void onLinkStatsResults(wifi_request_id id, wifi_iface_stat *iface_stat,
         int num_radios, wifi_radio_stat *radio_stats)
{
    if (iface_stat != 0) {
        memcpy(&link_stat, iface_stat, sizeof(wifi_iface_stat));
    } else {
        memset(&link_stat, 0, sizeof(wifi_iface_stat));
    }

    if (num_radios > 0 && radio_stats != 0) {
        memcpy(&radio_stat, radio_stats, sizeof(wifi_radio_stat));
    } else {
        memset(&radio_stat, 0, sizeof(wifi_radio_stat));
    }
}

static jobject android_net_wifi_getLinkLayerStats (JNIEnv *env, jclass cls, jint iface)  {

    wifi_stats_result_handler handler;
    memset(&handler, 0, sizeof(handler));
    handler.on_link_stats_results = &onLinkStatsResults;
    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    int result = wifi_get_link_stats(0, handle, handler);
    if (result < 0) {
        ALOGE("android_net_wifi_getLinkLayerStats: failed to get link statistics\n");
        return NULL;
    }

    jobject wifiLinkLayerStats = createObject(env, "android/net/wifi/WifiLinkLayerStats");
    if (wifiLinkLayerStats == NULL) {
       ALOGE("Error in allocating wifiLinkLayerStats");
       return NULL;
    }

    setIntField(env, wifiLinkLayerStats, "beacon_rx", link_stat.beacon_rx);
    setIntField(env, wifiLinkLayerStats, "rssi_mgmt", link_stat.rssi_mgmt);
    setLongField(env, wifiLinkLayerStats, "rxmpdu_be", link_stat.ac[WIFI_AC_BE].rx_mpdu);
    setLongField(env, wifiLinkLayerStats, "rxmpdu_bk", link_stat.ac[WIFI_AC_BK].rx_mpdu);
    setLongField(env, wifiLinkLayerStats, "rxmpdu_vi", link_stat.ac[WIFI_AC_VI].rx_mpdu);
    setLongField(env, wifiLinkLayerStats, "rxmpdu_vo", link_stat.ac[WIFI_AC_VO].rx_mpdu);
    setLongField(env, wifiLinkLayerStats, "txmpdu_be", link_stat.ac[WIFI_AC_BE].tx_mpdu);
    setLongField(env, wifiLinkLayerStats, "txmpdu_bk", link_stat.ac[WIFI_AC_BK].tx_mpdu);
    setLongField(env, wifiLinkLayerStats, "txmpdu_vi", link_stat.ac[WIFI_AC_VI].tx_mpdu);
    setLongField(env, wifiLinkLayerStats, "txmpdu_vo", link_stat.ac[WIFI_AC_VO].tx_mpdu);
    setLongField(env, wifiLinkLayerStats, "lostmpdu_be", link_stat.ac[WIFI_AC_BE].mpdu_lost);
    setLongField(env, wifiLinkLayerStats, "lostmpdu_bk", link_stat.ac[WIFI_AC_BK].mpdu_lost);
    setLongField(env, wifiLinkLayerStats, "lostmpdu_vi",  link_stat.ac[WIFI_AC_VI].mpdu_lost);
    setLongField(env, wifiLinkLayerStats, "lostmpdu_vo", link_stat.ac[WIFI_AC_VO].mpdu_lost);
    setLongField(env, wifiLinkLayerStats, "retries_be", link_stat.ac[WIFI_AC_BE].retries);
    setLongField(env, wifiLinkLayerStats, "retries_bk", link_stat.ac[WIFI_AC_BK].retries);
    setLongField(env, wifiLinkLayerStats, "retries_vi", link_stat.ac[WIFI_AC_VI].retries);
    setLongField(env, wifiLinkLayerStats, "retries_vo", link_stat.ac[WIFI_AC_VO].retries);


    setIntField(env, wifiLinkLayerStats, "on_time", radio_stat.on_time);
    setIntField(env, wifiLinkLayerStats, "tx_time", radio_stat.tx_time);
    setIntField(env, wifiLinkLayerStats, "rx_time", radio_stat.rx_time);
    setIntField(env, wifiLinkLayerStats, "on_time_scan", radio_stat.on_time_scan);

    return wifiLinkLayerStats;
}

static jint android_net_wifi_getSupportedFeatures(JNIEnv *env, jclass cls, jint iface) {
    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    feature_set set = 0;

    wifi_error result = WIFI_SUCCESS;
    /*
    set = WIFI_FEATURE_INFRA
        | WIFI_FEATURE_INFRA_5G
        | WIFI_FEATURE_HOTSPOT
        | WIFI_FEATURE_P2P
        | WIFI_FEATURE_SOFT_AP
        | WIFI_FEATURE_GSCAN
        | WIFI_FEATURE_PNO
        | WIFI_FEATURE_TDLS
        | WIFI_FEATURE_EPR;
    */

    result = wifi_get_supported_feature_set(handle, &set);
    if (result == WIFI_SUCCESS) {
        /* Temporary workaround for RTT capability */
        set = set | WIFI_FEATURE_D2AP_RTT;
        ALOGD("wifi_get_supported_feature_set returned set = 0x%x", set);
        return set;
    } else {
        ALOGD("wifi_get_supported_feature_set returned error = 0x%x", result);
        return 0;
    }
}

static void onRttResults(wifi_request_id id, unsigned num_results, wifi_rtt_result results[]) {
    JNIEnv *env = NULL;
    mVM->AttachCurrentThread(&env, NULL);

    ALOGD("onRttResults called, vm = %p, obj = %p, env = %p", mVM, mCls, env);

    jclass clsRttResult = (env)->FindClass("android/net/wifi/RttManager$RttResult");
    if (clsRttResult == NULL) {
        ALOGE("Error in accessing class");
        return;
    }

    jobjectArray rttResults = env->NewObjectArray(num_results, clsRttResult, NULL);
    if (rttResults == NULL) {
        ALOGE("Error in allocating array");
        return;
    }

    for (unsigned i = 0; i < num_results; i++) {

        wifi_rtt_result& result = results[i];

        jobject rttResult = createObject(env, "android/net/wifi/RttManager$RttResult");
        if (rttResult == NULL) {
            ALOGE("Error in creating rtt result");
            return;
        }

        char bssid[32];
        sprintf(bssid, "%02x:%02x:%02x:%02x:%02x:%02x", result.addr[0], result.addr[1],
            result.addr[2], result.addr[3], result.addr[4], result.addr[5]);

        setStringField(env, rttResult, "bssid", bssid);
        setIntField(env,  rttResult, "status",               result.status);
        setIntField(env,  rttResult, "requestType",          result.type);
        setLongField(env, rttResult, "ts",                   result.ts);
        setIntField(env,  rttResult, "rssi",                 result.rssi);
        setIntField(env,  rttResult, "rssi_spread",          result.rssi_spread);
        setIntField(env,  rttResult, "tx_rate",              result.tx_rate.bitrate);
        setLongField(env, rttResult, "rtt_ns",               result.rtt);
        setLongField(env, rttResult, "rtt_sd_ns",            result.rtt_sd);
        setLongField(env, rttResult, "rtt_spread_ns",        result.rtt_spread);
        setIntField(env,  rttResult, "distance_cm",          result.distance);
        setIntField(env,  rttResult, "distance_sd_cm",       result.distance_sd);
        setIntField(env,  rttResult, "distance_spread_cm",   result.distance_spread);

        env->SetObjectArrayElement(rttResults, i, rttResult);
    }

    reportEvent(env, mCls, "onRttResults", "(I[Landroid/net/wifi/RttManager$RttResult;)V",
        id, rttResults);
}

const int MaxRttConfigs = 16;

static jboolean android_net_wifi_requestRange(
        JNIEnv *env, jclass cls, jint iface, jint id, jobject params)  {

    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    ALOGD("sending rtt request [%d] = %p", id, handle);

    wifi_rtt_config configs[MaxRttConfigs];
    memset(&configs, 0, sizeof(configs));

    int len = env->GetArrayLength((jobjectArray)params);
    if (len > MaxRttConfigs) {
        return false;
    }

    for (int i = 0; i < len; i++) {

        jobject param = env->GetObjectArrayElement((jobjectArray)params, i);
        if (param == NULL) {
            ALOGD("could not get element %d", i);
            continue;
        }

        wifi_rtt_config &config = configs[i];

        parseMacAddress(env, param, config.addr);
        config.type = (wifi_rtt_type)getIntField(env, param, "requestType");
        config.peer = (wifi_peer_type)getIntField(env, param, "deviceType");
        config.channel.center_freq = getIntField(env, param, "frequency");
        config.channel.width = (wifi_channel_width)getIntField(env, param, "channelWidth");
        config.num_samples_per_measurement = getIntField(env, param, "num_samples");
        config.num_retries_per_measurement = getIntField(env, param, "num_retries");
    }

    wifi_rtt_event_handler handler;
    handler.on_rtt_results = &onRttResults;

    return wifi_rtt_range_request(id, handle, len, configs, handler) == WIFI_SUCCESS;
}

static jboolean android_net_wifi_cancelRange(
        JNIEnv *env, jclass cls, jint iface, jint id, jobject params)  {

    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    ALOGD("cancelling rtt request [%d] = %p", id, handle);

    mac_addr addrs[MaxRttConfigs];
    memset(&addrs, 0, sizeof(addrs));

    int len = env->GetArrayLength((jobjectArray)params);
    if (len > MaxRttConfigs) {
        return false;
    }

    for (int i = 0; i < len; i++) {

        jobject param = env->GetObjectArrayElement((jobjectArray)params, i);
        if (param == NULL) {
            ALOGD("could not get element %d", i);
            continue;
        }

        parseMacAddress(env, param, addrs[i]);
    }

    return wifi_rtt_range_cancel(id, handle, len, addrs) == WIFI_SUCCESS;
}

static jboolean android_net_wifi_setScanningMacOui(JNIEnv *env, jclass cls,
        jint iface, jbyteArray param)  {

    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    ALOGD("setting scan oui %p", handle);

    static const unsigned oui_len = 3;          /* OUI is upper 3 bytes of mac_address */
    int len = env->GetArrayLength(param);
    if (len != oui_len) {
        ALOGE("invalid oui length %d", len);
        return false;
    }

    jbyte* bytes = env->GetByteArrayElements(param, NULL);
    if (bytes == NULL) {
        ALOGE("failed to get array");
        return false;
    }

    return wifi_set_scanning_mac_oui(handle, (byte *)bytes) == WIFI_SUCCESS;
}

static jintArray android_net_wifi_getValidChannels(JNIEnv *env, jclass cls,
        jint iface, jint band)  {

    wifi_interface_handle handle = getIfaceHandle(env, cls, iface);
    ALOGD("getting valid channels %p", handle);

    static const int MaxChannels = 64;
    wifi_channel channels[64];
    int num_channels = 0;
    wifi_error result = wifi_get_valid_channels(handle, band, MaxChannels,
            channels, &num_channels);

    if (result == WIFI_SUCCESS) {
        jintArray channelArray = env->NewIntArray(num_channels);
        if (channelArray == NULL) {
            ALOGE("failed to allocate channel list");
            return NULL;
        }

        env->SetIntArrayRegion(channelArray, 0, num_channels, channels);
        return channelArray;
    } else {
        ALOGE("failed to get channel list : %d", result);
        return NULL;
    }
}

// ----------------------------------------------------------------------------

/*
 * JNI registration.
 */
static JNINativeMethod gWifiMethods[] = {
    /* name, signature, funcPtr */

    { "loadDriver", "()Z",  (void *)android_net_wifi_loadDriver },
    { "isDriverLoaded", "()Z",  (void *)android_net_wifi_isDriverLoaded },
    { "unloadDriver", "()Z",  (void *)android_net_wifi_unloadDriver },
    { "startSupplicant", "(Z)Z",  (void *)android_net_wifi_startSupplicant },
    { "killSupplicant", "(Z)Z",  (void *)android_net_wifi_killSupplicant },
    { "connectToSupplicantNative", "()Z", (void *)android_net_wifi_connectToSupplicant },
    { "closeSupplicantConnectionNative", "()V",
            (void *)android_net_wifi_closeSupplicantConnection },
    { "waitForEventNative", "()Ljava/lang/String;", (void*)android_net_wifi_waitForEvent },
    { "doBooleanCommandNative", "(Ljava/lang/String;)Z", (void*)android_net_wifi_doBooleanCommand },
    { "doIntCommandNative", "(Ljava/lang/String;)I", (void*)android_net_wifi_doIntCommand },
    { "doStringCommandNative", "(Ljava/lang/String;)Ljava/lang/String;",
            (void*) android_net_wifi_doStringCommand },
    { "startHalNative", "()Z", (void*) android_net_wifi_startHal },
    { "stopHalNative", "()V", (void*) android_net_wifi_stopHal },
    { "waitForHalEventNative", "()V", (void*) android_net_wifi_waitForHalEvents },
    { "getInterfacesNative", "()I", (void*) android_net_wifi_getInterfaces},
    { "getInterfaceNameNative", "(I)Ljava/lang/String;", (void*) android_net_wifi_getInterfaceName},
    { "getScanCapabilitiesNative", "(ILcom/android/server/wifi/WifiNative$ScanCapabilities;)Z",
            (void *) android_net_wifi_getScanCapabilities},
    { "startScanNative", "(IILcom/android/server/wifi/WifiNative$ScanSettings;)Z",
            (void*) android_net_wifi_startScan},
    { "stopScanNative", "(II)Z", (void*) android_net_wifi_stopScan},
    { "getScanResultsNative", "(IZ)[Landroid/net/wifi/ScanResult;",
            (void *) android_net_wifi_getScanResults},
    { "setHotlistNative", "(IILandroid/net/wifi/WifiScanner$HotlistSettings;)Z",
            (void*) android_net_wifi_setHotlist},
    { "resetHotlistNative", "(II)Z", (void*) android_net_wifi_resetHotlist},
    { "trackSignificantWifiChangeNative", "(IILandroid/net/wifi/WifiScanner$WifiChangeSettings;)Z",
            (void*) android_net_wifi_trackSignificantWifiChange},
    { "untrackSignificantWifiChangeNative", "(II)Z",
            (void*) android_net_wifi_untrackSignificantWifiChange},
    { "getWifiLinkLayerStatsNative", "(I)Landroid/net/wifi/WifiLinkLayerStats;",
            (void*) android_net_wifi_getLinkLayerStats},
    { "getSupportedFeatureSetNative", "(I)I",
            (void*) android_net_wifi_getSupportedFeatures},
    { "requestRangeNative", "(II[Landroid/net/wifi/RttManager$RttParams;)Z",
            (void*) android_net_wifi_requestRange},
    { "cancelRangeRequestNative", "(II[Landroid/net/wifi/RttManager$RttParams;)Z",
            (void*) android_net_wifi_cancelRange},
    { "setScanningMacOuiNative", "(I[B)Z", (void*) android_net_wifi_setScanningMacOui},
    { "getChannelsForBandNative", "(II)[I", (void*) android_net_wifi_getValidChannels}
};

int register_android_net_wifi_WifiNative(JNIEnv* env) {
    return AndroidRuntime::registerNativeMethods(env,
            "com/android/server/wifi/WifiNative", gWifiMethods, NELEM(gWifiMethods));
}


/* User to register native functions */
extern "C"
jint Java_com_android_server_wifi_WifiNative_registerNatives(JNIEnv* env, jclass clazz) {
    return AndroidRuntime::registerNativeMethods(env,
            "com/android/server/wifi/WifiNative", gWifiMethods, NELEM(gWifiMethods));
}

}; // namespace android
