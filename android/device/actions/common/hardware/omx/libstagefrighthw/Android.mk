LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng optional

LOCAL_SRC_FILES := \
    ActionOMXPlugin.cpp \


OMX_INCLUDES := $(TOP)/frameworks/native/include/media/openmax


LOCAL_C_INCLUDES += \
	  $(OMX_INCLUDES) \
    $(TOP)/frameworks/av/include \
    $(TOP)/frameworks/native/include \
    $(TOP)/frameworks/native/include/media/hardware

LOCAL_SHARED_LIBRARIES := \
    libbinder               \
    liblog \
    libutils                \
    libcutils               \
    libui                   \
    libdl                   \
    

LOCAL_MODULE := libstagefrighthw

include $(BUILD_SHARED_LIBRARY)

