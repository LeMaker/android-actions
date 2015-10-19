LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
ifeq ($(TARGET_BOARD_PLATFORM),ATM705X)
    include $(LOCAL_PATH)/gs705a/Android.mk
else ifeq ($(TARGET_BOARD_PLATFORM),ATM900X)
    include $(LOCAL_PATH)/gs900a/Android.mk
else
		include $(LOCAL_PATH)/gs705a/Android.mk
endif
