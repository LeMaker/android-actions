LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OMX_TOP := $(LOCAL_PATH)

OMX_SYSTEM := $(TOP)/frameworks/native/include/media/openmax
OMX_AUDIODEC := $(TOP)/device/actions/common/hardware/omx/components/audio_decoder/include

OMX_INCLUDES := \
	$(OMX_SYSTEM) \
	$(OMX_AUDIODEC) \

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	src/library_entry_point.c  \
	src/downmix.c \
	src/omx_maddec_component.c \
	src/omx_maddec_audio_port.c \

LOCAL_C_INCLUDES := \
	$(OMX_INCLUDES) \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/include/alsp/inc \
	$(TOP)/frameworks/native/include \
	$(LOCAL_PATH)/../..\
	$(LOCAL_PATH)/../../base\
	$(LOCAL_PATH)/include \
	

LOCAL_SHARED_LIBRARIES :=       \
        libutils \
        liblog \
        libalc \
        libdl

LOCAL_STATIC_LIBRARIES :=       \
		libomxBellagio_base	\

LOCAL_MODULE_TAGS := eng optional

LOCAL_MODULE:= libOMX.Action.Audio.Decoder
#LOCAL_CFLAGS += -DOMX_IN_ANDROID
#LOCAL_CFLAGS += -DLOG_NDEBUG=0 -DOMX_IN_ANDROID

include $(BUILD_SHARED_LIBRARY)
