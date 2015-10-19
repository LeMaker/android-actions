LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := eng optional
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_PACKAGE_NAME := ActSensorCalib
LOCAL_CERTIFICATE := platform
LOCAL_DEX_PREOPT := false
include $(BUILD_PACKAGE)
