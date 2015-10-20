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
test_makefile := external/libcxx/test/strings/basic.string/string.modifiers/string_insert/Android.mk

test_name := strings/basic.string/string.modifiers/string_insert/size_pointer
test_src := size_pointer.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_insert/size_string_size_size
test_src := size_string_size_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_insert/size_pointer_size
test_src := size_pointer_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_insert/iter_iter_iter
test_src := iter_iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_insert/iter_initializer_list
test_src := iter_initializer_list.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_insert/iter_char
test_src := iter_char.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_insert/size_string
test_src := size_string.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_insert/iter_size_char
test_src := iter_size_char.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_insert/size_size_char
test_src := size_size_char.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))