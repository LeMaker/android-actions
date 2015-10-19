# Copyright 2005 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= trace_readahead.c values.c readahead-blocks.c

LOCAL_C_INCLUDES :=$(LOCAL_PATH)/include

LOCAL_CFLAGS := -Wall -Wno-unused-parameter -std=gnu99
LOCAL_MODULE := treadahead
LOCAL_MODULE_TAGS := eng

LOCAL_SHARED_LIBRARIES := libcutils libc libsqlite

include $(BUILD_EXECUTABLE)
