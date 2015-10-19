
# Configuration

LOCAL_PATH := $(call my-dir)

ifeq ($(strip $(R_BT_TYPE)), rtl8723bu)
BUILD_RTL8723BS  := true
endif

ifeq ($(strip $(R_BT_TYPE)), rtl8723bs)
BUILD_RTL8723BS  := true
endif

include $(call all-subdir-makefiles)

