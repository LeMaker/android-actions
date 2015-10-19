LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    aac_check.c \
	dts_check.c \
 	format_check.c \
 	mp3_check.c \
 	rm_check.c \
 	ts_check.c \
 	wma_check.c

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/include \
        $(call include-path-for, alsp)

LOCAL_MODULE := libmmminfo

include $(BUILD_STATIC_LIBRARY)
