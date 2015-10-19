LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES_32 := 32/libactions_video_opt.a
LOCAL_SRC_FILES_64 := 64/libactions_video_opt.a
LOCAL_BUILT_MODULE_STEM := libactions_video_opt.a
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE := libactions_video_opt
LOCAL_MULTILIB := both
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
include $(BUILD_PREBUILT)