#
# Copyright (C) 2011-2012 The Android Open Source Project
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
#

LOCAL_PATH := $(call my-dir)

#=====================================================================
# Common: libbccExecutionEngine
#=====================================================================

libbcc_executionengine_SRC_FILES := \
  ELFObjectLoaderImpl.cpp \
  GDBJIT.cpp \
  GDBJITRegistrar.cpp \
  ObjectLoader.cpp \
  SymbolResolverProxy.cpp \
  SymbolResolvers.cpp

#=====================================================================
# Device Static Library: libbccExecutionEngine
#=====================================================================
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)

LOCAL_MODULE := libbccExecutionEngine
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_C_INCLUDES := \
  $(LIBBCC_ROOT_PATH) \
  $(RSLOADER_ROOT_PATH) \
  $(RSLOADER_ROOT_PATH)/include

LOCAL_SRC_FILES := $(libbcc_executionengine_SRC_FILES)

include $(LIBBCC_DEVICE_BUILD_MK)
include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_STATIC_LIBRARY)
endif

#=====================================================================
# Host Static Library: libbccExecutionEngine
#=====================================================================

include $(CLEAR_VARS)

LOCAL_MODULE := libbccExecutionEngine
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_C_INCLUDES := \
  $(LIBBCC_ROOT_PATH) \
  $(RSLOADER_ROOT_PATH) \
  $(RSLOADER_ROOT_PATH)/include

LOCAL_SRC_FILES := $(libbcc_executionengine_SRC_FILES)

include $(LIBBCC_HOST_BUILD_MK)
include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_STATIC_LIBRARY)
