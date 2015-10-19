LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PACKAGE_NAME := TvQuickSettings

LOCAL_MODULE_TAGS := optional
LOCAL_SDK_VERSION := current

LOCAL_SRC_FILES := \
    $(call all-java-files-under, src)

LOCAL_RESOURCE_DIR := \
    frameworks/support/v17/leanback/res \
    $(LOCAL_PATH)/res

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-v4 \
    android-support-v7-recyclerview \
    android-support-v17-leanback

LOCAL_AAPT_FLAGS += --auto-add-overlay --extra-packages android.support.v17.leanback

LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true

include $(BUILD_PACKAGE)
