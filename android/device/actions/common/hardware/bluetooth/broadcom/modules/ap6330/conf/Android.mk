
# Configuration
AP_BT_FIRMWARE_DIR := bluetooth
LOCAL_PATH := $(call my-dir)

ifneq ($(strip $(R_BT_USE_UART)), )
$(info R_BT_USE_UART=$(R_BT_USE_UART))

ret=$(shell grep "$(R_BT_USE_UART)" $(LOCAL_PATH)/bt_vendor.conf)
ifeq ($(strip $(ret)), )
$(info replace bt UART with $(R_BT_USE_UART) in $(LOCAL_PATH)/bt_vendor.conf)
$(shell sed -i "s/ttyS[0-9]/$(R_BT_USE_UART)/g" $(LOCAL_PATH)/bt_vendor.conf)
endif

endif

include $(CLEAR_VARS)
LOCAL_MODULE := bt_vendor.conf
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/$(AP_BT_FIRMWARE_DIR)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := bcm40183b2.hcd
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/$(AP_BT_FIRMWARE_DIR)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)
