LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	bootanimation_main.cpp \
	AudioPlayer.cpp \
	BootAnimation.cpp

LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

LOCAL_C_INCLUDES += external/tinyalsa/include

#Actionscode(liuxinxu, new_feature)
#import libmedia

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog \
	libandroidfw \
	libutils \
	libbinder \
	libui \
	libskia \
	libEGL \
	libGLESv1_CM \
	libgui \
	libmedia \
	libtinyalsa

LOCAL_MODULE:= bootanimation
LOCAL_ADDITIONAL_DEPENDENCIES :=bootanimation_warn.zip

ifdef TARGET_32_BIT_SURFACEFLINGER
LOCAL_32_BIT_ONLY := true
endif

include $(BUILD_EXECUTABLE)

# check lcd refresh rate
include $(CLEAR_VARS)
LOCAL_MODULE :=bootanimation_warn.zip
LOCAL_SRC_FILES :=bootanimation_warn.zip
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE_CLASS :=ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/media/
include $(BUILD_PREBUILT)