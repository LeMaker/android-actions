LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := eng optional
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_PACKAGE_NAME := update
LOCAL_CERTIFICATE := platform
LOCAL_DEX_PREOPT := false
LOCAL_PROGUARD_ENABLED :=full
LOCAL_MODULE_PATH :=$(TARGET_OUT)/app
include $(BUILD_PACKAGE)
