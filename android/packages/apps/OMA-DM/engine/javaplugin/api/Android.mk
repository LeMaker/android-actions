LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-subdir-java-files) \
                   com/android/omadm/plugin/IDMClientService.aidl \
                   com/android/omadm/plugin/IDmtPlugin.aidl

#LOCAL_JAVA_RESOURCE_DIRS := resources

#LOCAL_JAVA_LIBRARIES := com.android.omadm.plugin
#LOCAL_STATIC_JAVA_LIBRARIES := com.android.omadm.plugin

LOCAL_MODULE:= com.android.omadm.plugin

#LOCAL_DX_FLAGS := --core-library

#include $(BUILD_JAVA_LIBRARY)
include $(BUILD_STATIC_JAVA_LIBRARY)
