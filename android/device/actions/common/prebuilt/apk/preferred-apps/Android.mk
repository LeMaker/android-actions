

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE :=browser.xml
LOCAL_SRC_FILES :=browser.xml
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS :=etc
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/preferred-apps
include $(BUILD_PREBUILT)

preferred_apps_PRODUCT_PACKAGES=

ifeq ($(shell [[ $(PRODUCT_PREBUILT_WEBVIEWCHROMIUM) == yes || $(strip $(R_GMS_TYPE)) == full ]] && echo true ),true)

preferred_apps_PRODUCT_PACKAGES += \
    	browser.xml 

endif


include $(CLEAR_VARS)
LOCAL_MODULE := preferred_apps_xml
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(preferred_apps_PRODUCT_PACKAGES)
include $(BUILD_PHONY_PACKAGE)
