# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= actionslogcat.cpp

LOCAL_SHARED_LIBRARIES := liblog libcutils

LOCAL_MODULE:= actionslogcat

LOCAL_MODULE_TAGS := eng optional

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

include $(BUILD_EXECUTABLE)
