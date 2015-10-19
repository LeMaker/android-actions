LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OMX_INCLUDES := \
	$(TOP)/frameworks/native/include/media/openmax

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	omxcore.c \

LOCAL_C_INCLUDES := \
	$(TOP)/frameworks/av/include/alsp/inc \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../ \
	$(OMX_INCLUDES) \
	
LOCAL_SHARED_LIBRARIES :=       \
        libutils \
        liblog \
        libdl	\
		libexpat
        
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libOMX_Core

include $(BUILD_SHARED_LIBRARY)
