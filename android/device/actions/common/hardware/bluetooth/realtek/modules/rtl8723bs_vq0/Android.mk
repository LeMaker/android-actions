
# Configuration

LOCAL_PATH := $(call my-dir)

#ifeq ($(strip $(R_BT_TYPE)), rtl8723bs_vq0)
#BUILD_RTL8723BS_VQ0  := true
#endif

#LOCAL_BT_MODULES :=
ifeq ($(R_BT_TYPE), rtl8723bs_vq0)
	include $(call all-subdir-makefiles)
endif

#include $(CLEAR_VARS)
#LOCAL_MODULE := $(R_BT_TYPE)_bt_rtl
#LOCAL_MODULE_TAGS := eng optional
#LOCAL_REQUIRED_MODULES := $(LOCAL_BT_MODULES)
#include $(BUILD_PHONY_PACKAGE)

#LOCAL_BT_MODULES :=


