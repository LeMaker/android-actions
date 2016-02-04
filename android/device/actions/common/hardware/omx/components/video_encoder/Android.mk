LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OMX_TOP := $(LOCAL_PATH)

OMX_SYSTEM := $(TOP)/frameworks/native/include/media/openmax
OMX_VCE := $(TOP)/device/actions/common/hardware/omx/components/video_encoder/include
OMX_CORE := $(TOP)/device/actions/common/hardware/omx
OMX_BASE := $(TOP)/device/actions/common/hardware/omx/base

OMX_INCLUDES := \
	$(OMX_SYSTEM) \
	$(OMX_VCE) \
	$(OMX_CORE) \
	$(OMX_BASE)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	vce_component/src/library_entry_point.c  \
	vce_component/src/omx_videoenc_component.c \
	vce_component/src/omx_videoenc_port.c \
	vce_component/src/buffer_mng.c \
	vce_component/src/omx_malloc.c \
	vce_component/src/frame_mng.c \
	vce_component/src/face_engine.c \
	vce_component/src/Actbuffer.c

LOCAL_C_INCLUDES := \
	$(OMX_INCLUDES) \
	$(TOP)/frameworks/av/include/alsp/inc \
	$(LOCAL_PATH)/vce_component/inc \
	$(LOCAL_PATH)/../../../libgralloc

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libalc \
	libdl \
	libgralloc

LOCAL_STATIC_LIBRARIES := \
	libomxBellagio_base

LOCAL_MODULE_TAGS := eng optional

LOCAL_MODULE:= libOMX.Action.Video.Encoder
#LOCAL_CFLAGS += -DOMX_IN_ANDROID
#LOCAL_CFLAGS += -DLOG_NDEBUG=0 -DOMX_IN_ANDROID

include $(BUILD_SHARED_LIBRARY)
