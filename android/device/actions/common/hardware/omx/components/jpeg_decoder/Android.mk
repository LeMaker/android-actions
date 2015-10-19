LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OMX_TOP := $(LOCAL_PATH)

OMX_SYSTEM := $(TOP)/frameworks/native/include/media/openmax

OMX_INCLUDES := \
	$(OMX_SYSTEM)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	src/OMX_ActJpegDecoder.c \
	src/omx_jpegdec_component.c \
	src/library_entry_point.c

LOCAL_C_INCLUDES := \
	$(OMX_INCLUDES) \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/include/alsp/inc \
	$(TOP)/frameworks/native/include \
	$(LOCAL_PATH)/../..\
	$(LOCAL_PATH)/../../base\
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/../../omx-include
	
	
	
LOCAL_SHARED_LIBRARIES :=       \
        libutils \
        liblog \
        libalc \
        libdl

LOCAL_STATIC_LIBRARIES :=       \
		libomxBellagio_base	\

LOCAL_MODULE_TAGS := eng optional

LOCAL_MODULE:= libOMX.Action.Image.Decoder
#LOCAL_CFLAGS += -DOMX_IN_ANDROID
#LOCAL_CFLAGS += -DLOG_NDEBUG=0 -DOMX_IN_ANDROID

include $(BUILD_SHARED_LIBRARY)
