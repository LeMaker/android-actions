# Copyright (C) 2014 Actions
 
LOCAL_PATH:= $(call my-dir)
# RTK mac

include $(CLEAR_VARS)
LOCAL_SRC_FILES := setmacaddr.c 
LOCAL_MODULE := setmacaddr
LOCAL_MODULE_TAGS := optional setmacaddr
LOCAL_SHARED_LIBRARIES := liblog libcutils
include $(BUILD_EXECUTABLE)

