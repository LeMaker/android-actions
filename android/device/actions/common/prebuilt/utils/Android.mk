LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE :=i2cdetect
LOCAL_SRC_FILES   := i2cdev-detect.sh
LOCAL_MODULE_STEM:=i2cdev-detect.sh
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin
include $(BUILD_PREBUILT)



include $(CLEAR_VARS)
LOCAL_MODULE :=libactions-ril
ifneq ($(filter $(TARGET_PRODUCT),full_gs900a),)
LOCAL_SRC_FILES_64 := libactions-ril_64.so
LOCAL_SRC_FILES_32 := libactions-ril.so
LOCAL_MULTILIB := both
else
LOCAL_SRC_FILES   := libactions-ril.so
endif
LOCAL_MODULE_STEM :=libactions-ril.so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE :=acc_policy
LOCAL_SRC_FILES :=acc_policy.dat
LOCAL_MODULE_STEM := acc_policy.dat
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE :=gpu_config
LOCAL_SRC_FILES :=gpu_config
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE :=locales_list.txt
LOCAL_SRC_FILES :=locales_list.txt
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/
include $(BUILD_PREBUILT)

#include $(CLEAR_VARS)
#LOCAL_MODULE :=actionsframework
#LOCAL_MODULE_STEM :=actionsframework.jar
#LOCAL_SRC_FILES   := actionsframework.jar
#LOCAL_MODULE_TAGS :=eng optional
#LOCAL_MODULE_CLASS :=JAVA_LIBRARIES
#LOCAL_MODULE_PATH := $(TARGET_OUT)/framework
#include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := actionsframework_org:actionsframework_org.jar
include $(BUILD_MULTI_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := actionsframework
LOCAL_MODULE_CLASS := JAVA_LIBRARIES
LOCAL_NO_STANDARD_LIBRARIES := true
LOCAL_STATIC_JAVA_LIBRARIES := actionsframework_org
LOCAL_DX_FLAGS := --core-library
include $(BUILD_JAVA_LIBRARY)


ifeq ($(strip $(ACTIONS_RELEASE_BUILD_FOR_CUSTOM)), true)



#omx/components/audio_decorder
include $(CLEAR_VARS)
LOCAL_MODULE :=libOMX.Action.Audio.Decoder
LOCAL_SRC_FILES   := libOMX.Action.Audio.Decoder.so
LOCAL_MODULE_STEM:=libOMX.Action.Audio.Decoder.so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)

#omx/components/camera
include $(CLEAR_VARS)
LOCAL_MODULE :=libOMX.Action.Video.Camera
LOCAL_SRC_FILES   := libOMX.Action.Video.Camera.so
LOCAL_MODULE_STEM:=libOMX.Action.Video.Camera.so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE :=libACT_V4L2HAL
LOCAL_SRC_FILES   := libACT_V4L2HAL.so
LOCAL_MODULE_STEM:=libACT_V4L2HAL.so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)

#omx/components/jpeg_decorder
include $(CLEAR_VARS)
LOCAL_MODULE :=libOMX.Action.Image.Decoder
LOCAL_SRC_FILES   := libOMX.Action.Image.Decoder.so
LOCAL_MODULE_STEM:=libOMX.Action.Image.Decoder.so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)

#omx/components/omx_vce
include $(CLEAR_VARS)
LOCAL_MODULE :=libOMX.Action.Video.Encoder
LOCAL_SRC_FILES   := libOMX.Action.Video.Encoder.so
LOCAL_MODULE_STEM:=libOMX.Action.Video.Encoder.so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE :=libACT_EncAPI
LOCAL_SRC_FILES   := libACT_EncAPI.so
LOCAL_MODULE_STEM:=libACT_EncAPI.so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)

#omx/components/video_decorder
include $(CLEAR_VARS)
LOCAL_MODULE :=libOMX.Action.Video.Decoder
LOCAL_SRC_FILES   := libOMX.Action.Video.Decoder.so
LOCAL_MODULE_STEM:=libOMX.Action.Video.Decoder.so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)

include $(CLEAR_VARS) 
LOCAL_MODULE :=libOMX.Action.Video.Decoder.Enhance 
LOCAL_SRC_FILES   := libOMX.Action.Video.Decoder.Enhance.so 
LOCAL_MODULE_STEM:=libOMX.Action.Video.Decoder.Enhance.so 
LOCAL_MODULE_TAGS :=eng optional 
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES 
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib 
include $(BUILD_PREBUILT) 

#omx/libstagefreighthw
include $(CLEAR_VARS)
LOCAL_MODULE :=libstagefrighthw
LOCAL_SRC_FILES   := libstagefrighthw.so
LOCAL_MODULE_STEM:=libstagefrighthw.so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)


#omx/omx_core
include $(CLEAR_VARS)
LOCAL_MODULE :=libOMX_Core
LOCAL_SRC_FILES   := libOMX_Core.so
LOCAL_MODULE_STEM:=libOMX_Core.so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)


#gralloc_HW
include $(CLEAR_VARS)
LOCAL_MODULE :=gralloc.$(TARGET_BOARD_PLATFORM)
LOCAL_SRC_FILES   := gralloc.$(TARGET_BOARD_PLATFORM).so
LOCAL_MODULE_STEM:=gralloc.$(TARGET_BOARD_PLATFORM).so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib/hw
include $(BUILD_PREBUILT)


#omx/base
include $(CLEAR_VARS)
LOCAL_MODULE    := libomxBellagio_base  
LOCAL_SRC_FILES := libomxBellagio_base.a
LOCAL_MODULE_STEM:=libomxBellagio_base.a
LOCAL_MODULE_CLASS :=STATIC_LIBRARIES
LOCAL_MODULE_TAGS :=eng optional
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE :=pfmnceserver
LOCAL_SRC_FILES :=pfmnceserver
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=etc
LOCAL_MODULE_PATH := $(TARGET_OUT)/bin/
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE :=libjni_tinyplanet
LOCAL_SRC_FILES   := libjni_tinyplanet.so
LOCAL_MODULE_STEM:=libjni_tinyplanet.so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)

endif	
utils_prebuilt_target :=

#if source code not available do prebuilt
ifneq ($(words $(shell find device/actions/common/packages/OTA/Android.mk)),1)
include $(CLEAR_VARS)
LOCAL_MODULE :=update
LOCAL_MODULE_STEM:=update.apk
LOCAL_SRC_FILES :=update.apk
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
include $(BUILD_PREBUILT)
else
utils_prebuilt_target += $(TARGET_OUT)/app/update.apk
endif


#prebuilt libperformance 
ifneq ($(words $(shell find device/actions/common/performancemanager/performanceservice/Android.mk)),1)
include $(CLEAR_VARS)
LOCAL_MODULE := libperformance
LOCAL_SRC_FILES   := $(LOCAL_MODULE).so
LOCAL_MODULE_STEM:=$(LOCAL_MODULE).so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)
else
utils_prebuilt_target += $(TARGET_OUT)/lib/libperformance.so
endif

ifneq ($(words $(shell find frameworks/base/libs/hwui/Android.mk)),1)
include $(CLEAR_VARS)
LOCAL_MODULE :=libhwui
LOCAL_SRC_FILES   := libhwui.so
LOCAL_MODULE_STEM:=libhwui.so
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
include $(BUILD_PREBUILT)
endif


#prebuilt ActSensorCalib 
ifneq ($(words $(shell find device/actions/common/packages/ActSensorCalib/Android.mk)),1)
include $(CLEAR_VARS)
LOCAL_MODULE :=ActSensorCalib
LOCAL_MODULE_STEM:=ActSensorCalib.apk
LOCAL_SRC_FILES :=ActSensorCalib.apk
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
include $(BUILD_PREBUILT)
else
utils_prebuilt_target += $(TARGET_OUT)/app/ActSensorCalib.apk
endif

.PHONY: copy_to_actions_prebuilt
copy_to_actions_prebuilt:$(utils_prebuilt_target)
	-cp -r $(utils_prebuilt_target)   device/actions/common/prebuilt/utils/
