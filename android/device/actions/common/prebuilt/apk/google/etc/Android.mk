LOCAL_PATH := $(call my-dir)
###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := google.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/preferred-apps
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

###############################################################################
include $(CLEAR_VARS)
LOCAL_MODULE := google_generic_update.txt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/updatecmds
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)
