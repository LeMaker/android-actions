LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_JAVA_LIBRARIES := com.android.omadm.plugin

LOCAL_MODULE:= com.android.omadm.plugin.diagmon

include $(BUILD_STATIC_JAVA_LIBRARY)
