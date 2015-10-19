LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OMX_TOP := $(LOCAL_PATH)

OMX_SYSTEM := $(TOP)/frameworks/native/include/media/openmax
OMX_CAMERA := $(TOP)/device/actions/common/hardware/omx/components/camera/include

OMX_INCLUDES := \
	$(OMX_SYSTEM) \
	$(OMX_CAMERA) \

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	src/library_entry_point.c  \
	src/omx_camera_component.c \
	src/omx_base_camera_video_port.c \
	act_filter/actim_resize.c \
	act_isp/ispctl_adapter.c \
	act_isp/imxapi_adapter.c \
	utils/watch_dog.c 

LOCAL_C_INCLUDES := \
	$(OMX_INCLUDES) \
	$(LOCAL_PATH)/../..\
	$(LOCAL_PATH)/../osal\
	$(LOCAL_PATH)/../../base\
	$(LOCAL_PATH)/act_filter \
	$(LOCAL_PATH)/act_isp \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/include/alsp/inc \
	$(TOP)/frameworks/native/include \
	$(TOP)/device/actions/common/hardware/libgralloc \
	$(LOCAL_PATH)/utils
	
LOCAL_SHARED_LIBRARIES :=       \
        libutils \
        libcutils \
        liblog \
        libalc \
        libui \
        libhardware\
        libdl \
        libgralloc

LOCAL_STATIC_LIBRARIES :=       \
		libActionsOMX_OSAL	\
		libomxBellagio_base	\

LOCAL_MODULE_TAGS := eng optional

LOCAL_MODULE:= libOMX.Action.Video.Camera
#LOCAL_CFLAGS += -DOMX_IN_ANDROID
#LOCAL_CFLAGS += -DLOG_NDEBUG=0 -DOMX_IN_ANDROID

include $(BUILD_SHARED_LIBRARY)


######################################################################

include $(CLEAR_VARS)

OMX_TOP := $(LOCAL_PATH)

OMX_SYSTEM := $(TOP)/frameworks/native/include/media/openmax
OMX_CAMERA := $(TOP)/device/actions/common/hardware/omx/components/camera/include

OMX_INCLUDES := \
	$(OMX_SYSTEM) \
	$(OMX_CAMERA) \

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	v4l2_camera/base_v4l2_module.c  \
	v4l2_camera/isp_camera.c \
	v4l2_camera/v4l2_camera.c \
	utils/watch_dog.c 
	
LOCAL_C_INCLUDES := \
	$(OMX_INCLUDES) \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/include/alsp/inc \
	$(TOP)/frameworks/native/include \
	$(LOCAL_PATH)/../.. \
	$(LOCAL_PATH)/../../base \
	$(LOCAL_PATH)/act_isp \
	$(LOCAL_PATH)/utils
	
LOCAL_SHARED_LIBRARIES :=       \
        libutils \
        liblog \
        libalc \
        libdl

LOCAL_MODULE_TAGS := eng optional

LOCAL_MODULE:= libACT_V4L2HAL
#LOCAL_CFLAGS += -DOMX_IN_ANDROID
#LOCAL_CFLAGS += -DLOG_NDEBUG=0 -DOMX_IN_ANDROID

include $(BUILD_SHARED_LIBRARY)

