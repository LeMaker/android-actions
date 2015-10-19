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

omx_cameratest_files :=  \
	surface_display.cpp \
	yuv_to_rgb.cpp \
	main.c \
    allocbuftest.c \
	usebuftest.c \
	bufhandletest.c \
	buffer_handle.cpp


LOCAL_C_INCLUDES :=  \
	hardware/libhardware/include          \
	frameworks/base/include               \
	frameworks/base/native/include        \
	frameworks/native/include/media/openmax \
	device/actions/common/hardware/omx   \
	device/actions/common/hardware/omx/base   


LOCAL_SHARED_LIBRARIES :=  libOMX_Core\
	libcutils \
	libutils \
	libui \
	libgui \
	libbinder 

LOCAL_STATIC_LIBRARIES :=       \
	libomxBellagio_base	

LOCAL_SRC_FILES := $(omx_cameratest_files)
LOCAL_MODULE := cameraomxtest_test
LOCAL_MODULE_TAGS := tests
include $(BUILD_EXECUTABLE)




