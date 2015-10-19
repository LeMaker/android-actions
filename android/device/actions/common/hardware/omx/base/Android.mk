LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OMX_TOP := $(LOCAL_PATH)

OMX_SYSTEM := $(TOP)/frameworks/native/include/media/openmax

OMX_INCLUDES := \
	$(OMX_SYSTEM) \

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	omx_base_audio_port.c \
	omx_base_clock_port.c \
	omx_base_component.c \
	omx_base_filter.c  \
	omx_base_image_port.c \
	omx_base_port.c \
	omx_base_sink.c \
	omx_base_source.c \
	omx_base_video_port.c \
	../content_pipe_file.c \
	../content_pipe_inet.c \
	../queue.c \
	../tsemaphore.c\

LOCAL_C_INCLUDES := \
	$(OMX_INCLUDES) \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/include/alsp/inc \
	$(TOP)/frameworks/native/include \
	$(LOCAL_PATH)/..\

LOCAL_SHARED_LIBRARIES :=       \
        libutils \
        liblog \
        libalc \
        libdl
        
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libomxBellagio_base

include $(BUILD_STATIC_LIBRARY)
