LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OMX_SYSTEM := $(TOP)/frameworks/native/include/media/openmax
OMX_VIDEO := $(TOP)/device/actions/common/hardware/omx/components/video_decoder/include

OMX_INCLUDES := \
	$(OMX_SYSTEM) \
	$(OMX_VIDEO) \

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	src/library_entry_point.c  \
	src/omx_actvideodec_component.c \
	src/OMX_ActVideoFifo.c \
	src/utils.c \

LOCAL_C_INCLUDES := \
	$(OMX_INCLUDES) \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/include/alsp/inc \
	$(TOP)/frameworks/native/include/media/hardware \
	$(LOCAL_PATH)/../..\
	$(LOCAL_PATH)/../osal\
	$(LOCAL_PATH)/../../base\
	$(TOP)/device/actions/common/hardware/libgralloc \

	
	
LOCAL_SHARED_LIBRARIES :=       \
        libutils \
        libcutils \
        liblog \
        libalc \
        libui \
        libhardware\
        libdl \
        libACT_VceResize \
        libgralloc

LOCAL_STATIC_LIBRARIES :=       \
		libActionsOMX_OSAL	\
		libomxBellagio_base	\
		

LOCAL_MODULE_TAGS := eng optional

LOCAL_MODULE:= libOMX.Action.Video.Decoder

include $(BUILD_SHARED_LIBRARY)


######################################################################
include $(CLEAR_VARS)
OMX_SYSTEM := $(TOP)/frameworks/native/include/media/openmax
OMX_VIDEO := $(TOP)/device/actions/common/hardware/omx/components/video_decoder/video_decoder_4k/include

OMX_INCLUDES := \
	$(OMX_SYSTEM) \
	$(OMX_VIDEO) \

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	video_decoder_4k/src/library_entry_point_4k.c  \
	video_decoder_4k/src/omx_actvideodec_component_4k.c \
	video_decoder_4k/src/OMX_ActVideoFifo_4k.c \
	video_decoder_4k/src/utils_4k.c \

LOCAL_C_INCLUDES := \
	$(OMX_INCLUDES) \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/include/alsp/inc \
	$(TOP)/frameworks/native/include/media/hardware \
	$(LOCAL_PATH)/../..\
	$(LOCAL_PATH)/../osal\
	$(LOCAL_PATH)/../../base\
	$(TOP)/device/actions/common/hardware/libgralloc \

	
	
LOCAL_SHARED_LIBRARIES :=       \
        libutils \
        libcutils \
        liblog \
        libalc \
        libui \
        libhardware\
        libdl \
        libACT_VceResize \
        libgralloc

LOCAL_STATIC_LIBRARIES :=       \
		libomxBellagio_base	\
		libActionsOMX_OSAL	\
		

LOCAL_MODULE_TAGS := eng optional

LOCAL_MODULE:= libOMX.Action.Video.Decoder.Deinterlace

include $(BUILD_SHARED_LIBRARY)