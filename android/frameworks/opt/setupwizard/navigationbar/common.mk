#
# This file is to be included in makefiles that use the navigation bar
# Please include this just before building the package
#

LOCAL_RESOURCE_DIR += $(call my-dir)/res
LOCAL_AAPT_FLAGS += --auto-add-overlay --extra-packages com.android.setupwizard.navigationbar
LOCAL_STATIC_JAVA_LIBRARIES += setup-wizard-navbar
