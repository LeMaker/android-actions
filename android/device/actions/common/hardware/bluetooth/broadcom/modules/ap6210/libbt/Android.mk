LOCAL_PATH := $(call my-dir)

#ifneq ($(BOARD_HAVE_BLUETOOTH_RTK),)
ifeq ($(strip $(R_BT_TYPE)), ap6210)

ifneq ($(strip $(R_BT_USE_UART)), )
$(info R_BT_USE_UART=$(R_BT_USE_UART))

ret=$(shell grep "$(R_BT_USE_UART)" $(LOCAL_PATH)/include/vnd_buildcfg.h)
ifeq ($(strip $(ret)), )
$(info replace bt UART with $(R_BT_USE_UART) in $(LOCAL_PATH)/include/vnd_buildcfg.h)
$(shell sed -i "s/ttyS[0-9]/$(R_BT_USE_UART)/g" $(LOCAL_PATH)/include/vnd_buildcfg.h)
endif

endif

include $(CLEAR_VARS)

BDROID_DIR := $(TOP_DIR)external/bluetooth/bluedroid

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
