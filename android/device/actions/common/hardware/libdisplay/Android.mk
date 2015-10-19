# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#ifeq ($(TARGET_BOARD_PLATFORM),ATM702X)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# our own branch needs these headers
LOCAL_C_INCLUDES += \
	device/actions/common/hardware/include/ 
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := liblog libcutils

ifneq (,$(findstring gs900a,$(TARGET_PRODUCT)))
LOCAL_SRC_FILES := display_gs900a.cpp
else
LOCAL_SRC_FILES := display.cpp
endif

LOCAL_CFLAGS:= -DLOG_TAG=\"displayengine\"
LOCAL_MODULE_TAGS :=eng optional
LOCAL_MODULE := libdisplay.$(TARGET_BOARD_PLATFORM)
include $(BUILD_SHARED_LIBRARY)

#endif
