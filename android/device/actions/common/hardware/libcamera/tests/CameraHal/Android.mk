LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	camera_test_menu.cpp \
	camera_test_script.cpp

LOCAL_SHARED_LIBRARIES:= \
	libdl \
	libui \
	libutils \
	libcutils \
	libbinder \
	libmedia \
	libui \
	libgui \
	libcamera_client

LOCAL_C_INCLUDES += \
	frameworks/base/include/ui \
	frameworks/base/include/surfaceflinger \
	frameworks/base/include/camera \
	frameworks/base/include/media \
	$(PV_INCLUDES)

ifeq ($(PLATFORM_SDK_VERSION),16)
LOCAL_CFLAGS += -DCAMERA_ANDROID16
endif

LOCAL_MODULE:= camera_test
LOCAL_MODULE_TAGS:= tests

LOCAL_CFLAGS += -Wall -fno-short-enums -Wno-write-strings -g 


include $(BUILD_EXECUTABLE)


