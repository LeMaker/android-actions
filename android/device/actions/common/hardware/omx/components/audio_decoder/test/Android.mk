LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
OMX_TOP := $(LOCAL_PATH)/../../../../../
OMX_SYSTEM := $(OMX_TOP)/system/src/openmax_il
OMX_AUDIO := $(OMX_TOP)/audio/src/openmax_il
OMX_VIDEO := $(OMX_TOP)/video/src/openmax_il
DRV_CODA := $(OMX_TOP)/video/src/coda

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	libparser.c \
	omxaudiodectest.c \
	storageio.c \
	
	
LOCAL_C_INCLUDES := \
	$(OMX_VIDEO)/audio_decoder/inc/  \
	$(OMX_VIDEO)/audio_decoder/inc/inc/  \
	$(DRV_CODA)/inc \
	$(OMX_SYSTEM)/omx_core/inc \
	$(OMX_SYSTEM)/common/inc \
	$(TOP)/frameworks/base/include

LOCAL_SHARED_LIBRARIES := $(ACTION_OMX_COMP_SHARED_LIBRARIES) \
        libAction_OMX_Core   \


LOCAL_CFLAGS := $(ACTION_OMX_CFLAGS) -DOMX_DEBUG
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:= ActAudioDecTest_common

include $(BUILD_EXECUTABLE)
