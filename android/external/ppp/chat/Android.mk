#ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

TARGET_PRELINK_MODULE := false
LOCAL_SRC_FILES := chat.c

LOCAL_SHARED_LIBRARIES := \
		libcutils\
		libutils

#LOCAL_C_INCLUDES :=  $(KERNEL_HEADERS)

LOCAL_CFLAGS := -DANDROID_CHANGES -DCHAPMS=1 -DMPPE=1 -Iexternal/openssl/include

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := chat

include $(BUILD_EXECUTABLE)

#endif
