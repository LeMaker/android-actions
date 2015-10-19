# Copyright 2007-2008 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

# This finds and builds the test apk as well, so a single make does both.
include $(CLEAR_VARS)
include $(call all-makefiles-under,$(LOCAL_PATH))
