LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	PerformanceServer.cpp 

LOCAL_SHARED_LIBRARIES := \
	libperformance	\
	libutils \
	libbinder 
	

actions_only_service_base := $(LOCAL_PATH)/..
actions_service_base := $(LOCAL_PATH)/../../base
android_service_base := $(LOCAL_PATH)/../../../../../frameworks/base

LOCAL_C_INCLUDES := \
    $(actions_only_service_base)/include	\
	$(actions_only_service_base)/performanceservice
	
LOCAL_MODULE_TAGS := eng optional
LOCAL_MODULE:= pfmnceserver

include $(BUILD_EXECUTABLE)
