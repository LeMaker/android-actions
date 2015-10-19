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

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)  

libomxil_camera_files :=  \
	library_entry_point.c \
	omx_camera_source_component.c    \
	buffer_handle_mapper.cpp        \
	omx_camera_source_capabilities.c


LOCAL_C_INCLUDES := \
	hardware/libhardware/include          \
	frameworks/base/include                \
	frameworks/native/include/media/openmax \
	device/actions/common/hardware/omx   \
	device/actions/common/hardware/omx/base   \
	frameworks/av/include \
	frameworks/av/include/alsp/inc 




LOCAL_SHARED_LIBRARIES := libdl\
	libion       \
	libutils	\
	libcutils   \
	libui

LOCAL_STATIC_LIBRARIES :=       \
	libomxBellagio_base	

ifeq ($(CAMERA_ANDROID_LOG),true)
LOCAL_CFLAGS+= -DCAMERA_ANDROID_LOG
endif 

LOCAL_SRC_FILES := $(libomxil_camera_files)
LOCAL_MODULE := libOMX.Action.Video.Camera.Test
LOCAL_MODULE_TAGS := tests
include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))




