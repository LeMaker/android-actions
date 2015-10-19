LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

OMX_TOP := $(LOCAL_PATH)
#OMX_INCLUDE := $(OMX_TOP)/include
OMX_CAMERAINC := $(OMX_TOP)/components/camera/include

OMX_BASE := $(OMX_TOP)/base
OMX_CORE := $(OMX_TOP)/omx_core
OMX_OSAL := $(OMX_TOP)/components/osal
OMX_CAMERA := $(OMX_TOP)/components/camera
OMX_VCE := $(OMX_TOP)/components/omx_vce
OMX_AUDIO_DEC := $(OMX_TOP)/components/audio_decoder
OMX_VIDEO_DEC := $(OMX_TOP)/components/video_decoder
OMX_JPEG_DEC := $(OMX_TOP)/components/jpeg_decoder
OMX_HW := $(OMX_TOP)/libstagefrighthw

OMX_COMP_C_INCLUDES := \
#	$(OMX_INCLUDES) \
	$(OMX_CAMERAINC) \

include $(OMX_HW)/Android.mk
include $(OMX_BASE)/Android.mk
include $(OMX_CORE)/Android.mk
include $(OMX_OSAL)/Android.mk
include $(OMX_CAMERA)/Android.mk
include $(OMX_AUDIO_DEC)/Android.mk
include $(OMX_VIDEO_DEC)/Android.mk
include $(OMX_JPEG_DEC)/Android.mk
#include $(OMX_V4L2HAL)/Android.mk
#include $(OMX_ISPHAL)/Android.mk
#include $(OMX_VENCODER)/Android.mk
include $(OMX_VCE)/Android.mk
#include $(OMX_FACE)/Android.mk
