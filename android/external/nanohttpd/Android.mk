LOCAL_PATH := $(call my-dir)

# This module target includes just the single core file: NanoHTTPD.java, which
# is enough for HTTP 1.1 support and nothing else.

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, core/src/main)
LOCAL_MODULE := libnanohttpd
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_JAVA_LIBRARY)
