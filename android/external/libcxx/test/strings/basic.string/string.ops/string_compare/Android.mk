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
test_makefile := external/libcxx/test/strings/basic.string/string.ops/string_compare/Android.mk

test_name := strings/basic.string/string.ops/string_compare/string
test_src := string.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.ops/string_compare/size_size_pointer
test_src := size_size_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.ops/string_compare/pointer
test_src := pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.ops/string_compare/size_size_pointer_size
test_src := size_size_pointer_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.ops/string_compare/size_size_string
test_src := size_size_string.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.ops/string_compare/size_size_string_size_size
test_src := size_size_string_size_size.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))