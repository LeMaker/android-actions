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
test_makefile := external/libcxx/test/strings/basic.string/string.modifiers/string_erase/Android.mk

test_name := strings/basic.string/string.modifiers/string_erase/iter_iter
test_src := iter_iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_erase/erase_iter_iter_db4
test_src := erase_iter_iter_db4.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_erase/iter
test_src := iter.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_erase/pop_back
test_src := pop_back.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_erase/erase_iter_iter_db1
test_src := erase_iter_iter_db1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_erase/erase_iter_iter_db3
test_src := erase_iter_iter_db3.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_erase/erase_iter_db2
test_src := erase_iter_db2.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_erase/size_size
test_src := size_size.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_erase/erase_iter_db1
test_src := erase_iter_db1.pass.cpp
include external/libcxx/test/Android.build.mk

test_name := strings/basic.string/string.modifiers/string_erase/erase_iter_iter_db2
test_src := erase_iter_iter_db2.pass.cpp
include external/libcxx/test/Android.build.mk

include $(call all-makefiles-under,$(LOCAL_PATH))