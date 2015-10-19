#
# Copyright (C) 2012 The Android Open Source Project
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
#

# This package provides the 'glue' layer between Chromium and WebView.

LOCAL_PATH := $(call my-dir)
CHROMIUM_PATH := external/chromium_org

# Native support library (libwebviewchromium_plat_support.so) - does NOT link
# any native chromium code.
include $(CLEAR_VARS)

LOCAL_MODULE:= libwebviewchromium_plat_support

LOCAL_SRC_FILES:= \
        plat_support/draw_gl_functor.cpp \
        plat_support/jni_entry_point.cpp \
        plat_support/graphics_utils.cpp \
        plat_support/graphic_buffer_impl.cpp \

LOCAL_C_INCLUDES:= \
        $(CHROMIUM_PATH) \
        external/skia/include/core \
        frameworks/base/core/jni/android/graphics \
        frameworks/native/include/ui \

LOCAL_SHARED_LIBRARIES += \
        libandroid_runtime \
        liblog \
        libcutils \
        libskia \
        libui \
        libutils \

LOCAL_MODULE_TAGS := optional

# To remove warnings from skia header files
LOCAL_CFLAGS := -Wno-unused-parameter

include $(BUILD_SHARED_LIBRARY)


# Loader library which handles address space reservation and relro sharing.
# Does NOT link any native chromium code.
include $(CLEAR_VARS)

LOCAL_MODULE:= libwebviewchromium_loader

LOCAL_SRC_FILES := \
        loader/loader.cpp \

LOCAL_CFLAGS := \
        -Werror \

LOCAL_SHARED_LIBRARIES += \
        libdl \
        liblog \

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


# Include the makefile for the main package unless we are using a prebuilt.
ifneq ($(PRODUCT_PREBUILT_WEBVIEWCHROMIUM),yes)
extra_java_files :=
include $(LOCAL_PATH)/package.mk
endif

# Build other stuff
include $(call first-makefiles-under,$(LOCAL_PATH))
