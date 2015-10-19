
# Configuration


LOCAL_PATH := $(call my-dir)

# applied to REALTECK8723BU



ifeq ($(strip $(R_BT_TYPE)), rtl8723bu)
BUILD_RTL8723BU  := true
endif

ifeq ($(strip $(R_BT_TYPE)), rtl8723bs)
BUILD_RTL8723BU  := true
endif

#subdirectory
include $(call first-makefiles-under,$(LOCAL_PATH))











