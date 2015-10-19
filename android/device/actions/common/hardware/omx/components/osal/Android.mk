LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	Actions_OSAL_Android.cpp \

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libActionsOMX_OSAL

LOCAL_CFLAGS :=


LOCAL_STATIC_LIBRARIES := liblog libcutils

LOCAL_C_INCLUDES :=\
	$(OMX_INCLUDES) \
	$(TOP)/hardware/libhardware/include/hardware \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/include/alsp/inc \
	$(TOP)/frameworks/native/include \
	$(TOP)/frameworks/native/include/media/hardware \
	$(TOP)/frameworks/native/include/media/openmax \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../..\
	$(LOCAL_PATH)/../../base\


include $(BUILD_STATIC_LIBRARY)
