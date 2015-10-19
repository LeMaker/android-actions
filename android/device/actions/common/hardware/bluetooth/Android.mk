
# Configuration

LOCAL_PATH := $(call my-dir)


# applied to RTL8723
ifeq ($(strip $(R_BT_TYPE)), rtl8723as)
BUILD_RTL8723AS  := true
endif

ifeq ($(strip $(R_BT_TYPE)), rtl8723bs)
BUILD_RTL8723BS_RTL8723BU  := true
endif

ifeq ($(strip $(R_BT_TYPE)), rtl8723bs_vq0)
BUILD_RTL8723BS_VQ0  := true
endif

ifeq ($(strip $(R_BT_TYPE)), rtl8723au)
BUILD_RTL8723AU  := true
endif

ifeq ($(strip $(R_BT_TYPE)), rtl8723bu)
BUILD_RTL8723BS_RTL8723BU  := true
endif

# applied to AP6210
ifeq ($(strip $(R_BT_TYPE)), ap6210)
BUILD_AP6210  := true
endif

# applied to AP6212
ifeq ($(strip $(R_BT_TYPE)), ap6212)
BUILD_AP6212  := true
endif

# applied to AP6476
ifeq ($(strip $(R_BT_TYPE)), ap6476)
BUILD_AP6476  := true
endif

# applied to AP6330
ifeq ($(strip $(R_BT_TYPE)), ap6330)
BUILD_AP6330  := true
endif


ifeq ($(BUILD_RTL8723AS), true)
#subdirectory
#include $(call first-makefiles-under,$(LOCAL_PATH)/rtl8723AS)
include $(LOCAL_PATH)/realtek/modules/rtl8723as/Android.mk
endif

ifeq ($(BUILD_RTL8723BS_VQ0), true)
#subdirectory
#include $(call first-makefiles-under,$(LOCAL_PATH)/rtl8723bs_vq0)
$(info   prepare build bluetooth rtl8723bs_vq0)
include $(LOCAL_PATH)/realtek/modules/rtl8723bs_vq0/Android.mk
include $(TOP_DIR)device/actions/common/hardware/bluetooth/realtek/bluedroid/Android.mk
endif

ifeq ($(BUILD_RTL8723AU), true)
#subdirectory
#include $(call first-makefiles-under,$(LOCAL_PATH)/rtl8723au)
include $(LOCAL_PATH)/realtek/modules/rtl8723au/Android.mk
endif

ifeq ($(BUILD_RTL8723BS_RTL8723BU), true)
#subdirectory
#include $(call first-makefiles-under,$(LOCAL_PATH)/rtl8723bu)
include $(TOP_DIR)device/actions/common/hardware/bluetooth/realtek/modules/rtl8723bu/Android.mk
include $(TOP_DIR)device/actions/common/hardware/bluetooth/realtek/modules/rtl8723bs/Android.mk
include $(TOP_DIR)device/actions/common/hardware/bluetooth/realtek/bluedroid/Android.mk
endif

ifeq ($(BUILD_AP6210), true)
#subdirectory
include $(LOCAL_PATH)/broadcom/modules/ap6210/Android.mk
endif

ifeq ($(BUILD_AP6212), true)
#subdirectory
include $(LOCAL_PATH)/broadcom/modules/ap6212/Android.mk
endif

ifeq ($(BUILD_AP6476), true)
#subdirectory
include $(LOCAL_PATH)/broadcom/modules/ap6476/Android.mk
include $(TOP_DIR)device/actions/common/hardware/bluetooth/broadcom/bluedroid_ap6476/Android.mk
endif

ifeq ($(BUILD_AP6330), true)
#subdirectory
#include $(call first-makefiles-under,$(LOCAL_PATH)/ap6330)
include $(LOCAL_PATH)/broadcom/modules/ap6330/Android.mk
endif
