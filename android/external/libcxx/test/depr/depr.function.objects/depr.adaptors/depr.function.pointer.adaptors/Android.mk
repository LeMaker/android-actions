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
test_makefile := external/libcxx/test/depr/depr.function.objects/depr.adaptors/depr.function.pointer.adaptors/Android.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.function.pointer.adaptors/ptr_fun1
test_src := ptr_fun1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.function.pointer.adaptors/pointer_to_binary_function
test_src := pointer_to_binary_function.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.function.pointer.adaptors/ptr_fun2
test_src := ptr_fun2.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := depr/depr.function.objects/depr.adaptors/depr.function.pointer.adaptors/pointer_to_unary_function
test_src := pointer_to_unary_function.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))