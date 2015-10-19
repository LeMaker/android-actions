LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	buf_op.c \
	config_1.c \
	device_op.c \
	display.c \
	interface.c \
	test_case_1.c \
	video_dev.c \
	ion.c

#LOCAL_STATIC_LIBRARIES:= libpthread

LOCAL_SHARED_LIBRARIES:= libion

LOCAL_C_INCLUDES +=
LOCAL_DLIBS = lpthread

LOCAL_MODULE:= camera_driver_test
LOCAL_MODULE_TAGS:= tests


LOCAL_CFLAGS += -Wall -fno-short-enums -O0 -g -D___ANDROID___

include $(BUILD_EXECUTABLE)
