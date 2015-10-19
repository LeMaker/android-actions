
#ifeq ($(TARGET_BOARD_PLATFORM),S500)

LOCAL_PATH:= $(call my-dir)

CAMERA_NEON_INS := false

CAMERA_SET_FREQ_RANGE := false
CAMERA_SET_GPUOUTSTANDING := false
CAMERA_VCE_OMX_FD := true
CAMERA_VCE_OMX_JPEG := true
CAMERA_VCE_OMX_RESIZE := true

CAMERA_RUN_IN_EMULATOR := false

CAMERA_USE_TEST_OMX := false

CAMERA_HEAPTRACKER := false

CAMERA_SENSOR_LISTENER := true

CAMERA_DISPLAY_RGB565 := false

#android log or terminal log
CAMERA_ANDROID_LOG := true


CAMERA_IMAGE_SNAPSHOT := true

CAMERA_VIDEO_SNAPSHOT := true

CAMERA_FOCUS_AREA := true
CAMERA_FOCUS_MODE_INFINITY := true
CAMERA_FOCUS_MODE_AUTO := true

CAMERA_FPS_FROM_OMX := false

CAMERA_HDR := true

CAMERA_PROFILES_AUTOGEN := true

#fps statistic
CAMERA_FRAME_STAT := false
CAMERA_WATCHDOG := true
$(warning "broad_target is $(TARGET_PRODUCT)")
$(warning "PLATFORM_SDK_VERSION is $(PLATFORM_SDK_VERSION)")

#gs702a,4.1 do not support FD
ifeq ($(PLATFORM_SDK_VERSION),16)
ifneq (,$(findstring gs702a,$(TARGET_PRODUCT)))
CAMERA_VCE_OMX_FD := false
endif
endif

CAMERA_HAL_SRC := \
	CameraHal_Module.cpp \
	CameraHal.cpp \
	CameraHalUtilClasses.cpp \
	AppCallbackNotifier.cpp \
	ANativeWindowDisplayAdapter.cpp \
	CameraProperties.cpp \
	MemoryManager.cpp \
	Encoder_libjpeg.cpp \
	Resize.cpp \
	Converters.cpp \
	CameraUtils.cpp \
	SensorListener.cpp  \
	CameraGpuOutstanding.cpp  \
	CameraConfigs.cpp    \
	CameraResTable.cpp    \
	CameraHWCaps.cpp    \
	CameraWatchDog.cpp    \
	MediaProfileGenerator/MediaProfileGenerator.cpp
    
	
ifeq ($(CAMERA_SET_FREQ_RANGE),true)
CAMERA_HAL_SRC+= 	CameraFreqAdapter.cpp
endif

CAMERA_COMMON_SRC:= \
	CameraParameters.cpp \
	ActCameraParameters.cpp \
	CameraHalCommon.cpp \
	libutils/ErrorUtils.cpp \
	libutils/MessageQueue.cpp \
	libutils/Semaphore.cpp 

CAMERA_OMX_SRC:= \
	BaseCameraAdapter.cpp \
	OMXCameraAdapter/OMX3A.cpp \
	OMXCameraAdapter/OMXCameraAdapter.cpp \
	OMXCameraAdapter/OMXCapabilities.cpp \
	OMXCameraAdapter/OMXCapture.cpp \
	OMXCameraAdapter/OMXDefaults.cpp \
	OMXCameraAdapter/OMXFocus.cpp \
	OMXCameraAdapter/OMXFD.cpp \
	OMXCameraAdapter/OMXZoom.cpp \
	OMXCameraAdapter/OMXExif.cpp \
	OMXCameraAdapter/OMXFlash.cpp

CAMERA_VCE_SRC:= \
	Vce/OMXVce.cpp 

CAMERA_HEAP_TRACEK_SRC:= \
	heaptracker/heaptracker.c \
	heaptracker/mapinfo.c \
	heaptracker/stacktrace.c   \
	heaptracker/newtracker.cpp
	

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	$(CAMERA_VCE_SRC) \
	$(CAMERA_HAL_SRC) \
	$(CAMERA_OMX_SRC)  \
	$(CAMERA_COMMON_SRC) 
	

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc/ \
    $(LOCAL_PATH)/inc/OMXCameraAdapter \
    $(LOCAL_PATH)/libutils \
    $(LOCAL_PATH)/Vce \
    frameworks/native/include/media/openmax \
    frameworks/base/include/ui \
    frameworks/base/include/utils \
    hardware/libhardware/include/hardware \
    external/jpeg \
    external/jhead  \
    system/media/camera/include \
    device/actions/common/hardware/omx/components/omx_vce/include \
    device/actions/common/hardware/omx/components/omx_vce/vce_resize \

ifeq ($(CAMERA_SET_FREQ_RANGE),true)
LOCAL_C_INCLUDES+= 	device/actions/common/frameworks/include
endif

LOCAL_SHARED_LIBRARIES:= \
    libui \
    libbinder \
    libutils \
    libcutils \
    libcamera_client \
    libgui \
    libjpeg \
    libjhead \
    libion  \
    libOMX_Core \
    libalc \
    libgralloc \
    libhardware \

LOCAL_STATIC_LIBRARIES := \
	libACT_VceResize \
		
	
ifeq ($(CAMERA_SET_FREQ_RANGE),true)
LOCAL_SHARED_LIBRARIES+= 	libperformance
endif
	
	

LOCAL_CFLAGS := -fno-short-enums  -DCOPY_IMAGE_BUFFER


ifeq ($(CAMERA_NEON_INS),true)
LOCAL_CFLAGS+= -mfpu=neon -DCAMERA_NEON_INS
endif

ifeq ($(CAMERA_SET_FREQ_RANGE),true)
LOCAL_CFLAGS+= 	-DCAMERA_SET_FREQ_RANGE
endif

ifeq ($(CAMERA_SET_GPUOUTSTANDING),true)
LOCAL_CFLAGS+= 	-DCAMERA_SET_GPUOUTSTANDING
endif


ifeq ($(CAMERA_VCE_OMX_FD),true)
LOCAL_CFLAGS+= -DCAMERA_VCE_OMX_FD
endif 

ifeq ($(CAMERA_VCE_OMX_JPEG),true)
LOCAL_CFLAGS+= -DCAMERA_VCE_OMX_JPEG
endif 

ifeq ($(CAMERA_VCE_OMX_RESIZE),true)
LOCAL_CFLAGS+= -DCAMERA_VCE_OMX_RESIZE
endif 

ifeq ($(CAMERA_RUN_IN_EMULATOR),true)
LOCAL_CFLAGS+= -DCAMERA_RUN_IN_EMULATOR
endif 

ifeq ($(CAMERA_DISPLAY_RGB565),true)
LOCAL_CFLAGS+= -DCAMERA_DISPLAY_RGB565
endif 

ifeq ($(CAMERA_SENSOR_LISTENER),true)
LOCAL_CFLAGS+= -DCAMERA_SENSOR_LISTENER
endif 

ifeq ($(CAMERA_ANDROID_LOG),true)
LOCAL_CFLAGS+= -DCAMERA_ANDROID_LOG
endif 

ifeq ($(CAMERA_USE_TEST_OMX),true)
LOCAL_CFLAGS+= -DCAMERA_USE_TEST_OMX
endif

ifeq ($(CAMERA_IMAGE_SNAPSHOT),true)
LOCAL_CFLAGS+= -DCAMERA_IMAGE_SNAPSHOT

ifeq ($(CAMERA_VIDEO_SNAPSHOT),true)
LOCAL_CFLAGS+= -DCAMERA_VIDEO_SNAPSHOT
endif

#endif


ifeq ($(CAMERA_FOCUS_AREA),true)
LOCAL_CFLAGS+= -DCAMERA_FOCUS_AREA
endif

ifeq ($(CAMERA_FOCUS_MODE_INFINITY),true)
LOCAL_CFLAGS+= -DCAMERA_FOCUS_MODE_INFINITY
endif

ifeq ($(CAMERA_FOCUS_MODE_AUTO),true)
LOCAL_CFLAGS+= -DCAMERA_FOCUS_MODE_AUTO
endif

ifeq ($(CAMERA_FRAME_STAT),true)
LOCAL_CFLAGS+= -DCAMERA_FRAME_STAT
endif

ifeq ($(CAMERA_WATCHDOG),true)
LOCAL_CFLAGS+= -DCAMERA_WATCHDOG
endif

ifeq ($(CAMERA_HEAPTRACKER),true)
LOCAL_SRC_FILES+=$(CAMERA_HEAP_TRACEK_SRC)
LOCAL_CFLAGS += -DCAMERA_HEAPTRACKER 
LOCAL_LDFLAGS += $(foreach f, $(strip malloc realloc calloc free), -Wl,--wrap=$(f))
endif

ifeq ($(CAMERA_FPS_FROM_OMX),true)
LOCAL_CFLAGS+= -DCAMERA_FPS_FROM_OMX
endif

#Close DCAMERA_IGNORE_STOPPREVIEW_AFTER_CAPTURE for several cts tests fail,
# eg: testBurstVideoSnapshot,testStillPreviewCombination. (liyuan) 
#LOCAL_CFLAGS += -DCAMERA_IGNORE_STOPPREVIEW_AFTER_CAPTURE

#ifneq (,$(findstring gs705a,$(TARGET_PRODUCT)))
#ifneq (,$(findstring gs705a,$(TARGET_PRODUCT)), $(findstring gs900a,$(TARGET_PRODUCT)))
#gs705a
#LOCAL_CFLAGS+= -DCAMERA_FORMAT_NV12YU12
#endif # PRODUCT_DEVICE

#ifneq (,$(findstring gs705a,$(TARGET_PRODUCT)))
LOCAL_CFLAGS+= -DCAMERA_GS705A
#endif

ifneq (,$(findstring gs900a,$(TARGET_PRODUCT)))
LOCAL_CFLAGS+= -DCAMERA_GS900A
endif

#Android 4.1 do not support video snapshot
ifeq ($(PLATFORM_SDK_VERSION),16)
LOCAL_CFLAGS += -DCAMERA_ANDROID16
LOCAL_CFLAGS+= -DCAMERA_VCEJPEG_KEEPIDLE
CAMERA_HDR := false

else
LOCAL_CFLAGS+= -DCAMERA_VIDEO_SNAPSHOT
endif

ifeq ($(CAMERA_HDR),true)
LOCAL_CFLAGS+= -DCAMERA_HDR
endif

ifeq ($(CAMERA_PROFILES_AUTOGEN),true)
LOCAL_CFLAGS+= -DCAMERA_PROFILES_AUTOGEN
endif
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS:= eng optional
LOCAL_MODULE_RELATIVE_PATH := hw
include $(BUILD_SHARED_LIBRARY)

#include $(call all-makefiles-under,$(LOCAL_PATH))

endif

