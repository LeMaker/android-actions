LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	preview_test.cpp 

LOCAL_SHARED_LIBRARIES:= \
	libdl \
	libui \
	libutils \
	libcutils \
	libbinder \
	libmedia \
	libmedia_native \
	libui \
	libgui \
	libcamera_client

LOCAL_C_INCLUDES += \
	frameworks/base/include/ui \
	frameworks/base/include/surfaceflinger \
	frameworks/base/include/camera \
	frameworks/base/include/media \
	$(PV_INCLUDES)

LOCAL_MODULE:= camerahal_preview_test
LOCAL_MODULE_TAGS:= tests


LOCAL_CFLAGS += -Wall -fno-short-enums -O0 -g -D___ANDROID___

include $(BUILD_EXECUTABLE)


