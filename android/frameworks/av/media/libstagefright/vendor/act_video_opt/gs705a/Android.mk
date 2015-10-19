LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libactions_video_opt.a
LOCAL_MODULE_TAGS := eng optional
include $(BUILD_MULTI_PREBUILT)
