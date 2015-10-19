ifeq ($(strip $(ACTIONS_RELEASE_BUILD_FOR_CUSTOM)), false)

LOCAL_PATH:= $(call my-dir)

$(warning "PRODUCT_PREBUILT_CAMERA="$(PRODUCT_PREBUILT_CAMERA))
ifneq ($(PRODUCT_PREBUILT_CAMERA),yes)


# TinyPlanet
include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_LDFLAGS   := -llog -ljnigraphics
LOCAL_SDK_VERSION := 9
LOCAL_MODULE    := libjni_action_tinyplanet
LOCAL_SRC_FILES := tinyplanet.cc

LOCAL_CFLAGS    += -ffast-math -O3 -funroll-loops
LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)


endif

endif