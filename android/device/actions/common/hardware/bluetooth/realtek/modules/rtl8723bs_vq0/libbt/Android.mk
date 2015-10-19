LOCAL_PATH := $(call my-dir)

ifeq ($(strip $(R_BT_TYPE)), rtl8723bs_vq0)

include $(CLEAR_VARS)

#BDROID_DIR := $(TOP_DIR)external/bluetooth/bluedroid
BDROID_DIR := $(TOP_DIR)device/actions/common/hardware/bluetooth/realtek/bluedroid/

LOCAL_SRC_FILES := \
        src/bt_vendor_rtk.c \
        src/hardware.c \
        src/userial_vendor.c \
        src/upio.c

$(info built libbt... $(BDROID_DIR))


LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include

LOCAL_SHARED_LIBRARIES := \
        libcutils


LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional eng
LOCAL_BT_MODULES +=$(LOCAL_MODULE)
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := realtek
#LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)

ifneq ($(strip $(R_BT_USE_UART)), )
$(info R_BT_USE_UART=$(R_BT_USE_UART))
ret=$(shell grep "$(R_BT_USE_UART)" $(LOCAL_PATH)/include/vnd_generic.txt)
ifeq ($(strip $(ret)), )
$(info replace bt UART with $(R_BT_USE_UART) in $(LOCAL_PATH)/include/vnd_generic.txt)
$(shell sed -i "s/ttyS[0-9]/$(R_BT_USE_UART)/g" $(LOCAL_PATH)/include/vnd_generic.txt)
endif
ret=$(shell grep "$(R_BT_USE_UART)" $(LOCAL_PATH)/include/vnd_gs705a.txt)
ifeq ($(strip $(ret)), )
$(info replace bt USE_UART with $(R_BT_USE_UART) in $(LOCAL_PATH)/include/vnd_gs705a.txt)
$(shell sed -i "s/ttyS[0-9]/$(R_BT_USE_UART)/g" $(LOCAL_PATH)/include/vnd_gs705a.txt)
endif
endif

include $(LOCAL_PATH)/vnd_buildcfg.mk

include $(BUILD_SHARED_LIBRARY)

endif # BOARD_HAVE_BLUETOOTH_RTK
