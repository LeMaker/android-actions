#
# Copyright (C) 2014 The Android Open Source Project
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
COMMON_C_INCLUDES := \
	external/libcxx/test/support \
	external/libcxx/include \

COMMON_CPPFLAGS := \
	-std=c++11 \
	-fexceptions \
	-frtti \
	-nostdinc++ \

COMMON_DEPS := \
	external/libcxx/test/Android.build.mk \
	$(test_makefile) \

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(COMMON_DEPS)
LOCAL_CLANG := true
LOCAL_C_INCLUDES := $(COMMON_C_INCLUDES)
LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS)
LOCAL_SHARED_LIBRARIES := libc++
LOCAL_SYSTEM_SHARED_LIBRARIES := libc libm
LOCAL_MODULE := libc++tests/$(test_name)
LOCAL_SRC_FILES := $(test_src)
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(COMMON_DEPS)
LOCAL_CLANG := true
LOCAL_C_INCLUDES := $(COMMON_C_INCLUDES)
LOCAL_CPPFLAGS := $(COMMON_CPPFLAGS)
LOCAL_LDFLAGS := -nodefaultlibs
LOCAL_LDLIBS := -lc -lm -lpthread
LOCAL_SHARED_LIBRARIES := libc++
LOCAL_MODULE := libc++tests/$(test_name)
LOCAL_SRC_FILES := $(test_src)
include $(BUILD_HOST_EXECUTABLE)
