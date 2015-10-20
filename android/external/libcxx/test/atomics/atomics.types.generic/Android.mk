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
LOCAL_PATH := $(call my-dir)
test_makefile := external/libcxx/test/atomics/atomics.types.generic/Android.mk

test_name := atomics/atomics.types.generic/address
test_src := address.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.generic/bool
test_src := bool.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.generic/cstdint_typedefs
test_src := cstdint_typedefs.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.generic/integral_typedefs
test_src := integral_typedefs.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := atomics/atomics.types.generic/integral
test_src := integral.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))