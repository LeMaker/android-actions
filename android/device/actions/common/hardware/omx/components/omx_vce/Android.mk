LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OMX_TOP := $(LOCAL_PATH)

OMX_SYSTEM := $(TOP)/frameworks/native/include/media/openmax
OMX_VCE := $(TOP)/device/actions/common/hardware/omx/components/omx_vce/include
OMX_CORE := $(TOP)/device/actions/common/hardware/omx

OMX_INCLUDES := \
	$(OMX_SYSTEM) \
	$(OMX_VCE) \
	$(OMX_CORE) 


LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	vce_component/src/library_entry_point.c  \
	vce_component/src/omx_videoenc_component.c \
	vce_component/src/omx_videoenc_port.c \
	vce_component/src/buffer_mng.c \
	vce_component/src/omx_malloc.c \
	vce_component/src/frame_mng.c \
	vce_component/src/face_engine.c \
	vce_component/src/Actbuffer.c \
	vce_component/src/resize.c
	
LOCAL_C_INCLUDES := \
	$(OMX_INCLUDES) \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/include/alsp/inc \
	$(TOP)/frameworks/native/include \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/vce_component/inc\
	$(LOCAL_PATH)/../..\
	$(LOCAL_PATH)/../../base\
	$(LOCAL_PATH)/../../../libgralloc

	
LOCAL_SHARED_LIBRARIES :=       \
        libutils \
        liblog \
        libalc \
        libui \
        libhardware\
        libdl\
        libgralloc

LOCAL_STATIC_LIBRARIES :=       \
		libomxBellagio_base

LOCAL_MODULE_TAGS := eng optional

LOCAL_MODULE:= libOMX.Action.Video.Encoder
#LOCAL_CFLAGS += -DOMX_IN_ANDROID
#LOCAL_CFLAGS += -DLOG_NDEBUG=0 -DOMX_IN_ANDROID

include $(BUILD_SHARED_LIBRARY)

######################################################################

include $(CLEAR_VARS)

OMX_TOP := $(LOCAL_PATH)

OMX_SYSTEM := $(TOP)/frameworks/native/include/media/openmax
OMX_CORE   := $(TOP)/device/actions/common/hardware/omx
OMX_VCE := $(TOP)/device/actions/common/hardware/omx/components/omx_vce/include

OMX_INCLUDES := \
	$(OMX_SYSTEM) \
	$(OMX_CORE) \
	$(OMX_VCE)
	
LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	vce_resize/omx_malloc.c\
	vce_resize/vce_resize.c\
	vce_resize/drv_resize.c
	
LOCAL_C_INCLUDES := \
	$(OMX_INCLUDES)\
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/include/alsp/inc \
	$(TOP)/frameworks/native/include \
	$(LOCAL_PATH)/../../\
	$(LOCAL_PATH)/../../base\
	$(LOCAL_PATH)/../../../libgralloc
	
LOCAL_SHARED_LIBRARIES :=       \
        libutils \
        liblog \
        libalc \
        libdl\
        libgralloc

LOCAL_MODULE_TAGS := eng optional

LOCAL_MODULE:= libACT_VceResize

include $(BUILD_STATIC_LIBRARY)
#include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_EXECUTABLE)
