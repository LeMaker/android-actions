
ifeq ($(R_BT_TYPE), rtl8723bs_vq0)
LOCAL_PATH := $(call my-dir)
# Configuration
RTK_BT_FIRMWARE_DIR := firmware/rtl8723bs_vq0

include $(CLEAR_VARS)
LOCAL_MODULE := rtl8723b_VQ0_config
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/$(RTK_BT_FIRMWARE_DIR)
#LOCAL_SRC_FILES := $(LOCAL_MODULE)
LOCAL_SRC_FILES := rtl8723b_VQ0_config
include $(BUILD_PREBUILT)


include $(CLEAR_VARS)
LOCAL_MODULE := rtl8723b_fw
LOCAL_MODULE_TAGS := optional eng
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/$(RTK_BT_FIRMWARE_DIR)
LOCAL_SRC_FILES := rtl8723b_fw
include $(BUILD_PREBUILT)

endif
#subdirectory
#include $(call first-makefiles-under,$(LOCAL_PATH))
#rtl8723vq0_PRODUCT_PACKAGES += \
#	rtl8723b_VQ0_config \
#	rtl8723b_fw
#remove to device.mk	?

#include $(CLEAR_VARS)
#LOCAL_MODULE := bt_vendor
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_REQUIRED_MODULES := $(rtl8723vq0_PRODUCT_PACKAGES)
#include $(BUILD_PHONY_PACKAGE)




