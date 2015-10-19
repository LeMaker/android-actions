LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := src/com/android/omadm/service/DMIntent.java  \
                   src/com/android/omadm/service/DMResult.java  \
                   src/com/android/omadm/service/DMSettingsHelper.java \
                   src/com/android/omadm/service/DMDatabaseTable.java \

LOCAL_CERTIFICATE := platform
LOCAL_PROGUARD_FLAG_FILES := proguard.flags
LOCAL_MODULE := com.android.omadm.service.api

include $(BUILD_STATIC_JAVA_LIBRARY)

##############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := com.android.omadm.service.xml
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/permissions
LOCAL_SRC_FILES := ../config/com.android.omadm.service.xml
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-java-files-under, src)

# Sign with platform certificate to prevent disabling the app from Settings menu
LOCAL_PACKAGE_NAME := DMService
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true
LOCAL_PROGUARD_FLAG_FILES := proguard.flags
LOCAL_STATIC_JAVA_LIBRARIES := com.android.omadm.plugin \
                               com.android.omadm.plugin.dev \
                               com.android.omadm.pluginhelper \
                               com.android.omadm.plugin.diagmon

LOCAL_JAVA_LIBRARIES := telephony-common
LOCAL_JNI_SHARED_LIBRARIES := libdmengine libdmjavaplugin
LOCAL_REQUIRED_MODULES :=  dmAccounts.xml com.android.omadm.service.xml

LOCAL_MULTILIB := 32

include $(BUILD_PACKAGE)

# Use the following include to make our test apk.
#include $(call all-makefiles-under,$(LOCAL_PATH))
