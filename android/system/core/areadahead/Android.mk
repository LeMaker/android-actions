# Copyright 2005 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	readahead-blocks.c

LOCAL_CFLAGS := -Wall -Wno-unused-parameter -std=gnu99
LOCAL_MODULE := areadahead
LOCAL_MODULE_TAGS := eng

LOCAL_SHARED_LIBRARIES := libcutils libc

include $(BUILD_EXECUTABLE)

