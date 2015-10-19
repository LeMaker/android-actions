LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE       := pvrsrvctl
LOCAL_SRC_FILES    := vendor/bin/pvrsrvctl
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/bin
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := libsrv_init
LOCAL_SRC_FILES    := vendor/lib/libsrv_init.so
LOCAL_MODULE_STEM  := libsrv_init.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := libpvr2d
LOCAL_SRC_FILES    := vendor/lib/libpvr2d.so
LOCAL_MODULE_STEM  := libpvr2d.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := libsrv_um
LOCAL_SRC_FILES    := vendor/lib/libsrv_um.so
LOCAL_MODULE_STEM  := libsrv_um.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := libusc
LOCAL_SRC_FILES    := vendor/lib/libusc.so
LOCAL_MODULE_STEM  := libusc.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := libglslcompiler
LOCAL_SRC_FILES    := vendor/lib/libglslcompiler.so
LOCAL_MODULE_STEM  := libglslcompiler.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := libIMGegl
LOCAL_SRC_FILES    := vendor/lib/libIMGegl.so
LOCAL_MODULE_STEM  := libIMGegl.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := libpvrANDROID_WSEGL
LOCAL_SRC_FILES    := vendor/lib/libpvrANDROID_WSEGL.so
LOCAL_MODULE_STEM  := libpvrANDROID_WSEGL.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := libPVRScopeServices
LOCAL_SRC_FILES    := vendor/lib/libPVRScopeServices.so
LOCAL_MODULE_STEM  := libPVRScopeServices.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE       := libPVROCL
#LOCAL_SRC_FILES    := vendor/lib/libPVROCL.so
#LOCAL_MODULE_STEM  := libPVROCL.so
#LOCAL_MODULE_TAGS  := eng optional
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
#include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE       := liboclcompiler
#LOCAL_SRC_FILES    := vendor/lib/liboclcompiler.so
#LOCAL_MODULE_STEM  := liboclcompiler.so
#LOCAL_MODULE_TAGS  := eng optional
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
#include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE       := librsccompiler
#LOCAL_SRC_FILES    := vendor/lib/librsccompiler.so
#LOCAL_MODULE_STEM  := librsccompiler.so
#LOCAL_MODULE_TAGS  := eng optional
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
#include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE       := libPVRRS
#LOCAL_SRC_FILES    := vendor/lib/libPVRRS.so
#LOCAL_MODULE_STEM  := libPVRRS.so
#LOCAL_MODULE_TAGS  := eng optional
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
#include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE       := librsccore
#LOCAL_SRC_FILES    := vendor/lib/librsccore.bc
#LOCAL_MODULE_STEM  := librsccore.bc
#LOCAL_MODULE_TAGS  := eng optional
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib
#include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := memtrack.S500
LOCAL_SRC_FILES    := vendor/lib/hw/memtrack.S500.so
LOCAL_MODULE_STEM  := memtrack.S500.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib/hw
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := hwcomposer.S500
LOCAL_SRC_FILES    := vendor/lib/hw/hwcomposer.S500.so
LOCAL_MODULE_STEM  := hwcomposer.S500.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib/hw
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := libEGL_POWERVR_SGX544_115
LOCAL_SRC_FILES    := vendor/lib/egl/libEGL_POWERVR_SGX544_115.so 
LOCAL_MODULE_STEM  := libEGL_POWERVR_SGX544_115.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib/egl
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := libGLESv1_CM_POWERVR_SGX544_115
LOCAL_SRC_FILES    := vendor/lib/egl/libGLESv1_CM_POWERVR_SGX544_115.so
LOCAL_MODULE_STEM  := libGLESv1_CM_POWERVR_SGX544_115.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib/egl
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := libGLESv2_POWERVR_SGX544_115
LOCAL_SRC_FILES    := vendor/lib/egl/libGLESv2_POWERVR_SGX544_115.so
LOCAL_MODULE_STEM  := libGLESv2_POWERVR_SGX544_115.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib/egl
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE       := gralloc.S500
LOCAL_SRC_FILES    := vendor/lib/hw/gralloc.S500.so
LOCAL_MODULE_STEM  := gralloc.S500.so
LOCAL_MODULE_TAGS  := eng optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH  := $(TARGET_OUT)/vendor/lib/hw
include $(BUILD_PREBUILT)

#gpu related
gpu_PRODUCT_PACKAGES += \
	powervr.ini \
	pvrsrvctl \
	libsrv_init \
	libpvr2d \
	libsrv_um \
	libusc \
	libglslcompiler \
	libIMGegl \
	libpvrANDROID_WSEGL \
	libEGL_POWERVR_SGX544_115 \
	libGLESv1_CM_POWERVR_SGX544_115 \
	libGLESv2_POWERVR_SGX544_115 \
	gralloc.S500 \
	liboclcompiler \
	memtrack.S500 \
	hwcomposer.S500 \
	librsccompiler \
	libPVRRS \
	libPVRScopeServices \
	librsccore

include $(CLEAR_VARS)
LOCAL_MODULE := libgpu_prebuilt
LOCAL_MODULE_TAGS := eng optional
LOCAL_REQUIRED_MODULES := $(gpu_PRODUCT_PACKAGES)
include $(BUILD_PHONY_PACKAGE)
