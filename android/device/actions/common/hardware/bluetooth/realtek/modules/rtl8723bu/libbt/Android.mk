LOCAL_PATH := $(call my-dir)


ifeq ($(BUILD_RTL8723BU), true)

include $(CLEAR_VARS)

#BDROID_DIR := $(TOP_DIR)external/bluetooth/bluedroid
BDROID_DIR := $(TOP_DIR)device/actions/common/hardware/bluetooth/realtek/bluedroid/

LOCAL_SRC_FILES := \
        src/bt_vendor_rtk.c 

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include

LOCAL_SHARED_LIBRARIES := \
        libcutils


LOCAL_MODULE := libbt-vendor-rtl8723bu
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := realtek
#LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)
#LOCAL_PROPRIETARY_MODULE := true

#include $(LOCAL_PATH)/vnd_buildcfg.mk
include $(BUILD_SHARED_LIBRARY)

endif # BOARD_HAVE_BLUETOOTH_RTK
