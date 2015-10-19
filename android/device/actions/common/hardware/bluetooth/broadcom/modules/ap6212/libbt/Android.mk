LOCAL_PATH := $(call my-dir)

#ifneq ($(BOARD_HAVE_BLUETOOTH_RTK),)
ifeq ($(strip $(R_BT_TYPE)), ap6212)

include $(CLEAR_VARS)

BDROID_DIR := $(TOP_DIR)external/bluetooth/bluedroid

#ifeq ($(R_WIRELESS_SLEEP),true)
#$(info "R_WIRELESS_SLEEP define BT_USE_LPM_SLEEP")
LOCAL_CFLAGS += -DBT_USE_LPM_SLEEP
#endif

LOCAL_SRC_FILES := \
        src/bt_vendor_brcm.c \
        src/hardware.c \
        src/userial_vendor.c \
        src/upio.c \
        src/conf.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog

LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := broadcom
#LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)
#LOCAL_PROPRIETARY_MODULE := true

#include $(LOCAL_PATH)/vnd_buildcfg.mk

include $(BUILD_SHARED_LIBRARY)

endif # BOARD_HAVE_BLUETOOTH_RTK
